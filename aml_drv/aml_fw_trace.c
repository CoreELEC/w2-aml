/**
 ******************************************************************************
 *
 * @file aml_fw_trace.c
 *
 * Copyright (C) Amlogic 2017-2021
 *
 ******************************************************************************
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/vmalloc.h>
#include "aml_defs.h"
#include "wifi_w2_shared_mem_cfg.h"
#include "share_mem_map.h"

#define TRACE_LEVEL (0x00d25fd8)  //0x00825fd8

#define AML_TRACE_NL_PROTOCOL (28)

#define AML_FW_TRACE_HEADER_LEN 4
#define AML_FW_TRACE_HEADER_FMT "ts=%12u ID=%8d"
#define AML_FW_TRACE_HEADER_ASCII_LEN (3 + 12 + 4 + 8)
#define AML_FW_TRACE_PARAM_FMT ", %5d"
#define AML_FW_TRACE_PARAM_ASCII_LEN (7)

#define AML_FW_TRACE_NB_PARAM(a) ((*a >> 8) & 0xff)
#define AML_FW_TRACE_ID(a) (uint32_t)(((a[0] & 0xff) << 16) + a[1])
#define AML_FW_TRACE_ENTRY_SIZE(a) (AML_FW_TRACE_NB_PARAM(a) + \
                                     AML_FW_TRACE_HEADER_LEN)

#define AML_FW_TRACE_READY  0x1234
#define AML_FW_TRACE_LOCKED 0xdead
#define AML_FW_TRACE_LOCKED_HOST 0x0230
#define AML_FW_TRACE_LAST_ENTRY 0xffff

#define AML_FW_TRACE_RESET "*** RESET ***\n"
#define AML_FW_TRACE_RESET_SIZE sizeof(AML_FW_TRACE_RESET) - 1 // don't count '\0'

static int trace_last_reset=0;

static const int startup_max_to = 500;

static uint32_t *saved_filters = NULL;
static int saved_filters_cnt = 0;

#define AML_FW_TRACE_CHECK_INT_MS 1000

struct log_file_info trace_log_file_info;

extern struct auc_hif_ops g_auc_hif_ops;
extern struct pci_dev *g_pci_dev;

struct aml_trace_nl_info {
    struct sock * fw_log_sock;
    int user_pid;
    int enable;
};

struct log_nl_msg_info {
    int msg_type;
    int msg_len;
};

struct aml_trace_nl_info g_trace_nl_info;

int aml_send_log_to_user(char *pbuf, uint16_t len, int msg_type);

/**
 * aml_fw_trace_work() - Work function to check for new traces
 *                        process function for &struct aml_fw_trace.work
 *
 * @ws: work structure
 *
 * Check if new traces are available in the shared buffer, by comparing current
 * end index with end index in the last check. If so wake up pending threads,
 * otherwise re-schedule the work is there are still some pending readers.
 *
 * Note: If between two check firmware exactly write one buffer of trace then
 * those traces will be lost. Fortunately this is very unlikely to happen.
 *
 * Note: Even if wake_up doesn't actually wake up threads (because condition
 * failed), calling waitqueue_active just after will still return false.
 * Fortunately this should never happen (new trace should always trigger the
 * waiting condition) otherwise it may be needed to re-schedule the work after
 * wake_up.
 */
static void aml_fw_trace_work(struct work_struct *ws)
{
    struct delayed_work *dw = container_of(ws, struct delayed_work, work);
    struct aml_fw_trace *trace = container_of(dw, struct aml_fw_trace, work);

    if (trace->closing ||
        (!aml_fw_trace_empty(&trace->buf) &&
         trace->last_read_index != *trace->buf.end)) {
        trace->last_read_index = *trace->buf.end;
        wake_up_interruptible(&trace->queue);
        return;
    }

    if (waitqueue_active(&trace->queue) && !delayed_work_pending(dw)) {
        schedule_delayed_work(dw, msecs_to_jiffies(AML_FW_TRACE_CHECK_INT_MS));
    }
}

/**
 * aml_fw_trace_buf_lock() - Lock trace buffer for firmware
 *
 * @shared_buf: Pointer to shared buffer
 *
 * Very basic synchro mechanism so that fw do not update trace buffer while host
 * is reading it. Not safe to race condition if host and fw read lock value at
 * the "same" time.
 */
static void aml_fw_trace_buf_lock(struct aml_fw_trace_buf *shared_buf)
{
  wait:
    while (*shared_buf->lock == AML_FW_TRACE_LOCKED) {}
    *shared_buf->lock &= AML_FW_TRACE_LOCKED_HOST;

    /* re-read to reduce race condition window */
    if (*shared_buf->lock == AML_FW_TRACE_LOCKED)
        goto wait;
}

/**
 * aml_fw_trace_buf_unlock() - Unlock trace buffer for firmware
 *
 * @shared_buf: Pointer to shared buffer
 *
 */
static void aml_fw_trace_buf_unlock(struct aml_fw_trace_buf *shared_buf)
{
    *shared_buf->lock = AML_FW_TRACE_READY;
}

/**
 * aml_fw_trace_buf_init() - Initialize aml_fw_trace_buf structure
 *
 * @shared_buf: Structure to initialize
 * @ipc: Pointer to IPC shard structure that contains trace buffer info
 *
 *
 * Return: 0 if initialization succeed, <0 otherwise. It can only fail if
 * trace feature is not enabled in the firmware (or buffer is corrupted).
 */
