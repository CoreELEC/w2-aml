/**
 ******************************************************************************
 *
 * @file aml_main.c
 *
 * @brief Entry point of the AML driver
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/inetdevice.h>
#include <net/cfg80211.h>
#include <net/ip.h>
#include <linux/etherdevice.h>
#include <net/addrconf.h>

#include "aml_defs.h"
#include "aml_dini.h"
#include "aml_msg_tx.h"
#include "reg_access.h"
#include "hal_desc.h"
#include "aml_debugfs.h"
#include "aml_cfg.h"
#include "aml_irqs.h"
#include "aml_radar.h"
#include "aml_version.h"
#ifdef CONFIG_AML_BFMER
#include "aml_bfmer.h"
#endif //(CONFIG_AML_BFMER)
#include "aml_tdls.h"
#include "aml_events.h"
#include "aml_compat.h"
#include "aml_iwpriv_cmds.h"
#include "fi_cmd.h"
#include "aml_main.h"
#include "aml_regdom.h"
#include "aml_android.h"
#include "share_mem_map.h"
#include "aml_prealloc.h"
#include "aml_scc.h"
#include "aml_wq.h"
#include "aml_recy.h"
#include "aml_cmds.h"
#include "aml_msg_rx.h"
#include "aml_rps.h"
#include "aml_prof.h"
#include "wifi_aon_addr.h"
#include "aml_mdns_offload.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
#include <linux/panic_notifier.h>
#endif

#define RW_DRV_DESCRIPTION  "Amlogic 11nac driver for Linux cfg80211"
#define RW_DRV_COPYRIGHT    "Copyright (C) Amlogic 2015-2021"
#define RW_DRV_AUTHOR       "Amlogic S.A.S"

#define PNO_MAX_SUPP_NETWORKS  16
struct aml_hw *g_pst_aml_hw = NULL;

#define AML_SCAN_INTERNAL_THR 12000


enum aml_intf_type {
    AML_IFTYPE_STA,
    AML_IFTYPE_P2P,
    AML_IFTYPE_SAP,
    AML_IFTYPE_MAX,
};

#define AML_IFNAME_STA  "wlan%d"
#define AML_IFNAME_P2P  "p2p%d"
#define AML_IFNAME_SAP  "ap%d"

const char *aml_intf_name[AML_IFTYPE_MAX] = {
    AML_IFNAME_STA,
    AML_IFNAME_P2P,
    AML_IFNAME_SAP,
};


#define AML_PRINT_CFM_ERR(req) \
        printk(KERN_CRIT "%s: Status Error(%d)\n", #req, (&req##_cfm)->status)

#define AML_HT_CAPABILITIES                                    \
{                                                               \
    .ht_supported   = true,                                     \
    .cap            = 0,                                        \
    .ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,               \
    .ampdu_density  = IEEE80211_HT_MPDU_DENSITY_16,              \
    .mcs        = {                                             \
        .rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },        \
        .rx_highest = cpu_to_le16(65),                          \
        .tx_params = IEEE80211_HT_MCS_TX_DEFINED,               \
    },                                                          \
}

#define AML_VHT_CAPABILITIES                                   \
{                                                               \
    .vht_supported = false,                                     \
    .cap       =                                                \
      (7 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT),\
    .vht_mcs       = {                                          \
        .rx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
        .tx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
    }                                                           \
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)  || (defined CONFIG_KERNEL_AX_PATCH)
#define AML_HE_CAPABILITIES                                    \
{                                                               \
    .has_he = false,                                            \
    .he_cap_elem = {                                            \
        .mac_cap_info[0] = 0,                                   \
        .mac_cap_info[1] = 0,                                   \
        .mac_cap_info[2] = 0,                                   \
        .mac_cap_info[3] = 0,                                   \
        .mac_cap_info[4] = 0,                                   \
        .mac_cap_info[5] = 0,                                   \
        .phy_cap_info[0] = 0,                                   \
        .phy_cap_info[1] = 0,                                   \
        .phy_cap_info[2] = 0,                                   \
        .phy_cap_info[3] = 0,                                   \
        .phy_cap_info[4] = 0,                                   \
        .phy_cap_info[5] = 0,                                   \
        .phy_cap_info[6] = 0,                                   \
        .phy_cap_info[7] = 0,                                   \
        .phy_cap_info[8] = 0,                                   \
        .phy_cap_info[9] = 0,                                   \
        .phy_cap_info[10] = 0,                                  \
    },                                                          \
    .he_mcs_nss_supp = {                                        \
        .rx_mcs_80 = cpu_to_le16(0xfffa),                       \
        .tx_mcs_80 = cpu_to_le16(0xfffa),                       \
        .rx_mcs_160 = cpu_to_le16(0xffff),                      \
        .tx_mcs_160 = cpu_to_le16(0xffff),                      \
        .rx_mcs_80p80 = cpu_to_le16(0xffff),                    \
        .tx_mcs_80p80 = cpu_to_le16(0xffff),                    \
    },                                                          \
    .ppe_thres = {0x00},                                        \
}
#endif

#define RATE(_bitrate, _hw_rate, _flags) {      \
    .bitrate    = (_bitrate),                   \
    .flags      = (_flags),                     \
    .hw_value   = (_hw_rate),                   \
}

#define CHAN(_freq) {                           \
    .center_freq    = (_freq),                  \
    .max_power  = 30, /* FIXME */               \
}

static struct ieee80211_rate aml_ratetable[] = {
    RATE(10,  0x00, 0),
    RATE(20,  0x01, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(55,  0x02, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60,  0x04, 0),
    RATE(90,  0x05, 0),
    RATE(120, 0x06, 0),
    RATE(180, 0x07, 0),
    RATE(240, 0x08, 0),
    RATE(360, 0x09, 0),
    RATE(480, 0x0A, 0),
    RATE(540, 0x0B, 0),
};

/* The channels indexes here are not used anymore */
static struct ieee80211_channel aml_2ghz_channels[] = {
    CHAN(2412),
    CHAN(2417),
    CHAN(2422),
    CHAN(2427),
    CHAN(2432),
    CHAN(2437),
    CHAN(2442),
    CHAN(2447),
    CHAN(2452),
    CHAN(2457),
    CHAN(2462),
    CHAN(2467),
    CHAN(2472),
    CHAN(2484),
    // Extra channels defined only to be used for PHY measures.
    // Enabled only if custregd and custchan parameters are set
    CHAN(2390),
    CHAN(2400),
    CHAN(2410),
    CHAN(2420),
    CHAN(2430),
    CHAN(2440),
    CHAN(2450),
    CHAN(2460),
    CHAN(2470),
    CHAN(2480),
    CHAN(2490),
    CHAN(2500),
    CHAN(2510),
};

static struct ieee80211_channel aml_5ghz_channels[] = {
    CHAN(5180),             // 36 -   20MHz
    CHAN(5200),             // 40 -   20MHz
    CHAN(5220),             // 44 -   20MHz
    CHAN(5240),             // 48 -   20MHz
    CHAN(5260),             // 52 -   20MHz
    CHAN(5280),             // 56 -   20MHz
    CHAN(5300),             // 60 -   20MHz
    CHAN(5320),             // 64 -   20MHz
    CHAN(5500),             // 100 -  20MHz
    CHAN(5520),             // 104 -  20MHz
    CHAN(5540),             // 108 -  20MHz
    CHAN(5560),             // 112 -  20MHz
    CHAN(5580),             // 116 -  20MHz
    CHAN(5600),             // 120 -  20MHz
    CHAN(5620),             // 124 -  20MHz
    CHAN(5640),             // 128 -  20MHz
    CHAN(5660),             // 132 -  20MHz
    CHAN(5680),             // 136 -  20MHz
    CHAN(5700),             // 140 -  20MHz
    CHAN(5720),             // 144 -  20MHz
    CHAN(5745),             // 149 -  20MHz
    CHAN(5765),             // 153 -  20MHz
    CHAN(5785),             // 157 -  20MHz
    CHAN(5805),             // 161 -  20MHz
    CHAN(5825),             // 165 -  20MHz
    CHAN(5845),             // 168 -  20MHz
    CHAN(5865),             // 173 -  20MHz
    CHAN(5885),             // 177 -  20MHz
    // Extra channels defined only to be used for PHY measures.
    // Enabled only if custregd and custchan parameters are set
    CHAN(5190),
    CHAN(5210),
    CHAN(5230),
    CHAN(5250),
    CHAN(5270),
    CHAN(5290),
    CHAN(5310),
    CHAN(5330),
    CHAN(5340),
    CHAN(5350),
    CHAN(5360),
    CHAN(5370),
    CHAN(5380),
    CHAN(5390),
    CHAN(5400),
    CHAN(5410),
    CHAN(5420),
    CHAN(5430),
    CHAN(5440),
    CHAN(5450),
    CHAN(5460),
    CHAN(5470),
    CHAN(5480),
    CHAN(5490),
    CHAN(5510),
    CHAN(5530),
    CHAN(5550),
    CHAN(5570),
    CHAN(5590),
    CHAN(5610),
    CHAN(5630),
    CHAN(5650),
    CHAN(5670),
    CHAN(5690),
    CHAN(5710),
    CHAN(5730),
    CHAN(5750),
    CHAN(5760),
    CHAN(5770),
    CHAN(5780),
    CHAN(5790),
    CHAN(5800),
    CHAN(5810),
    CHAN(5820),
    CHAN(5830),
    CHAN(5840),
    CHAN(5850),
    CHAN(5860),
    CHAN(5870),
    CHAN(5880),
    CHAN(5890),
    CHAN(5900),
    CHAN(5910),
    CHAN(5920),
    CHAN(5930),
    CHAN(5940),
    CHAN(5950),
    CHAN(5960),
    CHAN(5970),
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)  || (defined CONFIG_KERNEL_AX_PATCH)
static struct ieee80211_sband_iftype_data aml_he_capa[] = {
    {
        .types_mask = BIT(NL80211_IFTYPE_STATION),
        .he_cap = AML_HE_CAPABILITIES,
    },
    {
        .types_mask = BIT(NL80211_IFTYPE_AP),
        .he_cap = AML_HE_CAPABILITIES,
    },
};
#endif

static struct ieee80211_supported_band aml_band_2GHz = {
    .channels   = aml_2ghz_channels,
    .n_channels = ARRAY_SIZE(aml_2ghz_channels) - 13, // -13 to exclude extra channels
    .bitrates   = aml_ratetable,
    .n_bitrates = ARRAY_SIZE(aml_ratetable),
    .ht_cap     = AML_HT_CAPABILITIES,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    .iftype_data = aml_he_capa,
    .n_iftype_data = ARRAY_SIZE(aml_he_capa),
#endif
};

static struct ieee80211_supported_band aml_band_5GHz = {
    .channels   = aml_5ghz_channels,
    .n_channels = ARRAY_SIZE(aml_5ghz_channels) - 59, // -59 to exclude extra channels
    .bitrates   = &aml_ratetable[4],
    .n_bitrates = ARRAY_SIZE(aml_ratetable) - 4,
    .ht_cap     = AML_HT_CAPABILITIES,
    .vht_cap    = AML_VHT_CAPABILITIES,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    .iftype_data = aml_he_capa,
    .n_iftype_data = ARRAY_SIZE(aml_he_capa),
#endif
};

static struct ieee80211_iface_limit aml_limits[] = {
    {
        .max = NX_VIRT_DEV_MAX,
        .types = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP)
    },
    {
        .max = NX_VIRT_DEV_MAX,
        .types = BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_P2P_GO)
    }
};

static struct ieee80211_iface_limit aml_limits_dfs[] = {
    { .max = NX_VIRT_DEV_MAX, .types = BIT(NL80211_IFTYPE_AP)}
};

static const struct ieee80211_iface_combination aml_combinations[] = {
    {
        .limits                 = aml_limits,
        .n_limits               = ARRAY_SIZE(aml_limits),
#ifdef SUPPLICANT_SCC_MODE
        .num_different_channels = 1,
#else
        .num_different_channels = NX_CHAN_CTXT_CNT,
#endif
        .max_interfaces         = NX_VIRT_DEV_MAX,
    },
    /* Keep this combination as the last one */
    {
        .limits                 = aml_limits_dfs,
        .n_limits               = ARRAY_SIZE(aml_limits_dfs),
        .num_different_channels = 1,
        .max_interfaces         = NX_VIRT_DEV_MAX,
        .radar_detect_widths = (BIT(NL80211_CHAN_WIDTH_20_NOHT) |
                                BIT(NL80211_CHAN_WIDTH_20) |
                                BIT(NL80211_CHAN_WIDTH_40) |
                                BIT(NL80211_CHAN_WIDTH_80)),
    }
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static struct ieee80211_txrx_stypes
aml_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
    [NL80211_IFTYPE_STATION] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ACTION >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4)),
    },
    [NL80211_IFTYPE_AP] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
               BIT(IEEE80211_STYPE_DISASSOC >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4) |
               BIT(IEEE80211_STYPE_DEAUTH >> 4) |
               BIT(IEEE80211_STYPE_ACTION >> 4)),
    },
    [NL80211_IFTYPE_AP_VLAN] = {
        /* copy AP */
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
               BIT(IEEE80211_STYPE_DISASSOC >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4) |
               BIT(IEEE80211_STYPE_DEAUTH >> 4) |
               BIT(IEEE80211_STYPE_ACTION >> 4)),
    },
    [NL80211_IFTYPE_P2P_CLIENT] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ACTION >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4)),
    },
    [NL80211_IFTYPE_P2P_GO] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
               BIT(IEEE80211_STYPE_DISASSOC >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4) |
               BIT(IEEE80211_STYPE_DEAUTH >> 4) |
               BIT(IEEE80211_STYPE_ACTION >> 4)),
    },
    [NL80211_IFTYPE_P2P_DEVICE] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ACTION >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4)),
    },
    [NL80211_IFTYPE_MESH_POINT] = {
        .tx = 0xffff,
        .rx = (BIT(IEEE80211_STYPE_ACTION >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4) |
               BIT(IEEE80211_STYPE_DEAUTH >> 4)),
    },
};

/* if wowlan is not supported, kernel generate a disconnect at each suspend
 * cf: /net/wireless/sysfs.c, so register a stub wowlan.
 * Moreover wowlan has to be enabled via a the nl80211_set_wowlan callback.
 * (from user space, e.g. iw phy0 wowlan enable)
 */
static const struct wiphy_wowlan_support wowlan_stub =
{
    .flags = WIPHY_WOWLAN_ANY,
    .n_patterns = 0,
    .pattern_max_len = 0,
    .pattern_min_len = 0,
    .max_pkt_offset = 0,
};

static u32 cipher_suites[] = {
    WLAN_CIPHER_SUITE_WEP40,
    WLAN_CIPHER_SUITE_WEP104,
    WLAN_CIPHER_SUITE_TKIP,
    WLAN_CIPHER_SUITE_CCMP,
    0, // reserved entries to enable AES-CMAC, GCMP-128/256, CCMP-256, SMS4
    0,
    0,
    0,
    0,
};

#define NB_RESERVED_CIPHER 5;

static const int aml_ac2hwq[1][NL80211_NUM_ACS] = {
    {
        [NL80211_TXQ_Q_VO] = AML_HWQ_VO,
        [NL80211_TXQ_Q_VI] = AML_HWQ_VI,
        [NL80211_TXQ_Q_BE] = AML_HWQ_BE,
        [NL80211_TXQ_Q_BK] = AML_HWQ_BK
    }
};

const int aml_tid2hwq[IEEE80211_NUM_TIDS] = {
    AML_HWQ_BE,
    AML_HWQ_BK,
    AML_HWQ_BK,
    AML_HWQ_BE,
    AML_HWQ_VI,
    AML_HWQ_VI,
    AML_HWQ_VO,
    AML_HWQ_VO,
    /* TID_8 is used for management frames */
    AML_HWQ_VO,
    /* At the moment, all others TID are mapped to BE */
    AML_HWQ_BE,
    AML_HWQ_BE,
    AML_HWQ_BE,
    AML_HWQ_BE,
    AML_HWQ_BE,
    AML_HWQ_BE,
    AML_HWQ_BE,
};

static const int aml_hwq2uapsd[NL80211_NUM_ACS] = {
    [AML_HWQ_VO] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VO,
    [AML_HWQ_VI] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VI,
    [AML_HWQ_BE] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BE,
    [AML_HWQ_BK] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BK,
};

extern void aml_print_version(void);
#ifdef CONFIG_AML_DEBUGFS
extern int aml_trace_buf_init(void);

extern void aml_trace_buf_deinit(void);
#endif
extern struct aml_bus_state_detect bus_state_detect;
extern struct usb_device *g_udev;
/*********************************************************************
 * helper
 *********************************************************************/
struct aml_sta *aml_get_sta(struct aml_hw *aml_hw, const u8 *mac_addr)
{
    int i;

    for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
        struct aml_sta *sta = aml_hw->sta_table + i;
        if (sta && sta->valid && (memcmp(mac_addr, &sta->mac_addr, 6) == 0))
            return sta;
    }

    return NULL;
}

void aml_enable_wapi(struct aml_hw *aml_hw)
{
    cipher_suites[aml_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_SMS4;
    aml_hw->wiphy->n_cipher_suites ++;
    aml_hw->wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;
}

void aml_enable_mfp(struct aml_hw *aml_hw)
{
    cipher_suites[aml_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_AES_CMAC;
    aml_hw->wiphy->n_cipher_suites ++;
}

void aml_enable_gcmp(struct aml_hw *aml_hw)
{
    // Assume that HW supports CCMP-256 if it supports GCMP
    cipher_suites[aml_hw->wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_CCMP_256;
    cipher_suites[aml_hw->wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_GCMP;
    cipher_suites[aml_hw->wiphy->n_cipher_suites++] = WLAN_CIPHER_SUITE_GCMP_256;
}

u8 *aml_build_bcn(struct aml_bcn *bcn, struct cfg80211_beacon_data *new)
{
    u8 *buf, *pos;

    if (new->head) {
        u8 *head = kmalloc(new->head_len, GFP_KERNEL);

        if (!head)
            return NULL;

        if (bcn->head)
            kfree(bcn->head);

        bcn->head = head;
        bcn->head_len = new->head_len;
        memcpy(bcn->head, new->head, new->head_len);
    }
    if (new->tail) {
        u8 *tail = kmalloc(new->tail_len, GFP_KERNEL);

        if (!tail)
            return NULL;

        if (bcn->tail)
            kfree(bcn->tail);

        bcn->tail = tail;
        bcn->tail_len = new->tail_len;
        memcpy(bcn->tail, new->tail, new->tail_len);
    }

    if (!bcn->head)
        return NULL;

    bcn->tim_len = 6;
    bcn->len = bcn->head_len + bcn->tail_len + bcn->ies_len + bcn->tim_len;

    buf = kmalloc(bcn->len, GFP_KERNEL);
    if (!buf)
        return NULL;

    // Build the beacon buffer
    pos = buf;
    memcpy(pos, bcn->head, bcn->head_len);
    pos += bcn->head_len;
    *pos++ = WLAN_EID_TIM;
    *pos++ = 4;
    *pos++ = 0;
    *pos++ = bcn->dtim;
    *pos++ = 0;
    *pos++ = 0;
    if (bcn->tail) {
        memcpy(pos, bcn->tail, bcn->tail_len);
        pos += bcn->tail_len;
    }
    if (bcn->ies) {
        memcpy(pos, bcn->ies, bcn->ies_len);
    }

    return buf;
}

static void aml_del_bcn(struct aml_bcn *bcn)
{
    if (bcn->head) {
        kfree(bcn->head);
        bcn->head = NULL;
    }
    bcn->head_len = 0;

    if (bcn->tail) {
        kfree(bcn->tail);
        bcn->tail = NULL;
    }
    bcn->tail_len = 0;

    if (bcn->ies && bcn->ies_len) {
        kfree(bcn->ies);
        bcn->ies = NULL;
    }
    bcn->ies_len = 0;
    bcn->tim_len = 0;
    bcn->dtim = 0;
    bcn->len = 0;
}

/**
 * Link channel ctxt to a vif and thus increments count for this context.
 */
void aml_chanctx_link(struct aml_vif *vif, u8 ch_idx, struct cfg80211_chan_def *chandef)
{
    struct aml_chanctx *ctxt;

    if (ch_idx >= NX_CHAN_CTXT_CNT) {
        WARN(1, "Invalid channel ctxt id %d", ch_idx);
        return;
    }

    AML_INFO("vif index:%d ch idx:%d", vif->vif_index, ch_idx);
    vif->ch_index = ch_idx;
    ctxt = &vif->aml_hw->chanctx_table[ch_idx];
    ctxt->count++;

    // For now chandef is NULL for STATION interface
    if (chandef) {

        AML_INFO("band:%d, bw:%d cfreq:%d, cfreq1:%d, cfreq2:%d",
            chandef->chan->band, chandef->width, chandef->chan->center_freq,
            chandef->center_freq1, chandef->center_freq2);

        if ((!ctxt->chan_def.chan)
            || (chandef->chan->band != ctxt->chan_def.chan->band)
            || (chandef->width != ctxt->chan_def.width)
            || (chandef->chan->center_freq != ctxt->chan_def.chan->center_freq)
            || (chandef->center_freq1 != ctxt->chan_def.center_freq1)
            || (chandef->center_freq2 != ctxt->chan_def.center_freq2)) {

            ctxt->chan_def = *chandef;
        } else {
            // TODO. check that chandef is the same as the one already
            // set for this ctxt
            AML_INFO("chandef is the same as the one already");
        }
    }
}

/**
 * Unlink channel ctxt from a vif and thus decrements count for this context
 */
void aml_chanctx_unlink(struct aml_vif *vif)
{
    struct aml_chanctx *ctxt;

    if (vif->ch_index == AML_CH_NOT_SET)
        return;

    AML_INFO("vif index:%d", vif->vif_index);

    ctxt = &vif->aml_hw->chanctx_table[vif->ch_index];

    if (ctxt->count == 0) {
        WARN(1, "Chan ctxt ref count is already 0");
    } else {
        ctxt->count--;
    }

    if (ctxt->count == 0) {
        if (vif->ch_index == vif->aml_hw->cur_chanctx) {
            /* If current chan ctxt is no longer linked to a vif
               disable radar detection (no need to check if it was activated) */
            aml_radar_detection_enable(&vif->aml_hw->radar,
                                        AML_RADAR_DETECT_DISABLE,
                                        AML_RADAR_RIU);
        }
        /* set chan to null, so that if this ctxt is relinked to a vif that
           don't have channel information, don't use wrong information */
        AML_INFO("set chan to null");
        ctxt->chan_def.chan = NULL;
    }
    vif->ch_index = AML_CH_NOT_SET;
}

int aml_chanctx_band(struct aml_hw *aml_hw, u8 ch_idx)
{
    struct ieee80211_channel *chan;

    if (ch_idx < NX_CHAN_CTXT_CNT &&
        (chan = aml_hw->chanctx_table[ch_idx].chan_def.chan)) {
        return chan->band;
    }

    return -1;
}

bool aml_work_on_5g_band(struct aml_hw *aml_hw)
{
    struct aml_vif *vif;

    list_for_each_entry(vif, &aml_hw->vifs, list) {
        /* ignore down or vlan interface(s) */
        if (!vif->up || AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP_VLAN)
            continue;
        if (aml_chanctx_band(aml_hw, vif->ch_index) > NL80211_BAND_2GHZ)
            return true;
    }
    return false;
}

static void aml_del_csa(struct aml_vif *vif)
{
    struct aml_hw *aml_hw = vif->aml_hw;
    struct aml_csa *csa = vif->ap.csa;

    if (!csa)
        return;

    aml_ipc_buf_dealloc(aml_hw, &csa->buf);
    aml_del_bcn(&csa->bcn);
    kfree(csa);
    vif->ap.csa = NULL;
}

static void aml_csa_finish(struct work_struct *ws)
{
    struct aml_csa *csa = container_of(ws, struct aml_csa, work);
    struct aml_vif *vif = csa->vif;
    struct aml_hw *aml_hw = vif->aml_hw;
    int error = csa->status;
    unsigned int addr;

    if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)csa->buf.addr, (unsigned char *)(unsigned long)addr, csa->buf.size, USB_EP4);
    } else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)csa->buf.addr, (unsigned char *)(unsigned long)addr, csa->buf.size);
    }

    if (!error)
        error = aml_send_bcn_change(aml_hw, vif->vif_index, csa->buf.dma_addr,
                                     csa->bcn.len, csa->bcn.head_len,
                                     csa->bcn.tim_len, NULL);

    if (error)
        cfg80211_stop_iface(aml_hw->wiphy, &vif->wdev, GFP_KERNEL);
    else {
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
            aml_txq_vif_start(vif, AML_TXQ_STOP_CHAN, aml_hw);
        } else
            aml_txq_vif_stop(vif, AML_TXQ_STOP_CHAN, aml_hw);
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
    aml_del_csa(vif);
}

/**
 * aml_external_auth_enable - Enable external authentication on a vif
 *
 * @vif: VIF on which external authentication must be enabled
 *
 * External authentication requires to start TXQ for unknown STA in
 * order to send auth frame pusehd by user space.
 * Note: It is assumed that fw is on the correct channel.
 */
void aml_external_auth_enable(struct aml_vif *vif)
{
    vif->sta.flags |= AML_STA_EXT_AUTH;
    aml_txq_unk_vif_init(vif);
    aml_txq_start(aml_txq_vif_get(vif, NX_UNK_TXQ_TYPE), 0);
}

/**
 * aml_external_auth_disable - Disable external authentication on a vif
 *
 * @vif: VIF on which external authentication must be disabled
 */
void aml_external_auth_disable(struct aml_vif *vif)
{
    if (!(vif->sta.flags & AML_STA_EXT_AUTH))
        return;

    vif->sta.flags &= ~AML_STA_EXT_AUTH;
    aml_txq_unk_vif_deinit(vif);
}

/**
 * aml_update_mesh_power_mode -
 *
 * @vif: mesh VIF  for which power mode is updated
 *
 * Does nothing if vif is not a mesh point interface.
 * Since firmware doesn't support one power save mode per link select the
 * most "active" power mode among all mesh links.
 * Indeed as soon as we have to be active on one link we might as well be
 * active on all links.
 *
 * If there is no link then the power mode for next peer is used;
 */
void aml_update_mesh_power_mode(struct aml_vif *vif)
{
    enum nl80211_mesh_power_mode mesh_pm;
    struct aml_sta *sta, *tmp;
    struct mesh_config mesh_conf;
    struct mesh_update_cfm cfm;
    u32 mask;

    memset(&mesh_conf, 0, sizeof(mesh_conf));
    if (AML_VIF_TYPE(vif) != NL80211_IFTYPE_MESH_POINT)
        return;

    if (list_empty(&vif->ap.sta_list)) {
        mesh_pm = vif->ap.next_mesh_pm;
    } else {
        mesh_pm = NL80211_MESH_POWER_DEEP_SLEEP;
        list_for_each_entry_safe(sta, tmp, &vif->ap.sta_list, list) {
            if (sta->valid && (sta->mesh_pm < mesh_pm)) {
                mesh_pm = sta->mesh_pm;
            }
        }
    }

    if (mesh_pm == vif->ap.mesh_pm)
        return;

    mask = BIT(NL80211_MESHCONF_POWER_MODE - 1);
    mesh_conf.power_mode = mesh_pm;
    if (aml_send_mesh_update_req(vif->aml_hw, vif, mask, &mesh_conf, &cfm) ||
        cfm.status)
        return;

    vif->ap.mesh_pm = mesh_pm;
}

/**
 * aml_save_assoc_ie_for_ft - Save association request elements if Fast
 * Transition has been configured.
 *
 * @vif: VIF that just connected
 * @sme: Connection info
 */
void aml_save_assoc_info_for_ft(struct aml_vif *vif,
                                 struct cfg80211_connect_params *sme)
{
    int ies_len = sme->ie_len + sme->ssid_len + 2;
    u8 *pos;

    if (!cfg80211_find_ie(WLAN_EID_MOBILITY_DOMAIN, sme->ie, sme->ie_len))
        return;

    if (!vif->sta.ft_assoc_ies) {
        vif->sta.ft_assoc_ies_len = ies_len;
        vif->sta.ft_assoc_ies = kmalloc(ies_len, GFP_KERNEL);
    } else if (vif->sta.ft_assoc_ies_len < ies_len) {
        kfree(vif->sta.ft_assoc_ies);
        vif->sta.ft_assoc_ies = kmalloc(ies_len, GFP_KERNEL);
    }

    if (!vif->sta.ft_assoc_ies)
        return;

    // Also save SSID (as an element) in the buffer
    pos = vif->sta.ft_assoc_ies;
    *pos++ = WLAN_EID_SSID;
    *pos++ = sme->ssid_len;
    memcpy(pos, sme->ssid, sme->ssid_len);
    pos += sme->ssid_len;
    memcpy(pos, sme->ie, sme->ie_len);
    vif->sta.ft_assoc_ies_len = ies_len;
}

/**
 * aml_rsne_to_connect_params - Initialise cfg80211_connect_params from
 * RSN element.
 *
 * @rsne: RSN element
 * @sme: Structure cfg80211_connect_params to initialize
 *
 * The goal is only to initialize enough for aml_send_sm_connect_req
 */
int aml_rsne_to_connect_params(const struct element *rsne,
                                struct cfg80211_connect_params *sme)
{
    int len = rsne->datalen;
    int clen;
    const u8 *pos = rsne->data ;

    if (len < 8)
        return 1;

    sme->crypto.control_port_no_encrypt = false;
    sme->crypto.control_port = true;
    sme->crypto.control_port_ethertype = cpu_to_be16(ETH_P_PAE);

    pos += 2;
    sme->crypto.cipher_group = ntohl(*((u32 *)pos));
    pos += 4;
    clen = le16_to_cpu(*((u16 *)pos)) * 4;
    pos += 2;
    len -= 8;
    if (len < clen + 2)
        return 1;
    // only need one cipher suite
    sme->crypto.n_ciphers_pairwise = 1;
    sme->crypto.ciphers_pairwise[0] = ntohl(*((u32 *)pos));
    pos += clen;
    len -= clen;

