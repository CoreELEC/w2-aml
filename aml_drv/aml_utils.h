/**
 * aml_ipc_utils.h
 *
 * IPC utility function declarations
 *
 * Copyright (C) Amlogic 2012-2021
 */
#ifndef _AML_IPC_UTILS_H_
#define _AML_IPC_UTILS_H_


#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/skbuff.h>
#include "ipc_host.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <uapi/linux/sched/types.h>
#endif
#include "fi_w2_sdio.h"

#include "lmac_msg.h"
#include "wifi_debug.h"

#ifndef CONFIG_AML_DBG
/*  #define AML_DBG(format, arg...) pr_warn(format, ## arg) */
#define AML_DBG printk
#else
#define AML_DBG(a...) do {} while (0)
#endif

#define AML_FN_ENTRY_STR ">>> %s()\n", __func__

#define AML_INFO(fmt, ...) do { \
    printk("[%s %d] "fmt, __func__, __LINE__, ##__VA_ARGS__); \
} while (0);


#define MACFMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACARG(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
//rename IRQ FOR USB OR SDIO
#define IPC_A2E_MSG_IND CO_BIT(4)
#define DEBUG_MSGE2A_BUF_CNT       130
#define USB_AMSDU_BUF_LEN (4624)
#define TRAFFIC_CPU_FLAG           BIT(0)
#define TRAFFIC_SCAN_FLAG          BIT(1)
#define TRAFFIC_TXCFM_FLAG         BIT(2)


enum aml_dev_flag {
    AML_DEV_RESTARTING,
    AML_DEV_STACK_RESTARTING,
    AML_DEV_STARTED,
    AML_DEV_ADDING_STA,
};

struct aml_hw;
struct aml_sta;
struct aml_sw_txhdr;

/**
 * struct aml_ipc_buf - Generic IPC buffer
 * An IPC buffer is a buffer allocated in host memory and "DMA mapped" to be
 * accessible by the firmware.
 *
 * @addr: Host address of the buffer. If NULL other field are invalid
 * @dma_addr: DMA address of the buffer.
 * @size: Size, in bytes, of the buffer
 */
struct aml_ipc_buf
{
    void *addr;
    dma_addr_t dma_addr;
    size_t size;
};

/**
 * struct aml_ipc_buf_pool - Generic pool of IPC buffers
 *
 * @nb: Number of buffers currently allocated in the pool
 * @buffers: Array of buffers (size of array is @nb)
 * @pool: DMA pool in which buffers have been allocated
 */
struct aml_ipc_buf_pool {
    int nb;
    struct aml_ipc_buf *buffers;
    struct dma_pool *pool;
};

/**
 * struct aml_ipc_dbgdump - IPC buffer for debug dump
 *
 * @mutex: Mutex to protect access to debug dump
 * @buf: IPC buffer
 */
struct aml_ipc_dbgdump {
    struct mutex mutex;
    struct aml_ipc_buf buf;
};

struct aml_tx_list {
    struct list_head list;
    struct aml_sw_txhdr *sw_txhdr;
};

struct aml_txbuf {
    struct list_head list;
    uint32_t index;
    struct sk_buff *skb;
};

struct aml_tx_cfmed {
    struct list_head list;
    uint32_t tx_cfmed_idx;
};

#define TX_BUF_CNT    127
#define TX_LIST_CNT   64
#define TXDESC_WRITE_ONCE_CNT  32
#define USB_TXCMD_CARRY_RXRD_START_INDEX 401
#define UPDATE_FLAG 0x11223344


static const u32 aml_tx_pattern = 0xCAFEFADE;

/*
 * Maximum Length of Radiotap header vendor specific data(in bytes)
 */
#define RADIOTAP_HDR_VEND_MAX_LEN   16

/*
 * Maximum Radiotap Header Length without vendor specific data (in bytes)
 */
#define RADIOTAP_HDR_MAX_LEN        80

/*
 * Unsupported HT Frame data length (in bytes)
 */
#define UNSUP_RX_VEC_DATA_LEN       2

/// Number of radar event structures
#define RADAR_EVENT_MAX   10

#define TX_AC_BUF_SIZE 0x4dc8 //19912

