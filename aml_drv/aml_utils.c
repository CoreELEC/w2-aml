/**
 * aml_utils.c
 *
 * IPC utility function definitions
 *
 * Copyright (C) Amlogic 2012-2021
 */
#include <linux/vmalloc.h>
#include "aml_utils.h"
#include "aml_defs.h"
#include "aml_rx.h"
#include "aml_tx.h"
#include "aml_msg_rx.h"
#include "aml_debugfs.h"
#include "aml_prof.h"
#include "ipc_host.h"
#include "share_mem_map.h"
#include "aml_prealloc.h"
#include "reg_ipc_app.h"
#include "wifi_top_addr.h"
#include "sg_common.h"
#include "aml_rps.h"
#include "aml_mdns_offload.h"
#ifdef CONFIG_AML_SOFTMAC
#define FW_STR  "lmac"
#elif defined CONFIG_AML_FULLMAC
#define FW_STR  "fmac"
#endif

#define DGB_INFO_OFFSET (16) //for sdio and usb, sizeof(struct dma_desc)

#ifdef CONFIG_AML_DEBUGFS
extern struct log_file_info trace_log_file_info;
#endif
extern struct aml_pm_type w2_g_wifi_pm;

/**
 * aml_ipc_buf_pool_alloc() - Allocate and push to fw a pool of IPC buffer.
 *
 * @aml_hw: Main driver structure
 * @pool: Pool to allocate
 * @nb: Size of the pool to allocate
 * @buf_size: Size of one pool element
 * @pool_name: Name of the pool
 * @push: Function to push one pool buffer to fw
 *
 * This function will allocate an array to store the list of IPC buffers,
 * a dma pool and @nb element in the dma pool.
 * Each buffer is initialized with '0' and then pushed to fw using the @push function.
 *
 * Return: 0 on success and <0 upon error. If error is returned any allocated
 * memory is NOT freed and aml_ipc_buf_pool_dealloc() must be called.
 */
static int aml_ipc_buf_pool_alloc(struct aml_hw *aml_hw,
                                   struct aml_ipc_buf_pool *pool,
                                   int nb, size_t buf_size, char *pool_name,
                                   int (*push)(struct ipc_host_env_tag *,
                                               struct aml_ipc_buf *))
{
    struct aml_ipc_buf *buf;
    int i;

    pool->nb = 0;
    nb++;
    /* allocate buf array */
    pool->buffers = kzalloc(nb * sizeof(struct aml_ipc_buf), GFP_KERNEL);
    if (!pool->buffers) {
        dev_err(aml_hw->dev, "Allocation of buffer array for %s failed\n",
                pool_name);
        return -ENOMEM;
    }

    /* allocate dma pool */
    pool->pool = dma_pool_create(pool_name, aml_hw->dev, buf_size,
                                 cache_line_size(), 0);
    if (!pool->pool) {
        dev_err(aml_hw->dev, "Allocation of dma pool %s failed\n",
                pool_name);
        return -ENOMEM;
    }

    for (i = 0, buf = pool->buffers; i < nb; buf++, i++) {
        /* allocate a buffer */
        buf->size = buf_size;
        buf->addr = dma_pool_alloc(pool->pool, GFP_KERNEL, &buf->dma_addr);
        if (!buf->addr) {
            dev_err(aml_hw->dev, "Allocation of block %d/%d in %s failed\n",
                    (i + 1), nb, pool_name);
            return -ENOMEM;
        }
        if (!buf->dma_addr) {
            printk("err:dma_addr null i=%d\n",i);
            continue;
        }
        pool->nb++;

        /* reset the buffer */
        memset(buf->addr, 0, buf_size);

        /* push it to FW */
        push(aml_hw->ipc_env, buf);

        if (pool->nb == (nb - 1))
            break;
    }
    return 0;
}

/**
 * aml_ipc_buf_pool_dealloc() - Free all memory allocated for a pool
 *
 * @pool: Pool to free
 *
 * Must be call once after aml_ipc_buf_pool_alloc(), even if it returned
 * an error
 */
static void aml_ipc_buf_pool_dealloc(struct aml_ipc_buf_pool *pool)
{
    struct aml_ipc_buf *buf;
    int i;

    for (i = 0, buf = pool->buffers; i < (pool->nb + 1) ; buf++, i++) {
        if (buf->addr) {
            dma_pool_free(pool->pool, buf->addr, buf->dma_addr);
        }
    }
    pool->nb = 0;

    if (pool->pool)
        dma_pool_destroy(pool->pool);
    pool->pool = NULL;

    if (pool->buffers)
        kfree(pool->buffers);
    pool->buffers = NULL;
}

/**
 * aml_ipc_buf_alloc - Alloc a single ipc buffer and MAP it for DMA access
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to allocate
 * @buf_size: Size of the buffer to allocate
 * @dir: DMA direction
 * @init: Pointer to initial data to write in buffer before DMA sync. Used
 * only if direction is DMA_TO_DEVICE and it must be at least @buf_size long
 *
 * It allocates a buffer, initializes it if @init is set, and map it for DMA
 * Use @aml_ipc_buf_dealloc when this buffer is no longer needed.
 *
 * @return: 0 on success and <0 upon error. If error is returned any allocated
 * memory has been freed.
 */
int aml_ipc_buf_alloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                       size_t buf_size, enum dma_data_direction dir, const void *init)
{
    buf->addr = kmalloc(buf_size, GFP_KERNEL);
    if (!buf->addr)
        return -ENOMEM;

    buf->size = buf_size;

    if ((dir == DMA_TO_DEVICE) && init) {
        memcpy(buf->addr, init, buf_size);
    }

    if (w2_aml_bus_type == PCIE_MODE) {
        buf->dma_addr = dma_map_single(aml_hw->dev, buf->addr, buf_size, dir);
        if (dma_mapping_error(aml_hw->dev, buf->dma_addr)) {
            kfree(buf->addr);
            buf->addr = NULL;
            return -EIO;
        }
    }

    return 0;
}

/**
 * aml_ipc_buf_prealloc - Requesting prealloc a single ipc buffer and MAP it for DMA access
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to allocate
 * @buf_size: Size of the buffer to allocate
 * @buf_type: Type of the buffer to allocate
 * @dir: DMA direction
 * @init: Pointer to initial data to write in buffer before DMA sync. Used
 * only if direction is DMA_TO_DEVICE and it must be at least @buf_size long
 *
 * It allocates a buffer, initializes it if @init is set, and map it for DMA
 * Use @aml_ipc_buf_dealloc when this buffer is no longer needed.
 *
 * @return: 0 on success and <0 upon error. If error is returned any allocated
 * memory has been freed.
 */
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
int aml_ipc_buf_prealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                          size_t buf_size, int buf_type,
                          enum dma_data_direction dir, const void *init)
{
    size_t out_size;
    buf->addr = aml_prealloc_get(buf_type, buf_size, &out_size);
    if (!buf->addr)
        return -ENOMEM;

    buf->size = out_size;
    if ((dir == DMA_TO_DEVICE) && init) {
        memcpy(buf->addr, init, buf->size);
    }

    if (w2_aml_bus_type == PCIE_MODE) {
        buf->dma_addr = dma_map_single(aml_hw->dev, buf->addr, buf->size, dir);
        if (dma_mapping_error(aml_hw->dev, buf->dma_addr)) {
            buf->addr = NULL;
            return -EIO;
        }
    }

    return 0;
}
#endif

/**
 * aml_ipc_buf_dealloc() - Free memory allocated for a single ipc buffer
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to free
 *
 * IPC buffer must have been allocated by @aml_ipc_buf_alloc() or initialized
 * by @aml_ipc_buf_init() and pointing to a buffer allocated by kmalloc.
 */
void aml_ipc_buf_dealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    if (!buf->addr)
        return;
    if (w2_aml_bus_type == PCIE_MODE) {
        dma_unmap_single(aml_hw->dev, buf->dma_addr, buf->size, DMA_TO_DEVICE);
        kfree(buf->addr);
        buf->addr = NULL;
    }
}

/**
 * aml_ipc_buf_a2e_init - Initialize an Application to Embedded IPC buffer
 * with a pre-allocated buffer
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to initialize
 * @data: Data buffer to use for the IPC buffer.
 * @buf_size: Size of the buffer the @data buffer
 *
 * Initialize the IPC buffer with the provided buffer and map it for DMA transfer.
 * The mapping direction is always DMA_TO_DEVICE as this an "a2e" buffer.
 * Use @aml_ipc_buf_dealloc() when this buffer is no longer needed.
 *
 * @return: 0 on success and <0 upon error. If error is returned the @data buffer
 * is freed.
 */
int aml_ipc_buf_a2e_init(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                          void *data, size_t buf_size)
{
    buf->addr = data;
    buf->size = buf_size;
    if (w2_aml_bus_type == PCIE_MODE) {
        buf->dma_addr = dma_map_single(aml_hw->dev, buf->addr, buf_size,
                                       DMA_TO_DEVICE);
        if (dma_mapping_error(aml_hw->dev, buf->dma_addr)) {
            buf->addr = NULL;
            return -EIO;
        }
    }

    return 0;
}

/**
 * aml_ipc_buf_release() - Release DMA mapping for an IPC buffer
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to release
 * @dir: DMA direction.
 *
 * This also "release" the IPC buffer structure (i.e. its addr field is reset)
 * so that it cannot be re-used except to map another buffer.
 */
void aml_ipc_buf_release(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                          enum dma_data_direction dir)
{
    if (!buf->addr)
        return;
   if (w2_aml_bus_type == PCIE_MODE) {
        dma_unmap_single(aml_hw->dev, buf->dma_addr, buf->size, dir);
        buf->addr = NULL;
    }
}

/**
 * aml_ipc_buf_e2a_sync() - Synchronize all (or part) of an IPC buffer before
 * reading content written by the embedded
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to sync
 * @len: Length to read, 0 means the whole buffer
 */
void aml_ipc_buf_e2a_sync(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                           size_t len)
{
    if (!len)
        len = buf->size;

    dma_sync_single_for_cpu(aml_hw->dev, buf->dma_addr, len, DMA_FROM_DEVICE);
}

/**
 * aml_ipc_buf_e2a_sync_back() - Synchronize back all (or part) of an IPC buffer
 * to allow embedded updating its content.
 *
 * @aml_hw: Main driver structure
 * @buf: IPC buffer to sync
 * @len: Length to sync back, 0 means the whole buffer
 *
 * Must be called after each call to aml_ipc_buf_e2a_sync() even if host didn't
 * modified the content of the buffer.
 */
void aml_ipc_buf_e2a_sync_back(struct aml_hw *aml_hw, struct aml_ipc_buf *buf,
                            size_t len)
{
    if (!len)
        len = buf->size;

    dma_sync_single_for_device(aml_hw->dev, buf->dma_addr, len, DMA_FROM_DEVICE);
}

/**
 * aml_ipc_rxskb_alloc() - Allocate a skb for RX path
 *
 * @aml_hw: Main driver data
 * @buf: aml_ipc_buf structure to store skb address
 * @skb_size: Size of the buffer to allocate
 *
 * Allocate a skb for RX path, meaning that the data buffer is written by the firmware
 * and needs then to be DMA mapped.
 *
 * Note that even though the result is stored in a struct aml_ipc_buf, in this case the
 * aml_ipc_buf.addr points to skb structure whereas the aml_ipc_buf.dma_addr is the
 * DMA address of the skb data buffer (i.e. skb->data)
 */
static int aml_ipc_rxskb_alloc(struct aml_hw *aml_hw,
                                struct aml_ipc_buf *buf, size_t skb_size)
{
    struct sk_buff *skb = dev_alloc_skb(skb_size);
#ifdef CONFIG_AML_PREALLOC_BUF_SKB
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;

    if (unlikely(!skb)) {
        prealloc_rxbuf = aml_prealloc_get_free_rxbuf(aml_hw);
        if (!prealloc_rxbuf) {
            AML_INFO("prealloc: get free rxbuf failed");
            return -ENOMEM;
        }
        if (!prealloc_rxbuf->skb) {
            AML_INFO("prealloc: get free skb failed");
            return -ENOMEM;
        }

        skb = prealloc_rxbuf->skb;
        spin_lock_bh(&aml_hw->prealloc_rxbuf_lock);
        aml_hw->prealloc_rxbuf_count++;
        if (aml_hw->prealloc_rxbuf_count >= PREALLOC_RXBUF_FACTOR) {
            AML_INFO("prealloc: signaling to request new skb buffer");
            up(&aml_hw->prealloc_rxbuf_sem);
        }
        spin_unlock_bh(&aml_hw->prealloc_rxbuf_lock);
    }
#else
    if (unlikely(!skb)) {
        dev_err(aml_hw->dev, "Allocation of RX skb failed\n");
        buf->addr = NULL;
        return -ENOMEM;
    }
#endif

    buf->dma_addr = dma_map_single(aml_hw->dev, skb->data, skb_size,
                                       DMA_FROM_DEVICE);
    if (unlikely(dma_mapping_error(aml_hw->dev, buf->dma_addr))) {
        dev_err(aml_hw->dev, "DMA mapping of RX skb failed\n");
        dev_kfree_skb(skb);
        buf->addr = NULL;
        return -EIO;
    }