    // no need for AKM
    clen = le16_to_cpu(*((u16 *)pos)) * 4;
    pos += 2;
    len -= 2;
    if (len < clen)
        return 1;
    pos += clen;
    len -= clen;

    if (len < 4)
        return 0;

    pos += 2;
    clen = le16_to_cpu(*((u16 *)pos)) * 16;
    len -= 4;
    if (len > clen)
        sme->mfp = NL80211_MFP_REQUIRED;

    return 0;
}

int g_cali_cfg_done = 0;
/*********************************************************************
 * netdev callbacks
 ********************************************************************/
/**
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * - Start FW if this is the first interface opened
 * - Add interface at fw level
 */
static int aml_open(struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct mm_add_if_cfm add_if_cfm;
    int error = 0;

    AML_DBG(AML_FN_ENTRY_STR);

#ifdef CONFIG_AML_RECOVERY
    if ((aml_bus_type != PCIE_MODE) && (bus_state_detect.bus_err)) {
        if ((AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_AP) && (aml_recy != NULL) && (aml_recy_flags_chk(AML_RECY_OPEN_VIF_PROC))) {
            aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
        }
        AML_INFO("bus reset err(%d), can't open now !\n", bus_state_detect.bus_err);
        return -EBUSY;
    }

    if (aml_recy_check_aml_vif_exit(aml_hw, aml_vif)) {
        AML_INFO("**********vif: 0x%x, name: %s exist, return\n", aml_vif, aml_vif->ndev->name);
        return 0;
    }
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        u8 cnt = 0;
        AML_INFO("recy ongoing!\n");
        while (aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
            msleep(20);
            if (cnt++ > 100) {
                AML_INFO("recy ongoing, can't open!\n");
                return -EBUSY;
            }
        }
    }
    aml_recy_flags_set(AML_RECY_OPEN_VIF_PROC);
#endif

    // Check if it is the first opened VIF
    if (strncmp(dev->name, AML_IFNAME_STA, 4) == 0) {
        memcpy(dev->dev_addr, aml_hw->wiphy->addresses[0].addr, ETH_ALEN);
    } else if (strncmp(dev->name, AML_IFNAME_P2P, 3) == 0) {
        memcpy(dev->dev_addr, aml_hw->wiphy->addresses[1].addr, ETH_ALEN);
    } else if (strncmp(dev->name, AML_IFNAME_SAP, 2) == 0) {
        memcpy(dev->dev_addr, aml_hw->wiphy->addresses[2].addr, ETH_ALEN);
    } else {
        AML_INFO("open netdev name(%s) error\n", dev->name);
        return -1;
    }

    if (aml_hw->vif_started == 0) {
        // Start the FW
       if ((error = aml_send_start(aml_hw))) {
#ifdef CONFIG_AML_RECOVERY
           aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
#endif
           return error;
       }

       /* Device is now started */
       set_bit(AML_DEV_STARTED, &aml_hw->flags);

    } else {
        struct aml_vif *vif = NULL;
        if (aml_hw->scan_request) {
            vif = container_of(aml_hw->scan_request->wdev, struct aml_vif, wdev);
            error = aml_cancel_scan(aml_hw, vif);
            if (error) {
                printk("cancel scan fail:error = %d\n",error);
            }
            aml_set_scan_hang(vif, 0, __func__, __LINE__);
        }
    }

    if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN) {
        /* For AP_vlan use same fw and drv indexes. We ensure that this index
           will not be used by fw for another vif by taking index >= NX_VIRT_DEV_MAX */
        add_if_cfm.inst_nbr = aml_vif->drv_vif_index;
        netif_tx_stop_all_queues(dev);
    } else {
        /* Forward the information to the LMAC,
         *     p2p value not used in FMAC configuration, iftype is sufficient */
        if ((error = aml_send_add_if(aml_hw, dev->dev_addr,
                                      AML_VIF_TYPE(aml_vif), false, &add_if_cfm))) {
#ifdef CONFIG_AML_RECOVERY
            aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
#endif
            return error;
        }

        if (add_if_cfm.status != 0) {
            AML_PRINT_CFM_ERR(add_if);
#ifdef CONFIG_AML_RECOVERY
            aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
#endif
            return -EIO;
        }
    }

    if (strcmp(dev->name, "wlan0")) {
        aml_vif->is_sta_mode = false;
    } else {
        aml_vif->is_sta_mode = true;
    }
    AML_INFO("vif_idx=%d, type=%d, name=%s sta_mode=%d",
            add_if_cfm.inst_nbr, AML_VIF_TYPE(aml_vif), dev->name, aml_vif->is_sta_mode);

    /* Save the index retrieved from LMAC */
    spin_lock_bh(&aml_hw->cb_lock);
    aml_vif->vif_index = add_if_cfm.inst_nbr;
    aml_vif->up = true;
    aml_hw->vif_started++;
    aml_hw->vif_table[add_if_cfm.inst_nbr] = aml_vif;
    aml_hw->show_switch_info = 0;
    spin_unlock_bh(&aml_hw->cb_lock);

    if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MONITOR) {
        aml_hw->monitor_vif = aml_vif->vif_index;
        if (aml_vif->ch_index != AML_CH_NOT_SET) {
            //Configure the monitor channel
            error = aml_send_config_monitor_req(aml_hw,
                                                 &aml_hw->chanctx_table[aml_vif->ch_index].chan_def,
                                                 NULL);
        }
    }

    netif_carrier_off(dev);

    /*FIXME:
     * config cali param to fw only need operation once.
     * so, it's more suitable to move this to aml_cfg80211_init,
     * however, if we do like this, fw doesn't config cali param
     * cause software runtime sequence really.
     * */
    aml_config_cali_param(aml_hw);
    if ((AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_AP) && (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_P2P_GO))  {
#ifdef CONFIG_AML_RECOVERY
        aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
#endif
    }

    return error;
}

/**
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * - Remove interface at fw level
 * - Reset FW if this is the last interface opened
 */
 static int aml_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *dev,
                                    u16 reason_code);
static int aml_close(struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    int err = 0;
 #ifdef CONFIG_AML_RECOVERY
    int recy_clr_flag = 0;
 #endif

    AML_DBG(AML_FN_ENTRY_STR);

    netdev_info(dev, "CLOSE");
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        u8 cnt = 0;
        while (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
            msleep(20);
            if (cnt++ > 100) {
                AML_INFO("bus recy ongoing, can't close!\n");
                return -EBUSY;
            }
        }
    }
    aml_recy_flags_set(AML_RECY_CLOSE_VIF_PROC);
#endif

    aml_radar_cancel_cac(&aml_hw->radar);

    /* Abort scan request on the vif */
    if (aml_hw->scan_request &&
        aml_hw->scan_request->wdev == &aml_vif->wdev) {
        err = aml_cancel_scan(aml_hw, aml_vif);
        if (err) {
            printk("cancel scan fail:err = %d\n", err);

            #if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
            struct cfg80211_scan_info info = {
            .aborted = false,
            };

            cfg80211_scan_done(aml_hw->scan_request, &info);
            #else
            cfg80211_scan_done(aml_hw->scan_request, false);
            #endif

            aml_hw->scan_request = NULL;
        }
    }
    aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
    if ((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION ||
        AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) &&
        (aml_connect_flags_chk(aml_vif, AML_CONNECTING))) {
        AML_INFO("vif is connecting\n");
        aml_cfg80211_disconnect(aml_hw->wiphy, dev, 0);
    }

    aml_send_remove_if(aml_hw, aml_vif->vif_index);

    spin_lock_bh(&aml_hw->roc_lock);
    if (aml_hw->roc && (aml_hw->roc->vif == aml_vif)) {
        aml_tx_cfm_wait_rsp(aml_hw, false, __func__, __LINE__);
        kfree(aml_hw->roc);
        aml_hw->roc = NULL;
    }
    spin_unlock_bh(&aml_hw->roc_lock);

    /* Ensure that we won't process disconnect ind */
    spin_lock_bh(&aml_hw->cb_lock);
    aml_vif->up = false;
    if (netif_carrier_ok(dev)) {
        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION ||
            AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) {
            if (aml_vif->sta.ft_assoc_ies) {
                kfree(aml_vif->sta.ft_assoc_ies);
                aml_vif->sta.ft_assoc_ies = NULL;
                aml_vif->sta.ft_assoc_ies_len = 0;
            }
            cfg80211_disconnected(dev, WLAN_REASON_DEAUTH_LEAVING,
                                  NULL, 0, true, GFP_ATOMIC);
            if (aml_vif->sta.ap) {
                aml_txq_sta_deinit(aml_hw, aml_vif->sta.ap);
                aml_txq_tdls_vif_deinit(aml_vif);
            }
            netif_tx_stop_all_queues(dev);
            netif_carrier_off(dev);
        } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN) {
            netif_carrier_off(dev);
        } else {
            netdev_warn(dev, "AP not stopped when disabling interface");
        }
    }

    aml_hw->vif_table[aml_vif->vif_index] = NULL;
    spin_unlock_bh(&aml_hw->cb_lock);

    aml_chanctx_unlink(aml_vif);

    if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MONITOR)
        aml_hw->monitor_vif = AML_INVALID_VIF;

    aml_hw->vif_started--;

    aml_hw->show_switch_info = 0;
    if (aml_hw->vif_started == 0) {
        /* This also lets both ipc sides remain in sync before resetting */
        if (aml_bus_type == PCIE_MODE) {
            aml_ipc_tx_drain(aml_hw);
            aml_send_reset(aml_hw);
        }

        // Set parameters to firmware
        aml_send_me_config_req(aml_hw);

        // Set channel parameters to firmware
        aml_send_me_chan_config_req(aml_hw);

        clear_bit(AML_DEV_STARTED, &aml_hw->flags);
    }

    if (aml_bus_type != PCIE_MODE)
        aml_scan_clear_scan_res(aml_hw);
#ifdef CONFIG_AML_RECOVERY
    if ((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) || (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO)) {
        recy_clr_flag |= AML_RECY_STOP_AP_PROC;
     }
     aml_recy_flags_clr(recy_clr_flag | AML_RECY_CLOSE_VIF_PROC);
#endif
    if ((aml_bus_type == USB_MODE) && aml_hw->g_urb) {
        u8 cnt = 0;
        /*wait for cmd cmplete*/
        while (aml_hw->cmd_mgr.queue_sz > 0) {
            msleep(10);
            if (cnt++ > 100) {
                AML_INFO("err cmd not complete!\n");
                break;
            }
        }
        USB_BEGIN_LOCK();
        usb_kill_urb(aml_hw->g_urb);
        USB_END_LOCK();
    }
    return 0;
}

/**
 * struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
 *	Called when a user wants to get the network device usage
 *	statistics. Drivers must do one of the following:
 *	1. Define @ndo_get_stats64 to fill in a zero-initialised
 *	   rtnl_link_stats64 structure passed by the caller.
 *	2. Define @ndo_get_stats to update a net_device_stats structure
 *	   (which should normally be dev->stats) and return a pointer to
 *	   it. The structure may be changed asynchronously only if each
 *	   field is written atomically.
 *	3. Update dev->stats asynchronously and atomically, and define
 *	   neither operation.
 */
static struct net_device_stats *aml_get_stats(struct net_device *dev)
{
    struct aml_vif *vif = netdev_priv(dev);

    return &vif->net_stats;
}

/**
 * u16 (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
 *                         struct net_device *sb_dev);
 *	Called to decide which queue to when device supports multiple
 *	transmit queues.
 */
u16 aml_select_queue(struct net_device *dev, struct sk_buff *skb,
                      struct net_device *sb_dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    return aml_select_txq(aml_vif, skb);
}

/**
 * int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
 *	This function  is called when the Media Access Control address
 *	needs to be changed. If this interface is not defined, the
 *	mac address can not be changed.
 */
static int aml_set_mac_address(struct net_device *dev, void *addr)
{
    struct sockaddr *sa = addr;
    int ret;
    dev->priv_flags |= IFF_LIVE_ADDR_CHANGE;
    ret = eth_mac_addr(dev, sa);

    return ret;
}

static int _aml_priv_ioctl(struct net_device *dev, void __user *data, int cmd)
{
    struct aml_vif *aml_vif = netdev_priv(dev);

    switch (cmd) {
        case SIOCDEVPRIVATE + 1:
            return aml_android_priv_ioctl(aml_vif, data);
        default:
            break;
    }
    return -EINVAL;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static int aml_priv_ioctl_wrapper(struct net_device *dev, struct ifreq *ifr,
	void __user *data, int cmd)
{
    UNUSED(ifr);

    return _aml_priv_ioctl(dev, data, cmd);
}
#else
static int aml_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    return _aml_priv_ioctl(dev, ifr->ifr_data, cmd);
}
#endif

static const struct net_device_ops aml_netdev_ops = {
    .ndo_open               = aml_open,
    .ndo_stop               = aml_close,
    .ndo_start_xmit         = aml_start_xmit,
    .ndo_get_stats          = aml_get_stats,
    .ndo_select_queue       = aml_select_queue,
    .ndo_set_mac_address    = aml_set_mac_address,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
    .ndo_siocdevprivate     = aml_priv_ioctl_wrapper,
#else
    .ndo_do_ioctl           = aml_priv_ioctl,
#endif
//    .ndo_set_features       = aml_set_features,
//    .ndo_set_rx_mode        = aml_set_multicast_list,
};

static const struct net_device_ops aml_netdev_monitor_ops = {
    .ndo_open               = aml_open,
    .ndo_stop               = aml_close,
    .ndo_get_stats          = aml_get_stats,
    .ndo_set_mac_address    = aml_set_mac_address,
};

static void aml_netdev_setup(struct net_device *dev)
{
    ether_setup(dev);
    dev->priv_flags &= ~IFF_TX_SKB_SHARING;
    dev->netdev_ops = &aml_netdev_ops;
#if LINUX_VERSION_CODE <  KERNEL_VERSION(4, 12, 0)
    dev->destructor = free_netdev;
#else
    dev->needs_free_netdev = true;
#endif
    dev->watchdog_timeo = AML_TX_LIFETIME_MS;

    dev->needed_headroom = AML_TX_MAX_HEADROOM;
#ifdef CONFIG_AML_AMSDUS_TX
    dev->needed_headroom = max(dev->needed_headroom,
                               (unsigned short)(sizeof(struct aml_amsdu_txhdr)
                                                + sizeof(struct ethhdr) + 4
                                                + sizeof(rfc1042_header) + 2));
#endif /* CONFIG_AML_AMSDUS_TX */

    //add iwpriv_cmd module
    dev->wireless_handlers = &iw_handle;

    dev->hw_features = 0;
}

/*********************************************************************
 * Cfg80211 callbacks (and helper)
 *********************************************************************/
static struct wireless_dev *aml_interface_add(struct aml_hw *aml_hw,
                                               const char *name,
                                               unsigned char name_assign_type,
                                               enum nl80211_iftype type,
                                               struct vif_params *params)
{
    struct net_device *ndev;
    struct aml_vif *vif;
    int min_idx, max_idx;
    int vif_idx = -1;
    int i;

    // Look for an available VIF
    if (type == NL80211_IFTYPE_AP_VLAN) {
        min_idx = NX_VIRT_DEV_MAX;
        max_idx = NX_ITF_MAX;
    } else {
        min_idx = 0;
        max_idx = NX_VIRT_DEV_MAX;
    }

    for (i = min_idx; i < max_idx; i++) {
        if ((aml_hw->avail_idx_map) & BIT(i)) {
            vif_idx = i;
            break;
        }
    }
    if (vif_idx < 0)
        return NULL;

    AML_INFO("vif index=%d, type=%d\n", vif_idx, type);
#ifndef CONFIG_AML_MON_DATA
    list_for_each_entry(vif, &aml_hw->vifs, list) {
        // Check if monitor interface already exists or type is monitor
        if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_MONITOR) ||
           (type == NL80211_IFTYPE_MONITOR)) {
            wiphy_err(aml_hw->wiphy,
                    "Monitor+Data interface support (MON_DATA) disabled\n");
            return NULL;
        }
    }
#endif
    ndev = alloc_netdev_mqs(sizeof(*vif), name, name_assign_type,
                aml_netdev_setup, NX_NB_NDEV_TXQ,
                num_online_cpus() > 0 ? num_online_cpus() : 1);
    if (!ndev)
        return NULL;

    vif = netdev_priv(ndev);
    ndev->ieee80211_ptr = &vif->wdev;
    vif->wdev.wiphy = aml_hw->wiphy;
    vif->aml_hw = aml_hw;
    vif->ndev = ndev;
    vif->drv_vif_index = vif_idx;
    SET_NETDEV_DEV(ndev, wiphy_dev(vif->wdev.wiphy));
    vif->wdev.netdev = ndev;
    vif->wdev.iftype = type;
    vif->up = false;
    vif->ch_index = AML_CH_NOT_SET;
    vif->generation = 0;
    memset(&vif->net_stats, 0, sizeof(vif->net_stats));

    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.flags = 0;
        vif->sta.ap = NULL;
        vif->sta.tdls_sta = NULL;
        vif->sta.ft_assoc_ies = NULL;
        vif->sta.ft_assoc_ies_len = 0;
        aml_set_scan_hang(vif, 0, __func__, __LINE__);
        vif->sta.scan_duration = 0;
        vif->sta.cancel_scan_cfm = 0;
        vif->sta.connect_flags = 0;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        vif->ap.mesh_pm = NL80211_MESH_POWER_ACTIVE;
        vif->ap.next_mesh_pm = NL80211_MESH_POWER_ACTIVE;
        // no break
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        vif->ap.flags = 0;
        break;
    case NL80211_IFTYPE_AP_VLAN:
    {
        struct aml_vif *master_vif;
        bool found = false;
        list_for_each_entry(master_vif, &aml_hw->vifs, list) {
            if ((AML_VIF_TYPE(master_vif) == NL80211_IFTYPE_AP) &&
                !(!memcmp(master_vif->ndev->dev_addr, params->macaddr,
                           ETH_ALEN))) {
                 found=true;
                 break;
            }
        }

        if (!found)
            goto err;

         vif->ap_vlan.master = master_vif;
         vif->ap_vlan.sta_4a = NULL;
         break;
    }
    case NL80211_IFTYPE_MONITOR:
        ndev->type = ARPHRD_IEEE80211_RADIOTAP;
        ndev->netdev_ops = &aml_netdev_monitor_ops;
        break;
    default:
        break;
    }

    if (strncmp(ndev->name, AML_IFNAME_STA, 4) == 0) {
        memcpy(ndev->dev_addr, aml_hw->wiphy->addresses[0].addr, ETH_ALEN);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 1)
        memcpy(ndev->dev_addr_shadow, aml_hw->wiphy->addresses[0].addr, ETH_ALEN);
#endif
    } else if (strncmp(ndev->name, AML_IFNAME_P2P, 3) == 0) {
        memcpy(ndev->dev_addr, aml_hw->wiphy->addresses[1].addr, ETH_ALEN);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 1)
        memcpy(ndev->dev_addr_shadow, aml_hw->wiphy->addresses[1].addr, ETH_ALEN);
#endif
    } else if (strncmp(ndev->name, AML_IFNAME_SAP, 2) == 0) {
        memcpy(ndev->dev_addr, aml_hw->wiphy->addresses[2].addr, ETH_ALEN);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 1)
        memcpy(ndev->dev_addr_shadow, aml_hw->wiphy->addresses[2].addr, ETH_ALEN);
#endif
    } else {
        AML_INFO("add interface name(%s) error\n", ndev->name);
        goto err;
    }

    if (params) {
        vif->use_4addr = params->use_4addr;
        ndev->ieee80211_ptr->use_4addr = params->use_4addr;
    } else
        vif->use_4addr = false;


    if (register_netdevice(ndev))
        goto err;

    spin_lock_bh(&aml_hw->cb_lock);
    list_add_tail(&vif->list, &aml_hw->vifs);
    spin_unlock_bh(&aml_hw->cb_lock);
    aml_hw->avail_idx_map &= ~BIT(vif_idx);

    spin_lock_init(&vif->vif_lock);
#ifndef CONFIG_LINUXPC_VERSION
    if (aml_bus_type == PCIE_MODE) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        aml_rps_cpus_enable(ndev);
        //aml_xps_cpus_enable(ndev);
        aml_rps_dev_flow_table_enable(ndev);
        aml_rps_sock_flow_sysctl_enable();
#endif
    } else if (aml_bus_type == SDIO_MODE) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
        aml_rps_cpus_disable(ndev);
#endif
    }
#endif

    return &vif->wdev;

err:
    free_netdev(ndev);
    return NULL;
}

/**
 * @brief Retrieve the aml_sta object allocated for a given MAC address
 * and a given role.
 */
static struct aml_sta *aml_retrieve_sta(struct aml_hw *aml_hw,
                                          struct aml_vif *aml_vif, u8 *addr,
                                          __le16 fc, bool ap)
{
    if (ap) {
        /* only deauth, disassoc and action are bufferable MMPDUs */
        bool bufferable = ieee80211_is_deauth(fc) ||
                          ieee80211_is_disassoc(fc) ||
                          ieee80211_is_action(fc);

        /* Check if the packet is bufferable or not */
        if (bufferable)
        {
            /* Check if address is a broadcast or a multicast address */
            if (is_broadcast_ether_addr(addr) || is_multicast_ether_addr(addr)) {
                /* Returned STA pointer */
                struct aml_sta *aml_sta = aml_hw->sta_table + aml_vif->ap.bcmc_index;

                if (aml_sta && aml_sta->valid)
                    return aml_sta;
            } else {
                /* Returned STA pointer */
                struct aml_sta *aml_sta, *tmp;

                /* Go through list of STAs linked with the provided VIF */
                list_for_each_entry_safe(aml_sta, tmp, &aml_vif->ap.sta_list, list) {
                    if (aml_sta->valid &&
                        ether_addr_equal(aml_sta->mac_addr, addr)) {
                        /* Return the found STA */
                        return aml_sta;
                    }
                }
            }
        }
    } else {
        return aml_vif->sta.ap;
    }

    return NULL;
}

/**
 * @add_virtual_intf: create a new virtual interface with the given name,
 *	must set the struct wireless_dev's iftype. Beware: You must create
 *	the new netdev in the wiphy's network namespace! Returns the struct
 *	wireless_dev, or an ERR_PTR. For P2P device wdevs, the driver must
 *	also set the address member in the wdev.
 */
static struct wireless_dev *aml_cfg80211_add_iface(struct wiphy *wiphy,
                                                    const char *name,
                                                    unsigned char name_assign_type,
                                                    enum nl80211_iftype type,
                                                    struct vif_params *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct wireless_dev *wdev;

    wdev = aml_interface_add(aml_hw, name, name_assign_type, type, params);

    if (!wdev)
        return ERR_PTR(-EINVAL);

    return wdev;
}

/**
 * @del_virtual_intf: remove the virtual interface
 */
static int aml_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    struct net_device *dev = wdev->netdev;
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);

    netdev_info(dev, "Remove Interface");

    if (dev->reg_state == NETREG_REGISTERED) {
        /* Will call aml_close if interface is UP */
        unregister_netdevice(dev);
    }

    spin_lock_bh(&aml_hw->cb_lock);
    list_del(&aml_vif->list);
    spin_unlock_bh(&aml_hw->cb_lock);
    aml_hw->avail_idx_map |= BIT(aml_vif->drv_vif_index);
    aml_vif->ndev = NULL;

    /* Clear the priv in adapter */
    dev->ieee80211_ptr = NULL;

    return 0;
}

/**
 * @change_virtual_intf: change type/configuration of virtual interface,
 *	keep the struct wireless_dev's iftype updated.
 */
int aml_cfg80211_change_iface(struct wiphy *wiphy,
                                      struct net_device *dev,
                                      enum nl80211_iftype type,
                                      struct vif_params *params)
{
#ifndef CONFIG_AML_MON_DATA
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
#endif
    struct aml_vif *vif = netdev_priv(dev);

    AML_DBG(AML_FN_ENTRY_STR);

    AML_INFO("interface type=%d\n", type);
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        AML_INFO("recy ongoing, can't change iface type!\n");
        return -EBUSY;
    }
#endif
    if (vif->up)
        return (-EBUSY);

#ifndef CONFIG_AML_MON_DATA
    if ((type == NL80211_IFTYPE_MONITOR) &&
       (AML_VIF_TYPE(vif) != NL80211_IFTYPE_MONITOR)) {
        struct aml_vif *vif_el;
        list_for_each_entry(vif_el, &aml_hw->vifs, list) {
            // Check if data interface already exists
            if ((vif_el != vif) &&
               (AML_VIF_TYPE(vif) != NL80211_IFTYPE_MONITOR)) {
                wiphy_err(aml_hw->wiphy,
                        "Monitor+Data interface support (MON_DATA) disabled\n");
                return -EIO;
            }
        }
    }
#endif

    AML_INFO("vif index=%d, type=%d", vif->vif_index, type);
    // Reset to default case (i.e. not monitor)
    dev->type = ARPHRD_ETHER;
    dev->netdev_ops = &aml_netdev_ops;

    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.flags = 0;
        spin_lock_bh(&vif->vif_lock);
        vif->sta.ap = NULL;
        spin_unlock_bh(&vif->vif_lock);
        vif->sta.tdls_sta = NULL;
        vif->sta.ft_assoc_ies = NULL;
        vif->sta.ft_assoc_ies_len = 0;
#ifdef CONFIG_AML_RECOVERY
        if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_GO) {
            AML_INFO("GO --> DEVICE");
            aml_recy_flags_clr(AML_RECY_GO_ONGOING|AML_RECY_STOP_AP_PROC | AML_RECY_OPEN_VIF_PROC);
        }
        if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP) {
            aml_recy_flags_clr(AML_RECY_STOP_AP_PROC | AML_RECY_OPEN_VIF_PROC);
        }
#endif
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        // no break
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        vif->ap.flags = 0;
        break;
    case NL80211_IFTYPE_AP_VLAN:
        return -EPERM;
    case NL80211_IFTYPE_MONITOR:
        dev->type = ARPHRD_IEEE80211_RADIOTAP;
        dev->netdev_ops = &aml_netdev_monitor_ops;
        break;
    default:
        break;
    }

    vif->generation = 0;
    vif->wdev.iftype = type;
    if (params && params->use_4addr != -1)
        vif->use_4addr = params->use_4addr;

    return 0;
}

void aml_set_scan_hang(struct aml_vif *aml_vif, int scan_hang, u8* func, u32 line)
{
    aml_vif->sta.scan_hang = scan_hang;
    printk("set_scan_hang, flag:%d, [%s %d]\n", scan_hang, func, line);
}

static unsigned long last_scan_time = 0;
/**
 * @scan: Request to do a scan. If returning zero, the scan request is given
 *	the driver, and will be valid until passed to cfg80211_scan_done().
 *	For scan results, call cfg80211_inform_bss(); you can call this outside
 *	the scan/scan_done bracket too.
 */
static int aml_cfg80211_scan(struct wiphy *wiphy,
                              struct cfg80211_scan_request *request)
{
#ifndef CONFIG_PT_MODE
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = container_of(request->wdev, struct aml_vif,wdev);
    int error;
    struct aml_roc *roc;
    unsigned long last_time = 0;

    AML_INFO("n_channels:%d,iftype:%d",request->n_channels,request->wdev->iftype);
    last_time = last_scan_time;
    last_scan_time = jiffies;

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy) {
        if (aml_recy_flags_chk(AML_RECY_STATE_ONGOING) ||
                (aml_recy->link_loss.is_enabled
                 && aml_recy->link_loss.is_requested)) {
            printk("recovery is ongoing, can't scan now!\n");
            return -EBUSY;
        }
    }
#endif

    if (aml_vif->sta.scan_hang) {
        printk("%s scan_hang is on, can't scan now!\n", __func__);
        return -EAGAIN;
    }

    if ((aml_hw->traffic_busy) && (time_after(jiffies, last_time + msecs_to_jiffies(AML_SCAN_INTERNAL_THR)))) {
        AML_INFO("abort scan,traffic busy\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        struct cfg80211_scan_info info = {
            .aborted = false,
        };

        cfg80211_scan_done(request, &info);
#else
        cfg80211_scan_done(request, false);
#endif
        return 0;
    }

    spin_lock_bh(&aml_hw->roc_lock);
    roc = aml_hw->roc;
    if (roc) {
        if (roc->vif->is_sta_mode) {
            int ret;
            spin_unlock_bh(&aml_hw->roc_lock);
            ret = aml_send_cancel_roc(aml_hw);
            AML_INFO("sta mode in roc,cancel roc first,ret:%d\n", ret);
            if (ret) {
                return -EBUSY;
            }
        }
        else {
            AML_INFO("avoid scan as roc,roc vif type:%d\n",AML_VIF_TYPE(roc->vif));
            spin_unlock_bh(&aml_hw->roc_lock);
            return -EBUSY;
        }
    }
    else {
        spin_unlock_bh(&aml_hw->roc_lock);
    }

    if ((error = aml_send_scanu_req(aml_hw, aml_vif, request)))
        return error;

    aml_hw->scan_request = request;
#endif
    return 0;
}

/**
 * @add_key: add a key with the given parameters. @mac_addr will be %NULL
 *	when adding a group key.
 */
