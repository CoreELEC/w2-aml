/**
 ******************************************************************************
 *
 * @file aml_pci.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define AML_MODULE   PCI

#include <linux/pci.h>
#include <linux/module.h>
#include <linux/init.h>

#include "aml_w2_v7.h"
#include "usb_common.h"
#include "aml_interface.h"
#include "chip_intf_reg.h"
#include "aml_log.h"
#include "chip_bt_pmu_reg.h"

#define W2p_VENDOR_AMLOGIC_EFUSE 0x1F35
#define W2p_PRODUCT_AMLOGIC_EFUSE 0x0602

#define W2pRevB_PRODUCT_AMLOGIC_EFUSE 0x0642
#define W2pRevC_PRODUCT_AMLOGIC_EFUSE 0x0682

struct aml_plat_pci *w2_g_aml_plat_pci;
unsigned char w2_g_pci_driver_insmoded;
unsigned char w2_g_pci_after_probe;
unsigned char w2_g_pci_shutdown;
unsigned char w2_g_pci_msg_suspend;

extern struct aml_pm_type w2_g_wifi_pm;
extern struct aml_bus_state_detect w2_bus_state_detect;

uint32_t w2_aml_pci_read_for_bt(int base, u32 offset);
static u8* aml_pci_get_address_from_domain(struct aml_plat_pci *aml_plat, int addr_name,
                               unsigned int offset);

static const struct pci_device_id aml_pci_ids[] =
{
    {PCI_DEVICE(0x0, 0x0)},
    {PCI_DEVICE(W2p_VENDOR_AMLOGIC_EFUSE, W2p_PRODUCT_AMLOGIC_EFUSE)},
    {PCI_DEVICE(W2p_VENDOR_AMLOGIC_EFUSE, W2pRevB_PRODUCT_AMLOGIC_EFUSE)},
    {PCI_DEVICE(W2p_VENDOR_AMLOGIC_EFUSE, W2pRevC_PRODUCT_AMLOGIC_EFUSE)},
    {0,}
};

#ifndef CONFIG_AML_FPGA_PCIE
const struct pcie_mem_map_struct w2_pcie_ep_addr_range[PCIE_TABLE_NUM] =
{
    // bar1 EP addr range
    {AML_ADDR_CPU,     PCIE_BAR2, PCIE_BAR2_TABLE0_EP_BASE_ADDR, PCIE_BAR2_TABLE0_EP_END_ADDR, PCIE_BAR2_TABLE0_OFFSET},
    {AML_ADDR_CPU,     PCIE_BAR2, PCIE_BAR2_TABLE1_EP_BASE_ADDR, PCIE_BAR2_TABLE1_EP_END_ADDR, PCIE_BAR2_TABLE1_OFFSET},
    {AML_ADDR_CPU,     PCIE_BAR2, PCIE_BAR2_TABLE2_EP_BASE_ADDR, PCIE_BAR2_TABLE2_EP_END_ADDR, PCIE_BAR2_TABLE2_OFFSET},
    {AML_ADDR_CPU,     PCIE_BAR2, PCIE_BAR2_TABLE3_EP_BASE_ADDR, PCIE_BAR2_TABLE3_EP_END_ADDR, PCIE_BAR2_TABLE3_OFFSET},
    {AML_ADDR_MAC_PHY, PCIE_BAR2, PCIE_BAR2_TABLE4_EP_BASE_ADDR, PCIE_BAR2_TABLE4_EP_END_ADDR, PCIE_BAR2_TABLE4_OFFSET},
    {AML_ADDR_MAC_PHY, PCIE_BAR2, PCIE_BAR2_TABLE5_EP_BASE_ADDR, PCIE_BAR2_TABLE5_EP_END_ADDR, PCIE_BAR2_TABLE5_OFFSET},

    // bar2 EP addr range
    {AML_ADDR_SYSTEM, PCIE_BAR4, PCIE_BAR4_TABLE0_EP_BASE_ADDR, PCIE_BAR4_TABLE0_EP_END_ADDR, PCIE_BAR4_TABLE0_OFFSET},
    {AML_ADDR_CPU,    PCIE_BAR4, PCIE_BAR4_TABLE1_EP_BASE_ADDR, PCIE_BAR4_TABLE1_EP_END_ADDR, PCIE_BAR4_TABLE1_OFFSET},
    {AML_ADDR_SYSTEM, PCIE_BAR4, PCIE_BAR4_TABLE2_EP_BASE_ADDR, PCIE_BAR4_TABLE2_EP_END_ADDR, PCIE_BAR4_TABLE2_OFFSET},
    {AML_ADDR_SYSTEM, PCIE_BAR4, PCIE_BAR4_TABLE3_EP_BASE_ADDR, PCIE_BAR4_TABLE3_EP_END_ADDR, PCIE_BAR4_TABLE3_OFFSET},
    {AML_ADDR_SYSTEM, PCIE_BAR4, PCIE_BAR4_TABLE4_EP_BASE_ADDR, PCIE_BAR4_TABLE4_EP_END_ADDR, PCIE_BAR4_TABLE4_OFFSET},
    {AML_ADDR_SYSTEM, PCIE_BAR4, PCIE_BAR4_TABLE5_EP_BASE_ADDR, PCIE_BAR4_TABLE5_EP_END_ADDR, PCIE_BAR4_TABLE5_OFFSET},
    {AML_ADDR_AON,    PCIE_BAR4, PCIE_BAR4_TABLE6_EP_BASE_ADDR, PCIE_BAR4_TABLE6_EP_END_ADDR, PCIE_BAR4_TABLE6_OFFSET},
    {AML_ADDR_CPU,    PCIE_BAR4, PCIE_BAR4_TABLE7_EP_BASE_ADDR, PCIE_BAR4_TABLE7_EP_END_ADDR, PCIE_BAR4_TABLE7_OFFSET},
};
#endif

/* Uncomment this for depmod to create module alias */
/* We don't want this on development platform */
//MODULE_DEVICE_TABLE(pci, aml_pci_ids);

