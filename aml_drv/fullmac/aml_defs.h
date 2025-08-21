/**
 ******************************************************************************
 *
 * @file aml_defs.h
 *
 * @brief Main driver structure declarations for fullmac driver
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_DEFS_H_
#define _AML_DEFS_H_

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/pm_qos.h>
#include <linux/wireless.h>
#include <linux/platform_device.h>
#include <net/cfg80211.h>
#ifdef CONFIG_AML_APF
#include <linux/amlogic/pm.h>
#endif

#include "fw/dp_tx.h"

#include "aml_mod_params.h"
#include "aml_debugfs.h"
#include "aml_tx.h"
#include "aml_rx.h"
#include "aml_sdio_usb_rx.h"
#include "aml_radar.h"
#include "aml_utils.h"
#include "aml_mu_group.h"
#include "aml_platform.h"
#include "aml_cmds.h"
#include "aml_recy.h"
#include "ipc_host.h"
#include "fi_cmd.h"
#include "aml_compat.h"
#include "aml_task.h"
#include "aml_tcp_ack.h"

#define WPI_HDR_LEN    18
#define WPI_PN_LEN     16
#define WPI_PN_OFST     2
#define WPI_MIC_LEN    16
#define WPI_KEY_LEN    32
#define WPI_SUBKEY_LEN 16 // WPI key is actually two 16bytes key

#define LEGACY_PS_ID   0
#define UAPSD_ID       1

#define PS_SP_INTERRUPTED  255

/// Maximum size of a beacon frame
#define NX_BCNFRAME_LEN 512
#define BCN_TXLBUF_TAG_LEN 252

#define TX_MAX_CNT  124

#define SCANU_ADD_IE_OFFSET (16)
#define SCAN_RESULTS_MAX_CNT  (50)

// Maximum number of AP_VLAN interfaces allowed.
// At max we can have one AP_VLAN per station, but we also limit the
// maximum number of interface to 16 (to fit in avail_idx_map)
#define MAX_AP_VLAN_ITF (((16 - NX_VIRT_DEV_MAX) > NX_REMOTE_STA_MAX) ? \
    NX_REMOTE_STA_MAX : (16 - NX_VIRT_DEV_MAX))

// Maximum number of interface at driver level
#define NX_ITF_MAX (NX_VIRT_DEV_MAX + MAX_AP_VLAN_ITF)

// WIFI_CALI_VERSION must be consistent with the version field in "/vendor/firmware/"
// After updating the parameters, it must be modified at the same time.
#define WIFI_CALI_VERSION   (19)
#define WIFI_CALI_FILENAME  "aml_wifi_rf.txt"

#define WIFI_COUNTRY_PWR_LIMIT_VERSION  (1)
#define WIFI_COUNTRY_PWR_LIMIT  "aml_country_pwr_limit.txt"

#define STRUCT_BUFF_LEN   252
#define MAX_HEAD_LEN      92
#define MAX_TAIL_LEN      20
#define CO_ALIGN4_HI(val) (((val)+3)&~3)
#define TX_BUFFER_POOL_SIZE  0x4dc8

#define AMSDU_PADDING(x) ((4 - ((x) & 0x3)) & 0x3)
#define NORMAL_AMSDU_MAX_LEN    2304

#define AML_NAPI_WEIGHT 48

#define AML_STA_VIF_IDX         0
#define AML_AP_VIF_IDX          1
#define AML_P2P_VIF_IDX         1
#define AML_P2P_DEVICE_VIF_IDX  2

#define AML_CONNECTING   BIT(0)
#define AML_DISCONNECTING   BIT(1)
#define AML_GETTING_IP   BIT(2)

#define HOST_REQUEST_DISCONNECT (BIT(15))
#define MAC_RS_DEAUTH_SENDER_LEFT_IBSS_ESS        3

enum wifi_module_sn {
      MODULE_ITON = 0X1,
      MODULE_AMPAK,
      MODULE_FN_LINK,
};

#define TS_STAT0 0x00a04940
typedef union TS_STAT0_FIELD
{
    unsigned int data;
    struct
    {
        unsigned int yout_d2 : 16;
        unsigned int yvalid_d2 : 1;
        unsigned int detected_hi_temp_r : 1;
        unsigned int detect_hi_temp_cnt : 14;
    }b;
}TS_STAT0_FIELD_T;

/**
 * struct aml_bcn - Information of the beacon in used (AP mode)
 *
 * @head: head portion of beacon (before TIM IE)
 * @tail: tail portion of beacon (after TIM IE)
 * @ies: extra IEs (not used ?)
 * @head_len: length of head data
 * @tail_len: length of tail data
 * @ies_len: length of extra IEs data
 * @tim_len: length of TIM IE
 * @len: Total beacon len (head + tim + tail + extra)
 * @dtim: dtim period
 */
struct aml_bcn {
    u8 *head;
    u8 *tail;
    u8 *ies;
    size_t head_len;
    size_t tail_len;
    size_t ies_len;
    size_t tim_len;
    size_t len;
    u8 dtim;
};

