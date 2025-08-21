#ifndef __WIFI_W2_SHARED_MEM_CFG_H__
#define __WIFI_W2_SHARED_MEM_CFG_H__

#ifdef CONFIG_AML_LA
#define LA
#undef TX_BUF_DYNA
#endif

/* original patch_rxdesc bigger than mini patch_rxdesc 136 byte */
/* reference code patch_fwmain.c line 400 */
#define TX_RX_MINISIZE_BUF_OFFSET        (6500 + 136)
#define TX_RX_BUF_OFFSET                 (6500) /* 6500 Byte */
#define SDIO_BUF_TAG4_OFFSET                 (7*1024) /* 0x1c00 Byte */

#define SHARED_MEM_BASE_ADDR             (0x60000000)

#define TXLBUF_TAG_SDIO_3                (0x60000928) /* size 0x1AB0 */
#define TXLBUF_TAG_SDIO_1                (0x6000f4f4) /* size 0x1800*/

#define TXPAGE_DESC_ADDR                 (0x60010f40) /* size 0x114 */
#define DMA_DESC_TEST_ADDR               (0x60011054) /* size 0x10 */
#define DMA_DESC_PATTERN_ADDR            (0x60011064) /* size 0x10 */
#define TXL_SPEC_FRAME_BUFFER_CTRL_ADDR  (0x60011074) /* size 0x40 */

#define TXLBUF_TAG_SDIO                  (0x600110b4) /* size 0x1c14 */
#define E2A_MSG_EXT_BUF                  (0x60012cc8) /* size 0x40c */
#define TXL_TBD_START                    (0x600130d4) /*size 0x2d00*/

#define TXLBUF_TAG_SDIO_2                (0x600163e0) /* size 0x1964 */
#define HW_RXBUF2_START_ADDR             (0x60019944) /* size 0x2BC */
#define HW_RXBUF1_START_ADDR             (0x60019C00)
/*
LA OFF: rx buffer large size 0x40000, small size: 0x20000;
LA ON: rx buffer large size 0x30000, small size: 0x20000
*/

#define TXLBUF_TAG_SDIO_4                (0x60017d44) /* size 0x1c00*/
#define RXBUF_START_ADDR                 (0x60019C00)


#define TXBUF_START_ADDR                 (0x60040000)
#define RXBUF_END_ADDR_SMALL             (0x60040000)
#define RXBUF_END_ADDR_LARGE             (0x60069800)
#define RXBUF_END_ADDR_LA_LARGE          (0x60059400) // 101 PAGE

#define USB_TXBUF_START_ADDR                 (0x60029130)
#define USB_RXBUF_END_ADDR_SMALL             (0x60029130)

#if defined (USB_TX_USE_LARGE_PAGE) || defined (CONFIG_AML_USB_LARGE_PAGE)
#define USB_RXBUF_END_ADDR_LARGE             (0x600684b0) // tx end 0x6007ff6c
#define USB_RXBUF_END_ADDR_LA_LARGE          (0x600575c0) // rx small + 41 tx page
#define USB_RXBUF_END_ADDR_TRACE_LARGE       (0x6005f430) // rx small + 48 tx page
#else
#define USB_RXBUF_END_ADDR_LARGE             (0x60067788) // tx end 0x6007fcc0
#endif

/* usb trace use sharemem tx last 32K */
#define USB_TRACE_START_ADDR                 (0x60078000) /* trace size: 0x8000 */
#define USB_TRACE_END_ADDR                   (0x60080000)
/* sdio trace use sram 27K size */
#define SDIO_TRACE_START_ADDR                 (0xa10400)  /* trace size: 0x6C00 */
#define SDIO_TRACE_END_ADDR                   (0xa17000)

#define SDIO_TRACE_TOTAL_SIZE    (SDIO_TRACE_END_ADDR - SDIO_TRACE_START_ADDR)
#define USB_TRACE_TOTAL_SIZE     (USB_TRACE_END_ADDR - USB_TRACE_START_ADDR)
#define SDIO_TRACE_MAX_SIZE      (SDIO_TRACE_TOTAL_SIZE >> 1) /* trace max size is total size 1/2 */
#define USB_TRACE_MAX_SIZE       (USB_TRACE_TOTAL_SIZE >> 1)  /* trace max size is total size 1/2 */

//sdio
#define RX_BUFFER_LEN_SMALL              (RXBUF_END_ADDR_SMALL - RXBUF_START_ADDR)
#define RX_BUFFER_LEN_LARGE              (RXBUF_END_ADDR_LARGE - RXBUF_START_ADDR)
#define RX_BUFFER_LEN_LA_LARGE           (RXBUF_END_ADDR_LA_LARGE - RXBUF_START_ADDR)

