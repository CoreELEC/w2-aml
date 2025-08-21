/**
 ******************************************************************************
 *
 * @file aml_v7.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_w2_V7_H_
#define _AML_w2_V7_H_

#include <linux/pci.h>

/**
 * struct aml_plat_pci - Operation pointers for AML PCI platform
 *
 * @pci_dev: pointer to pci dev
 * @enabled: Set if embedded platform has been enabled (i.e. fw loaded and
 *          ipc started)
 * @enable: Configure communication with the fw (i.e. configure the transfers
 *         enable and register interrupt)
 * @disable: Stop communication with the fw
 * @deinit: Free all ressources allocated for the embedded platform
 * @get_address: Return the virtual address to access the requested address on
 *              the platform.
 * @ack_irq: Acknowledge the irq at link level.
 * @get_config_reg: Return the list (size + pointer) of registers to restore in
 * order to reload the platform while keeping the current configuration.
 *
 * @priv Private data for the link driver
 */
struct aml_plat_pci {
    struct usb_device *usb_dev;
    struct auc_hif_ops *hif_ops;

    struct device *dev;
    struct aml_hif_sdio_ops *hif_sdio_ops;

    struct pci_dev *pci_dev;

    bool enabled;

    int (*enable)(void *aml_hw);
    int (*disable)(void *aml_hw);
    void (*deinit)(struct aml_plat_pci *aml_plat_pci);
    u8* (*get_address)(struct aml_plat_pci *aml_plat_pci, int addr_name,
                       unsigned int offset);
    void (*ack_irq)(struct aml_plat_pci *aml_plat_pci);
    int (*get_config_reg)(struct aml_plat_pci *aml_plat_pci, const u32 **list);

    u8 priv[0] __aligned(sizeof(void *));
};
#define EP2XHCI 0x0
#define RC2XHCI 0x1

#define ATR_PCIE_WIN0   0x600
#define ATR_PCIE_WIN1   0x700
#define ATR_AXI4_SLV0   0x800

#define ATR_TABLE_SIZE  0x20


#define BAR2_TABLE_NUM 0x4
#define BAR4_TABLE_NUM 0x6
#define PCIE_TABLE_NUM (BAR2_TABLE_NUM + BAR4_TABLE_NUM)

// PCIE BAR TABLE OFFSET
#define PCIE_BAR2_TABLE0_OFFSET 0x00000000
#define PCIE_BAR2_TABLE1_OFFSET 0x00100000
#define PCIE_BAR2_TABLE2_OFFSET 0x00180000
#define PCIE_BAR2_TABLE3_OFFSET 0x001c0000
#define PCIE_BAR2_TABLE4_OFFSET 0x001e0000
#define PCIE_BAR2_TABLE5_OFFSET 0x001f0000
#define PCIE_BAR2_TABLE6_OFFSET 0x001f8000

#define PCIE_BAR4_TABLE0_OFFSET 0x00000000
#define PCIE_BAR4_TABLE1_OFFSET 0x00080000
#define PCIE_BAR4_TABLE2_OFFSET 0x00100000
#define PCIE_BAR4_TABLE3_OFFSET 0x00140000
#define PCIE_BAR4_TABLE4_OFFSET 0x00180000
#define PCIE_BAR4_TABLE5_OFFSET 0x00190000
#define PCIE_BAR4_TABLE6_OFFSET 0x001b0000
#define PCIE_BAR4_TABLE7_OFFSET 0x001c0000

#define PCIE_BAR2_TABLE0_SIZE 0x00100000
#define PCIE_BAR2_TABLE1_SIZE 0x00080000
#define PCIE_BAR2_TABLE2_SIZE 0x00040000
#define PCIE_BAR2_TABLE3_SIZE 0x00020000
#define PCIE_BAR2_TABLE4_SIZE 0x00010000
#define PCIE_BAR2_TABLE5_SIZE 0x00008000
#define PCIE_BAR2_TABLE6_SIZE 0x00008000

#define PCIE_BAR4_TABLE0_SIZE 0x00080000
#define PCIE_BAR4_TABLE1_SIZE 0x00080000
#define PCIE_BAR4_TABLE2_SIZE 0x00040000
#define PCIE_BAR4_TABLE3_SIZE 0x00040000
#define PCIE_BAR4_TABLE4_SIZE 0x00010000
#define PCIE_BAR4_TABLE5_SIZE 0x00020000
#define PCIE_BAR4_TABLE6_SIZE 0x00010000
#define PCIE_BAR4_TABLE7_SIZE 0x00020000

