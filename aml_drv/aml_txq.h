/**
 ****************************************************************************************
 *
 * @file aml_txq.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */
#ifndef _AML_TXQ_H_
#define _AML_TXQ_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/ieee80211.h>

#ifdef CONFIG_AML_SOFTMAC
#include <net/mac80211.h>
#include "aml_baws.h"

/**
 * Softmac TXQ configuration
 *  - STA have one TXQ per TID
 *  - VIF have one TXQ per HW queue
 *
 * Txq mapping looks like
 * for NX_REMOTE_STA_MAX=10 and NX_VIRT_DEV_MAX=4
 *
 * | TXQ | VIF |   STA |  TID | HWQ |
 * |-----+-----+-------+------+-----|-
 * |   0 |     |     0 |    0 |   1 | 16 TXQ per STA
 * |   1 |     |     0 |    1 |   0 |
 * |   2 |     |     0 |    2 |   0 |
 * |   3 |     |     0 |    3 |   1 |
 * |   4 |     |     0 |    4 |   2 |
 * |   5 |     |     0 |    5 |   2 |
 * |   6 |     |     0 |    6 |   3 |
 * |   7 |     |     0 |    7 |   3 |
 * |   8 |     |     0 |    8 |   1 |
 * |  ...|     |       |      |     |
 * |  16 |     |     0 |   16 |   1 |
 * |-----+-----+-------+------+-----|-
 * | ... |     |       |      |     | same for all STAs
 * |-----+-----+-------+------+-----|-
 * | 160 |   0 |       |      |   0 | 5 TXQ per VIF
 * | ... |     |       |      |     |
 * | 164 |   0 |       |      |   4 |
 * |-----+-----+-------+------+-----|-
 * | ... |     |       |      |     | same for all VIFs
 * |-----+-----+-------+------+-----|-
 *
 * NOTE: When using CONFIG_MAC80211_TXQ only one TXQ is allocated by mac80211
 * for the VIF (associated to BE ac). To avoid too much differences with case
 * where TXQ are allocated by the driver the "missing" VIF TXQs are allocated
 * by the driver. Actually driver also allocates txq for BE (to avoid having
 * modify ac parameter to access the TXQ) but this one is never used.
 * Driver check if nb_ready_mac80211 field is equal to NOT_MAC80211_TXQ in
 * order to distinguish non mac80211 txq.
 * When the txq interface (.wake_tx_queue) is used only the TXQ
 * allocated by mac80211 will be used and thus BE access category will always
 * be used. When "VIF" frames needs to be pushed on different access category
 * mac80211 will use the tx interface (.tx) and in this case driver will select
 * the txq associated to the requested access category.
 */
#define NX_NB_TID_PER_STA IEEE80211_NUM_TIDS
#define NX_NB_TXQ_PER_STA NX_NB_TID_PER_STA
#define NX_NB_TXQ_PER_VIF NX_TXQ_CNT
#define NX_NB_TXQ ((NX_NB_TXQ_PER_STA * NX_REMOTE_STA_MAX) +    \
                   (NX_NB_TXQ_PER_VIF * NX_VIRT_DEV_MAX))

#define NX_FIRST_VIF_TXQ_IDX (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA)

#define NOT_MAC80211_TXQ ULONG_MAX

#else /* i.e. #ifdef CONFIG_AML_FULLMAC */
/**
 * Fullmac TXQ configuration:
 *  - STA: 1 TXQ per TID (limited to 8)
 *         1 TXQ for bufferable MGT frames
 *  - VIF: 1 TXQ for Multi/Broadcast +
 *         1 TXQ for MGT for unknown STAs or non-bufferable MGT frames
 *  - 1 TXQ for offchannel transmissions
 *
 *
 * Txq mapping looks like
 * for NX_REMOTE_STA_MAX=10 and NX_VIRT_DEV_MAX=4
 *
 * | TXQ | NDEV_ID | VIF |   STA |  TID | HWQ |
 * |-----+---------+-----+-------+------+-----|-
 * |   0 |       0 |     |     0 |    0 |   1 | 9 TXQ per STA
 * |   1 |       1 |     |     0 |    1 |   0 | (8 data + 1 mgmt)
 * |   2 |       2 |     |     0 |    2 |   0 |
 * |   3 |       3 |     |     0 |    3 |   1 |
 * |   4 |       4 |     |     0 |    4 |   2 |
 * |   5 |       5 |     |     0 |    5 |   2 |
 * |   6 |       6 |     |     0 |    6 |   3 |
 * |   7 |       7 |     |     0 |    7 |   3 |
 * |   8 |     N/A |     |     0 | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * | ... |         |     |       |      |     | Same for all STAs
 * |-----+---------+-----+-------+------+-----|-
 * |  90 |      80 |   0 | BC/MC |    0 | 1/4 | 1 TXQ for BC/MC per VIF
 * | ... |         |     |       |      |     |
 * |  93 |      80 |   3 | BC/MC |    0 | 1/4 |
 * |-----+---------+-----+-------+------+-----|-
 * |  94 |     N/A |   0 |   N/A | MGMT |   3 | 1 TXQ for unknown STA per VIF
 * | ... |         |     |       |      |     |
 * |  97 |     N/A |   3 |   N/A | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * |  98 |     N/A |     |   N/A | MGMT |   3 | 1 TXQ for offchannel frame
 */