int aml_fw_trace_buf_init(struct aml_fw_trace_buf *shared_buf,
                           struct aml_fw_trace_ipc_desc *ipc)
{
    uint16_t lock_status = ipc->pattern;

    if ((lock_status != AML_FW_TRACE_READY &&
         lock_status != AML_FW_TRACE_LOCKED)) {
        shared_buf->data = NULL;
        return -ENOENT;
    }

    /* Buffer starts <offset> bytes from the location of ipc->offset */
    shared_buf->data = (uint16_t *)((uint8_t *)(&ipc->offset) + ipc->offset);
    shared_buf->lock = &ipc->pattern;
    shared_buf->size = ipc->size;
    shared_buf->start = &ipc->start;
    shared_buf->end = &ipc->end;
    shared_buf->reset_idx = ++trace_last_reset;

    /* backward compatibilty with firmware without trace activation */
    if ((ipc->nb_compo >> 16) == AML_FW_TRACE_READY) {
        shared_buf->nb_compo = (ipc->nb_compo & 0xffff) + 6;
        shared_buf->compo_table = (uint32_t *)((uint8_t *)(&ipc->offset_compo)
                                               + ipc->offset_compo);
    } else {
        shared_buf->nb_compo = 0;
        shared_buf->compo_table = NULL;
    }

    return 0;
}

/**
 * aml_fw_trace_init() - Initialize aml_fw_trace structure
 *
 * @trace: Structure to initialize
 * @ipc: Pointer to IPC shard structure that contains trace buffer info
 *
 * Return: 0 if initialization succeed, <0 otherwise. It can only fail if
 * trace feature is not enabled in the firmware (or buffer is corrupted).
 */
int aml_fw_trace_init(struct aml_fw_trace *trace,
                       struct aml_fw_trace_ipc_desc *ipc)
{
    if (aml_fw_trace_buf_init(&trace->buf, ipc))
        return -ENOENT;

    INIT_DELAYED_WORK(&trace->work, aml_fw_trace_work);
    init_waitqueue_head(&trace->queue);
    mutex_init(&trace->mutex);
    trace->closing = false;
    return 0;
}

/**
 * aml_fw_trace_deinit() - De-initialization before releasing aml_fw_trace
 *
 * @trace: fw trace control structure
 */
void aml_fw_trace_deinit(struct aml_fw_trace *trace)
{
    trace->closing = true;
    if (trace->buf.data)
        flush_delayed_work(&trace->work);
    trace->buf.data = NULL;
}

/**
 * aml_fw_trace_reset_local() - Reset local buffer pointer/status
 *
 * @local_buf: structure to reset
 */
static void aml_fw_trace_reset_local(struct aml_fw_trace_local_buf *local_buf)
{
    local_buf->read = local_buf->data;
    local_buf->write = local_buf->data;
    local_buf->nb_entries = 0;
    local_buf->free_space = local_buf->size;
    local_buf->last_read = NULL;
    local_buf->reset_idx = 0;
    local_buf->show_reset = NULL;
}

/**
 * aml_fw_trace_alloc_local() - Allocate a local buffer and initialize
 * aml_fw_trace_local_buf structure
 *
 * @local_buf: structure to initialize
 * @size: Size of the buffer to allocate
 *
 * @local structure is initialized to use the allocated buffer.
 *
 * Return: 0 if allocation succeed and <0 otherwise.
 */
int aml_fw_trace_alloc_local(struct aml_fw_trace_local_buf *local_buf,
                              int size)
{
    local_buf->data = kmalloc(size * sizeof(uint16_t), GFP_KERNEL);
    if (!local_buf->data) {
        return -ENOMEM;
    }

    local_buf->data_end = local_buf->data + size;
    local_buf->size = size;
    aml_fw_trace_reset_local(local_buf);
    return 0;
}

/**
 * aml_fw_trace_free_local() - Free local buffer
 *
 * @local_buf: structure containing buffer pointer to free.
 */
void aml_fw_trace_free_local(struct aml_fw_trace_local_buf *local_buf)
{
    if (local_buf->data)
        kfree(local_buf->data);
    local_buf->data = NULL;
}

/**
 * aml_fw_trace_strlen() - Return buffer size needed convert a trace entry into
 * string
 *
 * @entry: Pointer on trace entry
 *
 */
static inline int aml_fw_trace_strlen(uint16_t *entry)
{
    return (AML_FW_TRACE_HEADER_ASCII_LEN +
            (AML_FW_TRACE_NB_PARAM(entry) * AML_FW_TRACE_PARAM_ASCII_LEN) +
            1); /* for \n */
}

/**
 * aml_fw_trace_to_str() - Convert one trace entry to a string
 *
 * @trace: Poitner to the trace entry
 * @buf: Buffer for the string
 * @size: Size of the string buffer, updated with the actual string size
 *
 * Return: pointer to the next tag entry.
 */
