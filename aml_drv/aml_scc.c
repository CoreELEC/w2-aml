/**
 ******************************************************************************
 *
 * @file aml_scc.c
 *
 * @brief handle sta + softap scc modes
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#include "aml_msg_tx.h"
#include "aml_mod_params.h"
#include "reg_access.h"
#include "aml_compat.h"
#include "fi_cmd.h"
#include "share_mem_map.h"
#include "aml_scc.h"
#include "aml_wq.h"
#include "aml_sap.h"

#define CSA_COUNT               10
#define CSA_BLOCK_TX            1

/* Headers */
struct ieee80211_header {
    __le16 frame_ctl;
    __le16 duration_id;
    u8 da[MAC_ADDR_LEN];
    u8 sa[MAC_ADDR_LEN];
    u8 bssid[MAC_ADDR_LEN];
    __le16 seq_ctl;
    u8 payload[0];
} __packed;

struct aml_extended_csa {
    u8 category;
    u8 action_code;
    u8 info[4];
} __packed;

struct aml_csa_wrapper {
    u8 ie;
    u8 len;
    u8 info[3];
} __packed;

struct aml_csa_data {
    struct ieee80211_header mac_header;
    struct aml_extended_csa extend_csa;
    struct aml_csa_wrapper csa_wrapper;
} __packed;

static u8 scc_use_csa = 1;
static u8 probe_rsp_save[AML_SCC_FRMBUF_MAXLEN];
static u32 sap_init_band;
u32 beacon_need_update = 0;
static u8 *scc_bcn_buf = NULL;  //used in new channel
static u8 *scc_csa_bcn = NULL;  //used in old channel, add csa ie

/*p2p related*/
u8 g_scc_p2p_save[MAX_P2P_SAVE_LEN] = {0,};
u32 g_scc_p2p_len_before = 0;
u32 g_scc_p2p_len_diff = 0;
bool g_scc_p2p_peer_5g_support = 0;

char chan_width_trace[][35] = {
    "NL80211_CHAN_WIDTH_20_NOHT",
    "NL80211_CHAN_WIDTH_20",
    "NL80211_CHAN_WIDTH_40",
    "NL80211_CHAN_WIDTH_80",
    "NL80211_CHAN_WIDTH_80P80",
    "NL80211_CHAN_WIDTH_160",
    "NL80211_CHAN_WIDTH_5",
    "NL80211_CHAN_WIDTH_10"
};

/**
 * This function is used to check new vif's chan is same or diff with existing vif's chan
 *
 * Return vif's idx of diff chan
 */
u8 aml_scc_get_conflict_vif_idx(struct aml_vif *sap_vif)
{
    u32 i;

    for (i = 0; i < NX_VIRT_DEV_MAX; i++) {
        struct aml_vif *sta_vif = sap_vif->aml_hw->vif_table[i];

        if (sta_vif && sta_vif->up &&
                sta_vif->vif_index != sap_vif->vif_index &&
                sta_vif->ch_index != AML_CH_NOT_SET &&
                sap_vif->ch_index != AML_CH_NOT_SET) {
            if (sta_vif->ch_index != sap_vif->ch_index) {
                AML_INFO("scc channel conflict , sta:[%d,%d], softap:[%d,%d]\n",
                         sta_vif->vif_index, sta_vif->ch_index, sap_vif->vif_index, sap_vif->ch_index);
                return sta_vif->vif_index;
            }
        }
    }

    return 0xff;
}

/**
 * This function refresh beacon IEs by probe rsp frame
 *
 */
