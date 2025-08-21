#include "aml_static_buf.h"
#include <linux/kernel.h>
#include <linux/skbuff.h>

void* wlan_static_hw_rx_buf;
void* wlan_static_sdio_buf;
void* wlan_static_amsdu_buf;
void* wlan_static_aml_buf_type_txq;
void* wlan_static_aml_buf_type_dump;
void* wlan_static_download_fw;

void* aml_mem_prealloc(int section, unsigned long size)
{
    pr_info("sectoin %d, size %ld\n", section, size);

    if (section == AML_PREALLOC_BUF_TYPE_TXQ) {
        if (size > AML_PREALLOC_BUF_TYPE_TXQ_SIZE) {
            pr_err("request aml BUF_TYPE_TXQ size(%lu) > %d\n",
                size, AML_PREALLOC_BUF_TYPE_TXQ_SIZE);
            return NULL;
        }
        return wlan_static_aml_buf_type_txq;
    }

    if (section == AML_PREALLOC_BUF_TYPE_DUMP) {
        if (size > AML_PREALLOC_BUF_TYPE_DUMP_SIZE) {
            pr_err("request AML_PREALLOC_BUF_TYPE_DUMP(%lu) > %d\n",
                size, AML_PREALLOC_BUF_TYPE_DUMP_SIZE);
            return NULL;
        }

        return wlan_static_aml_buf_type_dump;
    }

    if (section == AML_PREALLOC_DOWNLOAD_FW) {
        if (size > FW_VERBOSE_RING_SIZE) {
            pr_err("request DHD_PREALLOC_FW_VERBOSE_RING(%lu) > %d\n",
                size, FW_VERBOSE_RING_SIZE);
            return NULL;
        }

        return wlan_static_download_fw;
    }

    if (section == AML_PREALLOC_HW_RX) {
        if (size > WLAN_AML_HW_RX_SIZE) {
            pr_err("request WLAN_AML_HW_RX_SIZE(%lu) > %d\n",
                size, WLAN_AML_HW_RX_SIZE);
            return NULL;
        }
        return wlan_static_hw_rx_buf;
    }
    if (section == AML_PREALLOC_SDIO) {
        if (size > WLAN_AML_SDIO_SIZE) {
            pr_err("request WLAN_AML_SDIO_SIZE(%lu) > %d\n",
                size, WLAN_AML_SDIO_SIZE);
            return NULL;
        }
        return wlan_static_sdio_buf;
    }

    if (section == AML_PREALLOC_AMSDU) {
        if (size > WLAN_AML_AMSDU_SIZE) {
            pr_err("request WLAN_AML_AMSDU_SIZE(%lu) > %d\n",
                size, WLAN_AML_AMSDU_SIZE);
            return NULL;
        }
        return wlan_static_amsdu_buf;
    }

    if (section < 0 || section > AML_PREALLOC_MAX)
        pr_err("request section id(%d) is out of max index %d\n",
            section, AML_PREALLOC_MAX);

    pr_err("%s: failed to alloc section %d, size=%ld\n",
        __func__, section, size);

    return NULL;
}
EXPORT_SYMBOL(aml_mem_prealloc);

int aml_init_wlan_mem(void)
{
    wlan_static_aml_buf_type_txq = kzalloc(AML_PREALLOC_BUF_TYPE_TXQ_SIZE, GFP_KERNEL);
    if (!wlan_static_aml_buf_type_txq)
        goto err_mem_alloc0;

    wlan_static_aml_buf_type_dump = kzalloc(AML_PREALLOC_BUF_TYPE_DUMP_SIZE, GFP_KERNEL);
    if (!wlan_static_aml_buf_type_dump) {
        /* alloc large memory fail sometimes, for example after OTA upgrade */
        wlan_static_aml_buf_type_dump = kzalloc(AML_PREALLOC_BUF_TYPE_DUMP_SIZE, GFP_KERNEL | __GFP_NOFAIL);
        if (!wlan_static_aml_buf_type_dump)
            goto err_mem_alloc1;
    }
    wlan_static_download_fw = kzalloc(FW_VERBOSE_RING_SIZE, GFP_KERNEL);
    if (!wlan_static_download_fw)
        goto err_mem_alloc2;

    wlan_static_hw_rx_buf = kzalloc(WLAN_AML_HW_RX_SIZE, GFP_KERNEL);
    if (!wlan_static_hw_rx_buf) {
        /* alloc large memory fail sometimes, for example after OTA upgrade */
        wlan_static_hw_rx_buf = kzalloc(WLAN_AML_HW_RX_SIZE, GFP_KERNEL | __GFP_NOFAIL);
        if (!wlan_static_hw_rx_buf)
            goto err_mem_alloc3;
    }
    wlan_static_sdio_buf = kzalloc(WLAN_AML_SDIO_SIZE, GFP_KERNEL);
    if (!wlan_static_sdio_buf)
        goto err_mem_alloc4;

    wlan_static_amsdu_buf = kzalloc(WLAN_AML_AMSDU_SIZE, GFP_KERNEL);
    if (!wlan_static_amsdu_buf)
        goto err_mem_alloc5;

    pr_info("%s ok\n", __func__);
    return 0;

err_mem_alloc5:
    kfree(wlan_static_sdio_buf);
err_mem_alloc4:
    kfree(wlan_static_hw_rx_buf);
err_mem_alloc3:
    kfree(wlan_static_download_fw);
err_mem_alloc2:
    kfree(wlan_static_aml_buf_type_dump);
err_mem_alloc1:
    kfree(wlan_static_aml_buf_type_txq);
err_mem_alloc0:
    return -ENOMEM;
}

void aml_deinit_wlan_mem(void)
{
    printk("%s\n", __func__);
    kfree(wlan_static_amsdu_buf);
    kfree(wlan_static_sdio_buf);
    kfree(wlan_static_hw_rx_buf);
    kfree(wlan_static_download_fw);
    kfree(wlan_static_aml_buf_type_dump);
    kfree(wlan_static_aml_buf_type_txq);
}
