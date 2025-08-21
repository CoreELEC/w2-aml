/**
****************************************************************************************
*
* @file aml_recy.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief Recovery Function Implementation.
*
****************************************************************************************
*/

#include <linux/list.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <net/cfg80211.h>

#include "aml_wq.h"
#include "aml_recy.h"
#include "aml_cmds.h"
#include "aml_defs.h"
#include "aml_utils.h"
#include "aml_msg_tx.h"
#include "aml_platform.h"
#include "aml_main.h"
#include "aml_scc.h"
#include "aml_compat.h"

#include "reg_access.h"
#include "wifi_intf_addr.h"
#include "chip_pmu_reg.h"
#include "wifi_top_addr.h"


#ifdef CONFIG_AML_RECOVERY

struct aml_recy *aml_recy = NULL;
static int recy_dbg = 1;

extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_pm_type g_wifi_pm;

extern int g_cali_cfg_done;
extern void aml_usb_reset(void);

static const char *const aml_recy_reason_code2str[RECY_REASON_CODE_MAX] = {
    [RECY_REASON_CODE_CMD_CRASH]       = "RECY_REASON_CODE_CMD_CRASH",
    [RECY_REASON_CODE_FW_LINKLOSS]     = "RECY_REASON_CODE_FW_LINKLOSS",
    [RECY_REASON_CODE_BUS_ERR]         = "RECY_REASON_CODE_BUS_ERR",
    [RECY_REASON_CODE_TX_TIMEOUT]      = "RECY_REASON_CODE_TX_PKTS_TIMEOUT",
};

void aml_recy_flags_set(u32 flags)
{
    aml_recy->flags |= flags;
    RECY_DBG("set flags(0x%0x), new flags(0x%0x)", flags, aml_recy->flags);
}

void aml_recy_flags_clr(u32 flags)
{
    aml_recy->flags &= ~(flags);
    RECY_DBG("clr flags(0x%0x), new flags(0x%0x)", flags, aml_recy->flags);
}

bool aml_recy_flags_chk(u32 flags)
{
    //RECY_DBG("chk flags(0x%0x), original flags(0x%0x)", flags, aml_recy->flags);
    return (!!(aml_recy->flags & flags));
}

void aml_recy_save_assoc_info(struct cfg80211_connect_params *sme, u8 vif_index)
{
    uint8_t bcst_bssid[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t *pos;
    int ssid_len;
    int ies_len;

    if (!sme || !sme->ie || !sme->ie_len || !sme->ssid || !sme->ssid_len)
        return;

    RECY_DBG("save assoc info");

    if (sme->bssid) {
        memcpy(&aml_recy->assoc_info.bssid, sme->bssid, ETH_ALEN);
    } else {
        memcpy(&aml_recy->assoc_info.bssid, bcst_bssid, ETH_ALEN);
    }
    if (sme->prev_bssid) {
        memcpy(&aml_recy->assoc_info.prev_bssid, sme->prev_bssid, ETH_ALEN);
    }
    memcpy(&aml_recy->assoc_info.crypto,
            &sme->crypto, sizeof(struct cfg80211_crypto_settings));
    aml_recy->assoc_info.auth_type = sme->auth_type;
    aml_recy->assoc_info.mfp = sme->mfp;
    aml_recy->assoc_info.key_idx = sme->key_idx;
    aml_recy->assoc_info.key_len = sme->key_len;

#define aml_recy_memcpy(dst, src, len) do { \
    if (src && len) { \
        if (dst && (sizeof(*dst) != len)) { kfree(dst); dst = NULL; } \
        if (!dst) { dst = kmalloc(len, GFP_KERNEL); } \
        if (!dst) { RECY_DBG("kmalloc failed"); return; } \
        memcpy(dst, src, len); \
    } \
} while (0);

    if (sme->channel)
        RECY_DBG("assoc info chan bw=%d, center freq=%d",
                sme->channel->band, sme->channel->center_freq);
    aml_recy_memcpy(aml_recy->assoc_info.chan,
            sme->channel, sizeof(struct ieee80211_channel));
    if (sme->key && sme->key_len)
        RECY_DBG("assoc info key_len=%d", sme->key_len);
    aml_recy_memcpy(aml_recy->assoc_info.key_buf, sme->key, sme->key_len);
#undef aml_recy_memcpy

    ssid_len = (sme->ssid_len > MAC_SSID_LEN) ? MAC_SSID_LEN : sme->ssid_len;
    ies_len = sme->ie_len + ssid_len + 2;
    if (!aml_recy->assoc_info.ies_buf) {
        aml_recy->assoc_info.ies_buf = kmalloc(ies_len, GFP_KERNEL);
        if (!aml_recy->assoc_info.ies_buf) {
            RECY_DBG("kmalloc ies buf failed");
            return;
        }
    } else if (aml_recy->assoc_info.ies_len < ies_len) {
        kfree(aml_recy->assoc_info.ies_buf);
        aml_recy->assoc_info.ies_buf = kmalloc(ies_len, GFP_KERNEL);
        if (!aml_recy->assoc_info.ies_buf) {
            RECY_DBG("kmalloc ies buf failed");
            return;
        }
    }
    pos = aml_recy->assoc_info.ies_buf;
    *pos++ = WLAN_EID_SSID;
    *pos++ = ssid_len;
    memcpy(pos, sme->ssid, ssid_len);
    pos += ssid_len;
    memcpy(pos, sme->ie, sme->ie_len);
    aml_recy->assoc_info.ies_len = ies_len;
    aml_recy->assoc_info.vif_idx = vif_index;
    aml_recy_flags_set(AML_RECY_ASSOC_INFO_SAVED);
}