    buf->addr = skb;
    buf->size = skb_size;

    return 0;
}

/**
 * aml_ipc_rxskb_reset_pattern() - Reset pattern in a RX skb or unsupported
 * RX vector buffer
 *
 * @aml_hw: Main driver data
 * @buf: RX skb to reset
 * @pattern_offset: Pattern location, in byte from the start of the buffer
 *
 * Reset the pattern in a RX/unsupported RX vector skb buffer to inform embedded
 * that it has been processed by the host.
 * Pattern in a 32bit value.
 */
static void aml_ipc_rxskb_reset_pattern(struct aml_hw *aml_hw,
                                         struct aml_ipc_buf *buf,
                                         size_t pattern_offset)
{
    struct sk_buff *skb = buf->addr;
    u32 *pattern = (u32 *)(skb->data + pattern_offset);

    *pattern = 0;
    dma_sync_single_for_device(aml_hw->dev, buf->dma_addr + pattern_offset,
                               sizeof(u32), DMA_FROM_DEVICE);
}

/**
 * aml_ipc_rxskb_dealloc() - Free a skb allocated for the RX path
 *
 * @aml_hw: Main driver data
 * @buf: Rx skb to free
 *
 * Free a RX skb allocated by @aml_ipc_rxskb_alloc
 */
static void aml_ipc_rxskb_dealloc(struct aml_hw *aml_hw,
                                   struct aml_ipc_buf *buf)
{
    if (!buf->addr)
        return;

    dma_unmap_single(aml_hw->dev, buf->dma_addr, buf->size, DMA_TO_DEVICE);
    dev_kfree_skb((struct sk_buff *)buf->addr);
    buf->addr = NULL;
}


/**
 * aml_ipc_unsup_rx_vec_elem_allocs() - Allocate and push an unsupported
 *                                       RX vector buffer for the FW
 *
 * @aml_hw: Main driver data
 * @elem: Pointer to the skb elem that will contain the address of the buffer
 */
int aml_ipc_unsuprxvec_alloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    int err;

    err = aml_ipc_rxskb_alloc(aml_hw, buf, aml_hw->ipc_env->unsuprxvec_sz);
    if (err)
        return err;

    aml_ipc_rxskb_reset_pattern(aml_hw, buf,
                                 offsetof(struct rx_vector_desc, pattern));
    ipc_host_unsuprxvec_push(aml_hw->ipc_env, buf);
    return 0;
}

/**
 * aml_ipc_unsuprxvec_repush() - Reset and repush an already allocated buffer
 * for unsupported RX vector
 *
 * @aml_hw: Main driver data
 * @buf: Buf to repush
 */
void aml_ipc_unsuprxvec_repush(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    aml_ipc_rxskb_reset_pattern(aml_hw, buf,
                                 offsetof(struct rx_vector_desc, pattern));
    ipc_host_unsuprxvec_push(aml_hw->ipc_env, buf);
}

/**
* aml_ipc_unsuprxvecs_alloc() - Allocate and push all unsupported RX
* vector buffers for the FW
*
* @aml_hw: Main driver data
*/
static int aml_ipc_unsuprxvecs_alloc(struct aml_hw *aml_hw)
{
   struct aml_ipc_buf *buf;
   int i;

   memset(aml_hw->unsuprxvecs, 0, sizeof(aml_hw->unsuprxvecs));

   for (i = 0, buf = aml_hw->unsuprxvecs; i < ARRAY_SIZE(aml_hw->unsuprxvecs); i++, buf++)
   {
       if (aml_ipc_unsuprxvec_alloc(aml_hw, buf)) {
           dev_err(aml_hw->dev, "Failed to allocate unsuprxvec buf %d\n", i + 1);
           return -ENOMEM;
       }
   }

   return 0;
}

/**
 * aml_ipc_unsuprxvecs_dealloc() - Free all unsupported RX vector buffers
 * allocated for the FW
 *
 * @aml_hw: Main driver data
 */
static void aml_ipc_unsuprxvecs_dealloc(struct aml_hw *aml_hw)
{
    struct aml_ipc_buf *buf;
    int i;

    for (i = 0, buf = aml_hw->unsuprxvecs; i < ARRAY_SIZE(aml_hw->unsuprxvecs); i++, buf++)
    {
        aml_ipc_rxskb_dealloc(aml_hw, buf);
    }
}

/**
 * aml_ipc_rxbuf_alloc() - Allocate and push a rx buffer for the FW
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer where to store address of the skb. In fullmac this
 * parameter is not available so look for the first free IPC buffer
 */
int aml_ipc_rxbuf_alloc(struct aml_hw *aml_hw
#ifdef CONFIG_AML_SOFTMAC
                         ,struct aml_ipc_buf *buf
#endif
                         )
{
    int err;

#ifdef CONFIG_AML_FULLMAC
    struct aml_ipc_buf *buf;
    int nb = 0, idx = aml_hw->rxbuf_idx;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    while ((aml_hw->rxbufs + idx)->addr && (nb < AML_RXBUFF_MAX)) {
#else
    while (aml_hw->rxbufs[idx].addr && (nb < AML_RXBUFF_MAX)) {
#endif
        idx = ( idx + 1 ) % AML_RXBUFF_MAX;
        nb++;
    }
    if (nb == AML_RXBUFF_MAX) {
        dev_err(aml_hw->dev, "No more free space for rxbuff");
        return -ENOMEM;
    }

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    buf = aml_hw->rxbufs + idx;
#else
    buf = &aml_hw->rxbufs[idx];
#endif
    /* Save idx so that on next push the free slot will be found quicker */
    aml_hw->rxbuf_idx = ( idx + 1 ) % AML_RXBUFF_MAX;
#endif

    err = aml_ipc_rxskb_alloc(aml_hw, buf, aml_hw->ipc_env->rxbuf_sz);
    if (err)
        return err;

    aml_ipc_rxskb_reset_pattern(aml_hw, buf, offsetof(struct hw_rxhdr, pattern));

#ifdef CONFIG_AML_FULLMAC
    AML_RXBUFF_HOSTID_SET(buf, AML_RXBUFF_IDX_TO_HOSTID(idx));
#endif
    ipc_host_rxbuf_push(aml_hw->ipc_env, buf);

    return 0;
}

/**
 * aml_ipc_rxbuf_dealloc() - Free a RX buffer for the FW
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer associated to the RX buffer to free
 */
void aml_ipc_rxbuf_dealloc(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    aml_ipc_rxskb_dealloc(aml_hw, buf);
}

/**
 * aml_ipc_rxbuf_repush() - Reset and repush an already allocated RX buffer
 *
 * @aml_hw: Main driver data
 * @buf: Buf to repush
 *
 * In case a skb is not forwarded to upper layer it can be re-used.
 */
void aml_ipc_rxbuf_repush(struct aml_hw *aml_hw, struct aml_ipc_buf *buf)
{
    aml_ipc_rxskb_reset_pattern(aml_hw, buf, offsetof(struct hw_rxhdr, pattern));
    ipc_host_rxbuf_push(aml_hw->ipc_env, buf);
}

/**
 * aml_ipc_rxbufs_alloc() - Allocate and push all RX buffer for the FW
 *
 * @aml_hw: Main driver data
 */
static int aml_ipc_rxbufs_alloc(struct aml_hw *aml_hw)
{
    int i, nb = aml_hw->ipc_env->rxbuf_nb;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    memset(aml_hw->rxbufs, 0, sizeof(struct aml_ipc_buf) * AML_RXBUFF_MAX);
#else
    memset(aml_hw->rxbufs, 0, sizeof(aml_hw->rxbufs));
#endif
    for (i = 0; i < nb; i++) {
        if (aml_ipc_rxbuf_alloc(aml_hw
#ifdef CONFIG_AML_SOFTMAC
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
                                 ,aml_hw->rxbufs + i
#else
                                 ,&aml_hw->rxbufs[i]
#endif
#endif
                                 )) {
            dev_err(aml_hw->dev, "Failed to allocate rx buf %d/%d\n",
                    i + 1, nb);
            return -ENOMEM;
        }
    }

    return 0;
}

/**
 * aml_ipc_rxbufs_dealloc() - Free all RX buffer allocated for the FW
 *
 * @aml_hw: Main driver data
 */
static void aml_ipc_rxbufs_dealloc(struct aml_hw *aml_hw)
{
    struct aml_ipc_buf *buf;
    int i;

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    for (i = 0, buf = aml_hw->rxbufs; i < AML_RXBUFF_MAX; i++, buf++) {
#else
    for (i = 0, buf = aml_hw->rxbufs; i < ARRAY_SIZE(aml_hw->rxbufs); i++, buf++) {
#endif
        aml_ipc_rxskb_dealloc(aml_hw, buf);
    }
}

#ifdef CONFIG_AML_FULLMAC
/**
 * aml_ipc_rxdesc_repush() - Repush a RX descriptor to FW
 *
 * @aml_hw: Main driver data
 * @buf: RX descriptor to repush
 *
 * Once RX buffer has been received, the RX descriptor used by FW to upload this
 * buffer can be re-used for another RX buffer.
 */
void aml_ipc_rxdesc_repush(struct aml_hw *aml_hw,
                            struct aml_ipc_buf *buf)
{
    struct rxdesc_tag *rxdesc = buf->addr;
    rxdesc->status = 0;
    dma_sync_single_for_device(aml_hw->dev, buf->dma_addr,
                               sizeof(struct rxdesc_tag), DMA_BIDIRECTIONAL);
    ipc_host_rxdesc_push(aml_hw->ipc_env, buf);
}

/**
 * aml_ipc_rxbuf_from_hostid() - Return IPC buffer of a RX buffer from a hostid
 *
 * @aml_hw: Main driver data
 * @hostid: Hostid of the RX buffer
 * @return: Pointer to the RX buffer with the provided hostid and NULL if the
 * hostid is invalid or no buffer is associated.
 */

#ifdef DEBUG_CODE
extern int aml_set_reg(struct net_device *dev, int addr, int val);
uint32_t addr_null_happen = 0;
//print all rxbufs msg
void aml_buf_addr_null_debug_fn(struct aml_hw *aml_hw)
{
    uint32_t last_push_index = aml_hw->rxbuf_idx;
    uint32_t i;

    for (i = 0; i < AML_RXBUFF_MAX; i++)
    {
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
        struct aml_ipc_buf *buf = aml_hw->rxbufs + last_push_index;
#else
        struct aml_ipc_buf *buf = &(aml_hw->rxbufs[last_push_index]);
#endif
        printk("host_id:%d, buf:%08x, addr:%08x, dma_addr:%08x\n",
                last_push_index, buf, buf->addr, buf->dma_addr);
        last_push_index = (last_push_index + 1 ) % AML_RXBUFF_MAX;
    }
}
#endif
struct aml_ipc_buf *aml_ipc_rxbuf_from_hostid(struct aml_hw *aml_hw, u32 hostid)
{
    int rxbuf_idx = AML_RXBUFF_HOSTID_TO_IDX(hostid);

    if (AML_RXBUFF_VALID_IDX(rxbuf_idx)) {
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
        struct aml_ipc_buf *buf = aml_hw->rxbufs + rxbuf_idx;
#else
        struct aml_ipc_buf *buf = &aml_hw->rxbufs[rxbuf_idx];
#endif
        if (buf->addr && (AML_RXBUFF_HOSTID_GET(buf) == hostid))
            return buf;

        dev_err(aml_hw->dev, "Invalid Rx buff: hostid=%d addr=%p hostid_in_buff=%d\n",
                hostid, buf->addr, (buf->addr) ? AML_RXBUFF_HOSTID_GET(buf): -1);
#if 0
        if (buf->addr == NULL)
        {
            addr_null_happen = 1;
            aml_buf_addr_null_debug_fn(aml_hw);
        }
#endif
        if (buf->addr)
            aml_ipc_rxbuf_dealloc(aml_hw, buf);
    }

    dev_err(aml_hw->dev, "RX Buff invalid hostid [%d]\n", hostid);
    return NULL;
}
#endif /* CONFIG_AML_FULLMAC */

/**
 * aml_elems_deallocs() - Deallocate IPC storage elements.
 * @aml_hw: Main driver data
 *
 * This function deallocates all the elements required for communications with
 * LMAC, such as Rx Data elements, MSGs elements, ...
 * This function should be called in correspondence with the allocation function.
 */
static void aml_elems_deallocs(struct aml_hw *aml_hw)
{
    if (w2_aml_bus_type == PCIE_MODE) {
#ifdef CONFIG_AML_PREALLOC_BUF_SKB
        aml_prealloc_rxbuf_deinit(aml_hw);
#endif
        aml_ipc_rxbufs_dealloc(aml_hw);
        aml_ipc_unsuprxvecs_dealloc(aml_hw);
#ifdef CONFIG_AML_FULLMAC
        aml_ipc_buf_pool_dealloc(&aml_hw->rxdesc_pool);
#endif
        aml_ipc_buf_pool_dealloc(&aml_hw->msgbuf_pool);
        aml_ipc_buf_pool_dealloc(&aml_hw->dbgbuf_pool);
        aml_ipc_buf_pool_dealloc(&aml_hw->radar_pool);
        aml_ipc_buf_pool_dealloc(&aml_hw->txcfm_pool);
        aml_ipc_buf_dealloc(aml_hw, &aml_hw->tx_pattern);
    }

#ifndef CONFIG_AML_PREALLOC_BUF_STATIC
    aml_ipc_buf_dealloc(aml_hw, &aml_hw->dbgdump.buf);
#endif
}

/**
 * aml_elems_allocs() - Allocate IPC storage elements.
 * @aml_hw: Main driver data
 *
 * This function allocates all the elements required for communications with
 * LMAC, such as Rx Data elements, MSGs elements, ...
 * This function should be called in correspondence with the deallocation function.
 */
static int aml_elems_allocs(struct aml_hw *aml_hw)
{
    AML_DBG(AML_FN_ENTRY_STR);

    if (w2_aml_bus_type == PCIE_MODE) {
        if (dma_set_coherent_mask(aml_hw->dev, DMA_BIT_MASK(32)))
            goto err_alloc;
#ifdef CONFIG_AML_SOFTMAC
        if (aml_ipc_rxbuf_init(aml_hw,
                                (4 * (aml_hw->mod_params->amsdu_rx_max + 1) + 1) * 1024))
#else
        if (aml_ipc_rxbuf_init(aml_hw, (sizeof(struct hw_rxhdr) + NORMAL_AMSDU_MAX_LEN)))
#endif
            goto err_alloc;

        if (aml_ipc_buf_pool_alloc(aml_hw, &aml_hw->msgbuf_pool,
                                    IPC_MSGE2A_BUF_CNT,
                                    sizeof(struct ipc_e2a_msg),
                                    "aml_ipc_msgbuf_pool",
                                    ipc_host_msgbuf_push))
            goto err_alloc;

        if (aml_ipc_buf_pool_alloc(aml_hw, &aml_hw->dbgbuf_pool,
                                    IPC_DBGBUF_CNT,
                                    sizeof(struct ipc_dbg_msg),
                                    "aml_ipc_dbgbuf_pool",
                                    ipc_host_dbgbuf_push))
            goto err_alloc;

        if (aml_ipc_buf_pool_alloc(aml_hw, &aml_hw->radar_pool,
                                    IPC_RADARBUF_CNT,
                                    sizeof(struct radar_pulse_array_desc),
                                    "aml_ipc_radar_pool",
                                    ipc_host_radar_push))
            goto err_alloc;

        if (aml_ipc_buf_pool_alloc(aml_hw, &aml_hw->txcfm_pool,
                                    IPC_TXCFM_CNT,
                                    sizeof(struct tx_cfm_tag),
                                    "aml_ipc_txcfm_pool",
                                    ipc_host_txcfm_push))
            goto err_alloc;

        if (aml_ipc_unsuprxvecs_alloc(aml_hw))
            goto err_alloc;

        if (aml_ipc_buf_a2e_alloc(aml_hw, &aml_hw->tx_pattern, sizeof(u32),
                                   &aml_tx_pattern))
            goto err_alloc;
        ipc_host_pattern_push(aml_hw->ipc_env, &aml_hw->tx_pattern);

    /*
     * Note that the RX buffers are no longer allocated here as their size depends on the
     * FW configuration, which is not available at that time.
     * They will be allocated when checking the parameter compatibility between the driver
     * and the underlying components (i.e. during the aml_handle_dynparams() execution)
     */
#ifdef CONFIG_AML_FULLMAC
        if (aml_ipc_buf_pool_alloc(aml_hw, &aml_hw->rxdesc_pool,
                                     aml_hw->ipc_env->rxdesc_nb,
                                     sizeof(struct rxdesc_tag),
                                     "aml_ipc_rxdesc_pool",
                                     ipc_host_rxdesc_push))
            goto err_alloc;

#endif /* CONFIG_AML_FULLMAC */
    }

#ifndef CONFIG_PT_MODE
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
        if (aml_ipc_buf_e2a_prealloc(aml_hw, &aml_hw->dbgdump.buf,
                                   sizeof(struct dbg_debug_dump_tag),
                                   PREALLOC_BUF_TYPE_DUMP))
#else
        if (aml_ipc_buf_e2a_alloc(aml_hw, &aml_hw->dbgdump.buf,
                                   sizeof(struct dbg_debug_dump_tag)))
#endif
            goto err_alloc;
#endif

    if (w2_aml_bus_type == PCIE_MODE) {
        ipc_host_dbginfo_push(aml_hw->ipc_env, &aml_hw->dbgdump.buf);
    }

    return 0;

err_alloc:
    dev_err(aml_hw->dev, "Error while allocating IPC buffers\n");
    aml_elems_deallocs(aml_hw);
    return -ENOMEM;
}

void aml_sdio_scatter_reg_init(struct aml_hw *aml_hw)
{
    unsigned char data;
    unsigned int reg_tmp;

    data = aml_hw->plat->hif_sdio_ops->hi_self_define_domain_read8(RG_SCFG_FUNC1_AUTO_TX);
    data |= BIT(4); //enable auto tx
    aml_hw->plat->hif_sdio_ops->hi_self_define_domain_write8(RG_SCFG_FUNC1_AUTO_TX, data);


    reg_tmp = 0;
    aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)&reg_tmp, (unsigned char *)RG_WIFI_IF_MAC_TXTABLE_RD_ID, 4);
    aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)&reg_tmp, (unsigned char *)RG_SDIO_IF_MISC_CTRL, 4);
    /*frame flag bypass for function4
    BIT(8)=1, w/o setting address.
    BIT(8)!=1,should setting address.
    BIT(9)=1, disable sdio updating page table ptr.
    BIT(10)=1, enable sdio update page read ptr(0xa070a8 low 8bit).
    BIT(16)=1, for func6 wrapper around by rtl
    */
    reg_tmp |= BIT(8);
    reg_tmp |= BIT(10);
    reg_tmp |= BIT(16);
    aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)&reg_tmp, (unsigned char *)RG_SDIO_IF_MISC_CTRL, 4);

    reg_tmp = BIT(0);
    aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)&reg_tmp, (unsigned char *)RG_SDIO_IF_INTR2CPU_ENABLE, 4);
}


