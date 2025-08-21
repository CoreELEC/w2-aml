/**
****************************************************************************************
*
* @file aml_static_buf.h
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022).
*
* @brief Declaration of the preallocing buffer.
*
****************************************************************************************
*/

#ifndef __AML_STATIC_BUF__
#define __AML_STATIC_BUF__

#define WLAN_AML_HW_RX_SIZE (5 * (324 * 1024))
#define WLAN_AML_SDIO_SIZE (320 * 1024)
#define WLAN_AML_AMSDU_SIZE (388 * 1024)
#define FW_VERBOSE_RING_SIZE (260 * 1024)
#define AML_PREALLOC_BUF_TYPE_TXQ_SIZE (36 * 1024)
#define AML_PREALLOC_BUF_TYPE_DUMP_SIZE (1290 * 1024)


enum aml_prealloc_index {
    AML_PREALLOC_BUF_TYPE_TXQ = 7,
    AML_PREALLOC_BUF_TYPE_DUMP = 11,
    AML_PREALLOC_DOWNLOAD_FW = 20,
    AML_PREALLOC_HW_RX = 24,
    AML_PREALLOC_SDIO = 25,
    AML_PREALLOC_AMSDU = 26,
    AML_PREALLOC_MAX
};

extern void* aml_mem_prealloc(int section, unsigned long size);
int aml_init_wlan_mem(void);
void aml_deinit_wlan_mem(void);

#endif
