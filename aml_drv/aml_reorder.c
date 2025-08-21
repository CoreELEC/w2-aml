/**
 ****************************************************************************************
 *
 * @file aml_reorder.c
 *
 * Copyright (C) Amlogic 2012-2024
 *
 ****************************************************************************************
 */

#define AML_MODULE                  REO

#include <linux/bitmap.h>
#include <linux/ieee80211.h>
#include <net/ip.h>

#include "aml_defs.h"
#include "aml_utils.h"
#include "aml_compat.h"

#ifndef IEEE80211_MAX_AMPDU_BUF_HT
#define IEEE80211_MAX_AMPDU_BUF_HT  0x40
#endif

#define AML_REO_DUMP_INTERVAL   usecs_to_jiffies(10 * USEC_PER_SEC)

static u16 aml_reo_timeout_per_tid[IEEE80211_NUM_UPS] = AML_REO_TIMEOUT_TU_DEFAULT;

static inline u32 aml_rx_ip_signature(struct iphdr *iphdr)
{
    u32 sign = (u32)ntohs(iphdr->id) << 16;
    void *hdr = (void *)iphdr + (iphdr->ihl << 2);

    if (iphdr->protocol == IPPROTO_UDP)
        sign |= ntohs(((struct udphdr *)hdr)->check);
    else if (iphdr->protocol == IPPROTO_TCP)
        sign |= ntohs(((struct tcphdr *)hdr)->check);

    return sign;
}

/* return the first TCP/UDP msdu's IPv4 identification(high 16-bit) and TCP/UDP checksum(low 16-bit) */
static inline u32 aml_rx_skb_signature(struct sk_buff *skb)
{
    struct aml_skb_rxcb *rxcb;
    struct ethhdr *eth;

    if (!skb)
        return 0;

    rxcb = AML_SKB_RXCB(skb);

    if (rxcb->amsdu) {
        skb = skb_peek(&rxcb->amsdu->msdus);
        if (!skb)
            return 0;
    }

    eth = (struct ethhdr *)skb->data;
    if (eth->h_proto == htons(ETH_P_IP)) {
        return aml_rx_ip_signature((struct iphdr *)(eth + 1));
    }
    return 0;
}

static inline long aml_reo_evt_msec(struct aml_reo_evt_record *rec)
{
    long delta = jiffies - rec->ts;

    return (rec->sn < 0 || delta < 0) ? -1 : jiffies_to_msecs(delta);
}

static inline void aml_reo_evt_set(struct aml_reo_evt_record *rec, s16 sn)
{
    rec->sn = sn;
    rec->ts = jiffies;
}

static void aml_reo_dump(struct aml_reo_session *reo)
{
    reo->stats.next_dump = jiffies + AML_REO_DUMP_INTERVAL;

    AML_INFO("sta %d tid %d [%4d ~ %4d] %2d/%2d: miss %u, dup %u, "
             "stale %u, out_win %u, pn_err %u, timeout %u, rx %u (lpn %llu)\n",
             reo->sta_id, reo->tid, reo->ssn, reo->lsn,
             reo->stats.in_buffer, ieee80211_sn_sub(reo->lsn, reo->ssn),
             reo->stats.miss, reo->stats.dup, reo->stats.stale, reo->stats.out_win,
             reo->stats.pn_err, reo->stats.timeout, reo->stats.rx, reo->pn);
}

static inline bool aml_reo_has_buffer(struct aml_reo_session *reo)
{
    return reo->lsn != reo->ssn;
}

static inline int aml_reo_entry_index(struct aml_reo_session *reo, u16 sn)
{
    return sn & (reo->win_size - 1);
}

static inline void aml_reo_entry_skb_set(struct aml_reo_session *reo, int index, struct sk_buff *skb)
{
    reo->buf[index].skb = skb;
    reo->buf[index].deadline = jiffies + reo->timeout_ticks;
}

static inline bool aml_reo_entry_expired(struct aml_reo_session *reo, int index, unsigned long now)
{
    struct sk_buff *skb = reo->buf[index].skb;
    unsigned long deadline = reo->buf[index].deadline;

    if (skb && time_after(now, reo->buf[index].deadline)) {
        ++reo->stats.timeout;
        AML_RLMT_WARN("skb %8p @ %3d: expired %lu > %lu!\n", skb, index, now, deadline);
        return true;
    }
    return false;
}