void aml_amsdu_buf_list_init(struct aml_hw *aml_hw)
{
    int i = 0;
    struct tx_amsdu_param *txamsdu;
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    size_t out_size;
    struct tx_amsdu_param *txamsdu_base;
#endif

    INIT_LIST_HEAD(&aml_hw->tx_amsdu_buf_free_list);
    INIT_LIST_HEAD(&aml_hw->tx_amsdu_buf_used_list);
#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    txamsdu_base =(struct tx_amsdu_param *)aml_prealloc_get(PREALLOC_BUF_TYPE_AMSDU,
             AMSDU_BUF_CNT * sizeof(struct tx_amsdu_param), &out_size);
    if (!txamsdu_base) {
        ASSERT_ERR(0);
        return;
    }
    for (i = 0; i < AMSDU_BUF_CNT; i++) {
        txamsdu = txamsdu_base + i;
        list_add_tail(&txamsdu->list, &aml_hw->tx_amsdu_buf_free_list);
    }
#else
    for (i = 0; i < AMSDU_BUF_CNT; i++) {
        txamsdu = kmalloc(sizeof(struct tx_amsdu_param), GFP_ATOMIC);
        if (!txamsdu) {
            ASSERT_ERR(0);
            return;
        }
        list_add_tail(&txamsdu->list, &aml_hw->tx_amsdu_buf_free_list);
    }

#endif
}
void aml_amsdu_buf_list_deinit(struct aml_hw *aml_hw)
{
    struct tx_amsdu_param *txamsdu = NULL;
    struct tx_amsdu_param *txamsdu_tmp = NULL;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    list_for_each_entry_safe(txamsdu, txamsdu_tmp, &aml_hw->tx_amsdu_buf_free_list, list) {
        kfree(txamsdu);
    }
    list_for_each_entry_safe(txamsdu, txamsdu_tmp, &aml_hw->tx_amsdu_buf_used_list, list) {
        kfree(txamsdu);
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);
}

struct tx_amsdu_param *aml_get_free_tx_amsdu_buf(struct aml_hw *aml_hw)
{
    struct tx_amsdu_param *txamsdu = NULL;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    if (!list_empty(&aml_hw->tx_amsdu_buf_free_list)) {
        txamsdu = list_first_entry(&aml_hw->tx_amsdu_buf_free_list, struct tx_amsdu_param, list);
        list_del(&txamsdu->list);
        list_add_tail(&txamsdu->list, &aml_hw->tx_amsdu_buf_used_list);

    } else {
        ASSERT_ERR(0);
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);

    return txamsdu;
}

void aml_set_free_tx_amsdu_buf(struct aml_hw *aml_hw)
{
    struct tx_amsdu_param *txamsdu = NULL;
    struct tx_amsdu_param *txamsdu_tmp = NULL;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    list_for_each_entry_safe(txamsdu, txamsdu_tmp, &aml_hw->tx_amsdu_buf_used_list, list) {
        list_del(&txamsdu->list);
        list_add_tail(&txamsdu->list, &aml_hw->tx_amsdu_buf_free_list);
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);
}


unsigned int tx_desc_index;

void aml_txbuf_list_init(struct aml_hw *aml_hw)
{
    struct aml_txbuf *txbuf = NULL;
    int i = 0;

    spin_lock_init(&aml_hw->tx_buf_lock);
    spin_lock_init(&aml_hw->tx_desc_lock);
    spin_lock_init(&aml_hw->rx_lock);
    spin_lock_init(&aml_hw->reoder_lock);
    spin_lock_init(&aml_hw->buf_start_lock);

    spin_lock_init(&aml_hw->free_list_lock);
    spin_lock_init(&aml_hw->used_list_lock);

    INIT_LIST_HEAD(&aml_hw->tx_buf_free_list);
    INIT_LIST_HEAD(&aml_hw->tx_buf_used_list);
    INIT_LIST_HEAD(&aml_hw->tx_desc_save);

    for (i = 1; i < TX_BUF_CNT + 1; i++) {
        txbuf = kmalloc(sizeof(struct aml_txbuf), GFP_ATOMIC);
        if (!txbuf) {
            ASSERT_ERR(0);
            return;
        }
        txbuf->index = i;
        txbuf->skb = NULL;
        list_add_tail(&txbuf->list, &aml_hw->tx_buf_free_list);
    }
}

void aml_tx_cfmed_list_init(struct aml_hw *aml_hw)
{
    memset(aml_hw->read_cfm, 0, sizeof(struct tx_sdio_usb_cfm_tag) * SRAM_TXCFM_CNT);
    INIT_LIST_HEAD(&aml_hw->tx_cfmed_list);
}

#ifdef CONFIG_SDIO_TX_ENH
void aml_tx_cfm_param_init(struct aml_hw *aml_hw)
{
    memset(&aml_hw->txcfm_param, 0, sizeof(txcfm_param_t));
    aml_hw->txcfm_param.dyn_en = 0;
    aml_hw->txcfm_param.read_blk = 6;
    aml_hw->txcfm_param.read_thresh = TXCFM_THRESH;
    spin_lock_init(&aml_hw->txcfm_rd_lock);
}
#endif

void aml_scan_results_list_init(struct aml_hw *aml_hw)
{
    int i =0;
    struct scan_results *scan_results;

    memset(aml_hw->scan_results, 0, sizeof(struct scan_results) * SCAN_RESULTS_MAX_CNT);
    spin_lock_init(&aml_hw->scan_lock);
    INIT_LIST_HEAD(&aml_hw->scan_res_list);
    INIT_LIST_HEAD(&aml_hw->scan_res_available_list);

    aml_hw->scanres_payload_buf_offset = 0;
    scan_results = aml_hw->scan_results;
    for (i = 0; i < SCAN_RESULTS_MAX_CNT; i++, scan_results++) {
        list_add_tail(&scan_results->list, &aml_hw->scan_res_available_list);
    }
}


struct scan_results *aml_scan_get_scan_res_node(struct aml_hw *aml_hw)
{
    struct scan_results *scan_res;
    spin_lock_bh(&aml_hw->scan_lock);
    scan_res = list_first_entry_or_null(&aml_hw->scan_res_available_list,
                                         struct scan_results, list);
    if (!scan_res) {
        spin_unlock_bh(&aml_hw->scan_lock);
        return 0;
    }

    list_del(&scan_res->list);
    list_add_tail(&scan_res->list, &aml_hw->scan_res_list);
    spin_unlock_bh(&aml_hw->scan_lock);
    return scan_res;
}

struct aml_txbuf *aml_get_from_free_txbuf(struct aml_hw *aml_hw)
{
    struct aml_txbuf *txbuf = NULL;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    if (!list_empty(&aml_hw->tx_buf_free_list)) {
        txbuf = list_first_entry(&aml_hw->tx_buf_free_list, struct aml_txbuf, list);
        list_del(&txbuf->list);
        list_add_tail(&txbuf->list, &aml_hw->tx_buf_used_list);

    } else {
        ASSERT_ERR(0);
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);

    return txbuf;
}

struct sk_buff *aml_get_skb_from_used_txbuf(struct aml_hw *aml_hw, u32_l hostid)
{
    struct aml_txbuf *txbuf = NULL;
    struct aml_txbuf *txbuf_tmp = NULL;
    u8 find_sign = 0;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    list_for_each_entry_safe(txbuf, txbuf_tmp, &aml_hw->tx_buf_used_list, list) {
        if (hostid == txbuf->index) {
            list_del(&txbuf->list);
            list_add_tail(&txbuf->list, &aml_hw->tx_buf_free_list);
            find_sign = 1;
            break;
        }
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);

