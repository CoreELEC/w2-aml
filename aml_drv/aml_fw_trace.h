/**
 ******************************************************************************
 *
 * aml_fw_trace.h
 *
 * Copyright (C) Amlogic 2017-2021
 *
 ******************************************************************************
 */

#ifndef _AML_FW_TRACE_H_
#define _AML_FW_TRACE_H_

#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define MAX_FILE_SIZE            (4 * 1024 * 1024)
#define FILE_SIZE_UNLIMIT_FLAG   0xFFFF
#define TRACE_LEVEL_READ         1
#define TRACE_LEVEL_WRITE        0

/**
 * struct aml_fw_trace_desc - Trace buffer info as provided by fw in ipc
 *
 * @pattern: Synchro pattern
 * @start: Index of first entry in trace buffer
 * @end: Index of last entry in trace buffer
 * @size: Size, in 16bits words, od the trace buffer
 * @offset: Offset, in bytest, to the start of the buffer from the address of
 * this field.
 * @nb_compo: Number of filterable component (16LSB) synchro pattern (16 MSB)
 * @offset_compo: Offset, in bytest, to the start of the component activation
 * table from the address of this field.
 */
struct aml_fw_trace_ipc_desc {
    volatile uint16_t pattern;
    volatile uint32_t start;
    volatile uint32_t end;
    volatile uint32_t size;
    volatile uint32_t offset;
    volatile uint32_t nb_compo;
    volatile uint32_t offset_compo;
};

/**
 * struct aml_fw_trace_buf - Info for trace buffer in shared memory
 *
 * @lock: Address of the synchro word
 * @data: Address of the trace buffer
 * @size: Size, in 16bits words, of the trace buffer
 * @start: Address on the current index (in 16 bits words) of the first trace
 * entry.
 * @end: Address on the current index (in 16 bits words) of the last trace
 * entry. If *end > size, it means that the trace buffer contains no traces.
 * @reset_idx: Increased each time the trace buffer is reset
 * (by calling _aml_fw_trace_reset())
 * @nb_compo: Size of the compo_table
 * @compo_table: Table containing component filter status.
 */
struct aml_fw_trace_buf {
    volatile uint16_t *lock;
    uint16_t *data;
    uint32_t size;
    volatile uint32_t *start;
    volatile uint32_t *end;
    int reset_idx;
    unsigned int nb_compo;
    uint32_t *compo_table;
};

/**
 * struct aml_fw_trace_local_buf - Info for a local trace buffer
 *
 * @data: Address of the local buffer
 * @data_end: Address after the end of the local buffer
 * @size: Size, in 16bits words, oth the local buffer
 * @read: Pointer to the next trace entry to read
 * @write: Pointer to the next entry to write
 * @nb_entries: Number of trace entries ready to be read
 * @free_space: Free space, in 16bits words, in the buffer.
 * @last_read: Address of the last entry read in the shared buffer
 * @last_read_value: First word of the last trace entry read.
 * @reset_idx: Reset index. If it doesn't match share buffer index then it
 * means that share buffer has been resetted since last read.
 */
struct aml_fw_trace_local_buf {
    uint16_t *data;
    uint16_t *data_end;
    uint32_t size;
    uint16_t *read;
    uint16_t *write;
    uint16_t nb_entries;
    uint32_t free_space;
    uint16_t *last_read;
    uint16_t last_read_value;
    int reset_idx;
    uint16_t * show_reset;
};


/**
 * struct aml_fw_trace - info to handle several reader of the shared buffer
 *
 * @buf: shared trace buffer.
 * @mutex: mutex, used to prevent several reader updating shared buffer at the
 * same time.
 * @queue: queue, used to delay reader.
 * @work: work used to periodically check for new traces in shared buffer.
 * @last_read_index: Last read index from shared buffer
 * @closing: Indicate whether is driver is being removed, meaning that reader
 * should no longer wait no new traces
 */
struct aml_fw_trace {
    struct aml_fw_trace_buf buf;
    struct mutex mutex;
    wait_queue_head_t queue;
    struct delayed_work work;
    int last_read_index;
    bool closing;
};

struct log_file_info {
    uint8_t *log_buf;
    uint8_t *ptr;
    struct mutex mutex;
};

struct aml_trace_nl_info {
    struct sock * fw_log_sock;
    int user_pid;
    int enable;
};

struct log_nl_msg_info {
    int msg_type;
    int msg_len;
};

enum {
    AML_TRACE_FW_LOG_START = 0xFF01,
    AML_TRACE_FW_LOG_STOP,
    AML_TRACE_FW_LOG_UPLOAD,
    AML_LA_MACTRACE_UPLOAD,
    AML_MEM_DUMP_UPLOAD,
};

int aml_fw_trace_init(struct aml_fw_trace *trace,
                       struct aml_fw_trace_ipc_desc *ipc);
void aml_fw_trace_deinit(struct aml_fw_trace *trace);

int aml_fw_trace_buf_init(struct aml_fw_trace_buf *shared_buf,
                           struct aml_fw_trace_ipc_desc *ipc);

int _aml_fw_trace_reset(struct aml_fw_trace *trace, bool lock);
void _aml_fw_trace_dump(struct aml_hw *aml_hw, struct aml_fw_trace_buf *trace);

int aml_fw_trace_alloc_local(struct aml_fw_trace_local_buf *local,
                              int size);
void aml_fw_trace_free_local(struct aml_fw_trace_local_buf *local);


size_t aml_fw_trace_read(struct aml_fw_trace *trace,
                          struct aml_fw_trace_local_buf *local_buf,
                          bool dont_wait, char __user *user_buf, size_t size);


size_t aml_fw_trace_level_read(struct aml_fw_trace *trace,
                                char __user *user_buf, size_t len, loff_t *ppos);
size_t aml_fw_trace_level_write(struct aml_fw_trace *trace,
                                 const char __user *user_buf, size_t len);


int aml_fw_trace_config_filters(struct aml_fw_trace_buf *trace_buf,
                                 struct aml_fw_trace_ipc_desc *ipc, char *ftl);

int aml_fw_trace_save_filters(struct aml_fw_trace *trace);
int aml_fw_trace_restore_filters(struct aml_fw_trace *trace);
int aml_trace_buf_init(void);
loff_t aml_get_file_size(const char *filename);
int aml_trace_log_to_file(uint16_t *trace, uint16_t *trace_limit);
void aml_send_err_info_to_diag(char *pbuf, int len);

/**
 * aml_fw_trace_empty() - Check if shared buffer is empty
 *
 * @shared_buf: Pointer to shared buffer
 */
static inline bool aml_fw_trace_empty(struct aml_fw_trace_buf *shared_buf)
{
    return (*shared_buf->end >= shared_buf->size);
}
int aml_log_nl_init(void);
void aml_log_nl_destroy(void);
int aml_send_log_to_user(char *pbuf, uint16_t len, int msg_type);

#endif /* _AML_FW_TRACE_H_ */
