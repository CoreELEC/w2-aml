/**
 ******************************************************************************
 *
 * @file aml_v7.c - Support for v7 platform
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define AML_MODULE          PCI
#define AML_FMT             AML_FMT_M

#include "aml_w2_v7.h"
#include "aml_log.h"

u8* ipc_basic_address = 0;

#ifdef CONFIG_AML_FPGA_PCIE
static void aml_v7_platform_deinit(struct aml_plat_pci *aml_plat_pci)
{
    struct aml_v7 *aml_v7 = (struct aml_v7 *)aml_plat_pci->priv;

    pci_disable_device(aml_plat_pci->pci_dev);
    iounmap(aml_v7->pci_bar0_vaddr);
    iounmap(aml_v7->pci_bar1_vaddr);
    pci_release_regions(aml_plat_pci->pci_dev);
    pci_clear_master(aml_plat_pci->pci_dev);
    pci_disable_msi(aml_plat_pci->pci_dev);
    kfree(aml_plat_pci);
}

/**
 * aml_v7_platform_init - Initialize the DINI platform
 *
 * @pci_dev PCI device
 * @aml_plat Pointer on struct aml_stat * to be populated
 *
 * @return 0 on success, < 0 otherwise
 *
 * Allocate and initialize a aml_plat structure for the dini platform.
 */
int aml_v7_platform_init(struct pci_dev *pci_dev, struct aml_plat_pci **aml_plat_pci)
{
    struct aml_v7 *aml_v7;
    u16 pci_cmd = 0;
    int ret = 0;

    *aml_plat_pci = kzalloc(sizeof(struct aml_plat_pci) + sizeof(struct aml_v7),
                        GFP_KERNEL);
    AML_FN_ENTRY();
    if (!*aml_plat_pci)
        return -ENOMEM;

    aml_v7 = (struct aml_v7 *)(*aml_plat_pci)->priv;

    /* Hotplug fixups */
    pci_read_config_word(pci_dev, PCI_COMMAND, &pci_cmd);
    pci_cmd |= PCI_COMMAND_PARITY | PCI_COMMAND_SERR;
    pci_write_config_word(pci_dev, PCI_COMMAND, pci_cmd);
    pci_write_config_byte(pci_dev, PCI_CACHE_LINE_SIZE, L1_CACHE_BYTES >> 2);

    AML_FN_ENTRY();
    if ((ret = pci_enable_device(pci_dev))) {
        dev_err(&(pci_dev->dev), "pci_enable_device failed\n");
        goto out_enable;
    }

    pci_set_master(pci_dev);

    if ((ret = pci_request_regions(pci_dev, KBUILD_MODNAME))) {
        dev_err(&(pci_dev->dev), "pci_request_regions failed\n");
        goto out_request;
    }

    if (pci_enable_msi(pci_dev))
    {
        dev_err(&(pci_dev->dev), "pci_enable_msi failed\n");
        goto out_msi;

    }

    if (!(aml_v7->pci_bar0_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 0))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 0);
        ret = -ENOMEM;
        goto out_bar0;
    }
    AML_INFO(" bar0 %x\n", aml_v7->pci_bar0_vaddr);
    if (!(aml_v7->pci_bar1_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 1))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 1);
        ret = -ENOMEM;
        goto out_bar1;
    }
    AML_INFO(" bar1 %x\n", aml_v7->pci_bar1_vaddr);

    if (!(aml_v7->pci_bar2_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 2))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 2);
        ret = -ENOMEM;
        goto out_bar2;
    }
    AML_INFO(" bar2 %x\n", aml_v7->pci_bar2_vaddr);

    if (!(aml_v7->pci_bar3_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 3))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 3);
        ret = -ENOMEM;
        goto out_bar3;
    }
    AML_INFO(" bar3 %x\n", aml_v7->pci_bar3_vaddr);

    if (!(aml_v7->pci_bar4_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 4))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 4);
        ret = -ENOMEM;
        goto out_bar4;
    }
    AML_INFO(" bar4 %x\n", aml_v7->pci_bar4_vaddr);

    if (!(aml_v7->pci_bar5_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 5))) {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 5);
        ret = -ENOMEM;
        goto out_bar5;
    }
    ipc_basic_address = aml_v7->pci_bar5_vaddr;

    (*aml_plat_pci)->deinit = aml_v7_platform_deinit;

    return 0;

  out_bar5:
     iounmap(aml_v7->pci_bar4_vaddr);
     iounmap(aml_v7->pci_bar3_vaddr);
     iounmap(aml_v7->pci_bar2_vaddr);
     iounmap(aml_v7->pci_bar1_vaddr);
     iounmap(aml_v7->pci_bar0_vaddr);
  out_bar4:
    iounmap(aml_v7->pci_bar3_vaddr);
    iounmap(aml_v7->pci_bar2_vaddr);
    iounmap(aml_v7->pci_bar1_vaddr);
    iounmap(aml_v7->pci_bar0_vaddr);
  out_bar3:
    iounmap(aml_v7->pci_bar2_vaddr);
    iounmap(aml_v7->pci_bar1_vaddr);
    iounmap(aml_v7->pci_bar0_vaddr);
  out_bar2:
    iounmap(aml_v7->pci_bar1_vaddr);
    iounmap(aml_v7->pci_bar0_vaddr);

  out_bar1:
    iounmap(aml_v7->pci_bar0_vaddr);
  out_bar0:
    pci_disable_msi(pci_dev);
  out_msi:
    pci_release_regions(pci_dev);
  out_request:
    pci_clear_master(pci_dev);
    pci_disable_device(pci_dev);
  out_enable:
    kfree(*aml_plat_pci);
    return ret;
}