/* return the next sn of the last expired frame */
static inline int aml_reo_entry_expired_lsn(struct aml_reo_session *reo, unsigned long now)
{
    int head = aml_reo_entry_index(reo, reo->ssn);
    int index = aml_reo_entry_index(reo, reo->lsn);

    if (index < head) {
        for (; index >= 0; index--) {
            if (aml_reo_entry_expired(reo, index, now))
                return ieee80211_sn_add(reo->ssn, reo->win_size + index - head + 1);
        }
        index = reo->win_size - 1;
    }
    for (; index > head; index--) {
        if (aml_reo_entry_expired(reo, index, now))
            return ieee80211_sn_add(reo->ssn, index - head + 1);
    }
    return -1;
}

static inline int aml_reo_pn_check(struct aml_reo_session *reo, struct aml_rhd_ext *hd)
{
    /* auto-learning */
    if (hd->pn_present)
        reo->pn_check = 1;

    if (reo->pn_check) {
        if (!hd->pn_present || reo->pn >= hd->pn[0]) {
            ++reo->stats.pn_err;
            AML_RLMT_ERR("PN error! sta %d, tid %d, pn_present %d, pn %llu, last %llu\n",
                         reo->sta_id, reo->tid,
                         hd->pn_present, (unsigned long long)hd->pn[0], reo->pn);
            return -1;
        }
        reo->pn = hd->pn[0];
    }
    return 0;
}

static int aml_reo_dequeue(struct aml_reo_session *reo, struct sk_buff_head *frames,
                           struct sk_buff *skb, int index)
{
    BUG_ON(!frames);
    lockdep_assert_held(&reo->lock);

    if (aml_reo_pn_check(reo, AML_RHD_EXT(skb))) {
        aml_mpdu_free(skb);
        return -1;
    }

    set_bit(index, reo->delivered);
    __skb_queue_tail(frames, skb);
    return 0;
}

static void aml_reo_shift_to(struct aml_reo_session *reo, u16 ssn, struct sk_buff_head *frames)
{
    u16 sn;

    /* shift ssn to new one and dequeue continuous frame(s) */
    AML_DBG("shift to %4d [%4d ~ %4d]\n", ssn, reo->ssn, reo->lsn);
    for (sn = reo->ssn; ; sn = ieee80211_sn_inc(sn)) {
        int index = aml_reo_entry_index(reo, sn);
        struct sk_buff *skb = reo->buf[index].skb;

        if (skb) {
            --reo->stats.in_buffer;
            reo->buf[index].skb = NULL;
            if (aml_reo_dequeue(reo, frames, skb, index) == 0) {
                AML_DBG("skb %8p @ %3d: pop!\n", skb, index);
                continue;
            }
            /* PN check failed, try to wait for the correct one if sn < ssn */
        }

        /* no skb or its PN is wrong */
        if (!ieee80211_sn_less(sn, ssn))
            break;

        ++reo->stats.miss;
        clear_bit(index, reo->delivered);
        AML_RLMT_WARN("shift to %4d [%4d ~ %4d], lost sn %d @ %d\n",
                      ssn, reo->ssn, reo->lsn, sn, index);
    }

    reo->ssn = sn;
    if (ieee80211_sn_less(reo->lsn, sn))
        reo->lsn = sn;

    if (!aml_reo_has_buffer(reo))
        list_del_init(&reo->aging_entry);
}

static inline void aml_reo_entry_aging(struct aml_reo_session *reo, struct sk_buff_head *frames)
{
    unsigned long now = jiffies;

    BUG_ON(!aml_reo_has_buffer(reo));

    if (reo->last_aging != now) {
        int ssn = aml_reo_entry_expired_lsn(reo, now);

        if (ssn >= 0) {
            BUG_ON(ssn == reo->ssn);
            aml_reo_evt_set(&reo->last.timeout, ssn);
            aml_reo_shift_to(reo, ssn, frames);
        }
        reo->last_aging = now;
    }
}

