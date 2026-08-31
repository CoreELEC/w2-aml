#include <linux/kernel.h>
#include <linux/slab.h>

#include "aml_static_buf.h"
#include "aml_log.h"

static const struct {
    const char *name;
    size_t size;
} pre_buf_infos[PREALLOC_BUF_TYPE_MAX] = {
#define PREALLOC_INFO(n) [PREALLOC_##n] = { #n, PREALLOC_##n##_SIZE }
    PREALLOC_INFO(BUF_FW_DL),
    PREALLOC_INFO(BUF_BUS),
    PREALLOC_INFO(BUF_TYPE_DUMP),
    PREALLOC_INFO(BUF_TYPE_RXBUF),
    PREALLOC_INFO(BUF_TYPE_TXQ),
    PREALLOC_INFO(BUF_TYPE_AMSDU),
    PREALLOC_INFO(TRACE_PTR_EXPEND),
    PREALLOC_INFO(TRACE_STR_EXPEND),
#undef PREALLOC_INFO
};

static struct {
    void *buf;
    size_t actual_size;
} pre_bufs[PREALLOC_BUF_TYPE_MAX];

void *__aml_mem_prealloc(enum prealloc_buf_type buf_type, size_t req_size, size_t *actual_size)
{
    void *buf = NULL;
    const char *name;

    if (buf_type >= PREALLOC_BUF_TYPE_MAX) {
        AML_ERR("type %d (>= %d) is invalid!\n", buf_type, PREALLOC_BUF_TYPE_MAX);
        return NULL;
    }

    name = pre_buf_infos[buf_type].name;
    buf = pre_bufs[buf_type].buf;
    if (actual_size)
        *actual_size = pre_bufs[buf_type].actual_size;

    if (pre_bufs[buf_type].actual_size >= req_size)
        AML_INFO("PREALLOC_%s req size %d\n", name, (int)req_size);
    else if (actual_size)
        AML_WARN("PREALLOC_%s actual size %d < %d\n", name, (int)(*actual_size), (int)req_size);
    else
        buf = NULL;

    if (!buf)
        AML_ERR("PREALLOC_%s req size %d: no memory!\n", name, (int)req_size);
    return buf;
}
EXPORT_SYMBOL(__aml_mem_prealloc);

void aml_deinit_wlan_mem(void)
{
    int i;

    AML_FN_ENTRY();

    for (i = 0; i < PREALLOC_BUF_TYPE_MAX; i++) {
        void *buf = pre_bufs[i].buf;

        if (buf) {
            kfree(buf);
            pre_bufs[i].buf = NULL;
        }
    }
}

int aml_init_wlan_mem(void)
{
    int i;

    for (i = 0; i < PREALLOC_BUF_TYPE_MAX; i++) {
        size_t size = pre_buf_infos[i].size;
        void *buf = kzalloc(size, GFP_KERNEL);

        if (!buf && i == PREALLOC_BUF_TYPE_RXBUF) {
            /* after OTA upgrade, large memory may fail, try small one */
            size >>= 1;
            buf = kzalloc(size, GFP_KERNEL);
            if (!buf) {
                size >>= 1;
                buf = kzalloc(size, GFP_KERNEL | __GFP_NOFAIL);
            }
            if (buf)
                AML_WARN("PREALLOC_%s gets smaller (%zd bytes)!\n", pre_buf_infos[i].name, size);
        }

        /* try again for no-RX buffer if system is temporarily out of resource */
        if (!buf && !(buf = kzalloc(size, GFP_KERNEL | __GFP_NOFAIL))) {
            AML_ERR("no memory for PREALLOC_%s (%zd bytes)!\n", pre_buf_infos[i].name, size);
            aml_deinit_wlan_mem();
            return -ENOMEM;
        }
        pre_bufs[i].buf = buf;
        pre_bufs[i].actual_size = size;
    }

    AML_INFO("OK\n");
    return 0;
}
