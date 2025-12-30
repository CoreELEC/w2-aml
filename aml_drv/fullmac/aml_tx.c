/**
 ******************************************************************************
 *
 * @file aml_tx.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define AML_MODULE   TX

#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include <net/sock.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 0, 0)
#include <net/sch_generic.h>
#endif
#include "aml_defs.h"
#include "aml_tx.h"
#include "aml_msg_tx.h"
#include "aml_p2p.h"
#include "aml_mesh.h"
#include "aml_events.h"
#include "aml_compat.h"
#include "share_mem_map.h"
#include "aml_utils.h"
#include "aml_log.h"
#include "sdio_common.h"
#include "wifi_top_addr.h"
#include "aml_interface.h"
#include "w2_sdio.h"
#ifdef CONFIG_AML_POWER_SAVE_MODE
#include "aml_platform.h"
#endif
#include "aml_scc.h"

static const char *sp_frame_status_trace[] = {
    [SP_STATUS_TX_START] = "[SP FRAME TX]",
    [SP_STATUS_RX] = "[SP FRAME RX]",
    [SP_STATUS_TX_SUC] = "[SP FRAME TX SUC]",
    [SP_STATUS_TX_FAIL] = "[SP FRAME TX FAIL]",
};

static const char *dpp_pub_action_trace_name(int type)
{
   /*Table 31-DPP Public Action Frame Type [easy connect v2.0]*/
   static const char *dpp_pub_action_trace[] = {
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

   return type < ARRAY_SIZE(dpp_pub_action_trace) ? dpp_pub_action_trace[type] : "dpp_pub NULL";
}

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
    struct aml_txq *txq = NULL;

    if (enable) {
        trace_ps_enable(sta);
        aml_spin_lock(&aml_hw->tx_lock);
        sta->ps.active = true;
        sta->ps.sp_cnt[LEGACY_PS_ID] = 0;
        sta->ps.sp_cnt[UAPSD_ID] = 0;
        aml_txq_sta_stop(aml_hw, sta, AML_TXQ_STOP_STA_PS);

        if (is_multicast_sta(sta->sta_idx)) {
            txq = aml_txq_sta_get(sta, 0, aml_hw);
            sta->ps.pkt_ready[LEGACY_PS_ID] = aml_txq_skb_queue_len(txq);
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            txq->hwq = &aml_hw->hwq[AML_HWQ_BE];
        } else {
            int i;
            sta->ps.pkt_ready[LEGACY_PS_ID] = 0;
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            foreach_sta_txq_safe(sta, txq, i, aml_hw) {
                sta->ps.pkt_ready[txq->ps_id] += aml_txq_skb_queue_len(txq);
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

        aml_txq_sta_start(aml_hw, sta, AML_TXQ_STOP_STA_PS);
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
    struct aml_txq *txq = NULL;
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
        aml_txq_start(txq, AML_TXQ_STOP_STA_PS);
    } else {
        int i, tid;

        foreach_sta_txq_prio_safe(sta, txq, tid, i, aml_hw) {
            int txq_len = aml_txq_skb_queue_len(txq);

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
                aml_txq_start(txq, AML_TXQ_STOP_STA_PS);
            } else {
                /* Enough pkt in this txq to complete the request
                   add this txq to hwq list and stop processing txq */
                txq->push_limit = pkt_req;
                sta->ps.sp_cnt[ps_id] += pkt_req;
                aml_txq_start(txq, AML_TXQ_STOP_STA_PS);
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
             "Unexpected ac %d for skb before downgrade", ac))
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
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0) && LINUX_VERSION_CODE <= KERNEL_VERSION(5, 10, 0))
    struct netdev_queue *netq;
    int queue_index = 0, count = 0;
    struct Qdisc *q;
#endif
    u16 netdev_queue = 0;
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
        fallthrough;
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

        if (!aml_vif->is_re_sending) {
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
            AML_INFO("set eap frame to vo\n");
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
                AML_ERR("txq flag is not ok, need wait dev active.  flag: %x\n", q->flags);
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
                          struct tx_cfm_tag *cfm)
{
    struct aml_txq *txq = sw_txhdr->txq;
    int retry = cfm->status.retry_required;

    /* MORE_DATA will be re-set if needed when pkt will be repushed */
    sw_txhdr->desc.api.host.flags &= ~TXU_CNTRL_MORE_DATA;

    if (retry) {
        // Firmware already tried to send the buffer but cannot retry it now
        // On next push, firmware needs to re-use the same SN
        sw_txhdr->desc.api.host.flags |= TXU_CNTRL_REUSE_SN;
        sw_txhdr->desc.api.host.sn_for_retry = cfm->status.sn;

        if (aml_bus_type != PCIE_MODE)
            AML_INFO("reuse sn = %d\n", cfm->status.sn);
    }

    // Add it to the linked list first, and then start sending the package
    // The credit of the retry package will not be used by the new skb
    aml_txq_queue_skb(aml_hw, txq, skb, true, NULL);

    txq->credits += cfm->credits;
    if (txq->credits > 0)
        aml_txq_start(txq, AML_TXQ_STOP_FULL);
    trace_skb_retry(skb, txq, retry ? cfm->status.sn : IEEE80211_SN_MODULO);
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
 * return 0 on success, -1 otherwise
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
                                    struct aml_sta *sta, struct aml_txq *txq, bool sp_frame)
{
    bool res = false;
    struct ethhdr *eth;