#define CIRCLE_Subtract(a,b,l)  \
    (((a) >= (b)) ? ((a) - (b)):((l)+(a)-(b)))

#define CIRCLE_Subtract2(a,b,l) \
    (((a) > (b)) ? ((a) - (b)):((l)+(a)-(b)))

#define CIRCLE_Addition2(a,b,l) \
    (((a) +(b)< (l)) ? ((a) + (b)):((a)+(b)-(l)))
/**
 * IPC environment control
 */
int aml_ipc_init(struct aml_hw *aml_hw, u8 *shared_ram, u8 *shared_host_rxbuf, u8 *shared_host_rxdesc);
void aml_ipc_deinit(struct aml_hw *aml_hw);
void aml_ipc_start(struct aml_hw *aml_hw);
void aml_ipc_stop(struct aml_hw *aml_hw);
void aml_ipc_msg_push(struct aml_hw *aml_hw, void *msg_buf, uint16_t len);

/**
 * IPC buffer management
 */
int aml_ipc_buf_alloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                       size_t buf_size, enum dma_data_direction dir, const void *init);
/**
 * aml_ipc_buf_e2a_alloc() - Allocate an Embedded To Application Input IPC buffer
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer structure to store I{PC buffer information
 * @buf_size: Size of the Buffer to allocate
 * @return: 0 on success and != 0 otherwise
 */
static inline int aml_ipc_buf_e2a_alloc(struct aml_hw *aml_hw,
                                         struct aml_ipc_buf *buf,
                                         size_t buf_size)
{
    return aml_ipc_buf_alloc(aml_hw, buf, buf_size, DMA_FROM_DEVICE, NULL);
}

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
int aml_ipc_buf_prealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf, size_t buf_size,
                          int buf_type, enum dma_data_direction dir, const void *init);

/**
 * aml_ipc_buf_e2a_prealloc() - Requesting prealloc an Embedded To Application Input IPC buffer
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer structure to store IPC buffer information
 * @buf_size: Size of the Buffer to allocate
 * @buf_type: Type of the Buffer to allocate
 * @return: 0 on success and != 0 otherwise
 */
static inline int aml_ipc_buf_e2a_prealloc(struct aml_hw *aml_hw,
                                         struct aml_ipc_buf *buf,
                                         size_t buf_size, int buf_type)
{
    return aml_ipc_buf_prealloc(aml_hw, buf, buf_size, buf_type, DMA_FROM_DEVICE, NULL);
}
#endif
/**
 * aml_ipc_buf_a2e_alloc() - Allocate an Application to Embedded Output IPC buffer
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer structure to store I{PC buffer information
 * @buf_size: Size of the Buffer to allocate
 * @buf_data: Initialization data for the buffer. Must be at least
 * @buf_size long
 * @return: 0 on success and != 0 otherwise
 */
static inline int aml_ipc_buf_a2e_alloc(struct aml_hw *aml_hw,
                                         struct aml_ipc_buf *buf,
                                         size_t buf_size, const void *buf_data)
{
    return aml_ipc_buf_alloc(aml_hw, buf, buf_size, DMA_TO_DEVICE, buf_data);
}
void aml_ipc_buf_dealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf);
int aml_ipc_buf_a2e_init(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                          void *data, size_t buf_size);

void aml_ipc_buf_release(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                          enum dma_data_direction dir);

/**
 * aml_ipc_buf_e2a_release() - Release DMA mapping for an Application to Embedded IPC buffer
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to release
 *
 * An A2E buffer is realeased when it has been read by the embbeded side. This is
 * used before giving back a buffer to upper layer, or before deleting a buffer
 * when aml_ipc_buf_dealloc() cannot be used.
 */
static inline void aml_ipc_buf_a2e_release(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    aml_ipc_buf_release(aml_hw, buf, DMA_TO_DEVICE);
}

/**
 * aml_ipc_buf_e2a_release() - Release DMA mapping for an Embedded to Application IPC buffer
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to release
 *
 * An E2A buffer is released when it has been updated by the embedded and it's ready
 * to be forwarded to upper layer (i.e. out of the driver) or to be deleted and
 * aml_ipc_buf_dealloc() cannot be used.
 *
 * Note: This function has the side effect to synchronize the buffer for the host so no need to
 * call aml_ipc_buf_e2a_sync().
 */