static uint16_t *aml_fw_trace_to_str(uint16_t *trace, char *buf, size_t *size)
{
    uint32_t ts, id;
    int nb_param;
    int res, buf_idx = 0, left = *size;

    id = AML_FW_TRACE_ID(trace);
    nb_param = AML_FW_TRACE_NB_PARAM(trace);

    trace +=2;
    ts = *trace++;
    ts <<= 16;
    ts += *trace++;

    res = scnprintf(&buf[buf_idx], left, AML_FW_TRACE_HEADER_FMT, ts, id);
    buf_idx += res;
    left    -= res;

    while (nb_param > 0) {
        res = scnprintf(&buf[buf_idx], left, AML_FW_TRACE_PARAM_FMT, *trace++);
        buf_idx += res;
        left    -= res;
        nb_param--;
    }

    res = scnprintf(&buf[buf_idx], left, "\n");
    left -= res;
    *size = (*size - left);

    return trace;
}

/**
 * aml_fw_trace_copy_entry() - Copy one trace entry in a local buffer
 *
 * @local_buf: Local buffer to copy trace into
 * @trace_entry: Pointer to the trace entry (in shared memory) to copy
 * @size: Size, in 16bits words, of the trace entry
 *
 * It is assumed that local has enough contiguous free-space available in
 * local buffer (i.e. from local_buf->write) to copy this trace.
 */
static void aml_fw_trace_copy_entry(struct aml_fw_trace_local_buf *local_buf,
                                     uint16_t *trace_entry, int size)
{
    uint16_t *write = local_buf->write;
    uint16_t *read = trace_entry;
    int i;

    for (i = 0; i < size; i++) {
        *write++ = *read++;
    }

    if (write >= local_buf->data_end)
        local_buf->write = local_buf->data;
    else
        local_buf->write = write;

    local_buf->free_space -= size;
    local_buf->last_read = trace_entry;
    local_buf->last_read_value = *trace_entry;
    local_buf->nb_entries++;
}

/**
 * aml_fw_trace_copy() - Copy trace entries from shared to local buffer
 *
 * @trace_buf: Pointer to shard buffer
 * @local_buf: Pointer to local buffer
 *
 * Copy has many trace entry as possible from shared buffer to local buffer
 * without overwriting traces in local buffer.
 *
 * Return: number of trace entries copied to local buffer
 */
static int aml_fw_trace_copy(struct aml_fw_trace *trace,
                              struct aml_fw_trace_local_buf *local_buf)
{
    struct aml_fw_trace_buf *trace_buf = &trace->buf;
    uint16_t *ptr, *ptr_end, *ptr_limit;
    int entry_size, ret = 0;

    if (mutex_lock_interruptible(&trace->mutex))
        return 0;

    /* reset last_read ptr if shared buffer has been reset */
    if (local_buf->reset_idx != trace_buf->reset_idx) {
        local_buf->show_reset = local_buf->write;
        local_buf->reset_idx = trace_buf->reset_idx;
        local_buf->last_read = NULL;
    }

    aml_fw_trace_buf_lock(trace_buf);

    ptr_end = trace_buf->data + *trace_buf->end;
    if (aml_fw_trace_empty(trace_buf) || (ptr_end == local_buf->last_read))
        goto end;
    ptr_limit = trace_buf->data + trace_buf->size;

    if (local_buf->last_read &&
        (local_buf->last_read_value == *local_buf->last_read)) {
        ptr = local_buf->last_read;
        ptr += AML_FW_TRACE_ENTRY_SIZE(ptr);
    } else {
        ptr = trace_buf->data + *trace_buf->start;
    }

    while (1) {

        if ((ptr == ptr_limit) || (*ptr == AML_FW_TRACE_LAST_ENTRY))
             ptr = trace_buf->data;

        entry_size = AML_FW_TRACE_ENTRY_SIZE(ptr);

        if ((ptr + entry_size) > ptr_limit) {
            pr_err("Corrupted trace buffer\n");
            _aml_fw_trace_reset(trace, false);
            break;
        } else if (entry_size > local_buf->size) {
            pr_err("FW_TRACE local buffer too small, trace skipped");
            goto next_entry;
        }

        if (local_buf->free_space >= entry_size) {
            int contiguous = local_buf->data_end - local_buf->write;

            if ((local_buf->write < local_buf->read) || contiguous >= entry_size) {
                /* enough contiguous memory from local_buf->write */
                aml_fw_trace_copy_entry(local_buf, ptr, entry_size);
                ret++;
            } else if ((local_buf->free_space - contiguous) >= entry_size) {
                /* not enough contiguous from local_buf->write but enough
                   from local_buf->data */
                *local_buf->write = AML_FW_TRACE_LAST_ENTRY;
                if (local_buf->show_reset == local_buf->write)
                    local_buf->show_reset = local_buf->data;
                local_buf->write = local_buf->data;
                local_buf->free_space -= contiguous;
                aml_fw_trace_copy_entry(local_buf, ptr, entry_size);
                ret++;
            } else {
                /* not enough contiguous memory */
                goto end;
            }
        } else {
            goto end;
        }

        if (ptr == ptr_end)
            break;

      next_entry:
        ptr += entry_size;
    }

  end:
    aml_fw_trace_buf_unlock(trace_buf);
    mutex_unlock(&trace->mutex);
    return ret;
}

/**
 * aml_fw_trace_read_local() - Read trace from local buffer and convert it to
 * string in a user buffer
 *
 * @local_buf: Pointer to local buffer
 * @user_buf: Pointer to user buffer
 * @size: Size of the user buffer
 *
 * Read traces from shared buffer to write them in the user buffer after string
 * conversion. Stop when no more space in user buffer or no more trace to read.
 *
 * Return: The size written in the user buffer.
 */
