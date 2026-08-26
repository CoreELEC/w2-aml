/**
 ******************************************************************************
 *
 * @file aml_rx.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define  AML_MODULE    RX

#include <linux/dma-mapping.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>
#include <net/ieee80211_radiotap.h>

#include "aml_recy.h"
#include "aml_wq.h"
#include "aml_defs.h"
#include "aml_main.h"
#include "aml_rx.h"
#include "aml_rate.h"
#include "aml_tx.h"
#include "aml_prof.h"
#include "ipc_host.h"
#include "aml_utils.h"
#include "aml_events.h"
#include "aml_compat.h"
#include "share_mem_map.h"
#include "reg_ipc_app.h"
#include "wifi_top_addr.h"
#include "aml_prealloc.h"

struct vendor_radiotap_hdr {
    u8 oui[3];
    u8 subns;
    u16 len;
    u8 data[];
};

/**
 * aml_rx_get_vif - Return pointer to the destination vif
 *
 * @aml_hw: main driver data
 * @vif_idx: vif index present in rx descriptor
 *
 * Select the vif that should receive this frame. Returns NULL if the destination
 * vif is not active or vif is not specified in the descriptor.
 */
struct aml_vif *aml_rx_get_vif(struct aml_hw *aml_hw, int vif_idx)
{
    struct aml_vif *aml_vif = NULL;

    if (vif_idx < NX_VIRT_DEV_MAX) {
        aml_vif = aml_hw->vif_table[vif_idx];
        if (!aml_vif || !aml_vif->up)
            return NULL;
    }

    return aml_vif;
}

/**
 * aml_rx_defer_skb - Defer processing of a SKB
 *
 * @aml_hw: main driver data
 * @aml_vif: vif that received the buffer
 * @skb: buffer to defer
 */
void aml_rx_defer_skb(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                       struct sk_buff *skb)
{
    struct aml_defer_rx_cb *rx_cb = (struct aml_defer_rx_cb *)skb->cb;

    // for now don't support deferring the same buffer on several interfaces
    if (skb_shared(skb))
        return;

    // Increase ref count to avoid freeing the buffer until it is processed
    skb_get(skb);

    rx_cb->vif = aml_vif;
    skb_queue_tail(&aml_hw->defer_rx.sk_list, skb);
    schedule_work(&aml_hw->defer_rx.work);
}

/**
 * aml_rx_data_skb - Process one data frame
 *
 * @aml_hw: main driver data
 * @aml_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 * @return Number of buffer processed (can only be > 1 for A-MSDU)
 *
 * If buffer is an A-MSDU, then each subframe is added in a list of skb
 * (and A-MSDU header is converted to ethernet header)
 * Then each skb may be:
 * - forwarded to upper layer
 * - resent on wireless interface
 *
 * When vif is a STA interface, every skb is only forwarded to upper layer.
 * When vif is an AP interface, multicast skb are forwarded and resent, whereas
 * skb for other BSS's STA are only resent.
 *
 * Whether it has been forwarded and/or resent the skb is always consumed
 * and as such it shall no longer be used after calling this function.
 */
static int aml_rx_data_skb(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                           struct sk_buff *skb,  struct hw_rxhdr *rxhdr)
{
    struct sk_buff_head list;
    struct sk_buff *rx_skb;
    bool amsdu = rxhdr->flags_is_amsdu;
    bool resend = false, forward = true;
    const struct ethhdr *eth = NULL;
    int skip_after_eth_hdr = 0;
    int res = 1;

    __skb_queue_head_init(&list);

    BUG_ON(aml_bus_type != PCIE_MODE);
    {
        if (amsdu) {
            u32 mpdu_len = le32_to_cpu(rxhdr->hwvect.len);

            /* the longer AMSDU (>2304) is already de-aggregated by f/w */
            if (mpdu_len > NORMAL_AMSDU_MAX_LEN) {
                int count = 0;
                u32 hostid;
                if (!rxhdr->amsdu_len[0] || (rxhdr->amsdu_len[0] > skb_tailroom(skb))) {
                    forward = false;
                } else {
                    skb_put(skb, rxhdr->amsdu_len[0]);
                }
                __skb_queue_tail(&list, skb);

                while ((count < ARRAY_SIZE(rxhdr->amsdu_hostids)) &&
                    (hostid = rxhdr->amsdu_hostids[count++])) {
                    struct aml_ipc_buf *ipc_buf = aml_ipc_rxbuf_from_hostid(aml_hw, hostid);

                    if (!ipc_buf) {
                        wiphy_err(aml_hw->wiphy, "Invalid hostid 0x%x for A-MSDU subframe\n",
                                  hostid);
                        break;
                    }
                    rx_skb = ipc_buf->addr;
                    // Index for amsdu_len is different (+1) than the one for amsdu_hostids
                    if (!rxhdr->amsdu_len[count] || (rxhdr->amsdu_len[count] > skb_tailroom(rx_skb))) {
                        forward = false;
                    } else {
                        skb_put(rx_skb, rxhdr->amsdu_len[count]);
                    }
                    rx_skb->priority = skb->priority;
                    rx_skb->dev = skb->dev;
                    __skb_queue_tail(&list, rx_skb);
                    aml_ipc_buf_e2a_release(aml_hw, ipc_buf);
                    res++;
                }

                aml_hw->stats->amsdus_rx[count - 1]++;
                if (!forward) {
                    wiphy_err(aml_hw->wiphy, "A-MSDU truncated, skip it\n");
                    goto resend_n_forward;
                }
            } else {
                int count;

                skb_put(skb, le32_to_cpu(rxhdr->hwvect.len));
                aml_amsdu_to_8023s(skb, &list, aml_vif->ndev->dev_addr,
                                   AML_VIF_TYPE(aml_vif), 0);
                count = skb_queue_len(&list);
                if (count > ARRAY_SIZE(aml_hw->stats->amsdus_rx))
                    count = ARRAY_SIZE(aml_hw->stats->amsdus_rx);
                if (count > 0)
                    aml_hw->stats->amsdus_rx[count - 1]++;
            }
        } else {
            u32 frm_len = le32_to_cpu(rxhdr->hwvect.len);

            __skb_queue_tail(&list, skb);
            aml_hw->stats->amsdus_rx[0]++;
            aml_filter_sp_data_frame(skb->data, skb->len, aml_vif, SP_STATUS_RX);

            if (frm_len > skb_tailroom(skb)) {
                wiphy_err(aml_hw->wiphy, "A-MSDU truncated, skip it\n");
                forward = false;
                goto resend_n_forward;
            }
            skb_put(skb, le32_to_cpu(rxhdr->hwvect.len));
        }
    }

    rx_skb = skb_peek(&list);
    if (rx_skb == NULL)
        return 0;

    skb_reset_mac_header(rx_skb);
    eth = eth_hdr(rx_skb);

    if (((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) ||
         (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN) ||
         (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO)) &&
        !(aml_vif->ap.flags & AML_AP_ISOLATE)) {
        if (eth && unlikely(is_multicast_ether_addr(eth->h_dest))) {
            /* broadcast pkt need to be forwarded to upper layer and resent
               on wireless interface */
            resend = true;
        } else if (rxhdr->flags_dst_idx != AML_STA_ID_UNKNOWN){
            struct aml_sta *sta = aml_sta_get(aml_hw, rxhdr->flags_dst_idx);

            /* unicast pkt for STA inside the BSS, no need to forward to upper
               layer simply resend on wireless interface */
            if (sta && sta->vlan_idx == aml_vif->vif_index) {
                forward = false;
                resend = true;
            }
        }
    } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MESH_POINT) {
        if (rxhdr->flags_dst_idx != AML_STA_ID_UNKNOWN)
        {
            resend = true;

            if (eth && is_multicast_ether_addr(eth->h_dest)) {
                // MC/BC frames are uploaded with mesh control and LLC/snap
                // (so they can be mesh forwarded) that need to be removed.
                uint8_t *mesh_ctrl = (uint8_t *)(eth + 1);
                skip_after_eth_hdr = 8 + 6;

                if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A4)
                    skip_after_eth_hdr += ETH_ALEN;
                else if ((*mesh_ctrl & MESH_FLAGS_AE) == MESH_FLAGS_AE_A5_A6)
                    skip_after_eth_hdr += 2 * ETH_ALEN;
            } else {
                forward = false;
            }
        }
    }

resend_n_forward:
    /* forward it and/or resend it */
    while ((rx_skb = __skb_dequeue(&list))) {
        /* resend pkt on wireless interface */
        if (resend) {
            struct sk_buff *skb_copy;
            /* always need to copy buffer even when forward=0 to get enough headroom for txdesc */
            /* TODO: check if still necessary when forward=0 */
            skb_copy = skb_copy_expand(rx_skb, AML_TX_MAX_HEADROOM, 0, GFP_ATOMIC);
            if (skb_copy) {
                int res;
                skb_copy->protocol = htons(ETH_P_802_3);
                skb_reset_network_header(skb_copy);
                skb_reset_mac_header(skb_copy);

                aml_vif->is_re_sending = true;
                res = dev_queue_xmit(skb_copy);
                aml_vif->is_re_sending = false;
                /* note: buffer is always consumed by dev_queue_xmit */
                if (res == NET_XMIT_DROP) {
                    aml_vif->net_stats.rx_dropped++;
                    aml_vif->net_stats.tx_dropped++;
                } else if (res != NET_XMIT_SUCCESS) {
                    netdev_err(aml_vif->ndev,
                               "Failed to re-send buffer to driver (res=%d)",
                               res);
                    aml_vif->net_stats.tx_errors++;
                }
            } else {
                netdev_err(aml_vif->ndev, "Failed to copy skb");
            }
        }

        /* forward pkt to upper layer */
        if (forward) {
            rx_skb->protocol = eth_type_trans(rx_skb, aml_vif->ndev);
            memset(rx_skb->cb, 0, sizeof(rx_skb->cb));

            // Special case for MESH when BC/MC is uploaded and resend
            if (unlikely(skip_after_eth_hdr)) {
                memmove(skb_mac_header(rx_skb) + skip_after_eth_hdr,
                        skb_mac_header(rx_skb), sizeof(struct ethhdr));
                __skb_pull(rx_skb, skip_after_eth_hdr);
                skb_reset_mac_header(rx_skb);
                skip_after_eth_hdr = 0;
            }
            /* Update statistics */
            aml_vif->net_stats.rx_packets++;
            aml_vif->net_stats.rx_bytes += rx_skb->len;
#ifdef CONFIG_AML_NAPI
            if (aml_hw->napi_enable) {
                __skb_queue_tail(&aml_hw->napi_rx_pending_queue, rx_skb);
               /*if rx pending pkts >= napi_pend_pkt_num,extract to napi_rx_upload_queue,and schedule napi for poll*/
               /*if rx pending pkts < napi_pend_pkt_num,schedule napi in ipc_host_rxdesc_handler*/
                if (skb_queue_len(&aml_hw->napi_rx_pending_queue) >= aml_hw->napi_pend_pkt_num) {
                    unsigned long flags;
                    spin_lock_irqsave(&aml_hw->napi_rx_upload_queue.lock, flags);
                    skb_queue_splice_tail_init(&aml_hw->napi_rx_pending_queue, &aml_hw->napi_rx_upload_queue);
                    spin_unlock_irqrestore(&aml_hw->napi_rx_upload_queue.lock, flags);
                    napi_schedule(&aml_hw->napi);
                }
                continue;
            }
#endif
            AML_PROF_CNT(rx, rx_skb->len);
            netif_receive_skb(rx_skb);
            AML_PROF_CNT(rx, 0);
        } else {
            dev_kfree_skb(rx_skb);
        }
    }

    return res;
}

static void aml_rx_assoc_req(struct aml_hw *aml_hw, struct sk_buff *skb)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
    const u8 *ht_cap_ie;
    int var_offset;

    if (ieee80211_is_assoc_req(mgmt->frame_control)) {
        var_offset = offsetof(struct ieee80211_mgmt, u.assoc_req.variable);
        ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, mgmt->u.assoc_req.variable, skb->len - var_offset);
    }
    else {
        var_offset = offsetof(struct ieee80211_mgmt, u.reassoc_req.variable);
        ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, mgmt->u.reassoc_req.variable, skb->len - var_offset);
    }

    memcpy(aml_hw->rx_assoc_info.addr, mgmt->sa, ETH_ALEN);

    if (ht_cap_ie) {
        const struct ieee80211_ht_cap *ht_cap = (void *)(ht_cap_ie + 2);
        aml_hw->rx_assoc_info.htcap = ht_cap->cap_info;
    }
    else
        aml_hw->rx_assoc_info.htcap = 0;
}

static int aml_wq_scan_cancel(struct aml_hw *aml_hw, void *data)
{
    struct aml_vif *vif = data;

    spin_lock_bh(&aml_hw->scan_req_lock);
    if (aml_hw->scan_request) {
        int error;
        spin_unlock_bh(&aml_hw->scan_req_lock);
        AML_INFO("action rx cancel scan, vif:%d\n",vif->vif_index);
        error = aml_cancel_scan(aml_hw, vif);
        if (error) {
            AML_INFO("cancel scan fail:error = %d\n",error);
        }
        aml_set_scan_hang(vif, 0, __func__, __LINE__);
    } else {
        spin_unlock_bh(&aml_hw->scan_req_lock);
    }
    return 0;
}