    /* Adjust the maximum number of MSDU allowed in A-MSDU */
    aml_adjust_amsdu_maxnb(aml_hw);

#ifdef CONFIG_AML_USB_LARGE_PAGE
    if (aml_bus_type == USB_MODE) {
        if (aml_hw->mod_params->amsdu_maxnb > 3)
            aml_hw->mod_params->amsdu_maxnb = 3; // USB limits the number of AMSDU aggregations, which can be sent in one BUF
        //AML_INFO("%s txq->id:%d, txq->amsdu_len:%d\n", __func__, txq->idx, txq->amsdu_len);
    }
#endif
    /* immediately return if amsdu are not allowed for this sta */
    if (!txq->amsdu_len || aml_hw->mod_params->amsdu_maxnb < 2 ||
        !aml_amsdu_is_aggregable(skb)
       )
        return false;

    AML_PROF_CNT(amsdu, 2);
    spin_lock_bh(&aml_hw->tx_lock);
    if (txq->amsdu) {
        /* aggregation already in progress, add this buffer if enough space
           available, otherwise end the current amsdu */
        struct aml_sw_txhdr *sw_txhdr = txq->amsdu;
        eth = (struct ethhdr *)(skb->data);

        //mac addr dismatch not asmdu
        if (memcmp(eth->h_dest, sw_txhdr->desc.api.host.eth_dest_addr.array, 6) || \
            memcmp(eth->h_source, sw_txhdr->desc.api.host.eth_src_addr.array, 6)) {
            txq->amsdu = NULL;
            goto end;
        }

        if (sp_frame || (sw_txhdr->desc.api.host.flags & TXU_CNTRL_SP_FRAME) ||
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
            (sw_txhdr->desc.api.host.flags & TXU_CNTRL_RETRY) || (sw_txhdr->desc.api.host.flags & TXU_CNTRL_SP_FRAME) || sp_frame)
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
    AML_PROF_CNT(amsdu, res ? 1 : 0);
    return res;
}

/**
 * aml_amsdu_dismantle - Dismantle an already formatted A-MSDU
 *
 * @aml_hw Driver main data
 * @sw_txhdr_main Software descriptor of the A-MSDU to dismantle.
 *
 * The a-mdsu is always fully dismantled (i.e don't try to reduce it is size to
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
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG|HW_LAST_AGG_FLAG|
                                        HW_MPDU_LEN_SET(sw_txhdr->frame_len);
        } else {
            txhdr = (struct aml_txhdr *)skb_push(skb, AML_TX_HEADROOM);
            txhdr->sw_hdr = sw_txhdr;
        }

        aml_txq_queue_skb(aml_hw, sw_txhdr->txq, skb, false, skb_prev);
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
            txq->amsdu_len = min(txq->amsdu_len, (u16)USB_AMSDU_BUF_LEN);
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

        aml_txq_stop(txq, AML_TXQ_STOP_DISMANTLE_AMSDU);
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
            txq->amsdu_len = min(txq->amsdu_len, (u16)USB_AMSDU_BUF_LEN);
#endif
        aml_txq_start(txq, AML_TXQ_STOP_DISMANTLE_AMSDU);
    }
}
#endif /* CONFIG_AML_AMSDUS_TX */

/*
 * aml_filter_sp_dns - get the dns name from skb
 * in:
 * @buf: the skb buf
 * @buf_len: the skb length
 * @offset: the offset of the start queries
 * out:
 * @qname: the decompress name
 *
 * return: 0 indicates success, while others indicate failure
 *
 * notes: qname: pre-allocated 256-byte buffer
 */
int aml_filter_sp_dns(const uint8_t *buf, uint32_t buf_len, int offset, char *qname)
{
    const uint8_t *p = NULL;
    uint16_t location = 0;
    uint32_t ptr_count = 0;
    uint32_t label_len = 0;
    uint32_t total_len = 0;

    if (!buf || !qname || buf_len < 1 || offset < 0 || offset > buf_len - 1)
        return -1;

    p = buf + offset;
    if (*p == 0)
        return -1;

    memset(qname, 0, MDNS_QNAME_LENGTH_MAX);
    while (*p) {
        if ((*p & 0xC0) == 0xC0) {
            if (ptr_count++ > 10)
                return -1;
            location = ((*p << 8) | *(p + 1)) & 0x3fff;
            if (location > (buf_len - 1))
                return -1;
            p = buf + location;
            continue;
        }

        label_len = *p++;
        if (label_len > MDNS_NAME_LABEL_LEN_MAX
          || total_len + label_len + 1 > MDNS_QNAME_LENGTH_MAX
          || p + label_len > buf + buf_len)
            return -1;

        memcpy(qname + total_len, p, label_len);
        p += label_len;
        total_len += label_len;
        qname[total_len++] = '.';
    }

    if (total_len > 0)
        qname[total_len-1] = '\0';
    else
        qname[0] = '\0';

    return 0;
}

uint8_t *get_dhcp_option(struct dhcp_packet *packet, int len, uint8_t option_code, uint8_t *length)
{
    uint8_t *options = packet->options;
    uint8_t *end = options + (len - sizeof(packet->options));
    uint8_t opt_len;

    while (options < end) {
        uint8_t code = *options++;
        if (code == DHCP_OPTION_END)
            break;
        if (code == DHCP_OPTION_PAD)
            continue;

        opt_len = *options++;
        if (code == option_code) {
            if (length)
                *length = opt_len;
            return options;
        }
        options += opt_len;
    }

    return NULL;
}

//
const char *get_dhcp_message_type_name(int message_type)
{
    switch (message_type) {
        case DHCP_DISCOVER:
            return "DISCOVER";
        case DHCP_OFFER:
            return "OFFER";
        case DHCP_REQUEST:
            return "REQUEST";
        case DHCP_DECLINE:
            return "DECLINE";
        case DHCP_ACK:
            return "ACK";
        case DHCP_NAK:
            return "NAK";
        case DHCP_RELEASE:
            return "RELEASE";
        case DHCP_INFORM:
            return "INFORM";
        default:
            return "UNKNOWN";
    }
}

void aml_filter_sp_data_dhcp_frame(const u8 *frame, int len, u32 pkt_types,
    struct aml_vif *vif, AML_SP_STATUS_E sp_status)
{
    char dhcp_print[200] = {0};
    int offset = 0;
    uint8_t length = 0;