void aml_reo_enqueue(struct aml_reo_session *reo, struct sk_buff *skb, struct sk_buff_head *frames)
{
    struct aml_rhd_ext *ext = AML_RHD_EXT(skb);
    u16 sn = ext->sn;
    int index = aml_reo_entry_index(reo, sn);

    lockdep_assert_held(&reo->lock);

    ++reo->stats.rx;
    reo->last_rx = jiffies;

    AML_DBG("skb %8p sn %d index %d sta %d tid %d [%4d ~ %4d]\n",
            skb, sn, index, reo->sta_id, reo->tid, reo->ssn, reo->lsn);

    /* the first frame after power resume or recovery? */
    if (reo->reset) {
        reo->reset = 0;

        BUG_ON(aml_reo_has_buffer(reo));    /* must be empty */

        /*
         * during power suspend or recovery, frames of this session may be dropped by firmware.
         * reset the session with the first frame's sn.
         */
        reo->ssn = reo->lsn = sn;
    }

    /* frame with out of date sequence number */
    if (ieee80211_sn_less(sn, reo->ssn)) {
        bool dup = test_bit(index, reo->delivered) &&
                   ieee80211_sn_sub(reo->ssn, sn) <= reo->win_size;

        if (dup)
            ++reo->stats.dup;
        else
            ++reo->stats.stale;
        AML_RLMT_WARN("drop %s frame (ip.id|checksum %x)! sn %d < ssn %d. "
                      "last bar/out_win/timeout %d/%d/%d, %ld/%ld/%ldms\n",
                      dup ? "duplicated" : "staled", aml_rx_skb_signature(skb), sn, reo->ssn,
                      reo->last.bar.sn, reo->last.out_win.sn, reo->last.timeout.sn,
                      aml_reo_evt_msec(&reo->last.bar),
                      aml_reo_evt_msec(&reo->last.out_win),
                      aml_reo_evt_msec(&reo->last.timeout));
        aml_mpdu_free(skb);

        /* don't aging for this case */
        return;
    }

    /* If the sequence number exceeds our buffering window size */
    if (!ieee80211_sn_less(sn, reo->ssn + reo->buf_size)) {
        u16 ssn = ieee80211_sn_sub(sn, reo->buf_size - 1);

        AML_DBG("shift to %4d for sn %4d [%4d ~ %4d]\n", ssn, sn, reo->ssn, reo->lsn);
        ++reo->stats.out_win;
        aml_reo_evt_set(&reo->last.out_win, sn);
        aml_reo_shift_to(reo, ssn, frames);
    }

    /* Now the new frame is always in the range of the reordering buffer */

    /* check if we already stored this frame */
    if (reo->buf[index].skb) {
        struct sk_buff *skb0 = reo->buf[index].skb;

        ++reo->stats.dup;
        AML_RLMT_WARN("skb %8p(%d) already exists at sn %d, index %d, new skb %8p(%d).\n",
                      skb0, skb0->len, sn, index, skb, skb->len);
        if (reo->pn_check || ext->pn_present) {
            struct aml_rhd_ext *ext0 = AML_RHD_EXT(skb0);

            if (!ext0->pn_present || ext0->pn[0] < ext->pn[0]) {
                AML_RLMT_WARN("replace original skb %8p(%d) because pn %lld < %lld.\n",
                              skb0,
                              skb0->len,
                              (unsigned long long)ext0->pn[0],
                              (unsigned long long)ext->pn[0]);
                aml_reo_entry_skb_set(reo, index, skb);
                skb = skb0;
            }
        }
        aml_mpdu_free(skb);
    } else if (sn == reo->ssn) {
        if (aml_reo_dequeue(reo, frames, skb, index) == 0) {
            AML_DBG("skb %8p sn %d index %d just in order\n", skb, sn, index);

            /* try to dequeue followed continuous mpdu(s) */
            reo->ssn = sn = ieee80211_sn_inc(sn);
            aml_reo_shift_to(reo, sn, frames);
        }
    } else {
        /* buffer the frame */
        ++reo->stats.in_buffer;
        aml_reo_entry_skb_set(reo, index, skb);
        AML_DBG("skb %8p sn %d index %d out of order\n", skb, sn, index);
        if (ieee80211_sn_less(reo->lsn, sn))
            reo->lsn = sn;
        if (reo->stats.in_buffer == 1)
            aml_reo_session_add_to_aging_list(reo);
    }
}