void aml_recy_save_ap_info(struct cfg80211_ap_settings *settings)
{
    if (!settings || !settings->chandef.chan) {
        RECY_DBG("settings or chandef.chan is null");
        return;
    }

    RECY_DBG("save ap info");
    aml_recy->ap_info.band = settings->chandef.chan->band;
    cfg80211_to_aml_chan(&settings->chandef, &aml_recy->ap_info.chan);
    memcpy(aml_recy->ap_info.settings, settings, sizeof(struct cfg80211_ap_settings));
    aml_recy_flags_set(AML_RECY_AP_INFO_SAVED);
}

int aml_recy_sta_connect(struct aml_hw *aml_hw, uint8_t *status)
{
    struct aml_vif *aml_vif;
    struct cfg80211_connect_params sme;
    struct sm_connect_cfm cfm;
    int ret = 0;

    /* if no connection or not station mode, do nothing */
    if (!aml_hw || !(aml_vif = aml_hw->vif_table[0]) || !(aml_vif->ndev)
        || !(aml_recy->flags & AML_RECY_ASSOC_INFO_SAVED)
        || (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_STATION))
        return ret;

    /* check aml_recy assoc info pointers legality */
    if (!aml_recy->assoc_info.chan) {
        RECY_DBG("check chan failed");
        return ret;
    }
    if (!aml_recy->assoc_info.ies_buf) {
        RECY_DBG("check ies_buf failed");
        return ret;
    }

    RECY_DBG("sta connect start");

    if (aml_hw->scan_request) {
        aml_scan_abort(aml_hw);
    }

    memset(&sme, 0, sizeof(sme));
    sme.bssid = aml_recy->assoc_info.bssid;
    sme.prev_bssid = aml_recy->assoc_info.prev_bssid;
    sme.channel = aml_recy->assoc_info.chan;
    memcpy(&sme.crypto, &aml_recy->assoc_info.crypto, sizeof(struct cfg80211_crypto_settings));
    sme.auth_type = aml_recy->assoc_info.auth_type;
    sme.mfp = aml_recy->assoc_info.mfp;

    /* recovery WEP key for shared-key authentication */
    sme.key_idx = aml_recy->assoc_info.key_idx;
    sme.key_len = aml_recy->assoc_info.key_len;
    sme.key = aml_recy->assoc_info.key_buf;

    /* install key for shared-key authentication */
    if (!(sme.crypto.wpa_versions & (NL80211_WPA_VERSION_1
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,0,0)
            | NL80211_WPA_VERSION_2 | NL80211_WPA_VERSION_3))
#else
            | NL80211_WPA_VERSION_2))