    if (len < sizeof(struct dhcp_packet)) {
        AML_M_ERR(TX, "dhcp length error len:%d", len);
        return;
    }

    offset += snprintf(dhcp_print + offset, sizeof(dhcp_print) - offset, "%s[%d] DHCP ",
                       sp_frame_status_trace[sp_status], vif->vif_index);

    if (pkt_types & BIT(AML_PKT_IP)) {
        struct dhcp_packet *dhcp = (struct dhcp_packet *)frame;
        uint8_t *option_pos;
        uint8_t option_type;

        offset += snprintf(dhcp_print + offset, sizeof(dhcp_print) - offset, "id:%x %s ",
            dhcp->xid, (dhcp->op == 1) ? "req" : (dhcp->op == 2) ? "rsp" : "nul");

        option_pos = get_dhcp_option(dhcp, len, DHCP_OPTION_MESSAGE_TYPE, &length);
        if (option_pos && length == 1) {
            option_type = *option_pos;
            offset += snprintf(dhcp_print + offset, sizeof(dhcp_print) - offset, "message type:%s, ip:%pI4",
                               get_dhcp_message_type_name(option_type), &dhcp->yiaddr);

        } else {
            AML_M_INFO(TX, "%p, %d\n", option_pos, length);
            offset += snprintf(dhcp_print + offset, sizeof(dhcp_print) - offset, "message type err");
        }
    } else {
        offset += snprintf(dhcp_print + offset, sizeof(dhcp_print) - offset, "v6");
    }

    if (offset >= sizeof(dhcp_print)) {
        AML_M_ERR(TX, "length is not enough for snprintf\n");
    }