static size_t aml_fw_trace_read_local(struct aml_fw_trace_local_buf *local_buf,
                                       char __user *user_buf, size_t size)
{
    uint16_t *ptr;
    char str[1824] = {0}; // worst case 255 params
    size_t str_size;
    int entry_size;
    size_t res = 0 , remain = size, not_cpy = 0;

    if (!local_buf->nb_entries)
        return res;

    ptr = local_buf->read;
    while(local_buf->nb_entries && !not_cpy) {

        if (local_buf->show_reset == ptr) {
            if (remain < AML_FW_TRACE_RESET_SIZE)
                break;

            local_buf->show_reset = NULL;
            not_cpy = copy_to_user(user_buf + res, AML_FW_TRACE_RESET,
                                   AML_FW_TRACE_RESET_SIZE);
            res += (AML_FW_TRACE_RESET_SIZE - not_cpy);
            remain -= (AML_FW_TRACE_RESET_SIZE - not_cpy);
        }

        if (remain < aml_fw_trace_strlen(ptr))
            break;

        entry_size = AML_FW_TRACE_ENTRY_SIZE(ptr);
        str_size = sizeof(str);
        ptr = aml_fw_trace_to_str(ptr, str, &str_size);
        not_cpy = copy_to_user(user_buf + res, str, str_size);
        str_size -= not_cpy;
        res += str_size;
        remain -= str_size;

        local_buf->nb_entries--;
        local_buf->free_space += entry_size;
        if (ptr >= local_buf->data_end) {
            ptr = local_buf->data;
        } else if (*ptr == AML_FW_TRACE_LAST_ENTRY) {
            local_buf->free_space += local_buf->data_end - ptr;
            ptr = local_buf->data;
        }
        local_buf->read = ptr;
    }

    /* read all entries reset pointer */
    if ( !local_buf->nb_entries) {

        local_buf->write = local_buf->read = local_buf->data;
        local_buf->free_space = local_buf->size;
    }

    return res;
}

/**
 * aml_fw_trace_read() - Update local buffer from shared buffer and convert
 * local buffer to string in user buffer
 *
 * @trace: Fw trace control structure
 * @local_buf: Local buffer to update and read from
 * @dont_wait: Indicate whether function should wait or not for traces before
 * returning
 * @user_buf: Pointer to user buffer
 * @size: Size of the user buffer
 *
 * Read traces from shared buffer to write them in the user buffer after string
 * conversion. Stop when no more space in user buffer or no more trace to read.
 *
 * Return: The size written in the user buffer if > 0, -EAGAIN if there is no
 * new traces and dont_wait is set and -ERESTARTSYS if signal has been
 * received while waiting for new traces.
 */
size_t aml_fw_trace_read(struct aml_fw_trace *trace,
                          struct aml_fw_trace_local_buf *local_buf,
                          bool dont_wait, char __user *user_buf, size_t size)
{
    size_t res = 0;

    aml_fw_trace_copy(trace, local_buf);

    while(!local_buf->nb_entries) {
        int last_index;

        if (dont_wait)
            return -EAGAIN;

        /* no trace, schedule work to periodically check trace buffer */
        if (!delayed_work_pending(&trace->work)) {
            trace->last_read_index = *trace->buf.end;
            schedule_delayed_work(&trace->work,
                                  msecs_to_jiffies(AML_FW_TRACE_CHECK_INT_MS));
        }

        /* and wait for traces */
        last_index = *trace->buf.end;
        if (wait_event_interruptible(trace->queue,
                                     (trace->closing ||
                                      (last_index != *trace->buf.end)))) {
            return -ERESTARTSYS;
        }

        if (trace->closing)
            return 0;

        aml_fw_trace_copy(trace, local_buf);
    }

    /* copy as many traces as possible in user buffer */
    while (1) {
        size_t read;
        read = aml_fw_trace_read_local(local_buf, user_buf + res, size - res);
        res += read;
        aml_fw_trace_copy(trace, local_buf);
        if (!read)
            break;
    }

    return res;
}


/**
 * _aml_fw_trace_dump() - Dump shared trace buffer in kernel buffer
 *
 * @trace_buf: Pointer to shared trace buffer;
 *
 * Called when error is detected, output trace on dmesg directly read from
 * shared memory
 */
