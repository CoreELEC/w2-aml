/*
****************************************************************************************
*
* @file aml_sdio_usb_rx.h
*
* @brief Implementation of the aml sdio_usb rx
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#ifndef AML_SDIO_USB_RX_H_
#define AML_SDIO_USB_RX_H_

/*
 * The shared memory layout of SDIO/USB is different for:
 *  logic analyzer buffer: (by default, la_enable = 0)
 *      for PCIe, this feature is not available.
 *      for SDIO/USB, CONFIG_AML_LA/LA=n
 *  trace buffer: (by default, trace_enable/usb_trace_enable = 0)
 *      for PCIe, host memory is used.
 *      for SDIO, 32K device SRAM is used (SDIO_TRACE_START_ADDR ~ SDIO_TRACE_END_ADDR)
 *      for USB, 27K shared memory is used (USB_TRACE_START_ADDR ~ USB_TRACE_END_ADDR)
 *  for USB, its TX buffer is also controlled by
 *      CONFIG_AML_USB_LARGE_PAGE/USB_TX_USE_LARGE_PAGE=y
 *
 * The layout is majorly changed to boost TX/RX performance.
 *
 * Enlarge TX buffer (AML_RX_BUF_NARROW):
 *  - f/w: set DYNAMIC_BUF_HOST_TX_STOP to SDIO_USB_EXTEND_E2A_IRQ_STATUS by calc_tx_speed(),
 *         set internal flags: BUFFER_TX_NEED_ENLARGE.
 *  - host: set DYNAMIC_BUF_NOTIFY_FW_TX_STOP to SDIO_USB_EXTEND_E2A_IRQ_STATUS, once TX stopped.
 *  - f/w: clear SDIO_USB_EXTEND_E2A_IRQ_STATUS (set 0),
 *         enlarge TX pages by patch_dynamic_txpage_enlarge_reinit(),
 *         stop RX,
 *         set internal flags: BUFFER_RX_WAIT_READ_DATA | BUFFER_RX_REDUCE_FLAG,
 *         set FW_BUFFER_NARROW to RG_WIFI_IF_FW2HST_IRQ_CFG to notify host.
 *  - host: drain RX buffer,
 *          reset f/w RX head to RXBUF_START_ADDR if overlapped,
 *          apply new TX buffer layout,
 *          set RX_REDUCE_READ_RX_DATA_FINISH by aml_sdio_usb_rx_confirm().
 *  - f/w: call rxbuf_reduce_process() to reduce RX buffer,
 *         clear internal flags: BUFFER_RX_WAIT_READ_DATA,
 *         clear FW_BUFFER_NARROW,
 *         set DYNAMIC_BUF_HOST_TX_START to SDIO_USB_EXTEND_E2A_IRQ_STATUS.
 *  - host: allow TX,
 *          set HOST_RXBUF_REDUCE_FINISH by aml_sdio_usb_rx_confirm()
 *          if FW_BUFFER_NARROW is cleared.
 *  - f/w: clear internal flags: BUFFER_TX_NEED_ENLARGE | BUFFER_RX_REDUCE_FLAG.
 *         done.
 *
 * Enlarge RX buffer (AML_RX_BUF_EXPAND):
 *  - f/w: set DYNAMIC_BUF_HOST_TX_STOP to SDIO_USB_EXTEND_E2A_IRQ_STATUS by rxl_calc_rx_speed(),
 *         set internal flags: BUFFER_RX_NEED_ENLARGE.
 *  - host: set DYNAMIC_BUF_NOTIFY_FW_TX_STOP to SDIO_USB_EXTEND_E2A_IRQ_STATUS, once TX stopped.
 *  - f/w: clear SDIO_USB_EXTEND_E2A_IRQ_STATUS (set 0),
 *         reduce TX pages by patch_dynamic_txpage_reduce_reinit(),
 *         stop RX,
 *         set internal flags: BUFFER_RX_WAIT_READ_DATA | BUFFER_RX_ENLARGE_FLAG,
 *         set FW_BUFFER_EXPAND to RG_WIFI_IF_FW2HST_IRQ_CFG to notify host.
 *  - host: drain RX buffer,
 *          apply new TX buffer layout,
 *          set RX_ENLARGE_READ_RX_DATA_FINISH by aml_sdio_usb_rx_confirm().
 *  - f/w: call rxbuf_enlarge_process() to enlarge RX buffer,
 *         clear internal flags: BUFFER_RX_WAIT_READ_DATA,
 *         clear FW_BUFFER_EXPAND,
 *         set DYNAMIC_BUF_HOST_TX_START to SDIO_USB_EXTEND_E2A_IRQ_STATUS.
 *  - host: allow TX,
 *          set HOST_RXBUF_ENLARGE_FINISH by aml_sdio_usb_rx_confirm()
 *          if FW_BUFFER_EXPAND is cleared.
 *  - f/w: clear internal flags: BUFFER_RX_NEED_ENLARGE | BUFFER_RX_ENLARGE_FLAG.
 *         done.
 */

#include <linux/skbuff.h>

#include "wifi_w2_shared_mem_cfg.h"
#include "fw/dp_rx.h"

typedef u32 addr32_t;

enum aml_rx_buf_layout {
    AML_RX_BUF_NARROW = 0,  /* TX page is bigger */
    AML_RX_BUF_EXPAND = 1,

    AML_RX_BUF_LAYOUT_LAST,
};

