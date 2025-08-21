/**
****************************************************************************************
*
* @file aml_recy.h
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief Declaration of the recovery mechanism.
*
****************************************************************************************
*/

#ifndef __AML_RECY__
#define __AML_RECY__

#include "aml_defs.h"
#include "lmac_mac.h"
#include "aml_log.h"

#ifdef CONFIG_AML_RECOVERY

#define AML_RECY_RECONNECT_TIMES    (5)
#define AML_RECY_MON_INTERVAL       (4 * HZ)
#define AML_WAKE_SRC_INTERVAL       (3 * HZ)

/* disconnect reason code for link loss, use internally */
#define AML_RECY_REASON_CODE_LINK_LOSS (40)

/* AML_RECY flag bits */
#define AML_RECY_ASSOC_INFO_SAVED   BIT(0)
#define AML_RECY_CHECK_SCC          BIT(1)
#define AML_RECY_AP_INFO_SAVED      BIT(2)
#define AML_RECY_STATE_ONGOING      BIT(3)
#define AML_RECY_FW_ONGOING         BIT(4)
#define AML_RECY_GO_ONGOING         BIT(5)
#define AML_RECY_STOP_AP_PROC       BIT(6)
#define AML_RECY_IPC_ONGOING        BIT(7)
#define AML_RECY_DEL_STA_PROC       BIT(8)
#define AML_RECY_OPEN_VIF_PROC      BIT(9)
#define AML_RECY_CLOSE_VIF_PROC     BIT(10)
#define AML_RECY_DROP_XMIT_PKT      BIT(11)


#define AML_AGCCNTL_ADDR            0x00C0B390

enum aml_recy_reason {
    RECY_REASON_CODE_CMD_CRASH = 1,
    RECY_REASON_CODE_FW_LINKLOSS,
    RECY_REASON_CODE_BUS_ERR,
    RECY_REASON_CODE_TX_TIMEOUT,
    //THE MAX
    RECY_REASON_CODE_MAX,
};

struct aml_recy_assoc_info {
    u8 bssid[ETH_ALEN];
    u8 prev_bssid[ETH_ALEN];
    struct ieee80211_channel *chan;
    struct cfg80211_crypto_settings crypto;
    enum nl80211_auth_type auth_type;
    enum nl80211_mfp mfp;
    u8 key_idx;
    u8 key_len;
    u8 *key_buf;
    size_t ies_len;
    u8 *ies_buf;
    u8 vif_idx;
};

struct aml_recy_ap_info {
    struct cfg80211_ap_settings *settings;
    struct mac_chan_op chan;
    enum nl80211_band band;
};

struct aml_recy_link_loss {
    bool is_enabled;
    bool is_requested;
    /* check link loss status and scan result */
    bool is_happened;
    u16 scan_result_cnt;
};

struct aml_recy_counter {
    uint32_t total_recy_cnt;
    uint32_t recy_reason_cmd_crash_cnt;
    uint32_t recy_reason_cmd_fw_linkloss_cnt;
    uint32_t recy_reason_cmd_bus_error_cnt;
    uint32_t recy_reason_cmd_tx_timeout_cnt;
};

struct aml_recy {
   /* AML_RECY_x flags */
    u32 flags;
    u8 reconnect_rest;
    u8 ps_state;
    u8 reason;
    struct aml_recy_link_loss link_loss;
    struct aml_hw *aml_hw;
    struct aml_recy_assoc_info assoc_info;
    struct aml_recy_ap_info ap_info;
    struct timer_list timer;
    struct aml_recy_counter recy_counter;
};

#define aml_recy_memcpy(dst, src, len) do { \
    if (src && len > 0) { \
        if (dst) { kfree(dst); dst = NULL; } \
        if (!dst) { dst = kmalloc(len, GFP_KERNEL); } \
        if (!dst) { AML_DBG("kmalloc failed"); return; } \
        memcpy(dst, src, len); \
    } \
} while (0);

extern struct aml_recy *aml_recy;

void aml_recy_enable(void);
void aml_recy_disable(void);
void aml_recy_flags_set(u32 flags);
void aml_recy_flags_clr(u32 flags);
bool aml_recy_flags_chk(u32 flags);
void aml_recy_save_assoc_info(struct cfg80211_connect_params *sme, u8 vif_index);
void aml_recy_save_ap_info(struct cfg80211_ap_settings *settings);
void aml_recy_save_bcn_info(u8 *bcn, size_t bcn_len);
void aml_recy_link_loss_test(void);
int aml_recy_doit(struct aml_hw *aml_hw, void *reason, int len);
int aml_recy_init(struct aml_hw *aml_hw);
void aml_recy_deinit(void);

int aml_recy_connect_retry(void);
bool aml_recy_check_aml_vif_exit(struct aml_hw *aml_hw, struct aml_vif *aml_vif);

int aml_recy_fw_reload_for_usb_sdio(struct aml_hw *aml_hw);

#endif  // CONFIG_AML_RECOVERY

void aml_wake_source_relax(struct aml_hw *aml_hw);
void aml_wake_source_set(struct aml_hw *aml_hw);
void aml_wake_source_deinit(struct aml_hw *aml_hw);
void aml_wake_source_init(struct aml_hw *aml_hw);

#endif  //__AML_RECY__