void _aml_fw_trace_dump(struct aml_hw *aml_hw, struct aml_fw_trace_buf *trace_buf)
{
    uint16_t *ptr = NULL, *ptr_end = NULL, *ptr_limit = NULL, *next_ptr, *ptr_flag;
    char *buf = NULL;
    int buf_size = 1824;
    size_t size;

    if ((buf = vmalloc(buf_size)) == NULL)
        return;

    if (!trace_buf->data || aml_fw_trace_empty(trace_buf)) {
        vfree(buf);
        return;
    }

    aml_fw_trace_buf_lock(trace_buf);

    if (aml_bus_type == PCIE_MODE) {
        ptr_flag = trace_buf->data;
        ptr = trace_buf->data + *trace_buf->start;
    } else {
        ptr = kmalloc(28*1024, GFP_DMA | GFP_ATOMIC);
        ptr_flag = ptr;
        aml_trace_buf_init();
        if (aml_bus_type == USB_MODE) {
            aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)ptr, (unsigned char *)(SYS_TYPE)USB_TRACE_START_ADDR, USB_TRACE_TOTAL_SIZE, USB_EP4);
        } else if (aml_bus_type == SDIO_MODE) {
            aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)ptr, (unsigned char *)(SYS_TYPE)SDIO_TRACE_START_ADDR, SDIO_TRACE_TOTAL_SIZE);
        }
        ptr += *trace_buf->start;
    }

    ptr_end = ptr_flag + *trace_buf->end;
    ptr_limit = ptr_flag + trace_buf->size;

    if (aml_bus_type == PCIE_MODE) {
        while (1) {
            size = buf_size;
            next_ptr = aml_fw_trace_to_str(ptr, buf, &size);
            pr_info("%s", buf);

            if (ptr == ptr_end) {
                break;
            } else if ((next_ptr == ptr_limit) ||
                       (*next_ptr == AML_FW_TRACE_LAST_ENTRY)) {
                ptr = trace_buf->data;
            } else if (next_ptr > ptr_limit) {
                pr_err("Corrupted trace buffer\n");
                break;
            } else {
                ptr = next_ptr;
            }
        }
    }else {
        aml_trace_log_to_file(ptr, ptr_limit);
        kfree(ptr);
    }

    aml_fw_trace_buf_unlock(trace_buf);

    vfree(buf);
}

/**
 * _aml_fw_trace_reset() - Reset trace buffer at firmware level
 *
 * @trace: Pointer to shared trace buffer;
 * @bool: Indicate if mutex must be aquired before
 */
int _aml_fw_trace_reset(struct aml_fw_trace *trace, bool lock)
{
    struct aml_fw_trace_buf *trace_buf = &trace->buf;

    if (lock && mutex_lock_interruptible(&trace->mutex))
        return -ERESTARTSYS;

    if (trace->buf.data) {
        aml_fw_trace_buf_lock(trace_buf);
        *trace_buf->start = 0;
        *trace_buf->end = trace_buf->size + 1;
        trace_buf->reset_idx = ++trace_last_reset;
        aml_fw_trace_buf_unlock(trace_buf);
    }

    if (lock)
        mutex_unlock(&trace->mutex);
    return 0;
}

/**
 * aml_fw_trace_get_trace_level() - Get trace level for a given component
 *
 * @trace: Pointer to shared trace buffer;
 * @compo_id: Index of the componetn in the table
 *
 * Return: The trace level set for the given component, 0 if component index
 * is invalid.
 */
static uint32_t aml_fw_trace_level_for_read_or_write(struct aml_fw_trace_buf *trace_buf,
                                            unsigned int compo_id, uint32_t level, int mode)
{
    struct aml_hif_sdio_ops *hif_ops = &g_hif_sdio_ops;
    struct auc_hif_ops *hif_ops_usb = &g_auc_hif_ops;
    struct aml_hw *aml_hw;
    struct aml_plat *aml_plat;

    if (mode == TRACE_LEVEL_WRITE) {
        if (compo_id < 14) {
            if (aml_bus_type == USB_MODE) {
                hif_ops_usb->hi_write_word((TRACE_COMPO_LEVEL + compo_id * 4), level, USB_EP4);
            } else if (aml_bus_type == SDIO_MODE) {
                hif_ops->hi_random_word_write((TRACE_COMPO_LEVEL + compo_id * 4), level);
            } else {
                trace_buf->compo_table[compo_id] = level;
            }
        }
        else {
            if (aml_bus_type == USB_MODE) {
                hif_ops_usb->hi_write_word((TRACE_LEVEL + (compo_id - 14) * 4), level, USB_EP4);
            } else if (aml_bus_type == SDIO_MODE) {
                hif_ops->hi_random_word_write((TRACE_LEVEL + (compo_id - 14) * 4), level);
            } else {
                aml_hw = pci_get_drvdata(g_pci_dev);
                aml_plat = aml_hw->plat;
                aml_reg_write(level, aml_plat, 0, ( TRACE_LEVEL + (compo_id - 14) * 4));
            }
        }
    }
    else if (mode == TRACE_LEVEL_READ) {
        if (compo_id < 14) {
            if (aml_bus_type == USB_MODE) {
                level = hif_ops_usb->hi_read_word(TRACE_COMPO_LEVEL + compo_id * 4, USB_EP4);
            } else if (aml_bus_type == SDIO_MODE) {
                level = hif_ops->hi_random_word_read(TRACE_COMPO_LEVEL + compo_id * 4);
            } else {
                level = trace_buf->compo_table[compo_id];
            }
        }
        else {
            if (aml_bus_type == USB_MODE) {
                level = hif_ops_usb->hi_read_word(TRACE_LEVEL + (compo_id - 14) * 4, USB_EP4);
            } else if (aml_bus_type == SDIO_MODE) {
                level = hif_ops->hi_random_word_read(TRACE_LEVEL + (compo_id - 14) * 4);
            } else {
                aml_hw = pci_get_drvdata(g_pci_dev);
                aml_plat = aml_hw->plat;
                level = aml_reg_read(aml_plat, 0, ( TRACE_LEVEL + (compo_id - 14) * 4));
            }
        }
        return level;
    }

    return 0;
}

static uint32_t aml_fw_trace_get_trace_level(struct aml_fw_trace_buf *trace_buf,
                                              unsigned int compo_id)
{
    uint32_t level = 0;

    if (compo_id >= trace_buf->nb_compo)
        return 0;

    level = aml_fw_trace_level_for_read_or_write(trace_buf, compo_id, level, TRACE_LEVEL_READ);
    return level;
}