    if (find_sign) {
        return txbuf->skb;

    } else {
        return NULL;
    }
}


void aml_txbuf_list_deinit(struct aml_hw *aml_hw)
{
    struct aml_txbuf *txbuf = NULL;
    struct aml_txbuf *txbuf_tmp = NULL;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    list_for_each_entry_safe(txbuf, txbuf_tmp, &aml_hw->tx_buf_free_list, list) {
        kfree(txbuf);
    }
    list_for_each_entry_safe(txbuf, txbuf_tmp, &aml_hw->tx_buf_used_list, list) {
        kfree(txbuf);
    }
    spin_unlock_bh(&aml_hw->tx_buf_lock);
}


static enum hrtimer_restart aml_set_tx_lock_timeout(struct hrtimer *timer)
{
    struct aml_hw *aml_hw = container_of(timer, struct aml_hw, hr_lock_timer);
    aml_hw->g_tx_to = 1;
    aml_hw->g_hr_lock_timer_valid = 0;

    up(&aml_hw->aml_tx_sem);
    return HRTIMER_NORESTART;
}

void aml_tx_lock_timer_attach(struct aml_hw *aml_hw)
{
    aml_hw->lock_kt = ktime_set(0, 600000); //0.6ms
    hrtimer_init(&aml_hw->hr_lock_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    aml_hw->hr_lock_timer.function = aml_set_tx_lock_timeout;
    hrtimer_start(&aml_hw->hr_lock_timer, aml_hw->lock_kt, HRTIMER_MODE_REL);
    aml_hw->g_hr_lock_timer_valid = 1;
}

void aml_tx_lock_timer_cancel(struct aml_hw *aml_hw)
{
    aml_hw->g_tx_to = 0;
    if (aml_hw->g_hr_lock_timer_valid) {
        hrtimer_cancel(&aml_hw->hr_lock_timer);
        aml_hw->g_hr_lock_timer_valid = 0;
    }
}

void aml_scatter_req_init(struct aml_hw *aml_hw)
{
    if (w2_aml_bus_type == SDIO_MODE) {
       aml_hw->g_tx_param.scat_req = aml_hw->plat->hif_sdio_ops->hi_get_scatreq(&w2_g_hwif_sdio);
    } else if (w2_aml_bus_type == USB_MODE) {
       aml_hw->g_tx_param.scat_req = aml_hw->plat->hif_ops->hi_get_scatreq();
    }
    if (aml_hw->g_tx_param.scat_req != NULL) {
       aml_hw->g_tx_param.scat_req->req = HIF_WRITE | HIF_ASYNCHRONOUS;
       aml_hw->g_tx_param.scat_req->addr = 0x0;
    } else {
        printk("%s, %d****************no free scatreq**************\n", __func__, __LINE__);
    }
}

void aml_host_send_stop_tx_to_fw(struct aml_hw *aml_hw)
{
    uint32_t cmd_buf[2] = {0};

    cmd_buf[0] = DYNAMIC_BUF_NOTIFY_FW_TX_STOP;
    cmd_buf[1] = 1;
    if (w2_aml_bus_type == USB_MODE) {
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char*)(cmd_buf), (unsigned char *)(SYS_TYPE)(SDIO_USB_EXTEND_E2A_IRQ_STATUS), 8, USB_EP4);
    } else if (w2_aml_bus_type == SDIO_MODE) {
        aml_hw->plat->hif_sdio_ops->hi_sram_write((unsigned char*)(cmd_buf), (unsigned char *)(SYS_TYPE)(SDIO_USB_EXTEND_E2A_IRQ_STATUS), 8);
    }
}

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
block_log blog = {0};
#endif
#endif
int aml_tx_task(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;
    struct aml_sw_txhdr *sw_txhdr,*next;
    struct txdesc_host *txdesc_host = NULL;
    struct sched_param sch_param;
    unsigned char *frm = NULL;
    unsigned char  page_num;
    unsigned int msdu_len = 0;
    unsigned char i = 0;
    unsigned int frame_tot_len = 0;
    unsigned int amsdu_len = 0;
    struct aml_sdio_txhdr *sdio_txhdr;
    uint8_t **amsdu_dynabuf = NULL;
    uint8_t  dynabuf_id = 0;
    uint32_t  dynabuf_size = 0;
    struct tx_amsdu_param *txamsdu = NULL;
    unsigned int blk_size = 512;

    if ((amsdu_dynabuf = vmalloc(sizeof(uint8_t *) * 256)) == NULL) {
        if (aml_hw->aml_tx_completion_init) {
            aml_hw->aml_tx_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
            complete_and_exit(&aml_hw->aml_tx_completion, 0);
#else
            complete(&aml_hw->aml_tx_completion);
#endif
        }
        return -1;
    }
    memset(amsdu_dynabuf, 0, sizeof(uint8_t *) * 256);

    sch_param.sched_priority = 91;
#ifndef CONFIG_PT_MODE
    sched_setscheduler(current, SCHED_FIFO, &sch_param);
#endif
    while (!aml_hw->aml_tx_task_quit) {
        /* wait for work */
        if (down_interruptible(&aml_hw->aml_tx_sem) != 0) {
            /* interrupted, exit */
            AML_PRINT(AML_DBG_MODULES_TX, "wait aml_tx_sem fail!\n");
            break;
        }
        if (aml_hw->aml_tx_task_quit) {
            break;
        }

        if (aml_hw->dynabuf_stop_tx == DYNAMIC_BUF_HOST_TX_STOP) {
            if (aml_hw->send_tx_stop_to_fw) {
                if (aml_hw->g_tx_param.tx_page_free_num == aml_hw->g_tx_param.tx_page_tot_num) {
                    aml_hw->send_tx_stop_to_fw = 0;
                    aml_host_send_stop_tx_to_fw(aml_hw);
                } else {
                    aml_update_tx_cfm((void *)aml_hw);
                    up(&aml_hw->aml_tx_sem);
                }
            }
            continue;
        }

        if (list_empty(&aml_hw->tx_desc_save)) {
            continue;
        }

        spin_lock_bh(&aml_hw->tx_desc_lock);
        list_for_each_entry_safe(sw_txhdr, next, &aml_hw->tx_desc_save, list) {
            txdesc_host = &sw_txhdr->desc;
            if (w2_aml_bus_type == USB_MODE) {
                #ifdef CONFIG_AML_USB_LARGE_PAGE
                frame_tot_len = 0;
                for (i = 0; i < txdesc_host->api.host.packet_cnt; i++) {
                    frame_tot_len += txdesc_host->api.host.packet_len[i];
                }
                page_num = 1;
                #else
                page_num = sw_txhdr->desc.api.host.packet_cnt; //the total of amsdu
                #endif
            } else {
                frame_tot_len = 0;

                for (i = 0; i < txdesc_host->api.host.packet_cnt; i++) {
                    frame_tot_len += txdesc_host->api.host.packet_len[i];
                }
                page_num = howmanypage(frame_tot_len + SDIO_DATA_OFFSET + SDIO_FRAME_TAIL_LEN, SDIO_PAGE_LEN);
            }

            if (((page_num + 1)  <= aml_hw->g_tx_param.tx_page_free_num)
                && ((aml_hw->g_tx_param.tot_page_num + page_num <= aml_hw->g_tx_param.tx_page_once) || (aml_hw->g_tx_param.tot_page_num == 0))) {
#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
                if (blog.block_begin != 0) {
                    blog.block_end = jiffies_to_usecs(jiffies);
                    blog.total_block += blog.block_end - blog.block_begin;
                    blog.block_begin = 0;
                    blog.avg_block = blog.total_block/blog.block_cnt;
                }
                blog.unblock_page += page_num;
#endif
#endif

                ASSERT(aml_hw->g_tx_param.scat_req != NULL);

                aml_hw->g_tx_param.tot_page_num += page_num;
                spin_lock_bh(&aml_hw->tx_buf_lock);
                aml_hw->g_tx_param.tx_page_free_num -= page_num;
                spin_unlock_bh(&aml_hw->tx_buf_lock);

#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
                blog.tx_tot_cnt++;
#endif
#endif

                if (w2_aml_bus_type == SDIO_MODE) {

                    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU) {
                        struct aml_amsdu_txhdr *amsdu_txhdr, *tmp;
                        sdio_txhdr = (struct aml_sdio_txhdr *)sw_txhdr->skb->data;
                        sdio_txhdr->mpdu_buf_flag = 0;
                        sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG|HW_LAST_MPDUBUF_FLAG|HW_LAST_AGG_FLAG;
                        sdio_txhdr->mpdu_buf_flag |= HW_MPDU_LEN_SET(frame_tot_len + SDIO_FRAME_TAIL_LEN);
                        frm = (unsigned char *)sw_txhdr->skb->data + sizeof(struct aml_txhdr);

                        memcpy(frm + TXDESC_OFFSET, txdesc_host, sizeof(*txdesc_host));
                        amsdu_len = sw_txhdr->frame_len + SDIO_TXHEADER_LEN;
#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
                        blog.tx_amsdu_cnt++;
                        if (txdesc_host->api.host.packet_cnt < 6)
                            blog.amsdu_num[txdesc_host->api.host.packet_cnt - 1]++;
                        else
                            blog.amsdu_num[5]++;
#endif
#endif

                        if (frame_tot_len + SDIO_TXHEADER_LEN + SDIO_FRAME_TAIL_LEN >= 4096) {
                            txamsdu = aml_get_free_tx_amsdu_buf(aml_hw);
                            memset(txamsdu->amsdu_buf, 0, AMSDU_BUF_MAX);
                            memcpy(txamsdu->amsdu_buf, frm, amsdu_len);
                            list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                                frm = amsdu_txhdr->skb->data + sizeof(*amsdu_txhdr) + sizeof(struct ethhdr);
                                memcpy(txamsdu->amsdu_buf + amsdu_len, frm, (amsdu_txhdr->msdu_len + sizeof(struct ethhdr) + amsdu_txhdr->pad));
                                amsdu_len += (amsdu_txhdr->msdu_len + sizeof(struct ethhdr) + amsdu_txhdr->pad);
                            }
                            aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = txamsdu->amsdu_buf;

                        } else {
                            dynabuf_size = ALIGN(frame_tot_len + SDIO_TXHEADER_LEN + SDIO_FRAME_TAIL_LEN + 1, blk_size);
                            amsdu_dynabuf[dynabuf_id] = kzalloc(dynabuf_size, GFP_ATOMIC);
                            if (!amsdu_dynabuf[dynabuf_id]) {
                                printk("*************%s:%d, malloc amsdu dynabuf failed****************\n", __LINE__, __func__);
                                break;
                            }
                            memcpy(amsdu_dynabuf[dynabuf_id], frm, amsdu_len);
                            list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                                frm = amsdu_txhdr->skb->data + sizeof(*amsdu_txhdr) + sizeof(struct ethhdr);
                                memcpy(amsdu_dynabuf[dynabuf_id] + amsdu_len, frm, (amsdu_txhdr->msdu_len + sizeof(struct ethhdr) + amsdu_txhdr->pad));
                                amsdu_len += (amsdu_txhdr->msdu_len + sizeof(struct ethhdr) + amsdu_txhdr->pad);
                            }
                            aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = amsdu_dynabuf[dynabuf_id];
                            dynabuf_id = (dynabuf_id + 1) % 256;
                        }
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = frame_tot_len + SDIO_TXHEADER_LEN + SDIO_FRAME_TAIL_LEN;
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = page_num;
                        aml_hw->g_tx_param.scat_req->scat_count++;
                        aml_hw->g_tx_param.scat_req->len += aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len;
                        aml_hw->g_tx_param.mpdu_num++;
                        AML_PRINT(AML_DBG_MODULES_TX, "amsdu %s, tx_page_free_num=%d, credit=%d, pagenum=%d, skb=%p\n", __func__, aml_hw->g_tx_param.tx_page_free_num, sw_txhdr->txq->credits, page_num, sw_txhdr->skb);

                    } else {
                        dynabuf_size = ALIGN(sw_txhdr->frame_len + SDIO_TXHEADER_LEN  + SDIO_FRAME_TAIL_LEN + 1, blk_size);
                        amsdu_dynabuf[dynabuf_id] = kzalloc(dynabuf_size, GFP_ATOMIC);
                        if (!amsdu_dynabuf[dynabuf_id]) {
                            printk("*************%s:%d, malloc amsdu dynabuf failed****************\n", __LINE__, __func__);
                            break;
                        }
                        frm = (unsigned char *)sw_txhdr->skb->data + sizeof(struct aml_txhdr);
                        memcpy(frm + TXDESC_OFFSET, txdesc_host, sizeof(*txdesc_host));
                        memcpy(amsdu_dynabuf[dynabuf_id], frm, sw_txhdr->frame_len + SDIO_TXHEADER_LEN  + SDIO_FRAME_TAIL_LEN);
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = sw_txhdr->frame_len + SDIO_TXHEADER_LEN  + SDIO_FRAME_TAIL_LEN;
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = amsdu_dynabuf[dynabuf_id];
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = page_num;
                        aml_hw->g_tx_param.scat_req->scat_count++;
                        aml_hw->g_tx_param.scat_req->len += aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len;
                        AML_PRINT(AML_DBG_MODULES_TX, "%s, tx_page_free_num=%d, credit=%d, pagenum=%d, skb=%p\n", __func__, aml_hw->g_tx_param.tx_page_free_num, sw_txhdr->txq->credits, page_num, sw_txhdr->skb);
                        aml_hw->g_tx_param.mpdu_num++;
                        dynabuf_id = (dynabuf_id + 1) % 256;
                    }
                } else {
#ifdef CONFIG_AML_USB_LARGE_PAGE
                    frm = (unsigned char *)sw_txhdr->skb->data + sizeof(struct aml_txhdr);
                    memcpy(frm, txdesc_host, sizeof(*txdesc_host));
                    amsdu_len = sw_txhdr->frame_len + sizeof(struct txdesc_host);
                    if (sw_txhdr->desc.api.host.flags & TXU_CNTRL_AMSDU) {
                        struct aml_amsdu_txhdr *amsdu_txhdr, *tmp;

                        txamsdu = aml_get_free_tx_amsdu_buf(aml_hw);
                        if (txamsdu != NULL) {
                            memset(txamsdu->amsdu_buf, 0, AMSDU_BUF_MAX);
                            memcpy(txamsdu->amsdu_buf, frm, amsdu_len);
                            list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                                msdu_len = amsdu_txhdr->msdu_len + 14 + amsdu_txhdr->pad;
                                frm = amsdu_txhdr->skb->data + sizeof(*amsdu_txhdr) + sizeof(struct ethhdr);
                                memcpy(txamsdu->amsdu_buf + amsdu_len, frm, msdu_len);
                                amsdu_len += msdu_len;
                            }
                            aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = txamsdu->amsdu_buf;

                        } else {
                            amsdu_dynabuf[dynabuf_id] = kzalloc(USB_AMSDU_BUF_LEN, GFP_ATOMIC);
                            memcpy(amsdu_dynabuf[dynabuf_id], frm, amsdu_len);

                            list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                                msdu_len = amsdu_txhdr->msdu_len + 14 + amsdu_txhdr->pad;
                                frm = (unsigned char *)amsdu_txhdr->skb->data + sizeof(*amsdu_txhdr) + sizeof(struct ethhdr);
                                memcpy(amsdu_dynabuf[dynabuf_id] + amsdu_len, frm, msdu_len);
                                amsdu_len += msdu_len;
                            }
                            aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = amsdu_dynabuf[dynabuf_id];
                            dynabuf_id = (dynabuf_id + 1) % 256;
                        }
                    } else {
                        aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = frm;
                    }
                    if (amsdu_len > 4620)
                        printk("%s amsdu_len exceeding buf size, amsdu_len:%d\n", __func__, amsdu_len);
                    aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = ALIGN(amsdu_len, 4);
                    aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = 1; //a packet consume one page mostly
                    aml_hw->g_tx_param.scat_req->scat_count++;
                    aml_hw->g_tx_param.scat_req->len += aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len;
                    //printk("%s, skb=%p, hostid=%x\n",  __func__, sw_txhdr->skb, txdesc_host->api.host.hostid);
                    //printk("frame_len=%x,len=%x, scat_count=%x, hostid=%x, Reserve=%x\n", sw_txhdr->frame_len,scat_req->scat_list[mpdu_num].len, scat_req->scat_count, tx_option->hostid, tx_option->Reserve);
                    aml_hw->g_tx_param.mpdu_num++;
