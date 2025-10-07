/**
 ******************************************************************************
 *
 * @file aml_platform.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_PLAT_H_
#define _AML_PLAT_H_

#include <linux/pci.h>
#include <linux/netdevice.h>
#include "usb_common.h"
#include "sdio_common.h"
#include "aml_interface.h"
#include "w2_sdio.h"
#include "w2_usb.h"

#ifdef CONFIG_AML_POWER_SAVE_MODE
#include "chip_intf_reg.h"
#endif

#define AML_PHY_CONFIG_TRD_NAME        "aml_trident.ini"

#define AML_MAC_FW_SDIO "aml/wifi_w2_fw_sdio.bin"
#define AML_MAC_FW_USB "aml/wifi_w2_fw_usb.bin"
#define AML_MAC_FW_PCIE "aml/wifi_w2_fw_pcie.bin"

#define AML_MAC_REVC_FW_SDIO "aml/wifi_w2_revC_fw_sdio.bin"
#define AML_MAC_REVC_FW_USB "aml/wifi_w2_revC_fw_usb.bin"
#define AML_MAC_REVC_FW_PCIE "aml/wifi_w2_revC_fw_pcie.bin"

#define W2s_C_PRODUCT_AMLOGIC_EFUSE 0x0680
#define W2u_PRODUCT_C_AMLOGIC_EFUSE 0x0681
#define W2pRevC_PRODUCT_AMLOGIC_EFUSE 0x0682

#define MAC_SRAM_BASE 0x00a10000
#define UBUNTU_PC_VERSION   0xA1B2C3D4  // indicate ubuntu pc + w2
#define UBUNTU_SYNC_PATTERN 0x4D3C2B1A
#define UBUNTU_SYNC_ADDR    0x00a10018

#define AML_SHOW_RSSI(value) ((((int8_t)(u8)(value)) >= 0) ? ((u8)(value)) : ((u8)(value) - 256))
#define REG_OF_SYNC_TWO_RSSI    0x00a1001c
#define REG_OF_SYNC_RSSI (MAC_SRAM_BASE + 0x14)
#define REG_OF_VENDOR_ID (MAC_SRAM_BASE + 0x4)

#define CPU_CLK_REG_ADDR 0x00a0d090
#define CPU_CLK_VALUE 0x4f530033 // 240M

#ifndef CONFIG_AML_FPGA_PCIE
#define BAR2_TABLE_NUM 0x6
#define BAR4_TABLE_NUM 0x8
#define PCIE_TABLE_NUM (BAR2_TABLE_NUM + BAR4_TABLE_NUM)
#define ISTATUS_HOST 0x00a0218c

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

/*RG_AON_A53:This register is used to mark the type that prevents the firmware from going to sleep*/
#define PS_MSG_PUSH       BIT(0)
#define PS_READ_WRITE_REG BIT(1)
#define PS_TX_START       BIT(2)

#define PS_AWAKE   0x1
#define PS_DOZE    0x2


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

struct pcie_mem_map_struct
{
    unsigned char mem_domain;
    unsigned int pcie_bar_index;
    unsigned int pcie_bar_table_base_addr;
    unsigned int pcie_bar_table_high_addr;
    unsigned int pcie_bar_table_offset;
};
#endif

/**
 * Type of memory to access (cf aml_plat.get_address)
 *
 * @AML_ADDR_CPU To access memory of the embedded CPU
 * @AML_ADDR_SYSTEM To access memory/registers of one subsystem of the
 * embedded system
 *
 */
enum aml_platform_addr {
    AML_ADDR_CPU,
    AML_ADDR_AON,
    AML_ADDR_MAC_PHY,
    AML_ADDR_SYSTEM,
    AML_ADDR_MAX,
};

#define AML_BASE_ADDR  0x60000000   /* base address of AML_ADDR_SYSTEM */

extern unsigned int w2_aml_bus_type;
extern char * aml_wifi_get_bus_type(void);
extern u32 w2_aml_pci_readl(u8* addr);
extern void w2_aml_pci_writel(u32 data, u8* addr);

struct aml_hw;

