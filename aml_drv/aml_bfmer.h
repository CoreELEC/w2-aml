/**
 ******************************************************************************
 *
 * @file aml_bfmer.h
 *
 * @brief VHT Beamformer function declarations
 *
 * Copyright (C) Amlogic 2016-2021
 *
 ******************************************************************************
 */

#ifndef _AML_BFMER_H_
#define _AML_BFMER_H_

/**
 * INCLUDE FILES
 ******************************************************************************
 */

#include "aml_defs.h"

/**
 * DEFINES
 ******************************************************************************
 */

/// Maximal supported report length (in bytes)
#define AML_BFMER_REPORT_MAX_LEN     2048

/// Size of the allocated report space (twice the maximum report length)
#define AML_BFMER_REPORT_SPACE_SIZE  (AML_BFMER_REPORT_MAX_LEN * 2)

/**
 * TYPE DEFINITIONS
 ******************************************************************************
 */

/*
 * Structure used to store a beamforming report.
 */
struct aml_bfmer_report {
    dma_addr_t dma_addr;    /* Virtual address provided to MAC for
                               DMA transfer of the Beamforming Report */
    unsigned int length;    /* Report Length */
    u8 report[1];           /* Report to be used for VHT TX Beamforming */
};

/**
 * FUNCTION DECLARATIONS
 ******************************************************************************
 */

/**
 ******************************************************************************
 * @brief Allocate memory aiming to contains the Beamforming Report received
 * from a Beamformee capable capable.
 * The providing length shall be large enough to contain the VHT Compressed
 * Beaforming Report and the MU Exclusive part.
 * It also perform a DMA Mapping providing an address to be provided to the HW
 * responsible for the DMA transfer of the report.
 * If successful a struct aml_bfmer_report object is allocated, it's address
 * is stored in aml_sta->bfm_report.
 *
 * @param[in] aml_hw   PHY Information
 * @param[in] aml_sta  Peer STA Information
 * @param[in] length    Memory size to be allocated
 *
 * @return 0 if operation is successful, else -1.
 ******************************************************************************
 */
int aml_bfmer_report_add(struct aml_hw *aml_hw, struct aml_sta *aml_sta,
                          unsigned int length);

/**
 ******************************************************************************
 * @brief Free a previously allocated memory intended to be used for
 * Beamforming Reports.
 *
 * @param[in] aml_hw   PHY Information
 * @param[in] aml_sta  Peer STA Information
 *
 ******************************************************************************
 */
void aml_bfmer_report_del(struct aml_hw *aml_hw, struct aml_sta *aml_sta);

#ifdef CONFIG_AML_FULLMAC
/**
 ******************************************************************************
 * @brief Parse a Rx VHT-MCS map in order to deduce the maximum number of
 * Spatial Streams supported by a beamformee.
 *
 * @param[in] vht_capa  Received VHT Capability field.
 *
 ******************************************************************************
 */
u8 aml_bfmer_get_rx_nss(const struct ieee80211_vht_cap *vht_capa);
#endif /* CONFIG_AML_FULLMAC */

#endif /* _AML_BFMER_H_ */