bool aml_pci_resume_complete(struct pci_dev *pdev)
{
    u8 *addr;
    int err;
    unsigned int wake_flag;
    uint32_t loop = 200;

    addr = aml_pci_get_address_from_domain(w2_g_aml_plat_pci, AML_ADDR_AON, RG_AON_A25);
    wake_flag = readl(addr);
    AML_INFO("wake_flag = 0x%x\n", wake_flag);
    do
    {
        err = pci_set_power_state(pdev, PCI_D0);
        if (err) {
            ERROR_DEBUG_OUT("pci_set_power_state error %d \n", err);
            return false;
        }
        wake_flag = readl(addr);
        msleep(10);

        if (loop == 1)
        {
            AML_INFO("aml_pci_resume incomplete = 0x%x\n", wake_flag);
            return true;
        }
    } while ((!((wake_flag != 0xffffffff) && (wake_flag & BIT(0)))) && (loop-- > 0));

    return true;
}

static int aml_pci_probe(struct pci_dev *pci_dev,
                          const struct pci_device_id *pci_id)
{
    int ret = -ENODEV;

    AML_INFO(" %x\n", pci_id->vendor);

    if (pci_id->vendor == PCI_VENDOR_ID_XILINX) {
        AML_INFO("%x\n", PCI_VENDOR_ID_XILINX);
        ret = aml_v7_platform_init(pci_dev, &w2_g_aml_plat_pci);
    }
    else if ((pci_id->vendor == 0) || (pci_id->vendor == W2p_VENDOR_AMLOGIC_EFUSE))
    {
        AML_INFO(" pcie vendor id %x\n", pci_id->vendor);
        ret = aml_v7_platform_init(pci_dev, &w2_g_aml_plat_pci);
    }

    w2_g_aml_device_id = pci_id->device;

    AML_INFO("device id 0x%x\n", w2_g_aml_device_id);
    if (ret)
        return ret;

    w2_g_aml_plat_pci->pci_dev = pci_dev;
    w2_g_pci_after_probe = 1;

    return ret;
}

static void aml_pci_remove(struct pci_dev *pci_dev)
{
    AML_FN_ENTRY();
    w2_g_aml_plat_pci->deinit(w2_g_aml_plat_pci);
}

