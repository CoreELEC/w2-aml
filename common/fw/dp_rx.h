/**
 ****************************************************************************************
 *
 * @file dp_rx.h
 *
 * Copyright (C) Amlogic 2012-2024
 *
 ****************************************************************************************
 */

#ifndef _COMMON_FW_DP_RX_H_
#define _COMMON_FW_DP_RX_H_

#include "aml_types.h"

/* only valid for non-PCIe implementation */
struct aml_rhd_patch0 {         /* take the place of tsf_lo/tsf_hi */
    uint8_t payl_offset;
    uint8_t status;

    uint16_t reserved16;

    uint32_t reserved32;
};

#define AML_RHD0(rxdesc)        ((struct aml_rhd_patch0 *)&(rxdesc)->dma_hdrdesc.hd.tsflo)

struct aml_rhd_patch1 {         /* take the place of rx_vec_2 ( = struct rx_info of W2L ) */
    uint32_t new_read;          /* next desc */

    uint32_t reserved;
};

#define AML_RHD1(rxdesc)        ((struct aml_rhd_patch1 *)&(rxdesc)->dma_hdrdesc.hd.rx_vec_2)

/* only valid for non-PCIe implementation and MSDU/A-MSDU */
struct aml_rhd_ext {
    uint16_t sn         :12;    /* sequence number */
    uint16_t fn         :4;     /* fragment number */

    uint16_t reserved   :13;
    uint16_t morefrag   :1;
    uint16_t qos        :1;
    uint16_t pn_present :1;

    /* pn[0] is only valid if pn_present. WAPI uses 64-bit PN, others only use 48-bit */
    uint64_t pn[];              /* packet number */
} __packed;

#define RX_HDR_EXT_SIZE             ((uint32_t)offsetof(struct aml_rhd_ext, pn[1]))

#endif /* _COMMON_FW_DP_RX_H_ */