#else
                    frm = (unsigned char *)sw_txhdr->skb->data + sizeof(struct aml_txhdr);
                    memcpy(frm, txdesc_host, sizeof(*txdesc_host));
                    aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = ALIGN(sw_txhdr->frame_len+sizeof(struct txdesc_host), 4);
                    aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = 1; //a packet consume one page mostly
                    aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = frm;
                    aml_hw->g_tx_param.scat_req->scat_count++;
                    aml_hw->g_tx_param.scat_req->len += aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len;
                    //printk("%s, skb=%p, hostid=%x\n",  __func__, sw_txhdr->skb, txdesc_host->api.host.hostid);
                    //printk("frame_len=%x,len=%x, scat_count=%x, hostid=%x, Reserve=%x\n", sw_txhdr->frame_len,scat_req->scat_list[mpdu_num].len, scat_req->scat_count, tx_option->hostid, tx_option->Reserve);
                    aml_hw->g_tx_param.mpdu_num++;
                    #ifdef CONFIG_AML_AMSDUS_TX
                    if (txdesc_host->api.host.flags & TXU_CNTRL_AMSDU) {
                        struct aml_amsdu_txhdr *amsdu_txhdr, *tmp;
                        list_for_each_entry_safe(amsdu_txhdr, tmp, &sw_txhdr->amsdu.hdrs, list) {
                            msdu_len = amsdu_txhdr->msdu_len + 14 + amsdu_txhdr->pad;
                            frm = (unsigned char *)amsdu_txhdr->skb->data + sizeof(*amsdu_txhdr) + sizeof(struct ethhdr);

                            if (w2_aml_bus_type == SDIO_MODE) {
                                page_num = howmanypage(msdu_len, SDIO_PAGE_LEN);
                                aml_hw->g_tx_param.tot_page_num += page_num;
                                spin_lock_bh(&aml_hw->tx_buf_lock);
                                aml_hw->g_tx_param.tx_page_free_num -= page_num;
                                spin_unlock_bh(&aml_hw->tx_buf_lock);
                                aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = msdu_len;
                                aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = page_num;
                            } else {
                                aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len = ALIGN(msdu_len, 4);
                                aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].page_num = 1;
                            }

                            aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].packet = frm;
                            aml_hw->g_tx_param.scat_req->scat_count++;
                            aml_hw->g_tx_param.scat_req->len += aml_hw->g_tx_param.scat_req->scat_list[aml_hw->g_tx_param.mpdu_num].len;
                            aml_hw->g_tx_param.mpdu_num++;
                        }
                    }
                    #endif
#endif
                }
            } else {
#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
                if (blog.last_hostid != txdesc_host->api.host.hostid) {
                    if (blog.block_begin) {
                        printk("SHOULD NOT HAPPEN");
                    }
                    blog.block_begin = jiffies_to_usecs(jiffies);
                    blog.block_cnt++;
                    blog.last_hostid = txdesc_host->api.host.hostid;
                    blog.block_rate = 1000 * page_num/(blog.unblock_page + page_num);
                    blog.tot_blk_rate += blog.block_rate;
                    blog.avg_blk_rate = blog.tot_blk_rate/blog.block_cnt;
                    blog.unblock_page = 0;
                }
#endif
#endif
                break;
            }

            list_del(&sw_txhdr->list);
        }
        spin_unlock_bh(&aml_hw->tx_desc_lock);

        if (aml_hw->g_tx_param.tot_page_num) {
#ifdef CONFIG_SDIO_TX_ENH
#ifdef SDIO_TX_ENH_DBG
            blog.send_cnt++;
            blog.page_total += aml_hw->g_tx_param.tot_page_num;
            blog.avg_page = blog.page_total/blog.send_cnt;
#endif
#endif

            if (w2_aml_bus_type == SDIO_MODE) {
                aml_hw->plat->hif_sdio_ops->hi_send_frame(aml_hw->g_tx_param.scat_req);
                for (i = 0; i < dynabuf_id; i++) {
                    if (amsdu_dynabuf[i]) {
                        kfree(amsdu_dynabuf[i]);
                    }
                }
                dynabuf_id = 0;
            } else {
                aml_hw->plat->hif_ops->hi_send_frame(aml_hw->g_tx_param.scat_req);
                #ifdef CONFIG_AML_USB_LARGE_PAGE
                for (i = 0; i < dynabuf_id; i++) {
                    if (amsdu_dynabuf[i]) {
                        kfree(amsdu_dynabuf[i]);
                    }
                }
                dynabuf_id = 0;
                #endif
            }

            aml_set_free_tx_amsdu_buf(aml_hw);
            aml_hw->g_tx_param.mpdu_num = 0;
            aml_hw->g_tx_param.tot_page_num = 0;
        }
    }
    vfree(amsdu_dynabuf);
    if (aml_hw->aml_tx_completion_init) {
        aml_hw->aml_tx_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
        complete_and_exit(&aml_hw->aml_tx_completion, 0);
#else
        complete(&aml_hw->aml_tx_completion);
#endif
    }

    return 0;
}

/**
 * aml_ipc_msg_push() - Push a msg to IPC queue
 *
 * @aml_hw: Main driver data
 * @msg_buf: Pointer to message
 * @len: Size, in bytes, of message
 */
void aml_ipc_msg_push(struct aml_hw *aml_hw, void *msg_buf, uint16_t len)
{
#ifdef CONFIG_AML_POWER_SAVE_MODE
    if (w2_aml_bus_type == PCIE_MODE)
    {
        aml_prevent_fw_sleep(aml_hw->plat, PS_MSG_PUSH);
        aml_wait_fw_wake(aml_hw->plat);
        ipc_host_msg_push(aml_hw->ipc_env, msg_buf, len);
        aml_allow_fw_sleep(aml_hw->plat, PS_MSG_PUSH);
    }
    else
    {
        ipc_host_msg_push(aml_hw->ipc_env, msg_buf, len);
    }
#else
    ipc_host_msg_push(aml_hw->ipc_env, msg_buf, len);
#endif
}

/**
 * aml_ipc_txdesc_push() - Push a txdesc to FW
 *
 * @aml_hw: Main driver data
 * @sw_txhdr: Pointer to the SW TX header associated to the descriptor to push
 * @skb: TX Buffer associated. Pointer saved in ipc env to retrieve it upon confirmation.
 * @hw_queue: Hw queue to push txdesc to
 */
void aml_sdio_ipc_txdesc_push(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                          struct sk_buff *skb, int hw_queue)
{
    struct txdesc_host *txdesc_host = &sw_txhdr->desc;
    txdesc_host->ctrl.hwq = hw_queue;
    txdesc_host->api.host.hostid = ipc_host_tx_host_ptr_to_id(aml_hw->ipc_env, skb);
    txdesc_host->ready = 0xFFFFFFFF;

    if (unlikely(!txdesc_host->api.host.hostid)) {
        dev_err(aml_hw->dev, "No more tx_hostid available \n");
        return;
    }

    spin_lock_bh(&aml_hw->tx_desc_lock);
    list_add_tail(&sw_txhdr->list, &aml_hw->tx_desc_save);
    spin_unlock_bh(&aml_hw->tx_desc_lock);

    up(&aml_hw->aml_tx_sem);

}

void aml_pci_ipc_txdesc_push(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                          struct sk_buff *skb, int hw_queue)
{
    struct txdesc_host *txdesc_host = &sw_txhdr->desc;
    struct aml_ipc_buf *ipc_desc = &sw_txhdr->ipc_desc;

    txdesc_host->ctrl.hwq = hw_queue;
    txdesc_host->api.host.hostid = ipc_host_tx_host_ptr_to_id(aml_hw->ipc_env, skb);
    txdesc_host->ready = 0xFFFFFFFF;

    if (unlikely(!txdesc_host->api.host.hostid)) {
        dev_err(aml_hw->dev, "No more tx_hostid available \n");
        return;
    }

    if (aml_ipc_buf_a2e_init(aml_hw, ipc_desc, txdesc_host, sizeof(*txdesc_host)))
        return ;

    ipc_host_txdesc_push(aml_hw->ipc_env, ipc_desc);
}

void aml_ipc_txdesc_push(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr,
                          struct sk_buff *skb, int hw_queue)
{
    if (w2_aml_bus_type != PCIE_MODE) {
        aml_sdio_ipc_txdesc_push(aml_hw, sw_txhdr, skb, hw_queue);
    } else {
        if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || w2_g_pci_shutdown) {
            AML_INFO("aml_ipc_txdesc_push,bus_suspend_cnt = %x, w2_g_pci_shutdown = %x \n", w2_g_wifi_pm.bus_suspend_cnt, w2_g_pci_shutdown);
        }
        else
            aml_pci_ipc_txdesc_push(aml_hw, sw_txhdr, skb, hw_queue);
    }
}

/**
 * aml_ipc_get_skb_from_cfm() - Retrieve the TX buffer associated to a confirmation buffer
 *
 * @aml_hw: Main driver data
 * @buf: IPC buffer for the confirmation buffer
 * @return: Pointer to TX buffer associated to this confirmation and NULL if confirmation
 * has not yet been updated by firmware
 *
 * To ensure that a confirmation has been processed by firmware check if the hostid field
 * has been updated. If this is the case retrieve TX buffer from it and reset it, otherwise
 * simply return NULL.
 */
struct sk_buff *aml_ipc_get_skb_from_cfm(struct aml_hw *aml_hw,
                                          struct aml_ipc_buf *buf)
{
    struct sk_buff *skb = NULL;
    struct tx_cfm_tag *cfm = buf->addr;

    /* get ownership of confirmation */
    aml_ipc_buf_e2a_sync(aml_hw, buf, 0);