/**
 * struct aml_key - Key information
 *
 * @hw_idx: Index of the key from hardware point of view
 */
struct aml_key {
    u8 hw_idx;
};

/**
 * struct aml_mesh_path - Mesh Path information
 *
 * @list: List element of aml_vif.mesh_paths
 * @path_idx: Path index
 * @tgt_mac_addr: MAC Address it the path target
 * @nhop_sta: Next Hop STA in the path
 */
struct aml_mesh_path {
    struct list_head list;
    u8 path_idx;
    struct mac_addr tgt_mac_addr;
    struct aml_sta *nhop_sta;
};

/**
 * struct aml_mesh_path - Mesh Proxy information
 *
 * @list: List element of aml_vif.mesh_proxy
 * @ext_sta_addr: Address of the External STA
 * @proxy_addr: Proxy MAC Address
 * @local: Indicate if interface is a proxy for the device
 */
struct aml_mesh_proxy {
    struct list_head list;
    struct mac_addr ext_sta_addr;
    struct mac_addr proxy_addr;
    bool local;
};

/**
 * struct aml_csa - Information for CSA (Channel Switch Announcement)
 *
 * @vif: Pointer to the vif doing the CSA
 * @bcn: Beacon to use after CSA
 * @buf: IPC buffer to send the new beacon to the fw
 * @chandef: defines the channel to use after the switch
 * @count: Current csa counter
 * @status: Status of the CSA at fw level
 * @ch_idx: Index of the new channel context
 * @work: work scheduled at the end of CSA
 */
struct aml_csa {
    struct aml_vif *vif;
    struct aml_bcn bcn;
    struct aml_ipc_buf buf;
    struct cfg80211_chan_def chandef;
    int count;
    int status;
    int ch_idx;
    struct work_struct work;
};

/**
 * enum tdls_status_tag - States of the TDLS link
 *
 * @TDLS_LINK_IDLE: TDLS link is not active (no TDLS peer connected)
 * @TDLS_SETUP_REQ_TX: TDLS Setup Request transmitted
 * @TDLS_SETUP_RSP_TX: TDLS Setup Response transmitted
 * @TDLS_LINK_ACTIVE: TDLS link is active (TDLS peer connected)
 */
enum tdls_status_tag {
        TDLS_LINK_IDLE,
        TDLS_SETUP_REQ_TX,
        TDLS_SETUP_RSP_TX,
        TDLS_LINK_ACTIVE,
        TDLS_STATE_MAX
};

/**
 * struct aml_tdls - Information relative to the TDLS peer
 *
 * @active: Whether TDLS link is active or not
 * @initiator: Whether TDLS peer is the TDLS initiator or not
 * @chsw_en: Whether channel switch is enabled or not
 * @chsw_allowed: Whether TDLS channel switch is allowed or not
 * @last_tid: TID of the latest MPDU transmitted over the TDLS link
 * @last_sn: Sequence number of the latest MPDU transmitted over the TDLS link
 * @ps_on: Whether power save mode is enabled on the TDLS peer or not
 */
struct aml_tdls {
    bool active;
    bool initiator;
    bool chsw_en;
    u8 last_tid;
    u16 last_sn;
    bool ps_on;
    bool chsw_allowed;
};

/**
 * enum aml_ap_flags - AP flags
 *
 * @AML_AP_ISOLATE: Isolate clients (i.e. Don't bridge packets transmitted by
 * one client to another one
 * @AML_AP_USER_MESH_PM: Mesh Peering Management is done by user space
 * @AML_AP_CREATE_MESH_PATH: A Mesh path is currently being created at fw level
 */
enum aml_ap_flags {
    AML_AP_ISOLATE = BIT(0),
    AML_AP_USER_MESH_PM = BIT(1),
    AML_AP_CREATE_MESH_PATH = BIT(2),
};

/**
 * enum aml_sta_flags - STATION flags
 *
 * @AML_STA_EXT_AUTH: External authentication is in progress
 */
enum aml_sta_flags {
    AML_STA_EXT_AUTH = BIT(0),
    AML_STA_FT_OVER_DS = BIT(1),
    AML_STA_FT_OVER_AIR = BIT(2),
};