#endif
        && sme.key && sme.key_len
        && ((sme.auth_type == NL80211_AUTHTYPE_SHARED_KEY)
            || (sme.auth_type == NL80211_AUTHTYPE_AUTOMATIC)
            || (sme.crypto.n_ciphers_pairwise &
            (WLAN_CIPHER_SUITE_WEP40 | WLAN_CIPHER_SUITE_WEP104)))) {
        struct key_params key_params;

        if (sme.auth_type == NL80211_AUTHTYPE_AUTOMATIC) {
            sme.auth_type = NL80211_AUTHTYPE_SHARED_KEY;
        }
        key_params.key = sme.key;
        key_params.seq = NULL;
        key_params.key_len = sme.key_len;
        key_params.seq_len = 0;
        key_params.cipher = sme.crypto.cipher_group;
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
        aml_cfg80211_add_key(aml_hw->wiphy, aml_vif->ndev,
                0, sme.key_idx, false, NULL, &key_params);
#else
        aml_cfg80211_add_key(aml_hw->wiphy, aml_vif->ndev,
                sme.key_idx, false, NULL, &key_params);
#endif
    }

    sme.ssid_len = aml_recy->assoc_info.ies_buf[1];
    sme.ssid = &aml_recy->assoc_info.ies_buf[2];
    sme.ie = &aml_recy->assoc_info.ies_buf[2 + sme.ssid_len];
    sme.ie_len = aml_recy->assoc_info.ies_len - (2 + sme.ssid_len);

    ret = aml_send_sm_connect_req(aml_hw, aml_vif, &sme, &cfm);
    if (ret) {
        RECY_DBG("sta connect cmd failed");
    } else if (status != NULL) {
        *status = cfm.status;
    }
    return ret;
}

extern int aml_sdio_platform_on(struct aml_hw *aml_hw, void *config);
extern void aml_platform_off(struct aml_hw *aml_hw, void **config);
extern void extern_wifi_set_enable(int is_on);
extern unsigned char g_usb_after_probe;
extern void aml_sdio_reset(void);
extern struct usb_device *g_udev;
int aml_recy_fw_reload_for_usb_sdio(struct aml_hw *aml_hw)
{
    int ret = 0;
    int try_cnt = 0;

    RECY_DBG("reload fw start");

    aml_recy_flags_set(AML_RECY_FW_ONGOING | AML_RECY_IPC_ONGOING);
    bus_state_detect.is_recy_ongoing = 1;
Try_again:

    aml_platform_off(aml_hw, NULL);
    if (aml_bus_type != PCIE_MODE) {
        aml_clear_reorder_list();
    }
    if (aml_bus_type == USB_MODE) {
        bus_state_detect.bus_reset_ongoing = 1;
        aml_usb_reset();

        /* realloc usb_dev in function@auc_probe when usb do reset, it need to reinit data */
        aml_hw->plat->usb_dev = g_udev;
        dev_set_drvdata(&aml_hw->plat->usb_dev->dev, aml_hw);
        aml_hw->dev = aml_platform_get_dev(aml_hw->plat);
        set_wiphy_dev(aml_hw->wiphy, aml_hw->dev);
        bus_state_detect.bus_reset_ongoing = 0;
    }
    if ((aml_bus_type == SDIO_MODE) && ((bus_state_detect.bus_err == 1) || (try_cnt > 1))) {
       aml_sdio_reset();
    }
    if (aml_sdio_platform_on(aml_hw, NULL)) {
        RECY_DBG("reload fw platform on failed");
        ret = -1;
        goto out;
    }
    if (aml_send_reset(aml_hw)) {
        RECY_DBG("send reset msg failed, reload fw failed");
        ret = -1;
        goto out;
    }
    if (aml_send_me_config_req(aml_hw)) {
        RECY_DBG("send me config msg failed, reload fw failed");
        ret = -1;
        goto out;
    }
    if (aml_send_me_chan_config_req(aml_hw)) {
        RECY_DBG("send me chan config msg failed, reload fw failed");
        ret = -1;
        goto out;
    }
    if ((ret = aml_send_start(aml_hw))) {
        RECY_DBG("reload fw failed");
        ret = -1;
        goto out;
    }
    RECY_DBG("recy fw reload success!!!\n");
out:
#ifndef CONFIG_PT_MODE
    if (ret && try_cnt < 3) {
        try_cnt++;
        ret = 0;
        RECY_DBG("fw reload fail, try again(%d)\n", try_cnt);
        goto Try_again;
    }
#endif
    bus_state_detect.is_recy_ongoing = 0;
    aml_recy_flags_clr(AML_RECY_FW_ONGOING);
    return ret;
}