int aml_scc_change_beacon(struct aml_hw *aml_hw, struct aml_vif *vif)
{
    AML_DBG(AML_FN_ENTRY_STR);

    if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP) {
        struct net_device *dev = vif->ndev;
        struct aml_bcn *bcn = &vif->ap.bcn;
        struct aml_ipc_buf buf = {0};
        int error;
        unsigned int addr;
        u8 *bcn_ie_addr;
        u32 offset;
        u8 e_id;
        u8 e_len;

        // Build the beacon
        if (scc_bcn_buf == NULL) {
            scc_bcn_buf = kmalloc(AML_SCC_FRMBUF_MAXLEN, GFP_KERNEL);

            if (!scc_bcn_buf) {
                return -ENOMEM;
            }
        }

        memcpy(scc_bcn_buf, bcn_save, bcn->len);
        bcn_ie_addr = aml_get_beacon_ie_addr(scc_bcn_buf, bcn->len, WLAN_EID_SSID);

        if (!bcn_ie_addr) {
            return -1;
        }

        *(bcn_ie_addr - 1 ) = 0;
        offset = bcn_ie_addr - scc_bcn_buf;

        while (offset < bcn->len) {
            e_id = scc_bcn_buf[offset];
            e_len = scc_bcn_buf[offset + 1];

            if ((e_id != WLAN_EID_TIM) && (e_id != WLAN_EID_SSID)) {
                u8 *src_ie_addr = aml_get_probe_rsp_ie_addr(probe_rsp_save, 500, e_id);

                if (src_ie_addr != NULL) {
                    u8 src_ie_len = *(src_ie_addr + 1);

                    if (src_ie_len == e_len) {
                        memcpy(&scc_bcn_buf[offset], src_ie_addr, e_len + 2);
                    }
                }
            }

            offset += e_len + 2;
        }

        // Sync buffer for FW
        if (aml_bus_type == PCIE_MODE) {
            if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, scc_bcn_buf, bcn->len))) {
                netdev_err(dev, "Failed to allocate IPC buf for new beacon\n");
                return error;
            }
        }
        else if (aml_bus_type == USB_MODE) {
            addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
            aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)scc_bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len, USB_EP4);
        }
        else if (aml_bus_type == SDIO_MODE) {
            addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
            aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)scc_bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len);
        }

        // Forward the information to the LMAC
        error = aml_send_bcn_change(aml_hw, vif->vif_index, buf.dma_addr,
                                    bcn->len, bcn->head_len, bcn->tim_len, NULL);

        if (aml_bus_type == PCIE_MODE) {
            if (buf.addr) {
                dma_unmap_single(aml_hw->dev, buf.dma_addr, buf.size, DMA_TO_DEVICE);
            }
        }

        return error;
    }
    else {
        AML_INFO("type error:%d", AML_VIF_TYPE(vif));
    }

    return -1;
}

/**
 * This function change beacon frame's HT IE primary chan info
 *
 */