/**
 * aml_rx_mgmt - Process one 802.11 management/control frame
 *
 * @aml_hw: main driver data
 * @aml_vif: vif to upload the buffer to
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 *
 * Forward the management frame to a given interface.
 */
static void aml_rx_mgmt(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                         struct sk_buff *skb,  struct hw_rxhdr *hw_rxhdr)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
    struct rx_vector_1 *rxvect = &hw_rxhdr->hwvect.rx_vect1;
    uint32_t sp_ret = 0;

    sp_ret = aml_filter_sp_mgmt_frame(aml_vif, skb->data, SP_STATUS_RX, skb->len, NULL, 0);

    if (sp_ret & AML_GAS_INIT_RSP_FRAME) {
        aml_tx_cfm_wait_rsp(aml_hw, true, (u8 *)__func__, __LINE__);
    }

    if (((sp_ret & AML_GAS_ACTION_FRAME) && (aml_vif->vif_index != AML_STA_VIF_IDX))
        || (sp_ret & AML_P2P_ACTION_FRAME)) {
        spin_lock_bh(&aml_hw->scan_req_lock);
        if (aml_hw->scan_request) {
            spin_unlock_bh(&aml_hw->scan_req_lock);
            aml_wq_do_ptr(aml_wq_scan_cancel, aml_hw, aml_vif);
        } else {
            spin_unlock_bh(&aml_hw->scan_req_lock);
        }
    }

    if (ieee80211_is_beacon(mgmt->frame_control)) {
        if ((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MESH_POINT) &&
            hw_rxhdr->flags_new_peer) {
            cfg80211_notify_new_peer_candidate(aml_vif->ndev, mgmt->sa,
                                               mgmt->u.beacon.variable,
                                               skb->len - offsetof(struct ieee80211_mgmt,
                                                                   u.beacon.variable),
                                               rxvect->rssi1, GFP_ATOMIC);
        } else {
            cfg80211_report_obss_beacon(aml_hw->wiphy, skb->data, skb->len,
                                        hw_rxhdr->phy_info.phy_prim20_freq,
                                        rxvect->rssi1);
        }
    } else if ((ieee80211_is_deauth(mgmt->frame_control) ||
                ieee80211_is_disassoc(mgmt->frame_control)) &&
               (mgmt->u.deauth.reason_code == WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
                mgmt->u.deauth.reason_code == WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA)) {
        cfg80211_rx_unprot_mlme_mgmt(aml_vif->ndev, skb->data, skb->len);
    } else if ((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_action(mgmt->frame_control) &&
                (mgmt->u.action.category == 6))) {
        // Wpa_supplicant will ignore the FT action frame if reported via cfg80211_rx_mgmt
        // and cannot call cfg80211_ft_event from atomic context so defer message processing
        aml_rx_defer_skb(aml_hw, aml_vif, skb);
    } else {
        if (ieee80211_is_assoc_req(mgmt->frame_control) || ieee80211_is_reassoc_req(mgmt->frame_control)) {
            aml_rx_assoc_req(aml_hw, skb);
        }
#ifdef DRV_P2P_SCC_MODE
        else if (ieee80211_is_public_action((struct ieee80211_hdr *)mgmt, skb->len)) {
            /* coverity[TAINTED_SCALAR] */
            aml_rx_parse_p2p_chan_list(skb->data, skb->len);
        }
#endif
        cfg80211_rx_mgmt(&aml_vif->wdev, hw_rxhdr->phy_info.phy_prim20_freq,
                         rxvect->rssi1, skb->data, skb->len, 0);
    }
}

/**
 * aml_rx_mgmt_any - Process one 802.11 management frame
 *
 * @aml_hw: main driver data
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 *
 * Process the management frame and free the corresponding skb.
 * If vif is not specified in the rx descriptor, the frame is uploaded
 * on all active vifs.
 */
void aml_rx_mgmt_any(struct aml_hw *aml_hw, struct sk_buff *skb, struct hw_rxhdr *hw_rxhdr)
{
    struct aml_vif *aml_vif;
    int vif_idx = hw_rxhdr->flags_vif_idx;

    trace_mgmt_rx(hw_rxhdr->phy_info.phy_prim20_freq, vif_idx,
                  hw_rxhdr->flags_sta_idx, (struct ieee80211_mgmt *)skb->data);

    if (vif_idx == AML_INVALID_VIF) {
        list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
            if (! aml_vif->up)
                continue;
            /* coverity[TAINTED_SCALAR] */
            aml_rx_mgmt(aml_hw, aml_vif, skb, hw_rxhdr);
        }
    } else {
        aml_vif = aml_rx_get_vif(aml_hw, vif_idx);
        if (aml_vif)
            aml_rx_mgmt(aml_hw, aml_vif, skb, hw_rxhdr);
    }

    dev_kfree_skb(skb);
}

/**
 * aml_rx_rtap_hdrlen - Return radiotap header length
 *
 * @rxvect: Rx vector used to fill the radiotap header
 * @has_vend_rtap: boolean indicating if vendor specific data is present
 *
 * Compute the length of the radiotap header based on @rxvect and vendor
 * specific data (if any).
 */
static u8 aml_rx_rtap_hdrlen(struct rx_vector_1 *rxvect,
                              bool has_vend_rtap)
{
    u8 rtap_len;
    u8 ant_num = hweight8(rxvect->antenna_set);

    /* Compute radiotap header length */
    rtap_len = sizeof(struct ieee80211_radiotap_header) + 8;

    // Check for multiple antennas
    if (ant_num > 1)
        // antenna and antenna signal fields
        rtap_len += 4 * ant_num;

    // TSFT
    if (!has_vend_rtap) {
        rtap_len = ALIGN(rtap_len, 8);
        rtap_len += 8;
    }

    // IEEE80211_HW_SIGNAL_DBM
    rtap_len++;

    // Check if single antenna
    if (ant_num == 1)
        rtap_len++; //Single antenna

    // padding for RX FLAGS
    rtap_len = ALIGN(rtap_len, 2);

    // Check for HT frames
    if ((rxvect->format_mod == FORMATMOD_HT_MF) ||
        (rxvect->format_mod == FORMATMOD_HT_GF))
        rtap_len += 3;

    // Check for AMPDU
    if (!(has_vend_rtap) && ((rxvect->format_mod >= FORMATMOD_VHT) ||
                             ((rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) &&
                                                     (rxvect->ht.aggregation)))) {
        rtap_len = ALIGN(rtap_len, 4);
        rtap_len += 8;
    }

    // Check for VHT frames
    if (rxvect->format_mod == FORMATMOD_VHT) {
        rtap_len = ALIGN(rtap_len, 2);
        rtap_len += 12;
    }

    // Check for HE frames
    if (rxvect->format_mod == FORMATMOD_HE_SU) {
        rtap_len = ALIGN(rtap_len, 2);
        rtap_len += sizeof(struct ieee80211_radiotap_he);
    }

    // Check for multiple antennas
    if (ant_num > 1) {
        // antenna and antenna signal fields
        rtap_len += 2 * ant_num;
    }

    // Check for vendor specific data
    if (has_vend_rtap) {
        /* vendor presence bitmap */
        rtap_len += 4;
        /* alignment for fixed 6-byte vendor data header */
        rtap_len = ALIGN(rtap_len, 2);
    }

    return rtap_len;
}

/**
 * aml_rx_add_rtap_hdr - Add radiotap header to sk_buff
 *
 * @aml_hw: main driver data
 * @skb: skb received (will include the radiotap header)
 * @rxvect: Rx vector
 * @phy_info: Information regarding the phy
 * @hwvect: HW Info (NULL if vendor specific data is available)
 * @rtap_len: Length of the radiotap header
 * @vend_rtap_len: radiotap vendor length (0 if not present)
 * @vend_it_present: radiotap vendor present
 *
 * Builds a radiotap header and add it to @skb.
 */