static int aml_recy_fw_reload_for_pcie(struct aml_hw *aml_hw)
{
    struct aml_plat *aml_plat = aml_hw->plat;
    unsigned int mac_clk_reg;
    int ret;
    u32 regval;

    if (!aml_hw->plat->enabled)
        return 0;

    RECY_DBG("reload fw start");

    aml_recy_flags_set(AML_RECY_FW_ONGOING|AML_RECY_IPC_ONGOING);

    aml_ipc_stop(aml_hw);

    //disable agc
    regval = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, AML_AGCCNTL_ADDR);
    regval |= (1 << 12);
    AML_REG_WRITE(regval, aml_plat, AML_ADDR_SYSTEM, AML_AGCCNTL_ADDR);
    msleep(150);

    if (aml_hw->plat->disable)
        aml_hw->plat->disable(aml_hw);

    aml_ipc_deinit(aml_hw);
    aml_platform_reset(aml_hw->plat);
    regval = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, NXMAC_MAC_CNTRL_2_ADDR);
    AML_REG_WRITE(regval | NXMAC_SOFT_RESET_BIT, aml_plat, AML_ADDR_SYSTEM, NXMAC_MAC_CNTRL_2_ADDR);
    aml_hw->plat->enabled = false;

    /* reset mac/phy */
    AML_REG_WRITE(0x00070000 | PHY_RESET | MAC_RESET | CPU_RESET | USB_RESET | SDIO_RESET, aml_plat, AML_ADDR_AON, RG_PMU_A22);
    AML_REG_WRITE(0x00070000 | CPU_RESET, aml_plat, AML_ADDR_AON, RG_PMU_A22);

    aml_plat_mpif_sel(aml_plat);
    aml_hw->phy.cnt = 1;
    aml_hw->rxbuf_idx = 0;
    aml_tx_rx_buf_init(aml_hw);

    aml_plat_lmac_load(aml_plat);

    aml_ipc_init(aml_hw, (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_PCI_START_ADDR),
            (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_HOST_RXBUF_ADDR),
            (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_HOST_RXDESC_ADDR));
    if ((ret = aml_plat->enable(aml_hw)))
        return 0;

    AML_REG_WRITE(CPU_CLK_VALUE, aml_plat, AML_ADDR_MAC_PHY, CPU_CLK_REG_ADDR);
    /* change mac clock to 240M */
    mac_clk_reg = AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);
    mac_clk_reg |= 0x30000;
    AML_REG_WRITE(mac_clk_reg, aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);

    /* start firmware */
    AML_REG_WRITE(0x00070000 | USB_RESET | SDIO_RESET, aml_plat, AML_ADDR_AON, RG_PMU_A22);

    //check W2 if fw is ready
    aml_get_vid(aml_plat);

    aml_ipc_start(aml_hw);
    aml_plat->enabled = true;
    aml_recy_flags_clr(AML_RECY_IPC_ONGOING);

    aml_send_reset(aml_hw);
    aml_send_me_config_req(aml_hw);
    aml_send_me_chan_config_req(aml_hw);

    if ((ret = aml_send_start(aml_hw))) {
        RECY_DBG("reload fw failed");
        return -1;
    }

    aml_recy_flags_clr(AML_RECY_FW_ONGOING);

    return ret;
}

static int aml_recy_fw_reload(struct aml_hw *aml_hw)
{
    int ret = 0;

    if (aml_bus_type != PCIE_MODE) {
        ret = aml_recy_fw_reload_for_usb_sdio(aml_hw);
    } else {
        ret = aml_recy_fw_reload_for_pcie(aml_hw);
    }

    return ret;
}