/**
 * struct aml_vif - VIF information
 *
 * @list: List element for aml_hw->vifs
 * @aml_hw: Pointer to driver main data
 * @wdev: Wireless device
 * @ndev: Pointer to the associated net device
 * @net_stats: Stats structure for the net device
 * @key: Conversion table between protocol key index and MACHW key index
 * @drv_vif_index: VIF index at driver level (only use to identify active
 * vifs in aml_hw->avail_idx_map)
 * @vif_index: VIF index at fw level (used to index aml_hw->vif_table, and
 * aml_sta->vif_idx)
 * @ch_index: Channel context index (within aml_hw->chanctx_table)
 * @up: Indicate if associated netdev is up (i.e. Interface is created at fw level)
 * @use_4addr: Whether 4address mode should be use or not
 * @is_re_sending: Whether a frame is being resent on this interface
 * @roc_tdls: Indicate if the ROC has been called by a TDLS station
 * @tdls_status: Status of the TDLS link
 * @tdls_chsw_prohibited: Whether TDLS Channel Switch is prohibited or not
 * @generation: Generation ID. Increased each time a sta is added/removed
 * @vif_lock: Add lock to protect for union sta/ap content
 *
 * STA / P2P_CLIENT interfaces
 * @flags: see aml_sta_flags
 * @ap: Pointer to the peer STA entry allocated for the AP
 * @tdls_sta: Pointer to the TDLS station
 * @ft_assoc_ies: Association Request Elements (only allocated for FT connection)
 * @ft_assoc_ies_len: Size, in bytes, of the Association request elements.
 * @ft_target_ap: Target AP for a BSS transition for FT over DS
 *
 * AP/P2P GO/ MESH POINT interfaces
 * @flags: see aml_ap_flags
 * @sta_list: List of station connected to the interface
 * @bcn: Beacon data
 * @bcn_interval: beacon interval in TU
 * @bcmc_index: Index of the BroadCast/MultiCast station
 * @csa: Information about current Channel Switch Announcement (NULL if no CSA)
 * @mpath_list: List of Mesh Paths (MESH Point only)
 * @proxy_list: List of Proxies Information (MESH Point only)
 * @mesh_pm: Mesh power save mode currently set in firmware
 * @next_mesh_pm: Mesh power save mode for next peer
 *
 * AP_VLAN interfaces
 * @mater: Pointer to the master interface
 * @sta_4a: When AP_VLAN interface are used for WDS (i.e. wireless connection
 * between several APs) this is the 'gateway' sta to 'master' AP
 */
struct aml_vif {
    struct list_head list;
    struct aml_hw *aml_hw;
    struct wireless_dev wdev;
    struct net_device *ndev;
    struct net_device_stats net_stats;
    struct aml_key key[6];
    u8 drv_vif_index;
    u8 vif_index;
    u8 ch_index;
    bool up;
    bool use_4addr;
    bool is_re_sending;
    bool roc_tdls;
    bool is_sta_mode;
    u8 tdls_status;
    bool tdls_chsw_prohibited;
    int generation;
    u32 filter;
    u8 is_disconnect;
    unsigned char ipv4_addr[IPV4_ADDR_LEN];
    unsigned char subnet_mask[IPV4_ADDR_LEN];
    /* protect union sta/ap content */
    spinlock_t vif_lock;
    struct tx_cfm_wait_rsp tx_cfm_wait;
    struct iw_statistics wstats;
    union
    {
        struct
        {
            u32 flags;
            struct aml_sta *ap;
            struct aml_sta *tdls_sta;
            u8 *ft_assoc_ies;
            int ft_assoc_ies_len;
            u8 ft_target_ap[ETH_ALEN];
            u8 scan_hang;
            u32 scan_duration;
            u8 cancel_scan_cfm;
            u8 assoc_ssid[MAC_SSID_LEN];
            int assoc_ssid_len;
            u8 connect_flags;
            u16 auth_status;
        } sta;
        struct
        {
            u32 flags;
            struct list_head sta_list;
            struct aml_bcn bcn;
            int bcn_interval;
            u8 bcmc_index;
            struct aml_csa *csa;

            struct list_head mpath_list;
            struct list_head proxy_list;
            enum nl80211_mesh_power_mode mesh_pm;
            enum nl80211_mesh_power_mode next_mesh_pm;
        } ap;
        struct
        {
            struct aml_vif *master;
            struct aml_sta *sta_4a;
        } ap_vlan;
    };
};

#define AML_INVALID_VIF 0xFF
#define AML_VIF_TYPE(vif) (vif->wdev.iftype)

/**
 * Structure used to store information relative to PS mode.
 *
 * @active: True when the sta is in PS mode.
 *          If false, other values should be ignored
 * @pkt_ready: Number of packets buffered for the sta in drv's txq
 *             (1 counter for Legacy PS and 1 for U-APSD)
 * @sp_cnt: Number of packets that remain to be pushed in the service period.
 *          0 means that no service period is in progress
 *          (1 counter for Legacy PS and 1 for U-APSD)
 */
struct aml_sta_ps {
    bool active;
    u16 pkt_ready[2];
    u16 sp_cnt[2];
};

/**
 * struct aml_rx_rate_stats - Store statistics for RX rates
 *
 * @table: Table indicating how many frame has been receive which each
 * rate index. Rate index is the same as the one used by RC algo for TX
 * @size: Size of the table array
 * @cpt: number of frames received
 * @rate_cnt: number of rate for which at least one frame has been received
 */
struct aml_rx_rate_stats {
    int *table;
    int size;
    int cpt;
    int rate_cnt;
};

/**
 * struct aml_sta_stats - Structure Used to store statistics specific to a STA
 *
 * @rx_pkts: Number of MSDUs and MMPDUs received from this STA
 * @tx_pkts: Number of MSDUs and MMPDUs sent to this STA
 * @rx_bytes: Total received bytes (MPDU length) from this STA
 * @tx_bytes: Total transmitted bytes (MPDU length) to this STA
 * @last_act: Timestamp (jiffies) when the station was last active (i.e. sent a
 frame: data, mgmt or ctrl )
 * @last_rx: Hardware vector of the last received frame
 * @rx_rate: Statistics of the received rates
 */
