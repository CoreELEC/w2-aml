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
#include "aml_log.h"
#include "wifi_w2_shared_mem_cfg.h"
#include "aml_static_buf.h"
#include "aml_reorder.h"

#define RXDESC_CNT_READ_ONCE 32

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

    /// Host reorder (combine two counter flags)
    RX_STAT_HOST_REO = (RX_STAT_ALLOC | RX_STAT_DELETE),
    // Defrag
    RX_STAT_DEFRAG = (RX_STAT_ALLOC | RX_STAT_LEN_UPDATE),
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

#define AML_VIF_ID_UNKNOWN      0xff
#define AML_STA_ID_UNKNOWN      0xff

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

#define DEBUG_RX_BUF_CNT       300

struct aml_vif *aml_rx_get_vif(struct aml_hw *aml_hw, int vif_idx);

u8 aml_unsup_rx_vec_ind(void *pthis, void *hostid);
int aml_pci_rxdataind(void *pthis, void *hostid);
void aml_rx_deferred(struct work_struct *ws);
void aml_rx_defer_skb(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                       struct sk_buff *skb);

void aml_rx_mgmt_any(struct aml_hw *aml_hw, struct sk_buff *skb, struct hw_rxhdr *hw_rxhdr);

void aml_scan_clear_scan_res(struct aml_hw *aml_hw);
void aml_scan_rx(struct aml_hw *aml_hw, struct hw_rxhdr *hw_rxhdr, struct sk_buff *skb);

#endif /* _AML_RX_H_ */