static int aml_recy_vif_reset(struct aml_hw *aml_hw)
{
    struct aml_vif *aml_vif;
    struct net_device *dev;
    int i;

    if (aml_hw->scan_request) {
        RECY_DBG("scan abort");
        aml_scan_abort(aml_hw);
    }

    for (i = 0; i < NX_VIRT_DEV_MAX; i++) {
        aml_vif = aml_hw->vif_table[i];
        if (!aml_vif) {
            continue;
        }
        RECY_DBG("reset vif(%d) interface", i);
        dev = aml_vif->ndev;
        if (!dev) {
            RECY_DBG("retrieve vif or dev failed, dev is null");
            return -1;
        }

        spin_lock_bh(&aml_hw->cb_lock);
        aml_chanctx_unlink(aml_vif);
        spin_unlock_bh(&aml_hw->cb_lock);

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) {
            /* delete any remaining STA before stopping the AP which aims to
             * flush pending TX buffers */
            while (!list_empty(&aml_vif->ap.sta_list)) {
                aml_cfg80211_del_station(aml_hw->wiphy, dev, NULL);
            }
        }

        aml_radar_cancel_cac(&aml_hw->radar);

        spin_lock_bh(&aml_hw->roc_lock);
        if (aml_hw->roc && (aml_hw->roc->vif == aml_vif)) {
            kfree(aml_hw->roc);
            aml_hw->roc = NULL;
        }
        spin_unlock_bh(&aml_hw->roc_lock);

        spin_lock_bh(&aml_hw->cb_lock);
        aml_vif->up = false;
        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION ||
            AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT)
            aml_connect_flags_clr(aml_vif, AML_GETTING_IP);

        if (netif_carrier_ok(dev)) {
            if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION ||
                AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) {
                netif_tx_stop_all_queues(dev);
                netif_carrier_off(dev);
                if (aml_vif->sta.ap) {
                    aml_txq_sta_deinit(aml_hw, aml_vif->sta.ap);
                    aml_txq_tdls_vif_deinit(aml_vif);
                    aml_dbgfs_unregister_sta(aml_hw, aml_vif->sta.ap);
                }
            } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN) {
                netif_carrier_off(dev);
            } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP ||
                    AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO) {
                netif_tx_stop_all_queues(dev);
                netif_carrier_off(dev);
                aml_txq_vif_deinit(aml_hw, aml_vif);
                aml_scc_deinit();

            }
        }
        spin_unlock_bh(&aml_hw->cb_lock);

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MONITOR)
            aml_hw->monitor_vif = AML_INVALID_VIF;
    }

    for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
        struct aml_sta *sta = aml_hw->sta_table + i;
        if (sta)
        {
            sta->valid = false;
        }
    }
    g_cali_cfg_done = 0;

    return 0;
}

static int aml_recy_vif_restart(struct aml_hw *aml_hw)
{
    struct aml_vif *aml_vif;
    struct net_device *dev;
    struct mm_add_if_cfm cfm;
    uint8_t i, status;
    int err = 0;

    for (i = 0; i < NX_VIRT_DEV_MAX; i++) {
        aml_vif = aml_hw->vif_table[i];
        if (!aml_vif) {
            continue;
        }
        RECY_DBG("restart vif(%d) interface", i);
        dev = aml_vif->ndev;
        if (!dev) {
            RECY_DBG("retrieve vif or dev failed, dev is null");
            return -1;
        }

        err = aml_send_add_if(aml_hw, dev->dev_addr, AML_VIF_TYPE(aml_vif), false, &cfm);
        if (err || (cfm.status != 0)) {
            RECY_DBG("add interface %d failed, err: %d, status: %d\n", i, err, cfm.status);
            return -1;
        }
        aml_config_cali_param(aml_hw);

        spin_lock_bh(&aml_hw->cb_lock);
        aml_vif->vif_index = cfm.inst_nbr;
        aml_vif->up = true;
        aml_hw->vif_table[cfm.inst_nbr] = aml_vif;
        aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
        spin_unlock_bh(&aml_hw->cb_lock);

        /* should keep STA operations before AP mode */
        if ((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION) &&
                (aml_recy->flags & AML_RECY_ASSOC_INFO_SAVED)) {
            err = aml_recy_sta_connect(aml_hw, &status);
            if (err || status) {
                RECY_DBG("sta connect failed");
                return -1;
            }
            aml_recy->reconnect_rest = AML_RECY_RECONNECT_TIMES;
            aml_recy_flags_set(AML_RECY_CHECK_SCC);
        }

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) {
            RECY_DBG("gc recovery");
            cfg80211_disconnected(dev, WLAN_REASON_UNSPECIFIED, NULL, 0, 1, GFP_ATOMIC);
        }

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO) {
            RECY_DBG("go recovery");
            aml_recy_flags_set(AML_RECY_GO_ONGOING);
            while (!list_empty(&aml_vif->ap.sta_list)) {
                aml_cfg80211_del_station(aml_hw->wiphy, dev, NULL);
            }
        }

        /* should keep AP operations after STA mode */
        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) {
            aml_cfg80211_change_iface(aml_hw->wiphy, dev, NL80211_IFTYPE_AP, NULL);
            err = aml_cfg80211_start_ap(aml_vif->aml_hw->wiphy, dev, aml_recy->ap_info.settings);
            if (err) {
                RECY_DBG("restart ap failed");
                return -1;
            }
            /* delete any remaining STA to let it reconnect */
            while (!list_empty(&aml_vif->ap.sta_list)) {
                aml_cfg80211_del_station(aml_hw->wiphy, dev, NULL);
            }
        }
    }

    return 0;
}

