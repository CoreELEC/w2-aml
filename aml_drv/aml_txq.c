/**
 ******************************************************************************
 *
 * @file aml_txq.c
 *
 * Copyright (C) Amlogic 2016-2021
 *
 ******************************************************************************
 */

#include "aml_defs.h"
#include "aml_tx.h"
#include "ipc_host.h"
#include "aml_events.h"
#include "aml_iwpriv_cmds.h"
#include "aml_msg_tx.h"
#include "aml_wq.h"
#include "reg_ipc_app.h"
#include "aml_compat.h"

/******************************************************************************
 * Utils functions
 *****************************************************************************/
#ifdef CONFIG_AML_SOFTMAC
const int nx_tid_prio[NX_NB_TXQ_PER_STA] = {15, 7, 14, 6, 13, 5, 12, 4,
                                            11, 3, 8, 0, 10, 2, 9, 1};

static inline int aml_txq_sta_idx(struct aml_sta *sta, u8 tid)
{
    return sta->sta_idx * NX_NB_TXQ_PER_STA;
}

static inline int aml_txq_vif_idx(struct aml_vif *vif, u8 ac)
{
    return vif->vif_index * NX_NB_TXQ_PER_VIF + ac + NX_FIRST_VIF_TXQ_IDX;
}

#ifdef CONFIG_MAC80211_TXQ
struct aml_txq *aml_txq_sta_get(struct aml_sta *aml_sta, u8 tid)
{
    struct ieee80211_sta *sta = aml_to_ieee80211_sta(aml_sta);
    struct ieee80211_txq *mac_txq = sta->txq[tid];

    return (struct aml_txq *)mac_txq->drv_priv;
}

struct aml_txq *aml_txq_vif_get(struct aml_vif *aml_vif, u8 ac)
{
    struct ieee80211_vif *vif = aml_to_ieee80211_vif(aml_vif);

    /* mac80211 only allocate one txq per vif for Best Effort */
    if (ac == AML_HWQ_BE) {
        struct ieee80211_txq *mac_txq = vif->txq;
        if (!mac_txq)
            return NULL;
        return (struct aml_txq *)mac_txq->drv_priv;
    }

    if (ac > NX_TXQ_CNT)
        ac = AML_HWQ_BK;

    return &aml_vif->txqs[ac];
}

#else /* ! CONFIG_MAC80211_TXQ */
struct aml_txq *aml_txq_sta_get(struct aml_sta *sta, u8 tid)
{
    if (tid >= NX_NB_TXQ_PER_STA)
        tid = 0;

    return &sta->txqs[tid];
}

struct aml_txq *aml_txq_vif_get(struct aml_vif *vif, u8 ac)
{
    if (ac > NX_TXQ_CNT)
        ac = AML_HWQ_BK;

    return &vif->txqs[ac];
}

#endif /* CONFIG_MAC80211_TXQ */

static inline struct aml_sta *aml_txq_2_sta(struct aml_txq *txq)
{
    if (txq->sta)
        return (struct aml_sta *)txq->sta->drv_priv;
    return NULL;
}

#else /* CONFIG_AML_FULLMAC */
const int nx_tid_prio[NX_NB_TXQ_PER_STA] = {8, 7, 6, 5, 4, 3, 0, 2, 1};

static inline int aml_txq_sta_idx(struct aml_sta *sta, u8 tid)
{
    if (!sta) {
        AML_INFO("sta is null, return 0 in case of race condition");
        return 0;
    }

    if (is_multicast_sta(sta->sta_idx))
        return NX_FIRST_VIF_TXQ_IDX + sta->vif_idx;
    else
        return (sta->sta_idx * NX_NB_TXQ_PER_STA) + tid;
}

static inline int aml_txq_vif_idx(struct aml_vif *vif, u8 type)
{
    return NX_FIRST_VIF_TXQ_IDX + master_vif_idx(vif) + (type * NX_VIRT_DEV_MAX);
}

struct aml_txq *aml_txq_sta_get(struct aml_sta *sta, u8 tid, struct aml_hw *aml_hw)
{
    if (tid >= NX_NB_TXQ_PER_STA)
        tid = 0;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    if (!sta) {
        AML_INFO("aml sta is null, return null in case of race condition");
        return NULL;
    }
    if (!aml_hw->txq) {
        AML_INFO("aml txq is null");
        return NULL;
    }
    return aml_hw->txq + aml_txq_sta_idx(sta, tid);
#else
    return &aml_hw->txq[aml_txq_sta_idx(sta, tid)];
#endif
}

struct aml_txq *aml_txq_vif_get(struct aml_vif *vif, u8 type)
{
    if (type > NX_UNK_TXQ_TYPE)
        type = NX_BCMC_TXQ_TYPE;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    return vif->aml_hw->txq + aml_txq_vif_idx(vif, type);
#else
    return &vif->aml_hw->txq[aml_txq_vif_idx(vif, type)];
#endif
}

static inline struct aml_sta *aml_txq_2_sta(struct aml_txq *txq)
{
    return txq->sta;
}

#endif /* CONFIG_AML_SOFTMAC */


/******************************************************************************
 * Init/Deinit functions
 *****************************************************************************/
/**
 * aml_txq_init - Initialize a TX queue
 *
 * @txq: TX queue to be initialized
 * @idx: TX queue index
 * @status: TX queue initial status
 * @hwq: Associated HW queue
 * @ndev: Net device this queue belongs to
 *        (may be null for non netdev txq)
 *
 * Each queue is initialized with the credit of @NX_TXQ_INITIAL_CREDITS.
 */
static void aml_txq_init(struct aml_txq *txq, int idx, u8 status,
                          struct aml_hwq *hwq, int tid,
#ifdef CONFIG_AML_SOFTMAC
                          struct ieee80211_sta *sta
#else
                          struct aml_sta *sta, struct net_device *ndev
#endif
                          )
{
    int i;

    txq->idx = idx;
    txq->status = status;
    txq->credits = NX_TXQ_INITIAL_CREDITS;
    txq->pkt_sent = 0;
    skb_queue_head_init(&txq->sk_list);
    txq->last_retry_skb = NULL;
    txq->nb_retry = 0;
    txq->hwq = hwq;
    txq->sta = sta;
    for (i = 0; i < CONFIG_USER_MAX ; i++)
        txq->pkt_pushed[i] = 0;
    txq->push_limit = 0;
    txq->tid = tid;
#ifdef CONFIG_MAC80211_TXQ
    txq->nb_ready_mac80211 = 0;
#endif
#ifdef CONFIG_AML_SOFTMAC
    txq->baw.agg_on = false;
#ifdef CONFIG_AML_AMSDUS_TX
    txq->amsdu_anchor = NULL;
    txq->amsdu_ht_len_cap = 0;
    txq->amsdu_vht_len_cap = 0;
#endif /* CONFIG_AML_AMSDUS_TX */

#else /* ! CONFIG_AML_SOFTMAC */
    txq->ps_id = LEGACY_PS_ID;
    if (idx < NX_FIRST_VIF_TXQ_IDX) {
        int sta_idx = sta->sta_idx;
        int sta_tid = idx - (sta_idx * NX_NB_TXQ_PER_STA);
        if (sta_tid < NX_NB_TID_PER_STA)
            txq->ndev_idx = NX_STA_NDEV_IDX(sta_tid, sta_idx);
        else
            txq->ndev_idx = NDEV_NO_TXQ;
    } else if (idx < NX_FIRST_UNK_TXQ_IDX) {
        txq->ndev_idx = NX_BCMC_TXQ_NDEV_IDX;
    } else {
        txq->ndev_idx = NDEV_NO_TXQ;
    }
    txq->ndev = ndev;
#ifdef CONFIG_AML_AMSDUS_TX
    txq->amsdu = NULL;
    txq->amsdu_len = 0;
#endif /* CONFIG_AML_AMSDUS_TX */
#endif /* CONFIG_AML_SOFTMAC */
    spin_lock_init(&txq->txq_lock);
}

/**
 * aml_txq_drop_skb - Drop the buffer skb from the TX queue
 *
 * @txq:          TX queue
 * @skb:          skb packet that should be dropped.
 * @aml_hw:      Driver main data
 * @retry_packet: Is it a retry packet
 *
 */
