/**
 ******************************************************************************
 *
 * @file aml_tx.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include <net/sock.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 0, 0)
#include <net/sch_generic.h>
#endif
#include "aml_defs.h"
#include "aml_tx.h"
#include "aml_msg_tx.h"
#include "aml_mesh.h"
#include "aml_events.h"
#include "aml_compat.h"
#include "share_mem_map.h"
#include "aml_utils.h"
#include "wifi_debug.h"
#include "sdio_common.h"
#include "wifi_top_addr.h"
#include "sg_common.h"
#include "aml_interface.h"
#include "w2_sdio.h"
#ifdef CONFIG_AML_POWER_SAVE_MODE
#include "aml_platform.h"
#endif
#include "aml_scc.h"

char sp_frame_status_trace[][25] = {
    "[SP FRAME TX] ",
    "[SP FRAME RX] ",
    "[SP FRAME TX SUC] ",
    "[SP FRAME TX FAIL] "
};

/*Table 31-DPP Public Action Frame Type [easy connect v2.0]*/
char dpp_pub_action_trace[][50] = {
    "Authentication Request",
    "Authentication Response",
    "Authentication Confirm",
    "Reserved",
    "Reserved",
    "Peer Discovery Request",
    "Peer Discovery Response",
    "PKEX Version 1 Exchange Request",
    "PKEX Exchange Response",
    "PKEX Commit-Reveal Request",
    "PKEX Commit-Reveal Response",
    "Configuration Result",
    "Connection Status Result",
    "Presence Announcement",
    "Reconfiguration Announcement",
    "Reconfiguration Authentication Request",
    "Reconfiguration Authentication Response",
    "Reconfiguration Authentication Confirm",
    "PKEX Exchange Request"
};

/******************************************************************************
 * Power Save functions
 *****************************************************************************/
/**
 * aml_set_traffic_status - Inform FW if traffic is available for STA in PS
 *
 * @aml_hw: Driver main data
 * @sta: Sta in PS mode
 * @available: whether traffic is buffered for the STA
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
  */
void aml_set_traffic_status(struct aml_hw *aml_hw,
                             struct aml_sta *sta,
                             bool available,
                             u8 ps_id)
{
    if (sta->tdls.active) {
        aml_send_tdls_peer_traffic_ind_req(aml_hw,
                                            aml_hw->vif_table[sta->vif_idx]);
    } else {
        bool uapsd = (ps_id != LEGACY_PS_ID);
        aml_send_me_traffic_ind(aml_hw, sta->sta_idx, uapsd, available);
        trace_ps_traffic_update(sta->sta_idx, available, uapsd);
    }
}

/**
 * aml_ps_bh_enable - Enable/disable PS mode for one STA
 *
 * @aml_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @enable: PS mode status
 *
 * This function will enable/disable PS mode for one STA.
 * When enabling PS mode:
 *  - Stop all STA's txq for AML_TXQ_STOP_STA_PS reason
 *  - Count how many buffers are already ready for this STA
 *  - For BC/MC sta, update all queued SKB to use hw_queue BCMC
 *  - Update TIM if some packet are ready
 *
 * When disabling PS mode:
 *  - Start all STA's txq for AML_TXQ_STOP_STA_PS reason
 *  - For BC/MC sta, update all queued SKB to use hw_queue AC_BE
 *  - Update TIM if some packet are ready (otherwise fw will not update TIM
 *    in beacon for this STA)
 *
 * All counter/skb updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from a bottom_half tasklet.
 */
void aml_ps_bh_enable(struct aml_hw *aml_hw, struct aml_sta *sta,
                       bool enable)
{
    struct aml_txq *txq;

    if (enable) {
        trace_ps_enable(sta);
        aml_spin_lock(&aml_hw->tx_lock);
        sta->ps.active = true;
        sta->ps.sp_cnt[LEGACY_PS_ID] = 0;
        sta->ps.sp_cnt[UAPSD_ID] = 0;
        aml_txq_sta_stop(sta, AML_TXQ_STOP_STA_PS, aml_hw);

        if (is_multicast_sta(sta->sta_idx)) {
            txq = aml_txq_sta_get(sta, 0, aml_hw);
            sta->ps.pkt_ready[LEGACY_PS_ID] = skb_queue_len(&txq->sk_list);
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            txq->hwq = &aml_hw->hwq[AML_HWQ_BE];
        } else {
            int i;
            sta->ps.pkt_ready[LEGACY_PS_ID] = 0;
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            foreach_sta_txq_safe(sta, txq, i, aml_hw) {
                sta->ps.pkt_ready[txq->ps_id] += skb_queue_len(&txq->sk_list);
            }
        }
        aml_spin_unlock(&aml_hw->tx_lock);

        if (sta->ps.pkt_ready[LEGACY_PS_ID])
            aml_set_traffic_status(aml_hw, sta, true, LEGACY_PS_ID);

        if (sta->ps.pkt_ready[UAPSD_ID])
            aml_set_traffic_status(aml_hw, sta, true, UAPSD_ID);
    } else {
        trace_ps_disable(sta->sta_idx);
        aml_spin_lock(&aml_hw->tx_lock);
        sta->ps.active = false;

        if (is_multicast_sta(sta->sta_idx)) {
            txq = aml_txq_sta_get(sta, 0, aml_hw);
            txq->hwq = &aml_hw->hwq[AML_HWQ_BE];
            txq->push_limit = 0;
        } else {
            int i;
            foreach_sta_txq_safe(sta, txq, i, aml_hw) {
                txq->push_limit = 0;
            }
        }

        aml_txq_sta_start(sta, AML_TXQ_STOP_STA_PS, aml_hw);
        aml_spin_unlock(&aml_hw->tx_lock);
        if (sta->ps.pkt_ready[LEGACY_PS_ID])
            aml_set_traffic_status(aml_hw, sta, false, LEGACY_PS_ID);

        if (sta->ps.pkt_ready[UAPSD_ID])
            aml_set_traffic_status(aml_hw, sta, false, UAPSD_ID);
    }
}

/**
 * aml_ps_bh_traffic_req - Handle traffic request for STA in PS mode
 *
 * @aml_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @pkt_req: number of pkt to push
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
 *
 * This function will make sure that @pkt_req are pushed to fw
 * whereas the STA is in PS mode.
 * If request is 0, send all traffic
 * If request is greater than available pkt, reduce request
 * Note: request will also be reduce if txq credits are not available
 *
 * All counter updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from the bottom_half tasklet.
 */
void aml_ps_bh_traffic_req(struct aml_hw *aml_hw, struct aml_sta *sta,
                            u16 pkt_req, u8 ps_id)
{
    int pkt_ready_all;
    struct aml_txq *txq;
    u8 bcmc_mac[ETH_ALEN] = {0,};

    if (!sta)
        return;
    if (!sta->ps.active) {
        WARN(memcmp(sta->mac_addr, bcmc_mac, ETH_ALEN), "sta %pM is not in Power Save mode", sta->mac_addr);
        return;
    }

    trace_ps_traffic_req(sta, pkt_req, ps_id);
    aml_spin_lock(&aml_hw->tx_lock);
    /* Fw may ask to stop a service period with PS_SP_INTERRUPTED. This only
       happens for p2p-go interface if NOA starts during a service period */
    if ((pkt_req == PS_SP_INTERRUPTED) && (ps_id == UAPSD_ID)) {
        int tid;
        sta->ps.sp_cnt[ps_id] = 0;
        foreach_sta_txq_safe(sta, txq, tid, aml_hw) {
            txq->push_limit = 0;
        }
        goto done;
    }

    pkt_ready_all = (sta->ps.pkt_ready[ps_id] - sta->ps.sp_cnt[ps_id]);

    /* Don't start SP until previous one is finished or we don't have
       packet ready (which must not happen for U-APSD) */
    if (sta->ps.sp_cnt[ps_id] || pkt_ready_all <= 0) {
        goto done;
    }

    /* Adapt request to what is available. */
    if (pkt_req == 0 || pkt_req > pkt_ready_all) {
        pkt_req = pkt_ready_all;
    }

    /* Reset the SP counter */
    sta->ps.sp_cnt[ps_id] = 0;

    /* "dispatch" the request between txq */
    if (is_multicast_sta(sta->sta_idx)) {
        txq = aml_txq_sta_get(sta, 0, aml_hw);
        if (txq->credits <= 0)
            goto done;
        if (pkt_req > txq->credits)
            pkt_req = txq->credits;
        txq->push_limit = pkt_req;
        sta->ps.sp_cnt[ps_id] = pkt_req;
        aml_txq_add_to_hw_list(txq);
    } else {
        int i, tid;

        foreach_sta_txq_prio_safe(sta, txq, tid, i, aml_hw) {
            u16 txq_len = skb_queue_len(&txq->sk_list);

            if (txq->ps_id != ps_id)
                continue;

            if (txq_len > txq->credits)
                txq_len = txq->credits;

            if (txq_len == 0)
                continue;

            if (txq_len < pkt_req) {
                /* Not enough pkt queued in this txq, add this
                   txq to hwq list and process next txq */
                pkt_req -= txq_len;
                txq->push_limit = txq_len;
                sta->ps.sp_cnt[ps_id] += txq_len;
                aml_txq_add_to_hw_list(txq);
            } else {
                /* Enough pkt in this txq to comlete the request
                   add this txq to hwq list and stop processing txq */
                txq->push_limit = pkt_req;
                sta->ps.sp_cnt[ps_id] += pkt_req;
                aml_txq_add_to_hw_list(txq);
                break;
            }
        }
    }

  done:
  aml_spin_unlock(&aml_hw->tx_lock);
}

/******************************************************************************
 * TX functions
 *****************************************************************************/
#define PRIO_STA_NULL 0xAA

static const int aml_down_hwq2tid[3] = {
    [AML_HWQ_BK] = 2,
    [AML_HWQ_BE] = 3,
    [AML_HWQ_VI] = 5,
};

static void aml_downgrade_ac(struct aml_sta *sta, struct sk_buff *skb)
{
    int8_t ac = aml_tid2hwq[skb->priority];

    if (WARN((ac > AML_HWQ_VO),
             "Unexepcted ac %d for skb before downgrade", ac))
        ac = AML_HWQ_VO;

    while (sta->acm & BIT(ac)) {
        if (ac == AML_HWQ_BK) {
            skb->priority = 1;
            return;
        }
        ac--;
        skb->priority = aml_down_hwq2tid[ac];
    }
}

static void aml_tx_statistic(struct aml_vif *vif, struct aml_txq *txq,
                              union aml_hw_txstatus status, unsigned int data_len)
{
    struct aml_sta *sta = txq->sta;

    if (!status.acknowledged) {
        if (sta)
            sta->stats.tx_fails++;
        return;
    }
    vif->net_stats.tx_packets++;
    vif->net_stats.tx_bytes += data_len;

    if (!sta)
        return;

    sta->stats.tx_pkts++;
    sta->stats.tx_bytes += data_len;
    sta->stats.last_act = jiffies;
}

u16 aml_select_txq(struct aml_vif *aml_vif, struct sk_buff *skb)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct wireless_dev *wdev = &aml_vif->wdev;
    struct aml_sta *sta = NULL;
    struct aml_txq *txq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
    struct netdev_queue *netq;
    int queue_index = 0, count = 0;
    struct Qdisc *q;