static inline void aml_ipc_buf_e2a_release(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    aml_ipc_buf_release(aml_hw, buf, DMA_FROM_DEVICE);
}

void aml_ipc_buf_e2a_sync(struct aml_hw *aml_hw, struct aml_ipc_buf *buf, size_t len);
void aml_ipc_buf_e2a_sync_back(struct aml_hw *aml_hw, struct aml_ipc_buf *buf, size_t len);

/**
 * IPC rx buffer management
 */
int aml_ipc_rxbuf_init(struct aml_hw *aml_hw, uint32_t rx_bufsz);
int aml_ipc_rxbuf_alloc(struct aml_hw *aml_hw
#ifdef CONFIG_AML_SOFTMAC
                         , struct aml_ipc_buf *buf
#endif
                         );
void aml_ipc_rxbuf_dealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf);
void aml_ipc_rxbuf_repush(struct aml_hw *aml_hw,
                           struct aml_ipc_buf *buf);
#ifdef CONFIG_AML_FULLMAC
void aml_ipc_rxdesc_repush(struct aml_hw *aml_hw,
                            struct aml_ipc_buf *buf);
struct aml_ipc_buf *aml_ipc_rxbuf_from_hostid(struct aml_hw *aml_hw, u32 hostid);
#endif /* CONFIG_AML_FULLMAC */

int aml_ipc_unsuprxvec_alloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf);
void aml_ipc_unsuprxvec_repush(struct aml_hw *aml_hw, struct aml_ipc_buf *buf);

/**
 * IPC TX specific functions
 */
void aml_ipc_txdesc_push(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                          struct sk_buff *hostid, int hw_queue);
struct sk_buff *aml_ipc_get_skb_from_cfm(struct aml_hw *aml_hw,
                                          struct aml_ipc_buf *buf);
void aml_ipc_tx_drain(struct aml_hw *aml_hw);
bool aml_ipc_tx_pending(struct aml_hw *aml_hw);

/**
 * FW dump handler / trace
 */
void aml_error_ind(struct aml_hw *aml_hw);
void aml_umh_done(struct aml_hw *aml_hw);
void *aml_ipc_fw_trace_desc_get(struct aml_hw *aml_hw);
const char* ssid_sprintf(const unsigned char *ssid, unsigned char ssid_len);
u32 aml_ieee80211_chan_to_freq(u32 chan, u32 band);
u8 aml_ieee80211_freq_to_chan(u32 freq, u32 band);
int aml_traceind(void *pthis, int mode);

void aml_txbuf_list_init(struct aml_hw *aml_hw);
void aml_tx_cfmed_list_init(struct aml_hw *aml_hw);
struct scan_results *aml_scan_get_scan_res_node(struct aml_hw *aml_hw);
void aml_scan_results_list_init(struct aml_hw *aml_hw);
void aml_sdio_scatter_reg_init(struct aml_hw *aml_hw);
void aml_scatter_req_init(struct aml_hw *aml_hw);

void aml_amsdu_buf_list_init(struct aml_hw *aml_hw);
void aml_amsdu_buf_list_deinit(struct aml_hw *aml_hw);

struct aml_txbuf *aml_get_from_free_txbuf(struct aml_hw *aml_hw);
void aml_txbuf_list_deinit(struct aml_hw *aml_hw);
struct sk_buff *aml_get_skb_from_used_txbuf(struct aml_hw *aml_hw, u32_l hostid);
uint32_t aml_read_reg(struct net_device *dev,uint32_t reg_addr);
void aml_get_proc_msg(struct net_device *dev);
void aml_get_proc_rxbuff(struct net_device *dev);

int aml_rx_task(void *data);
int aml_tx_task(void *data);
int aml_msg_task(void *data);
int aml_tx_cfm_task(void *data);
void aml_tx_cfm_param_init(struct aml_hw *aml_hw);
#ifdef CONFIG_SDIO_TX_ENH
int aml_update_dyn_txcfm(struct aml_hw *aml_hw, int en);
#endif

#endif /* _AML_IPC_UTILS_H_ */