bool w2_g_pcie_suspend = 0;
static int aml_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
    int ret;
    u64 start_time_ns;
    u64 elapsed_time_ns = 0;
    u64 wait_bt_time_ns = 8000000000; //wait bt 8s
    u64 wait_wifi_time_ns = 12000000000; //wait wifi 12s

    //bt open
    if (w2_aml_pci_read_for_bt(AML_ADDR_AON, RG_BT_PMU_A16) & BIT(31))
    {
        start_time_ns = sched_clock();
        //bt drv suspend set bit25
        while ((w2_aml_pci_read_for_bt(AML_ADDR_AON, RG_AON_A24) & BIT(25)) &&
                (w2_bus_state_detect.bus_err == 0) &&
                (w2_bus_state_detect.is_recy_ongoing == 0) &&
                (elapsed_time_ns < wait_bt_time_ns))
        {
            elapsed_time_ns = sched_clock() - start_time_ns;
            msleep(10);
        }

        if (elapsed_time_ns >= wait_bt_time_ns)
        {
            AML_INFO("bt suspend fail, return\n");
            return -1;
        }

        // Detect a bus error or ongoing recovery,
        // exit immediately to prevent blocking the kernel USB resume call.
        if (w2_bus_state_detect.bus_err || w2_bus_state_detect.is_recy_ongoing)
        {
            AML_INFO("Detect a bus error or ongoing recovery, return\n");
            return -1;
        }
    }

    elapsed_time_ns = 0;
    if (atomic_read(&w2_g_wifi_pm.wifi_enable))
    {
        start_time_ns = sched_clock();
        while ((atomic_read(&w2_g_wifi_pm.drv_suspend_cnt) == 0) &&
              (w2_bus_state_detect.bus_err == 0) &&
              (w2_bus_state_detect.is_recy_ongoing == 0) &&
              (elapsed_time_ns < wait_wifi_time_ns))
        {
            elapsed_time_ns = sched_clock() - start_time_ns;
            msleep(10);
        }

        if (elapsed_time_ns >= wait_wifi_time_ns)
        {
            AML_INFO("wifi suspend fail, return\n");
            return -1;
        }

        if (atomic_read(&w2_g_wifi_pm.wifi_suspend_state) != 0)
        {
            AML_INFO("Detect wifi suspend fail\n");
            return -1;
        }

        // Detect a bus error or ongoing recovery,
        // exit immediately to prevent blocking the kernel USB resume call.
        if (w2_bus_state_detect.bus_err || w2_bus_state_detect.is_recy_ongoing)
        {
            AML_INFO("Detect a bus error or ongoing recovery, return\n");
            return -1;
        }
    }

    w2_g_pcie_suspend = 1;
    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 1);
    AML_INFO("%s\n", __func__);
    //aml_suspend_dump_cfgregs(bus, "BEFORE_EP_SUSPEND");
    pci_save_state(pdev);
    pci_enable_wake(pdev, PCI_D0, 1);

    ret = pci_set_power_state(pdev, PCI_D3hot);
    if (ret) {
        ERROR_DEBUG_OUT("pci_set_power_state error %d\n", ret);
    }
    //Delay 100ms to ensure ltssm enters L1 completion, delaying PCIe PHY power-off.
    usleep_range(100000, 120000);
    AML_FN_EXIT();
    //aml_suspend_dump_cfgregs(bus, "AFTER_EP_SUSPEND");
    return ret;
}

static int aml_pci_resume(struct pci_dev *pdev)
{
    int err;
    bool pci_resume_ok = false;
    AML_FN_ENTRY();
    pci_restore_state(pdev);

    pci_set_master(pdev);
    err = pci_set_power_state(pdev, PCI_D0);
    if (err) {
        ERROR_DEBUG_OUT("pci_set_power_state error %d \n", err);
        goto out;
    }

    pci_resume_ok = aml_pci_resume_complete(pdev);
    if (!pci_resume_ok)
    {
        AML_INFO("pci_resume_ok = 0x%x\n", pci_resume_ok);
        goto out;
    }
    w2_g_pcie_suspend = 0;
    AML_INFO(" ok exit\n");
out:
    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    return err;
}