#else

unsigned long g_pcie_bar0_base_addr;

void aml_pcie_write(unsigned long addr, unsigned int val)
{
    writel(val, (void __iomem *)addr);
}

void pcie_addr_map(struct pci_dev *pci_dev, u64 src_addr, u64 trsl_addr, u64 table_size, unsigned int address, unsigned int direction)
{
    unsigned int atr_param;
    unsigned int src_addr_high;
    unsigned int trsl_addr_low;
    unsigned int trsl_addr_high;
    unsigned int trsl_param = 0;
    unsigned int atr_size = 0;
    u64 temp = table_size;

    while (temp != 0)
    {
        temp = temp / 2;
        atr_size++;
    }

    if (atr_size >= 2)
        atr_size = atr_size - 1 - 1;
    else {
        AML_ERR(" bar2 %d\n", atr_size);
        return;
    }
    /*
    [ATR_PARAM]
    Bit [0]:     ATR_IMPL: ATR_IMPL Field is 1 bit long. When set to 1, it
                   indicates that the Translation Address Table is implemented.
    Bit [6:1]:   ATR_SIZE: ATR_SIZE is 6 bits long and defines the Address
                   Translation Space Size. This space size in bytes is equal to
                   2^(ATR_SIZE +1). Allowed values for this field are from 6'd11 (2^12 = 4 KBytes)
                   to 6'd63 (2^64 = 16 ExaBytes) only.
    Bit [11:7]:  reserved
    Bit [31:12]: SRC_ADDR [31:12]
    */
    atr_param = (src_addr & 0xfffff000) | ((atr_size & 0x3f) << 1) | (1 << 0);
    //pci_write_config_dword(pci_dev, address, atr_param);
    aml_pcie_write(g_pcie_bar0_base_addr + address, atr_param);
     /*
      SRC_ADDR [63:32]
      SRC_ADDR [63:32] defines the starting address of the address translation space.
    */
    src_addr_high = (unsigned int )(src_addr >> 32);
    //pci_write_config_dword(pci_dev, address + 0x4, src_addr_high);
    aml_pcie_write(g_pcie_bar0_base_addr + address + 0x4, src_addr_high);

    /*
      TRSL_ADDR [31:12]
      TRSL_ADDR defines the starting translated address of the address translation space.
      SRC_ADDR and TRSL_ADDR are aligned on the Address Translation Space Size.
      Therefore, SRC_ADDR [integer (ATR_SIZE):0] and TRSL_ADDR [integer(ATR_SIZE):0] are ignored.
    */
    trsl_addr_low = trsl_addr;
    //pci_write_config_dword(pci_dev, address + 0x8, trsl_addr_low);
    aml_pcie_write(g_pcie_bar0_base_addr + address + 0x8, trsl_addr_low);

    /*
      TRSL_ADDR [63:32]
    */
    trsl_addr_high = 0;

    //pci_write_config_dword(pci_dev, address + 0xc, trsl_addr_high);
    aml_pcie_write(g_pcie_bar0_base_addr + address + 0xc, trsl_addr_high);

    if (direction ==  EP2XHCI) {
        trsl_param = 0x4e0004;
    } else if (direction == RC2XHCI) {
        trsl_param = 0x0;
    }
    //pci_write_config_dword(pci_dev, address + 0x10, trsl_param);
    aml_pcie_write(g_pcie_bar0_base_addr + address + 0x10, trsl_param);
}