static void aml_rx_add_rtap_hdr(struct aml_hw* aml_hw,
                                 struct sk_buff *skb,
                                 struct rx_vector_1 *rxvect,
                                 struct phy_channel_info_desc *phy_info,
                                 struct hw_vect *hwvect,
                                 int rtap_len,
                                 u8 vend_rtap_len,
                                 u32 vend_it_present)
{
    struct ieee80211_radiotap_header *rtap;
    u8 *pos, rate_idx;
    void *it_present;
    u32 it_present_val = 0;
    bool fec_coding = false;
    bool short_gi = false;
    bool stbc = false;
    bool aggregation = false;
    u8 ant_num = hweight8(rxvect->antenna_set);

    rtap = (struct ieee80211_radiotap_header *)skb_push(skb, rtap_len);
    memset((u8*) rtap, 0, rtap_len);

    rtap->it_version = 0;
    rtap->it_pad = 0;
    rtap->it_len = cpu_to_le16(rtap_len + vend_rtap_len);

    it_present = &rtap->it_present;

    if (phy_info->phy_band >= NUM_NL80211_BANDS)
        return;

    // Check for multiple antennas
    if (ant_num > 1) {
        int chain;
        unsigned long chains = rxvect->antenna_set;

        for_each_set_bit(chain, &chains, IEEE80211_MAX_CHAINS) {
            it_present_val |=
                BIT(IEEE80211_RADIOTAP_EXT) |
                BIT(IEEE80211_RADIOTAP_RADIOTAP_NAMESPACE);
            put_unaligned_le32(it_present_val, it_present);
            it_present += sizeof(__le32);
            it_present_val = BIT(IEEE80211_RADIOTAP_ANTENNA) |
                             BIT(IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
        }
    }

    // Check if vendor specific data is present
    if (vend_rtap_len) {
        it_present_val |= BIT(IEEE80211_RADIOTAP_VENDOR_NAMESPACE) |
                          BIT(IEEE80211_RADIOTAP_EXT);
        put_unaligned_le32(it_present_val, it_present);
        it_present += sizeof(__le32);
        it_present_val = vend_it_present;
    }

    /* coverity[overrun-local] - will not overrunning it_present */
    put_unaligned_le32(it_present_val, it_present);
    pos = it_present + sizeof(__le32);

    // IEEE80211_RADIOTAP_TSFT
    if (hwvect) {
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_TSFT);
        // padding
        while ((pos - (u8 *)rtap) & 7)
            *pos++ = 0;
        put_unaligned_le64((((u64)le32_to_cpu(hwvect->tsf_hi) << 32) +
                            (u64)le32_to_cpu(hwvect->tsf_lo)), pos);
        pos += 8;
    }

    // IEEE80211_RADIOTAP_FLAGS
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_FLAGS);
    if (hwvect && (!hwvect->status.frm_successful_rx))
        *pos |= IEEE80211_RADIOTAP_F_BADFCS;
    if (!rxvect->pre_type
            && (rxvect->format_mod <= FORMATMOD_NON_HT_DUP_OFDM))
        *pos |= IEEE80211_RADIOTAP_F_SHORTPRE;
    pos++;

    // IEEE80211_RADIOTAP_RATE
    // check for HT, VHT or HE frames
    if (rxvect->format_mod >= FORMATMOD_HE_SU) {
        rate_idx = rxvect->he.mcs;
        fec_coding = rxvect->he.fec;
        stbc = rxvect->he.stbc;
        aggregation = true;
        *pos = 0;
    } else if (rxvect->format_mod == FORMATMOD_VHT) {
        rate_idx = rxvect->vht.mcs;
        fec_coding = rxvect->vht.fec;
        short_gi = rxvect->vht.short_gi;
        stbc = rxvect->vht.stbc;
        aggregation = true;
        *pos = 0;
    } else if (rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) {
        rate_idx = rxvect->ht.mcs;
        fec_coding = rxvect->ht.fec;
        short_gi = rxvect->ht.short_gi;
        stbc = rxvect->ht.stbc;
        aggregation = rxvect->ht.aggregation;
        *pos = 0;
    } else {
        struct ieee80211_supported_band* band =
                aml_hw->wiphy->bands[phy_info->phy_band];
        s16 legrates_idx = legrates_lut[rxvect->leg_rate & 0xf].idx;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_RATE);
        BUG_ON(legrates_idx == -1);
        rate_idx = legrates_idx;
        if (phy_info->phy_band == NL80211_BAND_5GHZ)
            rate_idx -= 4;  /* aml_ratetable_5ghz[0].hw_value == 4 */
        *pos = DIV_ROUND_UP(band->bitrates[rate_idx].bitrate, 5);
    }
    pos++;

    // IEEE80211_RADIOTAP_CHANNEL
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_CHANNEL);
    put_unaligned_le16(phy_info->phy_prim20_freq, pos);
    pos += 2;

    if (phy_info->phy_band == NL80211_BAND_5GHZ)
        put_unaligned_le16(IEEE80211_CHAN_OFDM | IEEE80211_CHAN_5GHZ, pos);
    else if (rxvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM)
        put_unaligned_le16(IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ, pos);
    else
        put_unaligned_le16(IEEE80211_CHAN_CCK | IEEE80211_CHAN_2GHZ, pos);
    pos += 2;

    if (ant_num == 1) {
        // IEEE80211_RADIOTAP_DBM_ANTSIGNAL
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
        *pos++ = rxvect->rssi1;

        // IEEE80211_RADIOTAP_ANTENNA
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_ANTENNA);
        *pos++ = rxvect->antenna_set;
    }

    // IEEE80211_RADIOTAP_LOCK_QUALITY is missing
    // IEEE80211_RADIOTAP_DB_ANTNOISE is missing

    // IEEE80211_RADIOTAP_RX_FLAGS
    rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_RX_FLAGS);
    // 2 byte alignment
    if ((pos - (u8 *)rtap) & 1)
        *pos++ = 0;
    put_unaligned_le16(0, pos);
    //Right now, we only support fcs error (no RX_FLAG_FAILED_PLCP_CRC)
    pos += 2;

    // Check if HT
    if ((rxvect->format_mod == FORMATMOD_HT_MF) ||
        (rxvect->format_mod == FORMATMOD_HT_GF)) {
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_MCS);
        *pos++ = (IEEE80211_RADIOTAP_MCS_HAVE_MCS |
                  IEEE80211_RADIOTAP_MCS_HAVE_GI |
                  IEEE80211_RADIOTAP_MCS_HAVE_BW |
                  IEEE80211_RADIOTAP_MCS_HAVE_FMT |
                  IEEE80211_RADIOTAP_MCS_HAVE_FEC |
                  IEEE80211_RADIOTAP_MCS_HAVE_STBC);

        pos++;
        *pos = 0;
        if (short_gi)
            *pos |= IEEE80211_RADIOTAP_MCS_SGI;
        if (rxvect->ch_bw  == PHY_CHNL_BW_40)
            *pos |= IEEE80211_RADIOTAP_MCS_BW_40;
        if (rxvect->format_mod == FORMATMOD_HT_GF)
            *pos |= IEEE80211_RADIOTAP_MCS_FMT_GF;
        if (fec_coding)
            *pos |= IEEE80211_RADIOTAP_MCS_FEC_LDPC;
        *pos++ |= stbc << IEEE80211_RADIOTAP_MCS_STBC_SHIFT;
        *pos++ = rate_idx;
    }

    // check for HT or VHT frames
    if (aggregation && hwvect) {
        // 4 byte alignment
        while ((pos - (u8 *)rtap) & 3)
            pos++;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_AMPDU_STATUS);
        put_unaligned_le32(hwvect->ampdu_cnt, pos);
        pos += 4;
        put_unaligned_le32(0, pos);
        pos += 4;
    }

    // Check for VHT frames
    if (rxvect->format_mod == FORMATMOD_VHT) {
        u16 vht_details = IEEE80211_RADIOTAP_VHT_KNOWN_GI |
                          IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
        u8 vht_nss = rxvect->vht.nss + 1;

        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_VHT);

        if ((rxvect->ch_bw == PHY_CHNL_BW_160)
                && phy_info->phy_center2_freq)
            vht_details &= ~IEEE80211_RADIOTAP_VHT_KNOWN_BANDWIDTH;
        put_unaligned_le16(vht_details, pos);
        pos += 2;

        // flags
        if (short_gi)
            *pos |= IEEE80211_RADIOTAP_VHT_FLAG_SGI;
        if (stbc)
            *pos |= IEEE80211_RADIOTAP_VHT_FLAG_STBC;
        pos++;

        // bandwidth
        if (rxvect->ch_bw == PHY_CHNL_BW_40)
            *pos++ = 1;
        if (rxvect->ch_bw == PHY_CHNL_BW_80)
            *pos++ = 4;
        else if ((rxvect->ch_bw == PHY_CHNL_BW_160)
                && phy_info->phy_center2_freq)
            *pos++ = 0; //80P80
        else if  (rxvect->ch_bw == PHY_CHNL_BW_160)
            *pos++ = 11;
        else // 20 MHz
            *pos++ = 0;

        // MCS/NSS
        *pos++ = (rate_idx << 4) | vht_nss;
        *pos++ = 0;
        *pos++ = 0;
        *pos++ = 0;
        if (fec_coding)
            *pos |= IEEE80211_RADIOTAP_CODING_LDPC_USER0;
        pos++;
        // group ID
        pos++;
        // partial_aid
        pos += 2;
    }

    // Check for HE frames
    if (rxvect->format_mod >= FORMATMOD_HE_SU) {
        struct ieee80211_radiotap_he he = {0};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
        #define HE_PREP(f, val) cpu_to_le16(FIELD_PREP(IEEE80211_RADIOTAP_HE_##f, val))
#else
        #define HE_PREP(f, val) le16_encode_bits(val, IEEE80211_RADIOTAP_HE_##f)
#endif
        #define D1_KNOWN(f) cpu_to_le16(IEEE80211_RADIOTAP_HE_DATA1_##f##_KNOWN)
        #define D2_KNOWN(f) cpu_to_le16(IEEE80211_RADIOTAP_HE_DATA2_##f##_KNOWN)

        he.data1 = D1_KNOWN(BSS_COLOR) | D1_KNOWN(BEAM_CHANGE) |
                   D1_KNOWN(UL_DL) | D1_KNOWN(STBC) |
                   D1_KNOWN(DOPPLER) | D1_KNOWN(DATA_DCM);
        he.data2 = D2_KNOWN(GI) | D2_KNOWN(TXBF) | D2_KNOWN(TXOP);

        he.data3 |= HE_PREP(DATA3_BSS_COLOR, rxvect->he.bss_color);
        he.data3 |= HE_PREP(DATA3_BEAM_CHANGE, rxvect->he.beam_change);
        he.data3 |= HE_PREP(DATA3_UL_DL, rxvect->he.uplink_flag);
        he.data3 |= HE_PREP(DATA3_BSS_COLOR, rxvect->he.bss_color);
        he.data3 |= HE_PREP(DATA3_DATA_DCM, rxvect->he.dcm);

        he.data5 |= HE_PREP(DATA5_GI, rxvect->he.gi_type);
        he.data5 |= HE_PREP(DATA5_TXBF, rxvect->he.beamformed);
        he.data5 |= HE_PREP(DATA5_LTF_SIZE, rxvect->he.he_ltf_type + 1);

        he.data6 |= HE_PREP(DATA6_DOPPLER, rxvect->he.doppler);
        he.data6 |= HE_PREP(DATA6_TXOP, rxvect->he.txop_duration);

        if (rxvect->format_mod != FORMATMOD_HE_TB) {
            he.data1 |= (D1_KNOWN(DATA_MCS) | D1_KNOWN(CODING) |
                         D1_KNOWN(SPTL_REUSE) | D1_KNOWN(BW_RU_ALLOC));

            if (stbc) {
                he.data6 |= HE_PREP(DATA6_NSTS, 2);
                he.data3 |= HE_PREP(DATA3_STBC, 1);
            } else {
                he.data6 |= HE_PREP(DATA6_NSTS, rxvect->he.nss);
            }

            he.data3 |= HE_PREP(DATA3_DATA_MCS, rxvect->he.mcs);
            he.data3 |= HE_PREP(DATA3_CODING, rxvect->he.fec);

            he.data4 = HE_PREP(DATA4_SU_MU_SPTL_REUSE, rxvect->he.spatial_reuse);

            if (rxvect->format_mod == FORMATMOD_HE_MU) {
                he.data1 |= IEEE80211_RADIOTAP_HE_DATA1_FORMAT_MU;
                he.data5 |= HE_PREP(DATA5_DATA_BW_RU_ALLOC,
                                    rxvect->he.ru_size +
                                    IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_26T);
            } else {
                if (rxvect->format_mod == FORMATMOD_HE_SU)
                    he.data1 |= IEEE80211_RADIOTAP_HE_DATA1_FORMAT_SU;
                else
                    he.data1 |= IEEE80211_RADIOTAP_HE_DATA1_FORMAT_EXT_SU;

                switch (rxvect->ch_bw) {
                    case PHY_CHNL_BW_20:
                        he.data5 |= HE_PREP(DATA5_DATA_BW_RU_ALLOC,
                                            IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_20MHZ);
                        break;
                    case PHY_CHNL_BW_40:
                        he.data5 |= HE_PREP(DATA5_DATA_BW_RU_ALLOC,
                                            IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_40MHZ);
                        break;
                    case PHY_CHNL_BW_80:
                        he.data5 |= HE_PREP(DATA5_DATA_BW_RU_ALLOC,
                                            IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_80MHZ);
                        break;
                    case PHY_CHNL_BW_160:
                        he.data5 |= HE_PREP(DATA5_DATA_BW_RU_ALLOC,
                                            IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_160MHZ);
                        break;
                    default:
                        WARN_ONCE(1, "Invalid SU BW %d\n", rxvect->ch_bw);
                }
            }
        } else {
            he.data1 |= IEEE80211_RADIOTAP_HE_DATA1_FORMAT_TRIG;
        }

        /* ensure 2 bytes alignment */
        while ((pos - (u8 *)rtap) & 1)
            pos++;
        rtap->it_present |= cpu_to_le32(1 << IEEE80211_RADIOTAP_HE);
        // memcpy(pos, &he, sizeof(he));
        *(struct ieee80211_radiotap_he *)pos = he;
        pos += sizeof(he);
    }

    // Rx Chains
    if (ant_num > 1) {
        int chain;
        unsigned long chains = rxvect->antenna_set;
        u8 rssis[4] = {rxvect->rssi1, rxvect->rssi1, rxvect->rssi1, rxvect->rssi1};

        for_each_set_bit(chain, &chains, IEEE80211_MAX_CHAINS) {
            *pos++ = rssis[chain];
            *pos++ = chain;
        }
    }
}

#ifdef CONFIG_AML_MON_DATA
/**
 * aml_rx_dup_for_monitor - Duplicate RX skb for monitor path
 *
 * @aml_hw: main driver data
 * @rxhdr: RX header
 * @skb: RX buffer
 * @rtap_len: Length to reserve for radiotap header
 * @nb_buff: Updated with the number of skb processed (can only be >1 for A-MSDU)
 * @return 'duplicated' skb for monitor path and %NULL in case of error
 *
 * Use when RX buffer is forwarded to net layer and a monitor interface is active,
 * a 'copy' of the RX buffer is done for the monitor path.
 * This is not a simple copy as:
 * - Headroom is reserved for Radiotap header
 * - For MSDU, MAC header (included in RX header) is re-added in the buffer.
 * - A-MSDU subframes are gathered in a single buffer (adding A-MSDU and LLC/SNAP headers)
 */