#define NX_NB_TID_PER_STA 8
#define NX_NB_TXQ_PER_STA (NX_NB_TID_PER_STA + 1)
#define NX_NB_TXQ_PER_VIF 2
#define NX_NB_TXQ ((NX_NB_TXQ_PER_STA * NX_REMOTE_STA_MAX) +    \
                   (NX_NB_TXQ_PER_VIF * NX_VIRT_DEV_MAX) + 1)

#define NX_FIRST_VIF_TXQ_IDX (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA)
#define NX_FIRST_BCMC_TXQ_IDX  NX_FIRST_VIF_TXQ_IDX
#define NX_FIRST_UNK_TXQ_IDX  (NX_FIRST_BCMC_TXQ_IDX + NX_VIRT_DEV_MAX)

#define NX_OFF_CHAN_TXQ_IDX (NX_FIRST_VIF_TXQ_IDX +                     \
                             (NX_VIRT_DEV_MAX * NX_NB_TXQ_PER_VIF))
#define NX_BCMC_TXQ_TYPE 0
#define NX_UNK_TXQ_TYPE  1

/**
 * Each data TXQ is a netdev queue. TXQ to send MGT are not data TXQ as
 * they did not recieved buffer from netdev interface.
 * Need to allocate the maximum case.
 * AP : all STAs + 1 BC/MC
 */
#define NX_NB_NDEV_TXQ ((NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX) + 1 )
#define NX_BCMC_TXQ_NDEV_IDX (NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX)
#define NX_STA_NDEV_IDX(tid, sta_idx) ((tid) + (sta_idx) * NX_NB_TID_PER_STA)
#define NDEV_NO_TXQ 0xffff
#if (NX_NB_NDEV_TXQ >= NDEV_NO_TXQ)
#error("Need to increase struct aml_txq->ndev_idx size")
#endif

/* stop netdev queue when number of queued buffers if greater than this  */
#define AML_NDEV_FLOW_CTRL_STOP    10000
/* restart netdev queue when number of queued buffers is lower than this */
#define AML_NDEV_FLOW_CTRL_RESTART 9800

#endif /*  CONFIG_AML_SOFTMAC */

#define TXQ_INACTIVE 0xffff
#if (NX_NB_TXQ >= TXQ_INACTIVE)
#error("Need to increase struct aml_txq->idx size")
#endif

#define NX_TXQ_INITIAL_CREDITS 4

#define AML_TXQ_CLEANUP_INTERVAL (10 * HZ) //10s in jiffies
#define AML_TXQ_MAX_QUEUE_JIFFIES (20 * HZ)

/**
 * TXQ tid sorted by decreasing priority
 */
extern const int nx_tid_prio[NX_NB_TXQ_PER_STA];

/**
 * struct aml_hwq - Structure used to save information relative to
 *                   an AC TX queue (aka HW queue)
 * @list: List of TXQ, that have buffers ready for this HWQ
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW )
 * @id Id of the HWQ among AML_HWQ_....
 * @size size of the queue
 * @need_processing Indicate if hwq should be processed
 * @len number of packet ready to be pushed to fw for this HW queue
 * @len_stop threshold to stop mac80211(i.e. netdev) queues. Stop queue when
 *           driver has more than @len_stop packets ready.
 * @len_start threshold to wake mac8011 queues. Wake queue when driver has
 *            less than @len_start packets ready.
 */