#endif
    u16 netdev_queue;
    bool tdls_mgmgt_frame = false;

    switch (wdev->iftype) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        struct ethhdr *eth;
        eth = (struct ethhdr *)skb->data;
        if (eth->h_proto == cpu_to_be16(ETH_P_TDLS)) {
            tdls_mgmgt_frame = true;
        }
        if ((aml_vif->tdls_status == TDLS_LINK_ACTIVE) &&
            (aml_vif->sta.tdls_sta != NULL) &&
            (memcmp(eth->h_dest, aml_vif->sta.tdls_sta->mac_addr, ETH_ALEN) == 0))
            sta = aml_vif->sta.tdls_sta;
        else
            sta = aml_vif->sta.ap;
        break;
    }
    case NL80211_IFTYPE_AP_VLAN:
        if (aml_vif->ap_vlan.sta_4a) {
            sta = aml_vif->ap_vlan.sta_4a;
            break;
        }

        /* AP_VLAN interface is not used for a 4A STA,
           fallback searching sta amongs all AP's clients */
        aml_vif = aml_vif->ap_vlan.master;
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
    {
        struct aml_sta *cur, *tmp;
        struct ethhdr *eth = (struct ethhdr *)skb->data;

        if (is_multicast_ether_addr(eth->h_dest)) {
            sta = aml_hw->sta_table + aml_vif->ap.bcmc_index;
        } else {
            list_for_each_entry_safe(cur, tmp, &aml_vif->ap.sta_list, list) {
                if (!memcmp(cur->mac_addr, eth->h_dest, ETH_ALEN)) {
                    sta = cur;
                    break;
                }
            }
        }

        break;
    }
    case NL80211_IFTYPE_MESH_POINT:
    {
        struct ethhdr *eth = (struct ethhdr *)skb->data;

        if (!aml_vif->is_resending) {
            /*
             * If ethernet source address is not the address of a mesh wireless interface, we are proxy for
             * this address and have to inform the HW
             */
            if (memcmp(&eth->h_source[0], &aml_vif->ndev->perm_addr[0], ETH_ALEN)) {
                /* Check if LMAC is already informed */
                if (!aml_get_mesh_proxy_info(aml_vif, (u8 *)&eth->h_source, true)) {
                    aml_send_mesh_proxy_add_req(aml_hw, aml_vif, (u8 *)&eth->h_source);
                }
            }
        }

        if (is_multicast_ether_addr(eth->h_dest)) {
            sta = aml_hw->sta_table + aml_vif->ap.bcmc_index;
        } else {
            /* Path to be used */
            struct aml_mesh_path *p_mesh_path = NULL;
            struct aml_mesh_path *p_cur_path;
            /* Check if destination is proxied by a peer Mesh STA */
            struct aml_mesh_proxy *p_mesh_proxy = aml_get_mesh_proxy_info(aml_vif, (u8 *)&eth->h_dest, false);
            /* Mesh Target address */
            struct mac_addr *p_tgt_mac_addr;

            if (p_mesh_proxy) {
                p_tgt_mac_addr = &p_mesh_proxy->proxy_addr;
            } else {
                p_tgt_mac_addr = (struct mac_addr *)&eth->h_dest;
            }

            /* Look for path with provided target address */
            list_for_each_entry(p_cur_path, &aml_vif->ap.mpath_list, list) {
                if (!memcmp(&p_cur_path->tgt_mac_addr, p_tgt_mac_addr, ETH_ALEN)) {
                    p_mesh_path = p_cur_path;
                    break;
                }
            }

            if (p_mesh_path) {
                sta = p_mesh_path->nhop_sta;
            } else {
                aml_send_mesh_path_create_req(aml_hw, aml_vif, (u8 *)p_tgt_mac_addr);
            }
        }

        break;
    }
    default:
        break;
    }

    if (sta && sta->qos)
    {
        if (tdls_mgmgt_frame) {
            skb_set_queue_mapping(skb, NX_STA_NDEV_IDX(skb->priority, sta->sta_idx));
        } else {
            /* use the data classifier to determine what 802.1d tag the
             * data frame has */
            skb->priority = cfg80211_classify8021d(skb, NULL) & IEEE80211_QOS_CTL_TAG1D_MASK;
        }
        if (sta->acm)
            aml_downgrade_ac(sta, skb);

        if (skb->protocol == cpu_to_be16(ETH_P_PAE)) {
            skb->priority = 7;
            printk("%s: set eap frame to vo\n", __func__);
        }

        txq = aml_txq_sta_get(sta, skb->priority, aml_hw);
        netdev_queue = txq->ndev_idx;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0) && LINUX_VERSION_CODE <= KERNEL_VERSION(5, 10, 0))
        queue_index = netdev_queue;
        queue_index = netdev_cap_txqueue(aml_vif->ndev, queue_index);
        netq = netdev_get_tx_queue(aml_vif->ndev, queue_index);
        q = rcu_dereference_bh(netq->qdisc);
        while (skb->protocol == cpu_to_be16(ETH_P_PAE) && !(q->flags & TCQ_F_NOLOCK) && count < 50) {
            rcu_read_unlock_bh();
            q = rcu_dereference_bh(netq->qdisc);
            if (count%10 == 0) {
                printk("txq flag is not ok, need wait dev active.  flag: %x\n", q->flags);
            }
            msleep(10);
            count ++;
            rcu_read_lock_bh();
        }
#endif
    }
    else if (sta)
    {
        skb->priority = 0xFF;
        txq = aml_txq_sta_get(sta, 0, aml_hw);
        netdev_queue = txq->ndev_idx;
    }
    else
    {
        /* This packet will be dropped in xmit function, still need to select
           an active queue for xmit to be called. As it most likely to happen
           for AP interface, select BCMC queue
           (TODO: select another queue if BCMC queue is stopped) */
        skb->priority = PRIO_STA_NULL;
        netdev_queue = NX_BCMC_TXQ_NDEV_IDX;
    }

    BUG_ON(netdev_queue >= NX_NB_NDEV_TXQ);

    return netdev_queue;
}

/**
 * aml_set_more_data_flag - Update MORE_DATA flag in tx sw desc
 *
 * @aml_hw: Driver main data
 * @sw_txhdr: Header for pkt to be pushed
 *
 * If STA is in PS mode
 *  - Set EOSP in case the packet is the last of the UAPSD service period
 *  - Set MORE_DATA flag if more pkt are ready for this sta
 *  - Update TIM if this is the last pkt buffered for this sta
 *
 * note: tx_lock already taken.
 */
static inline void aml_set_more_data_flag(struct aml_hw *aml_hw,
                                           struct aml_sw_txhdr *sw_txhdr)
{
    struct aml_sta *sta = sw_txhdr->aml_sta;
    struct aml_vif *vif = sw_txhdr->aml_vif;
    struct aml_txq *txq = sw_txhdr->txq;

    if (unlikely(sta->ps.active)) {
        sta->ps.pkt_ready[txq->ps_id]--;
        sta->ps.sp_cnt[txq->ps_id]--;

        trace_ps_push(sta);

        if (((txq->ps_id == UAPSD_ID) || (vif->wdev.iftype == NL80211_IFTYPE_MESH_POINT) || (sta->tdls.active))
                && !sta->ps.sp_cnt[txq->ps_id]) {
            sw_txhdr->desc.api.host.flags |= TXU_CNTRL_EOSP;
        }

        if (sta->ps.pkt_ready[txq->ps_id]) {
            sw_txhdr->desc.api.host.flags |= TXU_CNTRL_MORE_DATA;
        } else {
            aml_set_traffic_status(aml_hw, sta, false, txq->ps_id);
        }
    }
}

/**
 * aml_get_tx_info - Get STA and tid for one skb
 *
 * @aml_vif: vif ptr
 * @skb: skb
 * @tid: pointer updated with the tid to use for this skb
 *
 * @return: pointer on the destination STA (may be NULL)
 *
 * skb has already been parsed in aml_select_queue function
 * simply re-read information form skb.
 */
static struct aml_sta *aml_get_tx_info(struct aml_vif *aml_vif,
                                         struct sk_buff *skb,
                                         u8 *tid)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_sta *sta;
    int sta_idx;

    *tid = skb->priority;
    if (unlikely(skb->priority == PRIO_STA_NULL)) {
        return NULL;
    } else {
        int ndev_idx = skb_get_queue_mapping(skb);

        if (ndev_idx == NX_BCMC_TXQ_NDEV_IDX)
            sta_idx = NX_REMOTE_STA_MAX + master_vif_idx(aml_vif);
        else
            sta_idx = ndev_idx / NX_NB_TID_PER_STA;

        sta = aml_hw->sta_table + sta_idx;
    }

    return sta;
}

/**
 * aml_prep_dma_tx - Prepare buffer for DMA transmission
 *
 * @aml_hw: Driver main data
 * @sw_txhdr: Software Tx descriptor
 * @frame_start: Pointer to the beginning of the frame that needs to be DMA mapped
 * @return: 0 on success, -1 on error
 *
 * Map the frame for DMA transmission and save its ipc address in the tx descriptor
 */
static int aml_sdio_prep_dma_tx(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                             void *frame_start)
{
    struct txdesc_api *desc = &sw_txhdr->desc.api;

    //if (aml_ipc_buf_a2e_init(aml_hw, &sw_txhdr->ipc_data, frame_start,
    //                          sw_txhdr->frame_len))
    //    return -1;

    /* Update DMA addresses and length in tx descriptor */
    desc->host.packet_len[0] = sw_txhdr->frame_len;
    //desc->host.packet_addr[0] = sw_txhdr->ipc_data.dma_addr;
    desc->host.packet_addr[0] = 1;
#ifdef CONFIG_AML_SPLIT_TX_BUF
    desc->host.packet_cnt = 1;
#endif

    return 0;
}

static int aml_pcie_prep_dma_tx(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                            void *frame_start)
{
    struct txdesc_api *desc = &sw_txhdr->desc.api;

    if (aml_ipc_buf_a2e_init(aml_hw, &sw_txhdr->ipc_data, frame_start,
                              sw_txhdr->frame_len))
        return -1;

    /* Update DMA addresses and length in tx descriptor */
    desc->host.packet_len[0] = sw_txhdr->frame_len;
    desc->host.packet_addr[0] = sw_txhdr->ipc_data.dma_addr;
#ifdef CONFIG_AML_SPLIT_TX_BUF
    desc->host.packet_cnt = 1;
#endif

    return 0;
}
int aml_prep_dma_tx(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                            void *frame_start)
{
    if (aml_bus_type != PCIE_MODE) {
        return aml_sdio_prep_dma_tx(aml_hw, sw_txhdr, frame_start);
    } else {
        return aml_pcie_prep_dma_tx(aml_hw, sw_txhdr, frame_start);
    }
}
/**
 *  aml_tx_push - Push one packet to fw
 *
 * @aml_hw: Driver main data
 * @txhdr: tx desc of the buffer to push
 * @flags: push flags (see @aml_push_flags)
 *
 * Push one packet to fw. Sw desc of the packet has already been updated.
 * Only MORE_DATA flag will be set if needed.
 */
void aml_tx_push(struct aml_hw *aml_hw, struct aml_txhdr *txhdr, int flags)
{
    struct aml_sw_txhdr *sw_txhdr = txhdr->sw_hdr;
    struct sk_buff *skb = sw_txhdr->skb;
    struct aml_txq *txq = sw_txhdr->txq;
    u16 hw_queue = txq->hwq->id;
    int user = 0;

    lockdep_assert_held(&aml_hw->tx_lock);

    /* RETRY flag is not always set so retest here */
    if (txq->nb_retry) {
        flags |= AML_PUSH_RETRY;
        txq->nb_retry--;
        if (txq->nb_retry == 0) {
            WARN(skb != txq->last_retry_skb,
                 "last retry buffer is not the expected one");
            txq->last_retry_skb = NULL;
        }
    } else if (!(flags & AML_PUSH_RETRY)) {
        txq->pkt_sent++;
    }
    sw_txhdr->desc.api.host.flags &= ~TXU_CNTRL_RETRY;

#ifdef CONFIG_AML_AMSDUS_TX
    if (txq->amsdu == sw_txhdr) {
        WARN((flags & AML_PUSH_RETRY), "End A-MSDU on a retry");
        aml_hw->stats->amsdus[sw_txhdr->amsdu.nb - 1].done++;
        txq->amsdu = NULL;
    } else if (!(flags & AML_PUSH_RETRY) &&
               !(sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU)) {
        aml_hw->stats->amsdus[0].done++;
    }
#endif /* CONFIG_AML_AMSDUS_TX */

    /* Wait here to update hw_queue, as for multicast STA hwq may change
       between queue and push (because of PS) */
    sw_txhdr->hw_queue = hw_queue;

#ifdef CONFIG_AML_MUMIMO_TX
    /* MU group is only selected during hwq processing */
    sw_txhdr->desc.api.host.mumimo_info = txq->mumimo_info;
    user = AML_TXQ_POS_ID(txq);
#endif /* CONFIG_AML_MUMIMO_TX */

    if (sw_txhdr->aml_sta) {
        /* only for AP mode */
        aml_set_more_data_flag(aml_hw, sw_txhdr);
    }

    trace_push_desc(skb, sw_txhdr, flags);
    txq->credits--;
    txq->pkt_pushed[user]++;
    if (txq->credits <= 0)
        aml_txq_stop(txq, AML_TXQ_STOP_FULL);

    if (txq->push_limit)
        txq->push_limit--;

    aml_ipc_txdesc_push(aml_hw, sw_txhdr, skb, hw_queue);
    txq->hwq->credits[user]--;
    aml_hw->stats->cfm_balance[hw_queue]++;
}



/**
 * aml_tx_retry - Re-queue a pkt that has been postponed by firmware
 *
 * @aml_hw: Driver main data
 * @skb: pkt to re-push
 * @sw_txhdr: software TX desc of the pkt to re-push
 * @status: Status on the transmission
 *
 * Called when a packet needs to be repushed to the firmware, because firmware
 * wasn't able to process it when first pushed (e.g. the station enter PS after
 * the driver first pushed this packet to the firmware).
 */
static void aml_tx_retry(struct aml_hw *aml_hw, struct sk_buff *skb,
                          struct aml_sw_txhdr *sw_txhdr,
                          union aml_hw_txstatus status)
{
    struct aml_txq *txq = sw_txhdr->txq;

    /* MORE_DATA will be re-set if needed when pkt will be repushed */
    sw_txhdr->desc.api.host.flags &= ~TXU_CNTRL_MORE_DATA;

    if (status.retry_required) {
        // Firmware already tried to send the buffer but cannot retry it now
        // On next push, firmware needs to re-use the same SN
        sw_txhdr->desc.api.host.flags |= TXU_CNTRL_REUSE_SN;
        sw_txhdr->desc.api.host.sn_for_retry = status.sn;

        if (aml_bus_type != PCIE_MODE)
            printk("%s, reuse sn = %d\n", __func__, status.sn);
    }