void aml_txq_drop_skb(struct aml_txq *txq, struct sk_buff *skb, struct aml_hw *aml_hw, bool retry_packet)
{
    struct aml_sw_txhdr *sw_txhdr;
    unsigned long queued_time = 0;

    if (retry_packet)
        AML_INFO("retry packet unlink skb list from txq");
    skb_unlink(skb, &txq->sk_list);

    sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;

#ifdef CONFIG_AML_FULLMAC
    queued_time = jiffies - sw_txhdr->jiffies;
#endif
    trace_txq_drop_skb(skb, txq, queued_time);

#ifdef CONFIG_AML_AMSDUS_TX
    if (sw_txhdr->desc.api.host.packet_cnt > 1) {
        struct aml_amsdu_txhdr *amsdu_txhdr, *next;
        list_for_each_entry_safe(amsdu_txhdr, next, &sw_txhdr->amsdu.hdrs, list) {
            aml_ipc_buf_a2e_release(aml_hw, &amsdu_txhdr->ipc_data);
            dev_kfree_skb_any(amsdu_txhdr->skb);
        }
#ifdef CONFIG_AML_FULLMAC
        if (txq->amsdu == sw_txhdr)
            txq->amsdu = NULL;
#endif
    }
#endif

    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_MGMT) {
        AML_INFO("cfm status for mgmt");
        /* Confirm transmission to CFG80211 */
        cfg80211_mgmt_tx_status(&sw_txhdr->aml_vif->wdev,
                                (unsigned long)skb, skb_mac_header(skb),
                                sw_txhdr->frame_len,
                                0,
                                GFP_ATOMIC);
    }

    aml_ipc_buf_a2e_release(aml_hw, &sw_txhdr->ipc_data);
    kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
    if (retry_packet) {
        txq->nb_retry--;
        if (txq->nb_retry == 0) {
            WARN(skb != txq->last_retry_skb,
                 "last dropped retry buffer is not the expected one");
            txq->last_retry_skb = NULL;
        }
    }
#ifdef CONFIG_AML_SOFTMAC
    else
        txq->hwq->len --; // hwq->len doesn't count skb to retry
    ieee80211_free_txskb(aml_hw->hw, skb);
#else
    dev_kfree_skb_any(skb);
#endif /* CONFIG_AML_SOFTMAC */

}

/**
 * aml_txq_flush - Flush all buffers queued for a TXQ
 *
 * @aml_hw: main driver data
 * @txq: txq to flush
 */
void aml_txq_flush(struct aml_hw *aml_hw, struct aml_txq *txq)
{
    int i = 0, pushed = 0;
    struct sk_buff *txq_skb;

    while(!skb_queue_empty(&txq->sk_list)) {
        txq_skb = skb_peek(&txq->sk_list);
        if (txq_skb != NULL) {
            i++;
            aml_txq_drop_skb(txq, txq_skb, aml_hw, txq->nb_retry);
        }
    }

    if (i > 0)
        AML_INFO("txq drop %d skb", i);

    for (i = 0; i < CONFIG_USER_MAX; i++) {
        pushed += txq->pkt_pushed[i];
    }

    if (pushed)
        dev_warn(aml_hw->dev, "TXQ[%d]: %d skb still pushed to the FW",
                 txq->idx, pushed);
}

/**
 * aml_txq_deinit - De-initialize a TX queue
 *
 * @aml_hw: Driver main data
 * @txq: TX queue to be de-initialized
 * Any buffer stuck in a queue will be freed.
 */
static void aml_txq_deinit(struct aml_hw *aml_hw, struct aml_txq *txq)
{

    spin_lock_bh(&aml_hw->tx_lock);
    if (!txq || (txq->idx == TXQ_INACTIVE)) {
        spin_unlock_bh(&aml_hw->tx_lock);
        return;
    }
    AML_INFO("txq deinit, txq idx=%d, vif_idx=%d",
            txq->idx, txq->sta == NULL ? 0xff : txq->sta->vif_idx);
    aml_txq_del_from_hw_list(txq);
    txq->idx = TXQ_INACTIVE;
    aml_txq_flush(aml_hw, txq);
    spin_unlock_bh(&aml_hw->tx_lock);
}

/**
 * aml_txq_vif_init - Initialize all TXQ linked to a vif
 *
 * @aml_hw: main driver data
 * @aml_vif: Pointer on VIF
 * @status: Intial txq status
 *
 * Softmac : 1 VIF TXQ per HWQ
 *
 * Fullmac : 1 VIF TXQ for BC/MC
 *           1 VIF TXQ for MGMT to unknown STA
 */
void aml_txq_vif_init(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                       u8 status)
{
    struct aml_txq *txq;
    int idx;

#ifdef CONFIG_AML_SOFTMAC
    int ac;

    idx = aml_txq_vif_idx(aml_vif, 0);
    foreach_vif_txq(aml_vif, txq, ac) {
        if (txq) {
            aml_txq_init(txq, idx, status, &aml_hw->hwq[ac], 0, NULL);
#ifdef CONFIG_MAC80211_TXQ
            if (ac != AML_HWQ_BE)
                txq->nb_ready_mac80211 = NOT_MAC80211_TXQ;
#endif
        }
        idx++;
    }

#else
    txq = aml_txq_vif_get(aml_vif, NX_BCMC_TXQ_TYPE);
    idx = aml_txq_vif_idx(aml_vif, NX_BCMC_TXQ_TYPE);
    aml_txq_init(txq, idx, status, &aml_hw->hwq[AML_HWQ_BE], 0,
                  aml_hw->sta_table + aml_vif->ap.bcmc_index, aml_vif->ndev);

    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    idx = aml_txq_vif_idx(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_init(txq, idx, status, &aml_hw->hwq[AML_HWQ_VO], TID_MGT,
                  NULL, aml_vif->ndev);

#endif /* CONFIG_AML_SOFTMAC */
}

/**
 * aml_txq_vif_deinit - Deinitialize all TXQ linked to a vif
 *
 * @aml_hw: main driver data
 * @aml_vif: Pointer on VIF
 */
void aml_txq_vif_deinit(struct aml_hw * aml_hw, struct aml_vif *aml_vif)
{
    struct aml_txq *txq;

#ifdef CONFIG_AML_SOFTMAC
    int ac;

    foreach_vif_txq(aml_vif, txq, ac) {
        if (txq)
            aml_txq_deinit(aml_hw, txq);
    }

#else
    txq = aml_txq_vif_get(aml_vif, NX_BCMC_TXQ_TYPE);
    aml_txq_deinit(aml_hw, txq);

    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_deinit(aml_hw, txq);

#endif /* CONFIG_AML_SOFTMAC */
}


/**
 * aml_txq_sta_init - Initialize TX queues for a STA
 *
 * @aml_hw: Main driver data
 * @aml_sta: STA for which tx queues need to be initialized
 * @status: Intial txq status
 *
 * This function initialize all the TXQ associated to a STA.
 * Softmac : 1 TXQ per TID
 *
 * Fullmac : 1 TXQ per TID (limited to 8)
 *           1 TXQ for MGMT
 */
void aml_txq_sta_init(struct aml_hw *aml_hw, struct aml_sta *aml_sta,
                       u8 status)
{
    struct aml_txq *txq;
    int tid, idx;

#ifdef CONFIG_AML_SOFTMAC
    struct ieee80211_sta *sta = aml_to_ieee80211_sta(aml_sta);
    idx = aml_txq_sta_idx(aml_sta, 0);

    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        aml_txq_init(txq, idx, status, &aml_hw->hwq[aml_tid2hwq[tid]], tid, sta);
        idx++;
    }

#else
    struct aml_vif *aml_vif = aml_hw->vif_table[aml_sta->vif_idx];
    idx = aml_txq_sta_idx(aml_sta, 0);

    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        aml_txq_init(txq, idx, status, &aml_hw->hwq[aml_tid2hwq[tid]], tid,
                      aml_sta, aml_vif->ndev);
        txq->ps_id = aml_sta->uapsd_tids & (1 << tid) ? UAPSD_ID : LEGACY_PS_ID;
        idx++;
    }

#endif /* CONFIG_AML_SOFTMAC*/
}

/**
 * aml_txq_sta_deinit - Deinitialize TX queues for a STA
 *
 * @aml_hw: Main driver data
 * @aml_sta: STA for which tx queues need to be deinitialized
 */
void aml_txq_sta_deinit(struct aml_hw *aml_hw, struct aml_sta *aml_sta)
{
    struct aml_txq *txq;
    int tid;

    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        aml_txq_deinit(aml_hw, txq);
    }
}

#ifdef CONFIG_AML_FULLMAC
/**
 * aml_txq_unk_vif_init - Initialize TXQ for unknown STA linked to a vif
 *
 * @aml_vif: Pointer on VIF
 */