struct aml_sta_stats {
    u32 rx_pkts;
    u32 tx_pkts;
    u32 tx_fails;
    u64 rx_bytes;
    u64 tx_bytes;
    unsigned long last_act;
    struct hw_vect last_rx;
    struct aml_rx_rate_stats rx_rate;
    u32_l bcn_interval;
    u8_l bw_max;
    u32_l dtim;
    u8_l format_mod;
    u8_l mcs_max;
    u8_l no_ss;
    u8_l short_gi;
    u32_l leg_rate;
};

/**
 * struct aml_sta - Managed STATION information
 *
 * @list: List element of aml_vif->ap.sta_list
 * @valid: Flag indicating if the entry is valid
 * @mac_addr:  MAC address of the station
 * @aid: association ID
 * @sta_idx: Firmware identifier of the station
 * @vif_idx: Firmware of the VIF the station belongs to
 * @vlan_idx: Identifier of the VLAN VIF the station belongs to (= vif_idx if
 * no vlan in used)
 * @band: Band (only used by TDLS, can be replaced by channel context)
 * @width: Channel width
 * @center_freq: Center frequency
 * @center_freq1: Center frequency 1
 * @center_freq2: Center frequency 2
 * @ch_idx: Identifier of the channel context linked to the station
 * @qos: Flag indicating if the station supports QoS
 * @acm: Bitfield indicating which queues have AC mandatory
 * @uapsd_tids: Bitfield indicating which tids are subject to UAPSD
 * @key: Information on the pairwise key install for this station
 * @ps: Information when STA is in PS (AP only)
 * @bfm_report: Beamforming report to be used for VHT TX Beamforming
 * @group_info: MU grouping information for the STA
 * @ht: Flag indicating if the station supports HT
 * @vht: Flag indicating if the station supports VHT
 * @ac_param[AC_MAX]: EDCA parameters
 * @tdls: TDLS station information
 * @stats: Station statistics
 * @mesh_pm: link-specific mesh power save mode
 * @listen_interval: listen interval (only for AP client)
 */
struct aml_sta {
    struct list_head list;
    bool valid;
    u8 mac_addr[ETH_ALEN];
    u16 aid;
    u8 sta_idx;
    u8 vif_idx;
    u8 vlan_idx;
    enum nl80211_band band;
    enum nl80211_chan_width width;
    u16 center_freq;
    u32 center_freq1;
    u32 center_freq2;
    u8 ch_idx;
    bool qos;
    u8 acm;
    u16 uapsd_tids;
    struct aml_key key;
    struct aml_sta_ps ps;
#ifdef CONFIG_AML_BFMER
    struct aml_bfmer_report *bfm_report;
#ifdef CONFIG_AML_MUMIMO_TX
    struct aml_sta_group_info group_info;
#endif /* CONFIG_AML_MUMIMO_TX */
#endif /* CONFIG_AML_BFMER */
    bool ht;
    bool vht;
    bool he;
    u32 ac_param[AC_MAX];
    struct aml_tdls tdls;
    struct aml_sta_stats stats;
    enum nl80211_mesh_power_mode mesh_pm;
    int listen_interval;
    struct twt_setup_ind twt_ind; /*TWT Setup indication*/
    struct aml_reo_session *reos[IEEE80211_NUM_UPS];
    struct sk_buff *frags[IEEE80211_NUM_UPS + 1];    /* "+ 1" for non-QoS */
};

/**
 * aml_sta_addr - Return MAC address of a STA
 *
 * @sta: Station whose address is returned
 */
static inline const u8 *aml_sta_addr(struct aml_sta *sta) {
    return sta->mac_addr;
}

#ifdef CONFIG_AML_SPLIT_TX_BUF
/**
 * struct aml_amsdu_stats - A-MSDU statistics
 *
 * @done: Number of A-MSDU push the firmware
 * @failed: Number of A-MSDU that failed to transit
 */
struct aml_amsdu_stats {
    int done;
    int failed;
};
#endif

#define AMPDUS_RX_MAP_NUM           4

/**
 * struct aml_stats - Global statistics
 *
 * @cfm_balance: Number of buffer currently pushed to firmware per HW queue
 * @ampdus_tx: Number of A-MPDU transmitted (indexed by A-MPDU length)
 * @ampdus_rx: Number of A-MPDU received (indexed by A-MPDU length)
 * @ampdus_rx_map: Internal variable to distinguish A-MPDU
 * @ampdus_rx_miss: Number of MPDU non missing while receiving a-MPDU
 * @ampdu_rx_last: Index (of ampdus_rx_map) of the last A-MPDU received.
 * @amsdus: statistics of a-MSDU transmitted
 * @amsdus_rx: Number of A-MSDU received (indexed by A-MSDU length)
 */