    AML_M_NOTICE(TX, "%s\n", dhcp_print);
}

u32 aml_filter_sp_data_frame(const u8 *frame, int len, struct aml_vif *aml_vif,
                             AML_SP_STATUS_E sp_status)
{
    static struct aml_vif vif_dummy = { .vif_index = 0x1f /* magic vif index */ };
    const struct ethhdr *ethhdr = NULL;
    const void *nethdr;
    u8 net_prot = 0;
    u32 pkt_types = 0;
    int offset = 0;
    const struct iphdr *iphdr = NULL;
    const struct ipv6hdr *ipv6hdr = NULL;

    if (!aml_vif) {
        aml_vif = &vif_dummy;
    }

    if (!frame)
        return pkt_types;

    ethhdr = (const struct ethhdr *)frame;


    //filter eapol
    if (ethhdr->h_proto == htons(ETH_P_PAE)) {
        AML_WARN("%s[%d] EAPOL\n", sp_frame_status_trace[sp_status], aml_vif->vif_index);
        return BIT(AML_PKT_EAPOL);
    }

    //filter arp
    if (ethhdr->h_proto == htons(ETH_P_ARP)) {
        const struct arphdr *arphdr = (const void *)(ethhdr + 1);
        const struct {  /* Ethernet ARP payload */
            unsigned char sha[ETH_ALEN]; /* sender hardware address  */
            unsigned char sip[4];        /* sender IP address        */
            unsigned char tha[ETH_ALEN]; /* target hardware address  */
            unsigned char tip[4];        /* target IP address        */
        } *ar = (const void *)(arphdr + 1);
        const u16 op = ntohs(arphdr->ar_op);
        const char *op_name = "???";
        pkt_types |= BIT(AML_PKT_ARP);

        switch (op) {
            case ARPOP_REQUEST:
                pkt_types |= BIT(AML_PKT_ARP_REQ);
                op_name = "req";
                break;

            case ARPOP_REPLY:
                pkt_types |= BIT(AML_PKT_ARP_REPLY);
                op_name = "rsp";
                break;

            default:
                op_name = "???";
                break;
        }

        if (aml_vif == &vif_dummy || sp_status != SP_STATUS_RX ||
                memcmp(ar->tip, &aml_vif->ipv4_addr, IPV4_ADDR_LEN) == 0) {
            AML_M_NOTICE(TX, "%s[%d] ARP %s(0x%x) sender:[%pM %pI4] receiver:[%pM %pI4]\n",
                     sp_frame_status_trace[sp_status], aml_vif->vif_index, op_name, op,
                     ar->sha, ar->sip, ar->tha, ar->tip);
        }

        return pkt_types;
    }

    //wfd rtsp frame
    if (aml_filter_rtsp_frame(aml_vif, len, frame, sp_status)) {
        return true;
    }

    //filter dhcp
    if (ethhdr->h_proto == htons(ETH_P_IPV6)) {
        ipv6hdr = (const struct ipv6hdr *)(ethhdr + 1);
        pkt_types |= BIT(AML_PKT_IPV6);
        net_prot = ipv6hdr->nexthdr;
        nethdr = (const void *)(ipv6hdr + 1);
        offset += sizeof(struct ipv6hdr);
    }
    else if (ethhdr->h_proto == htons(ETH_P_IP)) {
        iphdr = (const struct iphdr *)(ethhdr + 1);
        pkt_types |= BIT(AML_PKT_IP);
        net_prot = iphdr->protocol;
        nethdr = (const void *)iphdr + (iphdr->ihl << 2);
        offset += sizeof(struct iphdr);
    }
    else {
        return pkt_types;
    }

    switch (net_prot) {
        case IPPROTO_ICMP:
            pkt_types |= BIT(AML_PKT_ICMP);
            break;

        case IPPROTO_TCP:
            pkt_types |= BIT(AML_PKT_TCP);
            break;

        case IPPROTO_UDP: {
            const struct udphdr *udp = (const struct udphdr *)nethdr;
            u16 sport = ntohs(udp->source);
            u16 dport = ntohs(udp->dest);

            pkt_types |= BIT(AML_PKT_UDP);
            offset += sizeof(struct udphdr);

            if ((pkt_types & BIT(AML_PKT_IP)) && (sport == DHCP_SP_V4 || sport == DHCP_CP_V4)) {
                pkt_types |= BIT(AML_PKT_DHCP);
            }

            if ((pkt_types & BIT(AML_PKT_IPV6)) && (sport == DHCP_SP_V6 || sport == DHCP_CP_V6)) {
                pkt_types |= BIT(AML_PKT_DHCP_V6);
            }

            if ((sport == DNS_PORT) || (dport == DNS_PORT)) {
                int ret;
                struct mdns_pattern *dns = (struct mdns_pattern *)(udp + 1);
                char qname[MDNS_QNAME_LENGTH_MAX] = {0};

                ret = aml_filter_sp_dns((uint8_t *) dns, len - offset, sizeof(struct mdns_pattern), qname);
                if (ret == 0) {
                    AML_M_INFO(TX, "%s[%d] DNS:%s\n", sp_frame_status_trace[sp_status], aml_vif->vif_index, qname);
                }
            }

            if (pkt_types & (BIT(AML_PKT_DHCP) | BIT(AML_PKT_DHCP_V6))) {
                aml_filter_sp_data_dhcp_frame((u8 *)(udp + 1), len - offset, pkt_types, aml_vif, sp_status);
            }

            break;
        }

        default:
            break;
    }

    if (pkt_types & BIT(AML_PKT_ICMP)) {
        if (pkt_types & BIT(AML_PKT_IPV6))
            AML_M_INFO(TX, "%s[%d] IPV6 ICMP from:%pI6c to %pI6c\n", sp_frame_status_trace[sp_status],
                aml_vif->vif_index, &ipv6hdr->saddr, &ipv6hdr->daddr);
        else
            AML_M_INFO(TX, "%s[%d] IPV4 id:%d ICMP from:%pI4 to %pI4\n", sp_frame_status_trace[sp_status],
                aml_vif->vif_index, iphdr->id, &iphdr->saddr, &iphdr->daddr);
    }

    return pkt_types;
}

uint32_t aml_filter_sp_mgmt_frame(struct aml_vif *vif, u8 *buf, AML_SP_STATUS_E sp_status, u32 frame_len, u32 *len_diff, u64 cookie)
{
    u32 offset = 0;
    u32 subtype = ((*buf) & IEEE80211_FCTL_STYPE) >> 4;
    u8 str[256];
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

            if (category == PUBLIC_ACTION) {
                action_code = *(buf + ACTION_CODE_OFFSET);
                oui_type = *(buf + OUI_TYPE_OFFSET);
                oui_subtype = *(buf + OUI_SUBTYPE_OFFSET);

                if ((action_code == ACTION_CODE_VENDOR)
                        && (*(buf + OUI_OFFSET) == 0x50)
                        && (*(buf + OUI_OFFSET + 1) == 0x6f)
                        && (*(buf + OUI_OFFSET + 2) == 0x9a)) {
                    if (oui_type == OUI_TYPE_P2P) {
                        offset += sprintf(p + offset, "PUB ACT->%32s ", p2p_pub_action_trace_name(oui_subtype));

                        if (cookie) {
                            offset += sprintf(p + offset, " 0x%llx ", cookie);
                        }

                        ret |= AML_P2P_ACTION_FRAME;

                        //P2P_ACTION_GO_NEG_RSP & P2P_ACTION_GO_NEG_CFM & P2P_ACTION_INVIT_RSP:need sw retry
                        if ((oui_subtype == P2P_ACTION_GO_NEG_RSP)
                                || (oui_subtype == P2P_ACTION_GO_NEG_CFM)
                                || (oui_subtype == P2P_ACTION_INVIT_RSP)
                                || (oui_subtype == P2P_ACTION_INVIT_REQ)
                                || (oui_subtype == P2P_ACTION_GO_NEG_REQ)) {
                            ret |= AML_SP_FRAME;

                            if ((oui_subtype == P2P_ACTION_GO_NEG_CFM) || (oui_subtype == P2P_ACTION_INVIT_RSP)) {
                                ret |= AML_MUST_TX_SUC;
                            }
                        }

#ifdef DRV_P2P_SCC_MODE

                        if ((sp_status == SP_STATUS_TX_START) && (len_diff != NULL)) {
                            if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_INVIT_REQ)) {
                                AML_SCC_SET_P2P_PEER_5G_SUPPORT(false);    //rest 5g support flag
                            }

                            if ((oui_subtype == P2P_ACTION_GO_NEG_REQ) || (oui_subtype == P2P_ACTION_GO_NEG_RSP) || (oui_subtype == P2P_ACTION_INVIT_REQ)) {
                                struct aml_vif *sta_vif ;
                                sta_vif = vif->aml_hw->vif_table[0];

                                if (sta_vif && sta_vif->sta.ap && (sta_vif->sta.ap->valid)) {
                                    struct cfg80211_chan_def target_chdef;
                                    target_chdef = vif->aml_hw->chanctx_table[sta_vif->ch_index].chan_def;

                                    if ((target_chdef.chan->flags & IEEE80211_CHAN_RADAR)) {
                                        AML_INFO("dfs chan, skip change ie");
                                    }
                                    else {
                                        //AML_INFO("[P2P SCC] p2p channel to:%d", aml_ieee80211_freq_to_chan(target_chdef.chan->center_freq, target_chdef.chan->band));
                                        if (frame_len > MAX_P2P_SAVE_LEN) {
                                            AML_INFO("[P2P SCC] p2p frame len error:%d", frame_len);
                                        }
                                        else if (g_scc_p2p_len_before) {
                                            AML_INFO("[P2P SCC] p2p pre frame not cfm:%d", g_scc_p2p_len_before);
                                        }
                                        else {
                                            AML_SCC_SAVE_P2P_ACTION_FRAME(buf, frame_len);
                                            AML_SCC_SAVE_P2P_ACTION_LEN(frame_len);
                                            aml_change_p2p_chanlist(vif, buf, frame_len, len_diff, target_chdef);
                                            aml_change_p2p_operchan(vif, buf, frame_len, target_chdef);
                                            AML_SCC_SAVE_P2P_ACTION_LEN_DIFF(*len_diff);
                                        }
                                    }
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
                        offset += sprintf(p + offset, "DPP ACTION->%64s ", dpp_pub_action_trace_name(dpp_action_subtype));
                        ret |= AML_SP_FRAME | AML_DPP_ACTION_FRAME;
                        if (dpp_action_subtype == ACTION_DPP_CONNECT_STATUS_RESULT)
                            ret |= AML_DPP_CONNECT_STATUS_RESULT_FRAME;
                    }
                    else {
                        offset += sprintf(p + offset, "oui type:%d ", oui_type);
                    }
                }
                else if ((action_code == ACTION_GAS_INIT_REQ)
                         || (action_code == ACTION_GAS_INIT_RSP)) {
                    u8 tag_len = *(buf + CATEGORY_OFFSET + 4);
                    offset += sprintf(p + offset, "GAS ACTION,action code:%d ", action_code);
                    ret |= AML_SP_FRAME | AML_GAS_ACTION_FRAME;

                    if (action_code == ACTION_GAS_INIT_REQ) {
                        ret |= AML_GAS_INIT_REQ_FRAME;
                    }
                    else if (action_code == ACTION_GAS_INIT_RSP) {
                        ret |= AML_GAS_INIT_RSP_FRAME;
                    }

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
                    offset += sprintf(p + offset, "CSA ACTION:%d", action_code);
                }
                else {
                    offset += sprintf(p + offset, "action code:%d ", action_code);
                }

                p[offset] = '\0';
                AML_WARN("%s\n", p);
            }
            else if ((category == VENDOR_SPEC)
                     && (*(buf + CATEGORY_OFFSET + 1) == 0x50)
                     && (*(buf + CATEGORY_OFFSET + 2) == 0x6f)
                     && (*(buf + CATEGORY_OFFSET + 3) == 0x9a)) {
                oui_subtype = *(buf + CATEGORY_OFFSET + 5);
                offset += sprintf(p + offset, "P2P ACTION->%32s ", p2p_action_trace_name(oui_subtype));
                p[offset] = '\0';
                AML_INFO("%s\n", p);
            }

            return ret;
        }

        case AUTH_TYPE: {
            u16 auth_algo = *(buf + AUTH_ALGO_OFFSET);
            ret |= AML_SP_FRAME;
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            offset += sprintf(p + offset, "vif:%d, auth algo:%d ", vif->vif_index, auth_algo);
            p[offset] = '\0';
            AML_INFO("%s\n", p);

            if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) && (vif->sta.flags & AML_STA_EXT_AUTH)) {
                u8 *p = buf + AUTH_ALGO_OFFSET;
                vif->sta.auth_status = (*(p + 4) | *(p + 5) << 8);
                AML_INFO("auth status %d\n", vif->sta.auth_status);
            }