int aml_recy_doit(struct aml_hw *aml_hw)
{
    int ret;
    u32 flags = AML_RECY_STOP_AP_PROC
                | AML_RECY_OPEN_VIF_PROC
                | AML_RECY_STATE_ONGOING
                | AML_RECY_CLOSE_VIF_PROC;
    unsigned char fbuf[64] = {0};

    scnprintf(fbuf, sizeof(fbuf), "recovery reason: 0x%02x(%s)\n", aml_recy->reason, aml_recy_reason_code2str[aml_recy->reason]);
    AML_INFO("%s", fbuf);
    if (aml_bus_type != PCIE_MODE)
#ifdef CONFIG_AML_DEBUGFS
        aml_send_err_info_to_diag(fbuf, strlen(fbuf));
#endif

    if (aml_recy_flags_chk(flags)) {
        RECY_DBG("recy delay by flags: 0x%x\n", aml_recy->flags);
        if ((aml_bus_type != PCIE_MODE) && (bus_state_detect.bus_err == 1)) {
            bus_state_detect.bus_reset_ongoing = 0;
        }
        return 0;
    }

    aml_recy_flags_set(AML_RECY_STATE_ONGOING | AML_RECY_DROP_XMIT_PKT);
    aml_hw->traffic_busy = 0;

    ret = aml_recy_vif_reset(aml_hw);
    if (ret) {
        RECY_DBG("vif reset failed");
        goto out;
    }
    aml_recy_flags_clr(AML_RECY_DROP_XMIT_PKT);

    ret = aml_recy_fw_reload(aml_hw);
    if (ret) {
        RECY_DBG("fw reload failed");
        goto out;
    }

    ret = aml_recy_vif_restart(aml_hw);
    if (ret) {
        RECY_DBG("vif restart failed");
        goto out;
    }

out:
    aml_recy->link_loss.is_requested = 0;
    // recovery ps mode
    aml_send_me_set_ps_mode(aml_hw, aml_recy->ps_state);
    /* clear suspend state flag */
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.drv_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.is_shut_down, 0);
    aml_hw->state = WIFI_SUSPEND_STATE_NONE;
    spin_lock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
    aml_recy->reason = 0;
    spin_unlock_bh(&aml_recy->aml_hw->cmd_mgr.lock);
    aml_recy_flags_clr(AML_RECY_STATE_ONGOING | AML_RECY_DROP_XMIT_PKT);

    return ret;
}

void aml_recy_link_loss_test(void)
{
    if (!aml_recy || !aml_recy->link_loss.is_enabled)
        return;

    aml_recy->link_loss.is_requested = 1;
    aml_recy->link_loss.is_happened = 1;
    aml_recy->link_loss.scan_result_cnt = 0;
    RECY_DBG("force link loss recovery");
}