struct aml_stats {
    int cfm_balance[NX_TXQ_CNT];
    int ampdus_tx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx_map[AMPDUS_RX_MAP_NUM];
    int ampdus_rx_miss;
    int ampdus_rx_last;
#ifdef CONFIG_AML_SPLIT_TX_BUF
    struct aml_amsdu_stats amsdus[NX_TX_PAYLOAD_MAX];
#endif
    int amsdus_rx[64];

    /* SDIO/USB only */
#define AML_RX_TRANS_RANK_NUM           10
#define AML_RX_TRANS_RANK_SIZE_0        (128U << 10)    /* 128K */
    u32 rx_trans[AML_RX_TRANS_RANK_NUM];
    u32 rx_trans_total;
};

// maximum number of TX frame per RoC
#define NX_ROC_TX 5
/**
 * struct aml_roc - Remain On Channel information
 *
 * @vif: VIF for which RoC is requested
 * @chan: Channel to remain on
 * @duration: Duration in ms
 * @internal: Whether RoC has been started internally by the driver (e.g. to send
 * mgmt frame) or requested by user space
 * @on_chan: Indicate if we have switch on the RoC channel
 * @tx_cnt: Number of MGMT frame sent using this RoC
 * @tx_cookie: Cookie as returned by aml_cfg80211_mgmt_tx if Roc has been started
 * to send mgmt frame (valid only if internal is true)
 */
struct aml_roc {
    struct aml_vif *vif;
    struct ieee80211_channel *chan;
    unsigned int duration;
    bool internal;
    bool on_chan;
    int tx_cnt;
    u64 start_time;
    u64 tx_cookie[NX_ROC_TX];
};

/**
 * struct aml_survey_info - Channel Survey Information
 *
 * @filled: filled bitfield as per struct survey_info
 * @chan_time_ms: Amount of time in ms the radio spent on the channel
 * @chan_time_busy_ms: Amount of time in ms the primary channel was sensed busy
 * @noise_dbm: Noise in dbm
 */
struct aml_survey_info {
    u32 filled;
    u32 chan_time_ms;
    u32 chan_time_busy_ms;
    s8 noise_dbm;
};

/**
 * aml_chanctx - Channel context information
 *
 * @chan_def: Channel description
 * @count: number of vif using this channel context
 */
struct aml_chanctx {
    struct cfg80211_chan_def chan_def;
    u8 count;
};

#define AML_CH_NOT_SET 0xFF

/**
 * aml_phy_info - Phy information
 *
 * @cnt: Number of phy interface
 * @cfg: Configuration send to firmware
 * @sec_chan: Channel configuration of the second phy interface (if phy_cnt > 1)
 * @limit_bw: Set to true to limit BW on requested channel. Only set to use
 * VHT with old radio that don't support 80MHz (deprecated)
 */
struct aml_phy_info {
    u8 cnt;
    struct phy_cfg_tag cfg;
    struct mac_chan_op sec_chan;
    bool limit_bw;
};

enum wifi_suspend_state {
    WIFI_SUSPEND_STATE_NONE,
    WIFI_SUSPEND_STATE_WOW,
    WIFI_SUSPEND_STATE_DEEPSLEEP,
};

enum suspend_ind_state {
    SUSPEND_IND_NONE,
    SUSPEND_IND_RECV,
    SUSPEND_IND_DONE,
};

#define WOW_MAX_PATTERNS 4

/* wake up filter in wow mode */
#define WOW_FILTER_OPTION_ANY BIT(1)
#define WOW_FILTER_OPTION_MAGIC_PACKET BIT(2)
#define WOW_FILTER_OPTION_EAP_REQ BIT(3)
#define WOW_FILTER_OPTION_4WAYHS BIT(4)
#define WOW_FILTER_OPTION_DISCONNECT BIT(5)
#define WOW_FILTER_OPTION_GTK_ERROR BIT(6)
#define WOW_FILTER_OPTION_GOOGLE_CAST_EN BIT(7)
struct tx_task_param {
    u32 tx_page_free_num;
    u32 tx_page_tot_num;
    u32 tot_page_num;
    u32 mpdu_num;
    u8  tx_page_once;
    u8  txcfm_trigger_tx_thr;
    struct amlw_hif_scatter_req * scat_req;
};

#define AMSDU_BUF_MAX  9600
#define AMSDU_BUF_CNT  40
struct tx_amsdu_param {
    struct list_head list;
    u8 amsdu_buf[AMSDU_BUF_MAX];
};

struct assoc_info {
    u8 addr[ETH_ALEN];
    u16 htcap;
};

#ifdef CONFIG_AML_APF

/**
 * struct apf_param - Structure for Android Packet Filter (APF) parameters
 *   @apf_set:      Indicates if an APF program has been set (true = active).
 *   @apf_info:     Current APF status info, such as enable flag and filter age.
 *   @apf_cap:      APF engine capabilities (e.g., supported version and features).
 *   @program_len:  Length in bytes of the loaded APF program.
 */
struct apf_param {
    bool apf_set;
    struct apf_get_status_req apf_info;
    struct apf_capabilities apf_cap;
    u32 program_len;
};

