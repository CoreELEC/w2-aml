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

#ifdef __linux__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifndef NX_TX_PAYLOAD_MAX
#define NX_TX_PAYLOAD_MAX   6
#endif

struct txpage_info {
    uint16_t total_len;
    uint16_t first_msdu_len;    /* for patch_txu_cntrl_amsdu_hdr_append() */

    uint8_t msdu_num;           /* for patch_txl_buffer_is_amsdu_multi_buf() */
    uint8_t page_num;

#define TXPAGE_INFO_PAGE_NUM_MAX    (NX_TX_PAYLOAD_MAX * 2) /* up to 2 pages per msdu */
    uint8_t pages[TXPAGE_INFO_PAGE_NUM_MAX];                /* firmware only */
};

#define HOSTDESC_TXPAGE_INFO(host)      ((struct txpage_info *)&(host)->packet_addr[0])

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