static int aml_scc_change_beacon_ht_ie(struct wiphy *wiphy, struct net_device *dev,
                                       struct cfg80211_chan_def target_chdef)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_bcn *bcn = &vif->ap.bcn;
    struct aml_ipc_buf buf = {0};
    int error;
    unsigned int addr;
    u8 *ds_ie, *vht_ie, *ht_ie;
    u8 len;
    u8 *var_pos;
    u32 target_freq = target_chdef.chan->center_freq;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    u8 chan_no = aml_ieee80211_freq_to_chan(target_freq, target_chdef.chan->band);

    // Build the beacon
    if (scc_bcn_buf == NULL) {
        scc_bcn_buf = kmalloc(AML_SCC_FRMBUF_MAXLEN, GFP_KERNEL);

        if (!scc_bcn_buf) {
            return -ENOMEM;
        }
    }

    memcpy(scc_bcn_buf, bcn_save, bcn->len);
    /*change saved buffer ht INFO CHAN*/
    len = bcn->len - var_offset;
    var_pos = scc_bcn_buf + var_offset;
    /*find DS ie*/
    ds_ie = (u8 *)cfg80211_find_ie(WLAN_EID_DS_PARAMS, var_pos, len);

    if (ds_ie && (ds_ie[1] == 1)) {
        ds_ie[2] = chan_no;
    }

    /*find HT ie*/
    ht_ie = (u8 *)cfg80211_find_ie(WLAN_EID_HT_OPERATION, var_pos, len);

    if (ht_ie && ht_ie[1] >= sizeof(struct ieee80211_ht_operation)) {
        struct ieee80211_ht_operation *htop = (void *)(ht_ie + 2);
        htop->primary_chan = chan_no;
        /*clear second channel offset,channel width*/
        htop->ht_param &= 0xF8;

        if (target_chdef.width >= NL80211_CHAN_WIDTH_40) {
            htop->ht_param |= IEEE80211_HT_PARAM_CHAN_WIDTH_ANY;

            if (target_chdef.chan->center_freq > target_chdef.center_freq1) {
                htop->ht_param |= IEEE80211_HT_PARAM_CHA_SEC_BELOW;
            }
            else if (target_chdef.chan->center_freq < target_chdef.center_freq1) {
                htop->ht_param |= IEEE80211_HT_PARAM_CHA_SEC_ABOVE;
            }
            else {
                AML_INFO("err: primary=%u,center=%u,width=%u", target_chdef.chan->center_freq, target_chdef.center_freq1, target_chdef.width);
            }
        }
    }

    /*find vht ie*/
    vht_ie = (u8 *)cfg80211_find_ie(WLAN_EID_VHT_OPERATION, var_pos, len);

    if (vht_ie && (vht_ie[1] >= sizeof(struct ieee80211_vht_operation))) {
        struct ieee80211_vht_operation *vhtop = (void *)(vht_ie + 2);

        switch (target_chdef.width) {
            case NL80211_CHAN_WIDTH_20:
            case NL80211_CHAN_WIDTH_40:
                vhtop->chan_width = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
                vhtop->center_freq_seg0_idx = 0;
                vhtop->center_freq_seg1_idx = 0;
#else
                vhtop->center_freq_seg1_idx = 0;
                vhtop->center_freq_seg2_idx = 0;
#endif
                break;

            case NL80211_CHAN_WIDTH_80:
            case NL80211_CHAN_WIDTH_160:
            case NL80211_CHAN_WIDTH_80P80:
                vhtop->chan_width = 1;

                if (target_chdef.center_freq1)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
                    vhtop->center_freq_seg0_idx = aml_ieee80211_freq_to_chan(target_chdef.center_freq1, target_chdef.chan->band);

#else
                    vhtop->center_freq_seg1_idx = aml_ieee80211_freq_to_chan(target_chdef.center_freq1, target_chdef.chan->band);
#endif

                if (target_chdef.center_freq2)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
                    vhtop->center_freq_seg1_idx = aml_ieee80211_freq_to_chan(target_chdef.center_freq2, target_chdef.chan->band);

#else
                    vhtop->center_freq_seg2_idx = aml_ieee80211_freq_to_chan(target_chdef.center_freq2, target_chdef.chan->band);
#endif
                break;
            default:
                WARN_ON(1);
                break;
        }
    }

    aml_save_bcn_buf(scc_bcn_buf, bcn->len);

    // Sync buffer for FW
    if (aml_bus_type == PCIE_MODE) {
        if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, scc_bcn_buf, bcn->len))) {
            netdev_err(dev, "Failed to allocate IPC buf for new beacon\n");
            return error;
        }
    }
    else if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)scc_bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len, USB_EP4);
    }
    else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)scc_bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len);
    }

    // Forward the information to the LMAC
    error = aml_send_bcn_change(aml_hw, vif->vif_index, buf.dma_addr,
                                bcn->len, bcn->head_len, bcn->tim_len, NULL);

    if (aml_bus_type == PCIE_MODE) {
        if (buf.addr) {
            dma_unmap_single(aml_hw->dev, buf.dma_addr, buf.size, DMA_TO_DEVICE);
        }
    }

    return error;
}

static int aml_scc_sync_bcn(struct aml_hw *aml_hw, void *ptr)
{
    struct aml_vif *vif = ptr;

    if (AML_SCC_BEACON_WAIT_DOWNLOAD()) {
        if (!aml_scc_change_beacon(aml_hw, vif)) {
            AML_SCC_CLEAR_BEACON_UPDATE();
        }
    }

    return 0;
}

void aml_scc_save_probe_rsp(struct aml_vif *vif, u8 *buf, u32 buf_len)
{
    if (AML_SCC_BEACON_WAIT_PROBE() && (AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP)) {
        if (buf_len > AML_SCC_FRMBUF_MAXLEN) {
            AML_INFO("scc probe response buffer len exceed %d", AML_SCC_FRMBUF_MAXLEN);
            buf_len = AML_SCC_FRMBUF_MAXLEN;
        }

        memcpy(probe_rsp_save, buf, buf_len);
        AML_SCC_BEACON_SET_STATUS(BEACON_UPDATE_WAIT_DOWNLOAD);
        aml_wq_do_ptr(aml_scc_sync_bcn, vif->aml_hw, vif);
    }
}

/**
 * This function is called when start ap,and save band info
 *
 */
void aml_scc_save_init_band(u32 init_band)
{
    sap_init_band = init_band;
}

static u32 aml_scc_get_init_band(void)
{
    return sap_init_band;
}