/**
 * aml_fw_trace_set_trace_level() - Set trace level for a given component
 *
 * @trace_buf: Pointer to shared trace buffer;
 * @compo_id: Index of the componetn in the table
 * @level: Trace level to set
 *
 * Set all components if compo_id is equals to the number of component and
 * does nothing if it is greater.
 */
static void aml_fw_trace_set_trace_level(struct aml_fw_trace_buf *trace_buf,
                                          unsigned int compo_id, uint32_t level)
{
    if (compo_id > trace_buf->nb_compo)
        return;

    if (compo_id == trace_buf->nb_compo) {
        int i;
        for (i = 0; i < trace_buf->nb_compo; i++) {
            aml_fw_trace_level_for_read_or_write(trace_buf, i, level, TRACE_LEVEL_WRITE);
        }
    } else {
        aml_fw_trace_level_for_read_or_write(trace_buf, compo_id, level, TRACE_LEVEL_WRITE);
    }
}

/**
 * aml_fw_trace_level_read() - Write current trace level in a user buffer
 *                              as a string
 *
 * @trace: Fw trace control structure
 * @user_buf: Pointer to user buffer
 * @len: Size of the user buffer
 * @ppos: position offset
 *
 * Return: Number of bytes written in user buffer if > 0, error otherwise
 */
size_t aml_fw_trace_level_read(struct aml_fw_trace *trace,
                                char __user *user_buf, size_t len, loff_t *ppos)
{
    struct aml_fw_trace_buf *trace_buf = &trace->buf;
    size_t res = 0;
    int i, size;
    char *buf;

    trace_buf->nb_compo = 20;

    size = trace_buf->nb_compo * 16;
    buf = kmalloc(size, GFP_KERNEL);
    if (buf == NULL)
        return 0;

    if (mutex_lock_interruptible(&trace->mutex)) {
        kfree(buf);
        return -ERESTARTSYS;
    }

    for (i = 0 ; i < trace_buf->nb_compo ; i ++) {
        res += scnprintf(&buf[res], size - res, "%3d:0x%08x\n", i,
                         aml_fw_trace_get_trace_level(trace_buf, i));
    }
    mutex_unlock(&trace->mutex);

    res = simple_read_from_buffer(user_buf, len, ppos, buf, res);

    kfree(buf);
    return res;
}

/**
 * aml_fw_trace_level_write() - Read trace level from  a user buffer provided
 *                               as a string and apply them.
 *
 * @trace: Fw trace control structure
 * @user_buf: Pointer to user buffer
 * @len: Size of the user buffer
 *
 * trace level must be provided in the following form:
 * <compo_id>:<trace_level> where <compo_id> is in decimal notation and
 * <trace_level> in decical or hexadecimal notation.
 * Several trace level can be provided, separated by space,tab or new line.
 *
 * Return: Number of bytes read form user buffer if > 0, error otherwise
 */
size_t aml_fw_trace_level_write(struct aml_fw_trace *trace,
                                 const char __user *user_buf, size_t len)
{
    struct aml_fw_trace_buf *trace_buf = &trace->buf;
    char *buf, *token, *next;
    buf = kmalloc(len + 1, GFP_KERNEL);
    if (buf == NULL)
        return -ENOMEM;

    if (copy_from_user(buf, user_buf, len)) {
        kfree(buf);
        return -EFAULT;
    }
    buf[len] = '\0';

    if (mutex_lock_interruptible(&trace->mutex)) {
        kfree(buf);
        return -ERESTARTSYS;
    }

    trace_buf->nb_compo = 20;

    next = buf;
    token = strsep(&next, " \t\n");
    while (token) {
        unsigned int compo, level;
        if ((sscanf(token, "%d:0x%x", &compo, &level) == 2)||
            (sscanf(token, "%d:%d", &compo, &level) == 2)) {
            aml_fw_trace_set_trace_level(trace_buf, compo, level);
        }

        token = strsep(&next, " \t");
    }
    mutex_unlock(&trace->mutex);

    kfree(buf);
    return len;
}

/**
 * aml_fw_trace_config_filters() - Update FW trace filters
 *
 * @trace_buf: Pointer to shared buffer
 * @ipc: Pointer to IPC shared structure that contains trace buffer info
 * @ftl: Firmware trace level
 *
 * Return: 0 if the trace filters are successfully updated, <0 otherwise.
 */
int aml_fw_trace_config_filters(struct aml_fw_trace_buf *trace_buf,
                                 struct aml_fw_trace_ipc_desc *ipc, char *ftl)
{
    int to;
    char *next, *token;

    to = 0;
    while ((ipc->pattern != AML_FW_TRACE_READY) && (to < startup_max_to))
    {
        msleep(50);
        to += 50;
    }

    if (aml_fw_trace_buf_init(trace_buf, ipc))
        return -ENOENT;

    next = ftl;
    token = strsep(&next, " ");
    while(token)
    {
        unsigned int compo, ret, id, level = 0;
        char action;

        if ((sscanf(token, "%d%c0x%x", &compo, &action, &id) == 3)||
            (sscanf(token, "%d%c%d", &compo, &action, &id) == 3))
        {
            if(action == '=')
            {
                level = id;
            }
            else
            {
                ret = aml_fw_trace_get_trace_level(trace_buf, compo);
                if(action == '+')
                    level = (ret | id);
                else if (action == '-')
                    level = (ret & ~id);
            }
            aml_fw_trace_set_trace_level(trace_buf, compo, level);
        }

        token = strsep(&next, " ");
    }

