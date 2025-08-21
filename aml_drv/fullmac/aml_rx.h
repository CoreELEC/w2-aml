/**
 ******************************************************************************
 *
 * @file aml_rx.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */
#ifndef _AML_RX_H_
#define _AML_RX_H_

#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include "hal_desc.h"
#include "ipc_shared.h"
#include "wifi_debug.h"
#include "wifi_w2_shared_mem_cfg.h"
#include "aml_static_buf.h"

#define BUFFER_STATUS           (BIT(0) | BIT(1))
#define BUFFER_NARROW           BIT(0)
#define BUFFER_EXPAND           BIT(1)
#define BUFFER_UPDATE_FLAG      BIT(2)
#define BUFFER_REDUCE_FINSH     BIT(3)
#define BUFFER_WRAP             BIT(4)
#define BUFFER_EXPEND_FINSH     BIT(5)
#define BUFFER_TX_USED_FLAG     BIT(6)

#define RXDESC_CNT_READ_ONCE 32

#ifndef CONFIG_AML_RX_MINISIZE
#define RX_DESC_SIZE                   (128)
#define RX_HEADER_OFFSET               (48)
#define RX_HEADER_TO_PD_LEN            (80)
#define RX_PD_LEN                      (36)
#define RX_PAYLOAD_OFFSET              (RX_DESC_SIZE + RX_PD_LEN)
#define RX_HEADER_TO_PAYLOAD_LEN       (116)
#define RX_HOSTID_TO_PAYLOAD_LEN       (144)
#define RX_RPD_DATASTARTPTR            (RX_DESC_SIZE + 8)

#define AML_WRAP CO_BIT(31)
#define RX_DATA_MAX_CNT (512 + 128)

#define RX_HOSTID_OFFSET 36
#define RX_REORDER_LEN_OFFSET 40
#define RX_STATUS_OFFSET 44
#define RX_FRMLEN_OFFSET 48
#define RX_VECT1_OFFSET 60
#define RX_VECT2_OFFSET 76
#define NEXT_PKT_OFFSET 120
#else
#define RX_DESC_SIZE                   (80)
#define RX_HEADER_OFFSET               (28)
#define RX_PD_LEN                      (20)
#define RX_PAYLOAD_OFFSET              (RX_DESC_SIZE + RX_PD_LEN)
#define AML_WRAP CO_BIT(31)
#define RX_DATA_MAX_CNT (512 + 128)

#define RX_HOSTID_OFFSET               (36)
#define RX_REORDER_LEN_OFFSET          (38)
#define RX_STATUS_OFFSET               (32)
#define RX_FRMLEN_OFFSET               (28)
#define NEXT_PKT_OFFSET                (56)

/*no use in current code*/
#define RX_HEADER_TO_PD_LEN            (80)
#define RX_HEADER_TO_PAYLOAD_LEN       (72)
#define RX_HOSTID_TO_PAYLOAD_LEN       (144)
#define RX_RPD_DATASTARTPTR            (RX_DESC_SIZE + 8)
#define RX_VECT1_OFFSET 60
#define RX_VECT2_OFFSET 76
#endif
struct drv_stat_desc{
    uint32_t reserved[9];
    struct rxdesc_tag rx_stat_val;
};

struct drv_rxdesc{
    uint32_t msdu_offset;
    struct sk_buff *skb;
};

enum rx_status_bits
{
    /// The buffer can be forwarded to the networking stack
    RX_STAT_FORWARD = 1 << 0,
    /// A new buffer has to be allocated
    RX_STAT_ALLOC = 1 << 1,
    /// The buffer has to be deleted
    RX_STAT_DELETE = 1 << 2,
    /// The length of the buffer has to be updated
    RX_STAT_LEN_UPDATE = 1 << 3,
    /// The length in the Ethernet header has to be updated
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    /// Simple copy
    RX_STAT_COPY = 1 << 5,
    /// Spurious frame (inform upper layer and discard)
    RX_STAT_SPURIOUS = 1 << 6,
    /// packet for monitor interface
    RX_STAT_MONITOR = 1 << 7,
};

/* Maximum number of rx buffer the fw may use at the same time
   (must be at least IPC_RXBUF_CNT) */
#define AML_RXBUFF_MAX ((64 * NX_MAX_MSDU_PER_RX_AMSDU * NX_REMOTE_STA_MAX) < IPC_RXBUF_CNT ?     \
                         IPC_RXBUF_CNT : (64 * NX_MAX_MSDU_PER_RX_AMSDU * NX_REMOTE_STA_MAX))

/**
 * struct aml_skb_cb - Control Buffer structure for RX buffer
 *
 * @hostid: Buffer identifier. Written back by fw in RX descriptor to identify
 * the associated rx buffer
 */
struct aml_skb_cb {
    uint32_t hostid;
};

#define AML_RXBUFF_HOSTID_SET(buf, val)                                \
    ((struct aml_skb_cb *)((struct sk_buff *)buf->addr)->cb)->hostid = val

#define AML_RXBUFF_HOSTID_GET(buf)                                        \
    ((struct aml_skb_cb *)((struct sk_buff *)buf->addr)->cb)->hostid

#define AML_RXBUFF_VALID_IDX(idx) ((idx) < AML_RXBUFF_MAX)