#endif

/**
 * struct aml_hw - AML driver main data
 *
 * @dev: Device structure
 *
 * @plat: Underlying AML platform information
 * @phy: PHY Configuration
 * @version_cfm: MACSW/HW versions (as obtained via MM_VERSION_REQ)
 * @version_hashid: stroe the hashid of fw version
 * @machw_type: Type of MACHW (see AML_MACHW_xxx)
 *
 * @mod_params: Driver parameters
 * @flags: Global flags, see enum aml_dev_flag
 * @task: Tasklet to execute firmware IRQ bottom half
 * @wiphy: Wiphy structure
 * @ext_capa: extended capabilities supported
 *
 * @vifs: List of VIFs currently created
 * @vif_table: Table of all possible VIFs, indexed with fw id.
 * (NX_REMOTE_STA_MAX extra elements for AP_VLAN interface)
 * @vif_started: Number of vif created at firmware level
 * @avail_idx_map: Bitfield of created interface (indexed by aml_vif.drv_vif_index)
 * @monitor_vif:  FW id of the monitor interface (AML_INVALID_VIF if no monitor vif)
 *
 * @sta_table: Table of all possible Stations
 * (NX_VIRT_DEV_MAX] extra elements for BroadCast/MultiCast stations)
 *
 * @chanctx_table: Table of all possible Channel contexts
 * @cur_chanctx: Currently active Channel context
 * @survey: Table of channel surveys
 * @roc: Information of current Remain on Channel request (NULL if Roc requested)
 * @scan_request: Information of current scan request
 * @radar: Radar detection information
 * @scan_duration: backup scan_duration from cfg
 *
 * @tx_lock: Spinlock to protect TX path
 * @txq: Table of STA/TID TX queues
 * @hwq: Table of MACHW queues
 * @txq_cleanup: Timer list to drop packet queued too long in TXQs
 * @sw_txhdr_cache: Cache to allocate
 * @tcp_pacing_shift: TCP buffering configuration (buffering is ~ 2^(10-tps) ms)
 * @mu: Information for Multiuser TX groups
 *
 * @defer_rx: Work to defer RX processing out of IRQ tasklet. Only use for specific mgmt frames
 *
 * @ipc_env: IPC environment
 * @cmd_mgr: FW command manager
 * @cb_lock: Spinlock to protect code from FW confirmation/indication message
 *
 * @msgbuf_pool: Pool of shared buffers to retrieve FW messages
 * @dbgbuf_pool: Pool of shared buffers to retrieve FW debug messages
 * @radar_pool: Pool of shared buffers to retrieve FW radar events
 * @tx_pattern: Shared buffer for the FW to download end of TX buf pattern from
 * @dbgdump: Shared buffer to retrieve FW debug dump
 * @rxdesc_pool: Pool of shared buffers to retrieve RX descriptors
 * @rxbufs: Table of shared buffers to retrieve RX data
 * @rxbuf_idx: Index of the last allocated buffer in rxbufs table
 * @unsuprxvecs: Table of shared buffers to retrieve FW unsupported frames
 * @scan_ie: Shared buffer, allocated to push probe request elements to the FW
 *
 * @debugfs: Debug FS entries
 * @stats: global statistics
 */

struct aml_hw {
    struct device *dev;

    // Hardware info
    struct aml_plat *plat;
    struct aml_phy_info phy;
    struct mm_version_cfm version_cfm;
    u32 version_hashid;
    int machw_type;

    // Global wifi config
    struct aml_mod_params *mod_params;
    unsigned long flags;
    struct wiphy *wiphy;
    u8 ext_capa[10];

    // VIFs
    struct list_head vifs;
    struct aml_vif *vif_table[NX_ITF_MAX];

    int vif_started;
    u16 avail_idx_map;
    u8 monitor_vif;
    enum wifi_suspend_state state;
    u8 suspend_ind;
    u8 google_cast;

    // Stations
    struct aml_sta *sta_table;

    // Channels
    struct aml_chanctx chanctx_table[NX_CHAN_CTXT_CNT];
    u8 cur_chanctx;
    struct aml_survey_info survey[SCAN_CHANNEL_MAX];
    struct aml_roc *roc;
    spinlock_t roc_lock;
    spinlock_t scan_req_lock;
    spinlock_t tx_wait_cfm_lock;
    struct cfg80211_scan_request *scan_request;
    struct cfg80211_sched_scan_request *sched_request;
    struct aml_radar radar;
    int show_switch_info;
    int scan_duration;

    // TX path
    spinlock_t tx_lock;
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    struct aml_txq *txq;
#else
    struct aml_txq txq[NX_NB_TXQ];
#endif
    struct aml_hwq hwq[NX_TXQ_CNT];
    struct timer_list txq_cleanup;
    struct kmem_cache *sw_txhdr_cache;
    u32 tcp_pacing_shift;
#ifdef CONFIG_AML_MUMIMO_TX
    struct aml_mu_info mu;
#endif

    // RX path
    struct aml_rx rx;               /* SDIO/USB only */