static struct sk_buff * aml_rx_dup_for_monitor(struct aml_hw *aml_hw,
                                                struct sk_buff *skb,  struct hw_rxhdr *rxhdr,
                                                u8 rtap_len, int *nb_buff)
{
    struct sk_buff *skb_dup = NULL;
    u16 frm_len = le32_to_cpu(rxhdr->hwvect.len);
    int skb_count = 1;

    if (rxhdr->flags_is_80211_mpdu) {
        if (frm_len > skb_tailroom(skb))
            frm_len = skb_tailroom(skb);
        skb_put(skb, frm_len);
        skb_dup = skb_copy_expand(skb, rtap_len, 0, GFP_ATOMIC);
    } else {
        // For MSDU, need to re-add the MAC header
        u16 machdr_len = rxhdr->mac_hdr_backup.buf_len;
        u8* machdr_ptr = rxhdr->mac_hdr_backup.buffer;
        int tailroom = 0;
        int headroom = machdr_len + rtap_len;

        if (rxhdr->flags_is_amsdu) {
            int subfrm_len;
            subfrm_len = rxhdr->amsdu_len[0];
            // Set tailroom to store all subframes. frm_len is the size
            // of the A-MSDU as received by MACHW (i.e. with LLC/SNAP and padding)
            tailroom = frm_len - subfrm_len;
            if (tailroom < 0)
                goto end;
            headroom += sizeof(rfc1042_header) + 2;

            if (subfrm_len > skb_tailroom(skb))
                subfrm_len = skb_tailroom(skb);
            skb_put(skb, subfrm_len);

        } else {
            // Pull Ethernet header from skb
            if (frm_len > skb_tailroom(skb))
                frm_len = skb_tailroom(skb);
            skb_put(skb, frm_len);
            skb_pull(skb, sizeof(struct ethhdr));
        }

        // Copy skb and extend for adding the radiotap header and the MAC header
        skb_dup = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);
        if (!skb_dup)
            goto end;

        if (rxhdr->flags_is_amsdu) {
            // recopy subframes in a single buffer, and add SNAP/LLC if needed
            struct ethhdr *eth_hdr, *amsdu_hdr;
            int count = 0, padding;
            u32 hostid;

            eth_hdr = (struct ethhdr *)skb_dup->data;
            if (ntohs(eth_hdr->h_proto) >= 0x600) {
                // Re-add LLC/SNAP header
                int llc_len =  sizeof(rfc1042_header) + 2;
                amsdu_hdr = skb_push(skb_dup, llc_len);
                memmove(amsdu_hdr, eth_hdr, sizeof(*eth_hdr));
                amsdu_hdr->h_proto = htons(rxhdr->amsdu_len[0] +
                                           llc_len - sizeof(*eth_hdr));
                amsdu_hdr++;
                memcpy(amsdu_hdr, rfc1042_header, sizeof(rfc1042_header));
            }
            padding = AMSDU_PADDING(rxhdr->amsdu_len[0]);

            while ((count < ARRAY_SIZE(rxhdr->amsdu_hostids)) &&
                   (hostid = rxhdr->amsdu_hostids[count++])) {
                struct aml_ipc_buf *subfrm_ipc = aml_ipc_rxbuf_from_hostid(aml_hw, hostid);
                struct sk_buff *subfrm_skb;
                void *src;
                int subfrm_len, llc_len = 0, truncated = 0;

                if (!subfrm_ipc)
                    continue;

                // Cannot use e2a_release here as it will delete the pointer to the skb
                aml_ipc_buf_e2a_sync(aml_hw, subfrm_ipc, 0);

                subfrm_skb = subfrm_ipc->addr;
                eth_hdr = (struct ethhdr *)subfrm_skb->data;
                subfrm_len = rxhdr->amsdu_len[count];
                if (subfrm_len > skb_tailroom(subfrm_skb))
                    truncated = skb_tailroom(subfrm_skb) - subfrm_len;

                if (ntohs(eth_hdr->h_proto) >= 0x600)
                    llc_len = sizeof(rfc1042_header) + 2;

                if (skb_tailroom(skb_dup) < padding + subfrm_len + llc_len) {
                    dev_kfree_skb(skb_dup);
                    skb_dup = NULL;
                    goto end;
                }

                if (padding)
                    skb_put(skb_dup, padding);
                if (llc_len) {
                    int move_oft = offsetof(struct ethhdr, h_proto);
                    amsdu_hdr = skb_put(skb_dup, sizeof(struct ethhdr));
                    memcpy(amsdu_hdr, eth_hdr, move_oft);
                    amsdu_hdr->h_proto = htons(subfrm_len + llc_len
                                               - sizeof(struct ethhdr));
                    memcpy(skb_put(skb_dup, sizeof(rfc1042_header)),
                           rfc1042_header, sizeof(rfc1042_header));

                    src = &eth_hdr->h_proto;
                    subfrm_len -= move_oft;
                } else {
                    src = eth_hdr;
                }
                if (!truncated) {
                    memcpy(skb_put(skb_dup, subfrm_len), src, subfrm_len);
                } else {
                    memcpy(skb_put(skb_dup, subfrm_len - truncated), src, subfrm_len - truncated);
                    memset(skb_put(skb_dup, truncated), 0, truncated);
                }
                skb_count++;
            }
        }

        // Copy MAC Header in new headroom
        memcpy(skb_push(skb_dup, machdr_len), machdr_ptr, machdr_len);
    }

end:
    // Reset original state for skb
    skb->data = (void*) rxhdr;
    __skb_set_length(skb, 0);
    *nb_buff = skb_count;
    return skb_dup;
}
#endif // CONFIG_AML_MON_DATA

/**
 * aml_rx_monitor - Build radiotap header for skb and send it to netdev
 *
 * @aml_hw: main driver data
 * @aml_vif: vif that received the buffer
 * @skb: sk_buff received
 * @rxhdr: Pointer to HW RX header
 * @rtap_len: Radiotap Header length
 *
 * Add radiotap header to the received skb and send it to netdev
 */
static void aml_rx_monitor(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                            struct sk_buff *skb,  struct hw_rxhdr *rxhdr,
                            u8 rtap_len)
{
    skb->dev = aml_vif->ndev;

    /* Add RadioTap Header */
    aml_rx_add_rtap_hdr(aml_hw, skb, &rxhdr->hwvect.rx_vect1,
                         &rxhdr->phy_info, &rxhdr->hwvect,
                         rtap_len, 0, 0);

    skb_reset_mac_header(skb);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = htons(ETH_P_802_2);

    netif_receive_skb(skb);
}

/**
 * aml_unsup_rx_vec_ind() - IRQ handler callback for %IPC_IRQ_E2A_UNSUP_RX_VEC
 *
 * FMAC has triggered an IT saying that a rx vector of an unsupported frame has been
 * captured and sent to upper layer.
 * If no monitor interface is active ignore it, otherwise add a radiotap header with a
 * vendor specific header and upload it on the monitor interface.
 *
 * @pthis: Pointer to main driver data
 * @arg: Pointer to IPC buffer
 */
u8 aml_unsup_rx_vec_ind(void *pthis, void *arg) {
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *ipc_buf = arg;
    struct rx_vector_desc *rx_desc;
    struct sk_buff *skb;
    struct rx_vector_1 *rx_vect1;
    struct phy_channel_info_desc *phy_info;
    struct vendor_radiotap_hdr *rtap;
    u16 ht_length;
    struct aml_vif *aml_vif;
    struct rx_vector_desc rx_vect_desc;
    u8 rtap_len, vend_rtap_len = sizeof(*rtap);

    if (aml_bus_type != PCIE_MODE) {
        return -1;
    }

    aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, sizeof(struct rx_vector_desc));

    skb = ipc_buf->addr;
    if (((struct rx_vector_desc *)(skb->data))->pattern == 0) {
        aml_ipc_buf_e2a_sync_back(aml_hw, ipc_buf, sizeof(struct rx_vector_desc));
        return -1;
    }

    if (aml_hw->monitor_vif == AML_INVALID_VIF) {
        aml_ipc_unsuprxvec_repush(aml_hw, ipc_buf);
        return -1;
    }

    aml_vif = aml_hw->vif_table[aml_hw->monitor_vif];
    skb->dev = aml_vif->ndev;
    memcpy(&rx_vect_desc, skb->data, sizeof(rx_vect_desc));
    rx_desc = &rx_vect_desc;

    rx_vect1 = (struct rx_vector_1 *) (rx_desc->rx_vect1);
    aml_rx_vector_convert(aml_hw->machw_type, rx_vect1, NULL);
    phy_info = (struct phy_channel_info_desc *) (&rx_desc->phy_info);
    if (rx_vect1->format_mod >= FORMATMOD_VHT)
        ht_length = 0;
    else
        ht_length = (u16) le32_to_cpu(rx_vect1->ht.length);

    // Reserve space for radiotap
    skb_reserve(skb, RADIOTAP_HDR_MAX_LEN);

    /* Fill vendor specific header with fake values */
    rtap = (struct vendor_radiotap_hdr *) skb->data;
    rtap->oui[0] = 0x00;
    rtap->oui[1] = 0x25;
    rtap->oui[2] = 0x3A;
    rtap->subns  = 0;
    rtap->len = sizeof(ht_length);
    put_unaligned_le16(ht_length, rtap->data);
    vend_rtap_len += rtap->len;
    skb_put(skb, vend_rtap_len);

    /* Copy fake data */
    put_unaligned_le16(0, skb->data + vend_rtap_len);
    skb_put(skb, UNSUP_RX_VEC_DATA_LEN);

    /* Get RadioTap Header length */
    rtap_len = aml_rx_rtap_hdrlen(rx_vect1, true);

    /* Check headroom space */
    if (skb_headroom(skb) < rtap_len) {
        netdev_err(aml_vif->ndev, "not enough headroom %d need %d\n",
                   skb_headroom(skb), rtap_len);
        aml_ipc_unsuprxvec_repush(aml_hw, ipc_buf);
        return -1;
    }

    /* Add RadioTap Header */
    aml_rx_add_rtap_hdr(aml_hw, skb, rx_vect1, phy_info, NULL,
                         rtap_len, vend_rtap_len, BIT(0));

    skb_reset_mac_header(skb);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    skb->pkt_type = PACKET_OTHERHOST;
    skb->protocol = htons(ETH_P_802_2);

    aml_ipc_buf_e2a_release(aml_hw, ipc_buf);
    netif_receive_skb(skb);

    /* Allocate and push a new buffer to fw to replace this one */
    aml_ipc_unsuprxvec_alloc(aml_hw, ipc_buf);
    return 0;
}

/**
 * aml_rx_amsdu_free() - Free RX buffers used (if any) to upload A-MSDU subframes
 *
 * @aml_hw: Main driver data
 * @rxhdr: RX header for the buffer (Must have been synced)
 * @return: Number of A-MSDU subframes (including the first one), or 1 for non
 * A-MSDU frame
 *
 * If this RX header correspond to an A-MSDU then all the Rx buffer used to
 * upload subframes are freed.
 */
static int aml_rx_amsdu_free(struct aml_hw *aml_hw, struct hw_rxhdr *rxhdr)
{
    int count = 0;
    u32 hostid;
    int res = 1;

    if (!rxhdr->flags_is_amsdu)
        return res;

    while ((count < ARRAY_SIZE(rxhdr->amsdu_hostids)) &&
           (hostid = rxhdr->amsdu_hostids[count++])) {
        struct aml_ipc_buf *ipc_buf = aml_ipc_rxbuf_from_hostid(aml_hw, hostid);

        if (!ipc_buf)
            continue;
        aml_ipc_rxbuf_dealloc(aml_hw, ipc_buf);
        res++;
    }
    return res;
}

static uint8_t aml_scan_find_already_saved(struct aml_hw *aml_hw, struct sk_buff *skb)
{
    uint8_t ret = 0;
    struct scan_results *scan_res,*next;
    struct ieee80211_mgmt *cur_mgmt = (struct ieee80211_mgmt *)skb->data;

    spin_lock_bh(&aml_hw->scan_lock);
    list_for_each_entry_safe(scan_res, next, &aml_hw->scan_res_list, list) {
        struct sdio_scanu_result_ind *ind = &scan_res->scanu_res_ind;
        struct ieee80211_mgmt * saved_mgmt = (struct ieee80211_mgmt *)ind->payload;
        if (!memcmp(cur_mgmt->bssid, saved_mgmt->bssid, 6)) {
            ret = 1;
            break;
        }
    }
    spin_unlock_bh(&aml_hw->scan_lock);
    return ret;
}

void aml_scan_clear_scan_res(struct aml_hw *aml_hw)
{
    struct scan_results *scan_res,*next;

    spin_lock_bh(&aml_hw->scan_lock);
    list_for_each_entry_safe(scan_res, next, &aml_hw->scan_res_list, list) {
        list_del(&scan_res->list);
        list_add_tail(&scan_res->list, &aml_hw->scan_res_available_list);
    }
    spin_unlock_bh(&aml_hw->scan_lock);
}
void aml_scan_rx(struct aml_hw *aml_hw, struct hw_rxhdr *hw_rxhdr, struct sk_buff *skb)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;

    spin_lock_bh(&aml_hw->scan_req_lock);
    if (aml_hw->scan_request || aml_hw->sched_request) {
        spin_unlock_bh(&aml_hw->scan_req_lock);
        if (ieee80211_is_beacon(mgmt->frame_control) || ieee80211_is_probe_resp(mgmt->frame_control)) {
            struct scan_results *scan_res;
            uint8_t contain = 0;

#ifdef CONFIG_AML_RECOVERY
            if (aml_recy && aml_recy->link_loss.is_enabled
                    && aml_recy->link_loss.is_happened)
                aml_recy->link_loss.scan_result_cnt++;
#endif
            aml_hw->misc.scan_result_cnt++;
            contain = aml_scan_find_already_saved(aml_hw, skb);
            if (contain)
                return;

            scan_res = aml_scan_get_scan_res_node(aml_hw);
            if (scan_res == NULL)
                return;

            scan_res->scanu_res_ind.length = le32_to_cpu(hw_rxhdr->hwvect.len);
            scan_res->scanu_res_ind.framectrl = mgmt->frame_control;
            scan_res->scanu_res_ind.center_freq = hw_rxhdr->phy_info.phy_prim20_freq;
            scan_res->scanu_res_ind.band = hw_rxhdr->phy_info.phy_band;
            scan_res->scanu_res_ind.sta_idx = hw_rxhdr->flags_sta_idx;
            scan_res->scanu_res_ind.inst_nbr = hw_rxhdr->flags_vif_idx;
            scan_res->scanu_res_ind.rssi = hw_rxhdr->hwvect.rx_vect1.rssi1;
            scan_res->scanu_res_ind.rx_leg_inf.format_mod = hw_rxhdr->hwvect.rx_vect1.format_mod;
            scan_res->scanu_res_ind.rx_leg_inf.ch_bw      = hw_rxhdr->hwvect.rx_vect1.ch_bw;
            scan_res->scanu_res_ind.rx_leg_inf.pre_type   = hw_rxhdr->hwvect.rx_vect1.pre_type;
            scan_res->scanu_res_ind.rx_leg_inf.leg_length = hw_rxhdr->hwvect.rx_vect1.leg_length;
            scan_res->scanu_res_ind.rx_leg_inf.leg_rate   = hw_rxhdr->hwvect.rx_vect1.leg_rate;

            /*scanres payload process end*/
            if (aml_hw->scanres_payload_buf_offset + le32_to_cpu(hw_rxhdr->hwvect.len) > SCAN_RESULTS_MAX_CNT*500) {
                aml_hw->scanres_payload_buf_offset = 0;
                AML_INFO("scanres_payload_buf overflow, flush");
            }
            /*scanres payload process start*/
            memcpy(aml_hw->scanres_payload_buf + aml_hw->scanres_payload_buf_offset,
                skb->data, le32_to_cpu(hw_rxhdr->hwvect.len));
            scan_res->scanu_res_ind.payload = (u32_l *)(aml_hw->scanres_payload_buf + aml_hw->scanres_payload_buf_offset);

            aml_hw->scanres_payload_buf_offset += le32_to_cpu(hw_rxhdr->hwvect.len);
            spin_lock_bh(&aml_hw->scan_lock);
            list_add_tail(&scan_res->list, &aml_hw->scan_res_list);
            spin_unlock_bh(&aml_hw->scan_lock);
        }
    } else {
        spin_unlock_bh(&aml_hw->scan_req_lock);
    }
}