    return 0;
}

/**
 * aml_fw_trace_save_filters() - Save filters currently configured so that
 * they can be restored with aml_fw_trace_restore_filters()
 *
 * @trace: Fw trace control structure
 * @return 0 if filters have been saved and != 0 in case on error
 */
int aml_fw_trace_save_filters(struct aml_fw_trace *trace)
{
    int i;

    if (saved_filters)
        kfree(saved_filters);

    saved_filters_cnt = trace->buf.nb_compo;
    saved_filters = kmalloc(saved_filters_cnt * sizeof(uint32_t), GFP_KERNEL);
    if (!saved_filters)
        return -1;

    for (i = 0; i < saved_filters_cnt; i++) {
        saved_filters[i] = aml_fw_trace_get_trace_level(&trace->buf, i);
    }

    return 0;
}

/**
 * aml_fw_trace_restore_filters() - Restore filters previoulsy saved
 * by aml_fw_trace_save_filters()
 *
 * @trace: Fw trace control structure
 * @return 0 if filters have been restored and != 0 in case on error
 */
int aml_fw_trace_restore_filters(struct aml_fw_trace *trace)
{
    int i;

    if (!saved_filters || (trace->buf.data == NULL))
        return -1;

    if (saved_filters_cnt != trace->buf.nb_compo) {
        pr_warn("Number of trace components change between saved and restore\n");
        if (saved_filters_cnt > trace->buf.nb_compo) {
            saved_filters_cnt = trace->buf.nb_compo;
        }
    }

    for (i = 0; i < saved_filters_cnt; i++) {
        aml_fw_trace_set_trace_level(&trace->buf, i, saved_filters[i]);
    }

    kfree(saved_filters);
    saved_filters = NULL;
    saved_filters_cnt = 0;

    return 0;
}

int aml_trace_buf_init(void)
{
    int ret = 0;
    static int isInit = 0;

    if (aml_bus_type != PCIE_MODE) {
        if (!isInit) {
            AML_INFO("trace mutex init");
            mutex_init(&trace_log_file_info.mutex);
            isInit = 1;
        }

        mutex_lock(&trace_log_file_info.mutex);
        if (!trace_log_file_info.log_buf) {
            trace_log_file_info.log_buf = kmalloc(64 * 1024, GFP_DMA | GFP_ATOMIC);
            if (!trace_log_file_info.log_buf) {
                AML_INFO("alloc memory failed");
                ret = -1;
            }
        }

        if (!trace_log_file_info.ptr) {
            trace_log_file_info.ptr = kmalloc(33 * 1024, GFP_DMA | GFP_ATOMIC);
            if (!trace_log_file_info.ptr) {
                AML_INFO("alloc memory failed");
                ret = -1;
            }
        }
        mutex_unlock(&trace_log_file_info.mutex);
    }

    return ret;
}

void aml_trace_buf_deinit(void)
{
    if (trace_log_file_info.log_buf) {
        kfree(trace_log_file_info.log_buf);
        trace_log_file_info.log_buf = NULL;
    }

    if (trace_log_file_info.ptr) {
        kfree(trace_log_file_info.ptr);
        trace_log_file_info.ptr = NULL;
    }

    return;
}

loff_t aml_get_file_size(const char *filename)
{
    struct file *fp = NULL;
    struct kstat st;
#ifndef CONFIG_PT_MODE
    int ret = 0;
#endif

    fp = FILE_OPEN(filename, O_CREAT | O_RDONLY, 0644);
    if (IS_ERR(fp)) {
        printk("%s: %s open failed: PTR_ERR(fp) = %d\n", __func__, filename, PTR_ERR(fp));
        if (PTR_ERR(fp) == -13) {
            printk("/* Permission denied */");
        }
        goto err;
    }

#ifndef CONFIG_PT_MODE
    ret = FILE_STAT(filename, &st);
    if (ret < 0) {
        FILE_CLOSE(fp, NULL);
        printk("%s: file stat error, err = %d\n", __func__, ret);
        goto err;
    }
#endif

    if ((long long)st.size < 0) {
        FILE_CLOSE(fp, NULL);
        goto err;
    }

    FILE_CLOSE(fp, NULL);
    return st.size;
err:
    return (loff_t)(-1);
}


int aml_trace_log_to_file(uint16_t *trace, uint16_t *trace_limit)
{
    struct file *fp = NULL;
    loff_t file_size = 0;
    unsigned int file_mode;
    char str[1824] = {0};
    unsigned int offset = 0;
    size_t str_size;
    int sock_wr_len = 0;

    if (!trace_log_file_info.log_buf)
        goto err;

    memset(trace_log_file_info.log_buf, 0, 64*1024);

    while (1) {
        str_size = sizeof(str);
        memset(str, 0, str_size);
        trace = aml_fw_trace_to_str(trace, str, &str_size);
        memcpy(trace_log_file_info.log_buf + offset, str, str_size);
        offset += str_size;

        if (trace >= trace_limit || *trace == AML_FW_TRACE_LAST_ENTRY) {
            break;
        }
    }
    if (g_trace_nl_info.enable) {
        do {
            if (offset > 16 *1024) {
                aml_send_log_to_user(trace_log_file_info.log_buf + sock_wr_len, 16 *1024, AML_TRACE_FW_LOG_UPLOAD);
                sock_wr_len += 16 *1024;
                offset -= 16 *1024;
            } else {
                aml_send_log_to_user(trace_log_file_info.log_buf + sock_wr_len, offset, AML_TRACE_FW_LOG_UPLOAD);
                sock_wr_len += offset;
                offset = 0;
            }
        } while (offset > 0);
    }

    return 0;
err:
    return -1;
}