    /* Check host id in the confirmation. */
    /* If 0 it means that this confirmation has not yet been updated by firmware */
    if (cfm->hostid) {
        skb = ipc_host_tx_host_id_to_ptr(aml_hw->ipc_env, cfm->hostid);
        if (unlikely(!skb)) {
            dev_err(aml_hw->dev, "Cannot retrieve skb from cfm=%p/0x%llx, hostid %d in confirmation\n",
                    buf->addr, buf->dma_addr, cfm->hostid);
        } else {
            /* Unmap TX descriptor */
            struct aml_ipc_buf *ipc_desc = &((struct aml_txhdr *)skb->data)->sw_hdr->ipc_desc;
            aml_ipc_buf_a2e_release(aml_hw, ipc_desc);
        }

        cfm->hostid = 0;
    }

    /* always re-give ownership to firmware. */
    aml_ipc_buf_e2a_sync_back(aml_hw, buf, 0);

    return skb;
}

struct aml_fw_trace_ipc_desc ipc;
void *aml_usb_ipc_fw_trace_desc_get(struct aml_hw *aml_hw)
{
    aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)&ipc, (unsigned char *)&aml_hw->ipc_env->shared->trace_pattern, sizeof(ipc), USB_EP4);
    return (void *)&ipc;
}
void *aml_sdio_ipc_fw_trace_desc_get(struct aml_hw *aml_hw)
{
    aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)&ipc, (unsigned char *)&aml_hw->ipc_env->shared->trace_pattern, sizeof(ipc));
    return (void *)&ipc;
}
/**
 * aml_ipc_fw_trace_desc_get() - Return pointer to the start of trace
 * description in IPC environment
 *
 * @aml_hw: Main driver data
 */
void *aml_pci_ipc_fw_trace_desc_get(struct aml_hw *aml_hw)
{
    return (void *)&(aml_hw->ipc_env->shared->trace_pattern);
}

void *aml_ipc_fw_trace_desc_get(struct aml_hw *aml_hw)
{
    if (w2_aml_bus_type == USB_MODE) {
        return (void *)aml_usb_ipc_fw_trace_desc_get(aml_hw);
    } else if (w2_aml_bus_type == SDIO_MODE) {
        return (void *)aml_sdio_ipc_fw_trace_desc_get(aml_hw);
    } else {
        return (void *)aml_pci_ipc_fw_trace_desc_get(aml_hw);
    }
}

/**
 * aml_ipc_sta_buffer - Update counter of buffered data for a given sta
 *
 * @aml_hw: Main driver data
 * @sta: Managed station
 * @tid: TID on which data has been added or removed
 * @size: Size of data to add (or remove if < 0) to STA buffer.
 */
#if 0 //pcie1.0: reduce pcie access to improve throughput
void aml_ipc_sta_buffer(struct aml_hw *aml_hw, struct aml_sta *sta, int tid, int size)
{
    u32_l *buffered;

    if (!sta)
        return;

    if ((sta->sta_idx >= NX_REMOTE_STA_MAX) || (tid >= TID_MAX))
        return;

    buffered = &aml_hw->ipc_env->shared->buffered[sta->sta_idx][tid];

    if (size < 0) {
        size = -size;
        if (*buffered < size)
            *buffered = 0;
        else
            *buffered -= size;
    } else {
        // no test on overflow
        *buffered += size;
    }
}
#endif

void aml_get_noparammsg_info(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg)
{
    if (msg->id == DBG_ERROR_IND) {
        struct aml_ipc_buf *dbgdump_buf = &aml_hw->dbgdump.buf;

        if (w2_aml_bus_type == USB_MODE) {
            aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)dbgdump_buf->addr,
                (unsigned char *)(unsigned long)DEBUG_INFO, sizeof(struct dbg_debug_info_tag) + DGB_INFO_OFFSET, USB_EP4);
        } else if (w2_aml_bus_type == SDIO_MODE) {
            aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)dbgdump_buf->addr,
                (unsigned char *)(unsigned long)DEBUG_INFO, sizeof(struct dbg_debug_info_tag) + DGB_INFO_OFFSET);
        }
    }
}

struct debug_proc_msginfo debug_proc_msgbug[DEBUG_MSGE2A_BUF_CNT];
u8 debug_proc_idx = 0;
void record_proc_msg_buf(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf,struct ipc_e2a_msg *msg)
{
    debug_proc_msgbug[debug_proc_idx].addr = buf->dma_addr;
    debug_proc_msgbug[debug_proc_idx].idx = env->msgbuf_idx;
    debug_proc_msgbug[debug_proc_idx].id = msg->id;
    debug_proc_msgbug[debug_proc_idx].time = jiffies;
    debug_proc_idx++;
    if (debug_proc_idx == DEBUG_MSGE2A_BUF_CNT) {
        debug_proc_idx = 0;
    }
}

#ifdef CONFIG_SDIO_TX_ENH
int aml_update_dyn_txcfm(struct aml_hw *aml_hw, int en)
{
    if (aml_hw->txcfm_param.dyn_en == !!en) {
        printk("txcfm dyn_en isn't changed, ignore\n");
        return 0;
    }

    spin_lock_bh(&aml_hw->txcfm_rd_lock);
    memset(&aml_hw->txcfm_param, 0, sizeof(txcfm_param_t));
    aml_hw->txcfm_param.read_blk = 6;
    aml_hw->txcfm_param.read_thresh = TXCFM_THRESH;
    spin_unlock_bh(&aml_hw->txcfm_rd_lock);
    spin_lock_init(&aml_hw->txcfm_rd_lock);

    aml_hw->txcfm_param.dyn_en = !!en;
    printk("txcfm dyn_en:0x%x success\n", en);

    return 0;
}
#endif

static u8 aml_msg_process(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg, struct aml_ipc_buf *buf)
{
    u8 ret = 0;
    u32 msgcnt = 0;

    if (w2_aml_bus_type != PCIE_MODE) {
        /*get info for msg no param*/
#ifndef CONFIG_PT_MODE
        aml_get_noparammsg_info(aml_hw, msg);
#endif
    }

    if (w2_aml_bus_type == PCIE_MODE) {
        /* check msg cnt */
        msgcnt = ((msg->dummy_src_id << 16) | msg->dummy_dest_id);
        if (msgcnt != aml_hw->ipc_env->msgbuf_cnt + 1) {
            printk("error:fw msg cnt[%u],host last msg cnt[%u],msg id=0x%x,msgidx=%d\n",msgcnt,aml_hw->ipc_env->msgbuf_cnt,msg->id,aml_hw->ipc_env->msgbuf_idx);
            return -1;
        }
    }

    if (w2_aml_bus_type == PCIE_MODE) {
        record_proc_msg_buf(aml_hw->ipc_env, buf,msg);
    }
    /* Relay further actions to the msg parser */
    aml_rx_handle_msg(aml_hw, msg);

    /* Reset the msg buffer and re-use it */
    msg->pattern = 0;
    wmb();

    if (w2_aml_bus_type == PCIE_MODE) {
        /* Push back the buffer to the LMAC */
        ipc_host_msgbuf_push(aml_hw->ipc_env, buf);
        aml_hw->ipc_env->msgbuf_cnt = msgcnt;
    }
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
void aml_traffic_busy_msg(struct aml_hw *aml_hw,struct ipc_e2a_msg *msg1,struct ipc_e2a_msg *msg2)
{
    struct traffic_busy *host_cpu1 = NULL;
    struct traffic_busy *host_cpu2 = NULL;
    struct traffic_busy *msg_param = NULL;
    u32 msgcnt = 0;
    struct aml_vif *aml_vif;

    if (w2_aml_bus_type == PCIE_MODE) {
        if (msg1->pattern == IPC_MSGE2A_VALID_PATTERN) {
            msgcnt = ((msg1->dummy_src_id << 16) | msg1->dummy_dest_id);
            if (msgcnt != aml_hw->ipc_env->msgbuf_cnt + 1) {
                printk("error:fw msg cnt[%u],host last msg cnt[%u],msg id=0x%x,msgidx=%d\n",msgcnt,aml_hw->ipc_env->msgbuf_cnt,msg1->id,aml_hw->ipc_env->msgbuf_idx);
                return ;
            }
            if (msg1->id == PRIV_TRAFFIC_BUSY_IND) {
                msg_param = (struct traffic_busy *) msg1->param;
                if (msg_param->td_flag == TRAFFIC_CPU_FLAG) {
                    host_cpu1 = (struct traffic_busy *)msg1->param;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
                    if (host_cpu1->traffic_busy_flag)
                        aml_cpufreq_boost_update(aml_hw);
                    else
                        aml_cpufreq_boost_remove(aml_hw);
#endif
                }   else if (msg_param->td_flag == TRAFFIC_SCAN_FLAG) {
                    /*tx or rx has traffic*/
                    if (msg_param->traffic_busy_flag == 1)
                        aml_hw->traffic_busy = 1;
                    else
                        aml_hw->traffic_busy = 0;
                }
            }
        }
    } else {
        if (msg1->pattern == IPC_MSGE2A_VALID_PATTERN) {
            host_cpu1 = (struct traffic_busy *)msg1->param;

            if (msg1->id == PRIV_TRAFFIC_BUSY_IND) {
                msg_param = (struct traffic_busy *) msg1->param;
                if (msg_param->td_flag == TRAFFIC_CPU_FLAG) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
                    if (host_cpu1->traffic_busy_flag)
                        aml_cpufreq_boost_update(aml_hw);
                    else
                        aml_cpufreq_boost_remove(aml_hw);
#endif
                }   else if (msg_param->td_flag == TRAFFIC_SCAN_FLAG) {
                    /*tx or rx has traffic*/
                    if (msg_param->traffic_busy_flag == 1)
                        aml_hw->traffic_busy = 1;
                    else
                        aml_hw->traffic_busy = 0;
                }
#ifdef CONFIG_SDIO_TX_ENH
                else if (msg_param->td_flag == TRAFFIC_TXCFM_FLAG) {
                    /*tx or rx has traffic*/
                    if (msg_param->traffic_busy_flag == 1) {
                        aml_update_dyn_txcfm(aml_hw, 1);
                    } else {
                        aml_update_dyn_txcfm(aml_hw, 0);
                    }
                }
#endif
            }
        }
        if (msg2->pattern == IPC_MSGE2A_VALID_PATTERN) {
            host_cpu2 = (struct traffic_busy *)msg2->param;

            if (msg2->id == PRIV_TRAFFIC_BUSY_IND) {
                msg_param = (struct traffic_busy *) msg2->param;
                if (msg_param->td_flag == TRAFFIC_CPU_FLAG) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
                    if (host_cpu2->traffic_busy_flag)
                        aml_cpufreq_boost_update(aml_hw);
                    else
                        aml_cpufreq_boost_remove(aml_hw);
#endif
                } else if (msg_param->td_flag == TRAFFIC_SCAN_FLAG) {
                    /*tx or rx has traffic*/
                    if (msg_param->traffic_busy_flag == 1)
                        aml_hw->traffic_busy = 1;
                    else
                        aml_hw->traffic_busy = 0;
                }
#ifdef CONFIG_SDIO_TX_ENH
                else if (msg_param->td_flag == TRAFFIC_TXCFM_FLAG) {
                    /*tx or rx has traffic*/
                    if (msg_param->traffic_busy_flag == 1) {
                        aml_update_dyn_txcfm(aml_hw, 1);
                    } else {
                        aml_update_dyn_txcfm(aml_hw, 0);
                    }
                }
#endif
            }
        }
    }
    return;
}
#endif

/**
 * aml_msgind() - IRQ handler callback for %IPC_IRQ_E2A_MSG
 *
 * @pthis: Pointer to main driver data
 * @arg: Pointer to IPC buffer from msgbuf_pool
 */
static u8 aml_msgind(void *pthis, void *arg)
{
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *buf = arg;
    struct ipc_e2a_msg *msg1 = NULL;
    struct ipc_e2a_msg *msg2 = NULL;
    u8 ret = 0;

    REG_SW_SET_PROFILING(aml_hw, SW_PROF_MSGIND);

    if (aml_hw->cmd_mgr.state & AML_CMD_MGR_STATE_DEINIT) {
        return 0;
    }

    if (w2_aml_bus_type == USB_MODE) {
        msg1 = &aml_hw->g_msg1;
        msg2 = &aml_hw->g_msg2;

        aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)msg1,
            (unsigned char *)&(aml_hw->ipc_env->shared->msg_e2a_buf), sizeof(struct ipc_e2a_msg), USB_EP4);

        aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)msg2,
            (unsigned char *)E2A_MSG_EXT_BUF, sizeof(struct ipc_e2a_msg), USB_EP4);

        ipc_app2emb_trigger_set(aml_hw, IPC_A2E_MSG_IND);

    } else if (w2_aml_bus_type == SDIO_MODE) {
        msg1 = &aml_hw->g_msg1;
        msg2 = &aml_hw->g_msg2;

        aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)msg1,
            (unsigned char *)&(aml_hw->ipc_env->shared->msg_e2a_buf), sizeof(struct ipc_e2a_msg));

        aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)msg2,
            (unsigned char *)E2A_MSG_EXT_BUF, sizeof(struct ipc_e2a_msg));

        ipc_app2emb_trigger_set(aml_hw, IPC_A2E_MSG_IND);
    } else {
        msg1 = buf->addr;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)

    aml_traffic_busy_msg(aml_hw,msg1,msg2);