#define RXBUF_ENOUGH_FOR(_size, _pos, _end)     ((_pos) + (_size) <= (_end))

static inline uint32_t aml_rx_desc_get_next(struct rxdesc *rxdesc)
{
#ifdef CONFIG_AML_W2L_RX_MINISIZE
    return rxdesc->dma_hdrdesc.hd.new_read;
#elif defined(CONFIG_AML_W2_RX_MINISIZE)    /* W2 compressed RX descriptor */
#error "W2_RX_MINISIZE is unsupported!"
#else
    return rxdesc->new_read;
#endif
}

static inline int aml_sdio_usb_rx_desc_goto_next(struct rxbuf_list *rxbuf)
{
    u32 next_fw_pos = aml_rx_desc_get_next((struct rxdesc *)rxbuf->pos);

    /* next_fw_pos is rxdesc new_read addr, which is a sharemem rxdesc addr */
    if (next_fw_pos > rxbuf->rx_buf_end || next_fw_pos < RXBUF_START_ADDR) {
        pr_err("=======error:invalid address %08x, start:%08x, end:%08x, fw_pre_pos: %x, fw_new_pos: %x, fw_pos: %x\n",
            next_fw_pos, rxbuf->pos, rxbuf->end, rxbuf->rxbuf_data_start, rxbuf->rxbuf_data_end, rxbuf->fw_pos);
        rxbuf->pos = rxbuf->end;    /* drop the whole rxbuf */
        return -1;
    }

    if (!RXBUF_ENOUGH_FOR(RX_DESC_SIZE, next_fw_pos, rxbuf->rx_buf_end))
        next_fw_pos = RXBUF_START_ADDR;

    /* move to next */
    if (next_fw_pos > rxbuf->fw_pos)
        rxbuf->pos += next_fw_pos - rxbuf->fw_pos;
    else
        rxbuf->pos = rxbuf->gap + (next_fw_pos - RXBUF_START_ADDR);

    rxbuf->fw_pos = next_fw_pos;

    return 0;
}

/* get a frame (a mpdu(management), a msdu or an a-msdu) from rx buffer */
static inline struct sk_buff *aml_sdio_usb_rx_skb_from_rxbuf(struct aml_hw *aml_hw,
                                                             struct rxbuf_list *rxbuf)
{
    struct rx_hd *hd = &((struct rxdesc *)rxbuf->pos)->dma_hdrdesc.hd;
    struct hw_rxhdr *hw_rxhdr = (struct hw_rxhdr *)&hd->frmlen;
    u32 frame_len = hd->frmlen;
    u8 payl_offset = hd->payl_offset;
    u8 *pos = rxbuf->pos + RX_DESC_SIZE;
    int room = rxbuf->gap - pos;
    u32 payl_len2 = 0;
    struct sk_buff *skb;
    u8 *data;

    /* it's guaranteed by caller, never happen */
    BUG_ON(rxbuf->pos < rxbuf->gap && pos > rxbuf->gap);

    if (aml_sdio_usb_rx_desc_goto_next(rxbuf) < 0)
        return NULL;

    /* firmware resets rx_status, indicates to drop it. refer to rxl_mpdu_free() */
    if (hd->status == 0)
        return NULL;

    skb = dev_alloc_skb(sizeof(struct hw_rxhdr) + frame_len);
    if (!skb) {
        AML_INFO("failed to alloc skb! frame len: 0x%x\n", frame_len);
        return NULL;
    }

    /* put h/w RX header */
    data = skb_put(skb, sizeof(struct hw_rxhdr));
    memcpy(data, hw_rxhdr, sizeof(struct hw_rxhdr));

#if 0
    /* WAR: reset the fields overlapped with payload (44 bytes) */
#define OVERLAP_LEN ((int)(RX_HEADER_OFFSET + sizeof(struct hw_rxhdr) - RX_DESC_SIZE))
    if (OVERLAP_LEN > 0)
        memset(data + sizeof(struct hw_rxhdr) - OVERLAP_LEN, 0, OVERLAP_LEN);
#endif

    if (room <= 0 || room >= RX_PD_LEN + payl_offset + frame_len) {
        /* pos: rx_payloaddesc + payl_offset + payload */
        pos += RX_PD_LEN + payl_offset;
    } else if (room < RX_PD_LEN) {
        /* gap: rx_payloaddesc + payl_offset + payload */
        pos = rxbuf->gap + RX_PD_LEN + payl_offset;
    } else if (
#ifndef CONFIG_AML_W2L_RX_MINISIZE
            hw_rxhdr->flags_is_80211_mpdu ||
#endif
            room < RX_PD_LEN + UNWRAP_SIZE) {
        /*
         * pos: rx_payloaddesc
         * gap: payl_offset + payload
         */
        pos = rxbuf->gap + payl_offset;
    } else {
        /*
         * pos: rx_payloaddesc1 + payl_offset + payload1
         * gap: rx_payloaddesc2 + payload2
         */
        pos += RX_PD_LEN + payl_offset;
        payl_len2 = RX_PD_LEN + payl_offset + frame_len - room;
        frame_len -= payl_len2;
    }

    /* put payload */
    data = skb_put(skb, frame_len + payl_len2);
    memcpy(data, pos, frame_len);
    if (payl_len2)
        memcpy(data + frame_len, rxbuf->gap + RX_PD_LEN, payl_len2);

    /* copy external info if exists */
    if (payl_offset >= sizeof(struct aml_rhd_ext)) {
        struct aml_rhd_ext *ext = (struct aml_rhd_ext *)(pos - payl_offset);

        *AML_RHD_EXT(skb->data) = *ext;
    } else {
        *AML_RHD_EXT(skb->data) = (struct aml_rhd_ext){};
    }

    return skb;
}

static s8 aml_sdio_usb_rx_skb_handle(struct aml_hw *aml_hw,
                                     struct sk_buff *skb,
                                     enum rx_status_bits rx_status)
{
    struct hw_rxhdr *hw_rxhdr = (struct hw_rxhdr *)(skb->data);
    struct aml_vif *aml_vif;

    skb_pull(skb, sizeof(struct hw_rxhdr));

#ifndef CONFIG_AML_SDIO_USB_FW_REORDER
    if (rx_status == RX_STAT_ALLOC) {
        AML_INFO("SDIO/USB firmware reorder is enabled, but driver doesn't support!");
        dev_kfree_skb(skb);
        return 0;
    }
#endif

    if (!(rx_status & (RX_STAT_FORWARD | RX_STAT_SPURIOUS))) {
        AML_INFO("invalid rx status %x [%d]: %32ph\n", rx_status, skb->len, skb->data);
        dev_kfree_skb(skb);
        return 0;
    }

    spin_lock_bh(&aml_hw->rx_lock);

    /* Check if we need to forward the buffer coming from a monitor interface */
    if (rx_status & RX_STAT_MONITOR) {
        struct sk_buff *skb_monitor;
        struct hw_rxhdr hw_rxhdr_copy;
        u8 rtap_len;
        u16 frm_len;

       //Check if monitor interface exists and is open
        aml_vif = aml_rx_get_vif(aml_hw, aml_hw->monitor_vif);
        if (!aml_vif) {
            dev_err(aml_hw->dev, "Received monitor frame but there is no monitor interface open\n");
            goto check_len_update;
        }

        aml_rx_vector_convert(aml_hw->machw_type, &hw_rxhdr->hwvect.rx_vect1, &hw_rxhdr->hwvect.rx_vect2);
        rtap_len = aml_rx_rtap_hdrlen(&hw_rxhdr->hwvect.rx_vect1, false);

        if (rx_status == RX_STAT_MONITOR) {
        //aml_ipc_buf_e2a_release(aml_hw, ipc_buf);

        //Check if there is enough space to add the radiotap header
        if (skb_headroom(skb) > rtap_len) {

            skb_monitor = skb;
                //Duplicate the HW Rx Header to override with the radiotap header
            memcpy(&hw_rxhdr_copy, hw_rxhdr, sizeof(hw_rxhdr_copy));

            hw_rxhdr = &hw_rxhdr_copy;
        } else {
            //Duplicate the skb and extend the headroom
            skb_monitor = skb_copy_expand(skb, rtap_len, 0, GFP_ATOMIC);

            //Reset original skb->data pointer
            skb->data = (void*) hw_rxhdr;
        }
    } else {
        #ifdef CONFIG_AML_MON_DATA
        // Check if MSDU
        if (!hw_rxhdr->flags_is_80211_mpdu) {
        // MSDU
        //Extract MAC header
            u16 machdr_len = hw_rxhdr->mac_hdr_backup.buf_len;
            u8* machdr_ptr = hw_rxhdr->mac_hdr_backup.buffer;

            //Pull Ethernet header from skb
            skb_pull(skb, sizeof(struct ethhdr));

            // Copy skb and extend for adding the radiotap header and the MAC header
            skb_monitor = skb_copy_expand(skb, rtap_len + machdr_len, 0, GFP_ATOMIC);

            //Reserve space for the MAC Header
            skb_push(skb_monitor, machdr_len);

            //Copy MAC Header
            memcpy(skb_monitor->data, machdr_ptr, machdr_len);

            //Update frame length
            frm_len += machdr_len - sizeof(struct ethhdr);
        } else {
              // MPDU
            skb_monitor = skb_copy_expand(skb, rtap_len, 0, GFP_ATOMIC);
        }
        #else
        wiphy_err(aml_hw->wiphy, "RX status %d is invalid when MON_DATA is disabled\n", rx_status);
        goto check_len_update;
        #endif
    }

        skb_reset_tail_pointer(skb);
        skb->len = 0;
        if (skb_monitor != NULL) {
        skb_reset_tail_pointer(skb_monitor);
        skb_monitor->len = 0;
        //AML_INFO(" skb tail %x \n",skb->tail);
        skb_put(skb_monitor, frm_len);
#if 0  //TODO: SDIO not use ?

        /* coverity[remediation] -hwvect won't cross the line*/
        if (aml_rx_monitor(aml_hw, aml_vif, skb_monitor, hw_rxhdr, rtap_len))
            dev_kfree_skb(skb_monitor);
#endif
        }
        if (rx_status == RX_STAT_MONITOR) {
            rx_status |= RX_STAT_ALLOC;
            if (skb_monitor != skb) {
                dev_kfree_skb(skb);
            }
        }
    }

check_len_update:
    /* Check if we need to update the length */
    if (rx_status & RX_STAT_LEN_UPDATE) {
        if (rx_status & RX_STAT_ETH_LEN_UPDATE) {
            /* Update Length Field inside the Ethernet Header */
            struct ethhdr *hdr = (struct ethhdr *)(skb->data);
            hdr->h_proto = htons(hw_rxhdr->hwvect.len - sizeof(struct ethhdr));
        }

        dev_kfree_skb(skb);
        spin_unlock_bh(&aml_hw->rx_lock);
        return 0;
    }

    /* Check if it must be discarded after informing upper layer */
    if (rx_status & RX_STAT_SPURIOUS) {
        struct ieee80211_hdr *hdr;
        /* Read mac header to obtain Transmitter Address */
        //aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, sync_len);

        hdr = (struct ieee80211_hdr *)(skb->data);
        aml_vif = aml_rx_get_vif(aml_hw, hw_rxhdr->flags_vif_idx);
        if (aml_vif) {
            cfg80211_rx_spurious_frame(aml_vif->ndev, hdr->addr2, GFP_ATOMIC);
        }
        dev_kfree_skb(skb);
        spin_unlock_bh(&aml_hw->rx_lock);
        return 0;
    }

