/**
 ******************************************************************************
 *
 * @file reg_access.h
 *
 * @brief Definitions and macros for MAC HW and platform register accesses
 *
 * Copyright (C) Amlogic 2011-2021
 *
 ******************************************************************************
 */

#ifndef REG_ACCESS_H_
#define REG_ACCESS_H_
#include "aml_defs.h"

/*****************************************************************************
 * Addresses within AML_ADDR_CPU
 *****************************************************************************/
#define RAM_LMAC_FW_ADDR               0x00000000

/*****************************************************************************
 * Addresses within AML_ADDR_SYSTEM
 *****************************************************************************/
/* Shard RAM */
#define SHARED_RAM_SDIO_START_ADDR     0x60000000
#define IPC_BASIC_ADDRESS              0x60800000
#define SHARED_RAM_PCI_START_ADDR      0x00000000
#define SHARED_RAM_HOST_RXBUF_ADDR     0x00064400
#define SHARED_RAM_HOST_RXDESC_ADDR    0x00067c00

/* IPC registers */
#define IPC_REG_BASE_ADDR              0x00800000

#define AGCCCCACAL0_ADDR_CT 0x00c0b7e0
#define AGCCCCACAL1_ADDR_CT 0x00c0b7e4
#define AGCCCCACAL2_ADDR_CT 0x00c0b7e8

#define CLK_ADDR0 0x00805010
#define CLK_ADDR1 0x00805014
#define ENA_CLK_ADDR 0x00805300
#define DAC_CLK_ADDR 0x00805344
#define PLF_CLK_ADDR 0x00805340
#define MACWT_CLK_ADDR 0x0080533c
#define MACCORE_CLK_ADDR 0x00805338
#define LA_CLK_ADDR 0x00805334
#define MPIF_CLK_ADDR 0x00805330
#define PHY_CLK_ADDR 0x0080532c
#define VTB_CLK_ADDR 0x00805328
#define FEREF_CLK_ADDR 0x00805324
#define REF80_CLK_ADDR 0x00805320
#define REF40_CLK_ADDR 0x0080531c
#define LDPC_RX_CLK_ADDR 0x00805314
#define REF_44_CLK_ADDR 0x00805310



/* System Controller Registers */
#if 0  // old pcie(total 3 bar)
#define SYSCTRL_SIGNATURE_ADDR         0x00900000
// old diag register name
#define SYSCTRL_DIAG_CONF_ADDR         0x00900068
#define SYSCTRL_PHYDIAG_CONF_ADDR      0x00900074
#define SYSCTRL_RIUDIAG_CONF_ADDR      0x00900078
// new diag register name
#define SYSCTRL_DIAG_CONF0             0x00900064
#define SYSCTRL_DIAG_CONF1             0x00900068
#define SYSCTRL_DIAG_CONF2             0x00900074
#define SYSCTRL_DIAG_CONF3             0x00900078
#define SYSCTRL_MISC_CNTL_ADDR         0x009000E0

#else //new pcie(total 5 bar)

#define SYSCTRL_SIGNATURE_ADDR         0x00801000
// old diag register name
#define SYSCTRL_DIAG_CONF_ADDR         0x00801068
#define SYSCTRL_PHYDIAG_CONF_ADDR      0x00801074
#define SYSCTRL_RIUDIAG_CONF_ADDR      0x00801078
// new diag register name
#define SYSCTRL_DIAG_CONF0             0x00801064
#define SYSCTRL_DIAG_CONF1             0x00801068
#define SYSCTRL_DIAG_CONF2             0x00801074
#define SYSCTRL_DIAG_CONF3             0x00801078
#define SYSCTRL_MISC_CNTL_ADDR         0x008010E0

#endif

#define   BOOTROM_ENABLE               BIT(4)
#define   FPGA_B_RESET                 BIT(1)
#define   SOFT_RESET                   BIT(0)

#define   PHY_RESET                    BIT(0)
#define   MAC_RESET                    BIT(1)
#define   CPU_RESET                    BIT(2)
#define   USB_RESET                    BIT(5)
#define   SDIO_RESET                   BIT(6)