static void aml_reo_reset_ssn(struct aml_reo_session *reo, u16 ssn)
{
    int n = ieee80211_sn_sub(ssn, reo->lsn);

    if (n >= reo->win_size) {
        bitmap_clear(reo->delivered, 0, reo->win_size);
    } else {
        int start = aml_reo_entry_index(reo, reo->lsn);
        int end = aml_reo_entry_index(reo, ssn);

        if (start > end) {
            bitmap_clear(reo->delivered, start, reo->win_size - start);
            start = 0;
        }
        if (end > start)
            bitmap_clear(reo->delivered, start, end - start);
    }
    reo->ssn = reo->lsn = ssn;
}

static void aml_reo_forward_to(struct aml_rx *rx, struct aml_reo_session *reo, u16 ssn)
{
    struct sk_buff_head frames;

    __skb_queue_head_init(&frames);

    lockdep_assert_held(&reo->lock);

    aml_reo_evt_set(&reo->last.bar, ssn);
    if (ssn == reo->ssn) {
        /* nothing to do */
    } else if (!aml_reo_has_buffer(reo)) {
        /* nothing is buffered */
        aml_reo_reset_ssn(reo, ssn);
    } else if (!ieee80211_sn_less(reo->ssn, ssn) && !ieee80211_sn_less(reo->lsn, ssn)) {
        /* ssn is in buffer range (reo->ssn < ssn <= reo->lsn) */
        aml_reo_shift_to(reo, ssn, &frames);
    } else {
        /* forward all buffered frame and reset ssn/lsn */
        aml_reo_shift_to(reo, ieee80211_sn_inc(reo->lsn), &frames);
        aml_reo_reset_ssn(reo, ssn);
    }
    aml_reo_session_put(reo);

    aml_reo_forward(rx, &frames);
}

static inline void aml_reo_forward_all(struct aml_rx *rx, struct aml_reo_session *reo)
{
    spin_lock(&reo->lock);
    aml_reo_forward_to(rx, reo, reo->lsn);
}

int aml_reo_bar_process(struct aml_rx *rx, struct aml_sta *sta, struct sk_buff *skb)
{
#define IEEE80211_BAR_CTRL_GCR_MASK             (BIT(4) | BIT(3))

#define IEEE80211_BAR_CTRL_TYPE_MASK            (IEEE80211_BAR_CTRL_MULTI_TID | \
                                                 IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA)
#define IEEE80211_BAR_CTRL_TYPE_RESERVED        0
#define IEEE80211_BAR_CTRL_TYPE_COMPRESSED      IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA
#define IEEE80211_BAR_CTRL_TYPE_EXT_COMPRESSED  IEEE80211_BAR_CTRL_MULTI_TID
#define IEEE80211_BAR_CTRL_TYPE_MULTI_TID       IEEE80211_BAR_CTRL_TYPE_MASK

    struct per_tid {
        __le16 tid_info;
        __le16 start_seq_num;
    } __packed;

    struct ieee80211_bar *bar = (struct ieee80211_bar *)skb->data;
    int len = skb->len - offsetof(struct ieee80211_bar, start_seq_num);

    if (sta && len >= sizeof(bar->start_seq_num)) {
        struct per_tid *end = (void *)(skb->data + skb->len);
        struct per_tid *per = (void *)&bar->control;    /* if non-MULTI_TID */
        u16 control = le16_to_cpu(bar->control);

        if (control & IEEE80211_BAR_CTRL_GCR_MASK) {
            AML_RLMT_WARN("ignore the GCR BAR! [%d] %*ph\n", skb->len, (int)skb->len, skb->data);
            return -1;
        }

        switch (control & IEEE80211_BAR_CTRL_TYPE_MASK) {
        case IEEE80211_BAR_CTRL_TYPE_RESERVED:
        default:
            goto malformed;
        case IEEE80211_BAR_CTRL_TYPE_COMPRESSED:
        case IEEE80211_BAR_CTRL_TYPE_EXT_COMPRESSED:
            if (skb->len != sizeof(*bar))
                goto malformed;
            break;
        case IEEE80211_BAR_CTRL_TYPE_MULTI_TID:
            if ((len % sizeof(struct per_tid)) != 0 ||
                len > (sizeof(struct per_tid) * IEEE80211_NUM_UPS))
                goto malformed;
            per = (void *)&bar->start_seq_num;
            break;
        }

        AML_INFO("BAR: sta %u [%d] %*ph\n", sta->sta_idx, skb->len, (int)skb->len, skb->data);

        for (; per < end; per++) {
            struct aml_reo_session *reo;
            u8 tid = (le16_to_cpu(per->tid_info) & IEEE80211_BAR_CTRL_TID_INFO_MASK)
                            >> IEEE80211_BAR_CTRL_TID_INFO_SHIFT;

            if (tid >= IEEE80211_NUM_UPS)
                goto malformed;

            reo = aml_reo_session_get(sta, tid);
            if (reo)
                aml_reo_forward_to(rx, reo,
                                   IEEE80211_SEQ_TO_SN(le16_to_cpu(per->start_seq_num)));
            else
                AML_RLMT_WARN("no REO session! sta %u tid %u\n", sta->sta_idx, tid);
        }
        return 0;
    }