    /* Check if we need to forward the buffer */
    if (rx_status & RX_STAT_FORWARD) {
        struct aml_sta *sta = NULL;
        aml_rx_vector_convert(aml_hw->machw_type, &hw_rxhdr->hwvect.rx_vect1, &hw_rxhdr->hwvect.rx_vect2);

        if (hw_rxhdr->flags_sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
            sta = aml_hw->sta_table + hw_rxhdr->flags_sta_idx;
            /* coverity[remediation] -	hwvect won't cross the line*/
            aml_rx_statistic(aml_hw, hw_rxhdr, sta);
        }

        if (hw_rxhdr->flags_is_80211_mpdu) {
            aml_rx_mgmt_any(aml_hw, skb, hw_rxhdr);
        } else {
            aml_vif = aml_rx_get_vif(aml_hw, hw_rxhdr->flags_vif_idx);

            if (!aml_vif) {
                dev_err(aml_hw->dev, "Frame received but no active vif (%d)", hw_rxhdr->flags_vif_idx);
                dev_kfree_skb(skb);
                spin_unlock_bh(&aml_hw->rx_lock);
                return 0;
            }

            if (sta) {
                if (sta->vlan_idx != aml_vif->vif_index) {
                    aml_vif = aml_hw->vif_table[sta->vlan_idx];
                    if (!aml_vif) {
                        dev_kfree_skb(skb);
                        spin_unlock_bh(&aml_hw->rx_lock);
                        return 0;
                    }
                }

                if (hw_rxhdr->flags_is_4addr && !aml_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(aml_vif->ndev, sta->mac_addr, GFP_ATOMIC);
                }
            }

            skb->priority = 256 + hw_rxhdr->flags_user_prio;
            /* coverity[remediation] -	hwvect won't cross the line*/
            if (!aml_rx_data_skb(aml_hw, aml_vif, skb, hw_rxhdr))
                dev_kfree_skb(skb);
        }
    }
    spin_unlock_bh(&aml_hw->rx_lock);
    return 0;
}

void aml_reo_forward(struct aml_hw *aml_hw, struct sk_buff_head *frames)
{
    struct sk_buff *skb;

    while ((skb = __skb_dequeue(frames))) {
        aml_sdio_usb_rx_skb_handle(aml_hw, skb, RX_STAT_FORWARD);
    }
}

void aml_dynamic_update_tx_page(struct aml_hw *aml_hw)
{
    spin_lock_bh(&aml_hw->tx_buf_lock);
    if (aml_hw->rx_buf_state & BUFFER_EXPAND) {
        aml_hw->g_tx_param.tx_page_free_num = (aml_bus_type == SDIO_MODE) ? SDIO_TX_PAGE_NUM_SMALL : USB_TX_PAGE_NUM_SMALL;
    } else if (aml_hw->rx_buf_state & BUFFER_NARROW) {
        if (aml_bus_type == SDIO_MODE) {
            if (aml_hw->la_enable) {
                aml_hw->g_tx_param.tx_page_free_num = (SDIO_TX_PAGE_NUM_LARGE - SDIO_LA_PAGE_NUM);
            } else {
                aml_hw->g_tx_param.tx_page_free_num = SDIO_TX_PAGE_NUM_LARGE;
            }
        } else if (aml_bus_type == USB_MODE) {
            if (aml_hw->la_enable) {
                aml_hw->g_tx_param.tx_page_free_num = (USB_TX_PAGE_NUM_LARGE - USB_LA_PAGE_NUM);
            } else if (aml_hw->trace_enable) {
                aml_hw->g_tx_param.tx_page_free_num = (USB_TX_PAGE_NUM_LARGE - USB_TRACE_PAGE_NUM);
            } else {
                aml_hw->g_tx_param.tx_page_free_num = USB_TX_PAGE_NUM_LARGE;
            }
        }
    }
    aml_hw->g_tx_param.tx_page_tot_num = aml_hw->g_tx_param.tx_page_free_num;
    spin_unlock_bh(&aml_hw->tx_buf_lock);
}

extern uint8_t rx_need_update;
extern struct crg_msc_cbw *g_cmd_buf;

static void aml_trigger_rst_rxd(struct aml_hw *aml_hw, uint32_t addr_rst, uint32_t recv_pkt_len)
{
    uint32_t cmd_buf[2] = {0};
    static uint32_t last_addr = 0;
    uint16_t max_dyna_num = 0;
    uint16_t free_page_tot_num = 0;
    uint32_t rxbuf_end_small_addr = 0;

    if ((aml_hw->rx_buf_state & BUFFER_STATUS) && (aml_hw->rx_buf_state & BUFFER_NARROW)) {
        rxbuf_end_small_addr = (aml_bus_type == SDIO_MODE) ? RXBUF_END_ADDR_SMALL : USB_RXBUF_END_ADDR_SMALL;
        /* Host had read away all the data before hw_wr addr, if fw_new_pos is in contention space,
          update the host rxbuf pointer and address to the starting position */
        if ((aml_hw->fw_new_pos & ~AML_WRAP) >= rxbuf_end_small_addr) {
            addr_rst = RXBUF_START_ADDR;
            aml_hw->fw_new_pos = RXBUF_START_ADDR;
            if (aml_bus_type == SDIO_MODE)
                AML_REG_WRITE(RXBUF_START_ADDR & 0x1FFFF, aml_hw->plat, 0, RG_WIFI_IF_FW2HST_IRQ_CFG);
        }
        aml_dynamic_update_tx_page(aml_hw);
        addr_rst |= RX_REDUCE_READ_RX_DATA_FINISH;
        AML_INFO("reduce last_addr = %x, addr_rst = %x", last_addr, addr_rst);
    }

    if (aml_hw->rx_buf_state & BUFFER_REDUCE_FINISH) {
        /* Host rx reduce had finished, notify the firmware */
        addr_rst |= HOST_RXBUF_REDUCE_FINISH;
        AML_INFO("reduce finish last_addr = %x, addr_rst = %x", last_addr, addr_rst);
        aml_hw->rx_buf_state &= ~BUFFER_REDUCE_FINISH;
    }

    if ((aml_hw->rx_buf_state & BUFFER_STATUS) && (aml_hw->rx_buf_state & BUFFER_EXPAND)) {
        /* Host had read away all the data before hw_wr addr, update host tx_page_free_num */
        aml_dynamic_update_tx_page(aml_hw);
        addr_rst |= RX_ENLARGE_READ_RX_DATA_FINISH;
        AML_INFO("expend last_addr = %x, addr_rst = %x", last_addr, addr_rst);
    }

    if (aml_hw->rx_buf_state & BUFFER_EXPEND_FINISH) {
        /* Host rx expend had finished, notify the firmware */
        addr_rst |= HOST_RXBUF_ENLARGE_FINISH;
        AML_INFO("expend finish last_addr = %x, addr_rst = %x", last_addr, addr_rst);
        aml_hw->rx_buf_state &= ~BUFFER_EXPEND_FINISH;
    }

    cmd_buf[0] = 1;
    cmd_buf[1] = addr_rst;

    if (last_addr != addr_rst) {
        if (aml_bus_type == USB_MODE) {
            rx_need_update++;
            if (!(aml_hw->rx_buf_state & BUFFER_STATUS) && !(addr_rst & (0x1e000000)) &&
                (aml_hw->rx_buf_state & FW_BUFFER_NARROW) &&
                (rx_need_update < 2) &&
                recv_pkt_len < ((aml_hw->rx_buf_end - RXBUF_START_ADDR) / 4)) { // tx buf 256K
                /* RX read pointer (addr_rst) is embedded in TX CMD, later send it to firmware. */
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX]     = UPDATE_FLAG & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 1] = (UPDATE_FLAG >> 8) & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 2] = (UPDATE_FLAG >> 16) & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 3] = (UPDATE_FLAG >> 24) & 0xff;

                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 4] = cmd_buf[1] & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 5] = (cmd_buf[1] >> 8) & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 6] = (cmd_buf[1] >> 16) & 0xff;
                g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 7] = (cmd_buf[1] >> 24) & 0xff;
            } else {
                aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)cmd_buf, (unsigned char *)(SYS_TYPE)(CMD_DOWN_FIFO_FDH_ADDR), 8, USB_EP4);
            }
        } else if ((aml_bus_type == SDIO_MODE) && (aml_hw->state == WIFI_SUSPEND_STATE_NONE)) {
            aml_hw->plat->hif_sdio_ops->hi_sram_write((unsigned char*)cmd_buf, (unsigned char *)(SYS_TYPE)(CMD_DOWN_FIFO_FDH_ADDR), 8);
        }
        last_addr = addr_rst;
    }
}

static void aml_sdio_usb_dynamic_buffer_check(struct aml_hw *aml_hw)
{
    if (aml_hw->rx_buf_state & BUFFER_STATUS) {
        if (aml_hw->rx_buf_state & BUFFER_NARROW) {
            AML_INFO("reduce fw_new_pos=%x, fw_buf_pos=%x\n",
                       (aml_hw->fw_new_pos & ~AML_WRAP), (aml_hw->fw_buf_pos & ~AML_WRAP));

            if ((aml_hw->rx_buf_state & BUFFER_UPDATE_FLAG) == 0) {
                aml_hw->rx_buf_state |= BUFFER_UPDATE_FLAG;
                return;
            }
            if (aml_bus_type == SDIO_MODE) {
                aml_hw->rx_buf_end = RXBUF_END_ADDR_SMALL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
                aml_rps_switch_check(aml_hw, RPS_ON);
#endif
            } else {
                aml_hw->rx_buf_end = USB_RXBUF_END_ADDR_SMALL;
            }
            aml_hw->rx_buf_state &= ~(BUFFER_STATUS);
            aml_hw->rx_buf_state &= ~(BUFFER_UPDATE_FLAG);
            aml_hw->rx_buf_state |= BUFFER_REDUCE_FINISH;
            AML_INFO("reduce_finish rx_buf_state = %x\n", aml_hw->rx_buf_state);
        } else if (aml_hw->rx_buf_state & BUFFER_EXPAND) {
            AML_INFO("expend fw_new_pos=%x, fw_buf_pos=%x\n",
                (aml_hw->fw_new_pos & ~AML_WRAP), (aml_hw->fw_buf_pos & ~AML_WRAP));
            if ((aml_hw->rx_buf_state & BUFFER_UPDATE_FLAG) == 0) {
                aml_hw->rx_buf_state |= BUFFER_UPDATE_FLAG;
                return;
            }

            if (aml_bus_type == SDIO_MODE) {
                aml_hw->rx_buf_end = (aml_hw->la_enable) ? RXBUF_END_ADDR_LA_LARGE : RXBUF_END_ADDR_LARGE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
                aml_rps_switch_check(aml_hw, RPS_OFF);
#endif
            } else{
                if (aml_hw->la_enable) {
                    aml_hw->rx_buf_end = USB_RXBUF_END_ADDR_LA_LARGE;
                } else if (aml_hw->trace_enable) {
                    aml_hw->rx_buf_end = USB_RXBUF_END_ADDR_TRACE_LARGE;
                } else {
                    aml_hw->rx_buf_end = USB_RXBUF_END_ADDR_LARGE;
                }
            }

            aml_hw->rx_buf_state &= ~(BUFFER_STATUS);
            aml_hw->rx_buf_state &= ~(BUFFER_UPDATE_FLAG);
            aml_hw->rx_buf_state |= BUFFER_EXPEND_FINISH;
            AML_INFO("enlarge_finish rx_buf_state = %x\n", aml_hw->rx_buf_state);
        }
    }
}

void aml_rxbuf_list_init(struct aml_hw *aml_hw)
{
    int i = 0;

    INIT_LIST_HEAD(&aml_hw->rxbuf_free_list);
    INIT_LIST_HEAD(&aml_hw->rxbuf_used_list);

    for (i = 0; i < RXBUF_NUM; i++) {
        INIT_LIST_HEAD(&aml_hw->rxbuf_list[i].list);
        aml_hw->rxbuf_list[i].rxbuf = (u8 *)aml_hw->rxbufs + i * RXBUF_SIZE;
        list_add_tail(&aml_hw->rxbuf_list[i].list, &aml_hw->rxbuf_free_list);
     }
    AML_INFO("host_buf:%08x\n", aml_hw->rxbufs);
}

struct rxbuf_list *aml_get_rxbuf_list_from_used_list(struct aml_hw *aml_hw)
{
    struct rxbuf_list *rxbuf_list = NULL;

    spin_lock_bh(&aml_hw->used_list_lock);
    if (!list_empty(&aml_hw->rxbuf_used_list)) {
        rxbuf_list = list_first_entry(&aml_hw->rxbuf_used_list, struct rxbuf_list, list);
        list_del(&rxbuf_list->list);
    }
    spin_unlock_bh(&aml_hw->used_list_lock);

    return rxbuf_list;
}

static struct rxbuf_list *aml_get_rxbuf_list_from_free_list(struct aml_hw *aml_hw)
{
    struct rxbuf_list *rxbuf_list = NULL;

    spin_lock_bh(&aml_hw->free_list_lock);
    if (!list_empty(&aml_hw->rxbuf_free_list)) {
        rxbuf_list = list_first_entry(&aml_hw->rxbuf_free_list, struct rxbuf_list, list);
        list_del(&rxbuf_list->list);
    }
    spin_unlock_bh(&aml_hw->free_list_lock);

    return rxbuf_list;
}


