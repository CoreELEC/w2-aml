/*
****************************************************************************************
*
* @file dp_tx.h
*
* @brief Implementation of the aml sdio_usb tx
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#ifndef _COMMON_FW_DP_TX_H_
#define _COMMON_FW_DP_TX_H_

#include "aml_types.h"

#define W2_TX_DESC_MAGIC        0x5458504b          /* "TXPK" */

#define W2_TXHW_HDR_LEN_USB     0
#define W2_TXHW_HDR_LEN_SDIO    12                  /* sizeof(struct HW_TxBufferInfo) */

#define SDIO_PAGE_MAX           65
#define USB_PAGE_MAX            25

/*
 * compact TX confirmation tag for SDIO/USB
 */
#define COMPACT_TXCFM_CNT       256

#define COMPACT_HOST_ID_BITS    26
#define COMPACT_HOST_ID_MASK    ((1U << COMPACT_HOST_ID_BITS) - 1)
#define COMPACT_AMPDU_SZ_BITS   (32 - COMPACT_HOST_ID_BITS)

struct compact_tx_cfm_tag       /* for SDIO/USB only */
{
    uint16_t status;            /* refer to low 16-bit of union aml_hw_txstatus */
    uint16_t amsdu_size;
    uint32_t ampdu_size: COMPACT_AMPDU_SZ_BITS,     /* up to 63 */
             hostid: COMPACT_HOST_ID_BITS;
};

static inline uint32_t aml_compact_tx_host_id(uint32_t hi, uint32_t host_id)
{
    return ((hi << 16) | (host_id & 0xFFFF)) & COMPACT_HOST_ID_MASK;
}

#endif /* _COMMON_FW_DP_TX_H_ */