void aml_txq_unk_vif_init(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_txq *txq;
    int idx;

    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    idx = aml_txq_vif_idx(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_deinit(aml_hw,txq);
    aml_txq_init(txq, idx, 0, &aml_hw->hwq[AML_HWQ_VO], TID_MGT, NULL, aml_vif->ndev);
}

/**
 * aml_txq_unk_vif_deinit - Deinitialize TXQ for unknown STA linked to a vif
 *
 * @aml_vif: Pointer on VIF
 */
void aml_txq_unk_vif_deinit(struct aml_vif *aml_vif)
{
    struct aml_txq *txq;

    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_deinit(aml_vif->aml_hw, txq);
}

/**
 * aml_txq_offchan_init - Initialize TX queue for the transmission on a offchannel
 *
 * @vif: Interface for which the queue has to be initialized
 *
 * NOTE: Offchannel txq is only active for the duration of the ROC
 */
void aml_txq_offchan_init(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_txq *txq;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    txq = aml_hw->txq + NX_OFF_CHAN_TXQ_IDX;
#else
    txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
#endif
    aml_txq_init(txq, NX_OFF_CHAN_TXQ_IDX, AML_TXQ_STOP_CHAN,
                  &aml_hw->hwq[AML_HWQ_VO], TID_MGT, NULL, aml_vif->ndev);
}

/**
 * aml_deinit_offchan_txq - Deinitialize TX queue for offchannel
 *
 * @vif: Interface that manages the STA
 *
 * This function deintialize txq for one STA.
 * Any buffer stuck in a queue will be freed.
 */
void aml_txq_offchan_deinit(struct aml_vif *aml_vif)
{
    struct aml_txq *txq;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    txq = aml_vif->aml_hw->txq + NX_OFF_CHAN_TXQ_IDX;
#else
    txq = &aml_vif->aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
#endif
    aml_txq_deinit(aml_vif->aml_hw, txq);
}


/**
 * aml_txq_tdls_vif_init - Initialize TXQ vif for TDLS
 *
 * @aml_vif: Pointer on VIF
 */
void aml_txq_tdls_vif_init(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    if (!(aml_hw->wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS))
        return;

    aml_txq_unk_vif_init(aml_vif);
}

/**
 * aml_txq_tdls_vif_deinit - Deinitialize TXQ vif for TDLS
 *
 * @aml_vif: Pointer on VIF
 */
void aml_txq_tdls_vif_deinit(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    if (!(aml_hw->wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS))
        return;

    aml_txq_unk_vif_deinit(aml_vif);
}

/**
 * aml_txq_drop_old_traffic - Drop pkt queued for too long in a TXQ
 *
 * @txq: TXQ to process
 * @aml_hw: Driver main data
 * @skb_timeout: Max queue duration, in jiffies, for this queue
 * @dropped: Updated to inidicate if at least one skb was dropped
 *
 * @return Whether there is still pkt queued in this queue.
 */
static bool aml_txq_drop_old_traffic(struct aml_txq *txq, struct aml_hw *aml_hw,
                                      unsigned long skb_timeout, bool *dropped)
{
    struct sk_buff *skb, *skb_next;
    bool pkt_queued = false;
    int retry_packet;

    aml_spin_lock(&aml_hw->tx_lock);
    if (!txq || txq->idx == TXQ_INACTIVE) {
        aml_spin_unlock(&aml_hw->tx_lock);
        return false;
    }
    // get nb_retry; The variable should be in the lock to avoid updating the value after
    // obtaining the lock
    retry_packet = txq->nb_retry;

    skb_queue_walk_safe(&txq->sk_list, skb, skb_next) {

        struct aml_sw_txhdr *sw_txhdr;

        if (retry_packet) {
            // Don't drop retry packets
            retry_packet--;
            continue;
        }

        sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;

        if (!time_after(jiffies, sw_txhdr->jiffies + skb_timeout)) {
            pkt_queued = true;
            break;
        }

        *dropped = true;
        aml_txq_drop_skb(txq, skb, aml_hw, false);
        if (txq->sta && txq->sta->ps.active) {
            txq->sta->ps.pkt_ready[txq->ps_id]--;
            if (txq->sta->ps.pkt_ready[txq->ps_id] == 0)
                aml_set_traffic_status(aml_hw, txq->sta, false, txq->ps_id);

            // drop packet during PS service period ...
            if (txq->sta->ps.sp_cnt[txq->ps_id]) {
                txq->sta->ps.sp_cnt[txq->ps_id] --;
                if (txq->push_limit)
                    txq->push_limit--;
                if (WARN(((txq->ps_id == UAPSD_ID) &&
                          (txq->sta->ps.sp_cnt[txq->ps_id] == 0)),
                         "Drop last packet of UAPSD service period")) {
                    // TODO: inform FW to end SP
                }
            }
            trace_ps_drop(txq->sta);
        }
    }

    if (skb_queue_empty(&txq->sk_list)) {
        aml_txq_del_from_hw_list(txq);
        txq->pkt_sent = 0;
    }
    aml_spin_unlock(&aml_hw->tx_lock);

    /* restart netdev queue if number no more queued buffer */
    if (unlikely(txq->status & AML_TXQ_NDEV_FLOW_CTRL) &&
        skb_queue_empty(&txq->sk_list)) {
        txq->status &= ~AML_TXQ_NDEV_FLOW_CTRL;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
        trace_txq_flowctrl_restart(txq);
    }

    return pkt_queued;
}

/**
 * aml_txq_drop_ap_vif_old_traffic - Drop pkt queued for too long in TXQs
 * linked to an "AP" vif (AP, MESH, P2P_GO)
 *
 * @vif: Vif to process
 * @return Whether there is still pkt queued in any TXQ.
 */

void aml_show_tx_msg(struct aml_hw *aml_hw,struct aml_wq *aml_wq)
{
    struct aml_vif *vif = (struct aml_vif *)aml_wq->data;
    AML_INFO("vif:%d",vif->vif_index);
    aml_get_txq(vif->ndev);
    aml_txq_unexpection(vif->ndev);
}
#ifdef CONFIG_AML_RECOVERY
extern struct aml_recy *aml_recy;
#endif
static bool aml_txq_drop_ap_vif_old_traffic(struct aml_vif *vif)
{
    struct aml_sta *sta, *tmp;
    unsigned long timeout = (((unsigned long)vif->ap.bcn_interval) * HZ * 3) >> 10;
    bool pkt_queued = false;
    bool pkt_dropped = false;
#ifdef CONFIG_AML_RECOVERY
    struct aml_wq *aml_wq;
    enum aml_wq_type type = AML_WQ_RECY;
#endif

    // Should never be needed but still check VIF queues
    aml_txq_drop_old_traffic(aml_txq_vif_get(vif, NX_BCMC_TXQ_TYPE),
                              vif->aml_hw, AML_TXQ_MAX_QUEUE_JIFFIES, &pkt_dropped);
    aml_txq_drop_old_traffic(aml_txq_vif_get(vif, NX_UNK_TXQ_TYPE),
                              vif->aml_hw, AML_TXQ_MAX_QUEUE_JIFFIES, &pkt_dropped);
    WARN(pkt_dropped, "Dropped packet in BCMC/UNK queue");

    if (pkt_dropped) {
        struct aml_wq *aml_wq;
        aml_wq = aml_wq_alloc(sizeof(struct aml_vif));
        if (!aml_wq) {
            AML_INFO("alloc workqueue out of memory");
        }
        else
        {
            aml_wq->id = AML_WQ_SHOW_TX_MSG;
            memcpy(aml_wq->data, vif, sizeof(struct aml_vif));
            aml_wq_add(vif->aml_hw, aml_wq);
        }

#ifdef CONFIG_AML_RECOVERY
        spin_lock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
        if (!aml_recy->reason) {
            aml_wq = aml_wq_alloc(1);
            if (!aml_wq) {
                AML_INFO("alloc wq out of memory");
            } else {
                aml_recy->reason = RECY_REASON_CODE_TX_TIMEOUT;
                aml_wq->id = AML_WQ_RECY;
                memcpy(aml_wq->data, &type, 1);
                aml_wq_add(aml_recy->aml_hw, aml_wq);
           }
        }
        spin_unlock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
#endif
    }

    /* protect union struct vif->ap.sta_list was overwritten
     * by vif->sta.ap = NULL when aml_cfg80211_change_iface
     */
    spin_lock_bh(&vif->vif_lock);
    if (vif->ap.sta_list.next) {
        list_for_each_entry_safe(sta, tmp, &vif->ap.sta_list, list) {
            struct aml_txq *txq;
            int tid;
            foreach_sta_txq_safe(sta, txq, tid, vif->aml_hw) {
                pkt_queued |= aml_txq_drop_old_traffic(txq, vif->aml_hw,
                                                        timeout * sta->listen_interval,
                                                        &pkt_dropped);
            }
        }
    }
    spin_unlock_bh(&vif->vif_lock);

    return pkt_queued;
}

/**
 * aml_txq_drop_sta_vif_old_traffic - Drop pkt queued for too long in TXQs
 * linked to a "STA" vif. In theory this should not be required as there is no
 * case where traffic can accumulate in a STA interface.
 *
 * @vif: Vif to process
 * @return Whether there is still pkt queued in any TXQ.
 */
static bool aml_txq_drop_sta_vif_old_traffic(struct aml_vif *vif)
{
    struct aml_txq *txq;
    bool pkt_queued = false, pkt_dropped = false;
    int tid;
#ifdef CONFIG_AML_RECOVERY
    struct aml_wq *aml_wq;
    enum aml_wq_type type = AML_WQ_RECY;
#endif

    if (vif->tdls_status == TDLS_LINK_ACTIVE) {
        txq = aml_txq_vif_get(vif, NX_UNK_TXQ_TYPE);
        pkt_queued |= aml_txq_drop_old_traffic(txq, vif->aml_hw,
                                                AML_TXQ_MAX_QUEUE_JIFFIES,
                                                &pkt_dropped);
        foreach_sta_txq_safe(vif->sta.tdls_sta, txq, tid, vif->aml_hw) {
            pkt_queued |= aml_txq_drop_old_traffic(txq, vif->aml_hw,
                                                    AML_TXQ_MAX_QUEUE_JIFFIES,
                                                    &pkt_dropped);
        }
    }

    spin_lock_bh(&vif->vif_lock);
    if (vif->sta.ap) {
        foreach_sta_txq_safe(vif->sta.ap, txq, tid, vif->aml_hw) {
            pkt_queued |= aml_txq_drop_old_traffic(txq, vif->aml_hw,
                                                    AML_TXQ_MAX_QUEUE_JIFFIES,
                                                    &pkt_dropped);
        }
    }
    spin_unlock_bh(&vif->vif_lock);

    if (pkt_dropped) {
        struct aml_wq *aml_wq;
        netdev_warn(vif->ndev, "Dropped packet in STA interface TXQs");
        aml_wq = aml_wq_alloc(sizeof(struct aml_vif));
        if (!aml_wq) {
            AML_INFO("alloc workqueue out of memory");
        }
        else
        {
            aml_wq->id = AML_WQ_SHOW_TX_MSG;
            memcpy(aml_wq->data, vif, sizeof(struct aml_vif));
            aml_wq_add(vif->aml_hw, aml_wq);
        }
#ifdef CONFIG_AML_RECOVERY
        spin_lock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
        if (!aml_recy->reason) {
            aml_wq = aml_wq_alloc(1);
            if (!aml_wq) {
                AML_INFO("alloc wq out of memory");
            } else {
                aml_recy->reason = RECY_REASON_CODE_TX_TIMEOUT;
                aml_wq->id = AML_WQ_RECY;
                memcpy(aml_wq->data, &type, 1);
                aml_wq_add(aml_recy->aml_hw, aml_wq);
           }
        }
        spin_unlock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
#endif
    }
    return pkt_queued;
}
/**
 * aml_txq_cleanup_timer_cb - callack for TXQ cleaup timer
 * Used to prevent pkt to accumulate in TXQ. The main use case is for AP
 * interface with client in Power Save mode but just in case all TXQs are
 * checked.
 *
 * @t: timer structure
 */
static void aml_txq_cleanup_timer_cb(struct timer_list *t)
{
    struct aml_hw *aml_hw = from_timer(aml_hw, t, txq_cleanup);
    struct aml_vif *vif, *vif_tmp;
    bool pkt_queue = false;

    list_for_each_entry_safe(vif, vif_tmp, &aml_hw->vifs, list) {
        switch (AML_VIF_TYPE(vif)) {
            case NL80211_IFTYPE_AP:
            case NL80211_IFTYPE_P2P_GO:
            case NL80211_IFTYPE_MESH_POINT:
                pkt_queue |= aml_txq_drop_ap_vif_old_traffic(vif);
                break;
            case NL80211_IFTYPE_STATION:
            case NL80211_IFTYPE_P2P_CLIENT:
                pkt_queue |= aml_txq_drop_sta_vif_old_traffic(vif);
                break;
            case NL80211_IFTYPE_AP_VLAN:
            case NL80211_IFTYPE_MONITOR:
            default:
                break;
        }
    }

    if (pkt_queue)
        mod_timer(t, jiffies + AML_TXQ_CLEANUP_INTERVAL);
}

/**
 * aml_txq_start_cleanup_timer - Start 'cleanup' timer if not started
 *
 * @aml_hw: Driver main data
 */
void aml_txq_start_cleanup_timer(struct aml_hw *aml_hw, struct aml_sta *sta)
{
    if (sta && !is_multicast_sta(sta->sta_idx) &&
        !timer_pending(&aml_hw->txq_cleanup))
        mod_timer(&aml_hw->txq_cleanup, jiffies + AML_TXQ_CLEANUP_INTERVAL);
}

/**
 * aml_txq_prepare - Global initialization of txq
 *
 * @aml_hw: Driver main data
 */
void aml_txq_prepare(struct aml_hw *aml_hw)
{
    int i;

    for (i = 0; i < NX_NB_TXQ; i++) {
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
        (aml_hw->txq + i)->idx = TXQ_INACTIVE;
#else
        aml_hw->txq[i].idx = TXQ_INACTIVE;
#endif
    }

    timer_setup(&aml_hw->txq_cleanup, aml_txq_cleanup_timer_cb, 0);
}

#endif // CONFIG_AML_FULLMAC

/******************************************************************************
 * Start/Stop functions
 *****************************************************************************/
/**
 * aml_txq_add_to_hw_list - Add TX queue to a HW queue schedule list.
 *
 * @txq: TX queue to add
 *
 * Add the TX queue if not already present in the HW queue list.
 * To be called with tx_lock hold
 */
void aml_txq_add_to_hw_list(struct aml_txq *txq)
{
    aml_spin_lock(&txq->txq_lock);
    if (!(txq->status & AML_TXQ_IN_HWQ_LIST)) {
        trace_txq_add_to_hw(txq);
        txq->status |= AML_TXQ_IN_HWQ_LIST;
        list_add_tail(&txq->sched_list, &txq->hwq->list);
        txq->hwq->need_processing = true;
    }
    aml_spin_unlock(&txq->txq_lock);
}

/**
 * aml_txq_del_from_hw_list - Delete TX queue from a HW queue schedule list.
 *
 * @txq: TX queue to delete
 *
 * Remove the TX queue from the HW queue list if present.
 * To be called with tx_lock hold
 */
void aml_txq_del_from_hw_list(struct aml_txq *txq)
{
    aml_spin_lock(&txq->txq_lock);
    if (txq->status & AML_TXQ_IN_HWQ_LIST) {
        trace_txq_del_from_hw(txq);
        txq->status &= ~AML_TXQ_IN_HWQ_LIST;
        list_del(&txq->sched_list);
    }
    aml_spin_unlock(&txq->txq_lock);
}

/**
 * aml_txq_skb_ready - Check if skb are available for the txq
 *
 * @txq: Pointer on txq
 * @return True if there are buffer ready to be pushed on this txq,
 * false otherwise
 */
static inline bool aml_txq_skb_ready(struct aml_txq *txq)
{
#ifdef CONFIG_MAC80211_TXQ
    if (txq->nb_ready_mac80211 != NOT_MAC80211_TXQ)
        return ((txq->nb_ready_mac80211 > 0) || !skb_queue_empty(&txq->sk_list));
    else
#endif
    return !skb_queue_empty(&txq->sk_list);
}

/**
 * aml_txq_start - Try to Start one TX queue
 *
 * @txq: TX queue to start
 * @reason: reason why the TX queue is started (among aml_txq_stop_xxx)
 *
 * Re-start the TX queue for one reason.
 * If after this the txq is no longer stopped and some buffers are ready,
 * the TX queue is also added to HW queue list.
 * To be called with tx_lock hold
 */
void aml_txq_start(struct aml_txq *txq, u16 reason)
{
    BUG_ON(txq==NULL);
    if (txq->idx != TXQ_INACTIVE && (txq->status & reason))
    {
        trace_txq_start(txq, reason);
        txq->status &= ~reason;
        if (!aml_txq_is_stopped(txq) && aml_txq_skb_ready(txq))
            aml_txq_add_to_hw_list(txq);
    }
}

/**
 * aml_txq_stop - Stop one TX queue
 *
 * @txq: TX queue to stop
 * @reason: reason why the TX queue is stopped (among aml_txq_stop_xxx)
 *
 * Stop the TX queue. It will remove the TX queue from HW queue list
 * To be called with tx_lock hold
 */
void aml_txq_stop(struct aml_txq *txq, u16 reason)
{
    BUG_ON(txq==NULL);
    if (txq->idx != TXQ_INACTIVE)
    {
        trace_txq_stop(txq, reason);
        txq->status |= reason;
        aml_txq_del_from_hw_list(txq);
    }
}


/**
 * aml_txq_sta_start - Start all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be re-started
 * @reason: Reason why the TX queue are restarted (among aml_txq_stop_xxx)
 * @aml_hw: Driver main data
 *
 * This function will re-start all the TX queues of the STA for the reason
 * specified. It can be :
 * - aml_txq_stop_STA_PS: the STA is no longer in power save mode
 * - aml_txq_stop_VIF_PS: the VIF is in power save mode (p2p absence)
 * - aml_txq_stop_CHAN: the STA's VIF is now on the current active channel
 *
 * Any TX queue with buffer ready and not Stopped for other reasons, will be
 * added to the HW queue list
 * To be called with tx_lock hold
 */
void aml_txq_sta_start(struct aml_sta *aml_sta, u16 reason
#ifdef CONFIG_AML_FULLMAC
                        , struct aml_hw *aml_hw
#endif
                        )
{
    struct aml_txq *txq;
    int tid;

    trace_txq_sta_start(aml_sta->sta_idx);

    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        aml_txq_start(txq, reason);
    }
}