static inline void aml_stats_rx_trans_update(struct aml_stats *stats, unsigned int len)
{
    int i;

    ++stats->rx_trans_total;
    for (i = 0; i < AML_RX_TRANS_RANK_NUM - 1; i++) {
        if (len >= (AML_RX_TRANS_RANK_SIZE_0 >> i)) {
            ++stats->rx_trans[i];
            return;
        }
    }
    ++stats->rx_trans[i];
}

static inline void hi_rx_buffer_read(struct aml_hw *aml_hw, u8 *buf, u32 addr, unsigned int len)
{
    if (aml_bus_type == SDIO_MODE)
        aml_hw->plat->hif_sdio_ops->hi_rx_buffer_read(buf, (unsigned char *)addr, len, 0);
    else
        aml_hw->plat->hif_ops->hi_rx_buffer_read(buf, (unsigned char *)addr, len, USB_EP4);
}

static s8 aml_sdio_usb_rxdataind(struct aml_hw *aml_hw)
{
    uint32_t fw_buf_pos = aml_hw->fw_buf_pos & ~AML_WRAP;
    uint32_t fw_new_pos = aml_hw->fw_new_pos & ~AML_WRAP;

    REG_SW_SET_PROFILING(aml_hw, SW_PROF_AMLDATAIND);

    while (list_empty(&aml_hw->rxbuf_free_list)) {
        if (aml_hw->rx_buf_state & BUFFER_STATUS) {
            usleep_range(2, 3);
        } else {
            return -1;
        }
    }

    if (aml_hw->fw_buf_pos != aml_hw->fw_new_pos) {
        struct rxbuf_list *rxbuf = aml_get_rxbuf_list_from_free_list(aml_hw);

        if (rxbuf) {
            uint32_t first_len;
            uint32_t second_len;
            uint32_t recv_pkt_len;

            /* host rxbuf dynamic switch */
            aml_sdio_usb_dynamic_buffer_check(aml_hw);
            rxbuf->rx_buf_end = aml_hw->rx_buf_end;

            /*
             * if the remainder space of rx ring is not enough for a rx desc,
             * new RX starts from RXBUF_START_ADDR.
             */
            if (!RXBUF_ENOUGH_FOR(RX_DESC_SIZE, fw_buf_pos, rxbuf->rx_buf_end))
                fw_buf_pos = RXBUF_START_ADDR;

            /* set firmware positions (pointers) */
            rxbuf->fw_pos = fw_buf_pos;
            rxbuf->rxbuf_data_start = fw_buf_pos;
            rxbuf->rxbuf_data_end = fw_new_pos;

            if (fw_new_pos > fw_buf_pos) {
                first_len = fw_new_pos - fw_buf_pos;
                second_len = 0;
            } else {
                first_len = rxbuf->rx_buf_end - fw_buf_pos;
                second_len = fw_new_pos - RXBUF_START_ADDR;
            }
            BUG_ON(first_len < RX_DESC_SIZE);

            /* set host memory pointers */
            rxbuf->pos = rxbuf->rxbuf;
            rxbuf->gap = rxbuf->pos + first_len;
            rxbuf->end = rxbuf->gap + second_len;
            hi_rx_buffer_read(aml_hw, rxbuf->pos, fw_buf_pos, first_len);
            if (second_len)
                hi_rx_buffer_read(aml_hw, rxbuf->gap, RXBUF_START_ADDR, second_len);
            recv_pkt_len = first_len + second_len;
            aml_stats_rx_trans_update(aml_hw->stats, recv_pkt_len);

            spin_lock_bh(&aml_hw->used_list_lock);
            list_add_tail(&rxbuf->list, &aml_hw->rxbuf_used_list);
            spin_unlock_bh(&aml_hw->used_list_lock);

            BUG_ON((aml_hw->fw_new_pos & ~AML_WRAP) != fw_new_pos);

            /* FIXME: call it with aml_hw->fw_new_pos or fw_new_pos? */
            aml_trigger_rst_rxd(aml_hw, aml_hw->fw_new_pos, recv_pkt_len);
            aml_hw->fw_buf_pos = aml_hw->fw_new_pos;
        }
    }
    up(&aml_hw->aml_rx_sem);

    REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_AMLDATAIND);
    return -1;
}

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
static void aml_sdio_usb_reorder_list_check(struct aml_hw *aml_hw, struct fw_reo_info *reo)
{
    struct rxdata *rxdata = NULL;
    struct rxdata *rxdata_tmp = NULL;
    int i = 0;

    for (i = 0; i < reo->reorder_len; ++i) {
        u16 sn = ieee80211_sn_add(reo->hostid, i);

        if (!sn)
            sn = IEEE80211_SN_MODULO;

        list_for_each_entry_safe(rxdata, rxdata_tmp, &reorder_list, list) {
            if (rxdata->tid == reo->tid && rxdata->hostid == sn) {
                aml_sdio_usb_rx_skb_handle(aml_hw, rxdata->skb, RX_STAT_FORWARD);
                list_del(&rxdata->list);
                aml_put_rxdata_back_to_free_list(rxdata);
            }
        }
    }
}

static void aml_sdio_usb_fw_reorder_handle(struct aml_hw *aml_hw, const struct rxdesc *rxdesc)
{
    const struct rx_hd *hd = &rxdesc->dma_hdrdesc.hd;
    struct fw_reo_info embedded_info = {
        .hostid = hd->reorder.hostid,
        .tid = hd->reorder.tid,
        .reorder_len = hd->reorder.len,
    };
    int i;

    /*
     * NB: handle the reorder instruction embedded in rxdesc, or received by e2a msg here.
     * but the handling sequence can't be guaranteed same as it's generated in firmware.
     * it introduces the out-of-order issue.
     *
     * Therefore, for USB/SDIO, host reorder is preferred.
     * firmware reorder is just for internal test.
     */

    /* handle the reorder instruction embedded in rxdesc */
    if (embedded_info.reorder_len)
        aml_sdio_usb_reorder_list_check(aml_hw, &embedded_info);

    /* handle reorder timeout packets by e2a msg */
    for (i = 0; i < IEEE80211_NUM_UPS; i++) {
        extern struct fw_reo_info reorder_info[IEEE80211_NUM_UPS];

        spin_lock_bh(&aml_hw->reorder_lock);
        if (reorder_info[i].reorder_len) {
            aml_sdio_usb_reorder_list_check(aml_hw, &reorder_info[i]);
            reorder_info[i].reorder_len = 0;
        }
        spin_unlock_bh(&aml_hw->reorder_lock);
    }
}
#endif

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
extern cfm_log cfmlog;
#endif
#endif

/* SDIO/USB rx task */
int aml_rx_task(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;

    aml_sched_rt_set(SCHED_RR, AML_TASK_PRI);
    while (!aml_hw->aml_rx_task_quit) {
        struct rxbuf_list *rxbuf = aml_get_rxbuf_list_from_used_list(aml_hw);

        if (!rxbuf) {
            /* wait for work */
            if (down_interruptible(&aml_hw->aml_rx_sem) == 0)
                continue;

            /* interrupted, exit */
            AML_INFO("wait aml_rx_sem fail!\n");
            break;
        }

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
        cfmlog.rx_cnt_in_rx++;
#endif
#endif

        while (!aml_hw->aml_rx_task_quit && rxbuf->pos < rxbuf->end) {
            struct rxdesc *rxdesc = (struct rxdesc *)rxbuf->pos;
            enum rx_status_bits rx_status = rxdesc->dma_hdrdesc.hd.status;
            struct sk_buff *skb = aml_sdio_usb_rx_skb_from_rxbuf(aml_hw, rxbuf);

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
            cfmlog.mpdu_in_rx++;
#endif
#endif

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
            /* auto-learning, if firmware is configured to reorder in host */
            if (rx_status == RX_STAT_HOST_REO)
                aml_sdio_usb_host_reorder_detected(aml_hw);
#endif

            if (!skb) {
                /* current frame is skipped, or internal error happened. */
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
            } else if (!aml_hw->aml_sdio_usb_host_reorder) {
                if (rx_status == RX_STAT_ALLOC) {
                    struct rxdata *rxdata = aml_get_rxdata_from_free_list();

                    /* add this frame into reorder list */
                    if (rxdata) {
                        struct rx_hd *hd = &rxdesc->dma_hdrdesc.hd;

                        rxdata->skb = skb;
                        rxdata->hostid = hd->hostid;
                        rxdata->tid = hd->reorder.tid;
                        list_add_tail(&rxdata->list, &reorder_list);
                    }  else {
                        dev_kfree_skb(skb);
                    }
                } else {
                    aml_sdio_usb_rx_skb_handle(aml_hw, skb, rx_status);
                }
#endif
            } else {
                struct hw_rxhdr *rhd = (struct hw_rxhdr *)skb->data;
                struct aml_rhd_ext *rhd_ext = AML_RHD_EXT(rhd);
                struct aml_reo_session *reo = NULL;

                /*
                 * if rx_status != RX_STAT_HOST_REO, the frame is not under a RX BA,
                 * or it's received before the BA(reo) session is created,
                 * do not try to get the reo session or handle the frame under it.
                 */
                if (rhd_ext->qos && rx_status == RX_STAT_HOST_REO)
                    reo = aml_reo_session_get(aml_hw, rhd->flags_sta_idx, rhd->flags_user_prio);

                if (reo) {
                    struct sk_buff_head frames;

                    __skb_queue_head_init(&frames);

                    aml_reo_enqueue(reo, skb, &frames);
                    aml_reo_session_put(reo);

                    aml_reo_forward(aml_hw, &frames);
                } else {
                    if (rx_status == RX_STAT_HOST_REO) {
                        net_info_ratelimited("got a frame under BA, but no reo session! "
                                 "forward it by default. sta %u tid %u qos %d sn %u\n",
                                 rhd->flags_sta_idx, rhd->flags_user_prio,
                                 rhd_ext->qos, rhd_ext->sn);
                        rx_status = RX_STAT_FORWARD;
                    }
                    aml_sdio_usb_rx_skb_handle(aml_hw, skb, rx_status);
                }
            }

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
            /* handle reorder info (reorder instruction from firmware) */
            if (!aml_hw->aml_sdio_usb_host_reorder)
                aml_sdio_usb_fw_reorder_handle(aml_hw, rxdesc);
#endif
        }

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
        cfmlog.avg_mpdu_in_one_rx = cfmlog.mpdu_in_rx / cfmlog.rx_cnt_in_rx;
#endif
#endif

        spin_lock_bh(&aml_hw->free_list_lock);
        list_add_tail(&rxbuf->list, &aml_hw->rxbuf_free_list);
        spin_unlock_bh(&aml_hw->free_list_lock);
    }

    if (aml_hw->aml_rx_completion_init) {
        aml_hw->aml_rx_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
        complete_and_exit(&aml_hw->aml_rx_completion, 0);
#else
        complete(&aml_hw->aml_rx_completion);
#endif
    }

    return 0;
}

#ifdef DEBUG_CODE
struct debug_proc_rxbuff_info debug_proc_rxbuff[DEBUG_RX_BUF_CNT];
u16 debug_rxbuff_idx = 0;
static void record_proc_rx_buf(u16 status, u32 dma_addr, u32 host_id, struct aml_hw *aml_hw)
{
    debug_proc_rxbuff[debug_rxbuff_idx].addr = dma_addr;
    debug_proc_rxbuff[debug_rxbuff_idx].idx = aml_hw->ipc_env->rxdesc_idx;
    debug_proc_rxbuff[debug_rxbuff_idx].buff_idx = aml_hw->ipc_env->rxbuf_idx;
    debug_proc_rxbuff[debug_rxbuff_idx].status = status;
    debug_proc_rxbuff[debug_rxbuff_idx].hostid = host_id;
    debug_proc_rxbuff[debug_rxbuff_idx].time = jiffies;
    debug_rxbuff_idx++;
    if (debug_rxbuff_idx == DEBUG_RX_BUF_CNT) {
        debug_rxbuff_idx = 0;
    }
}
#endif

/**
 * aml_pci_rxdataind - Process rx buffer
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct aml_hw is this case)
 * @arg: Address of the RX descriptor
 *
 * This function is called for each buffer received by the fw
 *
 */
int aml_pci_rxdataind(void *pthis, void *hostid)
{
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *ipc_desc = hostid;
    struct aml_ipc_buf *ipc_buf;
    struct hw_rxhdr *hw_rxhdr = NULL;
    struct hw_rxhdr hw_rxhdr_copy;
    struct rxdesc_tag *rxdesc;
    struct aml_vif *aml_vif;
    struct sk_buff *skb = NULL;
    int msdu_offset = sizeof(struct hw_rxhdr);
    int nb_buff = 1;
    u16_l status;

    BUG_ON(aml_bus_type != PCIE_MODE);

    REG_SW_SET_PROFILING(aml_hw, SW_PROF_AMLDATAIND);

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy_flags_chk(AML_RECY_FW_ONGOING)) {
        /* recovery fw is ongoing, do nothing for rx data */
        return -1;
    }
#endif

    aml_ipc_buf_e2a_sync(aml_hw, ipc_desc, sizeof(struct rxdesc_tag));

    rxdesc = ipc_desc->addr;
    status = rxdesc->status;