    struct aml_defer_rx defer_rx;
    uint32_t dynabuf_stop_tx;     /* dynamic buf switch, tx stop flag */
    uint32_t send_tx_stop_to_fw;  /* dynamic buf switch, send tx stop to fw flag */
    uint8_t *host_buf;            /* host buf for test */
    struct assoc_info rx_assoc_info;

#ifdef CONFIG_AML_PREALLOC_BUF_SKB
    spinlock_t prealloc_rxbuf_lock;
    u16 prealloc_rxbuf_count;
    struct list_head prealloc_rxbuf_free;
    struct list_head prealloc_rxbuf_used;

    struct semaphore prealloc_rxbuf_sem;
    struct task_struct *prealloc_rxbuf_task;
    struct completion prealloc_completion;
    int prealloc_task_quit;
#endif

#ifdef CONFIG_AML_USE_TASK
    // Tasks
    struct {
        spinlock_t rxdesc_lock;         /* FIXME: useless lock? */
        struct aml_task task_rxdesc;
        struct aml_task task_irqhdlr;
    } pcie;
#else
    // IRQ
    struct tasklet_struct task;
#endif

    struct timer_list sync_trace_timer;

    // Workqueue
    spinlock_t wq_lock;
    struct workqueue_struct *wq;
    struct work_struct work;
    struct list_head work_list;

    // IPC
    struct ipc_host_env_tag *ipc_env;
    spinlock_t ipc_lock;
    struct aml_cmd_mgr cmd_mgr;
    spinlock_t cb_lock;

    // Shared buffers
    struct aml_ipc_buf_pool msgbuf_pool;
    struct aml_ipc_buf_pool dbgbuf_pool;
    struct aml_ipc_buf_pool radar_pool;
    struct aml_ipc_buf tx_pattern;
    struct aml_ipc_dbgdump dbgdump;
    struct aml_ipc_buf_pool rxdesc_pool;
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    struct aml_ipc_buf *rxbufs;
#else
    struct aml_ipc_buf rxbufs[AML_RXBUFF_MAX];
#endif
    int rxbuf_idx;
    uint16_t hostid_prefix;
    struct aml_ipc_buf unsuprxvecs[IPC_UNSUPRXVECBUF_CNT];
    struct aml_ipc_buf scan_ie;
    struct aml_ipc_buf_pool txcfm_pool;
    struct compact_tx_cfm_tag read_cfm[COMPACT_TXCFM_CNT];

    struct scan_results *scan_results;
    uint8_t *scanres_payload_buf;
    uint32_t scanres_payload_buf_offset;
    struct list_head scan_res_list;
    struct list_head scan_res_available_list;
    spinlock_t scan_lock;

    u32 irq;
    u32 irq_done;
    struct ipc_e2a_msg g_msg1;
    struct ipc_e2a_msg g_msg2;
    struct ipc_dbg_msg g_dbg_msg;
    struct radar_pulse_array_desc g_pulses;
    int radar_pulse_index;

    struct tx_task_param g_tx_param;

    struct list_head tx_desc_save;
    struct list_head tx_buf_free_list;
    struct list_head tx_buf_used_list;
    struct list_head tx_amsdu_buf_free_list;
    struct list_head tx_amsdu_buf_used_list;
    struct list_head tx_cfmed_list;
    spinlock_t tx_buf_lock;
    spinlock_t tx_desc_lock;

    /* FIXME: use the definition and API of "struct aml_task" */
    struct task_struct *aml_irq_task;
    struct semaphore aml_irq_sem;
    struct completion aml_irq_completion;
    char aml_irq_completion_init;
    int aml_irq_task_quit;

    struct task_struct *aml_rx_task;
    struct semaphore aml_rx_sem;
    struct completion aml_rx_completion;
    char aml_rx_completion_init;
    int aml_rx_task_quit;

    struct task_struct *aml_tx_task;
    struct semaphore aml_tx_sem;
    struct completion aml_tx_completion;
    char aml_tx_completion_init;
    int aml_tx_task_quit;

    struct task_struct *aml_msg_task;
    struct semaphore aml_msg_sem;
    struct completion aml_msg_completion;
    char aml_msg_completion_init;
    int aml_msg_task_quit;

    struct task_struct *aml_txcfm_task;
    struct semaphore aml_txcfm_sem;
    struct completion aml_txcfm_completion;
    char aml_txcfm_completion_init;
    int aml_txcfm_task_quit;

    /* FIXME: move the following struct into usb_common.h/c */
    struct {
        struct urb urb;
        struct usb_ctrlrequest req;
        u32 fw_ptrs[4];
    } *usb;