malformed:
    AML_RLMT_WARN("malformed BAR [%d] %*ph\n", skb->len, (int)skb->len, skb->data);
    return -1;
}

void aml_reo_aging(struct aml_reo_aging *reo_aging, struct sk_buff_head *frames)
{
    struct aml_reo_session *reo, *next;

    list_for_each_entry_safe(reo, next, &reo_aging->list, aging_entry) {
        spin_lock(&reo->lock);
        aml_reo_entry_aging(reo, frames);
        spin_unlock(&reo->lock);
    }
}

static void aml_reo_session_release(struct aml_rx *rx, struct aml_sta *sta, u8 tid)
{
    struct aml_reo_session *reo = sta->reos[tid];

    if (WARN_ON(tid >= ARRAY_SIZE(sta->reos)))
        return;

    if (!reo)
        return;

    AML_INFO("sta %u tid %u\n", sta->sta_idx, tid);

    sta->reos[tid] = NULL;  /* firstly detach it from station */

    list_del_init(&reo->aging_entry);
    aml_reo_forward_all(rx, reo);
    aml_reo_dump(reo);

    kfree(reo);
}

struct aml_reo_session *aml_reo_session_get(struct aml_sta *sta, u8 tid)
{
    struct aml_reo_session *reo;

    if (!sta)
        return NULL;

    if (WARN_ON(tid >= ARRAY_SIZE(sta->reos)))
        return NULL;

    reo = sta->reos[tid];
    if (!reo)
        return NULL;

    spin_lock(&reo->lock);

    return reo;
}

/* forward all buffered frames and set reset flag to all reorder sessions */
void aml_reo_reset_all(struct aml_rx *rx)
{
    int sta_id;
    u8 tid;
    struct aml_sta *sta = aml_rx2hw(rx)->sta_table;

    for (sta_id = 0; sta_id < NX_REMOTE_STA_MAX; sta_id++, sta++) {
        if (!sta->valid)
            continue;

        for (tid = 0; tid < ARRAY_SIZE(sta->reos); tid++) {
            struct aml_reo_session *reo = sta->reos[tid];

            if (reo) {
                reo->reset = 1;
                aml_reo_forward_all(rx, reo);
            }
        }
    }
}

int aml_reo_session_timeout_set(struct aml_reo_session *reo, u16 tu)
{
    if (tu < AML_REO_TIMEOUT_TU_MIN || tu > AML_REO_TIMEOUT_TU_MAX) {
        AML_ERR("sta %u, tid %u, invalid timeout %u!\n", reo->sta_id, reo->tid, tu);
        return -1;
    }

    reo->timeout_tu = tu;
    reo->timeout_ticks = usecs_to_jiffies(reo->timeout_tu << 10);
    if (!reo->timeout_ticks)
        reo->timeout_ticks = 1;
    return 0;
}