struct aml_hwq {
    struct list_head list;
    u8 credits[CONFIG_USER_MAX];
    u8 size;
    u8 id;
    bool need_processing;
#ifdef CONFIG_AML_SOFTMAC
    u8 len;
    u8 len_stop;
    u8 len_start;
#endif /* CONFIG_AML_SOFTMAC */
};

/**
 * enum aml_push_flags - Flags of pushed buffer
 *
 * @AML_PUSH_RETRY Pushing a buffer for retry
 * @AML_PUSH_IMMEDIATE Pushing a buffer without queuing it first
 */
enum aml_push_flags {
    AML_PUSH_RETRY  = BIT(0),
    AML_PUSH_IMMEDIATE = BIT(1),
};

/**
 * enum aml_txq_flags - TXQ status flag
 *
 * @AML_TXQ_IN_HWQ_LIST: The queue is scheduled for transmission
 * @AML_TXQ_STOP_FULL: No more credits for the queue
 * @AML_TXQ_STOP_CSA: CSA is in progress
 * @AML_TXQ_STOP_STA_PS: Destiniation sta is currently in power save mode
 * @AML_TXQ_STOP_VIF_PS: Vif owning this queue is currently in power save mode
 * @AML_TXQ_STOP_CHAN: Channel of this queue is not the current active channel
 * @AML_TXQ_STOP_MU_POS: TXQ is stopped waiting for all the buffers pushed to
 *                       fw to be confirmed
 * @AML_TXQ_STOP: All possible reason to have a txq stopped
 * @AML_TXQ_NDEV_FLOW_CTRL: associated netdev queue is currently stopped.
 *                          Note: when a TXQ is flowctrl it is NOT stopped
 */
enum aml_txq_flags {
    AML_TXQ_IN_HWQ_LIST  = BIT(0),
    AML_TXQ_STOP_FULL    = BIT(1),
    AML_TXQ_STOP_CSA     = BIT(2),
    AML_TXQ_STOP_STA_PS  = BIT(3),
    AML_TXQ_STOP_VIF_PS  = BIT(4),
    AML_TXQ_STOP_CHAN    = BIT(5),
    AML_TXQ_STOP_MU_POS  = BIT(6),
    AML_TXQ_NDEV_FLOW_CTRL = BIT(7),
    AML_TXQ_STOP_COEX_INACTIVE = BIT(8),
    AML_TXQ_STOP         = (AML_TXQ_STOP_FULL | AML_TXQ_STOP_CSA |
                             AML_TXQ_STOP_STA_PS | AML_TXQ_STOP_VIF_PS |
                             AML_TXQ_STOP_CHAN | AML_TXQ_STOP_COEX_INACTIVE) ,
};


/**
 * struct aml_txq - Structure used to save information relative to
 *                   a RA/TID TX queue
 *
 * @idx: Unique txq idx. Set to TXQ_INACTIVE if txq is not used.
 * @status: bitfield of @aml_txq_flags.
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW).
 * @pkt_sent: number of consecutive pkt sent without leaving HW queue list
 * @pkt_pushed: number of pkt currently pending for transmission confirmation
 * @sched_list: list node for HW queue schedule list (aml_hwq.list)
 * @sk_list: list of buffers to push to fw
 * @last_retry_skb: pointer on the last skb in @sk_list that is a retry.
 *                  (retry skb are stored at the beginning of the list)
 *                  NULL if no retry skb is queued in @sk_list
 * @nb_retry: Number of retry packet queued.
 * @hwq: Pointer on the associated HW queue.
 * @push_limit: number of packet to push before removing the txq from hwq list.
 *              (we always have push_limit < skb_queue_len(sk_list))
 * @tid: TID
 *
 * SOFTMAC specific:
 * @baw: Block Ack window information
 * @amsdu_anchor: pointer to aml_sw_txhdr of the first subframe of the A-MSDU.
 *                NULL if no A-MSDU frame is in construction
 * @amsdu_ht_len_cap:
 * @amsdu_vht_len_cap:
 * @nb_ready_mac80211: Number of buffer ready in mac80211 txq
 *
 * FULLMAC specific
 * @ps_id: Index to use for Power save mode (LEGACY or UAPSD)
 * @ndev_idx: txq idx from netdev point of view (0xFF for non netdev queue)
 * @ndev: pointer to ndev of the corresponding vif
 * @amsdu: pointer to aml_sw_txhdr of the first subframe of the A-MSDU.
 *         NULL if no A-MSDU frame is in construction
 * @amsdu_len: Maximum size allowed for an A-MSDU. 0 means A-MSDU not allowed
 */