    txq->credits++;
    trace_skb_retry(skb, txq, (status.retry_required) ? status.sn : 4096);
    if (txq->credits > 0)
        aml_txq_start(txq, AML_TXQ_STOP_FULL);

    /* Queue the buffer */
    aml_txq_queue_skb(skb, txq, aml_hw, true, NULL);
}


#ifdef CONFIG_AML_AMSDUS_TX
/**
 * aml_amsdu_subframe_length() - return size of A-MSDU subframe including
 * header but without padding
 *
 * @eth: Ethernet Header of the frame
 * @frame_len: Length of the ethernet frame (including ethernet header)
 * @return: length of the A-MSDU subframe
 */
static inline int aml_amsdu_subframe_length(struct ethhdr *eth, int frame_len)
{
    /* ethernet header is replaced with amdsu header that have the same size
       Only need to check if LLC/SNAP header will be added */
    int len = frame_len;

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        len += sizeof(rfc1042_header) + 2;
    }

    return len;
}

static inline bool aml_amsdu_is_aggregable(struct sk_buff *skb)
{
    /* need to add some check on buffer to see if it can be aggregated ? */
    return true;
}

/**
 * aml_amsdu_del_subframe_header - remove AMSDU header
 *
 * @amsdu_txhdr: amsdu tx descriptor
 *
 * Move back the ethernet header at the "beginning" of the data buffer.
 * (which has been moved in @aml_amsdu_add_subframe_header)
 */
static void aml_amsdu_del_subframe_header(struct aml_amsdu_txhdr *amsdu_txhdr)
{
    struct sk_buff *skb = amsdu_txhdr->skb;
    struct ethhdr *eth;
    u8 *pos;

    BUG_ON(skb == NULL);
    pos = skb->data;
    pos += sizeof(struct aml_amsdu_txhdr);
    eth = (struct ethhdr*)pos;
    pos += amsdu_txhdr->pad + sizeof(struct ethhdr);

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        pos += sizeof(rfc1042_header) + 2;
    }

    memmove(pos, eth, sizeof(*eth));
    skb_pull(skb, (pos - skb->data));
}

/**
 * aml_amsdu_add_subframe_header - Add AMSDU header and link subframe
 *
 * @aml_hw Driver main data
 * @skb Buffer to aggregate
 * @sw_txhdr Tx descriptor for the first A-MSDU subframe
 *
 * return 0 on sucess, -1 otherwise
 *
 * This functions Add A-MSDU header and LLC/SNAP header in the buffer
 * and update sw_txhdr of the first subframe to link this buffer.
 * If an error happens, the buffer will be queued as a normal buffer.
 *
 *
 *            Before           After
 *         +-------------+  +-------------+
 *         | HEADROOM    |  | HEADROOM    |
 *         |             |  +-------------+ <- data
 *         |             |  | amsdu_txhdr |
 *         |             |  | * pad size  |
 *         |             |  +-------------+
 *         |             |  | ETH hdr     | keep original eth hdr
 *         |             |  |             | to restore it once transmitted
 *         |             |  +-------------+ <- packet_addr[x]
 *         |             |  | Pad         |
 *         |             |  +-------------+
 * data -> +-------------+  | AMSDU HDR   |
 *         | ETH hdr     |  +-------------+
 *         |             |  | LLC/SNAP    |
 *         +-------------+  +-------------+
 *         | DATA        |  | DATA        |
 *         |             |  |             |
 *         +-------------+  +-------------+
 *
 * Called with tx_lock hold
 */
static int aml_amsdu_add_subframe_header(struct aml_hw *aml_hw,
                                          struct sk_buff *skb,
                                          struct aml_sw_txhdr *sw_txhdr)
{
    struct aml_amsdu *amsdu = &sw_txhdr->amsdu;
    struct aml_amsdu_txhdr *amsdu_txhdr;
    struct ethhdr *amsdu_hdr, *eth = (struct ethhdr *)skb->data;
    int headroom_need, msdu_len, amsdu_len;
    u8 *pos, *amsdu_start;

    msdu_len = skb->len - sizeof(*eth);
    headroom_need = sizeof(*amsdu_txhdr) + amsdu->pad +
        sizeof(*amsdu_hdr);
    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        headroom_need += sizeof(rfc1042_header) + 2;
        msdu_len += sizeof(rfc1042_header) + 2;
    }
    amsdu_len = msdu_len + sizeof(*amsdu_hdr) + amsdu->pad;

    /* we should have enough headroom (checked in xmit) */
    if (WARN_ON(skb_headroom(skb) < headroom_need)) {
        return -1;
    }

    /* allocate headroom */
    pos = skb_push(skb, headroom_need);
    amsdu_txhdr = (struct aml_amsdu_txhdr *)pos;
    pos += sizeof(*amsdu_txhdr);

    /* move eth header */
    memmove(pos, eth, sizeof(*eth));
    eth = (struct ethhdr *)pos;
    pos += sizeof(*eth);

    /* Add padding from previous subframe */
    amsdu_start = pos;
    memset(pos, 0, amsdu->pad);
    pos += amsdu->pad;

    /* Add AMSDU hdr */
    amsdu_hdr = (struct ethhdr *)pos;
    memcpy(amsdu_hdr->h_dest, eth->h_dest, ETH_ALEN);
    memcpy(amsdu_hdr->h_source, eth->h_source, ETH_ALEN);
    amsdu_hdr->h_proto = htons(msdu_len);
    pos += sizeof(*amsdu_hdr);

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        memcpy(pos, rfc1042_header, sizeof(rfc1042_header));
        pos += sizeof(rfc1042_header) + 2;
        // +2 is for protocol ID which is already here (i.e. just before the data)
    }

    /* Prepare IPC buffer for DMA transfer */
    if (aml_bus_type == PCIE_MODE) {
        if (aml_ipc_buf_a2e_init(aml_hw, &amsdu_txhdr->ipc_data, amsdu_start, amsdu_len)) {
            netdev_err(skb->dev, "Failed to add A-MSDU header\n");
            pos -= sizeof(*eth);
            memmove(pos, eth, sizeof(*eth));
            skb_pull(skb, headroom_need);
            return -1;
        }
    }

    /* update amdsu_txhdr */
    amsdu_txhdr->skb = skb;
    amsdu_txhdr->pad = amsdu->pad;
    amsdu_txhdr->msdu_len = msdu_len;

    /* update aml_sw_txhdr (of the first subframe) */
    BUG_ON(amsdu->nb != sw_txhdr->desc.api.host.packet_cnt);
    if (aml_bus_type == PCIE_MODE) {
        sw_txhdr->desc.api.host.packet_addr[amsdu->nb] = amsdu_txhdr->ipc_data.dma_addr;
    } else {
        sw_txhdr->desc.api.host.packet_addr[amsdu->nb] = 0;
    }
    sw_txhdr->desc.api.host.packet_len[amsdu->nb] = amsdu_len;
    sw_txhdr->desc.api.host.packet_cnt++;
    amsdu->nb++;

    amsdu->pad = AMSDU_PADDING(amsdu_len - amsdu->pad);
    list_add_tail(&amsdu_txhdr->list, &amsdu->hdrs);
    amsdu->len += amsdu_len;

    trace_amsdu_subframe(sw_txhdr);
    return 0;
}

/**
 * aml_amsdu_add_subframe - Add this buffer as an A-MSDU subframe if possible
 *
 * @aml_hw Driver main data
 * @skb Buffer to aggregate if possible
 * @sta Destination STA
 * @txq sta's txq used for this buffer
 *
 * Try to aggregate the buffer in an A-MSDU. If it succeed then the
 * buffer is added as a new A-MSDU subframe with AMSDU and LLC/SNAP
 * headers added (so FW won't have to modify this subframe).
 *
 * To be added as subframe :
 * - sta must allow amsdu
 * - buffer must be aggregable (to be defined)
 * - at least one other aggregable buffer is pending in the queue
 *  or an a-msdu (with enough free space) is currently in progress
 *
 * returns true if buffer has been added as A-MDSP subframe, false otherwise
 *
 */
static bool aml_amsdu_add_subframe(struct aml_hw *aml_hw, struct sk_buff *skb,
                                    struct aml_sta *sta, struct aml_txq *txq)
{
    bool res = false;
    struct ethhdr *eth;

    /* Adjust the maximum number of MSDU allowed in A-MSDU */
    aml_adjust_amsdu_maxnb(aml_hw);

#ifdef CONFIG_AML_USB_LARGE_PAGE
    if (aml_bus_type == USB_MODE) {
        if (aml_hw->mod_params->amsdu_maxnb > 3)
            aml_hw->mod_params->amsdu_maxnb = 3; // USB limits the number of AMSDU aggregations, which can be sent in one BUF
        //printk("%s txq->id:%d, txq->amsdu_len:%d\n", __func__, txq->idx, txq->amsdu_len);
    }
#endif
    /* immediately return if amsdu are not allowed for this sta */
    if (!txq->amsdu_len || aml_hw->mod_params->amsdu_maxnb < 2 ||
        !aml_amsdu_is_aggregable(skb)
       )
        return false;

    spin_lock_bh(&aml_hw->tx_lock);
    if (txq->amsdu) {
        /* aggreagation already in progress, add this buffer if enough space
           available, otherwise end the current amsdu */
        struct aml_sw_txhdr *sw_txhdr = txq->amsdu;
        eth = (struct ethhdr *)(skb->data);

        //mac addr dismatch not asmdu
        if (memcmp(eth->h_dest, sw_txhdr->desc.api.host.eth_dest_addr.array, 6) || \
            memcmp(eth->h_source, sw_txhdr->desc.api.host.eth_src_addr.array, 6)) {
            txq->amsdu = NULL;
            goto end;
        }

        if ((sw_txhdr->desc.api.host.flags & TXU_CNTRL_SP_FRAME) ||
            ((sw_txhdr->amsdu.len + sw_txhdr->amsdu.pad +
              aml_amsdu_subframe_length(eth, skb->len)) > txq->amsdu_len) ||
            aml_amsdu_add_subframe_header(aml_hw, skb, sw_txhdr)) {
            txq->amsdu = NULL;
            goto end;
        }

        if (sw_txhdr->amsdu.nb >= aml_hw->mod_params->amsdu_maxnb) {
            aml_hw->stats->amsdus[sw_txhdr->amsdu.nb - 1].done++;
            /* max number of subframes reached */
            txq->amsdu = NULL;
        }
    } else {
        /* Check if a new amsdu can be started with the previous buffer
           (if any) and this one */
        struct sk_buff *skb_prev = skb_peek_tail(&txq->sk_list);
        struct aml_txhdr *txhdr;
        struct aml_sw_txhdr *sw_txhdr;
        int len1, len2;

        if (!skb_prev || !aml_amsdu_is_aggregable(skb_prev))
            goto end;

        txhdr = (struct aml_txhdr *)skb_prev->data;
        sw_txhdr = txhdr->sw_hdr;
        if ((sw_txhdr->amsdu.len) ||
            (sw_txhdr->desc.api.host.flags & TXU_CNTRL_RETRY) || (sw_txhdr->desc.api.host.flags & TXU_CNTRL_SP_FRAME))
            /* previous buffer is already a complete amsdu or a retry or special frame */
            goto end;

        eth = (struct ethhdr *)skb_mac_header(skb_prev);
        len1 = aml_amsdu_subframe_length(eth, (sw_txhdr->frame_len +
                                                sizeof(struct ethhdr)));

        eth = (struct ethhdr *)(skb->data);
        len2 = aml_amsdu_subframe_length(eth, skb->len);

        //mac addr dismatch not asmdu
        if (memcmp(eth->h_dest, sw_txhdr->desc.api.host.eth_dest_addr.array, 6) || \
            memcmp(eth->h_source, sw_txhdr->desc.api.host.eth_src_addr.array, 6)) {
            goto end;
        }

        if (len1 + AMSDU_PADDING(len1) + len2 > txq->amsdu_len)
            /* not enough space to aggregate those two buffers */
            goto end;
        /* Add subframe header.
           Note: Fw will take care of adding AMDSU header for the first
           subframe while generating 802.11 MAC header */
        INIT_LIST_HEAD(&sw_txhdr->amsdu.hdrs);
        sw_txhdr->amsdu.len = len1;
        sw_txhdr->amsdu.nb = 1;
        sw_txhdr->amsdu.pad = AMSDU_PADDING(len1);
        if (aml_amsdu_add_subframe_header(aml_hw, skb, sw_txhdr))
            goto end;

        sw_txhdr->desc.api.host.flags |= TXU_CNTRL_AMSDU;

        if (sw_txhdr->amsdu.nb < aml_hw->mod_params->amsdu_maxnb)
            txq->amsdu = sw_txhdr;
        else
            aml_hw->stats->amsdus[sw_txhdr->amsdu.nb - 1].done++;
    }

    res = true;

  end:
    spin_unlock_bh(&aml_hw->tx_lock);
    return res;
}

/**
 * aml_amsdu_dismantle - Dismantle an already formatted A-MSDU
 *
 * @aml_hw Driver main data
 * @sw_txhdr_main Software descriptor of the A-MSDU to dismantle.
 *
 * The a-mdsu is always fully dismantled (i.e don't try to reduce it's size to
 * fit the new limit).
 * The DMA mapping can be re-used as aml_amsdu_add_subframe_header ensure that
 * enough data in the skb bufer are 'DMA mapped'.
 * It would have been slightly simple to unmap/re-map but it is a little faster like this
 * and not that much more complicated to read.
 */