/* MAC platform */
#define NXMAC_VERSION_1_ADDR           0x00B00004
#define   NXMAC_MU_MIMO_TX_BIT         BIT(19)
#define   NXMAC_BFMER_BIT              BIT(18)
#define   NXMAC_BFMEE_BIT              BIT(17)
#define   NXMAC_MAC_80211MH_FORMAT_BIT BIT(16)
#define   NXMAC_COEX_BIT               BIT(14)
#define   NXMAC_WAPI_BIT               BIT(13)
#define   NXMAC_TPC_BIT                BIT(12)
#define   NXMAC_VHT_BIT                BIT(11)
#define   NXMAC_HT_BIT                 BIT(10)
#define   NXMAC_GCMP_BIT               BIT(9)
#define   NXMAC_RCE_BIT                BIT(8)
#define   NXMAC_CCMP_BIT               BIT(7)
#define   NXMAC_TKIP_BIT               BIT(6)
#define   NXMAC_WEP_BIT                BIT(5)
#define   NXMAC_SECURITY_BIT           BIT(4)
#define   NXMAC_SME_BIT                BIT(3)
#define   NXMAC_HCCA_BIT               BIT(2)
#define   NXMAC_EDCA_BIT               BIT(1)
#define   NXMAC_QOS_BIT                BIT(0)

#define NXMAC_RX_CNTRL_ADDR                     0x00B00060
#define   NXMAC_EN_DUPLICATE_DETECTION_BIT      BIT(31)
#define   NXMAC_ACCEPT_UNKNOWN_BIT              BIT(30)
#define   NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT    BIT(29)
#define   NXMAC_ACCEPT_QO_S_NULL_BIT            BIT(28)
#define   NXMAC_ACCEPT_QCFWO_DATA_BIT           BIT(27)
#define   NXMAC_ACCEPT_Q_DATA_BIT               BIT(26)
#define   NXMAC_ACCEPT_CFWO_DATA_BIT            BIT(25)
#define   NXMAC_ACCEPT_DATA_BIT                 BIT(24)
#define   NXMAC_ACCEPT_OTHER_CNTRL_FRAMES_BIT   BIT(23)
#define   NXMAC_ACCEPT_CF_END_BIT               BIT(22)
#define   NXMAC_ACCEPT_ACK_BIT                  BIT(21)
#define   NXMAC_ACCEPT_CTS_BIT                  BIT(20)
#define   NXMAC_ACCEPT_RTS_BIT                  BIT(19)
#define   NXMAC_ACCEPT_PS_POLL_BIT              BIT(18)
#define   NXMAC_ACCEPT_BA_BIT                   BIT(17)
#define   NXMAC_ACCEPT_BAR_BIT                  BIT(16)
#define   NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT    BIT(15)
#define   NXMAC_ACCEPT_BFMEE_FRAMES_BIT         BIT(14)
#define   NXMAC_ACCEPT_ALL_BEACON_BIT           BIT(13)
#define   NXMAC_ACCEPT_NOT_EXPECTED_BA_BIT      BIT(12)
#define   NXMAC_ACCEPT_DECRYPT_ERROR_FRAMES_BIT BIT(11)
#define   NXMAC_ACCEPT_BEACON_BIT               BIT(10)
#define   NXMAC_ACCEPT_PROBE_RESP_BIT           BIT(9)
#define   NXMAC_ACCEPT_PROBE_REQ_BIT            BIT(8)
#define   NXMAC_ACCEPT_MY_UNICAST_BIT           BIT(7)
#define   NXMAC_ACCEPT_UNICAST_BIT              BIT(6)
#define   NXMAC_ACCEPT_ERROR_FRAMES_BIT         BIT(5)
#define   NXMAC_ACCEPT_OTHER_BSSID_BIT          BIT(4)
#define   NXMAC_ACCEPT_BROADCAST_BIT            BIT(3)
#define   NXMAC_ACCEPT_MULTICAST_BIT            BIT(2)
#define   NXMAC_DONT_DECRYPT_BIT                BIT(1)
#define   NXMAC_EXC_UNENCRYPTED_BIT             BIT(0)

#define NXMAC_DEBUG_PORT_SEL_ADDR      0x00B00510
#define NXMAC_MAC_CNTRL_2_ADDR         0x00B08050
#define NXMAC_SOFT_RESET_BIT           BIT(0)
#define NXMAC_SW_SET_PROFILING_ADDR    0x00B08564
#define NXMAC_SW_CLEAR_PROFILING_ADDR  0x00B08568