int aml_reo_session_create(struct aml_rx *rx, struct aml_sta *sta, u8 tid, u16 sz, u16 ssn)
{
    struct aml_reo_session *reo;
    u16 max_buf_sz;
    u16 win_size = 1;

    /* find the nearest cache window size */
    while (win_size < sz)
        win_size <<= 1;
    BUG_ON((win_size >> 1) >= sz);
    BUG_ON(win_size > IEEE80211_SN_MODULO);

    if (!sta)
        goto del_ba;

    AML_RLMT_NOTICE("%pM: tid %u ssn %u sz %u(%u)\n", sta->mac_addr, tid, ssn, sz, win_size);

    if (WARN_ON(tid >= ARRAY_SIZE(sta->reos)))
        goto del_ba;
    if (WARN_ON(ssn > IEEE80211_MAX_SN))
        goto del_ba;

    max_buf_sz = sta->he ? IEEE80211_MAX_AMPDU_BUF : IEEE80211_MAX_AMPDU_BUF_HT;
    if (WARN_ON(sz < 1))
        sz = 1;
    else if (WARN_ON(sz > max_buf_sz))
        sz = max_buf_sz;

    reo = sta->reos[tid];
    if (reo) {
        /*
         * if AP creates a BA session, but don't send frame(s) under BA (normal ACK),
         * then AP will "re-ADDBA" to shift SSN without DELBA prior.
         */
        if (reo->buf_size == sz) {
            spin_lock(&reo->lock);
            aml_reo_forward_to(rx, reo, ssn);
            return 0;
        }
        AML_RLMT_WARN("a new reo session will be rebuilt! new buffer size %d != %d\n",
                      sz, reo->buf_size);
        aml_reo_session_release(rx, sta, tid);
    }

    reo = kzalloc(sizeof(struct aml_reo_session)
                  + sizeof(reo->buf[0]) * win_size
                  + sizeof(reo->delivered[0]) * BITS_TO_LONGS(win_size),
                  in_atomic() ? GFP_ATOMIC: GFP_KERNEL);
    if (!reo) {
        AML_RLMT_ERR("no memory for reo_session (buffer size %d)!\n", sz);
        goto del_ba;
    }

    /* coverity[USELESS_CALL], spinlock_check(_lock); */
    spin_lock_init(&reo->lock);
    reo->rx = rx;
    reo->sta_id = sta->sta_idx;
    reo->tid = tid;
    reo->buf_size = sz;
    reo->win_size = win_size;
    reo->ssn = ssn;
    reo->lsn = ssn;

    /* set the proper pn_check according to sta->key.hw_idx instead of auto-learning */
    reo->pn_check = 0;

    aml_reo_session_timeout_set(reo, aml_reo_timeout_per_tid[tid]);
    INIT_LIST_HEAD(&reo->aging_entry);
    reo->last_rx = jiffies;
    reo->last_aging = jiffies;

    aml_reo_evt_set(&reo->last.bar, -1);
    aml_reo_evt_set(&reo->last.out_win, -1);
    aml_reo_evt_set(&reo->last.timeout, -1);

    reo->stats.next_dump = jiffies + AML_REO_DUMP_INTERVAL;

    reo->delivered = (unsigned long *)&reo->buf[win_size];

    sta->reos[tid] = reo;

    return 0;

del_ba:
    AML_RLMT_ERR("FIXME: delete BA (sta %p)!\n", sta);
    return -1;
}

int aml_reo_session_delete(struct aml_rx *rx, struct aml_sta *sta, u8 tid)
{
    if (!sta) {
        AML_RLMT_ERR("no sta!\n");
        return -1;
    }

    AML_RLMT_NOTICE("sta %u tid %u\n", sta->sta_idx, tid);
    aml_reo_session_release(rx, sta, tid);

    return 0;
}

void aml_reo_sta_deinit(struct aml_rx *rx, struct aml_sta *aml_sta)
{
    u8 tid;

    for (tid = 0; tid < ARRAY_SIZE(aml_sta->reos); tid++)
        aml_reo_session_release(rx, aml_sta, tid);
}

void aml_reo_sta_addkey(struct aml_sta *sta)
{
    int i = 0;
    struct aml_reo_session *reo = NULL;

    if (!sta)
        return;

    for (i = 0; i < IEEE80211_NUM_UPS; i++) {
        reo = sta->reos[i];
        if (reo) {
            AML_ERR("tid:%d old pn %llu", i, reo->pn);
            reo->pn = 0;
            reo->pn_check = 1;
        }
    }
}