            return ret;
        }

        case ASSOC_RSP_TYPE: {
            offset += sprintf(p + offset, sp_frame_status_trace[sp_status]);
            offset += sprintf(p + offset, "vif:%d, ASSOC_RSP", vif->vif_index);
            p[offset] = '\0';
            AML_INFO("%s\n", p);
            return ret;
        }

        case PROBE_RSP_TYPE: {
            if (sp_status == SP_STATUS_TX_START) {
                aml_scc_save_probe_rsp(vif, (u8 *)buf, frame_len);
            }

            if ((vif->vif_index == AML_P2P_DEVICE_VIF_IDX) && (sp_status == SP_STATUS_TX_START)) {
                if (aml_get_p2p_ie_offset(buf, frame_len, MAC_SHORT_MAC_HDR_LEN + PROBE_RSP_HDR_LEN)) {
                    vif->aml_hw->wfd_present = true;
                }
            }

            return ret;
        }

        default:
            break;
    }

    return ret;
}

/**
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *	Called when a packet needs to be transmitted.
 *	Must return NETDEV_TX_OK , NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED if NETIF_F_LLTX)
 *
 *  - Initialize the descriptor for this pkt (stored in skb before data)
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
    u32 sp_frame = 0;

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

    /* FIXME: Forrest */
    if (skb_has_frag_list(skb) && !skb_is_nonlinear(skb)) {
        kfree_skb_list(skb_shinfo(skb)->frag_list);
        skb_shinfo(skb)->frag_list = NULL;
        AML_WARN("kernel issue??? %d [%64ph]\n", skb->len, skb->data);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
    skb_orphan_partial(skb);
#endif

    // If buffer is shared (or may be used by another interface) need to make a
    // copy as TX information is stored inside buffer's headroom
    if (skb_shared(skb) || (skb_headroom(skb) < tx_max_headroom) ||
        (skb_cloned(skb) && (dev->priv_flags & IFF_BRIDGE_PORT))) {
        struct sk_buff *newskb = skb_copy_expand(skb, tx_max_headroom, 0, GFP_ATOMIC);
        if (unlikely(newskb == NULL))
            goto free;

        dev_kfree_skb_any(skb);
        skb = newskb;
    }

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
    sp_frame = aml_filter_sp_data_frame(skb->data, skb->len, aml_vif, SP_STATUS_TX_START)
                      & AML_PKT_SP_TX;
    if (sp_frame & BIT(AML_PKT_ARP)) {
        // Solving the compatibility problem between w2 softap and mtk
        if (aml_vif->wdev.iftype == NL80211_IFTYPE_AP)
            sp_frame = 0;
    } else if (sp_frame & BIT(AML_PKT_ICMP)) {
        extern bool pt_mode;

        if (pt_mode)
            sp_frame = 0;   /* skip if product test mode */
    }

    txq = aml_txq_sta_get(sta, tid, aml_hw);

    if (txq->idx == TXQ_INACTIVE)
        goto free;


#ifdef CONFIG_AML_AMSDUS_TX
    if (aml_amsdu_add_subframe(aml_hw, skb, sta, txq, sp_frame))
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
        (aml_vif->is_re_sending)) {
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
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG|HW_LAST_AGG_FLAG|
                                        HW_MPDU_LEN_SET(sw_txhdr->frame_len);
        }

    }
    if (aml_bus_type != PCIE_MODE) {
        AML_RLMT_DBG("ethertype:0x%04x, credits:%d, tid:%d, vif_idx:%d\n",
                cpu_to_be16(desc->host.ethertype), txq->credits, desc->host.tid, desc->host.vif_idx);
    }

    /* queue the buffer */
    AML_PROF_CNT(xmit, skb->len);
    spin_lock_bh(&aml_hw->tx_lock);

    if (txq->idx == TXQ_INACTIVE ) {
        AML_INFO(" Get txq idx is inactive after spin_lock_bh  \n");
       //"do not push and process it with kernel list lib it whill be re-pull out and used this freed buf"
       spin_unlock_bh(&aml_hw->tx_lock);
       goto free;
    }

    aml_txq_queue_skb(aml_hw, txq, skb, false, NULL);
    spin_unlock_bh(&aml_hw->tx_lock);
    AML_PROF_CNT(xmit, 0);

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
 * @sta: destination of the frame. May be NULL if the destination is unknown
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
    unsigned int frame_len;
    u8 *data;
    struct aml_txq *txq;
    bool robust;
    struct aml_sdio_txhdr *sdio_txhdr;
    u32 tx_headroom;
    struct aml_usb_txhdr *usb_txhdr;
    u32 len_diff = 0;
    u32 sp_mgmt_ret;