static int aml_recy_detection(void)
{
    struct aml_cmd_mgr *cmd_mgr;
    int ret = false;

    if (!aml_recy | !aml_recy->aml_hw)
        return 0;

#ifdef CONFIG_LINUXPC_VERSION
    // mutex_lock can't run in timer_cb, so pcie does only
    if (aml_bus_type == PCIE_MODE) {
        // channel is exception when pc pointer is 0xffffffff, and it can not recover now
        if (AML_REG_READ(aml_recy->aml_hw->plat, AML_ADDR_MAC_PHY, AML_FW_PC_POINTER) == 0xffffffff) {
            return 0;
        }
    }
#endif

    cmd_mgr = &aml_recy->aml_hw->cmd_mgr;
    spin_lock_bh(&cmd_mgr->lock);
    if (aml_recy->reason) {
        spin_unlock_bh(&cmd_mgr->lock);
        return false;
    }
    if ((cmd_mgr->state == AML_CMD_MGR_STATE_CRASHED)
        || (aml_recy->link_loss.is_enabled && aml_recy->link_loss.is_requested)) {
        if (cmd_mgr->state == AML_CMD_MGR_STATE_CRASHED) {
            aml_recy->reason = RECY_REASON_CODE_CMD_CRASH;
        } else {
            aml_recy->reason = RECY_REASON_CODE_FW_LINKLOSS;
        }
        ret = true;
    }
    if (aml_bus_type != PCIE_MODE) {
        if (!bus_state_detect.bus_reset_ongoing &&
            (bus_state_detect.bus_err == 1)) {
            bus_state_detect.bus_reset_ongoing = 1;
            aml_recy->reason = RECY_REASON_CODE_BUS_ERR;
            ret = true;
        }
    }
    spin_unlock_bh(&cmd_mgr->lock);

    return ret;
}

bool aml_recy_connect_retry(void)
{
    struct aml_wq *aml_wq;
    enum aml_wq_type type = AML_WQ_RECY_CONNECT_RETRY;

    aml_wq = aml_wq_alloc(1);
    if (!aml_wq) {
        RECY_DBG("alloc wq out of memory");
        return false;
    }
    aml_wq->id = AML_WQ_RECY_CONNECT_RETRY;
    memcpy(aml_wq->data, &type, 1);
    aml_wq_add(aml_recy->aml_hw, aml_wq);
    return true;
}

static void aml_recy_timer_cb(struct timer_list *t)
{
    struct aml_wq *aml_wq;
    enum aml_wq_type type = AML_WQ_RECY;
    int ret = 0;

    if ((ret = aml_recy_detection())) {
        aml_wq = aml_wq_alloc(1);
        if (!aml_wq) {
            RECY_DBG("alloc wq out of memory");
            return;
        }
        aml_wq->id = AML_WQ_RECY;
        memcpy(aml_wq->data, &type, 1);
        aml_wq_add(aml_recy->aml_hw, aml_wq);
    }
    mod_timer(&aml_recy->timer, jiffies + AML_RECY_MON_INTERVAL);
}

void aml_recy_enable(void)
{
    mod_timer(&aml_recy->timer, jiffies + AML_RECY_MON_INTERVAL);
}

void aml_recy_disable(void)
{
    del_timer_sync(&aml_recy->timer);
}

int aml_recy_init(struct aml_hw *aml_hw)
{
    RECY_DBG("recovery func init");

    aml_recy = kzalloc(sizeof(struct aml_recy), GFP_KERNEL);
    if (!aml_recy) {
        RECY_DBG("recy info alloc failed");
        return -ENOMEM;
    }
    aml_recy->ap_info.settings = kzalloc(sizeof(struct cfg80211_ap_settings), GFP_KERNEL);
    if (!aml_recy->ap_info.settings) {
        RECY_DBG("kmalloc ap settings failed");
        kfree(aml_recy);
        return -ENOMEM;
    }
    aml_recy->aml_hw = aml_hw;
    aml_recy->link_loss.is_enabled = 1;

#ifndef CONFIG_PT_MODE
    timer_setup(&aml_recy->timer, aml_recy_timer_cb, 0);
    aml_recy_enable();
#endif
    return 0;
}

int aml_recy_deinit(void)
{
    RECY_DBG("recovery func deinit");

    aml_recy_disable();

#define aml_recy_free(a) do { \
    if (a) kfree(a); \
} while (0);

    aml_recy_free(aml_recy->assoc_info.chan);
    aml_recy_free(aml_recy->assoc_info.key_buf);
    aml_recy_free(aml_recy->assoc_info.ies_buf);
    aml_recy_free(aml_recy->ap_info.settings);
    aml_recy_free(aml_recy);
#undef aml_recy_free
    aml_recy = NULL;

    return 0;
}

bool aml_recy_check_aml_vif_exit(struct aml_hw *aml_hw, struct aml_vif *aml_vif)
{
    int i = 0;
    struct aml_vif *vif;
    for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
        vif = aml_hw->vif_table[i];
        if (!vif) {
            continue;
        }
        if (aml_vif == vif) {
            return true;
        }
    }

    return false;
}
#endif