static void aml_amsdu_dismantle(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr_main)
{
    struct aml_amsdu_txhdr *amsdu_txhdr, *next;
    struct sk_buff *skb_prev = sw_txhdr_main->skb;
    struct aml_txq *txq =  sw_txhdr_main->txq;
    u32 tx_max_headroom = 0;

    trace_amsdu_dismantle(sw_txhdr_main);

    aml_hw->stats->amsdus[sw_txhdr_main->amsdu.nb - 1].done--;
    sw_txhdr_main->amsdu.len = 0;
    sw_txhdr_main->amsdu.nb = 0;
    sw_txhdr_main->desc.api.host.flags &= ~TXU_CNTRL_AMSDU;
    sw_txhdr_main->desc.api.host.packet_cnt = 1;

    list_for_each_entry_safe(amsdu_txhdr, next, &sw_txhdr_main->amsdu.hdrs, list) {
        struct sk_buff *skb = amsdu_txhdr->skb;
        struct aml_txhdr *txhdr;
        struct aml_usb_txhdr *usb_txhdr;
        struct aml_sdio_txhdr *sdio_txhdr;
        struct aml_sw_txhdr *sw_txhdr;
        size_t frame_len;
        size_t data_oft;

        list_del(&amsdu_txhdr->list);
        if (aml_bus_type == USB_MODE) {
            tx_max_headroom = AML_USB_TX_HEADROOM;
        } else {
            tx_max_headroom = AML_SDIO_TX_HEADROOM;
        }
        aml_amsdu_del_subframe_header(amsdu_txhdr);

        frame_len = AML_TX_DMA_MAP_LEN(skb);

        sw_txhdr = kmem_cache_alloc(aml_hw->sw_txhdr_cache, GFP_ATOMIC);


        if (aml_bus_type == PCIE_MODE) {
            if (unlikely((skb_headroom(skb) < AML_TX_HEADROOM)
               || (sw_txhdr == NULL) || (frame_len > amsdu_txhdr->ipc_data.size))) {
                dev_err(aml_hw->dev, "Failed to dismantle A-MSDU\n");
                if (sw_txhdr)
                    kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
                aml_ipc_buf_a2e_release(aml_hw, &amsdu_txhdr->ipc_data);
                dev_kfree_skb_any(skb);
                /* coverity[leaked_storage] - variable "sw_txhdr" was free in kmem_cache_free */
                continue;
            }
        } else {
            if (unlikely((skb_headroom(skb) < tx_max_headroom) || (sw_txhdr == NULL))) {
                dev_err(aml_hw->dev, "Failed to dismantle A-MSDU\n");
                if (sw_txhdr)
                    kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
                dev_kfree_skb_any(skb);
                /* coverity[leaked_storage] - variable "sw_txhdr" was free in kmem_cache_free */
                continue;
            }
        }
        // Offset between DMA mapping for an A-MSDU subframe and a simple MPDU
        data_oft = amsdu_txhdr->ipc_data.size - frame_len;

        memcpy(sw_txhdr, sw_txhdr_main, sizeof(*sw_txhdr));
        sw_txhdr->frame_len = frame_len;
        sw_txhdr->skb = skb;
        if (aml_bus_type == PCIE_MODE) {
           sw_txhdr->ipc_data = amsdu_txhdr->ipc_data; // It's OK to re-use amsdu_txhdr ptr
           sw_txhdr->desc.api.host.packet_addr[0] = sw_txhdr->ipc_data.dma_addr + data_oft;
        }
        sw_txhdr->desc.api.host.packet_len[0] = frame_len;
        sw_txhdr->desc.api.host.packet_cnt = 1;
        if (aml_bus_type == USB_MODE) {
            skb_pull(skb, sizeof(struct ethhdr));
            usb_txhdr = (struct aml_usb_txhdr *)skb_push(skb, AML_USB_TX_HEADROOM);
            usb_txhdr->sw_hdr = sw_txhdr;
        } else if (aml_bus_type == SDIO_MODE) {
            skb_pull(skb, sizeof(struct ethhdr));
            sdio_txhdr = (struct aml_sdio_txhdr *)skb_push(skb, AML_SDIO_TX_HEADROOM);
            sdio_txhdr->sw_hdr = sw_txhdr;
            sdio_txhdr->mpdu_buf_flag = 0;
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG|HW_LAST_AGG_FLAG;
            sdio_txhdr->mpdu_buf_flag |= HW_MPDU_LEN_SET(sw_txhdr->frame_len + SDIO_FRAME_TAIL_LEN);

            memset(&sdio_txhdr->tx_vector, 0, sizeof(struct hw_tx_vector_bits) + sizeof(struct txdesc_host)/*8 byte alignment*/);
        } else {
            txhdr = (struct aml_txhdr *)skb_push(skb, AML_TX_HEADROOM);
            txhdr->sw_hdr = sw_txhdr;
        }

        if (aml_txq_queue_skb(skb, sw_txhdr->txq, aml_hw, false, skb_prev)) {
            ;
        }
        skb_prev = skb;
    }
}


/**
 * aml_amsdu_update_len - Update length allowed for A-MSDU on a TXQ
 *
 * @aml_hw Driver main data.
 * @txq The TXQ.
 * @amsdu_len New length allowed ofr A-MSDU.
 *
 * If this is a TXQ linked to a STA and the allowed A-MSDU size is reduced it is
 * then necessary to disassemble all A-MSDU currently queued on all STA' txq that
 * are larger than this new limit.
 * Does nothing if the A-MSDU limit increase or stay the same.
 */
static void aml_amsdu_update_len(struct aml_hw *aml_hw, struct aml_txq *txq,
                                  u16 amsdu_len)
{
    struct aml_sta *sta = txq->sta;
    int tid;

    if (amsdu_len != txq->amsdu_len)
        trace_amsdu_len_update(txq->sta, amsdu_len);

    if (amsdu_len >= txq->amsdu_len) {
        txq->amsdu_len = amsdu_len;
#ifdef CONFIG_AML_USB_LARGE_PAGE
        if (aml_bus_type == USB_MODE)
            txq->amsdu_len = MIN(txq->amsdu_len, USB_AMSDU_BUF_LEN);
#endif
        return;
    }

    if (!sta) {
        netdev_err(txq->ndev, "Non STA txq(%d) with a-amsdu len %d\n",
                   txq->idx, amsdu_len);
        txq->amsdu_len = 0;
        return;
    }

    /* A-MSDU size has been reduced by the firmware, need to dismantle all
       queued a-msdu that are too large. Need to do this for all txq of the STA. */
    foreach_sta_txq_safe(sta, txq, tid, aml_hw) {
        struct sk_buff *skb, *skb_next;

        if (txq->amsdu_len <= amsdu_len)
            continue;

        if (txq->last_retry_skb)
            skb = txq->last_retry_skb->next;
        else
            skb = txq->sk_list.next;

        skb_queue_walk_from_safe(&txq->sk_list, skb, skb_next) {
            struct aml_txhdr *txhdr = (struct aml_txhdr *)skb->data;
            struct aml_sw_txhdr *sw_txhdr = txhdr->sw_hdr;
            if ((sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU) &&
                (sw_txhdr->amsdu.len > amsdu_len))
                aml_amsdu_dismantle(aml_hw, sw_txhdr);

            if (txq->amsdu == sw_txhdr)
                txq->amsdu = NULL;
        }

        txq->amsdu_len = amsdu_len;
#ifdef CONFIG_AML_USB_LARGE_PAGE
        if (aml_bus_type == USB_MODE)
            txq->amsdu_len = MIN(txq->amsdu_len, USB_AMSDU_BUF_LEN);
#endif
    }
}
#endif /* CONFIG_AML_AMSDUS_TX */

bool aml_filter_sp_data_frame(struct sk_buff *skb, struct aml_vif *aml_vif, AML_SP_STATUS_E sp_status)
{
    struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
    struct udphdr *udphdr;
    unsigned char iphdrlen = 0;
    struct iphdr *iphdr;
    struct ipv6hdr *ipv6hdr;
    bool is_dhcpv4;
    bool is_dhcpv6;
    u8 str[200];
    u32 offset = 0;
    u8 *p = str;

    //filter eapol
    if (ethhdr->h_proto == htons(ETH_P_PAE)) {
        offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
        offset += sprintf(p + offset, "eapol,vif_idx:%d",aml_vif->vif_index);
        printk("%s\n", p);
        return true;
    }

    //filter arp
    if (ethhdr->h_proto == htons(ETH_P_ARP)) {
        u16 op = (*(skb->data + ETH_HDR_LEN + 6) << 8) | (*(skb->data + ETH_HDR_LEN + 7));
        u8 *sender_mac = skb->data + ETH_HDR_LEN + 8;
        u8 *sender_ip = skb->data + ETH_HDR_LEN + 14;
        u8 *target_mac = skb->data + ETH_HDR_LEN + 18;
        u8 *target_ip = skb->data + ETH_HDR_LEN + 24;

        offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
        if (op == 1) {
            offset += sprintf(p + offset, "arp req,vif_idx:%d ", aml_vif->vif_index);
        }
        else if (op == 2) {
            offset += sprintf(p + offset, "arp rsp,vif_idx:%d ", aml_vif->vif_index);
        }
        else {
            offset += sprintf(p + offset, "arp unknown:%x,vif_idx:%d ", op, aml_vif->vif_index);
        }
        offset += sprintf(p + offset, "sender:[%02x:%02x:%02x:%02x:%02x:%02x ",
                    sender_mac[0], sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4], sender_mac[5]);
        offset += sprintf(p + offset, "%d.%d.%d.%d]",
                    sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3]);
        offset += sprintf(p + offset, "receiver:[%02x:%02x:%02x:%02x:%02x:%02x ",
                    target_mac[0], target_mac[1], target_mac[2], target_mac[3], target_mac[4], target_mac[5]);
        offset += sprintf(p + offset, "%d.%d.%d.%d]",
                    target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

        if (((sp_status == SP_STATUS_RX) && (!memcmp(target_ip, &aml_vif->ipv4_addr, IPV4_ADDR_LEN)))
            || (sp_status != SP_STATUS_RX))
            printk("%s\n", p);

        return false; // Solving the compatibility problem between w2 softap and mtk
    }

    //filter dhcp
    if (ethhdr->h_proto == htons(ETH_P_IPV6)) {
        ipv6hdr = (struct ipv6hdr *)(skb->data + ETH_HDR_LEN);
        /* check for udp header */
        if (ipv6hdr->nexthdr != IPPROTO_UDP)
            return false;
        iphdrlen = sizeof(*ipv6hdr);
    } else if (ethhdr->h_proto == htons(ETH_P_IP)) {
        iphdr = (struct iphdr *)(skb->data + ETH_HDR_LEN);
        /* check for udp header */
        if (iphdr->protocol != IPPROTO_UDP)
            return false;
        iphdrlen = iphdr->ihl * 4;
    } else {
        return false;
    }
    udphdr = (struct udphdr *)(skb->data + ETH_HDR_LEN + iphdrlen);
    if (ethhdr->h_proto == htons(ETH_P_IP) || ethhdr->h_proto == htons(ETH_P_IPV6)) {
        is_dhcpv4 = ((ethhdr->h_proto == htons(ETH_P_IP)) &&
                     ((udphdr->source == htons(DHCP_SP_V4)) || (udphdr->source == htons(DHCP_CP_V4))));
        is_dhcpv6 = ((ethhdr->h_proto == htons(ETH_P_IPV6)) &&
                     ((udphdr->source == htons(DHCP_SP_V6)) || (udphdr->source == htons(DHCP_CP_V6))));
        if (is_dhcpv4 || is_dhcpv6) {
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            offset += sprintf(p + offset, "DHCP[%d %d], vif_idx:%d", is_dhcpv4,is_dhcpv6, aml_vif->vif_index);
            printk("%s\n", p);
            return true;
        }
    }
    return false;
}