static void aml_v7_platform_deinit(struct aml_plat_pci *aml_plat_pci)
{
    struct aml_v7 *aml_v7 = (struct aml_v7 *)aml_plat_pci->priv;

    AML_FN_ENTRY();
    pci_disable_device(aml_plat_pci->pci_dev);
    iounmap(aml_v7->pci_bar0_vaddr);
    iounmap(aml_v7->pci_bar2_vaddr);
    iounmap(aml_v7->pci_bar4_vaddr);
    pci_release_regions(aml_plat_pci->pci_dev);
    pci_clear_master(aml_plat_pci->pci_dev);
    pci_disable_msi(aml_plat_pci->pci_dev);
    kfree(aml_plat_pci);
}

/* aml w2 wifi/bt Recommend PCIe 1.0 */
void aml_pcie_speed_check(struct pci_dev *dev)
{
    u16 lnksta = 0;
    int ret;
    enum pci_bus_speed pci_speed;

    ret = pcie_capability_read_word(dev, PCI_EXP_LNKSTA, &lnksta);
    if (ret) {
        AML_ERR("pcie link speed is other, ret:%d\n", ret);
        return;
    }

    pci_speed = lnksta & PCI_EXP_LNKSTA_CLS;
    AML_INFO(" pcie link speed is %s\n",
        (pci_speed == PCI_EXP_LNKSTA_CLS_2_5GB) ? "PCI_EXP_LNKSTA_CLS_2_5GB" :
        ((pci_speed == PCI_EXP_LNKSTA_CLS_5_0GB) ? "PCI_EXP_LNKSTA_CLS_5_0GB" : "OTHER"));

    if (pci_speed != PCI_EXP_LNKSTA_CLS_2_5GB)
        AML_ERR(" warning===> pcie link speed is NOT PCI_EXP_LNKSTA_CLS_2_5GB !!!\n");
}