/**
 * aml_stop_sta_txq - Stop all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be stopped
 * @reason: Reason why the TX queue are stopped (among AML_TX_STOP_xxx)
 * @aml_hw: Driver main data
 *
 * This function will stop all the TX queues of the STA for the reason
 * specified. It can be :
 * - aml_txq_stop_STA_PS: the STA is in power save mode
 * - aml_txq_stop_VIF_PS: the VIF is in power save mode (p2p absence)
 * - aml_txq_stop_CHAN: the STA's VIF is not on the current active channel
 *
 * Any TX queue present in a HW queue list will be removed from this list.
 * To be called with tx_lock hold
 */
void aml_txq_sta_stop(struct aml_sta *aml_sta, u16 reason
#ifdef CONFIG_AML_FULLMAC
                       , struct aml_hw *aml_hw
#endif
                       )
{
    struct aml_txq *txq;
    int tid;

    if (!aml_sta)
        return;

    trace_txq_sta_stop(aml_sta->sta_idx);
    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        aml_txq_stop(txq, reason);
    }
}

#ifdef CONFIG_AML_SOFTMAC
void aml_txq_tdls_sta_start(struct aml_sta *aml_sta, u16 reason,
                             struct aml_hw *aml_hw)
#else
void aml_txq_tdls_sta_start(struct aml_vif *aml_vif, u16 reason,
                             struct aml_hw *aml_hw)