extern uint32_t w2_aml_pci_read_for_bt(int base, u32 offset);
extern void w2_aml_pci_write_for_bt(u32 val, int base, u32 offset);
extern lp_shutdown_func w2_g_lp_shutdown_func;
extern bt_shutdown_func w2_g_bt_shutdown_func;

static void aml_pci_shutdown(struct pci_dev *pdev)
{
    AML_INFO(" aml_pci_shutdown begin \n");

    //Mask interrupt reporting to the host
    atomic_set(&w2_g_wifi_pm.is_shut_down, 2);

    // Notify fw to enter shutdown mode
    if (w2_g_bt_shutdown_func != NULL)
    {
        w2_g_bt_shutdown_func();
    }

    // Notify fw to enter shutdown mode
    if (w2_g_lp_shutdown_func != NULL)
    {
        w2_g_lp_shutdown_func();
    }

    //notify fw shutdown
    //notify bt wifi will go shutdown
    w2_aml_pci_write_for_bt(w2_aml_pci_read_for_bt(AML_ADDR_AON, RG_AON_A16) | BIT(28), AML_ADDR_AON, RG_AON_A16);
    w2_g_pci_shutdown = 1;

    if (pci_is_enabled(pdev))
    {
        msleep(100);
        pci_disable_device(pdev);
    }
    AML_FN_EXIT();
}

static struct pci_driver aml_pci_drv = {
    .name     = KBUILD_MODNAME,
    .id_table = aml_pci_ids,
    .probe    = aml_pci_probe,
    .remove   = aml_pci_remove,
    .suspend  = aml_pci_suspend,
    .resume   = aml_pci_resume,
    .shutdown = aml_pci_shutdown,
};

int w2_aml_pci_insmod(void)
{
    int err = 0;

    err = pci_register_driver(&aml_pci_drv);
    w2_g_pci_driver_insmoded = 1;
    w2_g_pci_shutdown = 0;
    w2_g_pci_msg_suspend = 0;

    if (err) {
        AML_ERR("failed to register pci driver: %d \n", err);
    }

    AML_FN_EXIT();
    return err;
}

void w2_aml_pci_rmmod(void)
{
    pci_unregister_driver(&aml_pci_drv);
    w2_g_pci_driver_insmoded = 0;
    w2_g_pci_after_probe = 0;

    AML_INFO(" aml common driver rmsmod\n");
}

static u8* aml_pci_get_address_from_domain(struct aml_plat_pci *aml_plat, int addr_name,
                               unsigned int offset)
{
#ifndef CONFIG_AML_FPGA_PCIE
    unsigned int i;
    unsigned int addr;
#endif
    struct aml_v7 *aml_pci = (struct aml_v7 *)aml_plat->priv;

    if (WARN(addr_name >= AML_ADDR_MAX, "Invalid address %d", addr_name))
        return NULL;

#ifdef CONFIG_AML_FPGA_PCIE

    if (addr_name == AML_ADDR_CPU) //0x00000000-0x0007ffff (ICCM)
    {
        AML_INFO(" address %x\n", aml_pci->pci_bar4_vaddr + offset);
        return aml_pci->pci_bar4_vaddr + offset;
    }
    else if (addr_name == AML_ADDR_MAC_PHY) //0x00a00000-0x00afffff
    {
        AML_INFO(" address %x\n", aml_pci->pci_bar3_vaddr + offset);
        return aml_pci->pci_bar3_vaddr + offset - 0x00a00000;
    }
    else if (addr_name == AML_ADDR_AON)// 0x00c00000 - 0x00ffffff (AON & DCCM)
    {
        AML_INFO(" address %x\n", aml_pci->pci_bar2_vaddr + offset);
        return aml_pci->pci_bar2_vaddr + offset - 0x00c00000;
    }
    else if (addr_name == AML_ADDR_SYSTEM)
    {
        if (offset >= IPC_REG_BASE_ADDR)
        {
            AML_INFO("bar5 %x, address %x\n", aml_pci->pci_bar5_vaddr, aml_pci->pci_bar5_vaddr + offset - IPC_REG_BASE_ADDR);
            return aml_pci->pci_bar5_vaddr + offset - IPC_REG_BASE_ADDR;
        }
        else
        {
            AML_INFO(" address %x\n", aml_pci->pci_bar0_vaddr + offset);
            return aml_pci->pci_bar0_vaddr + offset;
        }
    }
    else
    {
        AML_ERR(" error addr_name\n");
        return NULL;
    }

#else

    if (addr_name == AML_ADDR_SYSTEM)
    {
        addr = offset + PCIE_BAR4_TABLE0_EP_BASE_ADDR;
    }
    else
    {
        addr = offset;
    }

    for (i = 0; i < PCIE_TABLE_NUM; i++)
    {
        if ((addr_name == w2_pcie_ep_addr_range[i].mem_domain) &&
            (addr >= w2_pcie_ep_addr_range[i].pcie_bar_table_base_addr) &&
            (addr <= w2_pcie_ep_addr_range[i].pcie_bar_table_high_addr))
        {
            if (w2_pcie_ep_addr_range[i].pcie_bar_index == PCIE_BAR2)
            {
                return aml_pci->pci_bar2_vaddr + w2_pcie_ep_addr_range[i].pcie_bar_table_offset + (addr - w2_pcie_ep_addr_range[i].pcie_bar_table_base_addr);
            }
            else
            {
                return aml_pci->pci_bar4_vaddr + w2_pcie_ep_addr_range[i].pcie_bar_table_offset + (addr - w2_pcie_ep_addr_range[i].pcie_bar_table_base_addr);
            }
        }
    }

    AML_INFO(" addr(0x%x) or addr_name(0x%x) err\n", offset, addr_name);
    return NULL;

#endif //CONFIG_AML_FPGA_PCIE
}