struct aml_txq {
    u16 idx;
    u16 status;
    s8 credits;
    u8 pkt_sent;
    u8 pkt_pushed[CONFIG_USER_MAX];
    struct list_head sched_list;
    struct sk_buff_head sk_list;
    struct sk_buff *last_retry_skb;
    struct aml_hwq *hwq;
    int nb_retry;
    u8 push_limit;
    u8 tid;
#ifdef CONFIG_MAC80211_TXQ
    unsigned long nb_ready_mac80211;
#endif
#ifdef CONFIG_AML_SOFTMAC
    struct aml_baw baw;
    struct ieee80211_sta *sta;
#ifdef CONFIG_AML_AMSDUS_TX
    struct aml_sw_txhdr *amsdu_anchor;
    u16 amsdu_ht_len_cap;
    u16 amsdu_vht_len_cap;
#endif /* CONFIG_AML_AMSDUS_TX */
#else /* ! CONFIG_AML_SOFTMAC */
    struct aml_sta *sta;
    u8 ps_id;
    u16 ndev_idx;
    struct net_device *ndev;
#ifdef CONFIG_AML_AMSDUS_TX
    struct aml_sw_txhdr *amsdu;
    u16 amsdu_len;
#endif /* CONFIG_AML_AMSDUS_TX */
#endif /* CONFIG_AML_SOFTMAC */
#ifdef CONFIG_AML_MUMIMO_TX
    u8 mumimo_info;
#endif
    spinlock_t txq_lock;
};

struct aml_sta;
struct aml_vif;
struct aml_hw;
struct aml_sw_txhdr;

#ifdef CONFIG_AML_MUMIMO_TX
#define AML_TXQ_GROUP_ID(txq) ((txq)->mumimo_info & 0x3f)
#define AML_TXQ_POS_ID(txq)   (((txq)->mumimo_info >> 6) & 0x3)
#else
#define AML_TXQ_GROUP_ID(txq) 0
#define AML_TXQ_POS_ID(txq)   0
#endif /* CONFIG_AML_MUMIMO_TX */

static inline bool aml_txq_is_stopped(struct aml_txq *txq)
{
    return (txq->status & AML_TXQ_STOP);
}

static inline bool aml_txq_is_full(struct aml_txq *txq)
{
    return (txq->status & AML_TXQ_STOP_FULL);
}

static inline bool aml_txq_is_ps(struct aml_txq *txq)
{
    return (txq->status & AML_TXQ_STOP_STA_PS);
}

static inline bool aml_txq_is_scheduled(struct aml_txq *txq)
{
    return (txq->status & AML_TXQ_IN_HWQ_LIST);
}

/**
 * aml_txq_is_ready_for_push - Check if a TXQ is ready for push
 *
 * @txq: txq pointer
 *
 * if
 * - txq is not stopped (or has a push limit)
 * - and hwq has credits
 * - and there is no buffer queued
 * then a buffer can be immediately pushed without having to queue it first
 *
 * Note: txq push limit is incremented as soon as a PS buffer is pushed by
 * mac80211.
 * @return: true if the 3 conditions are met and false otherwise.
 */
static inline bool aml_txq_is_ready_for_push(struct aml_txq *txq)
{
    return ((!aml_txq_is_stopped(txq) || txq->push_limit) &&
            txq->hwq->credits[AML_TXQ_POS_ID(txq)] > 0 &&
            skb_queue_empty(&txq->sk_list));
}

/**
 * foreach_sta_txq - Macro to iterate over all TXQ of a STA in increasing
 *                   TID order
 *
 * @sta: pointer to aml_sta
 * @txq: pointer to aml_txq updated with the next TXQ at each iteration
 * @tid: int updated with the TXQ tid at each iteration
 * @aml_hw: main driver data
 */
#ifdef CONFIG_MAC80211_TXQ
#define foreach_sta_txq(sta, txq, tid, aml_hw)                         \
    for (tid = 0, txq = aml_txq_sta_get(sta, 0);                       \
         txq && tid < NX_NB_TXQ_PER_STA;                                       \
         tid++, txq = aml_txq_sta_get(sta, tid))