#endif
{
#ifdef CONFIG_AML_SOFTMAC
    trace_txq_vif_start(aml_sta->vif_idx);
    spin_lock_bh(&aml_hw->tx_lock);

    if (aml_sta->tdls.active) {
        aml_txq_sta_start(aml_sta, reason);
    }

    spin_unlock_bh(&aml_hw->tx_lock);
#else
    trace_txq_vif_start(aml_vif->vif_index);
    spin_lock_bh(&aml_hw->tx_lock);

    if (aml_vif->sta.tdls_sta)
        aml_txq_sta_start(aml_vif->sta.tdls_sta, reason, aml_hw);

    spin_unlock_bh(&aml_hw->tx_lock);
#endif
}

#ifdef CONFIG_AML_SOFTMAC
void aml_txq_tdls_sta_stop(struct aml_sta *aml_sta, u16 reason,
                            struct aml_hw *aml_hw)
#else
void aml_txq_tdls_sta_stop(struct aml_vif *aml_vif, u16 reason,
                            struct aml_hw *aml_hw)
#endif
{
#ifdef CONFIG_AML_SOFTMAC
    trace_txq_vif_stop(aml_sta->vif_idx);

    spin_lock_bh(&aml_hw->tx_lock);

    if (aml_sta->tdls.active) {
        aml_txq_sta_stop(aml_sta, reason);
    }

    spin_unlock_bh(&aml_hw->tx_lock);
#else
    trace_txq_vif_stop(aml_vif->vif_index);

    spin_lock_bh(&aml_hw->tx_lock);

    if (aml_vif->sta.tdls_sta)
        aml_txq_sta_stop(aml_vif->sta.tdls_sta, reason, aml_hw);

    spin_unlock_bh(&aml_hw->tx_lock);
#endif
}

#ifdef CONFIG_AML_FULLMAC
static inline
void aml_txq_vif_for_each_sta(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                               void (*f)(struct aml_sta *, u16, struct aml_hw *),
                               u16 reason)
{

    switch (AML_VIF_TYPE(aml_vif)) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        if (aml_vif->tdls_status == TDLS_LINK_ACTIVE)
            f(aml_vif->sta.tdls_sta, reason, aml_hw);

        if (aml_vif->sta.ap != NULL)
            f(aml_vif->sta.ap, reason, aml_hw);
        break;
    }
    case NL80211_IFTYPE_AP_VLAN:
        aml_vif = aml_vif->ap_vlan.master;
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_MESH_POINT:
    case NL80211_IFTYPE_P2P_GO:
    {
        struct aml_sta *sta, *tmp;
        list_for_each_entry_safe(sta, tmp, &aml_vif->ap.sta_list, list) {
            f(sta, reason, aml_hw);
        }
        break;
    }
    default:
        BUG();
        break;
    }
}

#endif

/**
 * aml_txq_vif_start - START TX queues of all STA associated to the vif
 *                      and vif's TXQ
 *
 * @vif: Interface to start
 * @reason: Start reason (aml_txq_stop_CHAN or aml_txq_stop_VIF_PS)
 * @aml_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and re-start them for the
 * reason @reason
 * Take tx_lock
 */
void aml_txq_vif_start(struct aml_vif *aml_vif, u16 reason,
                        struct aml_hw *aml_hw)
{
    struct aml_txq *txq;
#ifdef CONFIG_AML_SOFTMAC
    struct aml_sta *aml_sta;
    int ac;
#endif

    trace_txq_vif_start(aml_vif->vif_index);
    /* vif_lock should be outside of tx_lock, it avoid the lock cross @aml_txq_cleanup_timer_cb */
    spin_lock_bh(&aml_vif->vif_lock);
    spin_lock_bh(&aml_hw->tx_lock);

#ifdef CONFIG_AML_SOFTMAC
    list_for_each_entry(aml_sta, &aml_vif->stations, list) {
        if ((!aml_vif->roc_tdls) ||
            (aml_sta->tdls.active && aml_vif->roc_tdls && aml_sta->tdls.chsw_en))
            aml_txq_sta_start(aml_sta, reason);
    }

    foreach_vif_txq(aml_vif, txq, ac) {
        if (txq)
            aml_txq_start(txq, reason);
    }
#else
    //Reject if monitor interface
    if (aml_vif->wdev.iftype == NL80211_IFTYPE_MONITOR)
        goto end;

    if (aml_vif->roc_tdls && aml_vif->sta.tdls_sta && aml_vif->sta.tdls_sta->tdls.chsw_en) {
        aml_txq_sta_start(aml_vif->sta.tdls_sta, reason, aml_hw);
    }
    if (!aml_vif->roc_tdls) {
        aml_txq_vif_for_each_sta(aml_hw, aml_vif, aml_txq_sta_start, reason);
    }

    txq = aml_txq_vif_get(aml_vif, NX_BCMC_TXQ_TYPE);
    aml_txq_start(txq, reason);
    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_start(txq, reason);

end:
#endif /* CONFIG_AML_SOFTMAC */

    spin_unlock_bh(&aml_hw->tx_lock);
    spin_unlock_bh(&aml_vif->vif_lock);
}


/**
 * aml_txq_vif_stop - STOP TX queues of all STA associated to the vif
 *
 * @vif: Interface to stop
 * @arg: Stop reason (aml_txq_stop_CHAN or aml_txq_stop_VIF_PS)
 * @aml_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and stop them for the
 * reason aml_txq_stop_CHAN or aml_txq_stop_VIF_PS
 * Take tx_lock
 */
void aml_txq_vif_stop(struct aml_vif *aml_vif, u16 reason,
                       struct aml_hw *aml_hw)
{
    struct aml_txq *txq;
#ifdef CONFIG_AML_SOFTMAC
    struct aml_sta *sta;
    int ac;
#endif

    trace_txq_vif_stop(aml_vif->vif_index);
    /* vif_lock should be outside of tx_lock, it avoid the lock cross @aml_txq_cleanup_timer_cb */
    spin_lock_bh(&aml_vif->vif_lock);
    spin_lock_bh(&aml_hw->tx_lock);

#ifdef CONFIG_AML_SOFTMAC
    list_for_each_entry(sta, &aml_vif->stations, list) {
        aml_txq_sta_stop(sta, reason);
    }