    uint16_t trb_wait_time;
    u8 la_enable;
    u8 trace_enable;
    u8 trace_malloc_success;
    // Debug FS and stats
    struct aml_debugfs debugfs;
    struct aml_stats *stats;
    void *dyn_snr;      /* struct aml_dyn_snr* */

#ifdef TEST_MODE
    // for pcie dma pressure test
    struct aml_ipc_buf pcie_prssr_test;
    void * pcie_prssr_ul_addr;
#endif
    int tsq;
    u8 repush_rxdesc;
    u8 repush_rxbuff_cnt;
    u8 traffic_busy;
    u8 scan_abort_flag;
    /*management tcp session*/
    struct aml_tcp_sess_mgr ack_mgr;
#ifdef CONFIG_AML_NAPI
    /*napi struct for handling rx skb upload*/
    struct napi_struct napi;
    /*extract skb from the queue to send to network stack*/
    struct sk_buff_head napi_rx_upload_queue;
    /*enqueue skb from dma mem to pending queue,then appended to napi_rx_upload_queue*/
    struct sk_buff_head napi_rx_pending_queue;
    u8 napi_enable;
    u8 gro_enable;
    /*if the skb cnt of pending queue >= napi_pend_pkt_num,append to napi_rx_upload_queue*/
    u8 napi_pend_pkt_num;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    struct freq_qos_request qos_reqs[8];    /* up to 8 CPUs */
#endif
    bool wfd_present;
    u8 trace_mode;
    uint64_t pno_scan_reqid;
    /* prevent from suspend */
    struct wakeup_source *wifi_wakeup_source;
    struct timer_list wifi_wakeup_source_timer;
#ifdef CONFIG_AML_APF
    struct early_suspend wifi_early_suspend;
    struct apf_param apf_params;
#endif
};

#include "aml_hif.h"

static inline struct aml_hw *aml_rx2hw(struct aml_rx *rx)
{
    return container_of(rx, struct aml_hw, rx);
}

u8 *aml_build_bcn(struct aml_bcn *bcn, struct cfg80211_beacon_data *new);

void aml_chanctx_link(struct aml_vif *vif, u8 idx,
                        struct cfg80211_chan_def *chandef);
void aml_chanctx_unlink(struct aml_vif *vif);

int aml_chanctx_band(struct aml_hw *aml_hw, u8 ch_idx);

static inline int aml_chanctx_valid(struct aml_hw *aml_hw, u8 ch_idx)
{
    return aml_chanctx_band(aml_hw, ch_idx) >= 0;
}

bool aml_work_on_5g_band(struct aml_hw *aml_hw);

static inline bool is_multicast_sta(int sta_idx)
{
    return (sta_idx >= NX_REMOTE_STA_MAX);
}

struct aml_sta *aml_get_sta(struct aml_hw *aml_hw, const u8 *mac_addr);

static inline struct aml_sta *aml_sta_get(struct aml_hw *aml_hw, u8 sta_id)
{
    struct aml_sta *sta = &aml_hw->sta_table[sta_id];

    if (WARN_ON(sta_id >= (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)))
        return NULL;
    if (!sta->valid)    /* if recovering or disconnected */
        return NULL;

    return sta;
}

static inline struct aml_vif *aml_vif_get(struct aml_hw *aml_hw, u8 vif_index)
{
    struct aml_vif *aml_vif;

    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (aml_vif->vif_index == vif_index)
            return aml_vif;
    }
    return NULL;
}

static inline uint8_t master_vif_idx(struct aml_vif *vif)
{
    if (unlikely(vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN)) {
        return vif->ap_vlan.master->vif_index;
    } else {
        return vif->vif_index;
    }
}

static inline void *aml_get_shared_trace_buf(struct aml_hw *aml_hw)
{
#ifdef CONFIG_AML_DEBUGFS
    return (void *)&(aml_hw->debugfs.fw_trace.buf);
#else
    return NULL;
#endif
}
extern unsigned int aml_bus_type;

static inline void aml_spin_lock(spinlock_t* lock)
{
#ifdef CONFIG_AML_USE_TASK
    spin_lock_bh(lock);
#else
    (aml_bus_type == PCIE_MODE) ? spin_lock(lock) : spin_lock_bh(lock);
#endif
}

static inline void aml_spin_unlock(spinlock_t* lock)
{
#ifdef CONFIG_AML_USE_TASK
    spin_unlock_bh(lock);
#else
    (aml_bus_type == PCIE_MODE) ? spin_unlock(lock) : spin_unlock_bh(lock);
#endif
}
void aml_connect_flags_set(struct aml_vif *aml_vif, u32 flags);
void aml_connect_flags_clr(struct aml_vif *aml_vif, u32 flags);
bool aml_connect_flags_chk(struct aml_vif *aml_vif, u32 flags);

void aml_external_auth_enable(struct aml_vif *vif);
void aml_external_auth_disable(struct aml_vif *vif);
void aml_scan_abort(struct aml_hw *aml_hw);
int aml_cfg80211_add_key(struct wiphy *wiphy, struct net_device *netdev,
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
        int link_id, u8 key_index, bool pairwise, const u8 *mac_addr,
#else
        u8 key_index, bool pairwise, const u8 *mac_addr,
#endif
        struct key_params *params);

int aml_sta_init(struct aml_hw *aml_hw, struct aml_sta *aml_sta, u8 txq_status);
void aml_sta_deinit(struct aml_hw *aml_hw, struct aml_sta *aml_sta);

#endif /* _AML_DEFS_H_ */