#ifdef DEBUG_CODE
    record_proc_rx_buf(status, ipc_desc->dma_addr, rxdesc->host_id, aml_hw);
#endif
    if (!status) {
        /* frame is not completely uploaded, give back ownership of the descriptor */
        aml_ipc_buf_e2a_sync_back(aml_hw, ipc_desc, sizeof(struct rxdesc_tag));
        return -1;
    }

#ifdef CONFIG_AML_USE_TASK
    aml_spin_lock(&aml_hw->pcie.rxdesc_lock);
#endif

    ipc_buf = aml_ipc_rxbuf_from_hostid(aml_hw, rxdesc->host_id);
    if (!ipc_buf) {
        goto check_alloc;
    }
    skb = ipc_buf->addr;

    /* Check if we need to delete the buffer */
    if (status & RX_STAT_DELETE) {
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, msdu_offset);
        nb_buff = aml_rx_amsdu_free(aml_hw, hw_rxhdr);
        aml_ipc_rxbuf_dealloc(aml_hw, ipc_buf);
        goto check_alloc;
    }

    /* Check if we need to forward the buffer coming from a monitor interface */
    if (status & RX_STAT_MONITOR) {
        struct sk_buff *skb_monitor = NULL;
        u8 rtap_len;
        u16 frm_len;

        // Check if monitor interface exists and is open
        aml_vif = aml_rx_get_vif(aml_hw, aml_hw->monitor_vif);
        if (!aml_vif || (aml_vif->wdev.iftype != NL80211_IFTYPE_MONITOR)) {
            dev_err(aml_hw->dev, "Received monitor frame but there is no monitor interface open\n");
            goto check_len_update;
        }

        aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, sizeof(hw_rxhdr));
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        aml_rx_vector_convert(aml_hw->machw_type,
                               &hw_rxhdr->hwvect.rx_vect1,
                               &hw_rxhdr->hwvect.rx_vect2);
        rtap_len = aml_rx_rtap_hdrlen(&hw_rxhdr->hwvect.rx_vect1, false);

        skb_reserve(skb, msdu_offset);
        frm_len = le32_to_cpu(hw_rxhdr->hwvect.len);

        if (status == RX_STAT_MONITOR) {
            status |= RX_STAT_ALLOC;

            aml_ipc_buf_e2a_release(aml_hw, ipc_buf);

            if (frm_len > skb_tailroom(skb))
                frm_len = skb_tailroom(skb);
            skb_put(skb, frm_len);

            memcpy(&hw_rxhdr_copy, hw_rxhdr, sizeof(hw_rxhdr_copy));
            hw_rxhdr = &hw_rxhdr_copy;

            if (rtap_len > msdu_offset) {
                if (skb_end_offset(skb) < frm_len + rtap_len) {
                    // not enough space in the buffer need to re-alloc it
                    if (pskb_expand_head(skb, rtap_len, 0, GFP_ATOMIC)) {
                        dev_kfree_skb(skb);
                        goto check_alloc;
                    }
                } else {
                    // enough space but not enough headroom, move data (instead of re-alloc)
                    int delta = rtap_len - msdu_offset;
                    memmove(skb->data, skb->data + delta, frm_len);
                    skb_reserve(skb, delta);
                }
            }
            skb_monitor = skb;
        }
        else
        {
#ifdef CONFIG_AML_MON_DATA
            if (status & RX_STAT_FORWARD)
                // OK to release here, and other call to release in forward will do nothing
                aml_ipc_buf_e2a_release(aml_hw, ipc_buf);
            else
                aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, 0);

            // Use reserved field to save info that RX vect has already been converted
            hw_rxhdr->hwvect.reserved = 1;
            skb_monitor = aml_rx_dup_for_monitor(aml_hw, skb, hw_rxhdr, rtap_len, &nb_buff);
            if (!skb_monitor) {
                hw_rxhdr = NULL;
                goto check_len_update;
            }
#else
            wiphy_err(aml_hw->wiphy, "RX status %d is invalid when MON_DATA is disabled\n", status);
            goto check_len_update;
#endif
        }

        aml_rx_monitor(aml_hw, aml_vif, skb_monitor, hw_rxhdr, rtap_len);
    }

check_len_update:
    /* Check if we need to update the length */
    if (status & RX_STAT_LEN_UPDATE) {
        int sync_len = msdu_offset + sizeof(struct ethhdr);

        aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, sync_len);

        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        hw_rxhdr->hwvect.len = rxdesc->frame_len;

        if (status & RX_STAT_ETH_LEN_UPDATE) {
            /* Update Length Field inside the Ethernet Header */
            struct ethhdr *hdr = (struct ethhdr *)((u8 *)hw_rxhdr + msdu_offset);
            hdr->h_proto = htons(rxdesc->frame_len - sizeof(struct ethhdr));
        }

        aml_ipc_buf_e2a_sync_back(aml_hw, ipc_buf, sync_len);
        goto end;
    }

    /* Check if it must be discarded after informing upper layer */
    if (status & RX_STAT_SPURIOUS) {
        struct ieee80211_hdr *hdr;
        size_t sync_len =  msdu_offset + sizeof(*hdr);

        /* Read mac header to obtain Transmitter Address */
        aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, sync_len);

        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        hdr = (struct ieee80211_hdr *)(skb->data + msdu_offset);
        aml_vif = aml_rx_get_vif(aml_hw, hw_rxhdr->flags_vif_idx);
        if (aml_vif) {
            cfg80211_rx_spurious_frame(aml_vif->ndev, hdr->addr2, GFP_ATOMIC);
        }
        aml_ipc_buf_e2a_sync_back(aml_hw, ipc_buf, sync_len);
        aml_ipc_rxbuf_repush(aml_hw, ipc_buf);
        goto end;
    }

    /* Check if we need to forward the buffer */
    if (status & RX_STAT_FORWARD) {
        struct aml_sta *sta = NULL;

        aml_ipc_buf_e2a_release(aml_hw, ipc_buf);

        hw_rxhdr = (struct hw_rxhdr *)skb->data;
#ifdef CONFIG_AML_MON_DATA
        if (!hw_rxhdr->hwvect.reserved)
#endif
            aml_rx_vector_convert(aml_hw->machw_type,
                                   &hw_rxhdr->hwvect.rx_vect1,
                                   &hw_rxhdr->hwvect.rx_vect2);
        skb_reserve(skb, msdu_offset);

        aml_rx_statistic(aml_hw, &hw_rxhdr->hwvect);
        if (hw_rxhdr->flags_sta_idx != AML_STA_ID_UNKNOWN) {
            sta = aml_sta_get(aml_hw, hw_rxhdr->flags_sta_idx);
            if (sta) {
                /* coverity[remediation] -  hwvect won't cross the line */
                aml_rx_sta_stats(aml_hw, sta, &hw_rxhdr->hwvect);
            }
        }

        if (hw_rxhdr->flags_is_80211_mpdu) {
            u32 frm_len = le32_to_cpu(hw_rxhdr->hwvect.len);

            if (frm_len > skb_tailroom(skb)) {
                // frame has been truncated by firmware, skip it
                wiphy_err(aml_hw->wiphy, "MMPDU truncated, skip it\n");
                dev_kfree_skb(skb);
                goto end;
            }
            skb_put(skb, frm_len);
            /* coverity[TAINTED_SCALAR] */
            aml_rx_mgmt_any(aml_hw, skb, hw_rxhdr);
        } else {
            aml_vif = aml_rx_get_vif(aml_hw, hw_rxhdr->flags_vif_idx);
            if (!aml_vif) {
                dev_err(aml_hw->dev, "Frame received but no active vif (%d)",
                        hw_rxhdr->flags_vif_idx);
                nb_buff = aml_rx_amsdu_free(aml_hw, hw_rxhdr);
                dev_kfree_skb(skb);
                goto check_alloc;
            }

            if (sta) {
                if (sta->vlan_idx != aml_vif->vif_index) {
                    aml_vif = aml_hw->vif_table[sta->vlan_idx];
                    if (!aml_vif) {
                        nb_buff = aml_rx_amsdu_free(aml_hw, hw_rxhdr);
                        dev_kfree_skb(skb);
                        goto check_alloc;
                    }
                }

                if (hw_rxhdr->flags_is_4addr && !aml_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(aml_vif->ndev,
                                                       sta->mac_addr, GFP_ATOMIC);
                }
            }

            skb->priority = 256 + hw_rxhdr->flags_user_prio;
            skb->dev = aml_vif->ndev;
            nb_buff = aml_rx_data_skb(aml_hw, aml_vif, skb, hw_rxhdr);
        }
    }

check_alloc:
    /* Check if we need to allocate a new buffer */
    if (status & RX_STAT_ALLOC) {
        if (!hw_rxhdr && skb) {
            // True for buffer uploaded with only RX_STAT_ALLOC
            // (e.g. MPDU received out of order in a BA)
            hw_rxhdr = (struct hw_rxhdr *)skb->data;
            aml_ipc_buf_e2a_sync(aml_hw, ipc_buf, msdu_offset);
            if (hw_rxhdr->flags_is_amsdu) {
                int i;
                for (i = 0; i < ARRAY_SIZE(hw_rxhdr->amsdu_hostids); i++) {
                    if (!hw_rxhdr->amsdu_hostids[i])
                        break;
                    nb_buff++;
                }
            }
        }

        while (nb_buff--) {
            if ((aml_bus_type == PCIE_MODE) && atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
                aml_hw->repush_rxbuff_cnt++;
            } else {
                aml_ipc_rxbuf_alloc(aml_hw);
            }
        }
    }

end:
    REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_AMLDATAIND);

    /*if suspend,repush when resume*/
    if ((aml_bus_type == PCIE_MODE) && atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        struct rxdesc_tag *rxdesc = ipc_desc->addr;
        /* coverity[LOCK_EVASION] - ignore coverity warning */
        rxdesc->status = 0;
        dma_sync_single_for_device(aml_hw->dev, ipc_desc->dma_addr,
            sizeof(struct rxdesc_tag), DMA_BIDIRECTIONAL);
        aml_hw->repush_rxdesc = 1;
        AML_INFO("suspend state:rxdesc idx=%u\n",aml_hw->ipc_env->rxdesc_idx);
    } else {
        /* Reset and repush descriptor to FW */
        aml_ipc_rxdesc_repush(aml_hw, ipc_desc);
    }

#ifdef CONFIG_AML_USE_TASK
    aml_spin_unlock(&aml_hw->pcie.rxdesc_lock);
#endif
    return 0;
}

/**
 * aml_rx_deferred - Work function to defer processing of buffer that cannot be
 * done in aml_rxdataind (that is called in atomic context)
 *
 * @ws: work field within struct aml_defer_rx
 */
void aml_rx_deferred(struct work_struct *ws)
{
    struct aml_defer_rx *rx = container_of(ws, struct aml_defer_rx, work);
    struct sk_buff *skb;

    while ((skb = skb_dequeue(&rx->sk_list)) != NULL) {
        // Currently only management frame can be deferred
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;
        struct aml_defer_rx_cb *rx_cb = (struct aml_defer_rx_cb *)skb->cb;

        if (ieee80211_is_action(mgmt->frame_control) &&
            (mgmt->u.action.category == 6)) {
            struct cfg80211_ft_event_params ft_event;
            struct aml_vif *vif = rx_cb->vif;
            u8 *action_frame = (u8 *)&mgmt->u.action;
            u8 action_code = action_frame[1];
            /* coverity[overrun-local] - compute status code from ft action frame */
            u16 status_code = *((u16 *)&action_frame[2 + 2 * ETH_ALEN]);

            if ((action_code == 2) && (status_code == 0)) {
                ft_event.target_ap = action_frame + 2 + ETH_ALEN;
                ft_event.ies = action_frame + 2 + 2 * ETH_ALEN + 2;
                ft_event.ies_len = skb->len - (ft_event.ies - (u8 *)mgmt);
                ft_event.ric_ies = NULL;
                ft_event.ric_ies_len = 0;
                cfg80211_ft_event(rx_cb->vif->ndev, &ft_event);
                vif->sta.flags |= AML_STA_FT_OVER_DS;
                memcpy(vif->sta.ft_target_ap, ft_event.target_ap, ETH_ALEN);
            }
        } else if (ieee80211_is_auth(mgmt->frame_control)) {
            struct cfg80211_ft_event_params ft_event;
            struct aml_vif *vif = rx_cb->vif;
            ft_event.target_ap = vif->sta.ft_target_ap;
            ft_event.ies = mgmt->u.auth.variable;
            ft_event.ies_len = (skb->len -
                                offsetof(struct ieee80211_mgmt, u.auth.variable));
            ft_event.ric_ies = NULL;
            ft_event.ric_ies_len = 0;
            cfg80211_ft_event(rx_cb->vif->ndev, &ft_event);
            vif->sta.flags |= AML_STA_FT_OVER_AIR;
        } else {
            netdev_warn(rx_cb->vif->ndev, "Unexpected deferred frame fctl=0x%04x",
                        mgmt->frame_control);
        }

        dev_kfree_skb(skb);
    }
}