uint32_t aml_filter_sp_mgmt_frame(struct aml_vif *vif, u8 *buf, AML_SP_STATUS_E sp_status, u32 frame_len, u32* len_diff)
{
    u32 offset = 0;
    u32 subtype = ((*buf) & IEEE80211_FCTL_STYPE) >> 4;
    u8 str[80];
    u8 *p = str;
    u8 category;
    u8 action_code;
    u8 oui_type;
    u8 oui_subtype;
    uint32_t ret = 0;
    switch (subtype) {
        case ACTION_TYPE: {
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            category = *(buf + CATEGORY_OFFSET);
            action_code = *(buf + ACTION_CODE_OFFSET);
            oui_type = *(buf + OUI_TYPE_OFFSET);
            oui_subtype = *(buf + OUI_SUBTYPE_OFFSET);
            if (category == PUBLIC_ACTION) {
                if ((action_code == ACTION_CODE_VENDOR)
                    && (*(buf + OUI_OFFSET) == 0x50)
                    && (*(buf + OUI_OFFSET + 1) == 0x6f)
                    && (*(buf + OUI_OFFSET + 2) == 0x9a))
                {
                    if (oui_type == OUI_TYPE_P2P) {
                        offset += sprintf(p + offset, "PUBLIC ACTION->%s ", p2p_pub_action_trace[oui_subtype]);

                        ret |= AML_P2P_ACTION_FRAME;
                        //P2P_ACTION_GO_NEG_RSP & P2P_ACTION_GO_NEG_CFM & P2P_ACTION_INVIT_RSP:need sw retry
                        if ((oui_subtype == P2P_ACTION_GO_NEG_RSP) || (oui_subtype == P2P_ACTION_GO_NEG_CFM) || (oui_subtype == P2P_ACTION_INVIT_RSP)) {
                            ret |= AML_SP_FRAME;
                        }
#ifdef DRV_P2P_SCC_MODE
                        if ((sp_status == SP_STATUS_TX_START) && (len_diff != NULL)) {
                            if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_INVIT_REQ))
                                AML_SCC_SET_P2P_PEER_5G_SUPPORT(false); //rest 5g support flag

                            if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_GO_NEG_RSP) || (oui_subtype == P2P_ACTION_INVIT_REQ) || (oui_subtype == P2P_ACTION_INVIT_RSP)) {
                                struct aml_vif *sta_vif ;
                                sta_vif = vif->aml_hw->vif_table[0];
                                if (sta_vif && sta_vif->sta.ap && (sta_vif->sta.ap->valid)) {
                                    struct cfg80211_chan_def target_chdef;
                                    target_chdef = vif->aml_hw->chanctx_table[sta_vif->ch_index].chan_def;
                                    //AML_INFO("[P2P SCC] p2p channel to:%d", aml_ieee80211_freq_to_chan(target_chdef.chan->center_freq, target_chdef.chan->band));
                                    AML_SCC_SAVE_P2P_ACTION_FRAME(buf, frame_len);
                                    AML_SCC_SAVE_P2P_ACTION_LEN(frame_len);
                                    aml_change_p2p_chanlist(vif, buf, frame_len, len_diff, target_chdef);
                                    aml_change_p2p_operchan(vif, buf, frame_len, target_chdef);
                                    AML_SCC_SAVE_P2P_ACTION_LEN_DIFF(*len_diff);
                                }
                            }
                        }

                        if (((sp_status == SP_STATUS_TX_SUC) || (sp_status == SP_STATUS_TX_FAIL)) && (len_diff != NULL))  {
                            if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_GO_NEG_RSP) || (oui_subtype == P2P_ACTION_INVIT_REQ) || (oui_subtype == P2P_ACTION_INVIT_RSP)) {
                                aml_scc_p2p_action_restore(buf, len_diff);
                            }
                        }
#endif
#if 0    //code for change p2p go intent
                        if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_GO_NEG_RSP)) {
                            aml_change_p2p_intent(vif, buf, frame_len, frame_len_offset);
                        }
#endif
                    }
                    else if (oui_type == OUI_TYPE_DPP) {
                            u8 dpp_action_subtype = *(buf + DPP_PUBLIC_ACTION_SUBTYPE_OFFSET);
                            offset += sprintf(p + offset, "DPP ACTION->%s ", dpp_pub_action_trace[dpp_action_subtype]);
                            ret |= AML_SP_FRAME | AML_DPP_ACTION_FRAME;
                    }
                }
                else if ((action_code == ACTION_GAS_INIT_REQ)
                        || (action_code == ACTION_GAS_INIT_RSP)) {
                        u8 tag_len = *(buf + CATEGORY_OFFSET + 4);
                        offset += sprintf(p + offset, "GAS ACTION,action code:%d ", action_code);

                        ret |= AML_SP_FRAME | AML_GAS_ACTION_FRAME;

                        if (action_code == ACTION_GAS_INIT_REQ)
                            ret |= AML_GAS_INIT_REQ_FRAME;
                        else if (action_code == ACTION_GAS_INIT_RSP)
                            ret |= AML_GAS_INIT_RSP_FRAME;

                        if ((tag_len >= 8)
                            && (*(buf + CATEGORY_OFFSET + 6) == 0xdd)
                            && (*(buf + CATEGORY_OFFSET + 7) == 0x05)
                            && (*(buf + CATEGORY_OFFSET + 8) == 0x50)
                            && (*(buf + CATEGORY_OFFSET + 9) == 0x6f)
                            && (*(buf + CATEGORY_OFFSET + 10) == 0x9a)
                            && (*(buf + CATEGORY_OFFSET + 11) == 0x1a)
                            && (*(buf + CATEGORY_OFFSET + 12) == 0x01)) {
                                offset += sprintf(p + offset, "[DPP Configuration]");
                                ret |= AML_DPP_ACTION_FRAME;
                            }
                }
                else if (action_code == WLAN_PUB_ACTION_EXT_CHANSW_ANN) {
                    ret |= AML_CSA_ACTION_FRAME;
                }
            }
            else if (category == P2P_ACTION) {
                offset += sprintf(p + offset, "P2P ACTION->%s ", p2p_action_trace[oui_subtype]);
            }
            else {
                offset += sprintf(p + offset, "ACTION category %d ", category);
            }
            p[offset] = '\0';

            printk("%s\n", p);
            return ret;
            break;
        }
        case AUTH_TYPE: {
            u16 auth_algo = *(buf + AUTH_ALGO_OFFSET);
            ret |= AML_SP_FRAME;
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            offset += sprintf(p + offset, "auth algo:%d ", auth_algo);
            p[offset] = '\0';
            printk("%s\n", p);
            if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) && (vif->sta.flags & AML_STA_EXT_AUTH)) {
                u8 *p = buf + AUTH_ALGO_OFFSET;
                vif->sta.auth_status = (*(p + 4) | *(p + 5) << 8);
                printk("auth status %d\n", vif->sta.auth_status);
            }
            return ret;
        }
        case ASSOC_RSP_TYPE: {
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            offset += sprintf(p + offset, "ASSOC_RSP");
            p[offset] = '\0';
            printk("%s\n", p);
            return ret;
        }
        case PROBE_RSP_TYPE: {
            if (sp_status == SP_STATUS_TX_START) {
                aml_scc_save_probe_rsp(vif, (u8*)buf, frame_len);
            }
            return ret;
        }
        default:
            break;
        }

    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
#include <linux/kallsyms.h>
#include <net/sock.h>
void aml_pkt_orphan_partial(struct sk_buff *skb, int tsq)
{
    u32 fraction;
    static void *p_tcp_wfree = NULL;

    if (tsq <= 0) {
        return;
    }
    if (!skb->destructor || skb->destructor == sock_wfree) {
        return;
    }
    if (unlikely(!p_tcp_wfree)) {
        char sym[KSYM_SYMBOL_LEN];
        sprint_symbol(sym, (unsigned long)skb->destructor);
        sym[9] = 0;
        if (!strcmp(sym, "tcp_wfree")) {
            p_tcp_wfree = skb->destructor;
        }
        else {
            return;
        }
    }

    if (unlikely(skb->destructor != p_tcp_wfree || !skb->sk)) {
      return;
    }
    /* abstract a certain portion of skb truesize from the socket
           * sk_wmem_alloc to allow more skb can be allocated for this
           * socket for better cusion meeting WiFi device requirement
           */
    fraction = skb->truesize * (tsq - 1) / tsq;
    skb->truesize -= fraction;

    atomic_sub(fraction, &skb->sk->sk_wmem_alloc);
    skb_orphan(skb);
}
#endif


/**
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *	Called when a packet needs to be transmitted.
 *	Must return NETDEV_TX_OK , NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED if NETIF_F_LLTX)
 *
 *  - Initialize the desciptor for this pkt (stored in skb before data)
 *  - Push the pkt in the corresponding Txq
 *  - If possible (i.e. credit available and not in PS) the pkt is pushed
 *    to fw
 */
netdev_tx_t aml_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_txhdr *txhdr;
    struct aml_sw_txhdr *sw_txhdr = NULL;
    struct ethhdr *eth;
    struct txdesc_api *desc;
    struct aml_sta *sta;
    struct aml_txq *txq;
    u8 tid;
    struct aml_sdio_txhdr *sdio_txhdr;
    u32 tx_max_headroom;
#ifdef CONFIG_AML_POWER_SAVE_MODE
    struct aml_plat *aml_plat = aml_hw->plat;
#endif
    struct aml_usb_txhdr *usb_txhdr;
    bool sp_frame = false;

    sk_pacing_shift_update(skb->sk, aml_hw->tcp_pacing_shift);
    if (aml_bus_type == PCIE_MODE) {
        tx_max_headroom = AML_TX_MAX_HEADROOM;
    } else if (aml_bus_type == USB_MODE){
        tx_max_headroom = AML_USB_TX_MAX_HEADROOM;
    } else {
        tx_max_headroom = AML_SDIO_TX_MAX_HEADROOM;
    }
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_IPC_ONGOING | AML_RECY_DROP_XMIT_PKT)) {
        goto free;
    }
#endif

#ifdef CONFIG_AML_POWER_SAVE_MODE
    if (aml_bus_type == PCIE_MODE)
    {
        aml_prevent_fw_sleep(aml_plat, PS_TX_START);
        aml_wait_fw_wake(aml_plat);
    }
#endif
    // If buffer is shared (or may be used by another interface) need to make a
    // copy as TX infomration is stored inside buffer's headroom
    if (skb_shared(skb) || (skb_headroom(skb) < tx_max_headroom) ||
        (skb_cloned(skb) && (dev->priv_flags & IFF_BRIDGE_PORT))) {
        struct sk_buff *newskb = skb_copy_expand(skb, tx_max_headroom, 0, GFP_ATOMIC);
        if (unlikely(newskb == NULL))
            goto free;

        dev_kfree_skb_any(skb);
        skb = newskb;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
    aml_pkt_orphan_partial(skb, aml_hw->tsq);
#endif
    /* Get the STA id and TID information */
    sta = aml_get_tx_info(aml_vif, skb, &tid);
    if (!sta)
        goto free;

    if (aml_filter_tx_tcp_ack(dev, skb, sta))
        return NETDEV_TX_OK;

    /**
     * filer special frame,reuse TXU_CNTRL_MESH_FWD
     * TBD,use own flag in next rom version
     */
    if (aml_filter_sp_data_frame(skb,aml_vif,SP_STATUS_TX_START)) {
        sp_frame = true;
        txq = aml_txq_sta_get(sta, tid, aml_hw);
        tid = 0xff;
    } else {
        txq = aml_txq_sta_get(sta, tid, aml_hw);
    }

    if (txq->idx == TXQ_INACTIVE)
        goto free;


#ifdef CONFIG_AML_AMSDUS_TX
    if (aml_amsdu_add_subframe(aml_hw, skb, sta, txq))
        return NETDEV_TX_OK;
#endif

    sw_txhdr = kmem_cache_zalloc(aml_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL))
        goto free;

    sw_txhdr->txq       = txq;
    sw_txhdr->frame_len = AML_TX_DMA_MAP_LEN(skb);
    sw_txhdr->aml_sta  = sta;
    sw_txhdr->aml_vif  = aml_vif;
    sw_txhdr->skb       = skb;
    sw_txhdr->jiffies   = jiffies;