#endif

#ifdef CONFIG_LINUXPC_VERSION
    {
        int len;
        struct ipc_e2a_msg msg_pc;

        // check: fw msg is ready
        if (AML_REG_READ(aml_hw->plat, AML_ADDR_MAC_PHY, UBUNTU_SYNC_ADDR) != UBUNTU_SYNC_PATTERN
            || aml_hw->ipc_env == NULL || aml_hw->ipc_env->shared == NULL) {
            return -1;
        }

        // get msg len and copy it
        len = aml_hw->ipc_env->shared->msg_e2a_buf.param_len;
        // align len, the "8" is offsetof(struct ipc_e2a_msg, param)
        len = len & 0x3 ? (len & ~0x3) + 4 + 8 : len + 8;
        if (len <= sizeof(struct ipc_e2a_msg))
            memcpy(&msg_pc, &(aml_hw->ipc_env->shared->msg_e2a_buf), len);
        else {
            printk("error %s %d\n", __func__, __LINE__);
            return -1;
        }

        //indicate that host has read mem
        AML_REG_WRITE(0, aml_hw->plat, AML_ADDR_MAC_PHY, UBUNTU_SYNC_ADDR);

        aml_msg_process(aml_hw, &msg_pc, buf);

        return 0;
    }
#endif

    /* Look for pattern which means that this hostbuf has been used for a MSG */
    if (msg1->pattern == IPC_MSGE2A_VALID_PATTERN) {
        if (msg1->id != 0)
            ret = aml_msg_process(aml_hw, msg1, buf);
    } else {
        ret = -1;
    }

    if (w2_aml_bus_type != PCIE_MODE) {
        if (msg2->pattern == IPC_MSGE2A_VALID_PATTERN) {
            if (msg2->id != 0)
                ret = aml_msg_process(aml_hw, msg2, buf);
        } else {
            ret = -1;
        }
    }
    if (aml_hw->suspend_ind == SUSPEND_IND_RECV)
        aml_hw->suspend_ind = SUSPEND_IND_DONE;
    return ret;

}

/**
 * aml_msgackind() - IRQ handler callback for %IPC_IRQ_E2A_MSG_ACK
 *
 * @pthis: Pointer to main driver data
 * @hostid: Pointer to command acknowledged
 */
static u8 aml_msgackind(void *pthis, void *hostid)
{
    struct aml_hw *aml_hw = (struct aml_hw *)pthis;
    struct aml_cmd *cmd = (struct aml_cmd *)hostid;
    if (aml_hw->cmd_mgr.state & AML_CMD_MGR_STATE_DEINIT) {
        return 0;
    }

    if (cmd->id == MM_OTHER_REQ && is_mdnsoffload_msg(cmd->mm_sub_id))
        MDNS_OFFLOAD_DEBUG("msg ack hostid=0x%lx\n", hostid);
    else
        AML_INFO("msg ack hostid=0x%lx\n", hostid);
    aml_hw->cmd_mgr.llind(&aml_hw->cmd_mgr, cmd);

    return -1;
}

/**
 * aml_radarind() - IRQ handler callback for %IPC_IRQ_E2A_RADAR
 *
 * @pthis: Pointer to main driver data
 * @arg: Pointer to IPC buffer from radar_pool
 */
static u8 aml_radarind(void *pthis, void *arg)
{
#ifdef CONFIG_AML_RADAR
    struct aml_hw *aml_hw = pthis;
    struct aml_ipc_buf *buf = arg;
    struct radar_pulse_array_desc *pulses = NULL;
    u8 ret = 0;
    int i;
    if (w2_aml_bus_type != PCIE_MODE) {
        pulses = &aml_hw->g_pulses;
        aml_hw->radar_pulse_index = (aml_hw->radar_pulse_index + 1) % RADAR_EVENT_MAX;
    } else {
        pulses = buf->addr;
    }

    /* Look for pulse count meaning that this hostbuf contains RADAR pulses */
    if (pulses->cnt == 0) {
        if (w2_aml_bus_type != PCIE_MODE) {
            if (aml_hw->radar_pulse_index > 0) {
                aml_hw->radar_pulse_index = (aml_hw->radar_pulse_index - 1) % RADAR_EVENT_MAX;
            } else {
                aml_hw->radar_pulse_index = RADAR_EVENT_MAX - 1;
            }
        }
        ret = -1;
        goto radar_no_push;
    }

    if (pulses->idx >= AML_RADAR_LAST) {
         AML_INFO("invalid idx: %d\n", pulses->idx);
         return -1;
    }
    if (aml_radar_detection_is_enable(&aml_hw->radar, pulses->idx)) {
        /* Save the received pulses only if radar detection is enabled */
        for (i = 0; i < pulses->cnt; i++) {
            struct aml_radar_pulses *p = &aml_hw->radar.pulses[pulses->idx];

            p->buffer[p->index] = pulses->pulse[i];
            p->index = (p->index + 1) % AML_RADAR_PULSE_MAX;
            if (p->count < AML_RADAR_PULSE_MAX)
                p->count++;
        }

        /* Defer pulse processing in separate work */
        if (! work_pending(&aml_hw->radar.detection_work))
            schedule_work(&aml_hw->radar.detection_work);
    }

    /* Reset the radar bufent and re-use it */
    pulses->cnt = 0;
    wmb();

    if (w2_aml_bus_type == PCIE_MODE) {
        /* Push back the buffer to the LMAC */
        ipc_host_radar_push(aml_hw->ipc_env, buf);
    }

radar_no_push:
    return ret;
#else
    return -1;
#endif
}

extern struct debug_push_msginfo debug_push_msgbug[DEBUG_MSGE2A_BUF_CNT];
extern u8 debug_push_idx;
/**
 * aml_dbgind() - IRQ handler callback for %IPC_IRQ_E2A_DBG
 *
 * @pthis: Pointer to main driver data
 * @hostid: Pointer to IPC buffer from dbgbuf_pool
 */
static u8 aml_dbgind(void *pthis, void *arg)
{
    struct aml_hw *aml_hw = (struct aml_hw *)pthis;
    struct aml_ipc_buf *buf = NULL;
    struct ipc_dbg_msg *dbg_msg = NULL;
    u8 ret = 0;
    u8 is_assert = 0;
    u8 i = 0;
    char debug_string[60] = "ASSERT (***** assert_err *****: ) at patch_ipc_emb.c";

    if (w2_aml_bus_type == USB_MODE) {
        dbg_msg = &aml_hw->g_dbg_msg;
        aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)dbg_msg, (unsigned char *)&(aml_hw->ipc_env->shared->dbg_buf), sizeof(struct ipc_dbg_msg), USB_EP4);

        ipc_app2emb_trigger_set(aml_hw, IPC_IRQ_A2E_DBG);
    } else if (w2_aml_bus_type == SDIO_MODE) {
        dbg_msg = &aml_hw->g_dbg_msg;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)dbg_msg, (unsigned char *)&(aml_hw->ipc_env->shared->dbg_buf),  sizeof(struct ipc_dbg_msg));

        ipc_app2emb_trigger_set(aml_hw, IPC_IRQ_A2E_DBG);
    } else {
        buf = arg;
        dbg_msg = buf->addr;
    }

    REG_SW_SET_PROFILING(aml_hw, SW_PROF_DBGIND);

    /* Look for pattern which means that this hostbuf has been used for a MSG */
    if (dbg_msg->pattern != IPC_DBG_VALID_PATTERN) {
        ret = -1;
        goto dbg_no_push;
    }

    /* Display the string */
    if (strlen((const char *)dbg_msg->string) > 0)
        printk("%s %s", (char *)FW_STR, (char *)dbg_msg->string);
    if (0 == strncmp((const char *)dbg_msg->string,(const char *)debug_string,strlen((const char *)debug_string))) {
        is_assert = 1;
    }
    if ((w2_aml_bus_type == PCIE_MODE) && (is_assert == 1)) {
        printk("debug_push_idx=%d,msgbuf_idx=%d,debug_proc_idx=%d\n",debug_push_idx,aml_hw->ipc_env->msgbuf_idx,debug_proc_idx);
        while (i < DEBUG_MSGE2A_BUF_CNT) {
            printk("push msgbuf idx=%d,addr=0x%x,next_addr=0x%x,time=%u\n",debug_push_msgbug[i].idx,debug_push_msgbug[i].addr,debug_push_msgbug[i].next_addr,debug_push_msgbug[i].time);
            i++;
        }
        i=0;
        while (i < DEBUG_MSGE2A_BUF_CNT) {
            printk("proc msgbuf id=0x%x,idx=%d,addr=0x%x,time=%u\n",debug_proc_msgbug[i].id, debug_proc_msgbug[i].idx,debug_proc_msgbug[i].addr,debug_proc_msgbug[i].time);
            i++;
        }
    }
    /* Reset the msg buffer and re-use it */
    dbg_msg->pattern = 0;
    wmb();

    if (w2_aml_bus_type == PCIE_MODE) {
        /* Push back the buffer to the LMAC */
        ipc_host_dbgbuf_push(aml_hw->ipc_env, buf);
    }

dbg_no_push:
    REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_DBGIND);

    return ret;
}

void aml_get_proc_msg(struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    int32_t i = 0;

    printk("debug_push_idx=%d,msgbuf_idx=%d,debug_proc_idx=%d\n", debug_push_idx,
        aml_hw->ipc_env->msgbuf_idx, debug_proc_idx);
    while (i < DEBUG_MSGE2A_BUF_CNT) {
        printk("push msgbuf idx=%d,addr=0x%x,next_addr=0x%x,time=%u\n", debug_push_msgbug[i].idx,
            debug_push_msgbug[i].addr, debug_push_msgbug[i].next_addr, debug_push_msgbug[i].time);
        i++;
    }

    i = 0;
    while (i < DEBUG_MSGE2A_BUF_CNT) {
        printk("proc msgbuf id=0x%x,idx=%d,addr=0x%x,time=%u\n", debug_proc_msgbug[i].id,
            debug_proc_msgbug[i].idx, debug_proc_msgbug[i].addr, debug_proc_msgbug[i].time);
        i++;
    }
}

#ifdef DEBUG_CODE
extern struct debug_push_rxbuff_info debug_push_rxbuff[DEBUG_RX_BUF_CNT];
extern u16 debug_push_rxbuff_idx;
extern struct debug_proc_rxbuff_info debug_proc_rxbuff[DEBUG_RX_BUF_CNT];
extern u16 debug_rxbuff_idx;
extern struct ipc_shared_rx_desc *g_host_rxdesc;
extern struct debug_push_rxdesc_info debug_push_rxdesc[DEBUG_RX_BUF_CNT];
extern u16 debug_push_rxdesc_idx;
void aml_get_proc_rxbuff(struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct ipc_host_env_tag *env = aml_hw->ipc_env;
    struct ipc_shared_env_tag *shared_env = env->shared;
    struct ipc_shared_rx_desc *host_rxdesc;
    struct aml_ipc_buf *ipc_desc;
    struct rxdesc_tag *rxdesc;
    int32_t i = 0;

    printk("debug_rxbuff_idx=%u,rxdesc idx=%u,rxbuf_idx=%u,debug_push_idx=%u\n", debug_rxbuff_idx,aml_hw->ipc_env->rxdesc_idx,
        aml_hw->ipc_env->rxbuf_idx, debug_push_rxbuff_idx);
    while (i < DEBUG_RX_BUF_CNT) {
        printk("proc_rxdesc idx=%u,addr=0x%x,hostid=0x%x,status=0x%x,buffidx=%u,time=%u\n", debug_proc_rxbuff[i].idx,
            debug_proc_rxbuff[i].addr, debug_proc_rxbuff[i].hostid, debug_proc_rxbuff[i].status,debug_proc_rxbuff[i].buff_idx, debug_proc_rxbuff[i].time);
        i++;
    }

    i = 0;
    while (i < DEBUG_RX_BUF_CNT) {
        printk("push rxbuff idx=%d,addr=0x%x,hostid=0x%x,time=%u\n", debug_push_rxbuff[i].idx,
            debug_push_rxbuff[i].addr, debug_push_rxbuff[i].hostid, debug_push_rxbuff[i].time);
        i++;
    }

    i = 0;
    while (i < DEBUG_RX_BUF_CNT) {
        printk("push rxdesc idx=%d,addr=0x%x,time=%u\n", debug_push_rxdesc[i].idx,
            debug_push_rxdesc[i].addr, debug_push_rxdesc[i].time);
        i++;
    }

    i= 0;
    while (i < IPC_RXDESC_CNT) {
        host_rxdesc = (struct ipc_shared_rx_desc *)&shared_env->host_rxdesc[i];
        ipc_desc = env->rxdesc[i];
        aml_ipc_buf_e2a_sync(aml_hw, ipc_desc, sizeof(struct rxdesc_tag));
        rxdesc = ipc_desc->addr;
        printk("rx desc idx=%d,dma addr=0x%x,status=%d,hostid=0x%x\n",i,host_rxdesc->dma_addr,rxdesc->status,rxdesc->host_id);
        i++;
    }
    while (i < (IPC_RXDESC_CNT + IPC_RXDESC_CNT_EXT)) {
        host_rxdesc = g_host_rxdesc + (i - IPC_RXDESC_CNT);
        ipc_desc = env->rxdesc[i];
        aml_ipc_buf_e2a_sync(aml_hw, ipc_desc, sizeof(struct rxdesc_tag));
        rxdesc = ipc_desc->addr;
        printk("rx desc idx=%d,dma addr=0x%x,status=%d,hostid=0x%x\n",i,host_rxdesc->dma_addr,rxdesc->status,rxdesc->host_id);
        i++;
    }
}
#endif