int aml_cfg80211_add_key(struct wiphy *wiphy, struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
        int link_id, u8 key_index, bool pairwise, const u8 *mac_addr,
#else
        u8 key_index, bool pairwise, const u8 *mac_addr,
#endif
        struct key_params *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(netdev);
    int i, error = 0;
    struct mm_key_add_cfm key_add_cfm;
    u8_l cipher = 0;
    struct aml_sta *sta = NULL;
    struct aml_key *aml_key;
    UNUSED_LINK_ID(link_id);

    if (mac_addr) {
        sta = aml_get_sta(aml_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        aml_key = &sta->key;
    }
    else
        aml_key = &vif->key[key_index];

    AML_INFO("vif_idx:%d add %s for index:%d,sta_idx:%d",
            vif->vif_index, pairwise ? "PTK" : "GTK", key_index, sta == NULL ? 0xff : sta->sta_idx);
    /* Retrieve the cipher suite selector */
    switch (params->cipher) {
    case WLAN_CIPHER_SUITE_WEP40:
        cipher = MAC_CIPHER_WEP40;
        break;
    case WLAN_CIPHER_SUITE_WEP104:
        cipher = MAC_CIPHER_WEP104;
        break;
    case WLAN_CIPHER_SUITE_TKIP:
        cipher = MAC_CIPHER_TKIP;
        break;
    case WLAN_CIPHER_SUITE_CCMP:
        cipher = MAC_CIPHER_CCMP;
        break;
    case WLAN_CIPHER_SUITE_AES_CMAC:
        cipher = MAC_CIPHER_BIP_CMAC_128;
        break;
    case WLAN_CIPHER_SUITE_SMS4:
    {
        // Need to reverse key order
        u8 tmp, *key = (u8 *)params->key;
        cipher = MAC_CIPHER_WPI_SMS4;
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i];
            key[i] = key[WPI_SUBKEY_LEN - 1 - i];
            key[WPI_SUBKEY_LEN - 1 - i] = tmp;
        }
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i + WPI_SUBKEY_LEN];
            key[i + WPI_SUBKEY_LEN] = key[WPI_KEY_LEN - 1 - i];
            key[WPI_KEY_LEN - 1 - i] = tmp;
        }
        break;
    }
    case WLAN_CIPHER_SUITE_GCMP:
        cipher = MAC_CIPHER_GCMP_128;
        break;
    case WLAN_CIPHER_SUITE_GCMP_256:
        cipher = MAC_CIPHER_GCMP_256;
        break;
    case WLAN_CIPHER_SUITE_CCMP_256:
        cipher = MAC_CIPHER_CCMP_256;
        break;
    default:
        return -EINVAL;
    }

    if ((error = aml_send_key_add(aml_hw, vif->vif_index,
                                   (sta ? sta->sta_idx : 0xFF), pairwise,
                                   (u8 *)params->key, params->key_len,
                                   key_index, cipher, &key_add_cfm)))
        return error;

    if (key_add_cfm.status != 0) {
        AML_PRINT_CFM_ERR(key_add);
        return -EIO;
    }

    /* Save the index retrieved from LMAC */
    aml_key->hw_idx = key_add_cfm.hw_key_idx;

    return 0;
}

/**
 * @get_key: get information about the key with the given parameters.
 *	@mac_addr will be %NULL when requesting information for a group
 *	key. All pointers given to the @callback function need not be valid
 *	after it returns. This function should return an error if it is
 *	not possible to retrieve the key, -ENOENT if it doesn't exist.
 *
 */
static int aml_cfg80211_get_key(struct wiphy *wiphy, struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
                                 int link_id, u8 key_index, bool pairwise, const u8 *mac_addr,
#else
                                 u8 key_index, bool pairwise, const u8 *mac_addr,
#endif
                                 void *cookie,
                                 void (*callback)(void *cookie, struct key_params*))
{
    UNUSED_LINK_ID(link_id);

    AML_DBG(AML_FN_ENTRY_STR);

    return -1;
}


/**
 * @del_key: remove a key given the @mac_addr (%NULL for a group key)
 *	and @key_index, return -ENOENT if the key doesn't exist.
 */
static int aml_cfg80211_del_key(struct wiphy *wiphy, struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
                                 int link_id, u8 key_index, bool pairwise, const u8 *mac_addr)
#else
                                 u8 key_index, bool pairwise, const u8 *mac_addr)
#endif
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(netdev);
    int error;
    struct aml_sta *sta = NULL;
    struct aml_key *aml_key;
    UNUSED_LINK_ID(link_id);

    AML_INFO("vif_idx:%d", vif->vif_index);
    if (mac_addr) {
        sta = aml_get_sta(aml_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        aml_key = &sta->key;
    }
    else
        aml_key = &vif->key[key_index];

    error = aml_send_key_del(aml_hw, aml_key->hw_idx);

    return error;
}

/**
 * @set_default_key: set the default key on an interface
 */
static int aml_cfg80211_set_default_key(struct wiphy *wiphy,
                                         struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
                                         int link_id, u8 key_index, bool unicast, bool multicast)
#else
                                         u8 key_index, bool unicast, bool multicast)
#endif
{
    UNUSED_LINK_ID(link_id);

    AML_DBG(AML_FN_ENTRY_STR);

    return 0;
}

/**
 * @set_default_mgmt_key: set the default management frame key on an interface
 */
static int aml_cfg80211_set_default_mgmt_key(struct wiphy *wiphy,
                                              struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
                                              int link_id, u8 key_index)
#else
                                              u8 key_index)
#endif
{
    UNUSED_LINK_ID(link_id);

    return 0;
}

void aml_connect_flags_set(struct aml_vif *aml_vif, u32 flags)
{
    aml_vif->sta.connect_flags |= flags;
}

void aml_connect_flags_clr(struct aml_vif *aml_vif, u32 flags)
{
    aml_vif->sta.connect_flags &= ~(flags);
}

bool aml_connect_flags_chk(struct aml_vif *aml_vif, u32 flags)
{
    return (!!(aml_vif->sta.connect_flags & flags));
}

/**
 * @connect: Connect to the ESS with the specified parameters. When connected,
 *	call cfg80211_connect_result() with status code %WLAN_STATUS_SUCCESS.
 *	If the connection fails for some reason, call cfg80211_connect_result()
 *	with the status from the AP.
 *	(invoked with the wireless_dev mutex held)
 */
static int aml_cfg80211_connect(struct wiphy *wiphy, struct net_device *dev,
                                 struct cfg80211_connect_params *sme)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct sm_connect_cfm sm_connect_cfm;
    int error = 0;
    int err = 0;

    AML_INFO("<%s> vif:%d privacy=%d, key=%p, key_len=%d, key_idx=%d auth_type:%d wpa_versions:%d\n",
            dev->name, aml_vif->vif_index, sme->privacy, sme->key, sme->key_len, sme->key_idx,sme->auth_type,sme->crypto.wpa_versions);

    /* For SHARED-KEY authentication, must install key first */
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,0,0)
    if (!(sme->crypto.wpa_versions & (NL80211_WPA_VERSION_1 | NL80211_WPA_VERSION_2 | NL80211_WPA_VERSION_3)) &&  sme->key && sme->key_len
#else
    if (!(sme->crypto.wpa_versions & (NL80211_WPA_VERSION_1 | NL80211_WPA_VERSION_2)) &&  sme->key && sme->key_len
#endif
        && ((sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY) || (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC)
        || (sme->crypto.n_ciphers_pairwise & (WLAN_CIPHER_SUITE_WEP40 | WLAN_CIPHER_SUITE_WEP104))))
    {
        struct key_params key_params;

        if (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) {
            sme->auth_type = NL80211_AUTHTYPE_SHARED_KEY;
        }
        key_params.key = sme->key;
        key_params.seq = NULL;
        key_params.key_len = sme->key_len;
        key_params.seq_len = 0;
        key_params.cipher = sme->crypto.cipher_group;
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
        aml_cfg80211_add_key(wiphy, dev, 0, sme->key_idx, false, NULL, &key_params);
#else
        aml_cfg80211_add_key(wiphy, dev, sme->key_idx, false, NULL, &key_params);
#endif
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    else if ((sme->auth_type == NL80211_AUTHTYPE_SAE) &&
             !(sme->flags & CONNECT_REQ_EXTERNAL_AUTH_SUPPORT)) {
        netdev_err(dev, "Doesn't support SAE without external authentication\n");
        return -EINVAL;
    }
#endif

    if (aml_hw->scan_request) {
        err = aml_cancel_scan(aml_hw, aml_vif);
        if (err) {
            printk("cancel scan fail:err = %d\n",err);
        }
    } else {
        aml_set_scan_hang(aml_vif, 1, __func__, __LINE__);
    }

    /* Forward the information to the LMAC */
    if ((error = aml_send_sm_connect_req(aml_hw, aml_vif, sme, &sm_connect_cfm))) {
        aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
        return error;
    }

    // Check the status
    switch (sm_connect_cfm.status)
    {
        case CO_OK:
            if (sme->ssid != NULL && sme->channel != NULL) {
                AML_INFO("ssid=%s, center freq:%d\n", ssid_sprintf(sme->ssid, sme->ssid_len),
                        sme->channel->center_freq);
            }
            aml_vif->sta.assoc_ssid_len =
                sme->ssid_len > MAC_SSID_LEN ? MAC_SSID_LEN : sme->ssid_len;
            memcpy(&aml_vif->sta.assoc_ssid, sme->ssid, aml_vif->sta.assoc_ssid_len);
            aml_save_assoc_info_for_ft(aml_vif, sme);
#ifdef CONFIG_AML_RECOVERY
            aml_recy_save_assoc_info(sme, aml_vif->vif_index);
#endif
            aml_connect_flags_set(aml_vif, AML_CONNECTING);
            error = 0;
            break;
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_BAD_PARAM:
            error = -EINVAL;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }
    if(error != 0) {
        aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
        if (aml_vif->sta.ft_assoc_ies) {
            kfree(aml_vif->sta.ft_assoc_ies);
            aml_vif->sta.ft_assoc_ies = NULL;
            aml_vif->sta.ft_assoc_ies_len = 0;
        }
    }

    return error;
}

/**
 * @disconnect: Disconnect from the BSS/ESS.
 *	(invoked with the wireless_dev mutex held)
 */
static int aml_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *dev,
                                    u16 reason_code)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct wireless_dev *wdev = dev->ieee80211_ptr;
    int error = 0;
    int rtn = 0;
    int cnt = 0;

    if (aml_connect_flags_chk(aml_vif, AML_CONNECTING)) {
        AML_INFO("sta connecting:0x%x,waiting for connect event\n", aml_vif->sta.connect_flags);
        while (aml_connect_flags_chk(aml_vif, AML_CONNECTING)) {
            msleep(50);
            if (cnt++ > 100) {
                AML_INFO("wait connecting timeout vif_idx:%d \n", aml_vif->vif_index);
                break;
            }
        }
    }
    if (aml_hw->scan_request) {
        error = aml_cancel_scan(aml_hw, aml_vif);
        if (error) {
            printk("cancel scan fail:error = %d\n",error);
        }
    }

    if (!aml_vif->sta.ap) {
        AML_INFO("error,sta.ap is null");
    }

    if ((aml_recy != NULL) && (aml_vif->vif_index == aml_recy->assoc_info.vif_idx)) {
        aml_recy_flags_clr(AML_RECY_ASSOC_INFO_SAVED);
    }

    if (aml_vif->sta.ap && aml_vif->sta.ap->valid) {
        cnt = 0;
        aml_connect_flags_clr(aml_vif, AML_GETTING_IP);
        aml_connect_flags_set(aml_vif, AML_DISCONNECTING);
        rtn = aml_send_sm_disconnect_req(aml_hw, aml_vif, reason_code);
        if (!rtn) {
            /*not return directly,block the next connect command，avoid mismatch with upper layer*/
            while (aml_connect_flags_chk(aml_vif, AML_DISCONNECTING)) {
                msleep(50);
                if (cnt++ > 20) {
                    AML_INFO("wait disconnecting timeout \n");
                    break;
                }
            }
        }
        aml_connect_flags_clr(aml_vif, AML_DISCONNECTING);
    }
#if defined(IEEE80211_MLD_MAX_NUM_LINKS)
    else if (wdev->connected || wdev->u.client.ssid_len) {
#else
    else if (wdev->current_bss || wdev->ssid_len) {
#endif
        AML_INFO("wifi is disconnect, and state mismatch with upper layer, need disconnect to kernel\n");
        cfg80211_disconnected(dev, 0, NULL, 0,false, GFP_KERNEL);
    }
    aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
    AML_INFO("vif_idx:%d \n", aml_vif->vif_index);
    return rtn;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
/**
 * @external_auth: indicates result of offloaded authentication processing from
 *     user space
 */
static int aml_cfg80211_external_auth(struct wiphy *wiphy, struct net_device *dev,
                                       struct cfg80211_external_auth_params *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);

    AML_INFO("flags:0x%08x, status:%d", aml_vif->sta.flags, params->status);
    if (!(aml_vif->sta.flags & AML_STA_EXT_AUTH))
        return -EINVAL;

    aml_external_auth_disable(aml_vif);
    if ((params->status == 1) && (aml_vif->sta.auth_status != 0))
        params->status = aml_vif->sta.auth_status;
    return aml_send_sm_external_auth_required_rsp(aml_hw, aml_vif,
                                                   params->status);
}
#endif

/**
 * @add_station: Add a new station.
 */
static int aml_cfg80211_add_station(struct wiphy *wiphy, struct net_device *dev,
                                     const u8 *mac, struct station_parameters *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct me_sta_add_cfm me_sta_add_cfm;
    int error = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    WARN_ON(AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN);

    /* Do not add TDLS station */
    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        return 0;

    /* Indicate we are in a STA addition process - This will allow handling
     * potential PS mode change indications correctly
     */
    set_bit(AML_DEV_ADDING_STA, &aml_hw->flags);

    /* Forward the information to the LMAC */
    if ((error = aml_send_me_sta_add(aml_hw, params, mac, aml_vif->vif_index,
                                      &me_sta_add_cfm)))
        return error;

    // Check the status
    switch (me_sta_add_cfm.status)
    {
        case CO_OK:
        {
            struct aml_sta *sta = aml_hw->sta_table + me_sta_add_cfm.sta_idx;
            struct station_info sta_info;
            int tid;
            sta->aid = params->aid;

            sta->sta_idx = me_sta_add_cfm.sta_idx;
            sta->ch_idx = aml_vif->ch_index;
            sta->vif_idx = aml_vif->vif_index;
            sta->vlan_idx = sta->vif_idx;
            sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
            sta->ht = P_LINK_STA_PARAMS(params, ht_capa) ? 1 : 0;
            sta->vht = P_LINK_STA_PARAMS(params, vht_capa) ? 1 : 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,20,0) || (defined CONFIG_KERNEL_AX_PATCH)
            sta->he = P_LINK_STA_PARAMS(params, he_capa) ? 1 : 0;
#endif
            sta->acm = 0;
            sta->listen_interval = params->listen_interval;

            if (params->local_pm != NL80211_MESH_POWER_UNKNOWN)
                sta->mesh_pm = params->local_pm;
            else
                sta->mesh_pm = aml_vif->ap.next_mesh_pm;
            aml_update_mesh_power_mode(aml_vif);

            for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                int uapsd_bit = aml_hwq2uapsd[aml_tid2hwq[tid]];
                if (params->uapsd_queues & uapsd_bit)
                    sta->uapsd_tids |= 1 << tid;
                else
                    sta->uapsd_tids &= ~(1 << tid);
            }
            memcpy(sta->mac_addr, mac, ETH_ALEN);
            aml_dbgfs_register_sta(aml_hw, sta);

            /* Ensure that we won't process PS change or channel switch ind*/
            spin_lock_bh(&aml_hw->cb_lock);
            aml_txq_sta_init(aml_hw, sta, aml_txq_vif_get_status(aml_vif));
            spin_lock_bh(&aml_vif->vif_lock);
            list_add_tail(&sta->list, &aml_vif->ap.sta_list);
            spin_unlock_bh(&aml_vif->vif_lock);
            aml_vif->generation++;
            sta->valid = true;
            aml_ps_bh_enable(aml_hw, sta, sta->ps.active || me_sta_add_cfm.pm_state);
            spin_unlock_bh(&aml_hw->cb_lock);

            error = 0;

#ifdef CONFIG_AML_BFMER
            if (aml_hw->mod_params->bfmer)
                aml_send_bfmer_enable(aml_hw, sta, P_LINK_STA_PARAMS(params, vht_capa));

            aml_mu_group_sta_init(sta, P_LINK_STA_PARAMS(params, vht_capa));
#endif /* CONFIG_AML_BFMER */

            #define PRINT_STA_FLAG(f)                               \
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

            AML_INFO("Add sta %d (%pM) flags=%s%s%s%s%s%s%s",
                        sta->sta_idx, mac,
                        PRINT_STA_FLAG(AUTHORIZED),
                        PRINT_STA_FLAG(SHORT_PREAMBLE),
                        PRINT_STA_FLAG(WME),
                        PRINT_STA_FLAG(MFP),
                        PRINT_STA_FLAG(AUTHENTICATED),
                        PRINT_STA_FLAG(TDLS_PEER),
                        PRINT_STA_FLAG(ASSOCIATED));
            #undef PRINT_STA_FLAG
            memset(&sta_info, 0, sizeof(sta_info));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
            sta_info.filled = STATION_INFO_ASSOC_REQ_IES;
#endif
            /*FIXME:
             * need add the connected sta's assoc req ies info,
             * */
            sta_info.assoc_req_ies = NULL;
            sta_info.assoc_req_ies_len = 0;
            cfg80211_new_sta(dev, mac, &sta_info, GFP_KERNEL);
            break;
        }
        default:
            error = -EBUSY;
            break;
    }

    clear_bit(AML_DEV_ADDING_STA, &aml_hw->flags);

    return error;
}

/**
 * @del_station: Remove a station
 */
int aml_cfg80211_del_station(struct wiphy *wiphy,
        struct net_device *dev, struct station_del_parameters *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_sta *cur, *tmp;
    int error = 0, found = 0;
    const u8 *mac = NULL;

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_DEL_STA_PROC)) {
        u8 cnt = 0;
        while (aml_recy_flags_chk(AML_RECY_DEL_STA_PROC)) {
            msleep(10);
            if (cnt++ > 20) {
                AML_INFO("recy ongoing, can't del sta!\n");
                return -EBUSY;
            }
        }
    }
    aml_recy_flags_set(AML_RECY_DEL_STA_PROC);
#endif
    if (params)
        mac = params->mac;

    list_for_each_entry_safe(cur, tmp, &aml_vif->ap.sta_list, list) {
        if ((!mac) || (!memcmp(cur->mac_addr, mac, ETH_ALEN))) {
            AML_INFO("Del sta %d (%pM)", cur->sta_idx, cur->mac_addr);
            /* Ensure that we won't process PS change ind */
            spin_lock_bh(&aml_hw->cb_lock);
            cur->ps.active = false;
            cur->valid = false;
            spin_unlock_bh(&aml_hw->cb_lock);

            if (cur->vif_idx != cur->vlan_idx) {
                struct aml_vif *vlan_vif;
                vlan_vif = aml_hw->vif_table[cur->vlan_idx];
                if (vlan_vif->up) {
                    if ((AML_VIF_TYPE(vlan_vif) == NL80211_IFTYPE_AP_VLAN) &&
                        (vlan_vif->use_4addr)) {
                        vlan_vif->ap_vlan.sta_4a = NULL;
                    } else {
                        WARN(1, "Deleting sta belonging to VLAN other than AP_VLAN 4A");
                    }
                }
            }

            aml_txq_sta_deinit(aml_hw, cur);
            error = aml_send_me_sta_del(aml_hw, cur->sta_idx, false);
            if ((error != 0) && (error != -EPIPE)) {
#ifdef CONFIG_AML_RECOVERY
                aml_recy_flags_clr(AML_RECY_DEL_STA_PROC);
#endif
                return error;
            }
            aml_del_sta(aml_vif, cur->mac_addr, cur->center_freq);
#ifdef CONFIG_AML_BFMER
            // Disable Beamformer if supported
            aml_bfmer_report_del(aml_hw, cur);
            aml_mu_group_sta_del(aml_hw, cur);
#endif /* CONFIG_AML_BFMER */
            spin_lock_bh(&aml_vif->vif_lock);
            list_del(&cur->list);
            spin_unlock_bh(&aml_vif->vif_lock);
            aml_vif->generation++;
            aml_dbgfs_unregister_sta(aml_hw, cur);
            found++;
            break;
        }
    }
#ifdef CONFIG_AML_RECOVERY
    aml_recy_flags_clr(AML_RECY_DEL_STA_PROC);
#endif
    if (!found)
        return -ENOENT;

    aml_update_mesh_power_mode(aml_vif);

    return 0;
}

/**
 * @change_station: Modify a given station. Note that flags changes are not much
 *	validated in cfg80211, in particular the auth/assoc/authorized flags
 *	might come to the driver in invalid combinations -- make sure to check
 *	them, also against the existing state! Drivers must call
 *	cfg80211_check_station_change() to validate the information.
 */
static int aml_cfg80211_change_station(struct wiphy *wiphy, struct net_device *dev,
                                        const u8 *mac, struct station_parameters *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_sta *sta;

    sta = aml_get_sta(aml_hw, mac);
    if (!sta)
    {
        /* Add the TDLS station */
        if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        {
            struct aml_vif *aml_vif = netdev_priv(dev);
            struct me_sta_add_cfm me_sta_add_cfm;
            int error = 0;

            /* Indicate we are in a STA addition process - This will allow handling
             * potential PS mode change indications correctly
             */
            set_bit(AML_DEV_ADDING_STA, &aml_hw->flags);

            /* Forward the information to the LMAC */
            if ((error = aml_send_me_sta_add(aml_hw, params, mac, aml_vif->vif_index,
                                              &me_sta_add_cfm)))
                return error;

            // Check the status
            switch (me_sta_add_cfm.status)
            {
                case CO_OK:
                {
                    int tid;
                    sta = aml_hw->sta_table + me_sta_add_cfm.sta_idx;
                    sta->aid = params->aid;
                    sta->sta_idx = me_sta_add_cfm.sta_idx;
                    sta->ch_idx = aml_vif->ch_index;
                    sta->vif_idx = aml_vif->vif_index;
                    sta->vlan_idx = sta->vif_idx;
                    sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
                    sta->ht = P_LINK_STA_PARAMS(params, ht_capa) ? 1 : 0;
                    sta->vht = P_LINK_STA_PARAMS(params, vht_capa) ? 1 : 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,20,0) || (defined CONFIG_KERNEL_AX_PATCH)
                    sta->he = P_LINK_STA_PARAMS(params, he_capa) ? 1 : 0;
#endif
                    sta->acm = 0;
                    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                        int uapsd_bit = aml_hwq2uapsd[aml_tid2hwq[tid]];
                        if (params->uapsd_queues & uapsd_bit)
                            sta->uapsd_tids |= 1 << tid;
                        else
                            sta->uapsd_tids &= ~(1 << tid);
                    }
                    memcpy(sta->mac_addr, mac, ETH_ALEN);
                    aml_dbgfs_register_sta(aml_hw, sta);

                    /* Ensure that we won't process PS change or channel switch ind*/
                    spin_lock_bh(&aml_hw->cb_lock);
                    aml_txq_sta_init(aml_hw, sta, aml_txq_vif_get_status(aml_vif));
                    if (aml_vif->tdls_status == TDLS_SETUP_RSP_TX) {
                        aml_vif->tdls_status = TDLS_LINK_ACTIVE;
                        sta->tdls.initiator = true;
                        sta->tdls.active = true;
                    }
                    /* Set TDLS channel switch capability */
                    if ((params->ext_capab[3] & WLAN_EXT_CAPA4_TDLS_CHAN_SWITCH) &&
                        !aml_vif->tdls_chsw_prohibited)
                        sta->tdls.chsw_allowed = true;
                    aml_vif->sta.tdls_sta = sta;
                    sta->valid = true;
                    spin_unlock_bh(&aml_hw->cb_lock);
#ifdef CONFIG_AML_BFMER
                    if (aml_hw->mod_params->bfmer)
                        aml_send_bfmer_enable(aml_hw, sta, P_LINK_STA_PARAMS(params, vht_capa));

                    aml_mu_group_sta_init(sta, NULL);
#endif /* CONFIG_AML_BFMER */

                    #define PRINT_STA_FLAG(f)                               \
                        (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

                    netdev_info(dev, "Add %s TDLS sta %d (%pM) flags=%s%s%s%s%s%s%s",
                                sta->tdls.initiator ? "initiator" : "responder",
                                sta->sta_idx, mac,
                                PRINT_STA_FLAG(AUTHORIZED),
                                PRINT_STA_FLAG(SHORT_PREAMBLE),
                                PRINT_STA_FLAG(WME),
                                PRINT_STA_FLAG(MFP),
                                PRINT_STA_FLAG(AUTHENTICATED),
                                PRINT_STA_FLAG(TDLS_PEER),
                                PRINT_STA_FLAG(ASSOCIATED));
                    #undef PRINT_STA_FLAG

                    break;
                }
                default:
                    //error = -EBUSY;
                    break;
            }

            clear_bit(AML_DEV_ADDING_STA, &aml_hw->flags);
        } else  {
            return -EINVAL;
        }
    }

    if (params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED))
        aml_send_me_set_control_port_req(aml_hw,
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) != 0,
                sta->sta_idx);

    if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_MESH_POINT) {
        if (params->sta_modify_mask & STATION_PARAM_APPLY_PLINK_STATE) {
            if (params->plink_state < NUM_NL80211_PLINK_STATES) {
                aml_send_mesh_peer_update_ntf(aml_hw, vif, sta->sta_idx, params->plink_state);
            }
        }

        if (params->local_pm != NL80211_MESH_POWER_UNKNOWN) {
            sta->mesh_pm = params->local_pm;
            aml_update_mesh_power_mode(vif);
        }
    }

    if (params->vlan) {
        uint8_t vlan_idx;

        vif = netdev_priv(params->vlan);
        vlan_idx = vif->vif_index;

        if (sta->vlan_idx != vlan_idx) {
            struct aml_vif *old_vif;
            old_vif = aml_hw->vif_table[sta->vlan_idx];
            aml_txq_sta_switch_vif(sta, old_vif, vif);
            sta->vlan_idx = vlan_idx;

            if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP_VLAN) &&
                (vif->use_4addr)) {
                WARN((vif->ap_vlan.sta_4a),
                     "4A AP_VLAN interface with more than one sta");
                vif->ap_vlan.sta_4a = sta;
            }

            if ((AML_VIF_TYPE(old_vif) == NL80211_IFTYPE_AP_VLAN) &&
                (old_vif->use_4addr)) {
                old_vif->ap_vlan.sta_4a = NULL;
            }
        }
    }

    return 0;
}

/**
 * @start_ap: Start acting in AP mode defined by the parameters.
 */
int aml_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *dev,
                                  struct cfg80211_ap_settings *settings)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct apm_start_cfm apm_start_cfm;
    struct aml_sta *sta;
    int error = 0;
    struct aml_vif *vif = NULL;

    AML_INFO("center[%d %d],bw:%d",
        settings->chandef.center_freq1,
        settings->chandef.center_freq2,
        settings->chandef.width);

    if (aml_hw->scan_request) {
        vif = container_of(aml_hw->scan_request->wdev, struct aml_vif, wdev);
        error = aml_cancel_scan(aml_hw, vif);
        if (error) {
            printk("cancel scan fail:error = %d\n",error);
        }
    }
    if ((error = aml_send_apm_start_req(aml_hw, aml_vif, settings,
                                         &apm_start_cfm)))
        goto end;

    // Check the status
    switch (apm_start_cfm.status)
    {
        case CO_OK:
        {
            u8 txq_status = 0;
            aml_vif->ap.bcmc_index = apm_start_cfm.bcmc_idx;
            aml_vif->ap.flags = 0;
            aml_vif->ap.bcn_interval = settings->beacon_interval;
            sta = aml_hw->sta_table + apm_start_cfm.bcmc_idx;
            sta->valid = true;
            sta->aid = 0;
            sta->sta_idx = apm_start_cfm.bcmc_idx;
            sta->ch_idx = apm_start_cfm.ch_idx;
            sta->vif_idx = aml_vif->vif_index;
            sta->qos = false;
            sta->acm = 0;
            sta->ps.active = false;
            sta->listen_interval = 5;
            sta->ht = 0;
            sta->vht = 0;
            sta->he = 0;
            aml_mu_group_sta_init(sta, NULL);
            spin_lock_bh(&aml_hw->cb_lock);
            aml_chanctx_link(aml_vif, apm_start_cfm.ch_idx, &settings->chandef);
            if (aml_hw->cur_chanctx != apm_start_cfm.ch_idx) {
                txq_status = AML_TXQ_STOP_CHAN;
            }
            aml_txq_vif_init(aml_hw, aml_vif, txq_status);
            spin_unlock_bh(&aml_hw->cb_lock);

            netif_tx_start_all_queues(dev);
            netif_carrier_on(dev);
            error = 0;
            /* If the AP channel is already the active, we probably skip radar
               activation on MM_CHANNEL_SWITCH_IND (unless another vif use this
               ctxt). In anycase retest if radar detection must be activated
             */
            if (txq_status == 0) {
                aml_radar_detection_enable_on_cur_channel(aml_hw);
            }
#ifdef CONFIG_AML_RECOVERY
            if (!aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
                aml_recy_save_ap_info(settings);
            }
#endif
            break;
        }
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

end:
    if (error) {
        netdev_info(dev, "Failed to start AP (%d)", error);
    } else {
        netdev_info(dev, "AP started: ch=%d, bcmc_idx=%d",
                    aml_vif->ch_index, aml_vif->ap.bcmc_index);
#ifdef SCC_STA_SOFTAP
        aml_scc_init();
        aml_scc_check_chan_conflict(aml_hw);
#endif
    }
    if (vif != NULL) {
        aml_set_scan_hang(vif, 0, __func__, __LINE__);
    }
#ifdef CONFIG_AML_RECOVERY
    aml_recy_flags_clr(AML_RECY_OPEN_VIF_PROC);
#endif
    return error;
}

/**
 * @change_beacon: Change the beacon parameters for an access point mode
 *	interface. This should reject the call when AP mode wasn't started.
 */
static int aml_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
                                       struct cfg80211_ap_update *info)
#else
                                       struct cfg80211_beacon_data *info)
#endif
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_bcn *bcn = &vif->ap.bcn;
    struct aml_ipc_buf buf;
    u8 *bcn_buf;
    int error = 0;
    unsigned int addr;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    u8 *var_pos;
    u8 len;
    u8* htop_ie;
    u8 *ht_cap_ie;

    AML_DBG(AML_FN_ENTRY_STR);

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy_flags_chk(AML_RECY_GO_ONGOING) || aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        AML_INFO("status error:%x", aml_recy->flags);
        return -ENOMEM;
    }