#ifdef CONFIG_AML_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif

    /* Prepare IPC buffer for DMA transfer */
    eth = (struct ethhdr *)skb->data;
    if (unlikely(aml_prep_dma_tx(aml_hw, sw_txhdr, eth + 1)))
        goto free;

    /* Fill-in the API descriptor for the MACSW */
    desc = &sw_txhdr->desc.api;
    memcpy(&desc->host.eth_dest_addr, eth->h_dest, ETH_ALEN);
    memcpy(&desc->host.eth_src_addr, eth->h_source, ETH_ALEN);
    desc->host.ethertype = eth->h_proto;
    desc->host.staid = sta->sta_idx;
    desc->host.tid = tid;
    if (unlikely(aml_vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN))
        desc->host.vif_idx = aml_vif->ap_vlan.master->vif_index;
    else
        desc->host.vif_idx = aml_vif->vif_index;
    desc->host.flags = 0;

    if (aml_vif->use_4addr && (sta->sta_idx < NX_REMOTE_STA_MAX))
        desc->host.flags |= TXU_CNTRL_USE_4ADDR;

    if ((aml_vif->tdls_status == TDLS_LINK_ACTIVE) &&
        aml_vif->sta.tdls_sta &&
        (memcmp(desc->host.eth_dest_addr.array, aml_vif->sta.tdls_sta->mac_addr, ETH_ALEN) == 0)) {
        desc->host.flags |= TXU_CNTRL_TDLS;
        aml_vif->sta.tdls_sta->tdls.last_tid = desc->host.tid;
        aml_vif->sta.tdls_sta->tdls.last_sn = 0; //TODO: set this on confirm ?
    }

    if ((aml_vif->wdev.iftype == NL80211_IFTYPE_MESH_POINT) &&
        (aml_vif->is_resending)) {
        desc->host.flags |= TXU_CNTRL_MESH_FWD;
    }

    if (sp_frame) {
        desc->host.flags |= TXU_CNTRL_SP_FRAME;
    }
    if (aml_bus_type == PCIE_MODE) {
        /* store Tx info in skb headroom */
        txhdr = (struct aml_txhdr *)skb_push(skb, AML_TX_HEADROOM);
        txhdr->sw_hdr = sw_txhdr;
    } else {
        if (aml_bus_type == USB_MODE) {
            /* store Tx info in skb headroom */
            skb_pull(skb, sizeof(struct ethhdr));
            usb_txhdr = (struct aml_usb_txhdr *)skb_push(skb, AML_USB_TX_HEADROOM);
            usb_txhdr->sw_hdr = sw_txhdr;
        } else {

            skb_pull(skb, sizeof(struct ethhdr));
            sdio_txhdr = (struct aml_sdio_txhdr *)skb_push(skb, AML_SDIO_TX_HEADROOM);
            sdio_txhdr->sw_hdr = sw_txhdr;
            sdio_txhdr->mpdu_buf_flag = 0;
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG|HW_LAST_AGG_FLAG;
            sdio_txhdr->mpdu_buf_flag |= HW_MPDU_LEN_SET(sw_txhdr->frame_len + SDIO_FRAME_TAIL_LEN);

            memset(&sdio_txhdr->tx_vector, 0, sizeof(struct hw_tx_vector_bits) + sizeof(struct txdesc_host)/*8 byte alignment*/);
        }

    }
    if (aml_bus_type != PCIE_MODE) {
        AML_PRINT(AML_DBG_MODULES_TX, "ethertype:0x%04x, credits:%d, tid:%d, vif_idx:%d\n",
                cpu_to_be16(desc->host.ethertype), txq->credits, desc->host.tid, desc->host.vif_idx);
    }

    /* queue the buffer */
    spin_lock_bh(&aml_hw->tx_lock);

    if (txq->idx == TXQ_INACTIVE ) {
        printk("%s:%d Get txq idx is inactive after spin_lock_bh  \n",__func__, __LINE__);
       //"do not push and process it with kernel list lib it whill be re-pull out and used this freed buf"
       spin_unlock_bh(&aml_hw->tx_lock);
       goto free;
    }

    if (aml_txq_queue_skb(skb, txq, aml_hw, false, NULL)) {
        if (skb_queue_empty(&txq->sk_list))
            AML_INFO("txq queue skb list empty");
        aml_hwq_process(aml_hw, txq->hwq);
    }
    spin_unlock_bh(&aml_hw->tx_lock);

    return NETDEV_TX_OK;

free:
    if (sw_txhdr)
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
    dev_kfree_skb_any(skb);
    /* coverity[leaked_storage] - variable "sw_txhdr" was free in kmem_cache_free */
    return NETDEV_TX_OK;
}

/**
 * aml_start_mgmt_xmit - Transmit a management frame
 *
 * @vif: Vif that send the frame
 * @sta: Destination of the frame. May be NULL if the destiantion is unknown
 *       to the AP.
 * @params: Mgmt frame parameters
 * @offchan: Indicate whether the frame must be send via the offchan TXQ.
 *           (is is redundant with params->offchan ?)
 * @cookie: updated with a unique value to identify the frame with upper layer
 *
 */
int aml_start_mgmt_xmit(struct aml_vif *vif, struct aml_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         u64 *cookie)
{
    struct aml_hw *aml_hw = vif->aml_hw;
    struct aml_txhdr *txhdr;
    struct aml_sw_txhdr *sw_txhdr;
    struct txdesc_api *desc;
    struct sk_buff *skb;
    size_t frame_len;
    u8 *data;
    struct aml_txq *txq;
    bool robust;
    struct aml_sdio_txhdr *sdio_txhdr;
    u32 tx_headroom;
    struct aml_usb_txhdr *usb_txhdr;
    u32 len_diff = 0;
    u32 sp_mgmt_ret;
    frame_len = params->len;

    /* Set TID and Queues indexes */
    if (sta) {
        txq = aml_txq_sta_get(sta, 8, aml_hw);
    } else {
        if (offchan) {
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
            txq = aml_hw->txq + NX_OFF_CHAN_TXQ_IDX;
#else
            txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
#endif
        } else {
            txq = aml_txq_vif_get(vif, NX_UNK_TXQ_TYPE);
        }
    }

    if (aml_bus_type == PCIE_MODE) {
        tx_headroom = AML_TX_HEADROOM;
    } else if (aml_bus_type == USB_MODE){
        tx_headroom = AML_USB_TX_HEADROOM;
    } else {
        tx_headroom = AML_SDIO_TX_HEADROOM;
    }

    /* Create a SK Buff object that will contain the provided data */
    skb = dev_alloc_skb(tx_headroom + frame_len);
    if (!skb)
        return -ENOMEM;
    *cookie = (unsigned long)skb;

    sw_txhdr = kmem_cache_alloc(aml_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL)) {
        dev_kfree_skb(skb);
        return -ENOMEM;
    }
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_IPC_ONGOING | AML_RECY_DROP_XMIT_PKT)) {
        dev_kfree_skb(skb);
        return -ENOMEM;
    }
#endif

    /* Reserve headroom in skb. Do this so that we can easily re-use ieee80211
       functions that take skb with 802.11 frame as parameter */
    skb_reserve(skb, tx_headroom);
    skb_reset_mac_header(skb);

    /* Copy data in skb buffer */
    data = skb_put(skb, frame_len);
    memcpy(data, params->buf, frame_len);
    sp_mgmt_ret = aml_filter_sp_mgmt_frame(vif, data, SP_STATUS_TX_START, frame_len, &len_diff);
    if ((sp_mgmt_ret & AML_DPP_ACTION_FRAME) && offchan) {
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
        txq = aml_hw->txq + NX_OFF_CHAN_TXQ_IDX;
#else
        txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
#endif
        AML_INFO("DPP change txq ---> off chan");
    }
    if (len_diff) {
        AML_INFO("[P2P SCC] frame_len:%d frame_len_diff:%d", frame_len, len_diff);
        frame_len -= len_diff;
    }
    robust = ieee80211_is_robust_mgmt_frame(skb);

    /* Update CSA counter if present */
    if (unlikely(params->n_csa_offsets) &&
        vif->wdev.iftype == NL80211_IFTYPE_AP &&
        vif->ap.csa) {
        int i;
        for (i = 0; i < params->n_csa_offsets ; i++) {
            data[params->csa_offsets[i]] = vif->ap.csa->count;
        }
    }

    sw_txhdr->txq = txq;
    sw_txhdr->frame_len = frame_len;
    sw_txhdr->aml_sta = sta;
    sw_txhdr->aml_vif = vif;
    sw_txhdr->skb = skb;
    sw_txhdr->jiffies = jiffies;
#ifdef CONFIG_AML_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif

    /* Prepare IPC buffer for DMA transfer */
    if (unlikely(aml_prep_dma_tx(aml_hw, sw_txhdr, data))) {
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
        dev_kfree_skb(skb);
        /* coverity[leaked_storage] - variable "sw_txhdr" was free in kmem_cache_free */
        return -EBUSY;
    }

    /* Fill-in the API Descriptor for the MACSW */
    desc = &sw_txhdr->desc.api;
    desc->host.staid = (sta) ? sta->sta_idx : 0xFF;
    desc->host.vif_idx = vif->vif_index;
    desc->host.tid = 0xFF;
    desc->host.flags = TXU_CNTRL_MGMT;
    if (sp_mgmt_ret & AML_SP_FRAME) {
        desc->host.flags |= TXU_CNTRL_SP_FRAME;
    }

    if (robust)
        desc->host.flags |= TXU_CNTRL_MGMT_ROBUST;

    if (params->no_cck)
        desc->host.flags |= TXU_CNTRL_MGMT_NO_CCK;

    if (aml_bus_type == PCIE_MODE) {
        /* store Tx info in skb headroom */
        txhdr = (struct aml_txhdr *)skb_push(skb, tx_headroom);
        txhdr->sw_hdr = sw_txhdr;
    } else {
        if (aml_bus_type == USB_MODE) {
            /* store Tx info in skb headroom */
           usb_txhdr = (struct aml_usb_txhdr *)skb_push(skb, tx_headroom);
           usb_txhdr->sw_hdr = sw_txhdr;
        } else {
            sdio_txhdr = (struct aml_sdio_txhdr *)skb_push(skb, tx_headroom);
            sdio_txhdr->sw_hdr = sw_txhdr;
            sdio_txhdr->mpdu_buf_flag = 0;
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG;
            sdio_txhdr->mpdu_buf_flag |= HW_MPDU_LEN_SET(sw_txhdr->frame_len + SDIO_FRAME_TAIL_LEN);

            memset(&sdio_txhdr->tx_vector, 0, sizeof(struct hw_tx_vector_bits) + sizeof(struct txdesc_host)/*8 byte alignment*/);
        }
    }

    /* queue the buffer */
    spin_lock_bh(&aml_hw->tx_lock);

    /* ensure that TXQ is active */
    if (txq->idx == TXQ_INACTIVE) {
        struct ieee80211_mgmt *mgmt = (void *)params->buf;
        AML_INFO("txq inactive\n");
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
        dev_kfree_skb(skb);
        spin_unlock_bh(&aml_hw->tx_lock);
        if (ieee80211_is_action(mgmt->frame_control)) {
            AML_INFO("report action tx status as txq inactive");
            cfg80211_mgmt_tx_status(&(vif->wdev), *cookie, params->buf, params->len, 0, GFP_ATOMIC);
            return 0;
        }
        return -EBUSY;
    }
    if (aml_txq_queue_skb(skb, txq, aml_hw, false, NULL)) {
        if (skb_queue_empty(&txq->sk_list))
            AML_INFO("txq queue skb list empty");
        aml_hwq_process(aml_hw, txq->hwq);
    }

    spin_unlock_bh(&aml_hw->tx_lock);

    return 0;
}

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
cfm_log cfmlog = {0};
#endif
#endif

extern struct aml_bus_state_detect bus_state_detect;
int aml_update_tx_cfm(void *pthis)
{
    struct aml_hw *aml_hw = pthis;
    struct tx_sdio_usb_cfm_tag *read_cfm;
    int actual_length  = 0;
    int ret = 0;
#ifdef CONFIG_SDIO_TX_ENH
    unsigned int blk_size = 512;
#endif

    read_cfm = aml_hw->read_cfm;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err) {
        AML_INFO("bus err(%d), return\n", bus_state_detect.bus_err);
        return 0;
    }
#endif
    if (aml_bus_type == USB_MODE) {
        ret = usb_bulk_msg(aml_hw->plat->usb_dev, usb_rcvbulkpipe(aml_hw->plat->usb_dev, USB_EP5), (void *)read_cfm, sizeof(struct tx_sdio_usb_cfm_tag) * (SRAM_TXCFM_CNT), &actual_length, 100);
        if (ret)
            printk("usb bulk failed actual len is %d\n",actual_length);
    } else if (aml_bus_type == SDIO_MODE) {
#ifdef CONFIG_SDIO_TX_ENH
        if (aml_hw->txcfm_param.dyn_en) {
            uint32_t pushed_occupy_blk = 0;
            uint32_t read_blk = 6;
            uint32_t start_blk = 0;

            spin_lock_bh(&aml_hw->txcfm_rd_lock);
            pushed_occupy_blk = aml_hw->txcfm_param.hostid_pushed / TAGS_IN_SDIO_BLK;
            pushed_occupy_blk += (aml_hw->txcfm_param.hostid_pushed % TAGS_IN_SDIO_BLK) ? 1 : 0;
            if (pushed_occupy_blk > aml_hw->txcfm_param.read_blk)
                aml_hw->txcfm_param.read_blk = pushed_occupy_blk;

            /* make sure the read blocks should not be out of TXCFM sharemem range */
            /* reset txcfm reading as more cfm tags in fw */
            if (aml_hw->txcfm_param.start_blk > 5 || aml_hw->txcfm_param.read_blk == 0 ||
                        (aml_hw->txcfm_param.start_blk + aml_hw->txcfm_param.read_blk) > 6) {
                aml_hw->txcfm_param.thresh_cnt = 0;
                aml_hw->txcfm_param.read_blk = 6;
                aml_hw->txcfm_param.start_blk  = 0;
            }
            read_blk  = aml_hw->txcfm_param.read_blk;
            start_blk = aml_hw->txcfm_param.start_blk;
            spin_unlock_bh(&aml_hw->txcfm_rd_lock);

            aml_hw->plat->hif_sdio_ops->hi_sram_read((unsigned char *)(&aml_hw->read_cfm[start_blk*TAGS_IN_SDIO_BLK]),
                (unsigned char *)(SRAM_TXCFM_START_ADDR + start_blk * blk_size), read_blk * blk_size);

#ifdef SDIO_TX_ENH_DBG
            cfmlog.cfm_read_cnt++;
            cfmlog.cfm_read_blk_cnt += aml_hw->txcfm_param.read_blk;
            cfmlog.start_blk = aml_hw->txcfm_param.start_blk;
            cfmlog.read_blk = aml_hw->txcfm_param.read_blk;
#endif
        } else {
            aml_hw->plat->hif_sdio_ops->hi_sram_read((unsigned char *)(read_cfm),
                (unsigned char *)SRAM_TXCFM_START_ADDR, sizeof(struct tx_sdio_usb_cfm_tag) * SRAM_TXCFM_CNT);
        }
#else
        aml_hw->plat->hif_sdio_ops->hi_sram_read((unsigned char *)(read_cfm),
            (unsigned char *)SRAM_TXCFM_START_ADDR, sizeof(struct tx_sdio_usb_cfm_tag) * SRAM_TXCFM_CNT);
#endif
    }

    up(&aml_hw->aml_txcfm_sem);
    return 0;
}