static void aml_scc_csa_finish(struct work_struct *ws)
{
    struct aml_csa *csa = container_of(ws, struct aml_csa, work);
    struct aml_vif *vif = csa->vif;
    struct aml_hw *aml_hw = vif->aml_hw;
    AML_INFO("status:%d", csa->status);

    if (!(csa->status)) {
        if (aml_scc_change_beacon_ht_ie(vif->aml_hw->wiphy, vif->ndev, csa->chandef)) {
            AML_INFO("bcn change ht ie fail\n");
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
        mutex_lock(&vif->wdev.wiphy->mtx);
        __acquire(&vif->wdev.wiphy->mtx);
#else
        mutex_lock(&vif->wdev.mtx);
        __acquire(&vif->wdev.mtx);
#endif
        spin_lock_bh(&aml_hw->cb_lock);
        aml_chanctx_unlink(vif);
        aml_chanctx_link(vif, csa->ch_idx, &csa->chandef);

        if (aml_hw->cur_chanctx == csa->ch_idx) {
            aml_radar_detection_enable_on_cur_channel(aml_hw);
            aml_txq_vif_start(vif, AML_TXQ_STOP_CHAN);
        }
        else {
            aml_txq_vif_stop(vif, AML_TXQ_STOP_CHAN);
        }

        spin_unlock_bh(&aml_hw->cb_lock);
        aml_cfg80211_ch_switch_notify(vif->ndev, &csa->chandef, 0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
        mutex_unlock(&vif->wdev.wiphy->mtx);
        __release(&vif->wdev.wiphy->mtx);
#else
        mutex_unlock(&vif->wdev.mtx);
        __release(&vif->wdev.mtx);
#endif
    }

    kfree(csa);
    vif->ap.csa = NULL;
}

static int aml_csa_send_action(struct aml_hw *aml_hw, struct aml_vif *aml_vif, struct aml_sta *sta, struct cfg80211_chan_def chan_def)
{
    struct sk_buff *skb;
    int ret = 0;
    const u8 *peer = sta->mac_addr;
    struct aml_csa_data *csa_data_p = NULL;
    u64 cookie;
    struct cfg80211_mgmt_tx_params params = {0,};
    u8 operating_class = 0;
    ieee80211_chandef_to_operating_class(&chan_def, &operating_class);
    skb = netdev_alloc_skb(aml_vif->ndev, sizeof(struct aml_csa_data));
    csa_data_p = (struct aml_csa_data *)skb_put(skb, sizeof(struct aml_csa_data));
    csa_data_p->mac_header.frame_ctl = IEEE80211_STYPE_ACTION;
    csa_data_p->mac_header.duration_id = 0;
    memcpy(csa_data_p->mac_header.da, peer, MAC_ADDR_LEN);
    memcpy(csa_data_p->mac_header.sa, aml_vif->ndev->dev_addr, MAC_ADDR_LEN);
    memcpy(csa_data_p->mac_header.bssid, aml_vif->ndev->dev_addr, MAC_ADDR_LEN);
    csa_data_p->extend_csa.category = WLAN_CATEGORY_PUBLIC;
    csa_data_p->extend_csa.action_code = WLAN_PUB_ACTION_EXT_CHANSW_ANN; //ACT_PUBLIC_EXT_CHL_SWITCH
    csa_data_p->extend_csa.info[0] = CSA_BLOCK_TX;
    csa_data_p->extend_csa.info[1] = operating_class;
    csa_data_p->extend_csa.info[2] = ieee80211_frequency_to_channel(chan_def.chan->center_freq);
    csa_data_p->extend_csa.info[3] = CSA_COUNT;
    csa_data_p->csa_wrapper.ie = WLAN_EID_WIDE_BW_CHANNEL_SWITCH;
    csa_data_p->csa_wrapper.len = 3;

    /* New channel width */
    switch (chan_def.width) {
        case NL80211_CHAN_WIDTH_80:
            csa_data_p->csa_wrapper.info[0] = IEEE80211_VHT_CHANWIDTH_80MHZ;
            break;

        case NL80211_CHAN_WIDTH_160:
            csa_data_p->csa_wrapper.info[0] = IEEE80211_VHT_CHANWIDTH_160MHZ;
            break;

        case NL80211_CHAN_WIDTH_80P80:
            csa_data_p->csa_wrapper.info[0] = IEEE80211_VHT_CHANWIDTH_80P80MHZ;
            break;

        default:
            csa_data_p->csa_wrapper.info[0] = IEEE80211_VHT_CHANWIDTH_USE_HT;
    }

    /* new center frequency segment 0 */
    csa_data_p->csa_wrapper.info[1] = ieee80211_frequency_to_channel(chan_def.center_freq1);

    /* new center frequency segment 1 */
    if (chan_def.center_freq2) {
        csa_data_p->csa_wrapper.info[2] = ieee80211_frequency_to_channel(chan_def.center_freq2);
    }
    else {
        csa_data_p->csa_wrapper.info[2] = 0;
    }

    params.len = skb->len;
    params.buf = skb->data;
    ret = aml_start_mgmt_xmit(aml_vif, sta, &params, false, &cookie);
    return ret;
}

/**
 * This function is call when scc conflict detect
 * Build csa beacon and initiate channel-switch procedure.
 * When fw finish csa procedure, aml_scc_csa_finish will be called
 * Return false when csa procedure init fail
 */
static int aml_scc_channel_switch(struct aml_hw *aml_hw, struct aml_vif *vif, struct cfg80211_chan_def chan_def)
{
    struct aml_ipc_buf buf = {0};
    struct aml_bcn *bcn;
    struct aml_csa *csa;
    u16 csa_oft[BCN_MAX_CSA_CPT];//csa offset,fw use to change csa cnt
    u8 *bcn_buf;
    int error = 0;
    unsigned int addr;
    u32 idx = 0;
    u8 *pos = NULL;
    u8 operating_class = 0;
    AML_DBG(AML_FN_ENTRY_STR);

    if (vif->ap.csa) {
        AML_INFO("another csa procedure is already exit");
        return -ENOMEM;
    }

    bcn = &vif->ap.bcn;

    if (scc_csa_bcn == NULL) {
        scc_csa_bcn = kmalloc(AML_SCC_FRMBUF_MAXLEN, GFP_KERNEL);

        if (!scc_csa_bcn) {
            AML_INFO("csa bcn alloc fail");
            return -ENOMEM;
        }
    }

    memcpy(scc_csa_bcn, bcn_save, bcn->len);
    bcn_buf = scc_csa_bcn;
    pos = &scc_csa_bcn[bcn->len];
    *(pos + idx++) = WLAN_EID_CHANNEL_SWITCH; //ie
    *(pos + idx++) = 3; //len
    *(pos + idx++) = CSA_BLOCK_TX;//block tx
    *(pos + idx++) = ieee80211_frequency_to_channel(chan_def.chan->center_freq);//new chan no
    *(pos + idx++) = CSA_COUNT;//count
    *(pos + idx++) = WLAN_EID_EXT_CHANSWITCH_ANN; //ie
    *(pos + idx++) = 4; //len
    *(pos + idx++) = CSA_BLOCK_TX;//block tx
    ieee80211_chandef_to_operating_class(&chan_def, &operating_class);
    *(pos + idx++) = operating_class;//new operating class
    *(pos + idx++) = ieee80211_frequency_to_channel(chan_def.chan->center_freq);//chan no
    *(pos + idx++) = CSA_COUNT;//count

    /*add second channel offset ie for 40M width*/
    if (chan_def.width == NL80211_CHAN_WIDTH_40) {
        *(pos + idx++) = WLAN_EID_SECONDARY_CHANNEL_OFFSET; //ie
        *(pos + idx++) = 1; //len

        if (chan_def.chan->center_freq < chan_def.center_freq1) {
            *(pos + idx++) = IEEE80211_HT_PARAM_CHA_SEC_ABOVE;
        }
        else {
            *(pos + idx++) = IEEE80211_HT_PARAM_CHA_SEC_BELOW;
        }
    }

    if ((chan_def.width == NL80211_CHAN_WIDTH_80) || (chan_def.width == NL80211_CHAN_WIDTH_80P80) || (chan_def.width == NL80211_CHAN_WIDTH_160)) {
        *(pos + idx++) = WLAN_EID_CHANNEL_SWITCH_WRAPPER; //ie
        *(pos + idx++) = 5; //len
        *(pos + idx++) = WLAN_EID_WIDE_BW_CHANNEL_SWITCH;   /* EID */
        *(pos + idx++) = 3;                 /* IE length */

        /* New channel width */
        switch (chan_def.width) {
            case NL80211_CHAN_WIDTH_80:
                *(pos + idx++) = IEEE80211_VHT_CHANWIDTH_80MHZ;
                break;

            case NL80211_CHAN_WIDTH_160:
                *(pos + idx++) = IEEE80211_VHT_CHANWIDTH_160MHZ;
                break;

            case NL80211_CHAN_WIDTH_80P80:
                *(pos + idx++) = IEEE80211_VHT_CHANWIDTH_80P80MHZ;
                break;

            default:
                *(pos + idx++) = IEEE80211_VHT_CHANWIDTH_USE_HT;
        }

        /* new center frequency segment 0 */
        *(pos + idx++) = ieee80211_frequency_to_channel(chan_def.center_freq1);

        /* new center frequency segment 1 */
        if (chan_def.center_freq2) {
            *(pos + idx++) = ieee80211_frequency_to_channel(chan_def.center_freq2);
        }
        else {
            *(pos + idx++) = 0;
        }
    }

    memset(csa_oft, 0, sizeof(csa_oft));
    csa_oft[0] = bcn->len + 4;
    csa_oft[1] = bcn->len + 10;
    AML_INFO("vif_type:%d, bcn_len:%d, tim_len:%d, idx:%d, csa_oft[0]:%d, csa_oft[1]:%d",
             AML_VIF_TYPE(vif), (int)bcn->len, (int)bcn->tim_len, idx, csa_oft[0], csa_oft[1]);

    if (aml_bus_type == PCIE_MODE) {
        if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, bcn_buf, bcn->len + idx))) {
            AML_INFO("ipc init fail");
            return error;
        }
    }
    else if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len + idx, USB_EP4);
    }
    else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len + idx);
    }

    csa = kzalloc(sizeof(struct aml_csa), GFP_KERNEL);

    if (!csa) {
        AML_INFO("csa alloc fail");
        return -ENOMEM;
    }

    memset(csa, 0, sizeof(struct aml_csa));
    vif->ap.csa = csa;
    csa->vif = vif;
    csa->chandef = chan_def;
    /* Send new Beacon. FW will extract channel and count from the beacon */
    error = aml_send_bcn_change(aml_hw, vif->vif_index, buf.dma_addr, bcn->len + idx, bcn->head_len, bcn->tim_len, csa_oft);

    if (error) {
        kfree(csa);
        vif->ap.csa = NULL;
        AML_INFO("bcn send fail");
    }
    else {
        INIT_WORK(&csa->work, aml_scc_csa_finish);
#ifndef CONFIG_PT_MODE
        aml_cfg80211_ch_switch_started_notify(vif->ndev, &csa->chandef, 0, CSA_COUNT, CSA_BLOCK_TX);
#endif
    }

    if (aml_bus_type == PCIE_MODE) {
        if (buf.addr) {
            dma_unmap_single(aml_hw->dev, buf.dma_addr, buf.size, DMA_TO_DEVICE);
        }
    }

    /* coverity[leaked_storage] - csa have added to list */
    return error;
}