#endif

    // Build the beacon
    bcn_buf = aml_build_bcn(bcn, info);
    if (!bcn_buf)
        return -ENOMEM;

#ifdef SCC_STA_SOFTAP
    aml_save_bcn_buf(bcn_buf, bcn->len);
#endif
    len = bcn->len - var_offset;
    var_pos = bcn_buf + var_offset;
    htop_ie = (u8*)cfg80211_find_ie(WLAN_EID_HT_OPERATION, var_pos, len);
    if (htop_ie && htop_ie[1] >= sizeof(struct ieee80211_ht_operation)) {
       struct ieee80211_ht_operation *htop = (void *)(htop_ie + 2);
       u8 cur_primary;
       if (aml_chanctx_valid(aml_hw, vif->ch_index) == 0) {
           AML_INFO("chanctx invalid");
           kfree(bcn_buf);
           return -ENOMEM;
       }

       cur_primary = aml_ieee80211_freq_to_chan(vif->aml_hw->chanctx_table[vif->ch_index].chan_def.chan->center_freq,
                                                    vif->aml_hw->chanctx_table[vif->ch_index].chan_def.chan->band);
       if (htop->primary_chan != cur_primary) {
           AML_INFO("primary chan error,ie:%d cur:%d", htop->primary_chan,cur_primary);
           kfree(bcn_buf);
           return -ENOMEM;
       }
    }

    ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, var_pos, len);
    if (ht_cap_ie) {
        struct ieee80211_ht_cap *ht_cap = ht_cap_ie + 2;
        ht_cap->cap_info &= ~(IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40);
        ht_cap->cap_info |= aml_hw->mod_params->sgi ? IEEE80211_HT_CAP_SGI_20 : 0;
        ht_cap->cap_info |= aml_hw->mod_params->use_2040 ? IEEE80211_HT_CAP_SGI_40 : 0;
    }

    // Sync buffer for FW
    if (aml_bus_type == PCIE_MODE) {
        if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, bcn_buf, bcn->len))) {
            netdev_err(dev, "Failed to allocate IPC buf for new beacon\n");
            kfree(bcn_buf);
            return error;
        }
    } else if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len, USB_EP4);
        kfree(bcn_buf);
    } else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len);
        kfree(bcn_buf);
    }

    // Forward the information to the LMAC
    error = aml_send_bcn_change(aml_hw, vif->vif_index, buf.dma_addr,
                                 bcn->len, bcn->head_len, bcn->tim_len, NULL);
#ifdef SCC_STA_SOFTAP
    if (!error) {
        AML_SCC_CLEAR_BEACON_UPDATE();
    }
#endif
    aml_ipc_buf_dealloc(aml_hw, &buf);
    return error;
}

/**
 * * @stop_ap: Stop being an AP, including stopping beaconing.
 */
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
static int aml_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev, unsigned int link_id)
#else
static int aml_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev)
#endif
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    int error = 0;
    struct aml_vif *vif = NULL;
    UNUSED_LINK_ID(link_id);
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        u8 cnt = 0;
        AML_INFO("recy ongoing!\n");
        while (aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
            msleep(20);
            if (cnt++ > 100) {
                AML_INFO("recy ongoing, can't stop ap!\n");
                return -EBUSY;
            }
        }
    }
    aml_recy_flags_set(AML_RECY_STOP_AP_PROC);
#endif

    //struct aml_sta *sta;
    if (aml_hw->scan_request) {
        vif = container_of(aml_hw->scan_request->wdev, struct aml_vif, wdev);
        error = aml_cancel_scan(aml_hw, vif);
        if (error) {
            printk("cancel scan fail:error = %d\n",error);
        }
    }
    aml_radar_cancel_cac(&aml_hw->radar);
    aml_send_apm_stop_req(aml_hw, aml_vif);
    spin_lock_bh(&aml_hw->cb_lock);
    aml_chanctx_unlink(aml_vif);
    spin_unlock_bh(&aml_hw->cb_lock);

    /* delete any remaining STA*/
    while (!list_empty(&aml_vif->ap.sta_list)) {
        aml_cfg80211_del_station(wiphy, dev, NULL);
    }

    /* delete BC/MC STA */
    aml_txq_vif_deinit(aml_hw, aml_vif);
    aml_del_bcn(&aml_vif->ap.bcn);
    aml_del_csa(aml_vif);
    aml_scc_deinit();
#ifdef CONFIG_AML_RECOVERY
    aml_recy_flags_clr(AML_RECY_AP_INFO_SAVED);
#endif

    netif_tx_stop_all_queues(dev);
    netif_carrier_off(dev);

    netdev_info(dev, "AP Stopped");
    if (vif != NULL) {
        aml_set_scan_hang(vif, 0, __func__, __LINE__);
    }
    return 0;
}

/**
 * @set_monitor_channel: Set the monitor mode channel for the device. If other
 *	interfaces are active this callback should reject the configuration.
 *	If no interfaces are active or the device is down, the channel should
 *	be stored for when a monitor interface becomes active.
 *
 * Also called internaly with chandef set to NULL simply to retrieve the channel
 * configured at firmware level.
 */
static int aml_cfg80211_set_monitor_channel(struct wiphy *wiphy,
                                             struct cfg80211_chan_def *chandef)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif;
    struct me_config_monitor_cfm cfm;
    AML_DBG(AML_FN_ENTRY_STR);

    if (aml_hw->monitor_vif == AML_INVALID_VIF)
        return -EINVAL;

    aml_vif = aml_hw->vif_table[aml_hw->monitor_vif];

    // Do nothing if monitor interface is already configured with the requested channel
    if (aml_chanctx_valid(aml_hw, aml_vif->ch_index)) {
        struct aml_chanctx *ctxt;
        ctxt = &aml_vif->aml_hw->chanctx_table[aml_vif->ch_index];
        if (chandef && cfg80211_chandef_identical(&ctxt->chan_def, chandef))
            return 0;
    }

    // Always send command to firmware. It allows to retrieve channel context index
    // and its configuration.
    if (aml_send_config_monitor_req(aml_hw, chandef, &cfm))
        return -EIO;

    // Always re-set channel context info
    aml_chanctx_unlink(aml_vif);



    // If there is also a STA interface not yet connected then monitor interface
    // will only have a channel context after the connection of the STA interface.
    if (cfm.chan_index != AML_CH_NOT_SET)
    {
        struct cfg80211_chan_def mon_chandef;

        if (aml_hw->vif_started > 1) {
            // In this case we just want to update the channel context index not
            // the channel configuration
            aml_chanctx_link(aml_vif, cfm.chan_index, NULL);
            return -EBUSY;
        }

        memset(&mon_chandef, 0, sizeof(mon_chandef));
        mon_chandef.chan = ieee80211_get_channel(wiphy, cfm.chan.prim20_freq);
        mon_chandef.center_freq1 = cfm.chan.center1_freq;
        mon_chandef.center_freq2 = cfm.chan.center2_freq;
        mon_chandef.width = (enum nl80211_chan_width)chnl2bw[cfm.chan.type];
        aml_chanctx_link(aml_vif, cfm.chan_index, &mon_chandef);
    }

    return 0;
}

/**
 * @probe_client: probe an associated client, must return a cookie that it
 *	later passes to cfg80211_probe_status().
 */
int aml_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *dev,
                               const u8 *peer, u64 *cookie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_sta *sta = NULL, *tmp;
    struct apm_probe_client_cfm cfm;

    if ((AML_VIF_TYPE(vif) != NL80211_IFTYPE_AP) &&
        (AML_VIF_TYPE(vif) != NL80211_IFTYPE_AP_VLAN) &&
        (AML_VIF_TYPE(vif) != NL80211_IFTYPE_P2P_GO) &&
        (AML_VIF_TYPE(vif) != NL80211_IFTYPE_MESH_POINT))
        return -EINVAL;

    list_for_each_entry_safe(sta, tmp, &vif->ap.sta_list, list) {
        if (sta->valid && ether_addr_equal(sta->mac_addr, peer))
            break;
    }

    if (!sta)
        return -EINVAL;

    aml_send_apm_probe_req(aml_hw, vif, sta, &cfm);

    if (cfm.status != CO_OK)
        return -EINVAL;

    *cookie = (u64)cfm.probe_id;
    return 0;
}

/**
 * @set_wiphy_params: Notify that wiphy parameters have changed;
 *	@changed bitfield (see &enum wiphy_params_flags) describes which values
 *	have changed. The actual parameter values are available in
 *	struct wiphy. If returning an error, no value should be changed.
 */
static int aml_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
    return 0;
}


/**
 * @set_tx_power: set the transmit power according to the parameters,
 *	the power passed is in mBm, to get dBm use MBM_TO_DBM(). The
 *	wdev may be %NULL if power was set for the wiphy, and will
 *	always be %NULL unless the driver supports per-vif TX power
 *	(as advertised by the nl80211 feature flag.)
 */
static int aml_cfg80211_set_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev,
                                      enum nl80211_tx_power_setting type, int mbm)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif;
    s8 pwr;
    int res = 0;

    if (type == NL80211_TX_POWER_AUTOMATIC) {
        pwr = 0x7f;
    } else {
        pwr = MBM_TO_DBM(mbm);
    }

    if (wdev) {
        vif = container_of(wdev, struct aml_vif, wdev);
        res = aml_send_set_power(aml_hw, vif->vif_index, pwr, NULL);
    } else {
        list_for_each_entry(vif, &aml_hw->vifs, list) {
            res = aml_send_set_power(aml_hw, vif->vif_index, pwr, NULL);
            if (res)
                break;
        }
    }

    return res;
}


/**
 * @set_power_mgmt: set the power save to one of those two modes:
 *  Power-save off
 *  Power-save on - Dynamic mode
 */
static int aml_cfg80211_set_power_mgmt(struct wiphy *wiphy,
                                        struct net_device *dev,
                                        bool enabled, int timeout)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    u8 ps_mode;

    AML_DBG(AML_FN_ENTRY_STR);
    if (timeout >= 0)
        netdev_info(dev, "Ignore timeout value %d", timeout);

    if (!(aml_hw->version_cfm.features & BIT(MM_FEAT_PS_BIT)))
        enabled = false;

    if (enabled) {
        /* Switch to Dynamic Power Save */
        ps_mode = MM_PS_MODE_ON_DYN;
    } else {
        /* Exit Power Save */
        ps_mode = MM_PS_MODE_OFF;
    }

    /* TODO:
     * Enable power save mode which will cause STA
     * connect/disconnect repeating.
     * For stability, temporarily closed power save mode.
     * */
    //return aml_send_me_set_ps_mode(aml_hw, ps_mode);
    return aml_send_me_set_ps_mode(aml_hw, MM_PS_MODE_OFF);
}


void aml_lp_shutdown_send_req(void)
{
    struct aml_hw *aml_hw = g_pst_aml_hw;
    int err = 0;

    err = aml_send_me_shutdown(aml_hw);
    if (err)
    {
        printk("shut_msg send fail! \n");
    }
}


/**
 * @set_txq_params: Set TX queue parameters
 */
static int aml_cfg80211_set_txq_params(struct wiphy *wiphy, struct net_device *dev,
                                        struct ieee80211_txq_params *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    u8 hw_queue, aifs, cwmin, cwmax;
    u32 param;

    AML_DBG(AML_FN_ENTRY_STR);

    hw_queue = aml_ac2hwq[0][params->ac];

    aifs  = params->aifs;
    cwmin = fls(params->cwmin);
    cwmax = fls(params->cwmax);

    /* Store queue information in general structure */
    param  = (u32) (aifs << 0);
    param |= (u32) (cwmin << 4);
    param |= (u32) (cwmax << 8);
    param |= (u32) (params->txop) << 12;

    /* Send the MM_SET_EDCA_REQ message to the FW */
    return aml_send_set_edca(aml_hw, hw_queue, param, false, aml_vif->vif_index);
}


/**
 * @remain_on_channel: Request the driver to remain awake on the specified
 *	channel for the specified duration to complete an off-channel
 *	operation (e.g., public action frame exchange). When the driver is
 *	ready on the requested channel, it must indicate this with an event
 *	notification by calling cfg80211_ready_on_channel().
 */
static int
aml_cfg80211_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
                                struct ieee80211_channel *chan,
                                unsigned int duration, u64 *cookie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(wdev->netdev);
    struct aml_vif *vif;
    struct aml_roc *roc;
    int error;

    if (aml_hw->scan_request) {
        vif = container_of(aml_hw->scan_request->wdev, struct aml_vif, wdev);
        AML_INFO("roc cancel scan,vif:%d\n",vif->vif_index);
        error = aml_cancel_scan(aml_hw, vif);
        if (error) {
            AML_INFO("cancel scan fail:error = %d\n",error);
        }
        aml_set_scan_hang(vif, 0, __func__, __LINE__);
    }

    /* For debug purpose (use ftrace kernel option) */
    trace_roc(aml_vif->vif_index, chan->center_freq, duration);

    /* Check that no other RoC procedure has been launched */
    if (aml_hw->roc) {
        AML_INFO("lunch new roc,cancel cur,cur_cookie:0x%llu \n", aml_hw->roc);
        if (aml_send_cancel_roc(aml_hw)) {
            return -EBUSY;
        }
        else {
            int count = 0;
            //wait for cancel roc suc
            while (aml_hw->roc) {
                msleep(10);
                if (count++ > 100) {
                    AML_INFO("wait cancel roc fail\n");
                    break;
                }
            }
        }
    }

    list_for_each_entry(vif, &aml_hw->vifs, list) {
        if (!vif->up || vif->ndev == NULL) {
            continue;
        }

        if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION ||
            AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT) {
            if (vif->sta.ap && vif->sta.ap->valid) {
                int count = 0;
                while (aml_connect_flags_chk(vif, AML_GETTING_IP)) {
                    msleep(10);
                    if (count++ > 100) {
                        AML_INFO("get ip timeout, drop ROC event\n");
                        return -EBUSY;
                    }
                }
            }
        }
    }

    /* Allocate a temporary RoC element */
    roc = kmalloc(sizeof(struct aml_roc), GFP_KERNEL);
    if (!roc) {
        AML_INFO("alloc error\n");
        return -ENOMEM;
    }
    /* Initialize the RoC information element */
    roc->vif = aml_vif;
    roc->chan = chan;
    roc->duration = duration;
    roc->internal = false;
    roc->on_chan = false;
    roc->tx_cnt = 0;
    memset(roc->tx_cookie, 0, sizeof(roc->tx_cookie));

    /* Initialize the OFFCHAN TX queue to allow off-channel transmissions */
    aml_txq_offchan_init(aml_vif);

    /* Forward the information to the FMAC */
    aml_hw->roc = roc;
    error = aml_send_roc(aml_hw, aml_vif, chan, duration);
    AML_INFO("center:%d duration:%d ret:%d cookie:0x%llu\n", chan->center_freq, duration, error, roc);

    if (error) {
        kfree(roc);
        aml_hw->roc = NULL;
        aml_txq_offchan_deinit(aml_vif);
    } else if (cookie)
        *cookie = (u64)roc;

    return error;
}

/**
 * @cancel_remain_on_channel: Cancel an on-going remain-on-channel operation.
 *	This allows the operation to be terminated prior to timeout based on
 *	the duration value.
 */
static int aml_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                                  struct wireless_dev *wdev,
                                                  u64 cookie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(wdev->netdev);

    AML_INFO("cookie:0x%llu,aml_hw->roc:0x%llu,is_sta_mode:%d", cookie,(u64)aml_hw->roc,aml_vif->is_sta_mode);

    trace_cancel_roc(aml_vif->vif_index);

    if (!aml_hw->roc)
        return 0;

    if (cookie != (u64)aml_hw->roc)
        return -EINVAL;

    if (aml_vif->is_sta_mode)
        return -EINVAL;
    /* Forward the information to the FMAC */
    return aml_send_cancel_roc(aml_hw);
}

/**
 * @dump_survey: get site survey information.
 */
static int aml_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *netdev,
                                     int idx, struct survey_info *info)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct ieee80211_supported_band *sband;
    struct aml_survey_info *aml_survey;

    //AML_DBG(AML_FN_ENTRY_STR);

    if (idx >= ARRAY_SIZE(aml_hw->survey))
        return -ENOENT;

    aml_survey = &aml_hw->survey[idx];

    // Check if provided index matches with a supported 2.4GHz channel
    sband = wiphy->bands[NL80211_BAND_2GHZ];
    if (sband && idx >= sband->n_channels) {
        idx -= sband->n_channels;
        sband = NULL;
    }

    if (!sband) {
        // Check if provided index matches with a supported 5GHz channel
        sband = wiphy->bands[NL80211_BAND_5GHZ];

        if (!sband || idx >= sband->n_channels)
            return -ENOENT;
    }

    // Fill the survey
    info->channel = &sband->channels[idx];
    info->filled = aml_survey->filled;

    if (aml_survey->filled != 0) {
        info->time = (u64)aml_survey->chan_time_ms;
        info->time_busy = (u64)aml_survey->chan_time_busy_ms;
        info->noise = aml_survey->noise_dbm;

        // TODO: clear survey after some time ?
    }

    return 0;
}

/**
 * @get_channel: Get the current operating channel for the virtual interface.
 *	For monitor interfaces, it should return %NULL unless there's a single
 *	current monitoring channel.
 */
static int aml_cfg80211_get_channel(struct wiphy *wiphy,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
                                     struct wireless_dev *wdev, unsigned int link_id,
#else
                                     struct wireless_dev *wdev,
#endif
                                     struct cfg80211_chan_def *chandef) {
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = container_of(wdev, struct aml_vif, wdev);
    struct aml_chanctx *ctxt;
    UNUSED_LINK_ID(link_id);

    if (!aml_vif->up) {
        return -ENODATA;
    }

    if (aml_vif->vif_index == aml_hw->monitor_vif)
    {
        //retrieve channel from firmware
        aml_cfg80211_set_monitor_channel(wiphy, NULL);
    }

    //Check if channel context is valid
    if (!aml_chanctx_valid(aml_hw, aml_vif->ch_index)) {
        return -ENODATA;
    }

    ctxt = &aml_hw->chanctx_table[aml_vif->ch_index];
    *chandef = ctxt->chan_def;

    return 0;
}

/**
 * @mgmt_tx: Transmit a management frame.
 */
static int aml_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
                                 struct cfg80211_mgmt_tx_params *params,
                                 u64 *cookie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(wdev->netdev);
    struct aml_sta *aml_sta;
    struct ieee80211_mgmt *mgmt = (void *)params->buf;
    bool ap = false;
    bool offchan = false;
    int res;

    switch (AML_VIF_TYPE(aml_vif)) {
        case NL80211_IFTYPE_AP_VLAN:
            aml_vif = aml_vif->ap_vlan.master;
        case NL80211_IFTYPE_AP:
        case NL80211_IFTYPE_P2P_GO:
        case NL80211_IFTYPE_MESH_POINT:
            ap = true;
            break;
        case NL80211_IFTYPE_STATION:
        case NL80211_IFTYPE_P2P_CLIENT:
        default:
            break;
    }

    // Get STA on which management frame has to be sent
    aml_sta = aml_retrieve_sta(aml_hw, aml_vif, mgmt->da,
                                 mgmt->frame_control, ap);

    if (params->offchan) {
        if (!params->chan) {
            AML_INFO("error");
            return -EINVAL;
        }
        offchan = true;
        if (aml_chanctx_valid(aml_hw, aml_vif->ch_index)) {
            struct aml_chanctx *ctxt = &aml_hw->chanctx_table[aml_vif->ch_index];
            if (ctxt->chan_def.chan->center_freq == params->chan->center_freq)
                offchan = false;
        }
    }

    trace_mgmt_tx((offchan) ? params->chan->center_freq : 0,
                  aml_vif->vif_index, (aml_sta) ? aml_sta->sta_idx : 0xFF,
                  mgmt);

    if (offchan) {
        struct aml_roc *roc = aml_hw->roc;
        bool lunch_roc = false;
        // Offchannel transmission, need to start a RoC
        if (roc) {
            // Test if current RoC can be re-used
            if ((roc->vif != aml_vif) ||
                (roc->chan->center_freq != params->chan->center_freq)) {
                AML_INFO("roc chan=%d, params chan=%d\n", roc->chan->center_freq, params->chan->center_freq);
                lunch_roc = true;
            }
        }

        if (lunch_roc || !roc) {
            int error;
            unsigned int duration = 30;

            /* Start a new ROC procedure */
            if (params->wait)
                duration = params->wait;

            AML_INFO("lunch roc for mgmt tx, dur:%d chan:%d\n", duration, params->chan->center_freq);
            error = aml_cfg80211_remain_on_channel(wiphy, wdev, params->chan,
                                                    duration, NULL);
            if (error) {
                AML_INFO("error");
                return error;
            }
            // internal RoC, no need to inform user space about it
            spin_lock_bh(&aml_hw->roc_lock);
            if (aml_hw->roc) {
                aml_hw->roc->internal = true;
            }
            spin_unlock_bh(&aml_hw->roc_lock);
        }
    }

    if (ieee80211_is_assoc_resp(mgmt->frame_control) || ieee80211_is_reassoc_resp(mgmt->frame_control))
    {
        const u8 *ht_cap_ie;
        int var_offset = offsetof(struct ieee80211_mgmt, u.assoc_resp.variable);
        ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, params->buf + var_offset, params->len - var_offset);
        if (ht_cap_ie && !memcmp(mgmt->da, aml_hw->rx_assoc_info.addr, ETH_ALEN)) {
            struct ieee80211_ht_cap *ht_cap = ht_cap_ie + 2;

            ht_cap->cap_info &= ~(IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40);
            ht_cap->cap_info |= aml_hw->mod_params->sgi ? IEEE80211_HT_CAP_SGI_20 : 0;
            ht_cap->cap_info |= aml_hw->mod_params->use_2040 ? IEEE80211_HT_CAP_SGI_40 : 0;
        }
    }

    if (ieee80211_is_probe_resp(mgmt->frame_control))
    {
        const u8 *ht_cap_ie;
        int var_offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);

        ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, params->buf + var_offset, params->len - var_offset);
        if (ht_cap_ie) {
            struct ieee80211_ht_cap *ht_cap = ht_cap_ie + 2;

            ht_cap->cap_info &= ~(IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40);
            ht_cap->cap_info |= aml_hw->mod_params->sgi ? IEEE80211_HT_CAP_SGI_20 : 0;
            ht_cap->cap_info |= aml_hw->mod_params->use_2040 ? IEEE80211_HT_CAP_SGI_40 : 0;
        }
    }

    res = aml_start_mgmt_xmit(aml_vif, aml_sta, params, offchan, cookie);
    spin_lock_bh(&aml_hw->roc_lock);
    if (offchan && aml_hw->roc) {
        if (aml_hw->roc->tx_cnt < NX_ROC_TX)
            aml_hw->roc->tx_cookie[aml_hw->roc->tx_cnt] = *cookie;
        else
            wiphy_warn(wiphy, "%d frames sent within the same Roc (> NX_ROC_TX)",
                       aml_hw->roc->tx_cnt + 1);
        aml_hw->roc->tx_cnt++;
    }
    spin_unlock_bh(&aml_hw->roc_lock);
    if ((ieee80211_is_deauth(mgmt->frame_control)) && (res == 0)) {
        u8 cnt = 0;
        aml_vif->is_disconnect = 1;
        printk("send deauth, vif_idx:%d\n", aml_vif->vif_index);
        while (aml_vif->is_disconnect) {
            msleep(20);
            if (cnt++ > 20) {
                aml_vif->is_disconnect = 0;
                printk("deauth send fail\n");
                return res;
            }
        }
    }

    return res;
}

int aml_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    int i, nb_tx_cookie = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    if (!aml_hw->roc || !aml_hw->roc->tx_cnt)
        return 0;

    for (i = 0; i < NX_ROC_TX; i++) {
        if (!aml_hw->roc->tx_cookie[i])
            continue;

        nb_tx_cookie++;
        if (aml_hw->roc->tx_cookie[i] == cookie) {
            aml_hw->roc->tx_cookie[i] = 0;
            aml_hw->roc->tx_cnt--;
            break;
        }
    }

    if (i == NX_ROC_TX) {
        // Didn't find the cookie but this frame may still have been sent within this
        // Roc if more than NX_ROC_TX frame have been sent
        if (nb_tx_cookie != aml_hw->roc->tx_cnt)
            aml_hw->roc->tx_cnt--;
        else
            return 0;
    }

    // Stop the RoC if started to send TX frame and all frames have been "wait cancelled"
    if ((!aml_hw->roc->internal) || (aml_hw->roc->tx_cnt > 0))
        return 0;

    return aml_cfg80211_cancel_remain_on_channel(wiphy, wdev, (u64)aml_hw->roc);
}

/**
 * @start_radar_detection: Start radar detection in the driver.
 */
static int aml_cfg80211_start_radar_detection(struct wiphy *wiphy,
                                               struct net_device *dev,
                                               struct cfg80211_chan_def *chandef,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
                                               u32 cac_time_ms, int link_id)
#else
                                               u32 cac_time_ms)
#endif
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct apm_start_cac_cfm cfm;

    aml_radar_start_cac(&aml_hw->radar, cac_time_ms, aml_vif);
    aml_send_apm_start_cac_req(aml_hw, aml_vif, chandef, &cfm);

    if (cfm.status == CO_OK) {
        spin_lock_bh(&aml_hw->cb_lock);
        aml_chanctx_link(aml_vif, cfm.ch_idx, chandef);
        if (aml_hw->cur_chanctx == aml_vif->ch_index)
            aml_radar_detection_enable(&aml_hw->radar,
                                        AML_RADAR_DETECT_REPORT,
                                        AML_RADAR_RIU);
        spin_unlock_bh(&aml_hw->cb_lock);
    } else {
        return -EIO;
    }

    return 0;
}

/**
 * @update_ft_ies: Provide updated Fast BSS Transition information to the
 *	driver. If the SME is in the driver/firmware, this information can be
 *	used in building Authentication and Reassociation Request frames.
 */
static int aml_cfg80211_update_ft_ies(struct wiphy *wiphy,
                                       struct net_device *dev,
                                       struct cfg80211_update_ft_ies_params *ftie)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    const struct element *rsne = NULL, *mde = NULL, *fte = NULL, *elem;
    bool ft_in_non_rsn = false;
    int fties_len = 0;
    u8 *ft_assoc_ies, *pos;

    if (AML_VIF_TYPE(vif) != NL80211_IFTYPE_STATION)
        return 0;

    if (!vif->sta.ft_assoc_ies) {
        AML_INFO("save ft_assoc_ies\n");
        vif->sta.ft_assoc_ies = kmalloc(ftie->ie_len, GFP_KERNEL);
        if (!vif->sta.ft_assoc_ies)
            return 0;
        memcpy(vif->sta.ft_assoc_ies, ftie->ie, ftie->ie_len);
        vif->sta.ft_assoc_ies_len = ftie->ie_len;
        return 0;
    }

    for_each_element(elem, ftie->ie, ftie->ie_len) {
        if (elem->id == WLAN_EID_RSN)
            rsne = elem;
        else if (elem->id == WLAN_EID_MOBILITY_DOMAIN)
            mde = elem;
        else if (elem->id == WLAN_EID_FAST_BSS_TRANSITION)
            fte = elem;
        else
            netdev_warn(dev, "Unexpected FT element %d\n", elem->id);
    }
    if (!mde) {
        // maybe just test MDE for
        netdev_warn(dev, "Didn't find Mobility_Domain Element\n");
        return 0;
    } else if (!rsne && !fte) {
        // not sure this happen in real life ...
        ft_in_non_rsn = true;
    } else if (!rsne || !fte) {
        netdev_warn(dev, "Didn't find RSN or Fast Transition Element\n");
        return 0;
    }

    for_each_element(elem, vif->sta.ft_assoc_ies, vif->sta.ft_assoc_ies_len) {
        if ((elem->id == WLAN_EID_RSN) ||
            (elem->id == WLAN_EID_MOBILITY_DOMAIN) ||
            (elem->id == WLAN_EID_FAST_BSS_TRANSITION))
            fties_len += elem->datalen + sizeof(struct element);
    }

    ft_assoc_ies = kmalloc(vif->sta.ft_assoc_ies_len - fties_len + ftie->ie_len,
                        GFP_KERNEL);
    if (!ft_assoc_ies) {
        netdev_warn(dev, "Fail to allocate buffer for association elements");
        return 0;
    }

    // Recopy current Association Elements one at a time and replace FT
    // element with updated version.
    pos = ft_assoc_ies;
    for_each_element(elem, vif->sta.ft_assoc_ies, vif->sta.ft_assoc_ies_len) {
        if (elem->id == WLAN_EID_RSN) {
            if (ft_in_non_rsn) {
                netdev_warn(dev, "Found RSN element in non RSN FT");
                goto abort;
            } else if (!rsne) {
                netdev_warn(dev, "Found several RSN element");
                goto abort;
            } else {
                memcpy(pos, rsne, sizeof(*rsne) + rsne->datalen);
                pos += sizeof(*rsne) + rsne->datalen;
                rsne = NULL;
            }
        } else if (elem->id == WLAN_EID_MOBILITY_DOMAIN) {
            if (!mde) {
                netdev_warn(dev, "Found several Mobility Domain element");
                goto abort;
            } else {
                memcpy(pos, mde, sizeof(*mde) + mde->datalen);
                pos += sizeof(*mde) + mde->datalen;
                mde = NULL;
            }
        }
        else if (elem->id == WLAN_EID_FAST_BSS_TRANSITION) {
            if (ft_in_non_rsn) {
                netdev_warn(dev, "Found Fast Transition element in non RSN FT");
                goto abort;
            } else if (!fte) {
                netdev_warn(dev, "found several Fast Transition element");
                goto abort;
            } else {
                memcpy(pos, fte, sizeof(*fte) + fte->datalen);
                pos += sizeof(*fte) + fte->datalen;
                fte = NULL;
            }
        }
        else {
            // Put FTE after MDE if non present in Association Element
            if (fte && !mde) {
                memcpy(pos, fte, sizeof(*fte) + fte->datalen);
                pos += sizeof(*fte) + fte->datalen;
                fte = NULL;
            }
            memcpy(pos, elem, sizeof(*elem) + elem->datalen);
            pos += sizeof(*elem) + elem->datalen;
        }
    }
    if (fte) {
        memcpy(pos, fte, sizeof(*fte) + fte->datalen);
        pos += sizeof(*fte) + fte->datalen;
        //fte = NULL;
    }

    kfree(vif->sta.ft_assoc_ies);
    vif->sta.ft_assoc_ies = ft_assoc_ies;
    vif->sta.ft_assoc_ies_len = pos - ft_assoc_ies;

    if (vif->sta.flags & AML_STA_FT_OVER_DS) {
        struct sm_connect_cfm sm_connect_cfm;
        struct cfg80211_connect_params sme;

        memset(&sme, 0, sizeof(sme));
        rsne = cfg80211_find_elem(WLAN_EID_RSN, vif->sta.ft_assoc_ies,
                                  vif->sta.ft_assoc_ies_len);
        if (rsne && aml_rsne_to_connect_params(rsne, &sme)) {
            netdev_warn(dev, "FT RSN parsing failed\n");
            return 0;
        }

        sme.ssid_len = vif->sta.ft_assoc_ies[1];
        sme.ssid = &vif->sta.ft_assoc_ies[2];
        sme.bssid = vif->sta.ft_target_ap;
        sme.ie = &vif->sta.ft_assoc_ies[2 + sme.ssid_len];
        sme.ie_len = vif->sta.ft_assoc_ies_len - (2 + sme.ssid_len);
        sme.auth_type = NL80211_AUTHTYPE_FT;
        aml_send_sm_connect_req(aml_hw, vif, &sme, &sm_connect_cfm);
        vif->sta.flags &= ~AML_STA_FT_OVER_DS;

    } else if (vif->sta.flags & AML_STA_FT_OVER_AIR) {
        uint8_t ssid_len;
        vif->sta.flags &= ~AML_STA_FT_OVER_AIR;

        // Skip the first element (SSID)
        ssid_len = vif->sta.ft_assoc_ies[1] + 2;
        if (aml_send_sm_ft_auth_rsp(aml_hw, vif, &vif->sta.ft_assoc_ies[ssid_len],
                                     vif->sta.ft_assoc_ies_len - ssid_len))
            netdev_err(dev, "FT Over Air: Failed to send updated assoc elem\n");
    }

    return 0;