#ifdef CONFIG_SDIO_TX_ENH
void txcfm_analyze_handler(struct aml_hw *aml_hw, uint32_t cur_tags, uint32_t pre_tags, uint32_t txcfm_idx)
{
    uint32_t relative_idx = (txcfm_idx + 1) % TAGS_IN_SDIO_BLK;
    uint32_t cur_blk_idx = (txcfm_idx) / TAGS_IN_SDIO_BLK;
    uint32_t left_tag_num = TAGS_IN_SDIO_BLK - relative_idx;
    uint32_t occupy_blk = 0;

    if (aml_bus_type != SDIO_MODE)
        return;

    spin_lock_bh(&aml_hw->txcfm_rd_lock);
    if (cur_tags <= pre_tags) {
        aml_hw->txcfm_param.thresh_cnt++;

        /* suppose the txcfm reading are in a stable state, adjust txcfm reading blocks */
        if (aml_hw->txcfm_param.thresh_cnt == TXCFM_THRESH) {
            aml_hw->txcfm_param.thresh_cnt = 0;

            if (left_tag_num >= cur_tags) {
                /* if left tags number is enough per current txcfm tags, only need read one block */
                aml_hw->txcfm_param.start_blk = cur_blk_idx;
                aml_hw->txcfm_param.read_blk = 1;
            } else {
                /* calculate the occupy blocks per current txcfm tag numbers */
                occupy_blk = cur_tags / TAGS_IN_SDIO_BLK;
                occupy_blk += (left_tag_num != 0) ? 1 : 0;
                occupy_blk += (cur_tags % TAGS_IN_SDIO_BLK) ? 1 : 0;
                if (cur_blk_idx + occupy_blk < 6) {
                    /* use predicted occupy blocks for the next txcfm reading */
                    aml_hw->txcfm_param.start_blk = cur_blk_idx;
                    aml_hw->txcfm_param.read_blk = occupy_blk;
                } else {
                    /* if predicted occupy blocks will over TXCFM sharemem range, reset to read all */
                    aml_hw->txcfm_param.start_blk = 0;
                    aml_hw->txcfm_param.read_blk = 6;
                    aml_hw->txcfm_param.thresh_cnt = 0;
                }
            }
        } else {
            aml_hw->txcfm_param.start_blk = cur_blk_idx;
            if (aml_hw->txcfm_param.start_blk + aml_hw->txcfm_param.read_blk > 6) {
                /* if last read blocks will over TXCFM sharemem range, reset to read all */
                aml_hw->txcfm_param.start_blk = 0;
                aml_hw->txcfm_param.read_blk = 6;
            } else {
                /* calculate the occupy blocks per current txcfm tag numbers */
                occupy_blk = cur_tags / TAGS_IN_SDIO_BLK;
                occupy_blk += (left_tag_num != 0) ? 1 : 0;
                occupy_blk += (cur_tags % TAGS_IN_SDIO_BLK) ? 1 : 0;
                if (occupy_blk > aml_hw->txcfm_param.read_blk) {
                    if (aml_hw->txcfm_param.start_blk + occupy_blk > 6) {
                        /* if predicted occupy blocks will over TXCFM sharemem range, reset to read all */
                        aml_hw->txcfm_param.start_blk = 0;
                        aml_hw->txcfm_param.read_blk = 6;
                    } else {
                        /* need enlarge read blocks if txcfm_read_blk is small */
                        aml_hw->txcfm_param.start_blk = cur_blk_idx;
                        aml_hw->txcfm_param.read_blk = occupy_blk;
                    }
                }
            }
        }
    } else {
        /* reset txcfm reading as current handled tags are larged than previous one */
        aml_hw->txcfm_param.thresh_cnt = 0;
        aml_hw->txcfm_param.read_blk = 6;
        aml_hw->txcfm_param.start_blk  = 0;
    }

    aml_hw->txcfm_param.pre_tag = cur_tags;
    spin_unlock_bh(&aml_hw->txcfm_rd_lock);
}
#endif

int aml_tx_cfm_task(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;
    struct sk_buff *skb = NULL;
    struct tx_sdio_usb_cfm_tag cfm_data;
    struct tx_cfm_tag cfm;
    struct aml_sw_txhdr *sw_txhdr;
    struct aml_hwq *hwq;
    struct aml_txq *txq;
    struct tx_sdio_usb_cfm_tag *read_cfm;
    unsigned int drv_txcfm_idx = aml_hw->ipc_env->txcfm_idx;
    u8 i = 0;
    unsigned int frame_tot_len = 0;
    struct txdesc_host *txdesc_host = NULL;
    unsigned char  page_num = 0;
    struct sched_param sch_param;
    uint32_t sp_ret = 0;

    sch_param.sched_priority = 91;
#ifndef CONFIG_PT_MODE
    sched_setscheduler(current, SCHED_FIFO, &sch_param);
#endif
    while (!aml_hw->aml_txcfm_task_quit) {
        /* wait for work */
        if (down_interruptible(&aml_hw->aml_txcfm_sem) != 0) {
            /* interrupted, exit */
            AML_PRINT(AML_DBG_MODULES_TX, "wait aml_txcfm_sem fail!\n");
            break;
        }
        if (aml_hw->aml_txcfm_task_quit) {
            break;
        }

        spin_lock_bh(&aml_hw->tx_lock);
        read_cfm = aml_hw->read_cfm;

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
        cfmlog.cfm_rx_cnt++;
        cfmlog.cfm_num = 0;
#endif
#endif

        for (i = 0; i < SRAM_TXCFM_CNT; i++, drv_txcfm_idx = (drv_txcfm_idx + 1) % SRAM_TXCFM_CNT) {
            if (aml_hw->aml_txcfm_task_quit) {
                break;
            }
            aml_hw->ipc_env->txcfm_idx = drv_txcfm_idx;
            cfm_data = read_cfm[drv_txcfm_idx];
            cfm.credits = cfm_data.credits;
            cfm.ampdu_size = cfm_data.ampdu_size;
#ifdef CONFIG_AML_SPLIT_TX_BUF
            cfm.amsdu_size = cfm_data.amsdu_size;
#endif
            cfm.status.value = (u32)cfm_data.status.value;
            cfm.hostid = (u32_l)cfm_data.hostid;
            skb = ipc_host_tx_host_id_to_ptr_for_sdio_usb(aml_hw->ipc_env, cfm.hostid);

#ifdef CONFIG_SDIO_TX_ENH
            if (!skb) {
                if (aml_hw->txcfm_param.dyn_en)
                    txcfm_analyze_handler(aml_hw, i, aml_hw->txcfm_param.pre_tag, drv_txcfm_idx);

                #ifdef SDIO_TX_ENH_DBG
                cfmlog.drv_txcfm_idx = drv_txcfm_idx;
                #endif

                break;
            }
#else
            if (!skb)
                break;
#endif

#ifdef CONFIG_SDIO_TX_ENH
            if (aml_bus_type == SDIO_MODE)
                aml_hw->txcfm_param.hostid_pushed--;
#ifdef SDIO_TX_ENH_DBG
            cfmlog.cfm_num++;
            cfmlog.hostid_pushed = aml_hw->txcfm_param.hostid_pushed;
#endif
#endif

            sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;
            txq = sw_txhdr->txq;
            if (aml_bus_type == SDIO_MODE) {
                frame_tot_len = 0;
                txdesc_host = &sw_txhdr->desc;
                for (i = 0; i < txdesc_host->api.host.packet_cnt; i++) {
                    frame_tot_len += txdesc_host->api.host.packet_len[i];
                }
                page_num = howmanypage(frame_tot_len + SDIO_DATA_OFFSET + SDIO_FRAME_TAIL_LEN, SDIO_PAGE_LEN);
            } else {
                #ifdef CONFIG_AML_USB_LARGE_PAGE
                page_num = 1;
                #else
                page_num = sw_txhdr->desc.api.host.packet_cnt ;
                #endif
            }
            spin_lock_bh(&aml_hw->tx_buf_lock);
            aml_hw->g_tx_param.tx_page_free_num += page_num;

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
            cfmlog.cfm_page += page_num;
#endif
#endif
            spin_unlock_bh(&aml_hw->tx_buf_lock);
            AML_PRINT(AML_DBG_MODULES_TX, "%s, tx_page_free_num=%d, credit=%d, pagenum=%d, skb=%p, cfm.credits=%d, drv_txcfm_idx=%d\n", __func__, aml_hw->g_tx_param.tx_page_free_num, txq->credits, page_num, skb, cfm.credits, drv_txcfm_idx);
            if (aml_hw->g_tx_param.tx_page_free_num >= aml_hw->g_tx_param.txcfm_trigger_tx_thr) {
                up(&aml_hw->aml_tx_sem);
            }

            /* don't use txq->hwq as it may have changed between push and confirm */
            hwq = &aml_hw->hwq[sw_txhdr->hw_queue];

            aml_txq_confirm_any(aml_hw, txq, hwq, sw_txhdr);

            /* Update txq and HW queue credits */
            if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_MGMT) {
                struct ieee80211_mgmt *mgmt = NULL;
                bool cfm_tx_status = true;

                trace_mgmt_cfm(sw_txhdr->aml_vif->vif_index,
                    (sw_txhdr->aml_sta) ? sw_txhdr->aml_sta->sta_idx : 0xFF, cfm.status.acknowledged);
                if (aml_bus_type == USB_MODE)
                    mgmt = (struct ieee80211_mgmt *)(skb->data + AML_USB_TX_HEADROOM);
                else if (aml_bus_type == SDIO_MODE)
                    mgmt = (struct ieee80211_mgmt *)(skb->data + AML_SDIO_TX_HEADROOM);
                if ((ieee80211_is_deauth(mgmt->frame_control)) && (sw_txhdr->aml_vif->is_disconnect == 1)) {
                    sw_txhdr->aml_vif->is_disconnect = 0;
                }

                if (ieee80211_is_action(mgmt->frame_control)) {
                    sp_ret = aml_filter_sp_mgmt_frame(sw_txhdr->aml_vif, (u8*)mgmt, cfm.status.acknowledged ? SP_STATUS_TX_SUC:SP_STATUS_TX_FAIL, 0, &(sw_txhdr->frame_len));
                    if (sp_ret & AML_CSA_ACTION_FRAME) {
                        AML_INFO("csa action send cfm, status:%d", cfm.status.acknowledged);
                    }
                }

                if (!cfm.status.acknowledged && (sp_ret & AML_GAS_ACTION_FRAME) && aml_hw->roc && (txq->idx != TXQ_INACTIVE)) {
                    AML_INFO("retry GAS frame during roc");
                    aml_tx_retry(aml_hw, skb, sw_txhdr, cfm.status);
                    continue;
                }

                if (cfm.status.acknowledged && (sp_ret & AML_GAS_INIT_REQ_FRAME)) {
                    sw_txhdr->aml_vif->tx_cfm_wait.skb = skb_copy(skb, GFP_ATOMIC);
                    if (sw_txhdr->aml_vif->tx_cfm_wait.skb) {
                        sw_txhdr->aml_vif->tx_cfm_wait.cookie = (unsigned long)skb;
                        sw_txhdr->aml_vif->tx_cfm_wait.len = sw_txhdr->frame_len;
                        sw_txhdr->aml_vif->tx_cfm_wait.wdev = &sw_txhdr->aml_vif->wdev;
                        cfm_tx_status = false;
                        AML_INFO("gas init frame tx cfm delay, wait for rsp");
                    }
                }

                if (cfm_tx_status) {
                    /* Confirm transmission to CFG80211 */
                    cfg80211_mgmt_tx_status(&sw_txhdr->aml_vif->wdev,
                                        (unsigned long)skb, skb_mac_header(skb),
                                        sw_txhdr->frame_len,
                                        (sp_ret & AML_P2P_ACTION_FRAME) ? 0 : cfm.status.acknowledged,
                                        GFP_ATOMIC);
                }
                sp_ret = 0;
            } else if ((txq->idx != TXQ_INACTIVE) && cfm.status.sw_retry_required) {
                sw_txhdr->desc.api.host.flags |= TXU_CNTRL_RETRY;
                /* firmware postponed this buffer */
                aml_tx_retry(aml_hw, skb, sw_txhdr, cfm.status);
                continue;
            }

            trace_skb_confirm(skb, txq, hwq, &cfm);

            /* STA may have disconnect (and txq stopped) when buffers were stored
                        in fw. In this case do nothing when they're returned */
            if (txq->idx != TXQ_INACTIVE) {
                txq->credits++;
                if (txq->credits <= 0) {
                    aml_txq_stop(txq, AML_TXQ_STOP_FULL);
                }
                else if (txq->credits > 0)
                    aml_txq_start(txq, AML_TXQ_STOP_FULL);

                /* continue service period */
                if (unlikely(txq->push_limit && !aml_txq_is_full(txq))) {
                    aml_txq_add_to_hw_list(txq);
                }
            }
            /* coverity[result_independent_of_operands] - Enhance code robustness */
            if (cfm.ampdu_size && (cfm.ampdu_size < IEEE80211_MAX_AMPDU_BUF))
                aml_hw->stats->ampdus_tx[cfm.ampdu_size - 1]++;

#ifdef CONFIG_AML_AMSDUS_TX
            if (!cfm.status.acknowledged) {
                if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU)
                    aml_hw->stats->amsdus[sw_txhdr->amsdu.nb - 1].failed++;
                else if (!sw_txhdr->aml_sta || !is_multicast_sta(sw_txhdr->aml_sta->sta_idx))
                    aml_hw->stats->amsdus[0].failed++;
            }
            aml_amsdu_update_len(aml_hw, txq, cfm.amsdu_size);
#endif

  /* Release SKBs */
#ifdef CONFIG_AML_AMSDUS_TX
            if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU) {
                struct aml_amsdu_txhdr *amsdu_txhdr, *tmp;
                list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                    aml_amsdu_del_subframe_header(amsdu_txhdr);
                    if (aml_bus_type == PCIE_MODE) {
                        aml_ipc_buf_a2e_release(aml_hw, &amsdu_txhdr->ipc_data);
                    }
                    aml_tx_statistic(sw_txhdr->aml_vif, txq, cfm.status, amsdu_txhdr->msdu_len);
                    consume_skb(amsdu_txhdr->skb);
                }
            }