// PCIE EP AHB_ADDR RANGE
#define PCIE_BAR2_TABLE0_EP_BASE_ADDR 0x00200000
#define PCIE_BAR2_TABLE0_EP_END_ADDR 0x002fffff
#define PCIE_BAR2_TABLE1_EP_BASE_ADDR 0x00000000
#define PCIE_BAR2_TABLE1_EP_END_ADDR 0x0007ffff
#define PCIE_BAR2_TABLE2_EP_BASE_ADDR 0x00d00000
#define PCIE_BAR2_TABLE2_EP_END_ADDR 0x00d3ffff
#define PCIE_BAR2_TABLE3_EP_BASE_ADDR 0x00400000
#define PCIE_BAR2_TABLE3_EP_END_ADDR 0x0041ffff
#define PCIE_BAR2_TABLE4_EP_BASE_ADDR 0x00a00000
#define PCIE_BAR2_TABLE4_EP_END_ADDR 0x00a0ffff
#define PCIE_BAR2_TABLE5_EP_BASE_ADDR 0x00a10000
#define PCIE_BAR2_TABLE5_EP_END_ADDR 0x00a17fff
#define PCIE_BAR2_TABLE6_EP_BASE_ADDR 0x00ac0000
#define PCIE_BAR2_TABLE6_EP_END_ADDR 0x00ac7fff

#define PCIE_BAR4_TABLE0_EP_BASE_ADDR 0x60000000
#define PCIE_BAR4_TABLE0_EP_END_ADDR 0x6007ffff
#define PCIE_BAR4_TABLE1_EP_BASE_ADDR 0x00300000
#define PCIE_BAR4_TABLE1_EP_END_ADDR 0x0037ffff
#define PCIE_BAR4_TABLE2_EP_BASE_ADDR 0x60800000
#define PCIE_BAR4_TABLE2_EP_END_ADDR 0x6083ffff
#define PCIE_BAR4_TABLE3_EP_BASE_ADDR 0x60400000
#define PCIE_BAR4_TABLE3_EP_END_ADDR 0x6043ffff
#define PCIE_BAR4_TABLE4_EP_BASE_ADDR 0x60b00000
#define PCIE_BAR4_TABLE4_EP_END_ADDR 0x60b08fff
#define PCIE_BAR4_TABLE5_EP_BASE_ADDR 0x60c00000
#define PCIE_BAR4_TABLE5_EP_END_ADDR 0x60c1ffff
#define PCIE_BAR4_TABLE6_EP_BASE_ADDR 0x00f00000
#define PCIE_BAR4_TABLE6_EP_END_ADDR 0x00f07fff
#define PCIE_BAR4_TABLE7_EP_BASE_ADDR 0x00500000
#define PCIE_BAR4_TABLE7_EP_END_ADDR 0x0051ffff

enum
{
    PCIE_BAR0,
    PCIE_BAR1,
    PCIE_BAR2,
    PCIE_BAR3,
    PCIE_BAR4,
    PCIE_BAR5,
    PCIE_BAR_MAX,
};

enum aml_platform_addr {
    AML_ADDR_CPU,
    AML_ADDR_AON,
    AML_ADDR_MAC_PHY,
    AML_ADDR_SYSTEM,
    AML_ADDR_MAX,
};

struct aml_v7
{
    u8 *pci_bar0_vaddr;
    u8 *pci_bar1_vaddr;
    u8 *pci_bar2_vaddr;
    u8 *pci_bar3_vaddr;
    u8 *pci_bar4_vaddr;
    u8 *pci_bar5_vaddr;
};

struct pcie_mem_map_struct
{
    unsigned char mem_domain;
    unsigned int pcie_bar_index;
    unsigned int pcie_bar_table_base_addr;
    unsigned int pcie_bar_table_high_addr;
    unsigned int pcie_bar_table_offset;
};


int aml_v7_platform_init(struct pci_dev *pci_dev,
                          struct aml_plat_pci **aml_plat);

#endif /* _AML_V7_H_ */