abort:
    kfree(ft_assoc_ies);
    return 0;
}

/**
 * @set_cqm_rssi_config: Configure connection quality monitor RSSI threshold.
 */
static int aml_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy,
                                             struct net_device *dev,
                                             int32_t rssi_thold, uint32_t rssi_hyst)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);

    return aml_send_cfg_rssi_req(aml_hw, aml_vif->vif_index, rssi_thold, rssi_hyst);
}

/**
 *
 * @channel_switch: initiate channel-switch procedure (with CSA). Driver is
 *	responsible for veryfing if the switch is possible. Since this is
 *	inherently tricky driver may decide to disconnect an interface later
 *	with cfg80211_stop_iface(). This doesn't mean driver can accept
 *	everything. It should do it's best to verify requests and reject them
 *	as soon as possible.
 */
static int aml_cfg80211_channel_switch(struct wiphy *wiphy,
                                        struct net_device *dev,
                                        struct cfg80211_csa_settings *params)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_ipc_buf buf;
    struct aml_bcn *bcn, *bcn_after;
    struct aml_csa *csa;
    u16 csa_oft[BCN_MAX_CSA_CPT];
    u8 *bcn_buf;
    int i, error = 0;
    unsigned int addr;

    if (vif->ap.csa)
        return -EBUSY;

    if (params->n_counter_offsets_beacon > BCN_MAX_CSA_CPT)
        return -EINVAL;

    /* Build the new beacon with CSA IE */
    bcn = &vif->ap.bcn;
    bcn_buf = aml_build_bcn(bcn, &params->beacon_csa);
    if (!bcn_buf)
        return -ENOMEM;

    memset(csa_oft, 0, sizeof(csa_oft));
    for (i = 0; i < params->n_counter_offsets_beacon; i++)
    {
        csa_oft[i] = params->counter_offsets_beacon[i] + bcn->head_len +
            bcn->tim_len;
    }

    /* If count is set to 0 (i.e anytime after this beacon) force it to 2 */
    if (params->count == 0) {
        params->count = 2;
        for (i = 0; i < params->n_counter_offsets_beacon; i++)
        {
            bcn_buf[csa_oft[i]] = 2;
        }
    }

    if (aml_bus_type == PCIE_MODE) {
        if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, bcn_buf, bcn->len))) {
            netdev_err(dev, "Failed to allocate IPC buf for CSA beacon\n");
            kfree(bcn_buf);
            return error;
        }
    } else if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len, USB_EP4);
        kfree(bcn_buf);
    } else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len);
        kfree(bcn_buf);
    }

    /* Build the beacon to use after CSA. It will only be sent to fw once
       CSA is over, but do it before sending the beacon as it must be ready
       when CSA is finished. */
    csa = kzalloc(sizeof(struct aml_csa), GFP_KERNEL);
    if (!csa) {
        error = -ENOMEM;
        goto end;
    }
    memset(csa, 0, sizeof(struct aml_csa));
    bcn_after = &csa->bcn;
    bcn_buf = aml_build_bcn(bcn_after, &params->beacon_after);
    if (!bcn_buf) {
        error = -ENOMEM;
        aml_del_csa(vif);
        goto end;
    }

    if ((error = aml_ipc_buf_a2e_init(aml_hw, &csa->buf, bcn_buf, bcn_after->len))) {
        netdev_err(dev, "Failed to allocate IPC buf for after CSA beacon\n");
        kfree(bcn_buf);
        goto end;
    }

    vif->ap.csa = csa;
    csa->vif = vif;
    csa->chandef = params->chandef;

    /* Send new Beacon. FW will extract channel and count from the beacon */
    error = aml_send_bcn_change(aml_hw, vif->vif_index, buf.dma_addr,
                                 bcn->len, bcn->head_len, bcn->tim_len, csa_oft);

    if (error) {
        aml_del_csa(vif);
    } else {
        INIT_WORK(&csa->work, aml_csa_finish);
#ifndef CONFIG_PT_MODE
        aml_cfg80211_ch_switch_started_notify(dev, &csa->chandef, 0, params->count, params->block_tx);
#endif
    }

  end:
    aml_ipc_buf_dealloc(aml_hw, &buf);
    /* coverity[leaked_storage] - csa have added to list */
    return error;
}


/**
 * @@tdls_mgmt: Transmit a TDLS management frame.
 */
static int aml_cfg80211_tdls_mgmt(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
                                   const u8 *peer, int link_id, u8 action_code,  u8 dialog_token,
#else
                                   const u8 *peer, u8 action_code,  u8 dialog_token,
#endif
                                   u16 status_code, u32 peer_capability,
                                   bool initiator, const u8 *buf, size_t len)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    int ret = 0;

    /* make sure we support TDLS */
    if (!(wiphy->flags & WIPHY_FLAG_SUPPORTS_TDLS))
        return -ENOTSUPP;

    /* make sure we are in station mode (and connected) */
    if ((AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_STATION) ||
        (!aml_vif->up) || (!aml_vif->sta.ap))
        return -ENOTSUPP;

    /* only one TDLS link is supported */
    if ((action_code == WLAN_TDLS_SETUP_REQUEST) &&
        (aml_vif->sta.tdls_sta) &&
        (aml_vif->tdls_status == TDLS_LINK_ACTIVE)) {
        printk("%s: only one TDLS link is supported!\n", __func__);
        return -ENOTSUPP;
    }

    if ((action_code == WLAN_TDLS_DISCOVERY_REQUEST) &&
        (aml_hw->mod_params->ps_on)) {
        printk("%s: discovery request is not supported when "
                "power-save is enabled!\n", __func__);
        return -ENOTSUPP;
    }

    switch (action_code) {
    case WLAN_TDLS_SETUP_RESPONSE:
        /* only one TDLS link is supported */
        if ((status_code == 0) &&
            (aml_vif->sta.tdls_sta) &&
            (aml_vif->tdls_status == TDLS_LINK_ACTIVE)) {
            printk("%s: only one TDLS link is supported!\n", __func__);
            status_code = WLAN_STATUS_REQUEST_DECLINED;
        }
        /* fall-through */
    case WLAN_TDLS_SETUP_REQUEST:
    case WLAN_TDLS_TEARDOWN:
    case WLAN_TDLS_DISCOVERY_REQUEST:
    case WLAN_TDLS_SETUP_CONFIRM:
    case WLAN_PUB_ACTION_TDLS_DISCOVER_RES:
        ret = aml_tdls_send_mgmt_packet_data(aml_hw, aml_vif, peer, action_code,
                dialog_token, status_code, peer_capability, initiator, buf, len, 0, NULL);
        break;

    default:
        printk("%s: Unknown TDLS mgmt/action frame %pM\n",
                __func__, peer);
        ret = -EOPNOTSUPP;
        break;
    }

    if (action_code == WLAN_TDLS_SETUP_REQUEST) {
        aml_vif->tdls_status = TDLS_SETUP_REQ_TX;
    } else if (action_code == WLAN_TDLS_SETUP_RESPONSE) {
        aml_vif->tdls_status = TDLS_SETUP_RSP_TX;
    } else if ((action_code == WLAN_TDLS_SETUP_CONFIRM) && (ret == CO_OK)) {
        aml_vif->tdls_status = TDLS_LINK_ACTIVE;
        /* Set TDLS active */
        aml_vif->sta.tdls_sta->tdls.active = true;
    }

    return ret;
}

/**
 * @tdls_oper: Perform a high-level TDLS operation (e.g. TDLS link setup).
 */
static int aml_cfg80211_tdls_oper(struct wiphy *wiphy, struct net_device *dev,
                                   const u8 *peer, enum nl80211_tdls_operation oper)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    int error;

    if (oper != NL80211_TDLS_DISABLE_LINK)
        return 0;

    if (!aml_vif->sta.tdls_sta) {
        printk("%s: TDLS station %pM does not exist\n", __func__, peer);
        return -ENOLINK;
    }

    if (memcmp(aml_vif->sta.tdls_sta->mac_addr, peer, ETH_ALEN) == 0) {
        /* Disable Channel Switch */
        if (!aml_send_tdls_cancel_chan_switch_req(aml_hw, aml_vif,
                                                   aml_vif->sta.tdls_sta,
                                                   NULL))
            aml_vif->sta.tdls_sta->tdls.chsw_en = false;

        netdev_info(dev, "Del TDLS sta %d (%pM)",
                aml_vif->sta.tdls_sta->sta_idx,
                aml_vif->sta.tdls_sta->mac_addr);
        /* Ensure that we won't process PS change ind */
        spin_lock_bh(&aml_hw->cb_lock);
        aml_vif->sta.tdls_sta->ps.active = false;
        aml_vif->sta.tdls_sta->valid = false;
        spin_unlock_bh(&aml_hw->cb_lock);
        aml_txq_sta_deinit(aml_hw, aml_vif->sta.tdls_sta);
        error = aml_send_me_sta_del(aml_hw, aml_vif->sta.tdls_sta->sta_idx, true);
        if ((error != 0) && (error != -EPIPE))
            return error;

#ifdef CONFIG_AML_BFMER
        // Disable Beamformer if supported
        aml_bfmer_report_del(aml_hw, aml_vif->sta.tdls_sta);
        aml_mu_group_sta_del(aml_hw, aml_vif->sta.tdls_sta);
#endif /* CONFIG_AML_BFMER */

        /* Set TDLS not active */
        aml_vif->sta.tdls_sta->tdls.active = false;
        aml_dbgfs_unregister_sta(aml_hw, aml_vif->sta.tdls_sta);
        // Remove TDLS station
        aml_vif->tdls_status = TDLS_LINK_IDLE;
        aml_vif->sta.tdls_sta = NULL;
    }

    return 0;
}

/**
 *  @tdls_channel_switch: Start channel-switching with a TDLS peer. The driver
 *	is responsible for continually initiating channel-switching operations
 *	and returning to the base channel for communication with the AP.
 */
static int aml_cfg80211_tdls_channel_switch(struct wiphy *wiphy,
                                             struct net_device *dev,
                                             const u8 *addr, u8 oper_class,
                                             struct cfg80211_chan_def *chandef)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_sta *aml_sta = aml_vif->sta.tdls_sta;
    struct tdls_chan_switch_cfm cfm;
    int error;

    if ((!aml_sta) || (memcmp(addr, aml_sta->mac_addr, ETH_ALEN))) {
        printk("%s: TDLS station %pM doesn't exist\n", __func__, addr);
        return -ENOLINK;
    }

    if (!aml_sta->tdls.chsw_allowed) {
        printk("%s: TDLS station %pM does not support TDLS channel switch\n", __func__, addr);
        return -ENOTSUPP;
    }

    error = aml_send_tdls_chan_switch_req(aml_hw, aml_vif, aml_sta,
                                           aml_sta->tdls.initiator,
                                           oper_class, chandef, &cfm);
    if (error)
        return error;

    if (!cfm.status) {
        aml_sta->tdls.chsw_en = true;
        return 0;
    } else {
        printk("%s: TDLS channel switch already enabled and only one is supported\n", __func__);
        return -EALREADY;
    }
}

/**
 * @tdls_cancel_channel_switch: Stop channel-switching with a TDLS peer. Both
 *	peers must be on the base channel when the call completes.
 */
static void aml_cfg80211_tdls_cancel_channel_switch(struct wiphy *wiphy,
                                                     struct net_device *dev,
                                                     const u8 *addr)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_sta *aml_sta = aml_vif->sta.tdls_sta;
    struct tdls_cancel_chan_switch_cfm cfm;

    if (!aml_sta)
        return;

    if (!aml_send_tdls_cancel_chan_switch_req(aml_hw, aml_vif,
                                               aml_sta, &cfm))
        aml_sta->tdls.chsw_en = false;
}

/**
 * @change_bss: Modify parameters for a given BSS (mainly for AP mode).
 */
static int aml_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev,
                                    struct bss_parameters *params)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    int res =  -EOPNOTSUPP;

    if (((AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) ||
         (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO)) &&
        (params->ap_isolate > -1)) {

        if (params->ap_isolate)
            aml_vif->ap.flags |= AML_AP_ISOLATE;
        else
            aml_vif->ap.flags &= ~AML_AP_ISOLATE;

        res = 0;
    }

    return res;
}

static int aml_sta_rate_info(struct aml_vif *vif, struct aml_sta *sta, u8_l* temp_mcs ,u8_l* temp_nss)
{
    struct aml_hw *aml_hw = vif->aml_hw;
    struct me_rc_stats_cfm me_rc_stats_cfm;
    unsigned int no_samples;
    int i = 0;
    int error = 0;

     /* Forward the information to the LMAC */
    if ((error = aml_send_me_rc_stats(aml_hw, sta->sta_idx, &me_rc_stats_cfm)))
        return error;

    no_samples = me_rc_stats_cfm.no_samples;
    if (no_samples == 0)
        return 0;

    for (i = 0; i < no_samples; i++) {
        if (me_rc_stats_cfm.retry_step_idx[0] == i) {
            union aml_rate_ctrl_info *r_cfg = (union aml_rate_ctrl_info *)&(me_rc_stats_cfm.rate_stats[i].rate_config);
            union aml_mcs_index *mcs_index = (union aml_mcs_index *)&(me_rc_stats_cfm.rate_stats[i].rate_config);
            unsigned int ft, nss, mcs;

            ft = r_cfg->formatModTx;
            if (ft >= FORMATMOD_HE_SU) {
                mcs = mcs_index->he.mcs;
                nss = mcs_index->he.nss;
            } else if (ft == FORMATMOD_VHT) {
                mcs = mcs_index->vht.mcs;
                nss = mcs_index->vht.nss;
            } else if (ft >= FORMATMOD_HT_MF) {
                mcs = mcs_index->ht.mcs;
                nss = mcs_index->ht.nss;
                mcs += nss * 8;
            } else {
                mcs = mcs_index->legacy;
                nss = 0;
                if (mcs >= 4) {
                    mcs -= 4;
                }
            }
            *temp_mcs = mcs;
            *temp_nss = nss;
        }
    }
    return 0;
}

static int aml_fill_station_info(struct aml_sta *sta, struct aml_vif *vif,
                                  struct station_info *sinfo)
{
    struct aml_sta_stats *stats = &sta->stats;
    struct aml_hw *aml_hw = vif->aml_hw;
    struct aml_plat *aml_plat = vif->aml_hw->plat;
    struct rx_vector_1 *rx_vect1 = &stats->last_rx.rx_vect1;

    // Generic info
    rx_vect1->rssi1 = (AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, REG_OF_SYNC_RSSI) & 0xffff) - 256;
    aml_set_tcp_ack_accord_to_rssi(sta, aml_hw, rx_vect1->rssi1);
    sinfo->generation = vif->generation;
    sinfo->inactive_time = jiffies_to_msecs(jiffies - stats->last_act);
    sinfo->rx_bytes = stats->rx_bytes;
    sinfo->tx_bytes = stats->tx_bytes;
    sinfo->tx_packets = stats->tx_pkts;
    sinfo->rx_packets = stats->rx_pkts;
    sinfo->signal = rx_vect1->rssi1;
    sinfo->tx_failed = stats->tx_fails;
    switch (rx_vect1->ch_bw) {
        case PHY_CHNL_BW_20:
            sinfo->rxrate.bw = RATE_INFO_BW_20;
            break;
        case PHY_CHNL_BW_40:
            sinfo->rxrate.bw = RATE_INFO_BW_40;
            break;
        case PHY_CHNL_BW_80:
            sinfo->rxrate.bw = RATE_INFO_BW_80;
            break;
        case PHY_CHNL_BW_160:
            sinfo->rxrate.bw = RATE_INFO_BW_160;
            break;
        default:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) || (defined CONFIG_KERNEL_AX_PATCH)
            sinfo->rxrate.bw = RATE_INFO_BW_HE_RU;
#endif
            break;
    }
    switch (rx_vect1->format_mod) {
        case FORMATMOD_NON_HT:
        case FORMATMOD_NON_HT_DUP_OFDM:
            sinfo->rxrate.flags = 0;
            sinfo->rxrate.legacy = legrates_lut[rx_vect1->leg_rate].rate;
            break;
        case FORMATMOD_HT_MF:
        case FORMATMOD_HT_GF:
            sinfo->rxrate.flags = RATE_INFO_FLAGS_MCS;
            if (rx_vect1->ht.short_gi)
                sinfo->rxrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
            sinfo->rxrate.mcs = rx_vect1->ht.mcs;
            break;
        case FORMATMOD_VHT:
            sinfo->rxrate.flags = RATE_INFO_FLAGS_VHT_MCS;
            if (rx_vect1->vht.short_gi)
                sinfo->rxrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
            sinfo->rxrate.mcs = rx_vect1->vht.mcs;
            sinfo->rxrate.nss = rx_vect1->vht.nss + 1;
            break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
        case FORMATMOD_HE_MU:
            sinfo->rxrate.he_ru_alloc = rx_vect1->he.ru_size;
        case FORMATMOD_HE_SU:
        case FORMATMOD_HE_ER:
        case FORMATMOD_HE_TB:
            sinfo->rxrate.flags = RATE_INFO_FLAGS_HE_MCS;
            sinfo->rxrate.mcs = rx_vect1->he.mcs;
            sinfo->rxrate.nss = rx_vect1->he.nss + 1;
            sinfo->rxrate.he_gi = rx_vect1->he.gi_type;
            sinfo->rxrate.he_dcm = rx_vect1->he.dcm;
            break;
#endif
        default :
            return -EINVAL;
    }
    sinfo->filled = (BIT(NL80211_STA_INFO_INACTIVE_TIME) |
                     BIT(NL80211_STA_INFO_RX_BYTES64)    |
                     BIT(NL80211_STA_INFO_TX_BYTES64)    |
                     BIT(NL80211_STA_INFO_RX_PACKETS)    |
                     BIT(NL80211_STA_INFO_TX_PACKETS)    |
                     BIT(NL80211_STA_INFO_TX_FAILED)     |
                     BIT(NL80211_STA_INFO_SIGNAL)        |
                     BIT(NL80211_STA_INFO_RX_BITRATE));

    if (0) {
        aml_sta_rate_info(vif, sta, &stats->mcs_max, &stats->no_ss);
    }
    switch (stats->bw_max) {
        case PHY_CHNL_BW_20:
            sinfo->txrate.bw = RATE_INFO_BW_20;
            break;
        case PHY_CHNL_BW_40:
            sinfo->txrate.bw = RATE_INFO_BW_40;
            break;
        case PHY_CHNL_BW_80:
            sinfo->txrate.bw = RATE_INFO_BW_80;
            break;
        case PHY_CHNL_BW_160:
            sinfo->txrate.bw = RATE_INFO_BW_160;
            break;
        default:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) || (defined CONFIG_KERNEL_AX_PATCH)
            sinfo->txrate.bw = RATE_INFO_BW_HE_RU;
#endif
            break;
    }
     switch (stats->format_mod) {
        case FORMATMOD_NON_HT:
        case FORMATMOD_NON_HT_DUP_OFDM:
            sinfo->txrate.flags = 0;
            sinfo->txrate.legacy = stats->leg_rate;
            break;
        case FORMATMOD_HT_MF:
        case FORMATMOD_HT_GF:
            sinfo->txrate.flags = RATE_INFO_FLAGS_MCS;
            if (stats->short_gi)
                sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
            sinfo->txrate.mcs = stats->mcs_max;
            break;
        case FORMATMOD_VHT:
            sinfo->txrate.flags = RATE_INFO_FLAGS_VHT_MCS;
            if (stats->short_gi)
                sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
            sinfo->txrate.mcs = stats->mcs_max;
            sinfo->txrate.nss = stats->no_ss + 1;
            break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
        case FORMATMOD_HE_MU:
        case FORMATMOD_HE_SU:
        case FORMATMOD_HE_ER:
        case FORMATMOD_HE_TB:
            sinfo->txrate.flags = RATE_INFO_FLAGS_HE_MCS;
            sinfo->txrate.mcs = stats->mcs_max;
            sinfo->txrate.nss = stats->no_ss + 1;
            break;
#endif
        default :
            return -EINVAL;
    }
    sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE);

    sinfo->bss_param.flags = 0;
    //sinfo->bss_param.dtim_period = stats->dtim; TODO:need to add later
    sinfo->bss_param.beacon_interval = stats->bcn_interval / 1024;

    sinfo->filled |= BIT(NL80211_STA_INFO_BSS_PARAM);
    // Mesh specific info
    if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_MESH_POINT)
    {
        struct mesh_peer_info_cfm peer_info_cfm;
        if (aml_send_mesh_peer_info_req(vif->aml_hw, vif, sta->sta_idx,
                                         &peer_info_cfm))
            return -ENOMEM;

        peer_info_cfm.last_bcn_age = peer_info_cfm.last_bcn_age / 1000;
        if (peer_info_cfm.last_bcn_age < sinfo->inactive_time)
            sinfo->inactive_time = peer_info_cfm.last_bcn_age;

        sinfo->llid = peer_info_cfm.local_link_id;
        sinfo->plid = peer_info_cfm.peer_link_id;
        sinfo->plink_state = peer_info_cfm.link_state;
        sinfo->local_pm = (enum nl80211_mesh_power_mode)peer_info_cfm.local_ps_mode;
        sinfo->peer_pm = (enum nl80211_mesh_power_mode)peer_info_cfm.peer_ps_mode;
        sinfo->nonpeer_pm = (enum nl80211_mesh_power_mode)peer_info_cfm.non_peer_ps_mode;

        sinfo->filled |= (BIT(NL80211_STA_INFO_LLID) |
                          BIT(NL80211_STA_INFO_PLID) |
                          BIT(NL80211_STA_INFO_PLINK_STATE) |
                          BIT(NL80211_STA_INFO_LOCAL_PM) |
                          BIT(NL80211_STA_INFO_PEER_PM) |
                          BIT(NL80211_STA_INFO_NONPEER_PM));
    }

    return 0;
}

/**
 * @get_station: get station information for the station identified by @mac
 */
static int aml_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev,
                                     const u8 *mac, struct station_info *sinfo)
{
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_sta *sta = NULL;

    if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_MONITOR)
        return -EINVAL;
    else if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) ||
             (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT)) {
        if (vif->sta.ap && ether_addr_equal(vif->sta.ap->mac_addr, mac))
            sta = vif->sta.ap;
    }
    else
    {
        struct aml_sta *sta_iter, *tmp;
        list_for_each_entry_safe(sta_iter, tmp, &vif->ap.sta_list, list) {
            if (sta_iter->valid && ether_addr_equal(sta_iter->mac_addr, mac)) {
                sta = sta_iter;
                break;
            }
        }
    }

    if (sta)
        return aml_fill_station_info(sta, vif, sinfo);

    return -EINVAL;
}

/**
 * @dump_station: dump station callback -- resume dump at index @idx
 */
static int aml_cfg80211_dump_station(struct wiphy *wiphy, struct net_device *dev,
                                      int idx, u8 *mac, struct station_info *sinfo)
{
    struct aml_vif *vif = netdev_priv(dev);
    struct aml_sta *sta = NULL;

    if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_MONITOR)
        return -EINVAL;
    else if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) ||
             (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT)) {
        if ((idx == 0) && vif->sta.ap && vif->sta.ap->valid)
            sta = vif->sta.ap;
    } else {
        struct aml_sta *sta_iter, *tmp;
        int i = 0;
        list_for_each_entry_safe(sta_iter, tmp, &vif->ap.sta_list, list) {
            if (i == idx) {
                sta = sta_iter;
                break;
            }
            i++;
        }
    }

    if (sta == NULL)
        return -ENOENT;

    /* Copy peer MAC address */
    memcpy(mac, &sta->mac_addr, ETH_ALEN);

    return aml_fill_station_info(sta, vif, sinfo);
}

/**
 * @add_mpath: add a fixed mesh path
 */
static int aml_cfg80211_add_mpath(struct wiphy *wiphy, struct net_device *dev,
                                   const u8 *dst, const u8 *next_hop)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct mesh_path_update_cfm cfm;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    return aml_send_mesh_path_update_req(aml_hw, aml_vif, dst, next_hop, &cfm);
}

/**
 * @del_mpath: delete a given mesh path
 */
static int aml_cfg80211_del_mpath(struct wiphy *wiphy, struct net_device *dev,
                                   const u8 *dst)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct mesh_path_update_cfm cfm;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    return aml_send_mesh_path_update_req(aml_hw, aml_vif, dst, NULL, &cfm);
}

/**
 * @change_mpath: change a given mesh path
 */
static int aml_cfg80211_change_mpath(struct wiphy *wiphy, struct net_device *dev,
                                      const u8 *dst, const u8 *next_hop)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct mesh_path_update_cfm cfm;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    return aml_send_mesh_path_update_req(aml_hw, aml_vif, dst, next_hop, &cfm);
}

/**
 * @get_mpath: get a mesh path for the given parameters
 */
static int aml_cfg80211_get_mpath(struct wiphy *wiphy, struct net_device *dev,
                                   u8 *dst, u8 *next_hop, struct mpath_info *pinfo)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_mesh_path *mesh_path = NULL;
    struct aml_mesh_path *cur;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    list_for_each_entry(cur, &aml_vif->ap.mpath_list, list) {
        /* Compare the path target address and the provided destination address */
        if (memcmp(dst, &cur->tgt_mac_addr, ETH_ALEN)) {
            continue;
        }

        mesh_path = cur;
        break;
    }

    if (mesh_path == NULL)
        return -ENOENT;

    /* Copy next HOP MAC address */
    if (mesh_path->nhop_sta)
        memcpy(next_hop, &mesh_path->nhop_sta->mac_addr, ETH_ALEN);

    /* Fill path information */
    pinfo->filled = 0;
    pinfo->generation = aml_vif->generation;

    return 0;
}

/**
 * @dump_mpath: dump mesh path callback -- resume dump at index @idx
 */
static int aml_cfg80211_dump_mpath(struct wiphy *wiphy, struct net_device *dev,
                                    int idx, u8 *dst, u8 *next_hop,
                                    struct mpath_info *pinfo)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_mesh_path *mesh_path = NULL;
    struct aml_mesh_path *cur;
    int i = 0;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    list_for_each_entry(cur, &aml_vif->ap.mpath_list, list) {
        if (i < idx) {
            i++;
            continue;
        }

        mesh_path = cur;
        break;
    }

    if (mesh_path == NULL)
        return -ENOENT;

    /* Copy target and next hop MAC address */
    memcpy(dst, &mesh_path->tgt_mac_addr, ETH_ALEN);
    if (mesh_path->nhop_sta)
        memcpy(next_hop, &mesh_path->nhop_sta->mac_addr, ETH_ALEN);

    /* Fill path information */
    pinfo->filled = 0;
    pinfo->generation = aml_vif->generation;

    return 0;
}

/**
 * @get_mpp: get a mesh proxy path for the given parameters
 */
static int aml_cfg80211_get_mpp(struct wiphy *wiphy, struct net_device *dev,
                                 u8 *dst, u8 *mpp, struct mpath_info *pinfo)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_mesh_proxy *mesh_proxy = NULL;
    struct aml_mesh_proxy *cur;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    list_for_each_entry(cur, &aml_vif->ap.proxy_list, list) {
        if (cur->local) {
            continue;
        }

        /* Compare the path target address and the provided destination address */
        if (memcmp(dst, &cur->ext_sta_addr, ETH_ALEN)) {
            continue;
        }

        mesh_proxy = cur;
        break;
    }

    if (mesh_proxy == NULL)
        return -ENOENT;

    memcpy(mpp, &mesh_proxy->proxy_addr, ETH_ALEN);

    /* Fill path information */
    pinfo->filled = 0;
    pinfo->generation = aml_vif->generation;

    return 0;
}