/* Used to ensure that hostid set to fw is never 0 */
#define AML_RXBUFF_IDX_TO_HOSTID(idx) ((idx) + 1)
#define AML_RXBUFF_HOSTID_TO_IDX(hostid) ((hostid) - 1)

#define RX_MACHDR_BACKUP_LEN    64

/// MAC header backup descriptor
struct mon_machdrdesc
{
    /// Length of the buffer
    u32 buf_len;
    /// Buffer containing mac header, LLC and SNAP
    u8 buffer[RX_MACHDR_BACKUP_LEN];
};

struct hw_rxhdr {
    /** RX vector */
    struct hw_vect hwvect;

    /** PHY channel information */
    struct phy_channel_info_desc phy_info;

    /** RX flags */
    u32 flags_is_amsdu     : 1;
    u32 flags_is_80211_mpdu: 1;
    u32 flags_is_4addr     : 1;
    u32 flags_new_peer     : 1;
    u32 flags_user_prio    : 3;
    u32 flags_rsvd0        : 1;
    u32 flags_vif_idx      : 8;    // 0xFF if invalid VIF index
    u32 flags_sta_idx      : 8;    // 0xFF if invalid STA index
    u32 flags_dst_idx      : 8;    // 0xFF if unknown destination STA
#ifdef CONFIG_AML_MON_DATA
    /// MAC header backup descriptor (used only for MSDU when there is a monitor and a data interface)
    struct mon_machdrdesc mac_hdr_backup;
#endif
    /** Pattern indicating if the buffer is available for the driver */
    u32 pattern;
    u32 reserved[6];
    u32 amsdu_hostids[NX_MAX_MSDU_PER_RX_AMSDU - 1];
    u16 amsdu_len[NX_MAX_MSDU_PER_RX_AMSDU];
};

/**
 * struct aml_defer_rx - Defer rx buffer processing
 *
 * @skb: List of deferred buffers
 * @work: work to defer processing of this buffer
 */
struct aml_defer_rx {
    struct sk_buff_head sk_list;
    struct work_struct work;
};

/**
 * struct aml_defer_rx_cb - Control buffer for deferred buffers
 *
 * @vif: VIF that received the buffer
 */
struct aml_defer_rx_cb {
    struct aml_vif *vif;
};

#define GET_TID(x) ((x) >> 8 & 0xF)
#define EXCEPT_HOSTID(x) (((x) > 4096) ? ((x) % 4096):(x))

struct rxdata {
    struct list_head list;
    unsigned int host_id;
    unsigned int frame_len;
    unsigned int package_info;
    struct sk_buff *skb;
};

struct rx_desc_head {
    u32 hostid;
    u32 frame_len;
    u32 status;
    u32 package_info;
};

struct rx_reorder_info {
    u32 hostid;
    u32 reorder_len;
};

struct debug_proc_rxbuff_info {
    u32 time;
    u32 addr;
    u32 hostid;
    u16 status;
    u16 buff_idx;
    u16 idx;
};

struct debug_push_rxdesc_info {
    u32 time;
    u32 addr;
    u16 idx;
};

struct debug_push_rxbuff_info {
    u32 time;
    u32 addr;
    u32 hostid;
    u16 idx;
};

struct rxbuf_list{
    struct list_head list;
    unsigned char *rxbuf;
    unsigned int first_len;
    unsigned int second_len;
    unsigned int rx_buf_end;
    unsigned int rx_buf_len;
    unsigned int rxbuf_data_start;
    unsigned int rxbuf_data_end;
};

struct aml_dyn_snr_cfg {
    bool need_trial;
    unsigned int best_snr_cfg;
    unsigned int enable;
    unsigned int snr_mcs_ration;
    u64 rx_byte_1; //trial 1
    u64 rx_byte_2; //trial 2
    u64 rx_byte;   //all rx bytes
    //struct aml_rx_rate_stats rx_rate;
    unsigned long last_time;
};

#define DEBUG_RX_BUF_CNT       300

#define AML_WRAP CO_BIT(31)
#define HW2CPU(ptr) ((void *)(((uint32_t)(ptr)) / CHAR_LEN))
#define CO_ALIGN4_LO(val) ((val)&~3)

#define RXBUF_SIZE (324 * 1024)
#define RXBUF_NUM (WLAN_AML_HW_RX_SIZE / RXBUF_SIZE)

u8 aml_unsup_rx_vec_ind(void *pthis, void *hostid);
u8 aml_rxdataind(void *pthis, void *hostid);
void aml_rx_deferred(struct work_struct *ws);
void aml_rx_defer_skb(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                       struct sk_buff *skb);
void aml_rxdata_init(void);
void aml_rxdata_deinit(void);
void aml_scan_clear_scan_res(struct aml_hw *aml_hw);
void aml_scan_rx(struct aml_hw *aml_hw, struct hw_rxhdr *hw_rxhdr, struct sk_buff *skb);
void aml_rxbuf_list_init(struct aml_hw *aml_hw);
void aml_clear_reorder_list();

#ifndef CONFIG_AML_DEBUGFS
void aml_dealloc_global_rx_rate(struct aml_hw *aml_hw, struct aml_sta *sta);
#endif
#endif /* _AML_RX_H_ */