#endif /* CONFIG_AML_AMSDUS_TX */

            if (aml_bus_type == PCIE_MODE) {
                aml_ipc_buf_a2e_release(aml_hw, &sw_txhdr->ipc_data);
            }
            aml_tx_statistic(sw_txhdr->aml_vif, txq, cfm.status, sw_txhdr->frame_len);

            kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
            if (aml_bus_type == SDIO_MODE) {
                skb_pull(skb, AML_SDIO_TX_HEADROOM);
            } else {
                skb_pull(skb, AML_USB_TX_HEADROOM);
            }

            consume_skb(skb);
        }

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
        /* tx cfm statistic */
        cfmlog.total_cfm += cfmlog.cfm_num;
        cfmlog.avg_cfm = cfmlog.total_cfm/cfmlog.cfm_rx_cnt;
        cfmlog.avg_cfm_page = cfmlog.cfm_page/cfmlog.cfm_rx_cnt;
#endif
#endif

        spin_unlock_bh(&aml_hw->tx_lock);
    }
    if (aml_hw->aml_txcfm_completion_init) {
        aml_hw->aml_txcfm_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
        complete_and_exit(&aml_hw->aml_txcfm_completion, 0);
#else
        complete(&aml_hw->aml_txcfm_completion);
#endif
    }

    return 0;
}

/**
 * aml_txdatacfm - FW callback for TX confirmation
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct aml_hw is this case)
 * @arg: IPC buffer with the TX confirmation
 *
 * This function is called for each confimration of transmission by the fw.
 * Called with tx_lock hold
 *
 */
int aml_txdatacfm(void *pthis, void *arg)
{
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *ipc_cfm = arg;
    struct tx_cfm_tag *cfm = ipc_cfm->addr;
    struct sk_buff *skb;
    struct aml_sw_txhdr *sw_txhdr;
    struct aml_hwq *hwq;
    struct aml_txq *txq;
    uint32_t sp_ret = 0;
    skb = aml_ipc_get_skb_from_cfm(aml_hw, ipc_cfm);
    if (!skb)
        return -1;

    sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;
    txq = sw_txhdr->txq;
    /* don't use txq->hwq as it may have changed between push and confirm */
    hwq = &aml_hw->hwq[sw_txhdr->hw_queue];

    aml_txq_confirm_any(aml_hw, txq, hwq, sw_txhdr);

    /* Update txq and HW queue credits */
    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_MGMT) {
        struct ieee80211_mgmt *mgmt;
        bool cfm_tx_status = true;

        trace_mgmt_cfm(sw_txhdr->aml_vif->vif_index,
                       (sw_txhdr->aml_sta) ? sw_txhdr->aml_sta->sta_idx : 0xFF,
                       cfm->status.acknowledged);
        mgmt = (struct ieee80211_mgmt *)(skb->data + AML_TX_HEADROOM);
        if ((ieee80211_is_deauth(mgmt->frame_control)) && (sw_txhdr->aml_vif->is_disconnect == 1)) {
            sw_txhdr->aml_vif->is_disconnect = 0;
        }
        if (ieee80211_is_action(mgmt->frame_control)) {
            sp_ret = aml_filter_sp_mgmt_frame(sw_txhdr->aml_vif, (u8*)mgmt, cfm->status.acknowledged ? SP_STATUS_TX_SUC:SP_STATUS_TX_FAIL, 0, &(sw_txhdr->frame_len));
            if (sp_ret & AML_CSA_ACTION_FRAME) {
                AML_INFO("csa action send cfm, status:%d", cfm->status.acknowledged);
            }
        }

        if (!cfm->status.acknowledged && (sp_ret & AML_GAS_ACTION_FRAME) && aml_hw->roc && (txq->idx != TXQ_INACTIVE)) {
            AML_INFO("retry GAS frame during roc");
            aml_tx_retry(aml_hw, skb, sw_txhdr, cfm->status);
            return 0;
        }

        if (cfm->status.acknowledged && (sp_ret & AML_GAS_INIT_REQ_FRAME)) {
            sw_txhdr->aml_vif->tx_cfm_wait.skb = skb_copy(skb, GFP_ATOMIC);
            if (sw_txhdr->aml_vif->tx_cfm_wait.skb) {
                sw_txhdr->aml_vif->tx_cfm_wait.cookie = (unsigned long)skb;
                sw_txhdr->aml_vif->tx_cfm_wait.len = sw_txhdr->frame_len;
                sw_txhdr->aml_vif->tx_cfm_wait.wdev = &sw_txhdr->aml_vif->wdev;
                cfm_tx_status = false;
                AML_INFO("gas init frame tx cfm delay, wait for rsp");
            }
        }

        if (cfm_tx_status) {
            /* Confirm transmission to CFG80211 */
            cfg80211_mgmt_tx_status(&sw_txhdr->aml_vif->wdev,
                                    (unsigned long)skb, skb_mac_header(skb),
                                    sw_txhdr->frame_len,
                                    (sp_ret & AML_P2P_ACTION_FRAME) ? 0 : cfm->status.acknowledged,
                                    GFP_ATOMIC);
        }
    } else if ((txq->idx != TXQ_INACTIVE) && cfm->status.sw_retry_required) {
        sw_txhdr->desc.api.host.flags |= TXU_CNTRL_RETRY;
        /* firmware postponed this buffer */
        aml_tx_retry(aml_hw, skb, sw_txhdr, cfm->status);
        return 0;
    }

    trace_skb_confirm(skb, txq, hwq, cfm);

    /* STA may have disconnect (and txq stopped) when buffers were stored
       in fw. In this case do nothing when they're returned */
    if (txq->idx != TXQ_INACTIVE) {
        if (cfm->credits) {
            txq->credits += cfm->credits;
            if (txq->credits <= 0)
                aml_txq_stop(txq, AML_TXQ_STOP_FULL);
            else if (txq->credits > 0)
                aml_txq_start(txq, AML_TXQ_STOP_FULL);
        }

        /* continue service period */
        if (unlikely(txq->push_limit && !aml_txq_is_full(txq))) {
            aml_txq_add_to_hw_list(txq);
        }
    }
    /* coverity[result_independent_of_operands] - Enhance code robustness */
    if (cfm->ampdu_size && (cfm->ampdu_size < IEEE80211_MAX_AMPDU_BUF))
        aml_hw->stats->ampdus_tx[cfm->ampdu_size - 1]++;

#ifdef CONFIG_AML_AMSDUS_TX
    if (!cfm->status.acknowledged) {
        if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU)
            aml_hw->stats->amsdus[sw_txhdr->amsdu.nb - 1].failed++;
        else if (!sw_txhdr->aml_sta || !is_multicast_sta(sw_txhdr->aml_sta->sta_idx))
            aml_hw->stats->amsdus[0].failed++;
    }

    aml_amsdu_update_len(aml_hw, txq, cfm->amsdu_size);
#endif

    /* Release SKBs */
#ifdef CONFIG_AML_AMSDUS_TX
    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU) {
        struct aml_amsdu_txhdr *amsdu_txhdr, *tmp;
        list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
            aml_amsdu_del_subframe_header(amsdu_txhdr);
            aml_ipc_buf_a2e_release(aml_hw, &amsdu_txhdr->ipc_data);
            aml_tx_statistic(sw_txhdr->aml_vif, txq, cfm->status, amsdu_txhdr->msdu_len);
            consume_skb(amsdu_txhdr->skb);
        }
    }
#endif /* CONFIG_AML_AMSDUS_TX */

    aml_ipc_buf_a2e_release(aml_hw, &sw_txhdr->ipc_data);
    aml_tx_statistic(sw_txhdr->aml_vif, txq, cfm->status, sw_txhdr->frame_len);

    kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
    skb_pull(skb, AML_TX_HEADROOM);
    consume_skb(skb);

    return 0;
}

/**
 * aml_txq_credit_update - Update credit for one txq
 *
 * @aml_hw: Driver main data
 * @sta_idx: STA idx
 * @tid: TID
 * @update: offset to apply in txq credits
 *
 * Called when fw send ME_TX_CREDITS_UPDATE_IND message.
 * Apply @update to txq credits, and stop/start the txq if needed
 */
void aml_txq_credit_update(struct aml_hw *aml_hw, int sta_idx, u8 tid, s8 update)
{
    struct aml_sta *sta = aml_hw->sta_table + sta_idx;
    struct aml_txq *txq;
    struct sk_buff *tx_skb;
    int user = 0, credits = 0;

    aml_spin_lock(&aml_hw->tx_lock);
    txq = aml_txq_sta_get(sta, tid, aml_hw);
    if (!txq) {
        aml_spin_unlock(&aml_hw->tx_lock);
        return;
    }
    user = AML_TXQ_POS_ID(txq);
    if (txq->idx != TXQ_INACTIVE) {
#ifdef CONFIG_CREDIT124
        if (update > NX_TXQ_INITIAL_CREDITS) {
            update = TX_MAX_CNT - NX_TXQ_INITIAL_CREDITS;
        }
#else
        if (aml_bus_type != PCIE_MODE && update > NX_TXQ_INITIAL_CREDITS) {
            update = txq->hwq->size - NX_TXQ_INITIAL_CREDITS;
        }
#endif

        credits = txq->credits;
        if (aml_bus_type != PCIE_MODE) {
            if (((txq->credits + update) > 0) &&
                ((txq->credits + txq->pkt_pushed[user] + update) < txq->hwq->size )) {
                txq->credits += update;
            } else {
                txq->credits = txq->hwq->size - txq->pkt_pushed[user];
            }
        } else {
            if ((txq->credits + txq->pkt_pushed[user] + update) < txq->hwq->size ) {
                txq->credits += update;
            } else {
                txq->credits = txq->hwq->size - txq->pkt_pushed[user];
            }
        }
        AML_INFO("sta_idx=%d vif_idx=%d tid=%d update=%d pkt pushed=%d credits=%d old_credits=%d",
                sta_idx, sta->vif_idx, tid, update, txq->pkt_pushed[user], txq->credits, credits);
        trace_credit_update(txq, update);

        if (txq->credits <= 0)
            aml_txq_stop(txq, AML_TXQ_STOP_FULL);
        else
            aml_txq_start(txq, AML_TXQ_STOP_FULL);
    }

    // Drop all the retry packets of a BA that was deleted
    if (update < NX_TXQ_INITIAL_CREDITS) {
        int packet;

        for (packet = 0; packet < txq->nb_retry; packet++) {
            tx_skb = skb_peek(&txq->sk_list);
            if (tx_skb != NULL) {
                AML_INFO("txq drop skb list");
                aml_txq_drop_skb(txq, tx_skb, aml_hw, true);
            }
        }
    }
    aml_spin_unlock(&aml_hw->tx_lock);
}

void aml_tx_cfm_wait_rsp(struct aml_hw *aml_hw, bool ack, u8* func, u32 line)
{
    struct aml_vif *vif;
    struct aml_roc *roc = aml_hw->roc;
    if (!roc)
        return;

    vif = roc->vif;
    if (vif->tx_cfm_wait.cookie == 0)
        return;

    cfg80211_mgmt_tx_status(vif->tx_cfm_wait.wdev,
                        vif->tx_cfm_wait.cookie, skb_mac_header(vif->tx_cfm_wait.skb),
                        vif->tx_cfm_wait.len,
                        ack,
                        GFP_ATOMIC);

    consume_skb(vif->tx_cfm_wait.skb);
    vif->tx_cfm_wait.cookie = 0;
    AML_INFO("ack:%d, [%s %d]", ack, func, line);
}