/**
 * @dump_mpp: dump mesh proxy path callback -- resume dump at index @idx
 */
static int aml_cfg80211_dump_mpp(struct wiphy *wiphy, struct net_device *dev,
                                  int idx, u8 *dst, u8 *mpp, struct mpath_info *pinfo)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_mesh_proxy *mesh_proxy = NULL;
    struct aml_mesh_proxy *cur;
    int i = 0;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    list_for_each_entry(cur, &aml_vif->ap.proxy_list, list) {
        if (cur->local) {
            continue;
        }

        if (i < idx) {
            i++;
            continue;
        }

        mesh_proxy = cur;
        break;
    }

    if (mesh_proxy == NULL)
        return -ENOENT;

    /* Copy target MAC address */
    memcpy(dst, &mesh_proxy->ext_sta_addr, ETH_ALEN);
    memcpy(mpp, &mesh_proxy->proxy_addr, ETH_ALEN);

    /* Fill path information */
    pinfo->filled = 0;
    pinfo->generation = aml_vif->generation;

    return 0;
}

/**
 * @get_mesh_config: Get the current mesh configuration
 */
static int aml_cfg80211_get_mesh_config(struct wiphy *wiphy, struct net_device *dev,
                                         struct mesh_config *conf)
{
    struct aml_vif *aml_vif = netdev_priv(dev);

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    return 0;
}

/**
 * @update_mesh_config: Update mesh parameters on a running mesh.
 */
static int aml_cfg80211_update_mesh_config(struct wiphy *wiphy, struct net_device *dev,
                                            u32 mask, const struct mesh_config *nconf)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct mesh_update_cfm cfm;
    int status;

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    if (mask & CO_BIT(NL80211_MESHCONF_POWER_MODE - 1)) {
        aml_vif->ap.next_mesh_pm = nconf->power_mode;

        if (!list_empty(&aml_vif->ap.sta_list)) {
            // If there are mesh links we don't want to update the power mode
            // It will be updated with aml_update_mesh_power_mode() when the
            // ps mode of a link is updated or when a new link is added/removed
            mask &= ~BIT(NL80211_MESHCONF_POWER_MODE - 1);

            if (!mask)
                return 0;
        }
    }

    status = aml_send_mesh_update_req(aml_hw, aml_vif, mask, nconf, &cfm);

    if (!status && (cfm.status != 0))
        status = -EINVAL;

    return status;
}

/**
 * @join_mesh: join the mesh network with the specified parameters
 * (invoked with the wireless_dev mutex held)
 */
static int aml_cfg80211_join_mesh(struct wiphy *wiphy, struct net_device *dev,
                                   const struct mesh_config *conf, const struct mesh_setup *setup)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct mesh_start_cfm mesh_start_cfm;
    int error = 0;
    u8 txq_status = 0;
    /* STA for BC/MC traffic */
    struct aml_sta *sta;

    AML_DBG(AML_FN_ENTRY_STR);

    if (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)
        return -ENOTSUPP;

    /* Forward the information to the UMAC */
    if ((error = aml_send_mesh_start_req(aml_hw, aml_vif, conf, setup, &mesh_start_cfm))) {
        return error;
    }

    /* Check the status */
    switch (mesh_start_cfm.status) {
        case CO_OK:
            aml_vif->ap.bcmc_index = mesh_start_cfm.bcmc_idx;
            aml_vif->ap.flags = 0;
            aml_vif->ap.bcn_interval = setup->beacon_interval;
            aml_vif->use_4addr = true;
            if (setup->user_mpm)
                aml_vif->ap.flags |= AML_AP_USER_MESH_PM;

            sta = aml_hw->sta_table + mesh_start_cfm.bcmc_idx;
            sta->valid = true;
            sta->aid = 0;
            sta->sta_idx = mesh_start_cfm.bcmc_idx;
            sta->ch_idx = mesh_start_cfm.ch_idx;
            sta->vif_idx = aml_vif->vif_index;
            sta->qos = true;
            sta->acm = 0;
            sta->ps.active = false;
            sta->listen_interval = 5;
            aml_mu_group_sta_init(sta, NULL);
            spin_lock_bh(&aml_hw->cb_lock);
            aml_chanctx_link(aml_vif, mesh_start_cfm.ch_idx,
                              (struct cfg80211_chan_def *)(&setup->chandef));
            if (aml_hw->cur_chanctx != mesh_start_cfm.ch_idx) {
                txq_status = AML_TXQ_STOP_CHAN;
            }
            aml_txq_vif_init(aml_hw, aml_vif, txq_status);
            spin_unlock_bh(&aml_hw->cb_lock);

            netif_tx_start_all_queues(dev);
            netif_carrier_on(dev);

            /* If the AP channel is already the active, we probably skip radar
               activation on MM_CHANNEL_SWITCH_IND (unless another vif use this
               ctxt). In anycase retest if radar detection must be activated
             */
            if (aml_hw->cur_chanctx == mesh_start_cfm.ch_idx) {
                aml_radar_detection_enable_on_cur_channel(aml_hw);
            }
            break;

        case CO_BUSY:
            error = -EINPROGRESS;
            break;

        default:
            error = -EIO;
            break;
    }

    /* Print information about the operation */
    if (error) {
        netdev_info(dev, "Failed to start MP (%d)", error);
    } else {
        netdev_info(dev, "MP started: ch=%d, bcmc_idx=%d",
                    aml_vif->ch_index, aml_vif->ap.bcmc_index);
    }

    return error;
}

/**
 * @leave_mesh: leave the current mesh network
 * (invoked with the wireless_dev mutex held)
 */
static int aml_cfg80211_leave_mesh(struct wiphy *wiphy, struct net_device *dev)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct mesh_stop_cfm mesh_stop_cfm;
    int error = 0;

    error = aml_send_mesh_stop_req(aml_hw, aml_vif, &mesh_stop_cfm);

    if (error == 0) {
        /* Check the status */
        switch (mesh_stop_cfm.status) {
            case CO_OK:
                spin_lock_bh(&aml_hw->cb_lock);
                aml_chanctx_unlink(aml_vif);
                aml_radar_cancel_cac(&aml_hw->radar);
                spin_unlock_bh(&aml_hw->cb_lock);
                /* delete BC/MC STA */
                aml_txq_vif_deinit(aml_hw, aml_vif);
                aml_del_bcn(&aml_vif->ap.bcn);

                netif_tx_stop_all_queues(dev);
                netif_carrier_off(dev);

                break;

            default:
                error = -EIO;
                break;
        }
    }

    if (error) {
        netdev_info(dev, "Failed to stop MP");
    } else {
        netdev_info(dev, "MP Stopped");
    }

    return 0;
}

void aml_scan_abort(struct aml_hw *aml_hw)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    struct cfg80211_scan_info scan_info = {
        .aborted = true,
    };
    cfg80211_scan_done(aml_hw->scan_request, &scan_info);
#else
    cfg80211_scan_done(rwnx_hw->scan_request, true);
#endif
    aml_hw->scan_request = NULL;
}

#ifdef CONFIG_AML_SUSPEND
int aml_pwrsave_wow_sta(struct aml_hw *aml_hw, struct aml_vif *aml_vif)
{
    /*
    * bitmask where to match pattern and where to ignore
    * bytes, one bit per byte
    */
    int error = 0;
    unsigned char mask = 0x3f;
    struct cfg80211_pkt_pattern pattern;

    memset(&pattern, 0, sizeof(pattern));
    pattern.mask = (u8 *)&mask;
    pattern.pattern_len = ETH_ALEN;
    pattern.pkt_offset = 0;
    pattern.pattern = aml_vif->ndev->dev_addr;

    if ((error = aml_send_wow_pattern(aml_hw, aml_vif, &pattern, 0)))
        return error;

    return 0;
}


int aml_pwrsave_wow_usr(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
    struct cfg80211_wowlan *wow, unsigned int *filter)
{
    int i, error;

    /*
    * Configure the patterns that we received from the user.
    * And we save WOW_MAX_FILTERS patterns at most.
    */
    for (i = 0; i < wow->n_patterns; i++){
        if ((error = aml_send_wow_pattern(aml_hw, aml_vif, &wow->patterns[i], i)))
            return error;
    }

    /* pno offload*/
    if (wow->nd_config) {
        aml_send_sched_scan_req(aml_vif, wow->nd_config);
    }

    /*get wakeup filter */
    if (wow->disconnect)
        *filter |= WOW_FILTER_OPTION_DISCONNECT;

    if (wow->magic_pkt)
        *filter |= WOW_FILTER_OPTION_MAGIC_PACKET;

    if (wow->gtk_rekey_failure)
        *filter |= WOW_FILTER_OPTION_GTK_ERROR;

    if (wow->eap_identity_req)
        *filter |= WOW_FILTER_OPTION_EAP_REQ;

    if (wow->four_way_handshake)
        *filter |= WOW_FILTER_OPTION_4WAYHS;

    return 0;
}

int aml_cancel_scan(struct aml_hw *aml_hw, struct aml_vif *vif)
{
    struct scanu_cancel_cfm scanu_cancel_cfm;
    int error = 0, cnt = 0;

    aml_set_scan_hang(vif, 1, __func__, __LINE__);
    if ((error = aml_send_scanu_cancel_req(aml_hw, vif, &scanu_cancel_cfm))) {
        return error;
    }

    while (!vif->sta.cancel_scan_cfm) {
        msleep(20);
        if (cnt++ > 20) {
            return -EINVAL;
        }
    }
    vif->sta.cancel_scan_cfm = 0;

    return 0;
}

static int aml_set_arp_agent(struct aml_hw *aml_hw, struct aml_vif *aml_vif, u8 enable)
{
    struct in_device *in_dev;
    struct in_ifaddr *ifa_v4;
    struct inet6_dev *idev_v6 = NULL;
    struct inet6_ifaddr *ifa_v6 = NULL;
    struct in6_addr *ipv6_ptr = NULL;
    __be32 ipv4;

    unsigned char ipv6[IPV6_ADDR_BUF_LEN] = {0};
    int i = 0, j = 0;

    if (enable == 0) {
        /*just disable arp agent */
        return aml_send_arp_agent_req(aml_hw, aml_vif, enable, 0, NULL);
    }
    /* get ipv4 addr */
    in_dev = __in_dev_get_rtnl(aml_vif->ndev);
    if (!in_dev)
        return -EINVAL;

    ifa_v4 = in_dev->ifa_list;
    if (!ifa_v4) {
        return -EINVAL;
    }
    memset(&ipv4, 0, sizeof(ipv4));
    ipv4 = ifa_v4->ifa_local;

    /*get ipv6 addr */
    idev_v6 = __in6_dev_get(aml_vif->ndev);
    if (!idev_v6) {
        printk("not support ipv6\n");
        return -EINVAL;
    }

    read_lock_bh(&idev_v6->lock);
    list_for_each_entry(ifa_v6, &idev_v6->addr_list, if_list) {
        unsigned int addr_type = __ipv6_addr_type(&ifa_v6->addr);

        if ((ifa_v6->flags & IFA_F_TENTATIVE) &&
              (!(ifa_v6->flags & IFA_F_OPTIMISTIC)))
            continue;

        if (unlikely(addr_type == IPV6_ADDR_ANY ||
                     addr_type & IPV6_ADDR_MULTICAST))
            continue;

        ipv6_ptr = &ifa_v6->addr;
        if (ipv6_ptr->in6_u.u6_addr8[0] != 0) {
            memcpy(ipv6 + j * sizeof(struct in6_addr), ipv6_ptr->in6_u.u6_addr8, sizeof(struct in6_addr));
            j++;
        }
        i++;
        /* we just support 3 ipv6 addr at most. */
        if (i > 2)
            break;
    }
    read_unlock_bh(&idev_v6->lock);

    return aml_send_arp_agent_req(aml_hw, aml_vif, enable, ipv4, ipv6);
}

void aml_set_fw_wake(struct aml_hw *aml_hw)
{
    aml_hw->plat->hif_sdio_ops->hi_self_define_domain_write8(RG_SDIO_PMU_WAKE, BIT(0));
}

void aml_clear_fw_wake(struct aml_hw *aml_hw)
{
    unsigned char reg_value;

    reg_value = aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SDIO_PMU_WAKE);
    aml_hw->plat->hif_sdio_ops->hi_self_define_domain_write8(RG_SDIO_PMU_WAKE, (reg_value & ~BIT(0)));
}

unsigned char aml_get_fw_ps_status(struct aml_hw *aml_hw)
{
    return aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SDIO_PMU_STATUS);
}

int aml_wake_fw_req(struct aml_hw *aml_hw)
{
    unsigned char fw_ps_st;
    unsigned char fw_sleep, host_sleep_req;
    unsigned int loop = 0, wake_flag = 0;
    unsigned char host_req_status;

    while (1)
    {
        fw_ps_st = aml_get_fw_ps_status(aml_hw);
        fw_sleep = ((fw_ps_st & FW_SLEEP) != 0) ? 1 : 0;
        host_req_status = aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SDIO_PMU_HOST_REQ);
        host_sleep_req = ((host_req_status & HOST_SLEEP_REQ) != 0) ? 1 : 0;

        fw_ps_st = fw_ps_st & 0xF;
        if (fw_ps_st != PMU_ACT_MODE)
        {
            if (wake_flag == 0)
            {
                wake_flag = 1;
                //delay 10ms for pmu deep sleep
                msleep(10);
                continue;
            }
            else if ((fw_ps_st == PMU_SLEEP_MODE) && (wake_flag == 1))
            {
                wake_flag = 2;
                aml_clear_fw_wake(aml_hw);
                aml_set_fw_wake(aml_hw);
                udelay(20);
            }

        }

        if ((fw_ps_st == PMU_ACT_MODE) && (fw_sleep == 0) && (host_sleep_req == 0))
        {
            if (wake_flag == 2)
            {
                aml_clear_fw_wake(aml_hw);
            }
            return 0;
        }

        loop++;
        if (loop < 1000)
        {
            udelay(200);
        }
        else
        {
            AML_INFO("fw ps st 0x%x, fw_sleep 0x%x, host_sleep_req 0x%x\n", fw_ps_st, fw_sleep, host_sleep_req);
            return -1;
        }
    }

}
extern struct aml_pm_type g_wifi_pm;

int aml_ps_wow_resume(struct aml_hw *aml_hw)
{
    struct aml_vif *aml_vif;
    int error = 0;
    struct aml_txq *txq;
    int ret = 0;
    unsigned int reg_value;
    int cnt = 0;

    AML_DBG(AML_FN_ENTRY_STR);
    if (aml_hw->state == WIFI_SUSPEND_STATE_NONE) {
        return -EINVAL;
    }

    if (aml_bus_type == USB_MODE) {
        while (g_udev->state != USB_STATE_CONFIGURED) {
            udelay(100);
        }

        if (atomic_read(&g_wifi_pm.drv_suspend_cnt)) {
            atomic_set(&g_wifi_pm.drv_suspend_cnt, 0);
            USB_BEGIN_LOCK();
            if (aml_hw->g_urb->status != -EINPROGRESS)
            {
                printk("%s need submit urb\n", __func__);
                ret = usb_submit_urb(aml_hw->g_urb, GFP_ATOMIC);
            }
            USB_END_LOCK();
            if (ret < 0) {
                ERROR_DEBUG_OUT("usb_submit_urb failed %d\n", ret);
            }
            printk("%s aml_hw->g_urb->status %d\n", __func__,aml_hw->g_urb->status);
        }
    }

    if (aml_bus_type == SDIO_MODE)
    {
        // clear sleep req flag
        reg_value = aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SDIO_PMU_HOST_REQ);
        reg_value &= ~HOST_SLEEP_REQ;
        aml_hw->plat->hif_sdio_ops->hi_self_define_domain_write8(RG_SDIO_PMU_HOST_REQ, reg_value);
        if (aml_wake_fw_req(aml_hw) != 0)
        {
            ERROR_DEBUG_OUT("host wake fw fail \n");
            return -1;
        }
    }

    error = aml_send_suspend_req(aml_hw, 0, WIFI_SUSPEND_STATE_NONE);
    if (error) {
        return error;
    }

    aml_hw->state = WIFI_SUSPEND_STATE_NONE;

    aml_send_me_set_ps_mode(aml_hw, MM_PS_MODE_OFF);

    if (aml_bus_type == PCIE_MODE) {
        struct aml_ipc_buf *ipc_desc;
        struct rxdesc_tag *rxdesc;
        AML_INFO("repush:rxbuff cnt=%u,rxdesc=%u\n",aml_hw->repush_rxbuff_cnt,aml_hw->repush_rxdesc);
        while (aml_hw->repush_rxbuff_cnt--) {
            aml_ipc_rxbuf_alloc(aml_hw);
        }
        if (aml_hw->repush_rxdesc) {
            aml_hw->repush_rxdesc = 0;
            ipc_desc = aml_hw->ipc_env->rxdesc[aml_hw->ipc_env->rxdesc_idx];
            aml_ipc_buf_e2a_sync(aml_hw, ipc_desc, sizeof(struct rxdesc_tag));
            AML_INFO("repush:dma_addr=0x%x,index=%u\n",ipc_desc->dma_addr,aml_hw->ipc_env->rxdesc_idx);
            ipc_host_rxdesc_push(aml_hw->ipc_env, ipc_desc);
            up(&aml_hw->rxdesc->task_sem);
        }
    }

    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (!aml_vif->up || aml_vif->ndev == NULL) {
            continue;
        }

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION) {
            aml_txq_vif_start(aml_vif, AML_TXQ_STOP & ~(AML_TXQ_STOP_CHAN), aml_hw);

            if (aml_vif->sta.ap && aml_vif->sta.ap->valid) {
                aml_vif->filter = 0;
                aml_set_arp_agent(aml_hw, aml_vif, 0);
                aml_tko_activate(aml_hw, aml_vif, 0);
                aml_send_dhcp_req(aml_hw, aml_vif, 0);
            }

            aml_scan_hang(aml_vif, 0);
            aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
        } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP) {
            aml_txq_vif_start(aml_vif, AML_TXQ_STOP & ~(AML_TXQ_STOP_CHAN | AML_TXQ_STOP_STA_PS), aml_hw);
            txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
            aml_txq_start(txq, AML_TXQ_STOP);
        } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO) {
            aml_txq_vif_start(aml_vif,
                AML_TXQ_STOP & ~(AML_TXQ_STOP_CHAN | AML_TXQ_STOP_STA_PS | AML_TXQ_STOP_VIF_PS), aml_hw);
            txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
            aml_txq_start(txq, AML_TXQ_STOP);
        } else if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) {
            aml_txq_vif_start(aml_vif, AML_TXQ_STOP & ~(AML_TXQ_STOP_CHAN | AML_TXQ_STOP_VIF_PS), aml_hw);
        }

        netif_tx_start_all_queues(aml_vif->ndev);
        netif_wake_queue(aml_vif->ndev);
    }

    return 0;
}

static int aml_ps_wow_suspend(struct aml_hw *aml_hw, struct cfg80211_wowlan *wow)
{
    struct aml_vif *aml_vif;
    int error = 0;
    unsigned int filter = 0;
    int count = 0;
    unsigned int reg_value;
    enum nl80211_iftype iftype;

    AML_DBG(AML_FN_ENTRY_STR);
    if (aml_hw->state == WIFI_SUSPEND_STATE_WOW) {
        AML_INFO("wifi driver suspend state is WOW\n");
        return -EINVAL;
    }
    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (!aml_vif->up || aml_vif->ndev == NULL) {
            continue;
        }
        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION ||
            AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_CLIENT) {
            if (aml_connect_flags_chk(aml_vif, AML_CONNECTING)) {
                /*sta is connecting,so delay suspend*/
                AML_INFO("sta is connecting ap\n");
                return -EBUSY;
            }
        }
    }
    // reset suspend, flag for fw suspended
    aml_hw->suspend_ind = SUSPEND_IND_NONE;

    //only support suspend under sta interface.
    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (!aml_vif->up || aml_vif->ndev == NULL) {
            continue;
        }
        iftype = AML_VIF_TYPE(aml_vif);

        netif_tx_stop_all_queues(aml_vif->ndev);
        aml_txq_vif_stop(aml_vif, AML_TXQ_STOP, aml_hw);

        if (iftype == NL80211_IFTYPE_STATION) {
            error = aml_cancel_scan(aml_hw, aml_vif);
            if (error) {
                printk("before cancel scan:%d\n", aml_hw->cmd_mgr.queue_sz);
                goto err;
            }

            if (aml_vif->sta.ap && aml_vif->sta.ap->valid) {
                aml_set_arp_agent(aml_hw, aml_vif, 1);
                aml_tko_activate(aml_hw, aml_vif, 1);

                if (wow != NULL) {
                    aml_pwrsave_wow_usr(aml_hw, aml_vif, wow, &filter);
                } else {
                    aml_pwrsave_wow_sta(aml_hw, aml_vif);
                }
                if (aml_hw->google_cast == 1)
                    filter |= WOW_FILTER_OPTION_GOOGLE_CAST_EN;
#ifdef AML_WOW_GOOGLE_CAST_EN
                else
                    filter |= WOW_FILTER_OPTION_GOOGLE_CAST_EN;
#endif

#ifdef AML_WOW_MAGIC_PACKET_EN
                    filter |= WOW_FILTER_OPTION_MAGIC_PACKET;
#endif
                aml_vif->filter = filter;
                aml_send_dhcp_req(aml_hw, aml_vif, 1);

#ifdef CONFIG_SDIO_TX_ENH
                if (aml_bus_type == SDIO_MODE) {
                    if (aml_hw->txcfm_param.dyn_en) {
                        aml_update_dyn_txcfm(aml_hw, 0);
                        aml_update_tx_cfm(aml_hw);
                    }
                }
#endif

                while (!aml_txq_is_empty(aml_vif, aml_vif->sta.ap)) {
                    msleep(10);
                    if (count++ > 10) {
                        AML_INFO("txq is not empty\n");
                        goto err;
                    }
                }
            }
            continue;
        }
        if (iftype == NL80211_IFTYPE_P2P_CLIENT) {
            if (aml_vif->sta.ap && aml_vif->sta.ap->valid) {
                while (!aml_txq_is_empty(aml_vif, aml_vif->sta.ap)) {
                    msleep(10);
                    if (count++ > 10) {
                        AML_INFO("gc txq is not empty\n");
                        goto err;
                    }
                }
            }
            continue;
        }
        if ((iftype == NL80211_IFTYPE_AP) || (iftype == NL80211_IFTYPE_P2P_GO)) {
            struct aml_sta *sta, *tmp;
            while (!aml_unktxq_is_empty(aml_vif)) {
                msleep(10);
                if (count++ > 10) {
                    AML_INFO("ap[%u]:unktxq is not empty\n",iftype);
                    goto err;
                }
            }
            list_for_each_entry_safe(sta, tmp, &aml_vif->ap.sta_list, list) {
                while (!aml_txq_is_empty(aml_vif, sta)) {
                    msleep(10);
                    if (count++ > 10) {
                        AML_INFO("ap[%u]:txq is not empty\n",iftype);
                        goto err;
                    }
                }
            }
        }
    }

    /* Enable powersave mode by default in suspend process. */
    aml_send_me_set_ps_mode(aml_hw, MM_PS_MODE_ON);

    aml_hw->state = WIFI_SUSPEND_STATE_WOW;

    error = aml_send_suspend_req(aml_hw, filter, WIFI_SUSPEND_STATE_WOW);
    if (error) {
        AML_INFO("queue_sz:%d\n", aml_hw->cmd_mgr.queue_sz);
        goto err;
    }

    count = 0;
    while ((aml_hw->suspend_ind != SUSPEND_IND_DONE)
        || ((aml_bus_type == PCIE_MODE) && (AML_REG_READ(aml_hw->plat, AML_ADDR_MAC_PHY, ISTATUS_HOST) & BIT(24)))) {
        msleep(10);
        if (count++ > 100) {
            printk("%s %d, ERROR wait suspend_ind timeout:%d, start resume cmd:%d\n",
                __func__, __LINE__, aml_hw->suspend_ind, aml_hw->cmd_mgr.queue_sz);
            goto err;
        }
    }

    //When in rx buf reduce, not expend rx buf,after resume, do rx buf expend
    if (aml_bus_type != PCIE_MODE) {
        if (aml_hw->rx_buf_state & FW_BUFFER_EXPAND) {
            aml_tx_rx_buf_init(aml_hw);
            if (aml_bus_type == SDIO_MODE)
                aml_hw->g_tx_param.tx_page_free_num = SDIO_TX_PAGE_NUM_SMALL;
            else
                aml_hw->g_tx_param.tx_page_free_num = USB_TX_PAGE_NUM_SMALL;

        } else if (aml_hw->rx_buf_state & FW_BUFFER_NARROW) {//In rx buf reduce state,put ptr in rx start
           aml_hw->fw_buf_pos = RXBUF_START_ADDR;
           aml_hw->last_fw_pos = RXBUF_START_ADDR;
        }
    }

    AML_INFO("after suspend cmd:%d\n", aml_hw->cmd_mgr.queue_sz);
    count = 0;
    while (aml_hw->cmd_mgr.queue_sz != 0) {
        msleep(10);
        if (count++ > 50) {
            aml_hw->cmd_mgr.print(&aml_hw->cmd_mgr);
            goto err;
        }
    }

    if (aml_bus_type == USB_MODE) {
        USB_BEGIN_LOCK();
        atomic_set(&g_wifi_pm.drv_suspend_cnt, 1);
        if (aml_hw->g_urb->status != 0) {
            usb_kill_urb(aml_hw->g_urb);
            printk("%s kill urb status %d\n", __func__, aml_hw->g_urb->status);
        }
        USB_END_LOCK();
    } else if (aml_bus_type == PCIE_MODE) {
        aml_hw->repush_rxdesc = 0;
        aml_hw->repush_rxbuff_cnt = 0;
    } else {
        reg_value = aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SDIO_PMU_HOST_REQ);
        reg_value |= HOST_SLEEP_REQ;
        aml_hw->plat->hif_sdio_ops->hi_self_define_domain_write8(RG_SDIO_PMU_HOST_REQ, reg_value);
    }

    return 0;
err:
    aml_hw->state = WIFI_SUSPEND_STATE_WOW;
    aml_ps_wow_resume(aml_hw);
    return -EBUSY;
}
#endif