int aml_v7_platform_init(struct pci_dev *pci_dev, struct aml_plat_pci **aml_plat_pci)
{
    struct aml_v7 *aml_v7;
    int ret = -ENOMEM;
    unsigned long bar2_base_addr;
    unsigned long bar4_base_addr;
    *aml_plat_pci = kzalloc(sizeof(struct aml_plat_pci) + sizeof(struct aml_v7),
                        GFP_KERNEL);
    if (!*aml_plat_pci)
        return ret;

    AML_FN_ENTRY();

    aml_v7 = (struct aml_v7 *)(*aml_plat_pci)->priv;

    if ((ret = pci_enable_device(pci_dev))) {
        dev_err(&(pci_dev->dev), "pci_enable_device failed\n");
        goto out_enable;
    }

    // get ep bar2's base phy addr
    bar2_base_addr = pci_resource_start(pci_dev, 2);
    // get ep bar4's base phy addr
    bar4_base_addr = pci_resource_start(pci_dev, 4);

    AML_INFO("ep_bar2_addr: 0x%lx, ep_bar4_addr:0x%lx\n", bar2_base_addr, bar4_base_addr);

    if (!(aml_v7->pci_bar0_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 0)))
    {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 0);
        goto out_bar0;
    }

    g_pcie_bar0_base_addr = (unsigned long)aml_v7->pci_bar0_vaddr;

    AML_INFO("ep_bar0_addr:0x%lx\n", g_pcie_bar0_base_addr);
    // PCIe to AXI4 Master Address Translation Endpoint Mode
    // bar2 address translation
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE0_OFFSET, PCIE_BAR2_TABLE0_EP_BASE_ADDR,
        PCIE_BAR2_TABLE0_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 0, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE1_OFFSET, PCIE_BAR2_TABLE1_EP_BASE_ADDR,
        PCIE_BAR2_TABLE1_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 1, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE2_OFFSET, PCIE_BAR2_TABLE2_EP_BASE_ADDR,
        PCIE_BAR2_TABLE2_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 2, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE3_OFFSET, PCIE_BAR2_TABLE3_EP_BASE_ADDR,
        PCIE_BAR2_TABLE3_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 3, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE4_OFFSET, PCIE_BAR2_TABLE4_EP_BASE_ADDR,
        PCIE_BAR2_TABLE4_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 4, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE5_OFFSET, PCIE_BAR2_TABLE5_EP_BASE_ADDR,
        PCIE_BAR2_TABLE5_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 5, EP2XHCI);
    pcie_addr_map(pci_dev, bar2_base_addr + PCIE_BAR2_TABLE6_OFFSET, PCIE_BAR2_TABLE6_EP_BASE_ADDR,
        PCIE_BAR2_TABLE6_SIZE, ATR_PCIE_WIN0 + ATR_TABLE_SIZE * 6, EP2XHCI);

    // bar4 address translation
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE0_OFFSET, PCIE_BAR4_TABLE0_EP_BASE_ADDR,
        PCIE_BAR4_TABLE0_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 0, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE1_OFFSET, PCIE_BAR4_TABLE1_EP_BASE_ADDR,
        PCIE_BAR4_TABLE1_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 1, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE2_OFFSET, PCIE_BAR4_TABLE2_EP_BASE_ADDR,
        PCIE_BAR4_TABLE2_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 2, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE3_OFFSET, PCIE_BAR4_TABLE3_EP_BASE_ADDR,
        PCIE_BAR4_TABLE3_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 3, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE4_OFFSET, PCIE_BAR4_TABLE4_EP_BASE_ADDR,
        PCIE_BAR4_TABLE4_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 4, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE5_OFFSET, PCIE_BAR4_TABLE5_EP_BASE_ADDR,
        PCIE_BAR4_TABLE5_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 5, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE6_OFFSET, PCIE_BAR4_TABLE6_EP_BASE_ADDR,
        PCIE_BAR4_TABLE6_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 6, EP2XHCI);
    pcie_addr_map(pci_dev, bar4_base_addr + PCIE_BAR4_TABLE7_OFFSET, PCIE_BAR4_TABLE7_EP_BASE_ADDR,
        PCIE_BAR4_TABLE7_SIZE, ATR_PCIE_WIN1 + ATR_TABLE_SIZE * 7, EP2XHCI);

    pcie_addr_map(pci_dev, 0x0, 0x0, 0x100000000, ATR_AXI4_SLV0 + ATR_TABLE_SIZE * 1, RC2XHCI);

    pci_set_master(pci_dev);

    if ((ret = pci_request_regions(pci_dev, KBUILD_MODNAME))) {
        dev_err(&(pci_dev->dev), "pci_request_regions failed\n");
        goto out_request;
    }

    if ((ret = pci_enable_msi(pci_dev)))
    {
        dev_err(&(pci_dev->dev), "pci_enable_msi failed\n");
        goto out_msi;
    }

    // Get ep bar2 base addr
    if (!(aml_v7->pci_bar2_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 2)))
    {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 2);
        ret = -EIO;
        goto out_bar2;
    }

    AML_INFO("bar2 0x%lx\n", (unsigned long)aml_v7->pci_bar2_vaddr);
    // Get ep bar4 base addr
    if (!(aml_v7->pci_bar4_vaddr = (u8 *)pci_ioremap_bar(pci_dev, 4)))
    {
        dev_err(&(pci_dev->dev), "pci_ioremap_bar(%d) failed\n", 4);
        ret = -EIO;
        goto out_bar4;
    }

    AML_INFO("bar4 0x%lx\n", (unsigned long)aml_v7->pci_bar4_vaddr);

    ipc_basic_address = aml_v7->pci_bar4_vaddr + PCIE_BAR4_TABLE2_OFFSET; // bar4 table2 0x60800000
    (*aml_plat_pci)->deinit = aml_v7_platform_deinit;

    aml_pcie_speed_check(pci_dev);
    return 0;

  out_bar4:
    iounmap(aml_v7->pci_bar2_vaddr);
  out_bar2:
    pci_disable_msi(pci_dev);
  out_msi:
    pci_release_regions(pci_dev);
  out_request:
    pci_clear_master(pci_dev);
    pci_disable_device(pci_dev);
  out_enable:
    iounmap(aml_v7->pci_bar0_vaddr);
  out_bar0:
    kfree(*aml_plat_pci);
    return ret;
}

#endif

EXPORT_SYMBOL(ipc_basic_address);