    foreach_vif_txq(aml_vif, txq, ac) {
        if (txq)
            aml_txq_stop(txq, reason);
    }

#else
    //Reject if monitor interface
    if (aml_vif->wdev.iftype == NL80211_IFTYPE_MONITOR)
        goto end;

    aml_txq_vif_for_each_sta(aml_hw, aml_vif, aml_txq_sta_stop, reason);

    txq = aml_txq_vif_get(aml_vif, NX_BCMC_TXQ_TYPE);
    aml_txq_stop(txq, reason);
    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    aml_txq_stop(txq, reason);

end:
#endif /* CONFIG_AML_SOFTMAC*/

    spin_unlock_bh(&aml_hw->tx_lock);
    spin_unlock_bh(&aml_vif->vif_lock);
}

#ifdef CONFIG_AML_FULLMAC

/**
 * aml_start_offchan_txq - START TX queue for offchannel frame
 *
 * @aml_hw: Driver main data
 */
void aml_txq_offchan_start(struct aml_hw *aml_hw)
{
    struct aml_txq *txq;
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    txq = aml_hw->txq + NX_OFF_CHAN_TXQ_IDX;
#else
    txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
#endif
    spin_lock_bh(&aml_hw->tx_lock);
    aml_txq_start(txq, AML_TXQ_STOP_CHAN);
    spin_unlock_bh(&aml_hw->tx_lock);
}

/**
 * aml_switch_vif_sta_txq - Associate TXQ linked to a STA to a new vif
 *
 * @sta: STA whose txq must be switched
 * @old_vif: Vif currently associated to the STA (may no longer be active)
 * @new_vif: vif which should be associated to the STA for now on
 *
 * This function will switch the vif (i.e. the netdev) associated to all STA's
 * TXQ. This is used when AP_VLAN interface are created.
 * If one STA is associated to an AP_vlan vif, it will be moved from the master
 * AP vif to the AP_vlan vif.
 * If an AP_vlan vif is removed, then STA will be moved back to mastert AP vif.
 *
 */
void aml_txq_sta_switch_vif(struct aml_sta *sta, struct aml_vif *old_vif,
                             struct aml_vif *new_vif)
{
    struct aml_hw *aml_hw = new_vif->aml_hw;
    struct aml_txq *txq;
    int i;

    /* start TXQ on the new interface, and update ndev field in txq */
    if (!netif_carrier_ok(new_vif->ndev))
        netif_carrier_on(new_vif->ndev);
    txq = aml_txq_sta_get(sta, 0, aml_hw);
    for (i = 0; i < NX_NB_TID_PER_STA; i++, txq++) {
        txq->ndev = new_vif->ndev;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
    }
}
#endif /* CONFIG_AML_FULLMAC */

/******************************************************************************
 * TXQ queue/schedule functions
 *****************************************************************************/
/**
 * aml_txq_queue_skb - Queue a buffer in a TX queue
 *
 * @skb: Buffer to queue
 * @txq: TX Queue in which the buffer must be added
 * @aml_hw: Driver main data
 * @retry: Should it be queued in the retry list
 * @skb_prev: If not NULL insert buffer after this skb instead of the tail
 * of the list (ignored if retry is true)
 *
 * @return: Return 1 if txq has been added to hwq list, 0 otherwise
 *
 * Add a buffer in the buffer list of the TX queue
 * and add this TX queue in the HW queue list if the txq is not stopped.
 * If this is a retry packet it is added after the last retry packet or at the
 * beginning if there is no retry packet queued.
 *
 * If the STA is in PS mode and this is the first packet queued for this txq
 * update TIM.
 *
 * To be called with tx_lock hold
 */
int aml_txq_queue_skb(struct sk_buff *skb, struct aml_txq *txq,
                       struct aml_hw *aml_hw,  bool retry,
                       struct sk_buff *skb_prev)
{
#ifdef CONFIG_AML_FULLMAC
    if (unlikely(txq->sta && txq->sta->ps.active)) {
        txq->sta->ps.pkt_ready[txq->ps_id]++;
        trace_ps_queue(txq->sta);

        if (txq->sta->ps.pkt_ready[txq->ps_id] == 1) {
            aml_set_traffic_status(aml_hw, txq->sta, true, txq->ps_id);
        }
    }
#else
    // Update mac80211 when buffered traffic for station in PS change
    if (aml_txq_is_ps(txq) && skb_queue_empty(&txq->sk_list))
        ieee80211_sta_set_buffered(txq->sta, txq->tid, true);
#endif

    if (!retry) {
        /* add buffer in the sk_list */
        if (skb_prev)
            SKB_APPEND(skb_prev, skb, &txq->sk_list);
        else
            skb_queue_tail(&txq->sk_list, skb);

#ifdef CONFIG_AML_FULLMAC
        // to update for SOFTMAC
        aml_txq_start_cleanup_timer(aml_hw, txq->sta);
#endif
    } else {
        if (txq->last_retry_skb)
            SKB_APPEND(txq->last_retry_skb, skb, &txq->sk_list);
        else
            skb_queue_head(&txq->sk_list, skb);

        txq->last_retry_skb = skb;
        txq->nb_retry++;
    }

    trace_txq_queue_skb(skb, txq, retry);

    /* Flowctrl corresponding netdev queue if needed */
#ifdef CONFIG_AML_FULLMAC
    /* If too many buffer are queued for this TXQ stop netdev queue */
    if ((txq->ndev_idx != NDEV_NO_TXQ) &&
        (skb_queue_len(&txq->sk_list) > AML_NDEV_FLOW_CTRL_STOP)) {
        txq->status |= AML_TXQ_NDEV_FLOW_CTRL;
        netif_stop_subqueue(txq->ndev, txq->ndev_idx);
        trace_txq_flowctrl_stop(txq);
    }
#else /* ! CONFIG_AML_FULLMAC */

    if (!retry && ++txq->hwq->len == txq->hwq->len_stop) {
         trace_hwq_flowctrl_stop(txq->hwq->id);
         ieee80211_stop_queue(aml_hw->hw, txq->hwq->id);
         aml_hw->stats->queues_stops++;
     }
#endif /* CONFIG_AML_FULLMAC */

    /* add it in the hwq list if not stopped and not yet present */
    if (!aml_txq_is_stopped(txq)) {
        aml_txq_add_to_hw_list(txq);
        return 1;
    }

    return 0;
}

/**
 * aml_txq_confirm_any - Process buffer confirmed by fw
 *
 * @aml_hw: Driver main data
 * @txq: TX Queue
 * @hwq: HW Queue
 * @sw_txhdr: software descriptor of the confirmed packet
 *
 * Process a buffer returned by the fw. It doesn't check buffer status
 * and only does systematic counter update:
 * - hw credit
 * - buffer pushed to fw
 *
 * To be called with tx_lock hold
 */
void aml_txq_confirm_any(struct aml_hw *aml_hw, struct aml_txq *txq,
                          struct aml_hwq *hwq, struct aml_sw_txhdr *sw_txhdr)
{
    int user = 0;

#ifdef CONFIG_AML_MUMIMO_TX
    int group_id;

    user = AML_MUMIMO_INFO_POS_ID(sw_txhdr->desc.api.host.mumimo_info);
    group_id = AML_MUMIMO_INFO_GROUP_ID(sw_txhdr->desc.api.host.mumimo_info);

    if ((txq->idx != TXQ_INACTIVE) &&
        (txq->pkt_pushed[user] == 1) &&
        (txq->status & AML_TXQ_STOP_MU_POS))
        aml_txq_start(txq, AML_TXQ_STOP_MU_POS);

#endif /* CONFIG_AML_MUMIMO_TX */

    if (txq->pkt_pushed[user])
        txq->pkt_pushed[user]--;

    hwq->credits[user]++;
    hwq->need_processing = true;
    aml_hw->stats->cfm_balance[hwq->id]--;
}

/******************************************************************************
 * HWQ processing
 *****************************************************************************/
static inline
bool aml_txq_take_mu_lock(struct aml_hw *aml_hw)
{
    bool res = false;
#ifdef CONFIG_AML_MUMIMO_TX
    if (aml_hw->mod_params->mutx)
        res = (down_trylock(&aml_hw->mu.lock) == 0);
#endif /* CONFIG_AML_MUMIMO_TX */
    return res;
}

static inline
void aml_txq_release_mu_lock(struct aml_hw *aml_hw)
{
#ifdef CONFIG_AML_MUMIMO_TX
    up(&aml_hw->mu.lock);
#endif /* CONFIG_AML_MUMIMO_TX */
}