static int aml_cfg80211_suspend(struct wiphy *wiphy, struct cfg80211_wowlan *wow)
{
#ifdef CONFIG_AML_SUSPEND
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    int error = 0;
    printk("%s enter   %d\n", __func__, __LINE__);
    if (wow && (wow->n_patterns > WOW_MAX_PATTERNS))
        return -EINVAL;

    AML_DBG(AML_FN_ENTRY_STR);
#ifdef CONFIG_AML_RECOVERY
    if ((aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) ||
        ((aml_bus_type != PCIE_MODE) && (bus_state_detect.bus_err))) {
        AML_INFO("recy ongoing or bus err(%d), do not allow suspend now!\n", bus_state_detect.bus_err);
        return -EBUSY;
    }
#endif

    error = aml_ps_wow_suspend(aml_hw, wow);
    if (error){
        return error;
    }
    atomic_set(&g_wifi_pm.drv_suspend_cnt, 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    if (aml_bus_type == PCIE_MODE)
        free_irq(aml_hw->plat->pci_dev->irq, aml_hw);
#endif
    printk("%s ok exit   %d\n", __func__, __LINE__);
    return 0;
#else
    printk("test %s,%d, suspend is not supported\n", __func__, __LINE__);
    return 0;
#endif
}

static int aml_cfg80211_resume(struct wiphy *wiphy)
{
#ifdef CONFIG_AML_SUSPEND
    struct aml_hw *aml_hw = wiphy_priv(wiphy);
    int error = 0;
    int cnt = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    int ret;
#endif

    AML_DBG(AML_FN_ENTRY_STR);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    if (aml_bus_type == PCIE_MODE) {
        ret = request_irq(aml_hw->plat->pci_dev->irq, aml_irq_pcie_hdlr, 0, "aml", aml_hw);
        AML_INFO("alloc irq:%d, ret:%d\n", aml_hw->plat->pci_dev->irq, ret);
    }
#endif

#if 1
    while (atomic_read(&g_wifi_pm.bus_suspend_cnt) > 0)
    {
        msleep(50);
        cnt++;
        if (cnt > 200)
        {
            AML_INFO("no resume cnt 0x%x\n",
                    atomic_read(&g_wifi_pm.bus_suspend_cnt));
            atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
            return -1;
        }
    }
#else
    if ((atomic_read(&g_wifi_pm.bus_suspend_cnt) > 0) && (aml_bus_type == USB_MODE)) {
        struct aml_wq *aml_wq;

        aml_wq = aml_wq_alloc(0);
        if (!aml_wq) {
            AML_INFO("alloc workqueue out of memory");
            return 0;
        }
        aml_wq->id = AML_WQ_WAIT_USB;
        aml_wq_add(aml_hw, aml_wq);
        return 0;
    }
#endif

    error = aml_ps_wow_resume(aml_hw);
    if (error){
        return error;
    }
    atomic_set(&g_wifi_pm.drv_suspend_cnt, 0);
    printk("%s,%d, resume is ok\n", __func__, __LINE__);
    return 0;
#else
    printk("%s,%d, resume is not supported\n", __func__, __LINE__);
    return 0;
#endif
}

static int aml_cfg80211_set_rekey_data(struct wiphy *wiphy,
    struct net_device *dev,struct cfg80211_gtk_rekey_data *data)
{
    struct aml_vif *aml_vif = netdev_priv(dev);

    if (AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_STATION) {
        aml_set_rekey_data(aml_vif, data->kek, data->kck, data->replay_ctr);
        return 0;
    }

    return -1;
}

unsigned char aml_get_s8_item(char *varbuf, int len, char *item, char *item_value)
{
    unsigned int n;
    char tmpbuf[20];
    char *p = item_value;
    int ret = 0;
    unsigned int pos = 0;
    unsigned int index = 0;

    while (pos  < len) {
        index = pos;
        ret = 0;

        while ((varbuf[pos] != 0) && (varbuf[pos] != '=')) {
            if (((pos - index) >= strlen(item)) || (varbuf[pos] != item[pos - index])) {
                ret = 1;
                break;
            }
            else {
                pos++;
            }
        }

        pos++;

        if ((ret == 0) && (strlen(item) == pos - index - 1)) {
            do {
                memset(tmpbuf, 0, sizeof(tmpbuf));
                n = 0;
                while ((varbuf[pos] != 0) && (varbuf[pos] != ',') && (pos < len))
                    tmpbuf[n++] = varbuf[pos++];

                *p++ = (char)simple_strtol(tmpbuf, NULL, 0);
            }
            while (varbuf[pos++] == ',');

            return 0;
        }
    }

    return 1;
}

unsigned char aml_get_s16_item(char *varbuf, int len, char *item, short *item_value)
{
    unsigned int n;
    char tmpbuf[60];
    short *p = item_value;
    int ret = 0;
    unsigned int pos = 0;
    unsigned int index = 0;

    while (pos  < len) {
        index = pos;
        ret = 0;

        while ((varbuf[pos] != 0) && (varbuf[pos] != '=')) {
            if (((pos - index) >= strlen(item)) || (varbuf[pos] != item[pos - index])) {
                ret = 1;
                break;
            }
            else {
                pos++;
            }
        }

        pos++;

        if ((ret == 0) && (strlen(item) == pos - index - 1)) {
            do {
                memset(tmpbuf, 0, sizeof(tmpbuf));
                n = 0;
                while ((varbuf[pos] != 0) && (varbuf[pos] != ',') && (pos < len))
                    tmpbuf[n++] = varbuf[pos++];

                *p++ = (short)simple_strtol(tmpbuf, NULL, 0);
            }
            while (varbuf[pos++] == ',');

            return 0;
        }
    }

    return 1;
}

unsigned char aml_get_s32_item(char *varbuf, int len, char *item, unsigned int *item_value)
{
    unsigned int n;
    char tmpbuf[120];
    unsigned int *p = item_value;
    int ret = 0;
    unsigned int pos = 0;
    unsigned int index = 0;

    while (pos  < len) {
        index = pos;
        ret = 0;

        while ((varbuf[pos] != 0) && (varbuf[pos] != '=')) {
            if (((pos - index) >= strlen(item)) || (varbuf[pos] != item[pos - index])) {
                ret = 1;
                break;
            }
            else {
                pos++;
            }
        }

        pos++;

        if ((ret == 0) && (strlen(item) == pos - index - 1)) {
            do {
                memset(tmpbuf, 0, sizeof(tmpbuf));
                n = 0;
                while ((varbuf[pos] != 0) && (varbuf[pos] != ',') && (pos < len))
                    tmpbuf[n++] = varbuf[pos++];

                *p++ = (unsigned int)simple_strtol(tmpbuf, NULL, 0);
            }
            while (varbuf[pos++] == ',');

            return 0;
        }
    }

    return 1;
}

unsigned int aml_process_cali_content(char *varbuf, unsigned int len)
{
    char *dp;
    bool findNewline;
    int column;
    unsigned int buf_len, n;
    unsigned int pad = 0;

    dp = varbuf;
    findNewline = false;
    column = 0;

    for (n = 0; n < len; n++) {
        if (varbuf[n] == '\r')
            continue;

        if (findNewline && varbuf[n] != '\n')
            continue;
        findNewline = false;
        if (varbuf[n] == '#') {
            findNewline = true;
            continue;
        }
        if (varbuf[n] == '\n') {
            if (column == 0)
                continue;
            *dp++ = 0;
            column = 0;
            continue;
        }
        *dp++ = varbuf[n];
        column++;
    }
    buf_len = (unsigned int)(dp - varbuf);
    if (buf_len % 4) {
        pad = 4 - buf_len % 4;
        if (pad && (buf_len + pad <= len)) {
            buf_len += pad;
        }
    }

    while (dp < varbuf + n)
        *dp++ = 0;

    return buf_len;
}

unsigned char aml_parse_cali_param(char *varbuf, int len, struct Cali_Param *cali_param)
{
    //unsigned short platform_verid = 0; // default: 0
    unsigned short cali_config = 0;
    unsigned int version = 0;

    aml_get_s32_item(varbuf, len, "version", &version);
    aml_get_s16_item(varbuf, len, "cali_config", &cali_config);
    aml_get_s8_item(varbuf, len, "freq_offset", &cali_param->freq_offset);
    aml_get_s8_item(varbuf, len, "htemp_freq_offset", &cali_param->htemp_freq_offset);
    aml_get_s8_item(varbuf, len, "tssi_2g_offset", &cali_param->tssi_2g_offset);
    aml_get_s8_item(varbuf, len, "tssi_5g_offset_5200", &cali_param->tssi_5g_offset[0]);
    aml_get_s8_item(varbuf, len, "tssi_5g_offset_5400", &cali_param->tssi_5g_offset[1]);
    aml_get_s8_item(varbuf, len, "tssi_5g_offset_5600", &cali_param->tssi_5g_offset[2]);
    aml_get_s8_item(varbuf, len, "tssi_5g_offset_5800", &cali_param->tssi_5g_offset[3]);
    aml_get_s8_item(varbuf, len, "wf2g_spur_rmen", &cali_param->wf2g_spur_rmen);
    aml_get_s16_item(varbuf, len, "spur_freq", &cali_param->spur_freq);
    aml_get_s8_item(varbuf, len, "rf_count", &cali_param->rf_num);

    aml_get_s8_item(varbuf, len, "cw2mod", &cali_param->cw2mod[0]);

    aml_get_s8_item(varbuf, len, "wf2g_11b_tpwr", &cali_param->wf2g_11b_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_11g_tpwr", &cali_param->wf2g_11g_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_ht20_tpwr", &cali_param->wf2g_ht20_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_ht40_tpwr", &cali_param->wf2g_ht40_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_vht20_tpwr", &cali_param->wf2g_vht20_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_vht40_tpwr", &cali_param->wf2g_vht40_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_he20_tpwr", &cali_param->wf2g_he20_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf2g_he40_tpwr", &cali_param->wf2g_he40_tpwr[0]);

    aml_get_s8_item(varbuf, len, "wf5g_11a_tpwr_lm", &cali_param->wf5g_11a_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_ht20_tpwr_lm", &cali_param->wf5g_ht20_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_ht40_tpwr_lm", &cali_param->wf5g_ht40_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht20_tpwr_lm", &cali_param->wf5g_vht20_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht40_tpwr_lm", &cali_param->wf5g_vht40_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht80_tpwr_lm", &cali_param->wf5g_vht80_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he20_tpwr_lm", &cali_param->wf5g_he20_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he40_tpwr_lm", &cali_param->wf5g_he40_tpwr_lm[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he80_tpwr_lm", &cali_param->wf5g_he80_tpwr_lm[0]);

    aml_get_s8_item(varbuf, len, "wf5g_11a_tpwr_h", &cali_param->wf5g_11a_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_ht20_tpwr_h", &cali_param->wf5g_ht20_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_ht40_tpwr_h", &cali_param->wf5g_ht40_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht20_tpwr_h", &cali_param->wf5g_vht20_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht40_tpwr_h", &cali_param->wf5g_vht40_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht80_tpwr_h", &cali_param->wf5g_vht80_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he20_tpwr_h", &cali_param->wf5g_he20_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he40_tpwr_h", &cali_param->wf5g_he40_tpwr_h[0]);
    aml_get_s8_item(varbuf, len, "wf5g_he80_tpwr_h", &cali_param->wf5g_he80_tpwr_h[0]);

    aml_get_s8_item(varbuf, len, "wf5g_lch_he_lowrssi_tpwr", &cali_param->wf5g_lch_he_lowrssi_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf5g_mch_he_lowrssi_tpwr", &cali_param->wf5g_mch_he_lowrssi_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf5g_hch_he20_lowrssi_tpwr", &cali_param->wf5g_hch_he20_lowrssi_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf5g_hch_he40_lowrssi_tpwr", &cali_param->wf5g_hch_he40_lowrssi_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf5g_hch_he80_lowrssi_tpwr", &cali_param->wf5g_hch_he80_lowrssi_tpwr[0]);
    aml_get_s8_item(varbuf, len, "wf5g_vht_lowrssi_tpwr", &cali_param->wf5g_vht_lowrssi_tpwr[0]);

    aml_get_s8_item(varbuf, len, "customer_efuse_en", &cali_param->w2_efuse_param.customer_efuse_en);
    aml_get_s8_item(varbuf, len, "FT_efuse_en", &cali_param->w2_efuse_param.FT_efuse_en);

    aml_get_s8_item(varbuf, len, "xosc_ctune_vld", &cali_param->w2_efuse_param.xosc_ctune_vld);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_2g_vld", &cali_param->w2_efuse_param.absolute_power_wf0_2g_vld);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5g_vld", &cali_param->w2_efuse_param.absolute_power_wf0_5g_vld);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_2g_vld", &cali_param->w2_efuse_param.absolute_power_wf1_2g_vld);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5g_vld", &cali_param->w2_efuse_param.absolute_power_wf1_5g_vld);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_2g_vld", &cali_param->w2_efuse_param.offset_power_wf0_2g_vld);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5g_vld", &cali_param->w2_efuse_param.offset_power_wf0_5g_vld);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_2g_vld", &cali_param->w2_efuse_param.offset_power_wf1_2g_vld);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5g_vld", &cali_param->w2_efuse_param.offset_power_wf1_5g_vld);
    aml_get_s8_item(varbuf, len, "pwr_delta_wf0_customer_vld", &cali_param->w2_efuse_param.pwr_delta_wf0_customer_vld);
    aml_get_s8_item(varbuf, len, "pwr_delta_wf1_customer_vld", &cali_param->w2_efuse_param.pwr_delta_wf1_customer_vld);
    aml_get_s8_item(varbuf, len, "xosc_ctune", &cali_param->w2_efuse_param.xosc_ctune);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_2g_l", &cali_param->w2_efuse_param.absolute_power_wf0_2g_l);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_2g_m", &cali_param->w2_efuse_param.absolute_power_wf0_2g_m);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_2g_h", &cali_param->w2_efuse_param.absolute_power_wf0_2g_h);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5200", &cali_param->w2_efuse_param.absolute_power_wf0_5200);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5300", &cali_param->w2_efuse_param.absolute_power_wf0_5300);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5530", &cali_param->w2_efuse_param.absolute_power_wf0_5530);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5660", &cali_param->w2_efuse_param.absolute_power_wf0_5660);
    aml_get_s8_item(varbuf, len, "absolute_power_wf0_5780", &cali_param->w2_efuse_param.absolute_power_wf0_5780);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_2g_l", &cali_param->w2_efuse_param.absolute_power_wf1_2g_l);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_2g_m", &cali_param->w2_efuse_param.absolute_power_wf1_2g_m);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_2g_h", &cali_param->w2_efuse_param.absolute_power_wf1_2g_h);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5200", &cali_param->w2_efuse_param.absolute_power_wf1_5200);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5300", &cali_param->w2_efuse_param.absolute_power_wf1_5300);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5530", &cali_param->w2_efuse_param.absolute_power_wf1_5530);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5660", &cali_param->w2_efuse_param.absolute_power_wf1_5660);
    aml_get_s8_item(varbuf, len, "absolute_power_wf1_5780", &cali_param->w2_efuse_param.absolute_power_wf1_5780);

    aml_get_s8_item(varbuf, len, "offset_power_wf0_2g_l", &cali_param->w2_efuse_param.offset_power_wf0_2g_l);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_2g_m", &cali_param->w2_efuse_param.offset_power_wf0_2g_m);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_2g_h", &cali_param->w2_efuse_param.offset_power_wf0_2g_h);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5200", &cali_param->w2_efuse_param.offset_power_wf0_5200);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5300", &cali_param->w2_efuse_param.offset_power_wf0_5300);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5530", &cali_param->w2_efuse_param.offset_power_wf0_5530);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5660", &cali_param->w2_efuse_param.offset_power_wf0_5660);
    aml_get_s8_item(varbuf, len, "offset_power_wf0_5780", &cali_param->w2_efuse_param.offset_power_wf0_5780);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_2g_l", &cali_param->w2_efuse_param.offset_power_wf1_2g_l);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_2g_m", &cali_param->w2_efuse_param.offset_power_wf1_2g_m);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_2g_h", &cali_param->w2_efuse_param.offset_power_wf1_2g_h);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5200", &cali_param->w2_efuse_param.offset_power_wf1_5200);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5300", &cali_param->w2_efuse_param.offset_power_wf1_5300);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5530", &cali_param->w2_efuse_param.offset_power_wf1_5530);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5660", &cali_param->w2_efuse_param.offset_power_wf1_5660);
    aml_get_s8_item(varbuf, len, "offset_power_wf1_5780", &cali_param->w2_efuse_param.offset_power_wf1_5780);

    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_2g_l", &cali_param->w2_efuse_param.second_offset_power_wf0_2g_l);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_2g_m", &cali_param->w2_efuse_param.second_offset_power_wf0_2g_m);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_2g_h", &cali_param->w2_efuse_param.second_offset_power_wf0_2g_h);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_5200", &cali_param->w2_efuse_param.second_offset_power_wf0_5200);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_5300", &cali_param->w2_efuse_param.second_offset_power_wf0_5300);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_5530", &cali_param->w2_efuse_param.second_offset_power_wf0_5530);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_5660", &cali_param->w2_efuse_param.second_offset_power_wf0_5660);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf0_5780", &cali_param->w2_efuse_param.second_offset_power_wf0_5780);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_2g_l", &cali_param->w2_efuse_param.second_offset_power_wf1_2g_l);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_2g_m", &cali_param->w2_efuse_param.second_offset_power_wf1_2g_m);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_2g_h", &cali_param->w2_efuse_param.second_offset_power_wf1_2g_h);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_5200", &cali_param->w2_efuse_param.second_offset_power_wf1_5200);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_5300", &cali_param->w2_efuse_param.second_offset_power_wf1_5300);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_5530", &cali_param->w2_efuse_param.second_offset_power_wf1_5530);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_5660", &cali_param->w2_efuse_param.second_offset_power_wf1_5660);
    aml_get_s8_item(varbuf, len, "second_offset_power_wf1_5780", &cali_param->w2_efuse_param.second_offset_power_wf1_5780);

    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_2g_l", &cali_param->w2_efuse_param.txt_shift_value_wf0_2g_l);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_2g_m", &cali_param->w2_efuse_param.txt_shift_value_wf0_2g_m);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_2g_h", &cali_param->w2_efuse_param.txt_shift_value_wf0_2g_h);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_5200", &cali_param->w2_efuse_param.txt_shift_value_wf0_5200);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_5300", &cali_param->w2_efuse_param.txt_shift_value_wf0_5300);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_5530", &cali_param->w2_efuse_param.txt_shift_value_wf0_5530);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_5660", &cali_param->w2_efuse_param.txt_shift_value_wf0_5660);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf0_5780", &cali_param->w2_efuse_param.txt_shift_value_wf0_5780);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_2g_l", &cali_param->w2_efuse_param.txt_shift_value_wf1_2g_l);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_2g_m", &cali_param->w2_efuse_param.txt_shift_value_wf1_2g_m);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_2g_h", &cali_param->w2_efuse_param.txt_shift_value_wf1_2g_h);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_5200", &cali_param->w2_efuse_param.txt_shift_value_wf1_5200);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_5300", &cali_param->w2_efuse_param.txt_shift_value_wf1_5300);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_5530", &cali_param->w2_efuse_param.txt_shift_value_wf1_5530);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_5660", &cali_param->w2_efuse_param.txt_shift_value_wf1_5660);
    aml_get_s8_item(varbuf, len, "txt_shift_value_wf1_5780", &cali_param->w2_efuse_param.txt_shift_value_wf1_5780);

    aml_get_s8_item(varbuf, len, "pmu_efuse_en", &cali_param->w2_efuse_param.pmu_efuse_en);

    aml_get_s8_item(varbuf, len, "rg_pmu_bucka_efuse_trim_i_osc", &cali_param->w2_efuse_param.rg_pmu_bucka_efuse_trim_i_osc);
    aml_get_s8_item(varbuf, len, "rg_pmu_bucka_rsv3", &cali_param->w2_efuse_param.rg_pmu_bucka_rsv3);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_efuse_trim_i_osc", &cali_param->w2_efuse_param.rg_pmu_buckd_efuse_trim_i_osc);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_efuse_trim_offset_vref", &cali_param->w2_efuse_param.rg_pmu_buckd_efuse_trim_offset_vref);
    aml_get_s8_item(varbuf, len, "rg_aoldo_vosel", &cali_param->w2_efuse_param.rg_aoldo_vosel);
    aml_get_s8_item(varbuf, len, "rg_strup_rsva", &cali_param->w2_efuse_param.rg_strup_rsva);
    aml_get_s8_item(varbuf, len, "rg_ao_hifldo_vosel", &cali_param->w2_efuse_param.rg_ao_hifldo_vosel);
    aml_get_s8_item(varbuf, len, "rg_aldo_vo_adj", &cali_param->w2_efuse_param.rg_aldo_vo_adj);
    aml_get_s8_item(varbuf, len, "rg_bg_tc_adj", &cali_param->w2_efuse_param.rg_bg_tc_adj);
    aml_get_s8_item(varbuf, len, "rg_pmu_32k_adj", &cali_param->w2_efuse_param.rg_pmu_32k_adj);
    aml_get_s8_item(varbuf, len, "rg_pmu_bucka_efuse_trim_slp_csp", &cali_param->w2_efuse_param.rg_pmu_bucka_efuse_trim_slp_csp);
    aml_get_s8_item(varbuf, len, "rg_pmu_bucka_efuse_trim_iref_vlreg", &cali_param->w2_efuse_param.rg_pmu_bucka_efuse_trim_iref_vlreg);
    aml_get_s8_item(varbuf, len, "rg_pmu_bucka_efuse_trim_iref_vhreg", &cali_param->w2_efuse_param.rg_pmu_bucka_efuse_trim_iref_vhreg);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_ton_min_trim", &cali_param->w2_efuse_param.rg_pmu_buckd_ton_min_trim);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_cs_gain_ramp", &cali_param->w2_efuse_param.rg_pmu_buckd_cs_gain_ramp);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_vsel_vh", &cali_param->w2_efuse_param.rg_pmu_buckd_vsel_vh);
    aml_get_s8_item(varbuf, len, "rg_pmu_buckd_capslp_ramp", &cali_param->w2_efuse_param.rg_pmu_buckd_capslp_ramp);

    cali_param->version = version;
    cali_param->cali_config = cali_config;

    printk("======>>>>>> version = %d\n", cali_param->version);
    printk("======>>>>>> cali_config = %d\n", cali_param->cali_config);
    printk("======>>>>>> freq_offset = %d\n", cali_param->freq_offset);
    printk("======>>>>>> htemp_freq_offset = %d\n", cali_param->htemp_freq_offset);
    printk("======>>>>>> tssi_2g_offset = 0x%x\n", cali_param->tssi_2g_offset);
    printk("======>>>>>> tssi_5g_offset_5200 = 0x%x\n", cali_param->tssi_5g_offset[0]);
    printk("======>>>>>> tssi_5g_offset_5400 = 0x%x\n", cali_param->tssi_5g_offset[1]);
    printk("======>>>>>> tssi_5g_offset_5600 = 0x%x\n", cali_param->tssi_5g_offset[2]);
    printk("======>>>>>> tssi_5g_offset_5800 = 0x%x\n", cali_param->tssi_5g_offset[3]);
    printk("======>>>>>> wf2g_spur_rmen = %d\n", cali_param->wf2g_spur_rmen);
    printk("======>>>>>> spur_freq = %d\n", cali_param->spur_freq);
    printk("======>>>>>> rf_count = %d\n", cali_param->rf_num);

    printk("======>>>>>> cw2mod = %x\n", cali_param->cw2mod[0]);

    printk("======>>>>>> wf2g_he20_tpwr = %d\n", cali_param->wf2g_he20_tpwr[0]);
    printk("======>>>>>> wf2g_he40_tpwr = %d\n", cali_param->wf2g_he40_tpwr[0]);

    printk("======>>>>>> wf5g_he20_tpwr_h = %d\n", cali_param->wf5g_he20_tpwr_h[0]);
    printk("======>>>>>> wf5g_he40_tpwr_h = %d\n", cali_param->wf5g_he40_tpwr_h[0]);
    printk("======>>>>>> wf5g_he80_tpwr_h = %d\n", cali_param->wf5g_he80_tpwr_h[0]);

    printk("======>>>>>> customer_efuse_en = 0x%x\n", cali_param->w2_efuse_param.customer_efuse_en);
    printk("======>>>>>> FT_efuse_en = 0x%x\n", cali_param->w2_efuse_param.FT_efuse_en);


    return 0;
}

struct aml_product_id {
    unsigned int pid;
    char name[16];
};

struct aml_vendor_sn {
    unsigned int sn;
    char name[32];
};

#define AML_PRODUCT_ID_NUM 3
#define AML_VENDOR_SN_NUM  7

static struct aml_product_id aml_pid_tbl[AML_PRODUCT_ID_NUM] = {
    {0x640, "SDIO"},
    {0x641, "USB"},
    {0x642, "PCIe"}
};

static struct aml_vendor_sn aml_vsn_tbl[AML_VENDOR_SN_NUM] = {
    {0x01, "iton"},
    {0x02, "ampak"},
    {0x03, "fn-link"},
    {0x04, "cvte"},
    {0x05, "gaosd"},
    {0x06, "cdtech"},
    {0x07, "quectel"}
};

static char *aml_product_id2str(unsigned int pid)
{
    int i;

    for (i = 0; i < AML_PRODUCT_ID_NUM; i++) {
        if (aml_pid_tbl[i].pid == pid)
            return aml_pid_tbl[i].name;
    }

    return "unknown";
}

static char *aml_vendor_sn2str(unsigned int sn)
{
    int i;

    for (i = 0; i < AML_VENDOR_SN_NUM; i++) {
        if (aml_vsn_tbl[i].sn == ((sn & 0xff00) >> 8)) {
            return aml_vsn_tbl[i].name;
        }
    }
    return "unknown";
}

static int aml_get_cali_param(struct aml_hw *aml_hw, struct Cali_Param *cali_param)
{
    const struct firmware *cfg_fw = NULL;
    int ret = 0, len = 0, i = 0;
    unsigned int product_id = 0, vendor_sn = 0;
    unsigned char vendor_rf[128];
    unsigned int second_calibrationed = 0;

    product_id = aml_efuse_read(aml_hw, 0x0);
    product_id = (product_id & 0xffff0000) >> 16;
    vendor_sn = aml_efuse_read(aml_hw, 0xf);
    vendor_sn = vendor_sn & 0xffff;
    second_calibrationed = aml_efuse_read(aml_hw, 0x7);
    second_calibrationed = (second_calibrationed & 0x80000000) >> 31;

    if (second_calibrationed == 0) {
        sprintf(vendor_rf, "aml_w2_rf_%04x_%04x.txt", product_id, vendor_sn);
    } else {
        /* if secondary calibration is performed, use the default txt directly */
        sprintf(vendor_rf, WIFI_CALI_FILENAME);
    }
    ret = request_firmware(&cfg_fw, vendor_rf, aml_hw->dev);

    if (ret != 0) {
        if (second_calibrationed == 0) {
            /* if no second_calibrationed and no specific txt then use default */
            AML_INFO("vendor customized %s not existed, use default", vendor_rf);
            ret = request_firmware(&cfg_fw, WIFI_CALI_FILENAME, aml_hw->dev);
        }
        if  (ret != 0) {
            AML_INFO("failed to get %s (%d)", WIFI_CALI_FILENAME, ret);
            return ret;
        }
    } else {
        if (second_calibrationed == 0) {
            AML_INFO("vendor customized %s config loaded:", vendor_rf);
            AML_INFO("product id:%s, vendor sn:%s ver:%02x",
                    aml_product_id2str(product_id),
                    aml_vendor_sn2str(vendor_sn),
                    vendor_sn & 0xff);
        }
    }

    len = aml_process_cali_content((char *)cfg_fw->data, cfg_fw->size);
    aml_parse_cali_param((char *)cfg_fw->data, len, cali_param);
    if (!((cali_param->version <= WIFI_CALI_VERSION) && (cali_param->version >= 17))) {
        AML_INFO("*******************************************************");
        AML_INFO("WARNING: rf cali file out of date, please update!!! ");
        AML_INFO("*******************************************************");
        ret = -1;
    }
    release_firmware(cfg_fw);

    return ret;
}