// recv msg handl function
static void aml_recv_netlink(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct log_nl_msg_info * nl_log_info = NULL;
    nlh = nlmsg_hdr(skb); // get msg body
    AML_INFO("kernel rcv msg type: %d, pid: %d, len: %d, flag: %d, seq: %d\n",
        nlh->nlmsg_type, nlh->nlmsg_pid, nlh->nlmsg_len, nlh->nlmsg_flags, nlh->nlmsg_seq);
    AML_INFO("receive data from user process: %s", (char *)NLMSG_DATA(nlh));

    nl_log_info = (struct nl_log_info*)NLMSG_DATA(nlh);
    switch (nl_log_info->msg_type) {
        case AML_TRACE_FW_LOG_START:
            g_trace_nl_info.user_pid = nlh->nlmsg_pid;
            g_trace_nl_info.enable = 1;
            AML_INFO("user space process (pid: %d) start recv fw log !!!!\n", g_trace_nl_info.user_pid);
            break;
        case AML_TRACE_FW_LOG_STOP:
            g_trace_nl_info.enable = 0;
            AML_INFO("user space process (pid: %d) stop recv fw log !!!!\n", g_trace_nl_info.user_pid);
            break;
        default:
            AML_INFO("unknown msg (0x%x) from user space process (pid: %d), ignore !!!!\n",
                nl_log_info->msg_type, g_trace_nl_info.user_pid);
            break;
    }

    return;
}

int aml_log_nl_init(void)
{

    memset(&g_trace_nl_info, 0, sizeof(struct log_nl_msg_info));
    struct netlink_kernel_cfg cfg = {
        .input = aml_recv_netlink,
    };
    g_trace_nl_info.fw_log_sock = netlink_kernel_create(&init_net, AML_TRACE_NL_PROTOCOL, &cfg);
    if (!g_trace_nl_info.fw_log_sock) {
        AML_INFO("aml trace netlink init failed");
        return -1;
    }

    AML_INFO("aml trace netlink init OK!\n");
    return 0;
}
void aml_log_nl_destroy(void)
{
    if (g_trace_nl_info.fw_log_sock) {
        netlink_kernel_release(g_trace_nl_info.fw_log_sock);
    }
    AML_INFO("fw log upload socket destroy!!!\n");

    return;
}
int aml_send_log_to_user(char *pbuf, uint16_t len, int msg_type)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh = NULL;   //msg head
    struct log_nl_msg_info * nl_log_info = NULL;
    int ret;
    static int seq_num = 0;
    int buf_len = NLMSG_SPACE(len + sizeof(struct log_nl_msg_info));

    if (!g_trace_nl_info.fw_log_sock || !g_trace_nl_info.user_pid ||
        !g_trace_nl_info.enable) {
        AML_INFO("kernel trace nl sock para invalid , can not upload msg to user\n");
        return -1;
    }
    //create sk_buff
    nl_skb = nlmsg_new(buf_len, GFP_ATOMIC);
    if (!nl_skb)
    {
        AML_INFO("netlink alloc failure\n");
        return -1;
    }

    /* build netlink msg head */
    nlh = nlmsg_put(nl_skb, 0, 0, AML_TRACE_NL_PROTOCOL, buf_len, 0);
    if (nlh == NULL)
    {
        AML_INFO("nlmsg_put failure\n");
        nlmsg_free(nl_skb);
        return -1;
    }
    NETLINK_CB(nl_skb).portid = 0;
    NETLINK_CB(nl_skb).dst_group = 0;
    nl_log_info = (struct nl_log_info*)nlmsg_data(nlh);
    nl_log_info->msg_len = len;
    nl_log_info->msg_type = msg_type;
    nlh->nlmsg_seq = seq_num++;

    /* copy data and send it */
    if (pbuf) {
        memcpy(nlmsg_data(nlh) + sizeof(struct log_nl_msg_info), pbuf, len);
    }
    ret = netlink_unicast(g_trace_nl_info.fw_log_sock, nl_skb, g_trace_nl_info.user_pid, 0);

   // AML_INFO("==== kernel upload msg to user result: %d, seq: %d\n", ret, seq_num - 1);
    return ret;

}

void aml_send_err_info_to_diag(char *pbuf, int len)
{
    struct file *fp = NULL;
    loff_t file_size = 0;
    unsigned int file_mode;

    if (!trace_log_file_info.log_buf || !trace_log_file_info.ptr) {
        return;
    }

    mutex_lock(&trace_log_file_info.mutex);
    if (g_trace_nl_info.enable && len > 0) {
        aml_send_log_to_user(pbuf, len, AML_TRACE_FW_LOG_UPLOAD);
    }
    mutex_unlock(&trace_log_file_info.mutex);
}