#ifdef CONFIG_AML_MUMIMO_TX
static inline
void aml_txq_set_mu_info(struct aml_hw *aml_hw, struct aml_txq *txq,
                          int group_id, int pos)
{
    trace_txq_select_mu_group(txq, group_id, pos);
    if (group_id) {
        txq->mumimo_info = group_id | (pos << 6);
        aml_mu_set_active_group(aml_hw, group_id);
    } else
        txq->mumimo_info = 0;
}
#endif /* CONFIG_AML_MUMIMO_TX */

static inline
s8 aml_txq_get_credits(struct aml_txq *txq)
{
    s8 cred = txq->credits;
    /* if destination is in PS mode, push_limit indicates the maximum
       number of packet that can be pushed on this txq. */
    if (txq->push_limit && (cred > txq->push_limit)) {
        cred = txq->push_limit;
    }
    return cred;
}

/**
 * skb_queue_extract - Extract buffer from skb list
 *
 * @list: List of skb to extract from
 * @head: List of skb to append to
 * @nb_elt: Number of skb to extract
 *
 * extract the first @nb_elt of @list and append them to @head
 * It is assume that:
 * - @list contains more that @nb_elt
 * - There is no need to take @list nor @head lock to modify them
 */
static inline void skb_queue_extract(struct sk_buff_head *list,
                                     struct sk_buff_head *head, int nb_elt)
{
    int i;
    struct sk_buff *first, *last, *ptr;

    first = ptr = list->next;
    for (i = 0; i < nb_elt; i++) {
        ptr = ptr->next;
    }
    last = ptr->prev;

    /* unlink nb_elt in list */
    list->qlen -= nb_elt;
    list->next = ptr;
    ptr->prev = (struct sk_buff *)list;

    /* append nb_elt at end of head */
    head->qlen += nb_elt;
    last->next = (struct sk_buff *)head;
    head->prev->next = first;
    first->prev = head->prev;
    head->prev = last;
}


#ifdef CONFIG_MAC80211_TXQ
/**
 * aml_txq_mac80211_dequeue - Dequeue buffer from mac80211 txq and
 *                             add them to push list
 *
 * @aml_hw: Main driver data
 * @sk_list: List of buffer to push (initialized without lock)
 * @txq: TXQ to dequeue buffers from
 * @max: Max number of buffer to dequeue
 *
 * Dequeue buffer from mac80211 txq, prepare them for transmission and chain them
 * to the list of buffer to push.
 *
 * @return true if no more buffer are queued in mac80211 txq and false otherwise.
 */
static bool aml_txq_mac80211_dequeue(struct aml_hw *aml_hw,
                                      struct sk_buff_head *sk_list,
                                      struct aml_txq *txq, int max)
{
    struct ieee80211_txq *mac_txq;
    struct sk_buff *skb;
    unsigned long mac_txq_len;

    if (txq->nb_ready_mac80211 == NOT_MAC80211_TXQ)
        return true;

    mac_txq = container_of((void *)txq, struct ieee80211_txq, drv_priv);

    for (; max > 0; max--) {
        skb = aml_tx_dequeue_prep(aml_hw, mac_txq);
        if (skb == NULL)
            return true;

        __skb_queue_tail(sk_list, skb);
    }

    /* re-read mac80211 txq current length.
       It is mainly for debug purpose to trace dropped packet. There is no
       problems to have nb_ready_mac80211 != actual mac80211 txq length */
    ieee80211_txq_get_depth(mac_txq, &mac_txq_len, NULL);
    if (txq->nb_ready_mac80211 > mac_txq_len)
        trace_txq_drop(txq, txq->nb_ready_mac80211 - mac_txq_len);
    txq->nb_ready_mac80211 = mac_txq_len;

    return (txq->nb_ready_mac80211 == 0);
}
#endif

/**
 * aml_txq_get_skb_to_push() - Get list of buffer to push for one txq
 *
 * @aml_hw: main driver data
 * @hwq: HWQ on wich buffers will be pushed
 * @txq: TXQ to get buffers from
 * @user: user postion to use
 * @sk_list_push: list to update
 *
 *
 * This function will returned a list of buffer to push for one txq.
 * It will take into account the number of credit of the HWQ for this user
 * position and TXQ (and push_limit).
 * This allow to get a list that can be pushed without having to test for
 * hwq/txq status after each push
 *
 * If a MU group has been selected for this txq, it will also update the
 * counter for the group
 *
 * @return true if txq no longer have buffer ready after the ones returned.
 *         false otherwise
 */
static bool aml_txq_get_skb_to_push(struct aml_hw *aml_hw, struct aml_hwq *hwq,
                              struct aml_txq *txq, int user,
                              struct sk_buff_head *sk_list_push)
{
    int nb_ready = skb_queue_len(&txq->sk_list);
    int credits = min_t(int, aml_txq_get_credits(txq), hwq->credits[user]);
    bool res = false;

    __skb_queue_head_init(sk_list_push);

    if (credits >= nb_ready) {
        skb_queue_splice_init(&txq->sk_list, sk_list_push);
#ifdef CONFIG_MAC80211_TXQ
        res = aml_txq_mac80211_dequeue(aml_hw, sk_list_push, txq, credits - nb_ready);
        credits = skb_queue_len(sk_list_push);
#else
        res = true;
        credits = nb_ready;
#endif
    } else {
        skb_queue_extract(&txq->sk_list, sk_list_push, credits);

        /* When processing PS service period (i.e. push_limit != 0), no longer
           process this txq if the buffers extracted will complete the SP for
           this txq */
        if (txq->push_limit && (credits == txq->push_limit))
            res = true;
    }

    aml_mu_set_active_sta(aml_hw, aml_txq_2_sta(txq), credits);

    return res;
}

/**
 * aml_txq_select_user - Select User queue for a txq
 *
 * @aml_hw: main driver data
 * @mu_lock: true is MU lock is taken
 * @txq: TXQ to select MU group for
 * @hwq: HWQ for the TXQ
 * @user: Updated with user position selected
 *
 * @return false if it is no possible to process this txq.
 *         true otherwise
 *
 * This function selects the MU group to use for a TXQ.
 * The selection is done as follow:
 *
 * - return immediately for STA that don't belongs to any group and select
 *   group 0 / user 0
 *
 * - If MU tx is disabled (by user mutx_on, or because mu group are being
 *   updated !mu_lock), select group 0 / user 0
 *
 * - Use the best group selected by @aml_mu_group_sta_select.
 *
 *   Each time a group is selected (except for the first case where sta
 *   doesn't belongs to a MU group), the function checks that no buffer is
 *   pending for this txq on another user position. If this is the case stop
 *   the txq (aml_txq_stop_MU_POS) and return false.
 *
 */
static
bool aml_txq_select_user(struct aml_hw *aml_hw, bool mu_lock,
                          struct aml_txq *txq, struct aml_hwq *hwq, int *user)
{
    int pos = 0;
#ifdef CONFIG_AML_MUMIMO_TX
    int id, group_id = 0;
    struct aml_sta *sta = aml_txq_2_sta(txq);

    /* for sta that belong to no group return immediately */
    if (!sta || !sta->group_info.cnt)
        goto end;

    /* If MU is disabled, need to check user */
    if (!aml_hw->mod_params->mutx_on || !mu_lock)
        goto check_user;

    /* Use the "best" group selected */
    group_id = sta->group_info.group;

    if (group_id > 0)
        pos = aml_mu_group_sta_get_pos(aml_hw, sta, group_id);

  check_user:
    /* check that we can push on this user position */
#if CONFIG_USER_MAX == 2
    id = (pos + 1) & 0x1;
    if (txq->pkt_pushed[id]) {
        aml_txq_stop(txq, AML_TXQ_STOP_MU_POS);
        return false;
    }

#else
    for (id = 0 ; id < CONFIG_USER_MAX ; id++) {
        if (id != pos && txq->pkt_pushed[id]) {
            aml_txq_stop(txq, AML_TXQ_STOP_MU_POS);
            return false;
        }
    }
#endif

  end:
    aml_txq_set_mu_info(aml_hw, txq, group_id, pos);
#endif /* CONFIG_AML_MUMIMO_TX */

    *user = pos;
    return true;
}