int aml_config_cali_param(struct aml_hw *aml_hw)
{
    struct Cali_Param cali_param;
    int err = 0;

    if (g_cali_cfg_done)
        return 0;

    memset((void *)&cali_param, 0, sizeof(struct Cali_Param));

    err = aml_get_cali_param(aml_hw, &cali_param);
    if ((err == 0) && ((err = aml_set_cali_param_req(aml_hw, &cali_param)) == 0)) {
        AML_INFO("set calibration parameter success");
        g_cali_cfg_done = 1;
    } else {
        AML_INFO("set calibration parameter failed");
    }

    return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
void aml_cfg80211_sched_scan_results(struct wiphy *wiphy, uint64_t reqid)
{
    cfg80211_sched_scan_results(wiphy);
}
#else
void aml_cfg80211_sched_scan_results(struct wiphy *wiphy, uint64_t reqid)
{
    cfg80211_sched_scan_results(wiphy, reqid);
}
#endif


static int aml_cfg80211_sched_scan_start(struct wiphy *wiphy,
    struct net_device *dev, struct cfg80211_sched_scan_request *request)
{
    struct aml_vif *aml_vif = netdev_priv(dev);

    aml_send_sched_scan_req(aml_vif, request);

    return 0;
}

static int aml_cfg80211_sched_scan_stop(struct wiphy *wiphy, struct net_device *dev, u64 reqid)
{
    struct aml_vif *aml_vif = netdev_priv(dev);

    aml_send_sched_scan_stop_req(aml_vif, reqid);

    return 0;
}


static struct cfg80211_ops aml_cfg80211_ops = {
    .suspend = aml_cfg80211_suspend,
    .resume = aml_cfg80211_resume,
    .add_virtual_intf = aml_cfg80211_add_iface,
    .del_virtual_intf = aml_cfg80211_del_iface,
    .change_virtual_intf = aml_cfg80211_change_iface,
    .scan = aml_cfg80211_scan,
    .connect = aml_cfg80211_connect,
    .disconnect = aml_cfg80211_disconnect,
    .add_key = aml_cfg80211_add_key,
    .get_key = aml_cfg80211_get_key,
    .del_key = aml_cfg80211_del_key,
    .set_default_key = aml_cfg80211_set_default_key,
    .set_default_mgmt_key = aml_cfg80211_set_default_mgmt_key,
    .add_station = aml_cfg80211_add_station,
    .del_station = aml_cfg80211_del_station,
    .change_station = aml_cfg80211_change_station,
    .mgmt_tx = aml_cfg80211_mgmt_tx,
    //.mgmt_tx_cancel_wait = aml_cfg80211_mgmt_tx_cancel_wait,
    .start_ap = aml_cfg80211_start_ap,
    .change_beacon = aml_cfg80211_change_beacon,
    .stop_ap = aml_cfg80211_stop_ap,
    .set_monitor_channel = aml_cfg80211_set_monitor_channel,
    .probe_client = aml_cfg80211_probe_client,
    .set_wiphy_params = aml_cfg80211_set_wiphy_params,
    .set_txq_params = aml_cfg80211_set_txq_params,
    .set_tx_power = aml_cfg80211_set_tx_power,
//    .get_tx_power = aml_cfg80211_get_tx_power,
    .set_power_mgmt = aml_cfg80211_set_power_mgmt,
    .get_station = aml_cfg80211_get_station,
    .dump_station = aml_cfg80211_dump_station,
    .remain_on_channel = aml_cfg80211_remain_on_channel,
    .cancel_remain_on_channel = aml_cfg80211_cancel_remain_on_channel,
    .dump_survey = aml_cfg80211_dump_survey,
    .get_channel = aml_cfg80211_get_channel,
    .start_radar_detection = aml_cfg80211_start_radar_detection,
    .update_ft_ies = aml_cfg80211_update_ft_ies,
    .set_cqm_rssi_config = aml_cfg80211_set_cqm_rssi_config,
    .channel_switch = aml_cfg80211_channel_switch,
    .tdls_channel_switch = aml_cfg80211_tdls_channel_switch,
    .tdls_cancel_channel_switch = aml_cfg80211_tdls_cancel_channel_switch,
    .tdls_mgmt = aml_cfg80211_tdls_mgmt,
    .tdls_oper = aml_cfg80211_tdls_oper,
    .change_bss = aml_cfg80211_change_bss,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    .external_auth = aml_cfg80211_external_auth,
#endif
    .set_rekey_data =  aml_cfg80211_set_rekey_data,
    .sched_scan_start = aml_cfg80211_sched_scan_start,
    .sched_scan_stop = aml_cfg80211_sched_scan_stop,
};


/*********************************************************************
 * Init/Exit functions
 *********************************************************************/
static void aml_wdev_unregister(struct aml_hw *aml_hw)
{
    struct aml_vif *aml_vif, *tmp;

    rtnl_lock();
    list_for_each_entry_safe(aml_vif, tmp, &aml_hw->vifs, list) {
        aml_cfg80211_del_iface(aml_hw->wiphy, &aml_vif->wdev);
    }
    rtnl_unlock();
}

static void aml_set_vers(struct aml_hw *aml_hw)
{
    u32 vers = aml_hw->version_cfm.version_lmac;

    AML_DBG(AML_FN_ENTRY_STR);

    snprintf(aml_hw->wiphy->fw_version,
             sizeof(aml_hw->wiphy->fw_version), "%d.%d.%d.%d",
             (vers & (0xff << 24)) >> 24, (vers & (0xff << 16)) >> 16,
             (vers & (0xff <<  8)) >>  8, (vers & (0xff <<  0)) >>  0);

    aml_hw->machw_type = aml_machw_type(aml_hw->version_cfm.version_machw_2);
}

static void aml_reg_notifier(struct wiphy *wiphy,
                              struct regulatory_request *request)
{
    struct aml_hw *aml_hw = wiphy_priv(wiphy);

    AML_INFO("initiator=%d, hint_type=%d, alpha=%s, region=%d\n",
            request->initiator, request->user_reg_hint_type,
            request->alpha2, request->dfs_region);

    // For now trust all initiator
    aml_radar_set_domain(&aml_hw->radar, request->dfs_region);
    aml_send_me_chan_config_req(aml_hw);
}

static void aml_enable_mesh(struct aml_hw *aml_hw)
{
    struct wiphy *wiphy = aml_hw->wiphy;

    if (!aml_mod_params.mesh)
        return;

    aml_cfg80211_ops.add_mpath = aml_cfg80211_add_mpath;
    aml_cfg80211_ops.del_mpath = aml_cfg80211_del_mpath;
    aml_cfg80211_ops.change_mpath = aml_cfg80211_change_mpath;
    aml_cfg80211_ops.get_mpath = aml_cfg80211_get_mpath;
    aml_cfg80211_ops.dump_mpath = aml_cfg80211_dump_mpath;
    aml_cfg80211_ops.get_mpp = aml_cfg80211_get_mpp;
    aml_cfg80211_ops.dump_mpp = aml_cfg80211_dump_mpp;
    aml_cfg80211_ops.get_mesh_config = aml_cfg80211_get_mesh_config;
    aml_cfg80211_ops.update_mesh_config = aml_cfg80211_update_mesh_config;
    aml_cfg80211_ops.join_mesh = aml_cfg80211_join_mesh;
    aml_cfg80211_ops.leave_mesh = aml_cfg80211_leave_mesh;

    wiphy->flags |= (WIPHY_FLAG_MESH_AUTH | WIPHY_FLAG_IBSS_RSN);
    wiphy->features |= NL80211_FEATURE_USERSPACE_MPM;
    wiphy->interface_modes |= BIT(NL80211_IFTYPE_MESH_POINT);

    aml_limits[0].types |= BIT(NL80211_IFTYPE_MESH_POINT);
    aml_limits_dfs[0].types |= BIT(NL80211_IFTYPE_MESH_POINT);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
static inline void
aml_wiphy_set_max_sched_scans(struct wiphy *wiphy, uint8_t max_scans)
{
    if (max_scans == 0)
        wiphy->flags &= ~WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
    else
        wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
}
#else
static inline void
aml_wiphy_set_max_sched_scans(struct wiphy *wiphy, uint8_t max_scans)
{
    wiphy->max_sched_scan_reqs = max_scans;
}
#endif /* KERNEL_VERSION(4, 12, 0) */

/**
 * aml_cfg80211_add_connected_pno_support() - Set connected PNO support
 * @wiphy: Pointer to wireless phy
 *
 * This function is used to set connected PNO support to kernel
 *
 * Return: None
 */
#if defined(CFG80211_REPORT_BETTER_BSS_IN_SCHED_SCAN)
static void aml_cfg80211_add_connected_pno_support(struct wiphy *wiphy)
{
    wiphy_ext_feature_set(wiphy,
        NL80211_EXT_FEATURE_SCHED_SCAN_RELATIVE_RSSI);
}
#else
static void aml_cfg80211_add_connected_pno_support(struct wiphy *wiphy)
{
    return;
}
#endif

static int aml_inetaddr_event(struct notifier_block *this,
        unsigned long event, void *ptr)
{
    struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
    struct net_device *ndev;
    struct aml_vif *aml_vif;

    if (!ifa || !(ifa->ifa_dev->dev))
        return NOTIFY_DONE;
    if (ifa->ifa_dev->dev->netdev_ops != &aml_netdev_ops)
        return NOTIFY_DONE;

    ndev = ifa->ifa_dev->dev;
    aml_vif = netdev_priv(ndev);

    switch (event) {
        case NETDEV_UP:
            if (((aml_vif->vif_index == AML_STA_VIF_IDX)
                    && (aml_vif->wdev.iftype == NL80211_IFTYPE_STATION))
                    || ((aml_vif->vif_index == AML_P2P_VIF_IDX)
                    && (aml_vif->wdev.iftype == NL80211_IFTYPE_P2P_CLIENT))) {
                memcpy(aml_vif->ipv4_addr, (uint8_t *)&ifa->ifa_address, IPV4_ADDR_LEN);
                memcpy(aml_vif->subnet_mask, (uint8_t *)&ifa->ifa_mask, IPV4_ADDR_LEN);
                aml_send_notify_ip(aml_vif, IPV4_VER, aml_vif->ipv4_addr);
#ifdef SCC_STA_SOFTAP
                aml_scc_check_chan_conflict(aml_vif->aml_hw);
#endif
            }
            AML_INFO("dev %s is up, comm:%s, pid:%d, vif:%d, ipv4:%pI4",
                    ndev->name, current->comm, current->pid,
                    aml_vif->vif_index, aml_vif->ipv4_addr);
            aml_connect_flags_clr(aml_vif, AML_GETTING_IP);
            break;
        case NETDEV_DOWN:
            memset(aml_vif->ipv4_addr, 0, IPV4_ADDR_LEN);
            AML_INFO("dev %s is down, comm:%s, pid:%d, vif:%d",
                    ndev->name, current->comm, current->pid,
                    aml_vif->vif_index);
            break;
        default:
            break;
    }

    return NOTIFY_DONE;
}

static int aml_inetaddr6_event(struct notifier_block *this,
        unsigned long event, void *ptr)
{
    struct inet6_ifaddr *ifa = (struct inet6_ifaddr *)ptr;
    struct net_device *ndev;
    struct aml_vif *aml_vif;
    struct aml_wq *aml_wq;
    uint8_t *ipv6_addr;

    if (!ifa || !(ifa->idev->dev))
        return NOTIFY_DONE;
    if (ifa->idev->dev->netdev_ops != &aml_netdev_ops)
        return NOTIFY_DONE;

    ndev = ifa->idev->dev;
    aml_vif = netdev_priv(ndev);

    switch (event) {
        case NETDEV_UP:
            aml_wq = aml_wq_alloc(IPV6_ADDR_LEN);
            if (!aml_wq) {
                AML_INFO("alloc wq out of memory");
                return NOTIFY_DONE;
            }
            aml_wq->aml_vif = aml_vif;
            aml_wq->id = AML_WQ_IPV6;
            ipv6_addr = (uint8_t *)aml_wq->data;
            memcpy(ipv6_addr, (uint8_t *)&ifa->addr, IPV6_ADDR_LEN);
            aml_wq_add(aml_vif->aml_hw, aml_wq);
            AML_INFO("dev %s is up, comm:%s, pid:%d vif:%d, ipv6:%pI6",
                    ndev->name, current->comm, current->pid,
                    aml_vif->vif_index, &ifa->addr);
            break;
        case NETDEV_DOWN:
            AML_INFO("dev %s is down, comm:%s, pid:%d, vif:%d ipv6:%pI6",
                    ndev->name, current->comm, current->pid,
                    aml_vif->vif_index, &ifa->addr);
            break;
        default:
            break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block aml_ipv4_cb = {
    .notifier_call = aml_inetaddr_event
};

static struct notifier_block aml_ipv6_cb = {
    .notifier_call = aml_inetaddr6_event
};

static int aml_wiphy_addresses_add(struct wiphy *wiphy, struct aml_cfg cfg)
{
    wiphy->addresses = (struct mac_address *)kmalloc(ETH_ALEN * AML_IFTYPE_MAX, GFP_KERNEL);
    if (!wiphy->addresses) {
        wiphy_err(wiphy, "kmalloc mac address failed\n");
        return -1;
    }
    wiphy->n_addresses = AML_IFTYPE_MAX;
    memcpy(wiphy->addresses + 0, cfg.vif0_mac, ETH_ALEN);
    memcpy(wiphy->addresses + 1, cfg.vif1_mac, ETH_ALEN);
    memcpy(wiphy->addresses + 2, cfg.vif2_mac, ETH_ALEN);

    return 0;
}

static void aml_wiphy_addresses_free(struct wiphy *wiphy)
{
    if (wiphy->addresses != NULL) {
        kfree(wiphy->addresses);
    }
}

static int aml_hwctx_buf_init(struct aml_hw *aml_hw)
{
    uint32_t buf_size = 0;
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    size_t out_size = 0;
#endif

    if (!aml_hw)
        return -1;

    buf_size = sizeof(struct aml_sta) * (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX);
    aml_hw->sta_table = kmalloc(buf_size, GFP_ATOMIC);
    if (!aml_hw->sta_table) {
        AML_INFO("Failed to alloc sta table");
        return -1;
    }
    memset(aml_hw->sta_table, 0, buf_size);

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    aml_hw->rxbufs = (struct aml_ipc_buf *)aml_prealloc_get(PREALLOC_BUF_TYPE_RX,
            sizeof(struct aml_ipc_buf) * AML_RXBUFF_MAX, &out_size);
    aml_hw->txq = (struct aml_txq *)aml_prealloc_get(PREALLOC_BUF_TYPE_TXQ,
            sizeof(struct aml_txq) * NX_NB_TXQ, &out_size);
#endif
    buf_size = sizeof(struct aml_stats);
    aml_hw->stats = kzalloc(buf_size, GFP_ATOMIC);
    if (!aml_hw->stats) {
        AML_INFO("Failed to alloc stats buf");
        kfree(aml_hw->sta_table);
        return -1;
    }

    if (aml_bus_type != PCIE_MODE) {
        buf_size = sizeof(struct scan_results) * (SCAN_RESULTS_MAX_CNT);
        aml_hw->scan_results = kmalloc(buf_size, GFP_ATOMIC);
        if (!aml_hw->scan_results) {
            AML_INFO("Failed to alloc scan results buf");
            kfree(aml_hw->sta_table);
            kfree(aml_hw->stats);
            return -1;
        }

        aml_hw->scanres_payload_buf = kmalloc(SCAN_RESULTS_MAX_CNT*500, GFP_ATOMIC);
        if (!aml_hw->scanres_payload_buf) {
            AML_INFO("Failed to alloc scans payload buffer");
            aml_hw->scanres_payload_buf = NULL;
            kfree(aml_hw->sta_table);
            kfree(aml_hw->stats);
            kfree(aml_hw->scan_results);
            return -1;
        }
    }

    return 0;
}

static void aml_hwctx_buf_deinit(struct aml_hw *aml_hw)
{
    if (!aml_hw)
        return;

    if (aml_hw->sta_table)
        kfree(aml_hw->sta_table);
    if (aml_hw->stats)
        kfree(aml_hw->stats);
    if (aml_bus_type != PCIE_MODE) {
        if (aml_hw->scan_results)
            kfree(aml_hw->scan_results);
        if (aml_hw->scanres_payload_buf)
            kfree(aml_hw->scanres_payload_buf);
    }
}

#ifdef CONFIG_AML_NAPI
/*napi poll function to process rx skb and send to network stack*/
int aml_napi_poll(struct napi_struct *napi, int weight)
{
    struct aml_hw *aml_hw;
    int done = 0;
    unsigned long flags;
    struct sk_buff_head rx_process_queue;
    struct sk_buff * skb;
    aml_hw = container_of(napi, struct aml_hw, napi);
    __skb_queue_head_init(&rx_process_queue);
    spin_lock_irqsave(&aml_hw->napi_rx_upload_queue.lock, flags);
    skb_queue_splice_tail_init(&aml_hw->napi_rx_upload_queue, &rx_process_queue);
    spin_unlock_irqrestore(&aml_hw->napi_rx_upload_queue.lock, flags);
    while ((done < weight) && ((skb = __skb_dequeue(&rx_process_queue)) != NULL)) {
        REG_SW_SET_PROFILING(aml_hw, SW_PROF_IEEE80211RX);
        if ((aml_hw->gro_enable) && (!skb_cloned(skb)))
            napi_gro_receive(&aml_hw->napi, skb);
        else
            netif_receive_skb(skb);
        REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_IEEE80211RX);
        done++;
    }
    /*if num of sendup skb < weight,complete napi poll.else repoll*/
    if (done < weight) {
        napi_complete(napi);
        if (!skb_queue_empty(&aml_hw->napi_rx_upload_queue)) {
            napi_schedule(napi);
        }
    }
    return done;
}
#endif

extern lp_shutdown_func g_lp_shutdown_func;
void aml_interface_shutdown_init(struct aml_hw *aml_hw)
{
    //save aml_hw
    g_pst_aml_hw = aml_hw;

    //aml_lp_shutdown_send_req
    g_lp_shutdown_func = aml_lp_shutdown_send_req;
}

static int aml_panic_callback(struct notifier_block *nb, unsigned long event, void *arg)
{
    /* can support more operation for debug when panic happened,
     * just print version info.
     * */
    printk("panic version info:\n");
    printk("driver version:%s\n", AML_VERS_REV);
    printk("fw info:%s\n", FIRMWARE_INFO);

    return NOTIFY_DONE;
}

static struct notifier_block aml_panic_notifier = {
    .notifier_call = aml_panic_callback
};

static int aml_register_panic_notifier(void)
{
    atomic_notifier_chain_register(&panic_notifier_list, &aml_panic_notifier);

    return 0;
}

static void aml_unregister_panic_notifier(void)
{
    atomic_notifier_chain_unregister(&panic_notifier_list, &aml_panic_notifier);
}

const struct wiphy_vendor_command aml_wiphy_vendor_commands[] =
{
    ANDROID_MDNS_OFFLOAD_VENDOR_CMD,
};

static int aml_interface_add_all(struct aml_hw *aml_hw, bool custchan)
{
    struct wireless_dev *wdev;
    enum nl80211_iftype iftype;
    int i;

    for (i = 0; i < AML_IFTYPE_MAX; i++) {
        rtnl_lock();
        iftype = (custchan && (i == AML_IFTYPE_STA)) ? NL80211_IFTYPE_MONITOR : NL80211_IFTYPE_STATION;
        wdev = aml_interface_add(aml_hw, aml_intf_name[i], NET_NAME_UNKNOWN, iftype, NULL);
        rtnl_unlock();
        if (!wdev) {
            pr_err("add interface %s failed\n", aml_intf_name[i]);
            return -ENOMEM;
        }
        pr_info("new interface create %s success\n", wdev->netdev->name);
#ifdef CONFIG_AML_NAPI
        if (i == AML_IFTYPE_STA) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
            netif_napi_add(wdev->netdev, &aml_hw->napi, aml_napi_poll, AML_NAPI_WEIGHT);
#else
            netif_napi_add_weight(wdev->netdev, &aml_hw->napi, aml_napi_poll, AML_NAPI_WEIGHT);
#endif
            napi_enable(&aml_hw->napi);
            __skb_queue_head_init(&aml_hw->napi_rx_upload_queue);
            __skb_queue_head_init(&aml_hw->napi_rx_pending_queue);
            aml_hw->napi_pend_pkt_num = 16;
            aml_hw->napi_enable = 0;
            aml_hw->gro_enable = 0;
        }
#endif
    }
    return 0;
}

extern struct aml_bus_state_detect bus_state_detect;
int aml_cfg80211_init(struct aml_plat *aml_plat, void **platform_data)
{
    struct aml_hw *aml_hw = NULL;
    struct aml_cfg cfg;
    struct wiphy *wiphy;
    struct wireless_dev *wdev;
    char alpha2[3] = {'0', '0', '\0'};
    int ret = 0;
    int i;

    AML_DBG(AML_FN_ENTRY_STR);

    /* create a new wiphy for use with cfg80211 */
    wiphy = wiphy_new(&aml_cfg80211_ops, sizeof(struct aml_hw));
    if (!wiphy) {
        dev_err(aml_platform_get_dev(aml_plat), "Failed to create new wiphy\n");
        ret = -ENOMEM;
        goto err_out;
    }

    wiphy->n_vendor_commands = ARRAY_SIZE(aml_wiphy_vendor_commands);
    wiphy->vendor_commands = aml_wiphy_vendor_commands;

    aml_hw = wiphy_priv(wiphy);
    aml_hw->wiphy = wiphy;
    aml_hw->plat = aml_plat;
    aml_hw->dev = aml_platform_get_dev(aml_plat);
    aml_hw->mod_params = &aml_mod_params;
    aml_hw->tcp_pacing_shift = 4;

    /* set device pointer for wiphy */
    set_wiphy_dev(wiphy, aml_hw->dev);

    /* init sw context buffers */
    if ((ret = aml_hwctx_buf_init(aml_hw))) {
        AML_INFO("Failed to init sw context buffers");
        ret = -ENOMEM;
        goto err_out;
    }

    /* Create cache to allocate sw_txhdr */
    aml_hw->sw_txhdr_cache = KMEM_CACHE(aml_sw_txhdr, 0);
    if (!aml_hw->sw_txhdr_cache) {
        wiphy_err(wiphy, "Cannot allocate cache for sw TX header\n");
        ret = -ENOMEM;
        goto err_cache;
    }

    aml_hw->vif_started = 0;
    aml_hw->monitor_vif = AML_INVALID_VIF;

    aml_hw->scan_ie.addr = NULL;

    for (i = 0; i < NX_ITF_MAX; i++)
        aml_hw->avail_idx_map |= BIT(i);

    printk("%s:%d\n", __func__, __LINE__);
    aml_hwq_init(aml_hw);
    aml_txq_prepare(aml_hw);

    printk("%s:%d\n", __func__, __LINE__);
    aml_mu_group_init(aml_hw);

    aml_hw->roc = NULL;
    wiphy->mgmt_stypes = aml_default_mgmt_stypes;

    wiphy->wowlan = &wowlan_stub;

    wiphy->bands[NL80211_BAND_2GHZ] = &aml_band_2GHz;
    wiphy->bands[NL80211_BAND_5GHZ] = &aml_band_5GHz;
    wiphy->interface_modes =
        BIT(NL80211_IFTYPE_STATION)     |
        BIT(NL80211_IFTYPE_AP)          |
        BIT(NL80211_IFTYPE_AP_VLAN)     |
        BIT(NL80211_IFTYPE_P2P_CLIENT)  |
        BIT(NL80211_IFTYPE_P2P_GO)      |
        BIT(NL80211_IFTYPE_MONITOR);
    wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
        WIPHY_FLAG_HAS_CHANNEL_SWITCH |
        WIPHY_FLAG_4ADDR_STATION |
        WIPHY_FLAG_4ADDR_AP |
        WIPHY_FLAG_REPORTS_OBSS |
        WIPHY_FLAG_OFFCHAN_TX;

    /*init for pno*/
    aml_wiphy_set_max_sched_scans(wiphy, 1);
    wiphy->max_match_sets       = PNO_MAX_SUPP_NETWORKS;
    wiphy->max_sched_scan_ie_len = SCANU_MAX_IE_LEN;
    aml_cfg80211_add_connected_pno_support(wiphy);

    wiphy->max_scan_ssids = SCAN_SSID_MAX;
    wiphy->max_scan_ie_len = SCANU_MAX_IE_LEN;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
    wiphy->support_mbssid = 1;
#endif

    wiphy->max_num_csa_counters = BCN_MAX_CSA_CPT;

    wiphy->max_remain_on_channel_duration = aml_hw->mod_params->roc_dur_max;

    wiphy->features |= NL80211_FEATURE_NEED_OBSS_SCAN |
        NL80211_FEATURE_SK_TX_STATUS |
        NL80211_FEATURE_VIF_TXPOWER |
        NL80211_FEATURE_ACTIVE_MONITOR |
        NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    wiphy->features |= NL80211_FEATURE_SAE;
#endif

    wiphy->iface_combinations   = aml_combinations;
    /* -1 not to include combination with radar detection, will be re-added in
       aml_handle_dynparams if supported */
    wiphy->n_iface_combinations = ARRAY_SIZE(aml_combinations) - 1;
    wiphy->reg_notifier = aml_reg_notifier;

    aml_apply_regdom(aml_hw, wiphy, alpha2);
#ifdef CONFIG_CUSTOMER_11D_FORBIDDEN
    wiphy->regulatory_flags |= REGULATORY_COUNTRY_IE_IGNORE;
    AML_INFO("set REGULATORY_COUNTRY_IE_IGNORE\n");
#endif

    wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

    wiphy->cipher_suites = cipher_suites;
    wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites) - NB_RESERVED_CIPHER;

    aml_hw->ext_capa[0] = WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;
    aml_hw->ext_capa[2] = WLAN_EXT_CAPA3_MULTI_BSSID_SUPPORT;
    aml_hw->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;
    // max number of MSDUs in A-MSDU = 3 (=> 8 subframes max)
    aml_hw->ext_capa[7] |= WLAN_EXT_CAPA8_MAX_MSDU_IN_AMSDU_LSB;
    aml_hw->ext_capa[8] = WLAN_EXT_CAPA9_MAX_MSDU_IN_AMSDU_MSB;

    wiphy->extended_capabilities = aml_hw->ext_capa;
    wiphy->extended_capabilities_mask = aml_hw->ext_capa;
    wiphy->extended_capabilities_len = ARRAY_SIZE(aml_hw->ext_capa);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
#ifndef CONFIG_AML_USE_TASK
    if (aml_bus_type == PCIE_MODE) {
        tasklet_init(&aml_hw->task, aml_pcie_task, (unsigned long)aml_hw);
    }
#endif
#else
    if (aml_bus_type == PCIE_MODE) {
        tasklet_init(&aml_hw->task, aml_pcie_task, (unsigned long)aml_hw);
    }
#endif

    INIT_LIST_HEAD(&aml_hw->vifs);

    mutex_init(&aml_hw->dbgdump.mutex);
    spin_lock_init(&aml_hw->tx_lock);
    spin_lock_init(&aml_hw->cb_lock);
    spin_lock_init(&aml_hw->roc_lock);

    printk("%s:%d\n", __func__, __LINE__);

    if (aml_bus_type != PCIE_MODE) {
        aml_rxdata_init();
        aml_rxbuf_list_init(aml_hw);
    }

    if ((ret = aml_platform_on(aml_hw, NULL)))
        goto err_platon;

    /* Reset FW */
    printk("%s:%d\n", __func__, __LINE__);
    if ((ret = aml_send_reset(aml_hw)))
        goto err_lmac_reqs;

    if ((ret = aml_send_version_req(aml_hw, &aml_hw->version_cfm)))
        goto err_lmac_reqs;
    aml_set_vers(aml_hw);

    if ((ret = aml_handle_dynparams(aml_hw, aml_hw->wiphy)))
        goto err_lmac_reqs;

    aml_enable_mesh(aml_hw);
    aml_radar_detection_init(&aml_hw->radar);

    /* Set parameters to firmware */
    aml_send_me_config_req(aml_hw);
    /*set ext capability to fw*/
    aml_send_extcapab_req(aml_hw);

    /* Only monitor mode supported when custom channels are enabled */
    if (aml_mod_params.custchan) {
        aml_limits[0].types = BIT(NL80211_IFTYPE_MONITOR);
        aml_limits_dfs[0].types = BIT(NL80211_IFTYPE_MONITOR);
    }

    if ((ret = wiphy_register(wiphy))) {
        wiphy_err(wiphy, "Could not register wiphy device\n");
        goto err_register_wiphy;
    }
#ifndef CONFIG_PT_MODE
    aml_wq_init(aml_hw);
    aml_sync_trace_init(aml_hw);
#endif

#ifdef CONFIG_AML_DEBUGFS
    if (ret = aml_trace_buf_init()) {
        AML_INFO("alloc trace buf failed(%d)!\n", ret);
    }
#endif

#ifdef CONFIG_AML_RECOVERY
    aml_recy_init(aml_hw);
#endif

    /* Work to defer processing of rx buffer */
    INIT_WORK(&aml_hw->defer_rx.work, aml_rx_deferred);
    skb_queue_head_init(&aml_hw->defer_rx.sk_list);

    /* Update regulatory (if needed) and set channel parameters to firmware
       (must be done after wiphy registration) */
    aml_custregd(aml_hw, wiphy);
    aml_send_me_chan_config_req(aml_hw);

    *platform_data = aml_hw;

#ifndef CONFIG_PT_MODE
    if ((ret = aml_dbgfs_register(aml_hw, "aml"))) {
        wiphy_err(wiphy, "Failed to register debugfs entries");
        goto err_debugfs;
    }
#endif

    memset(&cfg, 0, sizeof(cfg));
    if ((ret = aml_cfg_parse(aml_hw, &cfg))) {
        wiphy_err(wiphy, "Failed to parse config from file\n");
    }

    if ((ret = aml_wiphy_addresses_add(wiphy, cfg))) {
        wiphy_err(wiphy, "Failed to add wiphy addresses\n");
        goto err_config;
    }

    if ((ret = aml_interface_add_all(aml_hw, aml_mod_params.custchan))) {
        wiphy_err(wiphy, "Failed to instantiate a network device\n");
        ret = -ENOMEM;
        goto err_add_interface;
    }

    /* register ipv4 addr notifier cb */
    ret = register_inetaddr_notifier(&aml_ipv4_cb);
    if (ret) {
        printk("%s failed to register ipv4 notifier(%d)!\n",__func__, ret);
    }

    /* register ipv6 addr notifier cb */
    ret = register_inet6addr_notifier(&aml_ipv6_cb);
    if (ret) {
        printk("%s failed to register ipv6 notifier(%d)!\n",__func__, ret);
    }

    ret = aml_register_panic_notifier();
    if (ret) {
        printk("%s failed to register panic notifier(%d)\n", __func__, ret);
    }

    if (aml_bus_type == PCIE_MODE) {
        g_txdesc_trigger.ths_enable = 1;
        g_txdesc_trigger.txdesc_cnt = 0;
        g_txdesc_trigger.dynamic_cnt = 6;
        g_txdesc_trigger.tx_pcie_ths = 8;
    }

    //tempsensor interrupt enable
    aml_set_temp_start(aml_hw);

    // init sdio/usb/pcie interface
    //aml_lp_shutdown_func_register
    aml_interface_shutdown_init(aml_hw);
    atomic_set(&g_wifi_pm.wifi_enable, 1);

    return 0;

err_add_interface:
err_debugfs:
    wiphy_unregister(aml_hw->wiphy);
err_register_wiphy:
err_lmac_reqs:
    aml_fw_trace_dump(aml_hw);
    aml_platform_off(aml_hw, NULL);
err_platon:
err_config:
    kmem_cache_destroy(aml_hw->sw_txhdr_cache);
err_cache:
    wiphy_free(wiphy);
err_out:
    return ret;
}

/**
 *
 */
void aml_cfg80211_deinit(struct aml_hw *aml_hw)
{
    AML_DBG(AML_FN_ENTRY_STR);

    unregister_inetaddr_notifier(&aml_ipv4_cb);
    unregister_inet6addr_notifier(&aml_ipv6_cb);
    aml_unregister_panic_notifier();
#ifdef CONFIG_AML_NAPI
    napi_disable(&aml_hw->napi);
    netif_napi_del(&aml_hw->napi);
    if (skb_queue_len(&aml_hw->napi_rx_upload_queue))
        skb_queue_purge(&aml_hw->napi_rx_upload_queue);
    if (skb_queue_len(&aml_hw->napi_rx_pending_queue))
        skb_queue_purge(&aml_hw->napi_rx_pending_queue);
#endif
#ifndef CONFIG_PT_MODE
    aml_dbgfs_unregister(aml_hw);
#endif
    aml_wdev_unregister(aml_hw);
    wiphy_unregister(aml_hw->wiphy);
    aml_radar_detection_deinit(&aml_hw->radar);
    del_timer_sync(&aml_hw->txq_cleanup);
#ifndef CONFIG_PT_MODE
#ifdef CONFIG_AML_RECOVERY
    aml_recy_deinit();
#endif
    aml_sync_trace_deinit(aml_hw);
    aml_wq_deinit(aml_hw);
#endif
    aml_platform_off(aml_hw, NULL);
    if (aml_bus_type != PCIE_MODE) {
        aml_rxdata_deinit();
#ifdef CONFIG_AML_RX_SG
        kfree(g_mmc_misc);
#endif
#ifdef CONFIG_AML_DEBUGFS
        aml_trace_buf_deinit();
#endif
    }
    aml_hwctx_buf_deinit(aml_hw);
    kmem_cache_destroy(aml_hw->sw_txhdr_cache);
    aml_wiphy_addresses_free(aml_hw->wiphy);
    wiphy_free(aml_hw->wiphy);
    g_cali_cfg_done = 0;
    g_lp_shutdown_func = NULL;
    atomic_set(&g_wifi_pm.wifi_enable, 0);
}

void aml_get_version(void)
{
    aml_print_version();
}

static int __init aml_mod_init(void)
{
    AML_DBG(AML_FN_ENTRY_STR);

    aml_print_version();
    printk("aml_bus_type = %d.\n", aml_bus_type);

    if (aml_bus_type == USB_MODE) {
         return aml_platform_register_usb_drv();
    } else if (aml_bus_type == SDIO_MODE) {
        return aml_platform_register_sdio_drv();
    } else if (aml_bus_type == PCIE_MODE) {
        return aml_platform_register_pcie_drv();
    } else {
        return -1;
    }
}

/**
 *
 */
static void __exit aml_mod_exit(void)
{
    AML_DBG(AML_FN_ENTRY_STR);

    if (aml_bus_type == USB_MODE) {
         aml_platform_unregister_usb_drv();
    } else if (aml_bus_type == SDIO_MODE) {
        aml_platform_unregister_sdio_drv();
    } else if (aml_bus_type == PCIE_MODE) {
        aml_platform_unregister_pcie_drv();
    }
}

module_init(aml_mod_init);
module_exit(aml_mod_exit);

MODULE_FIRMWARE(AML_CONFIG_FW_NAME);

MODULE_DESCRIPTION(RW_DRV_DESCRIPTION);
MODULE_VERSION(AML_VERS_MOD);
MODULE_AUTHOR(RW_DRV_COPYRIGHT " " RW_DRV_AUTHOR);
MODULE_LICENSE("GPL");
