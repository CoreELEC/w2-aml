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
 *         set SDIO_USB_IPC_EXT_TX_START to SDIO_USB_EXTEND_E2A_IRQ_STATUS.
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
 *         set SDIO_USB_IPC_EXT_TX_START to SDIO_USB_EXTEND_E2A_IRQ_STATUS.
 *  - host: allow TX,
 *          set HOST_RXBUF_ENLARGE_FINISH by aml_sdio_usb_rx_confirm()
 *          if FW_BUFFER_EXPAND is cleared.
 *  - f/w: clear internal flags: BUFFER_RX_NEED_ENLARGE | BUFFER_RX_ENLARGE_FLAG.
 *         done.
 */

#include <linux/skbuff.h>

#include "wifi_w2_shared_mem_cfg.h"
#include "fw/dp_rx.h"

#define AML_RX_DEBUG

enum aml_rx_buf_layout {
    AML_RX_BUF_NARROW = 0,  /* TX page is bigger */
    AML_RX_BUF_EXPAND = 1,

    AML_RX_BUF_LAYOUT_LAST,
};

struct aml_sharedmem_layout {
    u32 tx_page;
    addr32_t rx_end;
};

enum {
    AML_RX_STATE_START,     /* fetch new RX data from firmware? */
    AML_RX_STATE_RESET,     /* firmware is reset or just resumed */
    AML_RX_STATE_READING,   /* host is reading new rx data */
    AML_RX_STATE_NAPI_EN,   /* NAPI is enabled */
    AML_RX_STATE_NO_BUF,
    AML_RX_STATE_DEFERRED,
    AML_RX_STATE_IDLE,
    AML_RX_STATE_DEINIT,
};

#ifdef AML_RX_DEBUG
enum aml_rx_dur_id {
    AML_RX_DUR_FORM = 0,
    AML_RX_DUR_REO,
    AML_RX_DUR_FWD,
    AML_RX_DUR_NAPI,

    AML_RX_DUR_LAST,
};

struct aml_rx_dur {
    ktime_t show;
    ktime_t last;

    uint64_t sum;
    uint32_t cnt;
    uint32_t min;
    uint32_t max;
};
#endif

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

    /* NAPI */
    struct net_device napi_dev;
    struct napi_struct napi;
    struct sk_buff_head napi_preq;
    struct sk_buff_head napi_pending;

    /* status monitor */
    struct {
        struct work_struct work;
        struct workqueue_struct *workqueue;
    } status_mon;

#ifdef AML_RX_DEBUG
    struct {
        ktime_t show;

        ktime_t indicate;
        ktime_t fetch;
        ktime_t confirm;
    } ts;
    struct aml_rx_dur durs[AML_RX_DUR_LAST];
#endif
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

int aml_rx_task(void *data);

int aml_sdio_usb_fw_rx_head_ind(struct aml_rx *rx, addr32_t fw_rx_head);
int aml_sdio_usb_rxdataind(struct aml_rx *rx);

void aml_shared_mem_layout_update(struct aml_rx *rx);
static inline enum aml_rx_buf_layout aml_shared_mem_layout_get(struct aml_rx *rx)
{
    return (rx->fw.end == rx->layouts[AML_RX_BUF_EXPAND].rx_end)
                ? AML_RX_BUF_EXPAND : AML_RX_BUF_NARROW;
}

static inline int aml_sdio_usb_rx_start(struct aml_rx *rx)
{
    return test_and_set_bit(AML_RX_STATE_START, &rx->state) ? -1 /* already started */ : 0;
}

int aml_sdio_usb_rx_reset(struct aml_rx *rx);
int aml_sdio_usb_rx_stop(struct aml_rx *rx);
void aml_sdio_usb_rx_restart(struct aml_rx *rx);

int aml_sdio_usb_rx_init(struct aml_rx *rx);
void aml_sdio_usb_rx_deinit(struct aml_rx *rx);

struct aml_sta;
void aml_rx_sta_deinit(struct aml_rx *rx, struct aml_sta *sta);

#endif /* AML_SDIO_USB_RX_H_ */