/**
 * aml_hwq_process - Process one HW queue list
 *
 * @aml_hw: Driver main data
 * @hw_queue: HW queue index to process
 *
 * The function will iterate over all the TX queues linked in this HW queue
 * list. For each TX queue, push as many buffers as possible in the HW queue.
 * (NB: TX queue have at least 1 buffer, otherwise it wouldn't be in the list)
 * - If TX queue no longer have buffer, remove it from the list and check next
 *   TX queue
 * - If TX queue no longer have credits or has a push_limit (PS mode) and it
 *   is reached , remove it from the list and check next TX queue
 * - If HW queue is full, update list head to start with the next TX queue on
 *   next call if current TX queue already pushed "too many" pkt in a row, and
 *   return
 *
 * To be called when HW queue list is modified:
 * - when a buffer is pushed on a TX queue
 * - when new credits are received
 * - when a STA returns from Power Save mode or receives traffic request.
 * - when Channel context change
 *
 * To be called with tx_lock hold
 */
#define ALL_HWQ_MASK  ((1 << CONFIG_USER_MAX) - 1)

void aml_hwq_process(struct aml_hw *aml_hw, struct aml_hwq *hwq)
{
    struct aml_txq *txq, *next;
    int user, credit_map = 0;
    bool mu_enable;

    trace_process_hw_queue(hwq);

    hwq->need_processing = false;

    mu_enable = aml_txq_take_mu_lock(aml_hw);
    if (!mu_enable)
        credit_map = ALL_HWQ_MASK - 1;

    list_for_each_entry_safe(txq, next, &hwq->list, sched_list) {
        struct aml_txhdr *txhdr = NULL;
        struct sk_buff_head sk_list_push;
        struct sk_buff *skb;
        bool txq_empty;

        trace_process_txq(txq);

        /* sanity check for debug */
        if ((!(txq->status & AML_TXQ_IN_HWQ_LIST))
            || (txq->idx == TXQ_INACTIVE)
            || (txq->credits <= 0)
            || (!aml_txq_skb_ready(txq))) {
            AML_INFO("error, txq not in hwlist=%d, idx inactive=%d,"
                    "no txq credits=%d, txq skb list empty=%d",
                    (!(txq->status & AML_TXQ_IN_HWQ_LIST)),
                    (txq->idx == TXQ_INACTIVE),
                    (txq->credits <= 0),
                    (!aml_txq_skb_ready(txq)));
            continue;
        }

        if (!aml_txq_select_user(aml_hw, mu_enable, txq, hwq, &user))
            continue;

        if (!hwq->credits[user]) {
            credit_map |= BIT(user);
            if (credit_map == ALL_HWQ_MASK)
                break;
            continue;
        }

        txq_empty = aml_txq_get_skb_to_push(aml_hw, hwq, txq, user,
                                             &sk_list_push);

        while ((skb = __skb_dequeue(&sk_list_push)) != NULL) {
            txhdr = (struct aml_txhdr *)skb->data;
            aml_tx_push(aml_hw, txhdr, 0);
            aml_check_tcpack_skb(aml_hw, skb, skb->len);
        }
        if ((aml_bus_type == PCIE_MODE) && (g_txdesc_trigger.txdesc_cnt > 0)) {
            ipc_app2emb_trigger_setf(aml_hw, IPC_IRQ_A2E_TXDESC);
            g_txdesc_trigger.txdesc_cnt = 0;
            if (g_txdesc_trigger.dynamic_cnt == 0) {
                if (g_txdesc_trigger.tx_pcie_ths > 0) {
                    g_txdesc_trigger.tx_pcie_ths--;
                }
            } else {
                g_txdesc_trigger.dynamic_cnt--;
            }
        }

        if (txq_empty) {
            aml_txq_del_from_hw_list(txq);
            txq->pkt_sent = 0;
#if defined CONFIG_AML_SOFTMAC && defined CONFIG_AML_AMSDUS_TX
            if (txq->amsdu_ht_len_cap)
                ieee80211_amsdu_ctl(aml_hw->hw, txq->sta, txq->tid, NULL,
                                    0, 0, false);
#endif
        } else if ((hwq->credits[user] == 0) &&
                   aml_txq_is_scheduled(txq)) {
            /* txq not empty,
               - To avoid starving need to process other txq in the list
               - For better aggregation, need to send "as many consecutive
               pkt as possible" for the same txq
               ==> Add counter to trigger txq switch
            */
            if (txq->pkt_sent > hwq->size) {
                txq->pkt_sent = 0;
                list_rotate_left(&hwq->list);
            }
        }

#ifdef CONFIG_AML_FULLMAC
        /* Unable to complete PS traffic request because of hwq credit */
        if (txq->push_limit && txq->sta) {
            if (txq->ps_id == LEGACY_PS_ID) {
                /* for legacy PS abort SP and wait next ps-poll */
                txq->sta->ps.sp_cnt[txq->ps_id] -= txq->push_limit;
                txq->push_limit = 0;
            }
            /* for u-apsd need to complete the SP to send EOSP frame */
        }

        /* restart netdev queue if number of queued buffer is below threshold */
        if (unlikely(txq->status & AML_TXQ_NDEV_FLOW_CTRL) &&
            skb_queue_len(&txq->sk_list) < AML_NDEV_FLOW_CTRL_RESTART) {
            txq->status &= ~AML_TXQ_NDEV_FLOW_CTRL;
            netif_wake_subqueue(txq->ndev, txq->ndev_idx);
            trace_txq_flowctrl_restart(txq);
        }
#endif /* CONFIG_AML_FULLMAC */

    }

#ifdef CONFIG_AML_SOFTMAC
    if (hwq->len < hwq->len_start &&
        ieee80211_queue_stopped(aml_hw->hw, hwq->id)) {
        trace_hwq_flowctrl_start(hwq->id);
        ieee80211_wake_queue(aml_hw->hw, hwq->id);
    }
#endif /* CONFIG_AML_SOFTMAC */

    if (mu_enable)
        aml_txq_release_mu_lock(aml_hw);
}

/**
 * aml_hwq_process_all - Process all HW queue list
 *
 * @aml_hw: Driver main data
 *
 * Loop over all HWQ, and process them if needed
 * To be called with tx_lock hold
 */
void aml_hwq_process_all(struct aml_hw *aml_hw)
{
    int id;

    aml_mu_group_sta_select(aml_hw);

    for (id = ARRAY_SIZE(aml_hw->hwq) - 1; id >= 0 ; id--) {
        if (aml_hw->hwq[id].need_processing) {
            aml_hwq_process(aml_hw, &aml_hw->hwq[id]);
        }
    }
}

const int nx_txdesc_cnt_ext[] =
{
    TX_MAX_CNT,
    TX_MAX_CNT,
    TX_MAX_CNT,
    TX_MAX_CNT,
#if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4,
#endif
};

/**
 * aml_hwq_init - Initialize all hwq structures
 *
 * @aml_hw: Driver main data
 *
 */
void aml_hwq_init(struct aml_hw *aml_hw)
{
    int i, j;

    for (i = 0; i < ARRAY_SIZE(aml_hw->hwq); i++) {
        struct aml_hwq *hwq = &aml_hw->hwq[i];

        for (j = 0 ; j < CONFIG_USER_MAX; j++) {
#ifdef CONFIG_CREDIT124
            hwq->credits[j] = nx_txdesc_cnt_ext[i];
#else
            hwq->credits[j] = (aml_bus_type == PCIE_MODE) ? nx_txdesc_cnt[i] : nx_txdesc_cnt_ext[i];
#endif
        }
        hwq->id = i;
#ifdef CONFIG_CREDIT124
        hwq->size = nx_txdesc_cnt_ext[i];
#else
        hwq->size = (aml_bus_type == PCIE_MODE) ? nx_txdesc_cnt[i] : nx_txdesc_cnt_ext[i];
#endif
        INIT_LIST_HEAD(&hwq->list);

#ifdef CONFIG_AML_SOFTMAC
        hwq->len = 0;
        hwq->len_stop = nx_txdesc_cnt[i] * 2;
        hwq->len_start = hwq->len_stop / 4;
#endif
    }
}

int aml_txq_is_empty(struct aml_vif *aml_vif, struct aml_sta *aml_sta)
{
    struct aml_txq *txq;
    int tid;
    int i;

    spin_lock_bh(&aml_vif->vif_lock);
    foreach_sta_txq_safe(aml_sta, txq, tid, aml_vif->aml_hw) {
        for (i = 0; i < CONFIG_USER_MAX ; i++) {
            if (txq->pkt_pushed[i]) {
                spin_unlock_bh(&aml_vif->vif_lock);
                return 0;
            }
        }
    }
    spin_unlock_bh(&aml_vif->vif_lock);

    return 1;
}

int aml_unktxq_is_empty(struct aml_vif *aml_vif)
{
    int i;
    struct aml_txq *txq;
    if (!aml_vif) {
        return 1;
    }
    txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
    for (i = 0; i < CONFIG_USER_MAX ; i++) {
        if (txq->pkt_pushed[i]) {
            return 0;
        }
    }
    return 1;
}