struct aml_sharedmem_layout {
    u32 tx_page;
    addr32_t rx_end;
};

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
struct fw_reo_inst {    /* reorder instruction from f/w */
    u16 hostid;         /* hostid = sn + 1 */
    u16 pad;
    u8 len;
    u8 tid;
    u16 flag;
};
#endif

enum {
    AML_RX_STATE_START,     /* fetch new RX data from firmware? */
    AML_RX_STATE_RESET,     /* firmware is reset or just resumed */
    AML_RX_STATE_READING,   /* host is reading new rx data */
    AML_RX_STATE_NAPI_EN,   /* NAPI is enabled */
    AML_RX_STATE_NO_BUF,
    AML_RX_STATE_DEFERRED,
};

struct aml_rx {
    struct aml_sharedmem_layout layouts[AML_RX_BUF_LAYOUT_LAST];

    int skb_head_room;

    unsigned long state;    /* bits of AML_RX_STATE_XXX */

    u32 irq_pending;        /* if RX is pending */


    u8 *buf;                /* pre-allocated buffer */
    int buf_sz;             /* <= PREALLOC_BUF_TYPE_RXBUF_SIZE */

    /* head/tail/last are positions based on "buf" */
    int head;               /* updated by producer */
    int tail;               /* updated by consumer */
    int last;               /* the last desc updated by producer */

    int frag0;              /* 1st fragment length of the mpdu at the end */

    /* memory address in devices */
    struct {
        u32 state;          /* indicated by firmware (FW_BUFFER_NARROW | FW_BUFFER_EXPAND) */
        addr32_t head;      /* indicated by firmware (RX_WRAP_FLAG) */
        addr32_t tail;      /* confirmed by host driver */

        struct {
            u32 flags;      /* confirm firmware's state */
            addr32_t last;  /* last tail combined with flags */
        } confirm;

        addr32_t end;       /* RX buffer end of current shared memory layout */

        u16 frag0;          /* 1st fragment length of the last mpdu */
        u16 skip;           /* W2 only: skip the payload buffer descriptor */
    } fw;

    /* reorder */
    struct aml_reo_aging reo_aging;
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
    bool host_reorder;
    struct {
        struct sk_buff_head list;   /* all peer/tid mixed in one queue */
        struct fw_reo_inst instructions[IEEE80211_NUM_UPS];
    } fw_reo;
#endif

    /* NAPI */
    struct net_device napi_dev;
    struct napi_struct napi;
    struct sk_buff_head napi_preq;
    struct sk_buff_head napi_pending;
};

/* use the head room of the last skb */
struct aml_rx_amsdu {
    struct sk_buff *last;
    struct sk_buff_head msdus;  /* all msdus but the last one */
};

struct aml_skb_rxcb {
    struct aml_rx_amsdu *amsdu;

    struct aml_rhd_ext rhd_ext;
    u64 pn;             /* must follow rhd_ext */

    u8 sta_idx;
    u8 dst_idx;
    u8 tid : 3,
       vif : 3,          /* 0 ~ 3: for vif index, otherwise invalid */
       is_mpdu : 1,
       is_4addr : 1;
} __packed;

static inline struct aml_skb_rxcb *AML_SKB_RXCB(struct sk_buff *skb)
{
    BUILD_BUG_ON(sizeof(struct aml_skb_rxcb) > sizeof(skb->cb));
    return (struct aml_skb_rxcb *)skb->cb;
}

static inline struct aml_rhd_ext *AML_RHD_EXT(struct sk_buff *skb)
{
    return &AML_SKB_RXCB(skb)->rhd_ext;
}

static inline void aml_mpdu_free(struct sk_buff *skb)
{
    struct aml_skb_rxcb *rxcb = AML_SKB_RXCB(skb);

    if (rxcb->amsdu)
        __skb_queue_purge(&rxcb->amsdu->msdus);
    dev_kfree_skb(skb);
}

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
int aml_sdio_usb_fw_reo_inst_save(struct aml_rx *rx, struct fw_reo_inst *reo_inst);

static inline bool aml_sdio_usb_host_reo_enabled(struct aml_rx *rx)
{
    return rx->host_reorder;
}

static inline void aml_sdio_usb_host_reo_detected(struct aml_rx *rx)
{
    if (!rx->host_reorder) {
        rx->host_reorder = true;
        AML_M_NOTICE(MSG_RX, "=== enable host reorder ===\n");
    }
}
#else
static inline bool aml_sdio_usb_host_reo_enabled(struct aml_rx *rx) { return true; }
static inline void aml_sdio_usb_host_reo_detected(struct aml_rx *rx) { }
#endif

int aml_rx_task(void *data);

int aml_sdio_usb_fw_rx_head_ind(struct aml_rx *rx, addr32_t fw_rx_head);
int aml_sdio_usb_rxdataind(struct aml_rx *rx);

void aml_shared_mem_layout_update(struct aml_rx *rx);
static inline enum aml_rx_buf_layout aml_shared_mem_layout_get(struct aml_rx *rx)
{
    return (rx->fw.end == rx->layouts[AML_RX_BUF_EXPAND].rx_end)
                ? AML_RX_BUF_EXPAND : AML_RX_BUF_NARROW;
}

int aml_sdio_usb_rx_stop(struct aml_rx *rx);
void aml_sdio_usb_rx_restart(struct aml_rx *rx);

int aml_sdio_usb_rx_init(struct aml_rx *rx);
void aml_sdio_usb_rx_deinit(struct aml_rx *rx);

struct aml_sta;
void aml_rx_sta_deinit(struct aml_rx *rx, struct aml_sta *sta);

#endif /* AML_SDIO_USB_RX_H_ */