/**
 * This function is called when start softap or sta_mode get ip success
 *
 */
int aml_scc_check_chan_conflict(struct aml_hw *aml_hw)
{
    struct aml_vif *vif;
    list_for_each_entry(vif, &aml_hw->vifs, list) {
        switch (AML_VIF_TYPE(vif)) {
            case NL80211_IFTYPE_AP:
            case NL80211_IFTYPE_P2P_GO: {
                u8 target_vif_idx = 0;
                struct mm_scc_cfm scc_cfm;
                struct aml_vif *target_vif ;
                struct cfg80211_chan_def target_chdef;
                struct cfg80211_chan_def cur_chdef;
#ifdef CONFIG_AML_RECOVERY

                if (aml_recy_flags_chk(AML_RECY_STOP_AP_PROC)) {
                    AML_INFO("proc stop ap,so not need to check");
                    break;
                }

#endif
                target_vif_idx = aml_scc_get_conflict_vif_idx(vif);
                AML_INFO("target_idx:%d, sta_list:%d, use_csa:%d", target_vif_idx, list_empty(&vif->ap.sta_list), scc_use_csa);

                if (target_vif_idx == 0xff) {
                    break;
                }

                target_vif = aml_hw->vif_table[target_vif_idx];
                target_chdef = vif->aml_hw->chanctx_table[target_vif->ch_index].chan_def;
                cur_chdef = vif->aml_hw->chanctx_table[vif->ch_index].chan_def;

                if (target_vif->ch_index >= NX_CHAN_CTXT_CNT) {
                    AML_INFO("target ch_index invalid:%d", target_vif->ch_index);
                    break;
                }

                if (target_chdef.chan == NULL) {
                    AML_INFO("target chdef is null");
                    break;
                }

                if ((aml_scc_get_init_band() != target_chdef.chan->band) && (AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP)) {
                    AML_INFO("init band conflict with target band [%d %d]\n", aml_scc_get_init_band(), target_chdef.chan->band);
                    break;
                }

                if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_GO) {
                    AML_INFO("GO mode do not switch scc\n");
                    break;
                }

                if (target_chdef.chan->flags & IEEE80211_CHAN_RADAR) {
                    AML_INFO("target is radar chan");
                    break;
                }

                AML_INFO("chan %d,bw:%s --> chan %d,bw:%s ",
                         aml_ieee80211_freq_to_chan(cur_chdef.chan->center_freq, cur_chdef.chan->band),
                         chan_width_trace[cur_chdef.width],
                         aml_ieee80211_freq_to_chan(target_chdef.chan->center_freq, target_chdef.chan->band),
                         chan_width_trace[target_chdef.width]);

                if (!(list_empty(&vif->ap.sta_list)) && scc_use_csa) {
                    struct aml_sta *sta, *tmp;
                    list_for_each_entry_safe(sta, tmp, &vif->ap.sta_list, list) {
                        if (sta->valid) {
                            int ret;
                            aml_ps_bh_enable(aml_hw, sta, 1);//we always consider peer is sleep
                            ret = aml_csa_send_action(aml_hw, vif, sta, target_chdef);
                            AML_INFO("send csa action to :%pM,ret:%d", sta->mac_addr, ret);
                        }
                    }
                    aml_scc_channel_switch(aml_hw, vif, target_chdef);
                }
                else {
                    if (!aml_scc_change_beacon_ht_ie(vif->aml_hw->wiphy, vif->ndev, target_chdef)) {
                        if (aml_send_scc_conflict_notify(vif, target_vif_idx, &scc_cfm) == 0) {//notify fw
                            if (scc_cfm.status == CO_OK) {
                                aml_chanctx_unlink(vif);
                                aml_chanctx_link(vif, target_vif->ch_index, &target_chdef);
                                aml_cfg80211_ch_switch_notify(vif->ndev, &target_chdef, 0);

                                if (vif->aml_hw->cur_chanctx == target_vif->ch_index)
                                    aml_txq_vif_start(vif, AML_TXQ_STOP_CHAN);
                                else
                                    aml_txq_vif_stop(vif, AML_TXQ_STOP_CHAN);

                                AML_SCC_BEACON_SET_STATUS(BEACON_UPDATE_WAIT_PROBE);
                            }

                            AML_INFO("scc_cfm.status:%d", scc_cfm.status);
                        }
                    }
                    else {
                        AML_INFO("bcn change ht ie fail \n");
                    }
                }

                break;
            }

            default:
                break;
        }
    }
    return 0;
}

void aml_scc_init(void)
{
    scc_bcn_buf = NULL;
    scc_csa_bcn = NULL;
}

void aml_scc_deinit(void)
{
    if (scc_bcn_buf) {
        kfree(scc_bcn_buf);
        scc_bcn_buf = NULL;
    }

    if (scc_csa_bcn) {
        kfree(scc_csa_bcn);
        scc_csa_bcn = NULL;
    }

    AML_SCC_CLEAR_BEACON_UPDATE();
}

void aml_check_scc(void)
{
#ifdef CONFIG_AML_RECOVERY
    aml_wq_do(aml_scc_check_chan_conflict, aml_recy->aml_hw);
#endif
}

void aml_scc_p2p_action_restore(u8 *buf, u32 *len_diff)
{
    if (g_scc_p2p_len_before) {
        memcpy(buf, g_scc_p2p_save, g_scc_p2p_len_before);
        AML_INFO("[P2P SCC] len %d -- > %d", *len_diff, *len_diff + g_scc_p2p_len_diff);
        *len_diff = *len_diff + g_scc_p2p_len_diff;
        g_scc_p2p_len_before = 0;
    }
}