/**
 * struct aml_plat - Operation pointers for AML PCI platform
 *
 * @pci_dev: pointer to pci dev
 * @enabled: Set if embedded platform has been enabled (i.e. fw loaded and
 *          ipc started)
 * @enable: Configure communication with the fw (i.e. configure the transfers
 *         enable and register interrupt)
 * @disable: Stop communication with the fw
 * @deinit: Free all resources allocated for the embedded platform
 * @get_address: Return the virtual address to access the requested address on
 *              the platform.
 * @ack_irq: Acknowledge the irq at link level.
 * @get_config_reg: Return the list (size + pointer) of registers to restore in
 * order to reload the platform while keeping the current configuration.
 *
 * @priv Private data for the link driver
 */
struct aml_plat {
    struct usb_device *usb_dev;
    struct auc_hif_ops *hif_ops;

    struct device *dev;
    struct aml_hif_sdio_ops *hif_sdio_ops;
    struct pci_dev *pci_dev;

    bool enabled;

    int (*enable)(struct aml_hw *aml_hw);
    int (*disable)(struct aml_hw *aml_hw);
    void (*deinit)(struct aml_plat *aml_plat);
    u8 *(*get_address)(struct aml_plat *aml_plat, int addr_name,
                       unsigned int offset);
    u32 (*ack_irq)(struct aml_hw *aml_hw);
    int (*get_config_reg)(struct aml_plat *aml_plat, const u32 **list);
    u8 priv[0] __aligned(sizeof(void *));
};

struct traffic_busy
{
    uint8_t traffic_busy_flag;
    uint8_t td_flag;
};


#define AML_ADDR(plat, base, offset)  (plat->get_address(plat, base, offset))


#ifdef CONFIG_AML_POWER_SAVE_MODE
/*RG_AON_A53 BIT(14) prevent fw goto sleep ,*/
static inline void aml_prevent_fw_sleep(struct aml_plat *plat, uint32_t sleep_prevent_type)
{
    uint32_t sleep_prevent;

    sleep_prevent = w2_aml_pci_readl(AML_ADDR(plat, AML_ADDR_AON, RG_AON_A53));
    sleep_prevent |= sleep_prevent_type;
    w2_aml_pci_writel(sleep_prevent, AML_ADDR(plat, AML_ADDR_AON, RG_AON_A53));
}

/*RG_AON_A53 BIT(14) prevent fw goto sleep ,*/
static inline void aml_allow_fw_sleep(struct aml_plat *plat, uint32_t sleep_allow_type)
{
    uint32_t sleep_allowed;

    sleep_allowed = w2_aml_pci_readl(AML_ADDR(plat, AML_ADDR_AON, RG_AON_A53));
    sleep_allowed &= (~sleep_allow_type);
    w2_aml_pci_writel(sleep_allowed, AML_ADDR(plat, AML_ADDR_AON, RG_AON_A53));
}

static inline void aml_wait_fw_wake(struct aml_plat *plat)
{
    uint32_t fw_status = 0;
    uint32_t loop = 20000;

    fw_status = w2_aml_pci_readl(AML_ADDR(plat, AML_ADDR_AON, RG_AON_A54));

    while ((fw_status == PS_DOZE) && (loop-- > 0))
    {
        fw_status = w2_aml_pci_readl(AML_ADDR(plat, AML_ADDR_AON, RG_AON_A54));
        udelay(10);
        if (loop == 0)
            return;
    }
}
#endif


static inline u32 aml_reg_read(struct aml_plat *plat, u32 base, u32 offset)
{
#ifdef CONFIG_AML_POWER_SAVE_MODE
    uint32_t reg_value = 0;
#endif

    if (w2_aml_bus_type == USB_MODE) {
        return plat->hif_ops->hi_read_word((unsigned int)(unsigned long)AML_ADDR(plat, base, offset), USB_EP4);
    } else if (w2_aml_bus_type == SDIO_MODE) {
       return plat->hif_sdio_ops->hi_random_word_read((unsigned int)(unsigned long)AML_ADDR(plat, base, offset));
    } else {
#ifdef CONFIG_AML_POWER_SAVE_MODE
        if (offset == RG_AON_A54) {
            return w2_aml_pci_readl(AML_ADDR(plat, base, offset));
        }

        aml_prevent_fw_sleep(plat, PS_READ_WRITE_REG);
        aml_wait_fw_wake(plat);
        reg_value = w2_aml_pci_readl(AML_ADDR(plat, base, offset));
        aml_allow_fw_sleep(plat, PS_READ_WRITE_REG);
        return reg_value;
#else
        return w2_aml_pci_readl(AML_ADDR(plat, base, offset));
#endif
    }

}