#ifdef CONFIG_AML_RECOVERY
    if (((aml_recy != NULL) && aml_recy_flags_chk(AML_RECY_IPC_ONGOING | AML_RECY_DROP_XMIT_PKT))
        || (aml_hw->state != WIFI_SUSPEND_STATE_NONE)) {
        return -ENOMEM;
    }
#endif

    if (!params || !params->buf)
        return -EINVAL;
    frame_len = params->len;

    /* Set TID and Queues indexes */
    if (sta) {
        txq = aml_txq_sta_get(sta, 8, aml_hw);
    } else {
        if (offchan) {
            txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
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
    skb = __dev_alloc_skb(tx_headroom + frame_len, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
    if (!skb)
        return -ENOMEM;
    *cookie = (unsigned long)skb;

    sw_txhdr = kmem_cache_zalloc(aml_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL)) {
        dev_kfree_skb(skb);
        return -ENOMEM;
    }

    /* Reserve headroom in skb. Do this so that we can easily re-use ieee80211
       functions that take skb with 802.11 frame as parameter */
    skb_reserve(skb, tx_headroom);
    skb_reset_mac_header(skb);

    /* Copy data in skb buffer */
    data = skb_put(skb, frame_len);
    memcpy(data, params->buf, frame_len);
    sp_mgmt_ret = aml_filter_sp_mgmt_frame(vif, data, SP_STATUS_TX_START, frame_len, &len_diff, (u64)(unsigned long)skb);
    if ((sp_mgmt_ret & AML_DPP_ACTION_FRAME) && offchan) {
        txq = &aml_hw->txq[NX_OFF_CHAN_TXQ_IDX];
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
        struct ieee80211_mgmt *mgmt = (void *)params->buf;
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
        dev_kfree_skb(skb);
        if (ieee80211_is_action(mgmt->frame_control)) {
            AML_INFO("report action tx status as ipc error");
#ifdef DRV_P2P_SCC_MODE
            if (sp_mgmt_ret & AML_P2P_ACTION_FRAME)
                g_scc_p2p_len_before = 0;
#endif
            cfg80211_mgmt_tx_status(&(vif->wdev), *cookie, params->buf, params->len, 0, GFP_ATOMIC);
            /* coverity[leaked_storage] - variable "sw_txhdr" was free in kmem_cache_free */
            return 0;
        }
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
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG | HW_LAST_MPDUBUF_FLAG |
                                        HW_MPDU_LEN_SET(sw_txhdr->frame_len);
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
#ifdef DRV_P2P_SCC_MODE
            if (sp_mgmt_ret & AML_P2P_ACTION_FRAME)
                g_scc_p2p_len_before = 0;
#endif
            cfg80211_mgmt_tx_status(&(vif->wdev), *cookie, params->buf, params->len, 0, GFP_ATOMIC);
            return 0;
        }
        return -EBUSY;
    }
    aml_txq_queue_skb(aml_hw, txq, skb, false, NULL);
    spin_unlock_bh(&aml_hw->tx_lock);

    return 0;
}