static unsigned int flag_end = 0;
int aml_traceind(void *pthis, int mode)
{
    struct aml_hw *aml_hw = (struct aml_hw *)pthis;
    unsigned int end = 0;
    uint16_t *ptr_limit = NULL;
    uint16_t *ptr_flag = NULL;
    int ret = 0;
    uint32_t trace_max_size = 0;

#ifdef CONFIG_AML_DEBUGFS
    mutex_lock(&trace_log_file_info.mutex);

    if (!trace_log_file_info.ptr)
        goto err;

    memset(trace_log_file_info.ptr, 0, 33*1024);
    ptr_flag = trace_log_file_info.ptr;
#endif

    if (w2_aml_bus_type == USB_MODE) {
        aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)ptr_flag, (unsigned char *)(SYS_TYPE)USB_TRACE_START_ADDR, USB_TRACE_TOTAL_SIZE, USB_EP4);
        end = aml_hw->plat->hif_ops->hi_read_word((unsigned long)&(aml_hw->ipc_env->shared->trace_end), USB_EP4);
        trace_max_size = USB_TRACE_MAX_SIZE;
    } else if (w2_aml_bus_type == SDIO_MODE) {
        aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)ptr_flag, (unsigned char *)(SYS_TYPE)SDIO_TRACE_START_ADDR, SDIO_TRACE_TOTAL_SIZE);
        end = aml_hw->plat->hif_sdio_ops->hi_random_word_read((unsigned long)&(aml_hw->ipc_env->shared->trace_end));
        trace_max_size = SDIO_TRACE_MAX_SIZE;
    }

#ifdef CONFIG_AML_DEBUGFS
    if (mode) {
        if (end >= (trace_max_size >> 1)) {
            ptr_limit = ptr_flag + end;
            flag_end = end;
        } else {
            if (flag_end) {
                ptr_limit = ptr_flag + trace_max_size;
                ptr_flag = ptr_flag + flag_end;
            }
        }
    } else {
        if (end > flag_end) {
            ptr_limit = ptr_flag + end;
            ptr_flag = ptr_flag + flag_end;
        } else {
            ptr_limit = ptr_flag + end;
        }
    }

    ret = aml_trace_log_to_file(ptr_flag, ptr_limit);
    if (ret) {
        printk("aml_traceind  trace log to file fail\n");
        goto err;
    }

    mutex_unlock(&trace_log_file_info.mutex);
#endif

    return 0;
err:
#ifdef CONFIG_AML_DEBUGFS
    mutex_unlock(&trace_log_file_info.mutex);
#endif

    return -1;
}

/**
 * aml_ipc_rxbuf_init() - Allocate and initialize RX buffers.
 *
 * @aml_hw: Main driver data
 * @rxbuf_sz: Size of the buffer to be allocated
 *
 * This function updates the RX buffer size according to the parameter and allocates the
 * RX buffers
 */
int aml_ipc_rxbuf_init(struct aml_hw *aml_hw, uint32_t rxbuf_sz)
{
    aml_hw->ipc_env->rxbuf_sz = rxbuf_sz;
#ifdef CONFIG_AML_PREALLOC_BUF_SKB
    aml_prealloc_rxbuf_init(aml_hw, rxbuf_sz);
#endif
    return aml_ipc_rxbufs_alloc(aml_hw);
}

/**
 * aml_ipc_init() - Initialize IPC interface.
 *
 * @aml_hw: Main driver data
 * @shared_ram: Pointer to shared memory that contains IPC shared struct
 *
 * This function initializes IPC interface by registering callbacks, setting
 * shared memory area and calling IPC Init function.
 * It should be called only once during driver's lifetime.
 */
int aml_ipc_init(struct aml_hw *aml_hw, u8 *shared_ram, u8 *shared_host_rxbuf, u8 *shared_host_rxdesc)
{
    struct ipc_host_cb_tag cb;
    int res = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    /* initialize the API interface */
    cb.recv_data_ind   = aml_rxdataind;
    cb.recv_radar_ind  = aml_radarind;
    cb.recv_msg_ind    = aml_msgind;
    cb.recv_msgack_ind = aml_msgackind;
    cb.recv_dbg_ind    = aml_dbgind;
    cb.send_data_cfm   = aml_txdatacfm;
    cb.recv_unsup_rx_vec_ind = aml_unsup_rx_vec_ind;
    cb.recv_trace_ind = aml_traceind;

    /* set the IPC environment */
    aml_hw->ipc_env = (struct ipc_host_env_tag *)
                       kzalloc(sizeof(struct ipc_host_env_tag), GFP_KERNEL);

    if (!aml_hw->ipc_env)
        return -ENOMEM;

    /* call the initialization of the IPC */
    ipc_host_init(aml_hw->ipc_env, &cb,
                   (struct ipc_shared_env_tag *)shared_ram,
                   (struct ipc_shared_rx_buf *)shared_host_rxbuf,
                   (struct ipc_shared_rx_desc *)shared_host_rxdesc,
                   aml_hw);

    aml_cmd_mgr_init(&aml_hw->cmd_mgr);

    res = aml_elems_allocs(aml_hw);
    if (res) {
        kfree(aml_hw->ipc_env);
        aml_hw->ipc_env = NULL;
    }

    return res;
}

/**
 * aml_ipc_deinit() - Release IPC interface
 *
 * @aml_hw: Main driver data
 */
void aml_ipc_deinit(struct aml_hw *aml_hw)
{
    AML_DBG(AML_FN_ENTRY_STR);
    aml_tcp_delay_ack_deinit(aml_hw);
    aml_ipc_tx_drain(aml_hw);
    aml_cmd_mgr_deinit(&aml_hw->cmd_mgr);
    aml_elems_deallocs(aml_hw);
    if (aml_hw->ipc_env) {
        kfree(aml_hw->ipc_env);
        aml_hw->ipc_env = NULL;
    }
}

/**
 * aml_ipc_start() - Start IPC interface
 *
 * @aml_hw: Main driver data
 */
void aml_ipc_start(struct aml_hw *aml_hw)
{
    ipc_host_enable_irq(aml_hw->ipc_env, IPC_IRQ_E2A_ALL);
}

/**
 * aml_ipc_stop() - Stop IPC interface
 *
 * @aml_hw: Main driver data
 */
void aml_ipc_stop(struct aml_hw *aml_hw)
{
    ipc_host_disable_irq(aml_hw->ipc_env, IPC_IRQ_E2A_ALL);
}

/**
 * aml_ipc_tx_drain() - Flush IPC TX buffers
 *
 * @aml_hw: Main driver data
 *
 * This assumes LMAC is still (tx wise) and there's no TX race until LMAC is up
 * tx wise.
 * This also lets both IPC sides remain in sync before resetting the LMAC,
 * e.g with aml_send_reset.
 */
void aml_ipc_tx_drain(struct aml_hw *aml_hw)
{
    struct sk_buff *skb;

    AML_DBG(AML_FN_ENTRY_STR);

    if (!aml_hw->ipc_env) {
        printk(KERN_CRIT "%s: bypassing (restart must have failed)\n", __func__);
        return;
    }

    while ((skb = ipc_host_tx_flush(aml_hw->ipc_env))) {
        struct aml_sw_txhdr *sw_txhdr = ((struct aml_txhdr *)skb->data)->sw_hdr;
        struct aml_hwq *hwq = &aml_hw->hwq[sw_txhdr->hw_queue];
        struct aml_txq *txq = sw_txhdr->txq;

        aml_txq_confirm_any(aml_hw, txq, hwq, sw_txhdr);

#ifdef CONFIG_AML_AMSDUS_TX
        if (sw_txhdr->desc.api.host.packet_cnt > 1) {
            struct aml_amsdu_txhdr *amsdu_txhdr;
            list_for_each_entry(amsdu_txhdr, &sw_txhdr->amsdu.hdrs, list) {
                aml_ipc_buf_a2e_release(aml_hw, &amsdu_txhdr->ipc_data);
                dev_kfree_skb_any(amsdu_txhdr->skb);
            }
        }
#endif
        aml_ipc_buf_a2e_release(aml_hw, &sw_txhdr->ipc_data);
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
#ifdef CONFIG_AML_SOFTMAC
        skb_pull(skb, sw_txhdr->headroom);
        ieee80211_free_txskb(aml_hw->hw, skb);
#else
        if (w2_aml_bus_type == SDIO_MODE) {
            skb_pull(skb, AML_SDIO_TX_HEADROOM);
        } else if (w2_aml_bus_type == USB_MODE) {
            skb_pull(skb, AML_USB_TX_HEADROOM);
        } else {
            skb_pull(skb, AML_TX_HEADROOM);
        }
        dev_kfree_skb_any(skb);
#endif /* CONFIG_AML_SOFTMAC */
    }
}

/**
 * aml_ipc_tx_pending() - Check if TX frames are pending at FW level
 *
 * @aml_hw: Main driver data
 */
bool aml_ipc_tx_pending(struct aml_hw *aml_hw)
{
    return ipc_host_tx_frames_pending(aml_hw->ipc_env);
}

/**
 * aml_error_ind() - %DBG_ERROR_IND message callback
 *
 * @aml_hw: Main driver data
 *
 * This function triggers the UMH script call that will indicate to the user
 * space the error that occurred and stored the debug dump. Once the UMH script
 * is executed, the aml_umh_done() function has to be called.
 */
void aml_error_ind(struct aml_hw *aml_hw)
{
    struct aml_ipc_buf *buf = &aml_hw->dbgdump.buf;
    struct dbg_debug_dump_tag *dump = buf->addr;

    if (w2_aml_bus_type == PCIE_MODE) {
        aml_ipc_buf_e2a_sync(aml_hw, buf, 0);
    } else {
        dump = (struct dbg_debug_dump_tag *)((char*)buf->addr + DGB_INFO_OFFSET);
    }
    dev_err(aml_hw->dev, "(type %d): dump received\n", dump->dbg_info.error_type);
#ifdef CONFIG_AML_DEBUGFS
    aml_hw->debugfs.trace_prst = true;
#endif
    aml_trigger_um_helper(&aml_hw->debugfs);
}

/**
 * aml_umh_done() - Indicate User Mode helper finished
 *
 * @aml_hw: Main driver data
 *
 */
void aml_umh_done(struct aml_hw *aml_hw)
{
    if (!test_bit(AML_DEV_STARTED, (void*)&aml_hw->flags))
        return;

    /* this assumes error_ind won't trigger before ipc_host_dbginfo_push
       is called and so does not irq protect (TODO) against error_ind */
#ifdef CONFIG_AML_DEBUGFS
    aml_hw->debugfs.trace_prst = false;
#endif
    if (w2_aml_bus_type == PCIE_MODE) {
        ipc_host_dbginfo_push(aml_hw->ipc_env, &aml_hw->dbgdump.buf);
    }
}

const char *ssid_sprintf(const unsigned char *ssid, unsigned char ssid_len) {
    static unsigned char slssid[MAC_SSID_LEN + 1];
    memset(slssid, 0, MAC_SSID_LEN + 1);
    if (ssid && ssid_len > 0 && ssid_len <= MAC_SSID_LEN)
        memcpy(slssid, ssid, ssid_len);
    return slssid;
}

u32 aml_ieee80211_chan_to_freq(u32 chan, u32 band)
{
    if (band == NL80211_BAND_5GHZ) {
        if (chan >= 182 && chan <= 196) {
            return 4000 + chan * 5;
        } else {
            return 5000 + chan * 5;
        }
    } else {
        if (chan == 14) {
            return 2484;
        } else if (chan < 14) {
            return 2407 + chan * 5;
        } else {
            return 0;
        }
    }
}

u8 aml_ieee80211_freq_to_chan(u32 freq, u32 band)
{
    if (band == NL80211_BAND_5GHZ) {
        if (freq >= 4910 && freq <= 4980) {
            return (freq - 4000) / 5;
        } else {
            return (freq - 5000) / 5;
        }
    } else {
        if (freq == 2484) {
            return 14;
        } else if (freq < 2484) {
            return (freq - 2407) / 5;
        } else {
            return 0;
        }
    }
}

uint32_t aml_read_reg(struct net_device *dev,uint32_t reg_addr)
{
    unsigned char *map_address = NULL;
    if (w2_aml_bus_type == PCIE_MODE) {
        map_address = aml_pci_get_map_address(dev, reg_addr);
        if (map_address) {
            return w2_aml_pci_readl(map_address);
        }
    } else {
        struct aml_vif *aml_vif = netdev_priv(dev);
        struct aml_hw *aml_hw = aml_vif->aml_hw;
        struct aml_plat *aml_plat = aml_hw->plat;
        return (AML_REG_READ(aml_plat, 0, reg_addr));
    }
    return 0;
}