static inline void aml_reg_write(u32 val, struct aml_plat *plat, u32 base, u32 offset)
{
    if (w2_aml_bus_type == USB_MODE) {
        plat->hif_ops->hi_write_word((unsigned int)(unsigned long)AML_ADDR(plat, base, offset), val, USB_EP4);
    } else if (w2_aml_bus_type == SDIO_MODE) {
        plat->hif_sdio_ops->hi_random_word_write((unsigned int)(unsigned long)AML_ADDR(plat, base, offset), val);
    } else {
#ifdef CONFIG_AML_POWER_SAVE_MODE
        if (offset == RG_AON_A54) {
            w2_aml_pci_writel(val, AML_ADDR(plat, base, offset));
        } else {
            aml_prevent_fw_sleep(plat, PS_READ_WRITE_REG);
            aml_wait_fw_wake(plat);
            w2_aml_pci_writel(val, AML_ADDR(plat, base, offset));
            aml_allow_fw_sleep(plat, PS_READ_WRITE_REG);
        }
#else
        w2_aml_pci_writel(val, AML_ADDR(plat, base, offset));
#endif
    }
}

#define AML_REG_READ(plat, base, offset)       \
    aml_reg_read(plat, base, offset)
#define AML_REG_WRITE(val, plat, base, offset)         \
    aml_reg_write(val, plat, base, offset)

struct aml_pci
{
    u8 *pci_bar0_vaddr;
    u8 *pci_bar1_vaddr;
    u8 *pci_bar2_vaddr;
    u8 *pci_bar3_vaddr;
    u8 *pci_bar4_vaddr;
    u8 *pci_bar5_vaddr;
};

static inline struct device *aml_platform_get_dev(struct aml_plat *aml_plat)
{
    if (w2_aml_bus_type == USB_MODE) {
        return &(aml_plat->usb_dev->dev);
    } else if (w2_aml_bus_type == SDIO_MODE) {
        return aml_plat->dev;
    } else {
        return &(aml_plat->pci_dev->dev);
    }
}
static inline unsigned int aml_platform_get_irq(struct aml_plat *aml_plat)
{
    return aml_plat->pci_dev->irq;
}
u8* aml_pci_get_map_address(struct net_device *dev, unsigned int offset);

int aml_platform_init(struct aml_plat *aml_plat, void **platform_data);
void aml_platform_deinit(struct aml_hw *aml_hw);
void aml_get_fw_info(struct aml_hw *aml_hw, char *filename);
char *aml_get_fw_version(void);

int aml_platform_on(struct aml_hw *aml_hw, void *config);
void aml_platform_off(struct aml_hw *aml_hw, void **config);

int aml_platform_register_pcie_drv(void);
void aml_platform_unregister_pcie_drv(void);

int aml_platform_register_usb_drv(void);
void aml_platform_unregister_usb_drv(void);

int aml_platform_register_sdio_drv(void);
void aml_platform_unregister_sdio_drv(void);
void aml_get_vid(struct aml_plat *aml_plat);

int aml_platform_reset(struct aml_plat *aml_plat);
int aml_plat_lmac_load(struct aml_plat *aml_plat);
void aml_plat_mpif_sel(struct aml_plat *aml_plat);

int aml_cpufreq_boost_update(struct aml_hw *aml_hw);
int aml_cpufreq_boost_remove(struct aml_hw *aml_hw);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
void aml_sdio_wifi_pm_link(struct aml_hw *aml_hw);
void aml_sdio_wifi_pm_unlink(struct aml_hw *aml_hw);
#endif
#endif /* _AML_PLAT_H_ */