u32 w2_aml_pci_readl(u8* addr)
{
    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || w2_g_pci_shutdown)
    {
        AML_ERR("pci readl err,bus_suspend_cnt = %x, w2_g_pci_shutdown = %x \n", atomic_read(&w2_g_wifi_pm.bus_suspend_cnt), w2_g_pci_shutdown);
        return 0;
    }
    else
        return readl(addr);
}

void w2_aml_pci_writel(u32 data, u8* addr)
{
    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || w2_g_pci_shutdown) {
        AML_ERR("pci writel err,bus_suspend_cnt = %x, w2_g_pci_shutdown = %x \n", atomic_read(&w2_g_wifi_pm.bus_suspend_cnt), w2_g_pci_shutdown);
    }
    else
        writel(data, addr);
    return;
}

uint32_t w2_aml_pci_read_for_bt(int base, u32 offset)
{
    u8 *addr;
    addr = aml_pci_get_address_from_domain(w2_g_aml_plat_pci, base, offset);

    if (addr == NULL)
    {
        AML_ERR("ERROR aml_pci_get_address_from_domain, addr is null\n");
        return 0;
    }

    return w2_aml_pci_readl(addr);
}

void w2_aml_pci_write_for_bt(u32 val, int base, u32 offset)
{
    u8 *addr;
    addr = aml_pci_get_address_from_domain(w2_g_aml_plat_pci, base, offset);

    if (addr == NULL)
    {
        AML_ERR("ERROR aml_pci_get_address_from_domain, addr is null\n");
        return;
    }

    w2_aml_pci_writel(val, addr);
}

EXPORT_SYMBOL(w2_aml_pci_readl);
EXPORT_SYMBOL(w2_aml_pci_writel);
EXPORT_SYMBOL(w2_aml_pci_read_for_bt);
EXPORT_SYMBOL(w2_aml_pci_write_for_bt);

EXPORT_SYMBOL(w2_aml_pci_insmod);
EXPORT_SYMBOL(w2_aml_pci_rmmod);
EXPORT_SYMBOL(w2_g_aml_plat_pci);
EXPORT_SYMBOL(w2_g_pci_driver_insmoded);
EXPORT_SYMBOL(w2_g_pci_after_probe);
EXPORT_SYMBOL(w2_g_pci_shutdown);
EXPORT_SYMBOL(w2_g_pci_msg_suspend);

#ifndef CONFIG_AML_FPGA_PCIE
EXPORT_SYMBOL(w2_pcie_ep_addr_range);
#endif
EXPORT_SYMBOL(w2_g_pcie_suspend);