#elif defined(CONFIG_AML_SOFTMAC)
#define foreach_sta_txq(sta, txq, tid, aml_hw)                         \
    for (tid = 0, txq = &sta->txqs[0];                                  \
         txq && tid < NX_NB_TXQ_PER_STA;                                       \
         tid++, txq++)

#else /* CONFIG_AML_FULLMAC */
#define foreach_sta_txq(sta, txq, tid, aml_hw)                          \
    for (tid = 0, txq = aml_txq_sta_get(sta, 0, aml_hw);               \
         txq && tid < (is_multicast_sta(sta->sta_idx) ? 1 : NX_NB_TXQ_PER_STA); \
         tid++, txq++)

#define foreach_sta_txq_safe(sta, txq, tid, aml_hw)                          \
    for (tid = 0, txq = aml_txq_sta_get(sta, 0, aml_hw);               \
         txq && (tid < (is_multicast_sta(sta->sta_idx) ? 1 : NX_NB_TXQ_PER_STA)); \
         tid++, txq++)
#endif

/**
 * foreach_sta_txq_prio - Macro to iterate over all TXQ of a STA in
 *                        decreasing priority order
 *
 * @sta: pointer to aml_sta
 * @txq: pointer to aml_txq updated with the next TXQ at each iteration
 * @tid: int updated with the TXQ tid at each iteration
 * @i: int updated with ieration count
 * @aml_hw: main driver data
 *
 * Note: For fullmac txq for mgmt frame is skipped
 */
#ifdef CONFIG_AML_SOFTMAC
#define foreach_sta_txq_prio(sta, txq, tid, i, aml_hw)                 \
    for (i = 0, tid = nx_tid_prio[0], txq = aml_txq_sta_get(sta, tid); \
         i < NX_NB_TXQ_PER_STA;                                         \
         i++, tid = nx_tid_prio[i % NX_NB_TXQ_PER_STA], txq = aml_txq_sta_get(sta, tid))
#else /* CONFIG_AML_FULLMAC */
#define foreach_sta_txq_prio(sta, txq, tid, i, aml_hw)                          \
    for (i = 0, tid = nx_tid_prio[0], txq = aml_txq_sta_get(sta, tid, aml_hw); \
         i < NX_NB_TXQ_PER_STA;                                                  \
         i++, tid = nx_tid_prio[i % NX_NB_TXQ_PER_STA], txq = aml_txq_sta_get(sta, tid, aml_hw))

#define foreach_sta_txq_prio_safe(sta, txq, tid, i, aml_hw)                          \
    for (i = 0, tid = nx_tid_prio[0], txq = aml_txq_sta_get(sta, tid, aml_hw); \
         txq && i < NX_NB_TXQ_PER_STA;                                                  \
         i++, tid = nx_tid_prio[i % NX_NB_TXQ_PER_STA], txq = aml_txq_sta_get(sta, tid, aml_hw))
#endif

/**
 * foreach_vif_txq - Macro to iterate over all TXQ of a VIF (in AC order)
 *
 * @vif: pointer to aml_vif
 * @txq: pointer to aml_txq updated with the next TXQ at each iteration
 * @ac:  int updated with the TXQ ac at each iteration
 */
#ifdef CONFIG_MAC80211_TXQ
#define foreach_vif_txq(vif, txq, ac)                                   \
    for (ac = AML_HWQ_BK, txq = aml_txq_vif_get(vif, ac);             \
         ac < NX_NB_TXQ_PER_VIF;                                        \
         ac++, txq = aml_txq_vif_get(vif, ac))

#else
#define foreach_vif_txq(vif, txq, ac)                                   \
    for (ac = AML_HWQ_BK, txq = &vif->txqs[0];                         \
         ac < NX_NB_TXQ_PER_VIF;                                        \
         ac++, txq++)
#endif

#ifdef CONFIG_AML_SOFTMAC
struct aml_txq *aml_txq_sta_get(struct aml_sta *sta, u8 tid);
struct aml_txq *aml_txq_vif_get(struct aml_vif *vif, u8 ac);
#else
struct aml_txq *aml_txq_sta_get(struct aml_sta *sta, u8 tid,
                                  struct aml_hw * aml_hw);