int aml_mpdu_page_num(struct txdesc_host *txdesc_host, int *len)
{
    int i;
    int frame_len = 0;

    for (i = 0; i < txdesc_host->api.host.packet_cnt; i++)
        frame_len += txdesc_host->api.host.packet_len[i];

    if (len)
        *len = frame_len;

    if (aml_bus_type == USB_MODE)
#ifdef CONFIG_AML_USB_LARGE_PAGE
        return 1;
#else
        return txdesc_host->api.host.packet_cnt;
#endif
    return howmanypage(frame_len + SDIO_DATA_OFFSET, SDIO_PAGE_LEN);;
}

static inline void aml_sdio_usb_tx_cfm(struct aml_hw *aml_hw,
                                       struct compact_tx_cfm_tag *compact,
                                       struct sk_buff *skb,
                                       int headroom)
{
    struct aml_sw_txhdr *sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;
    int page_num = aml_mpdu_page_num(&sw_txhdr->desc, NULL);
    struct tx_cfm_tag cfm = {};

    /*
     * txu_cntrl_cfm() sets cfm.credits to 1 by default.
     * or patch_bam_check_tx_baw() sets it to 0 if TX failed,
     * or bam_move_baw() sets it to to packet number that's confirmed.
     *
     * for SDIO/USB, since each TX confirmation is linked to a skb (mpdu), that's 1 credit.
     */
    cfm.credits      = 1;
    cfm.ampdu_size   = compact->ampdu_size;
    cfm.amsdu_size   = compact->amsdu_size;
    cfm.status.value = compact->status;
    cfm.hostid       = compact->hostid;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    aml_hw->g_tx_param.tx_page_free_num += page_num;
    AML_RLMT_DBG("tx_page_free_num=%d, credit=%d, page num=%d, skb=%p, idx=%d\n",
                 aml_hw->g_tx_param.tx_page_free_num, sw_txhdr->txq->credits,
                 page_num, skb, aml_hw->ipc_env->txcfm_idx);
    if (aml_hw->g_tx_param.tx_page_free_num >= aml_hw->g_tx_param.txcfm_trigger_tx_thr)
        up(&aml_hw->aml_tx_sem);
    spin_unlock_bh(&aml_hw->tx_buf_lock);

    aml_tx_cfm(aml_hw, &cfm, skb, headroom);
}

int aml_task_fn_tx_cfm(struct aml_task *t)
{
    struct aml_hw *aml_hw = container_of(t, struct aml_hw, cfm_task);
    struct ipc_host_env_tag *ipc_env = aml_hw->ipc_env;
    unsigned cur = ipc_env->txcfm_idx;
    int headroom = (aml_bus_type == SDIO_MODE) ? AML_SDIO_TX_HEADROOM : AML_USB_TX_HEADROOM;
    unsigned i = 0;

    AML_PROF_HI(tx_cfm_task);
    spin_lock_bh(&aml_hw->tx_lock);

    for (i = 0; i < COMPACT_TXCFM_CNT; i++, cur = (cur + 1) % COMPACT_TXCFM_CNT) {
        struct compact_tx_cfm_tag *compact = &aml_hw->read_cfm[cur];
        struct sk_buff *skb;

        ipc_env->txcfm_idx = cur;
        skb = ipc_host_tx_host_id_to_ptr_for_sdio_usb(ipc_env, compact->hostid);
        if (!skb)
            break;

        aml_sdio_usb_tx_cfm(aml_hw, compact, skb, headroom);

        if (timer_pending(&aml_hw->txq_cleanup))
            mod_timer(&aml_hw->txq_cleanup, jiffies + AML_TXQ_CLEANUP_INTERVAL);
    }

    spin_unlock_bh(&aml_hw->tx_lock);
    AML_PROF_LO(tx_cfm_task);
    return 0;
}

/**
 * aml_txdatacfm - FW callback for TX confirmation
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct aml_hw is this case)
 * @arg: IPC buffer with the TX confirmation
 *
 * This function is called for each confirmation of transmission by the fw.
 * Called with tx_lock hold
 *
 */