//usb
#define USB_RX_BUFFER_LEN_SMALL              (USB_RXBUF_END_ADDR_SMALL - RXBUF_START_ADDR)
#define USB_RX_BUFFER_LEN_LARGE              (USB_RXBUF_END_ADDR_LARGE - RXBUF_START_ADDR)
#define USB_RX_BUFFER_LEN_LA_LARGE           (USB_RXBUF_END_ADDR_LA_LARGE - RXBUF_START_ADDR)
#define USB_RX_BUFFER_LEN_TRACE_LARGE        (USB_RXBUF_END_ADDR_TRACE_LARGE - RXBUF_START_ADDR)

#define SDIO_USB_EXTEND_E2A_IRQ_STATUS CMD_DOWN_FIFO_FDN_ADDR

/* SDIO USB E2A EXTEND IRQ TYPE */
enum sdio_usb_e2a_irq_type {
    DYNAMIC_BUF_HOST_TX_STOP  = 1,
    DYNAMIC_BUF_HOST_TX_START,
    DYNAMIC_BUF_NOTIFY_FW_TX_STOP,
    DYNAMIC_BUF_LA_SWITCH_FINSH,
    DYNAMIC_BUF_TRACE_SWITCH_FINSH,
    EXCEPTION_IRQ,
};

struct sdio_buffer_control
{
    unsigned char flag;
    unsigned int tx_start_time;
    unsigned int tx_total_len;
    unsigned int rx_start_time;
    unsigned int rx_total_len;
    unsigned int tx_rate;
    unsigned int rx_rate;
    unsigned int buffer_status;
    unsigned int hwwr_switch_addr;

#ifdef USB
    bool     traffic_busy;
#endif
    bool     txdesp_rehandle;
    uint8_t  txdesp_rehandle_page_idx;
    uint8_t  free_page_cnt;
    uint8_t  buf_update_flag;
    uint8_t  rxl_has_pkt;
    uint8_t  la_enable;
    uint16_t tot_page_num;
    uint32_t occupy_buffer;
    uint32_t last_new_read;
    uint8_t  usb_trace_enable;
};
extern struct sdio_buffer_control sdio_buffer_ctrl;

#define BUFFER_TX_USED             BIT(0)
#define BUFFER_RX_USED             BIT(1)
#define BUFFER_TX_NEED_ENLARGE     BIT(2)
#define BUFFER_RX_NEED_ENLARGE     BIT(3)
#define BUFFER_RX_WAIT_READ_DATA   BIT(4)
#define BUFFER_TX_STOP_FLAG        BIT(5)
#define BUFFER_RX_REDUCE_FLAG      BIT(6)
#define BUFFER_RX_ENLARGE_FLAG     BIT(7)
#define BUFFER_RX_FORCE_REDUCE     BIT(8)
#define BUFFER_RX_FORCE_ENLARGE    BIT(9)
#define BUFFER_RX_FORBID_REDUCE    BIT(10)
#define BUFFER_RX_FORBID_ENLARGE   BIT(11)
#define BUFFER_LA_USED             BIT(12)
#define BUFFER_TRACE_USED          BIT(13)
#define BUFFER_LA_FREE             BIT(14)
#define BUFFER_TRACE_FREE          BIT(15)

#define RX_ENLARGE_READ_RX_DATA_FINSH BIT(25)
#define HOST_RXBUF_ENLARGE_FINSH      BIT(26)
#define RX_REDUCE_READ_RX_DATA_FINSH  BIT(27)
#define HOST_RXBUF_REDUCE_FINSH       BIT(28)

#define FW_BUFFER_STATUS   (BIT(20) | BIT(21))
#define FW_BUFFER_NARROW   BIT(20)
#define FW_BUFFER_EXPAND   BIT(21)
#define RX_WRAP_FLAG       BIT(31)
#define RX_WRAP_TEMP_FLAG  BIT(19)

#define RX_HAS_DATA        BIT(0)

#define CHAN_SWITCH_IND_MSG_ADDR       (0xa17fc0)
#define EXCEPTION_INFO_ADDR            (0xa17fc8)

struct exceptinon_info
{
    uint8_t  type;
    uint32_t mstatus_mps_bits;
    uint32_t mepc;
    uint32_t mtval;
    uint32_t mcause;
    uint32_t sp;
};

#define SDIO_IRQ_E2A_CHAN_SWITCH_IND_MSG           CO_BIT(15)

#define UNWRAP_SIZE (56)
#endif