struct aml_txq *aml_txq_vif_get(struct aml_vif *vif, u8 type);
#endif /* CONFIG_AML_SOFTMAC */

/**
 * aml_txq_vif_get_status - return status bits related to the vif
 *
 * @aml_vif: Pointer to vif structure
 */
static inline u8 aml_txq_vif_get_status(struct aml_vif *aml_vif)
{
    struct aml_txq *txq = aml_txq_vif_get(aml_vif, 0);
    return (txq->status & (AML_TXQ_STOP_CHAN | AML_TXQ_STOP_VIF_PS));
}

void aml_txq_vif_init(struct aml_hw * aml_hw, struct aml_vif *vif,
                       u8 status);
void aml_txq_vif_deinit(struct aml_hw * aml_hw, struct aml_vif *vif);
void aml_txq_sta_init(struct aml_hw * aml_hw, struct aml_sta *aml_sta,
                       u8 status);
void aml_txq_sta_deinit(struct aml_hw * aml_hw, struct aml_sta *aml_sta);
#ifdef CONFIG_AML_FULLMAC
void aml_txq_unk_vif_init(struct aml_vif *aml_vif);
void aml_txq_unk_vif_deinit(struct aml_vif *vif);
void aml_txq_offchan_init(struct aml_vif *aml_vif);
void aml_txq_offchan_deinit(struct aml_vif *aml_vif);
void aml_txq_tdls_vif_init(struct aml_vif *aml_vif);
void aml_txq_tdls_vif_deinit(struct aml_vif *vif);
void aml_txq_tdls_sta_start(struct aml_vif *aml_vif, u16 reason,
                             struct aml_hw *aml_hw);
void aml_txq_tdls_sta_stop(struct aml_vif *aml_vif, u16 reason,
                            struct aml_hw *aml_hw);
void aml_txq_prepare(struct aml_hw *aml_hw);
#endif


void aml_txq_add_to_hw_list(struct aml_txq *txq);
void aml_txq_del_from_hw_list(struct aml_txq *txq);
void aml_txq_stop(struct aml_txq *txq, u16 reason);
void aml_txq_start(struct aml_txq *txq, u16 reason);
void aml_txq_vif_start(struct aml_vif *vif, u16 reason,
                        struct aml_hw *aml_hw);
void aml_txq_vif_stop(struct aml_vif *vif, u16 reason,
                       struct aml_hw *aml_hw);

#ifdef CONFIG_AML_SOFTMAC
void aml_txq_sta_start(struct aml_sta *sta, u16 reason);
void aml_txq_sta_stop(struct aml_sta *sta, u16 reason);
void aml_txq_tdls_sta_start(struct aml_sta *aml_sta, u16 reason,
                             struct aml_hw *aml_hw);
void aml_txq_tdls_sta_stop(struct aml_sta *aml_sta, u16 reason,
                            struct aml_hw *aml_hw);
#else
void aml_txq_sta_start(struct aml_sta *sta, u16 reason,
                        struct aml_hw *aml_hw);
void aml_txq_sta_stop(struct aml_sta *sta, u16 reason,
                       struct aml_hw *aml_hw);
void aml_txq_offchan_start(struct aml_hw *aml_hw);
void aml_txq_sta_switch_vif(struct aml_sta *sta, struct aml_vif *old_vif,
                             struct aml_vif *new_vif);

#endif /* CONFIG_AML_SOFTMAC */

int aml_txq_queue_skb(struct sk_buff *skb, struct aml_txq *txq,
                       struct aml_hw *aml_hw,  bool retry,
                       struct sk_buff *skb_prev);
void aml_txq_confirm_any(struct aml_hw *aml_hw, struct aml_txq *txq,
                          struct aml_hwq *hwq, struct aml_sw_txhdr *sw_txhdr);
void aml_txq_drop_skb(struct aml_txq *txq,  struct sk_buff *skb, struct aml_hw *aml_hw, bool retry_packet);

void aml_hwq_init(struct aml_hw *aml_hw);
void aml_hwq_process(struct aml_hw *aml_hw, struct aml_hwq *hwq);
void aml_hwq_process_all(struct aml_hw *aml_hw);
int aml_txq_is_empty(struct aml_vif *aml_vif, struct aml_sta * aml_sta);
int aml_unktxq_is_empty(struct aml_vif *aml_vif);

#endif /* _AML_TXQ_H_ */