/* Modem Configuration */
#define MDM_HDMCONFIG_ADDR             0x00C00000

/* Modem Version */
#define MDM_HDMVERSION_ADDR            0x00C0003C

/* Clock gating configuration */
#define MDM_MEMCLKCTRL0_ADDR           0x00C00848
#define MDM_CLKGATEFCTRL0_ADDR         0x00C00874
#if 0 //old pcie
#define CRM_CLKGATEFCTRL0_ADDR         0x00940010
#else
#define CRM_CLKGATEFCTRL0_ADDR         0x00805010 //new pcie
#endif
/* AGC (trident) */
#define AGC_AMLAGCCNTL_ADDR           0x00C02060

/* LDPC RAM*/
#define PHY_LDPC_RAM_ADDR              0x00C09000

/* FCU (elma )*/
#define FCU_AMLFCAGCCNTL_ADDR         0x00C09034

/* AGC RAM */
#define PHY_AGC_UCODE_ADDR             0x00C0A000

/* RIU */
#define RIU_AMLVERSION_ADDR           0x00C0B000
#define RIU_AMLDYNAMICCONFIG_ADDR     0x00C0B008
#define RIU_AGCMEMBISTSTAT_ADDR        0x00C0B238
#define RIU_AGCMEMSIGNATURESTAT_ADDR   0x00C0B23C
#define RIU_AMLAGCCNTL_ADDR           0x00C0B390

/* Cataxia Radio Controller */
#define RC_SYSTEM_CONFIGURATION_ADDR   0x00C0C000
#define RC_ACCES_TO_CATAXIA_REG_ADDR   0x00C0C004

/* RF ITF */
#if 0 //old pcie
#define FPGAB_MPIF_SEL_ADDR            0x00C10030
#define RF_V6_DIAGPORT_CONF1_ADDR      0x00C10010
#define RF_v6_PHYDIAG_CONF1_ADDR       0x00C10018

#define RF_V7_DIAGPORT_CONF1_ADDR      0x00F10010
#define RF_v7_PHYDIAG_CONF1_ADDR       0x00F10018
#else //new pcie
#define FPGAB_MPIF_SEL_ADDR            0x00C10030
#define RF_V6_DIAGPORT_CONF1_ADDR      0x00C10010
#define RF_v6_PHYDIAG_CONF1_ADDR       0x00C10018

#define RF_V7_DIAGPORT_CONF1_ADDR      0x00830010
#define RF_v7_PHYDIAG_CONF1_ADDR       0x00830018

#endif

extern u8 * ipc_basic_address;
extern unsigned char g_pci_shutdown;

__INLINE u32 reg_ipc_app_rd(void *env, unsigned int INDEX)
{
    struct aml_hw *aml_hw = (struct aml_hw *)env;

    if (aml_bus_type == USB_MODE) {
        return (((struct aml_hw *)env)->plat->hif_ops->hi_read_word((unsigned long)ipc_basic_address + 4 * (INDEX), USB_EP4));
    } else if (aml_bus_type == SDIO_MODE) {
        return aml_hw->plat->hif_sdio_ops->hi_random_word_read((unsigned long)(IPC_BASIC_ADDRESS + 4 * INDEX));
    } else {
        return aml_pci_readl(ipc_basic_address + 4*(INDEX));
    }
}

__INLINE void reg_ipc_app_wr(void *env, unsigned int INDEX, u32 value)
{
    struct aml_hw *aml_hw = (struct aml_hw *)env;

    if (aml_bus_type == USB_MODE) {
        (((struct aml_hw *)env)->plat->hif_ops->hi_write_word((unsigned long)ipc_basic_address + 4 * (INDEX), value, USB_EP4));
    } else if (aml_bus_type == SDIO_MODE) {
        aml_hw->plat->hif_sdio_ops->hi_random_word_write((unsigned long)(IPC_BASIC_ADDRESS + 4 * INDEX), value);
    } else {
        aml_pci_writel(value, ipc_basic_address + 4*(INDEX));
    }
}

#define REG_IPC_APP_RD(env, INDEX)       \
    reg_ipc_app_rd(env, INDEX)
#define REG_IPC_APP_WR(env, INDEX, value)         \
    reg_ipc_app_wr(env, INDEX, value)

#endif /* REG_ACCESS_H_ */