int aml_tx_cfm(struct aml_hw *aml_hw, struct tx_cfm_tag *cfm, struct sk_buff *skb, int headroom)
{
    struct aml_sw_txhdr *sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;
    struct aml_txq *txq = sw_txhdr->txq;
    /* don't use txq->hwq as it may have changed between push and confirm */
    struct aml_hwq *hwq = &aml_hw->hwq[sw_txhdr->hw_queue];
    uint32_t sp_ret = 0;

    aml_txq_confirm_any(aml_hw, txq, hwq, sw_txhdr);

    /* Update txq and HW queue credits */
    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_MGMT) {
        struct ieee80211_mgmt *mgmt;
        bool cfm_tx_status = true;

        trace_mgmt_cfm(sw_txhdr->aml_vif->vif_index,
                       (sw_txhdr->aml_sta) ? sw_txhdr->aml_sta->sta_idx : 0xFF,
                       cfm->status.acknowledged);
        mgmt = (struct ieee80211_mgmt *)(skb->data + headroom);
        if ((ieee80211_is_deauth(mgmt->frame_control)) && (sw_txhdr->aml_vif->is_disconnect == 1)) {
            sw_txhdr->aml_vif->is_disconnect = 0;
        }
        if (ieee80211_is_action(mgmt->frame_control)) {
            u32 len_diff = sw_txhdr->frame_len;

            sp_ret = aml_filter_sp_mgmt_frame(sw_txhdr->aml_vif, (u8*)mgmt,
                (cfm->status.acknowledged ? SP_STATUS_TX_SUC : SP_STATUS_TX_FAIL), 0, &len_diff, (u64)(unsigned long)skb);
            sw_txhdr->frame_len = len_diff;
            if (sp_ret & AML_CSA_ACTION_FRAME) {
                AML_INFO("csa action send cfm, status:%d", cfm->status.acknowledged);
            }
        }

        if (!cfm->status.acknowledged
            && ((sp_ret & AML_GAS_ACTION_FRAME) || (sp_ret & AML_MUST_TX_SUC)
            || (sp_ret & AML_DPP_CONNECT_STATUS_RESULT_FRAME))
            && (txq->idx != TXQ_INACTIVE)) {
            spin_lock_bh(&aml_hw->roc_lock);
            if (aml_hw->roc && (jiffies_to_msecs(jiffies - aml_hw->roc->start_time) <= aml_hw->roc->duration)) {
                spin_unlock_bh(&aml_hw->roc_lock);
                AML_INFO("retry frame during roc:0x%x", sp_ret);
                aml_tx_retry(aml_hw, skb, sw_txhdr, cfm);
                return 0;
            }
            spin_unlock_bh(&aml_hw->roc_lock);
        }

        if (cfm->status.acknowledged && (sp_ret & AML_GAS_INIT_REQ_FRAME) && (sw_txhdr->aml_vif->vif_index != AML_STA_VIF_IDX)) {
            sw_txhdr->aml_vif->tx_cfm_wait.skb = skb_copy(skb, GFP_ATOMIC);
            if (sw_txhdr->aml_vif->tx_cfm_wait.skb) {
                sw_txhdr->aml_vif->tx_cfm_wait.cookie = (u64)(unsigned long)skb;
                sw_txhdr->aml_vif->tx_cfm_wait.len = sw_txhdr->frame_len;
                sw_txhdr->aml_vif->tx_cfm_wait.wdev = &sw_txhdr->aml_vif->wdev;
                cfm_tx_status = false;
                AML_INFO("gas init frame tx cfm delay, wait for rsp:%llx", sw_txhdr->aml_vif->tx_cfm_wait.cookie);
            }
        }

        if (cfm_tx_status) {
            /* Confirm transmission to CFG80211 */
            cfg80211_mgmt_tx_status(&sw_txhdr->aml_vif->wdev,
                                    (unsigned long)skb, skb_mac_header(skb),
                                    sw_txhdr->frame_len,
                                    (sp_ret & AML_REPORT_NO_ACKED) ? 0 : cfm->status.acknowledged,
                                    GFP_ATOMIC);
        }
    } else if ((txq->idx != TXQ_INACTIVE) && cfm->status.sw_retry_required) {
        sw_txhdr->desc.api.host.flags |= TXU_CNTRL_RETRY;
        /* firmware postponed this buffer */
        aml_tx_retry(aml_hw, skb, sw_txhdr, cfm);
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
            aml_txq_start(txq, AML_TXQ_STOP_FULL);
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
    consume_skb(skb);

    return 0;
}

int aml_txdatacfm(void *pthis, void *arg)
{
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *ipc_cfm = arg;
    struct tx_cfm_tag *cfm = ipc_cfm->addr;
    struct sk_buff *skb = aml_ipc_get_skb_from_cfm(aml_hw, ipc_cfm);

    BUG_ON(aml_bus_type != PCIE_MODE);

    if (!skb)
        return -1;

    return aml_tx_cfm(aml_hw, cfm, skb, AML_TX_HEADROOM);
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

void aml_tx_cfm_wait_rsp(struct aml_hw *aml_hw, bool ack, const char *func, u32 line)
{
    struct aml_vif *vif;
    struct aml_roc *roc = aml_hw->roc;
    if (!roc)
        return;

    spin_lock_bh(&aml_hw->tx_wait_cfm_lock);
    vif = roc->vif;

    if (vif->tx_cfm_wait.cookie != 0) {
        AML_INFO("ack:%d, [%s %d], cookie:%llx", ack, func, line, vif->tx_cfm_wait.cookie);

        cfg80211_mgmt_tx_status(vif->tx_cfm_wait.wdev,
                            vif->tx_cfm_wait.cookie, skb_mac_header(vif->tx_cfm_wait.skb),
                            vif->tx_cfm_wait.len,
                            ack,
                            GFP_ATOMIC);

        vif->tx_cfm_wait.cookie = 0;
        consume_skb(vif->tx_cfm_wait.skb);
    }
    spin_unlock_bh(&aml_hw->tx_wait_cfm_lock);
}

