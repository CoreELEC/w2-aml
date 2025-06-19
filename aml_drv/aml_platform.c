/**
 ******************************************************************************
 *
 * @file aml_platform.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define  AML_MODULE       PLATF

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>

#include "aml_platform.h"
#include "reg_access.h"
#include "hal_desc.h"
#include "aml_main.h"
#include "aml_w2_pci.h"
#ifndef CONFIG_AML_FHOST
#include "ipc_host.h"
#endif /* !CONFIG_AML_FHOST */

#include "chip_pmu_reg.h"
#include "aml_irqs.h"
#include "chip_ana_reg.h"
#include "wifi_intf_addr.h"
#include "wifi_top_addr.h"
#include "aml_utils.h"
#include <linux/interrupt.h>
#include "aml_prealloc.h"
#include "aml_task.h"

#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include "aml_agcram.h"

extern unsigned char auc_driver_insmoded;
extern struct usb_device *g_udev;
extern struct auc_hif_ops g_auc_hif_ops;
extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_plat_pci *g_aml_plat_pci;
extern unsigned char g_pci_driver_insmoded;
extern unsigned char g_pci_after_probe;
extern unsigned char g_pci_shutdown;
extern unsigned char g_pci_msg_suspend;
extern unsigned char g_sdio_after_porbe;
extern unsigned char g_usb_after_probe;
extern unsigned char wifi_drv_rmmod_ongoing;

#ifndef CONFIG_AML_FPGA_PCIE
extern struct pcie_mem_map_struct pcie_ep_addr_range[PCIE_TABLE_NUM];
#endif

struct pci_dev *g_pci_dev = NULL;
struct aml_hw *g_aml_hw = NULL;

int wifi_fw_download(char *firmware_filename);
int start_wifi(void);

#ifdef CONFIG_AML_TL4
/**
 * aml_plat_tl4_fw_upload() - Load the requested FW into embedded side.
 *
 * @aml_plat: pointer to platform structure
 * @fw_addr: Virtual address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a hex file, into the specified address
 */
static int aml_plat_tl4_fw_upload(struct aml_plat *aml_plat, u8* fw_addr,
                                   char *filename)
{
    struct device *dev = aml_platform_get_dev(aml_plat);
    const struct firmware *fw;
    int err = 0;
    u32 *dst;
    u8 const *file_data;
    char typ0, typ1;
    u32 addr0, addr1;
    u32 dat0, dat1;
    int remain;

    err = request_firmware(&fw, filename, dev);
    if (err) {
        return err;
    }
    file_data = fw->data;
    remain = fw->size;

    /* Copy the file on the Embedded side */
    dev_dbg(dev, "\n### Now copy %s firmware, @ = %p\n", filename, fw_addr);

    /* Walk through all the lines of the configuration file */
    while (remain >= 16) {
        u32 data, offset;

        if (sscanf(file_data, "%c:%08X %04X", &typ0, &addr0, &dat0) != 3)
            break;
        if ((addr0 & 0x01) != 0) {
            addr0 = addr0 - 1;
            dat0 = 0;
        } else {
            file_data += 16;
            remain -= 16;
        }
        if ((remain < 16) ||
            (sscanf(file_data, "%c:%08X %04X", &typ1, &addr1, &dat1) != 3) ||
            (typ1 != typ0) || (addr1 != (addr0 + 1))) {
            typ1 = typ0;
            addr1 = addr0 + 1;
            dat1 = 0;
        } else {
            file_data += 16;
            remain -= 16;
        }

        if (typ0 == 'C') {
            offset = 0x00200000;
            if ((addr1 % 4) == 3)
                offset += 2*(addr1 - 3);
            else
                offset += 2*(addr1 + 1);

            data = dat1 | (dat0 << 16);
        } else {
            offset = 2*(addr1 - 1);
            data = dat0 | (dat1 << 16);
        }
        dst = (u32 *)(fw_addr + offset);
        *dst = data;
    }

    release_firmware(fw);

    return err;
}
#endif

/**
 * aml_plat_bin_fw_upload() - Load the requested binary FW into embedded side.
 *
 * @aml_plat: pointer to platform structure
 * @fw_addr: Virtual address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a binary file, into the specified address
 */
#if 0
static int aml_plat_bin_fw_upload(struct aml_plat *aml_plat, u8* fw_addr,
                               char *filename)
{
    const struct firmware *fw = NULL;
    struct device *dev = aml_platform_get_dev(aml_plat);
    int err = 0;
    unsigned int size;
    u32 *src, *dst;
    unsigned int i;

    AML_FN_ENTRY();

    err = request_firmware(&fw, filename, dev);
    if (err) {
        AML_ERR("Please check version of agcram.bin, need update to %s !!!\n", filename);
        return err;
    }

    /* Copy the file on the Embedded side */
    dev_dbg(dev, "\n### Now copy %s firmware, @ = %p\n", filename, fw_addr);

    src = (u32 *)fw->data;
    dst = (u32 *)fw_addr;
    size = (unsigned int)fw->size;

    AML_INFO(" size %d\n", size);
    AML_INFO(" src %x\n", *src);

    /* check potential platform bug on multiple stores vs memcpy */
    if (aml_bus_type == USB_MODE) {
        aml_plat->hif_ops->hi_write_sram((unsigned char *)src, (unsigned char *)dst, size, USB_EP4);
    } else if (aml_bus_type == SDIO_MODE) {
        aml_plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)src, (unsigned char *)dst, size);
    } else {
        for (i = 0; i < size; i += 4) {
            *dst++ = *src++;
        }
    }
    release_firmware(fw);

    return err;
}
#endif

static int aml_plat_agc_download(struct aml_plat *aml_plat, u8* fw_addr)
{
    unsigned int size;
    u32 *src, *dst;
    unsigned int i;

    src = agc_ram;
    size = sizeof(agc_ram);
    dst = (u32 *)fw_addr;

    AML_INFO(" src addr 0x%x, size %d\n", fw_addr, size);

    /* check potential platform bug on multiple stores vs memcpy */
    if (aml_bus_type == USB_MODE) {
        aml_plat->hif_ops->hi_write_sram((unsigned char *)src, (unsigned char *)dst, size, USB_EP4);
    } else if (aml_bus_type == SDIO_MODE) {
        aml_plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)src, (unsigned char *)dst, size);
    } else {
        for (i = 0; i < size; i += 4) {
            *dst++ = *src++;
        }
    }

    return 0;
}

//sj
#define ICCM_ROM_LEN (256 * 1024)
#define ICCM_RAM_LEN (256 * 1024)
#define ICCM_ALL_LEN (ICCM_ROM_LEN + ICCM_RAM_LEN)
#define DCCM_ALL_LEN (256 * 1024)
#define ICCM_ROM_ADDR (0x00100000)
#define ICCM_RAM_ADDR (0x00100000 + ICCM_ROM_LEN)
#define DCCM_RAM_ADDR (0x00d00000)
#define DCCM_RAM_OFFSET (0x00700000) //0x00800000 - 0x00100000, in fw_flash
#define BYTE_IN_LINE (9)

#define RAM_BIN_LEN (1024 * 512 * 2)


#define IHEX_READ32(_val) {                                  \
        hex_buff[8] = 0;                                     \
        strncpy(hex_buff, (char *)src, 8);                   \
        if (kstrtouint(hex_buff, 16, &_val)) {               \
            AML_ERR(" goto end\n"); \
            goto end;                                        \
        }                                                    \
        src += BYTE_IN_LINE;                                 \
    }

static int aml_plat_fw_upload(struct aml_plat *aml_plat, u8* fw_addr,
                               char *filename)
{
    const struct firmware *fw = NULL;
    struct device *dev = aml_platform_get_dev(aml_plat);
    int err = 0;
    unsigned int i, size;
    u32 *dst;
    char hex_buff[9];
    u8 const *src;
    u32 data = 0;

    AML_FN_ENTRY();
    err = request_firmware(&fw, filename, dev);
    if (err) {
        return err;
    }

    src = (u8 *)fw->data;
    if (fw->size < RAM_BIN_LEN) {
        dst = (u32 *)(fw_addr + ICCM_ROM_LEN);
        size = ICCM_RAM_LEN;

    } else {
        dst = (u32 *)fw_addr;
        /* download iccm rom and ram */
        size = ICCM_ALL_LEN;
    }

    AML_INFO(" iccm dst %x\n", dst);
    AML_INFO(" iccm len %d\n", size/1024);
    for (i = 1; i <= size / 4; i += 1) {
        IHEX_READ32(data);
        *dst = __swab32(data);
        if (*dst != __swab32(data)) {
            AML_ERR("Download ICCM ERROR!\n");
            return -1;
        }
        dst++;
    }

    /* download dccm */
    src = (u8 *)(fw->data) + (size / 4) * BYTE_IN_LINE;
    size = DCCM_ALL_LEN;
#ifdef CONFIG_AML_FPGA_PCIE
    dst = (u32 *)AML_ADDR(aml_plat, AML_ADDR_AON, DCCM_RAM_ADDR);
#else
    dst = (u32 *)AML_ADDR(aml_plat, AML_ADDR_CPU, DCCM_RAM_ADDR);
#endif
    AML_INFO(" dccm dst %x, size %d\n", dst, size/1024);
    for (i = 1; i <= size / 4; i += 1) {
        IHEX_READ32(data);
        *dst = __swab32(data);
        if (*dst != __swab32(data)) {
            AML_ERR("Download DCCM ERROR!\n");
            return -1;
        }
        dst++;
    }

#if 0
    dst = (u32 *)fw_addr;
    for (i = 1; i < 50; i++)
        AML_INFO(" iccm check addr %x data %x\n", dst, *dst++);

    dst = (u32 *)AML_ADDR(aml_plat, AML_ADDR_CPU, DCCM_RAM_ADDR);
    for (i = 1; i < 50; i++)
        printk(" dccm check addr %x data %x\n", dst, *dst++);
#endif

end:
#undef IHEX_READ32
    release_firmware(fw);
    return err;
}

#ifndef CONFIG_AML_TL4
#define IHEX_REC_DATA           0
#define IHEX_REC_EOF            1
#define IHEX_REC_EXT_SEG_ADD    2
#define IHEX_REC_START_SEG_ADD  3
#define IHEX_REC_EXT_LIN_ADD    4
#define IHEX_REC_START_LIN_ADD  5

/**
 * aml_plat_ihex_fw_upload() - Load the requested intel hex 8 FW into embedded side.
 *
 * @aml_plat: pointer to platform structure
 * @fw_addr: Virtual address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a ihex file, into the specified address.
 */
 #if 0
static int aml_plat_ihex_fw_upload(struct aml_plat *aml_plat, u8* fw_addr,
                                    char *filename)
{
    const struct firmware *fw;
    struct device *dev = aml_platform_get_dev(aml_plat);
    u8 const *src, *end;
    u32 *dst;
    u16 haddr, segaddr, addr;
    u32 hwaddr;
    u8 load_fw, byte_count, checksum, csum, rec_type;
    int err, rec_idx;
    char hex_buff[9];

    err = request_firmware(&fw, filename, dev);
    if (err) {
        return err;
    }

    /* Copy the file on the Embedded side */
    dev_dbg(dev, "\n### Now copy %s firmware, @ = %p\n", filename, fw_addr);

    src = fw->data;
    end = src + (unsigned int)fw->size;
    haddr = 0;
    segaddr = 0;
    load_fw = 1;
    err = -EINVAL;
    rec_idx = 0;
    hwaddr = 0;

#define IHEX_READ8(_val, _cs) {                  \
        hex_buff[2] = 0;                         \
        strncpy(hex_buff, src, 2);               \
        if (kstrtou8(hex_buff, 16, &_val))       \
            goto end;                            \
        src += 2;                                \
        if (_cs)                                 \
            csum += _val;                        \
    }

#define IHEX_READ16(_val) {                        \
        hex_buff[4] = 0;                           \
        strncpy(hex_buff, src, 4);                 \
        if (kstrtou16(hex_buff, 16, &_val))        \
            goto end;                              \
        src += 4;                                  \
        csum += (_val & 0xff) + (_val >> 8);       \
    }

#define IHEX_READ32(_val) {                              \
        hex_buff[8] = 0;                                 \
        strncpy(hex_buff, src, 8);                       \
        if (kstrtouint(hex_buff, 16, &_val))             \
            goto end;                                    \
        src += 8;                                        \
        csum += (_val & 0xff) + ((_val >> 8) & 0xff) +   \
            ((_val >> 16) & 0xff) + (_val >> 24);        \
    }

#define IHEX_READ32_PAD(_val, _nb) {                    \
        memset(hex_buff, '0', 8);                       \
        hex_buff[8] = 0;                                \
        strncpy(hex_buff, src, (2 * _nb));              \
        if (kstrtouint(hex_buff, 16, &_val))            \
            goto end;                                   \
        src += (2 * _nb);                               \
        csum += (_val & 0xff) + ((_val >> 8) & 0xff) +  \
            ((_val >> 16) & 0xff) + (_val >> 24);       \
}

    /* loop until end of file is read*/
    while (load_fw) {
        rec_idx++;
        csum = 0;

        /* Find next colon start code */
        while (*src != ':') {
            src++;
            if ((src + 3) >= end) /* 3 = : + rec_len */
                goto end;
        }
        src++;

        /* Read record len */
        IHEX_READ8(byte_count, 1);
        if ((src + (byte_count * 2) + 8) >= end) /* 8 = rec_addr + rec_type + chksum */
            goto end;

        /* Read record addr */
        IHEX_READ16(addr);

        /* Read record type */
        IHEX_READ8(rec_type, 1);

        switch(rec_type) {
            case IHEX_REC_DATA:
            {
                /* Update destination address */
                dst = (u32 *) (fw_addr + hwaddr + addr);

                while (byte_count) {
                    u32 val;
                    if (byte_count >= 4) {
                        IHEX_READ32(val);
                        byte_count -= 4;
                    } else {
                        IHEX_READ32_PAD(val, byte_count);
                        byte_count = 0;
                    }
                    *dst++ = __swab32(val);
                }
                break;
            }
            case IHEX_REC_EOF:
            {
                load_fw = 0;
                err = 0;
                break;
            }
            case IHEX_REC_EXT_SEG_ADD: /* Extended Segment Address */
            {
                IHEX_READ16(segaddr);
                hwaddr = (haddr << 16) + (segaddr << 4);
                break;
            }
            case IHEX_REC_EXT_LIN_ADD: /* Extended Linear Address */
            {
                IHEX_READ16(haddr);
                hwaddr = (haddr << 16) + (segaddr << 4);
                break;
            }
            case IHEX_REC_START_LIN_ADD: /* Start Linear Address */
            {
                u32 val;
                IHEX_READ32(val); /* need to read for checksum */
                break;
            }
            case IHEX_REC_START_SEG_ADD:
            default:
            {
                dev_err(dev, "ihex: record type %d not supported\n", rec_type);
                load_fw = 0;
            }
        }

        /* Read and compare checksum */
        IHEX_READ8(checksum, 0);
        if (checksum != (u8)(~csum + 1))
            goto end;
    }

#undef IHEX_READ8
#undef IHEX_READ16
#undef IHEX_READ32
#undef IHEX_READ32_PAD

  end:
    release_firmware(fw);

    if (err)
        dev_err(dev, "%s: Invalid ihex record around line %d\n", filename, rec_idx);

    return err;
}
#endif
#endif /* CONFIG_AML_TL4 */

#ifndef CONFIG_AML_SDM
/**
 * aml_plat_get_rf() - Retrun the RF used in the platform
 *
 * @aml_plat: pointer to platform structure
 */
static u32 aml_plat_get_rf(struct aml_plat *aml_plat)
{
    u32 ver;
    ver = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, MDM_HDMCONFIG_ADDR);

    ver = __MDM_PHYCFG_FROM_VERS(ver);
    WARN(((ver != MDM_PHY_CONFIG_TRIDENT) &&
          (ver != MDM_PHY_CONFIG_CATAXIA) &&
          (ver != MDM_PHY_CONFIG_KARST)),
         "Unknown PHY version 0x%08x\n", ver);

    return ver;
}
#endif

/**
 * aml_plat_get_clkctrl_addr() - Return the clock control register address
 *
 * @aml_plat: platform data
 */
#ifndef CONFIG_AML_SDM
static u32 aml_plat_get_clkctrl_addr(struct aml_plat *aml_plat)
{
    u32 regval;

    if (aml_plat_get_rf(aml_plat) ==  MDM_PHY_CONFIG_TRIDENT)
        return MDM_MEMCLKCTRL0_ADDR;

    /* Get the FPGA signature */
    regval = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_SIGNATURE_ADDR);

    if (__FPGA_TYPE(regval) == 0xC0CA)
        return CRM_CLKGATEFCTRL0_ADDR;
    else
        return MDM_CLKGATEFCTRL0_ADDR;
}
#endif /* CONFIG_AML_SDM */

/**
 * aml_plat_stop_agcfsm() - Stop a AGC state machine
 *
 * @aml_plat: pointer to platform structure
 * @agg_reg: Address of the agccntl register (within AML_ADDR_SYSTEM)
 * @agcctl: Updated with value of the agccntl rgister before stop
 * @memclk: Updated with value of the clock register before stop
 * @agc_ver: Version of the AGC load procedure
 * @clkctrladdr: Indicates which AGC clock register should be accessed
 */
static void aml_plat_stop_agcfsm(struct aml_plat *aml_plat, int agc_reg,
                                  u32 *agcctl, u32 *memclk, u8 agc_ver,
                                  u32 clkctrladdr)
{
    /* First read agcctnl and clock registers */
    *memclk = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, clkctrladdr);

    /* Stop state machine : xxAGCCNTL0[AGCFSMRESET]=1 */
    *agcctl = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, agc_reg);
    AML_REG_WRITE((*agcctl) | BIT(12), aml_plat, AML_ADDR_SYSTEM, agc_reg);

    /* Force clock */
    if (agc_ver > 0) {
        /* CLKGATEFCTRL0[AGCCLKFORCE]=1 */
        AML_REG_WRITE((*memclk) | BIT(29), aml_plat, AML_ADDR_SYSTEM,
                       clkctrladdr);
        AML_INFO(" read 0x%x, 0x%x\n", clkctrladdr, AML_REG_READ(aml_plat, AML_ADDR_SYSTEM,clkctrladdr));
    } else {
        /* MEMCLKCTRL0[AGCMEMCLKCTRL]=0 */
        AML_REG_WRITE((*memclk) & ~BIT(3), aml_plat, AML_ADDR_SYSTEM,
                       clkctrladdr);
    }
}

/**
 * aml_plat_start_agcfsm() - Restart a AGC state machine
 *
 * @aml_plat: pointer to platform structure
 * @agg_reg: Address of the agccntl register (within AML_ADDR_SYSTEM)
 * @agcctl: value of the agccntl register to restore
 * @memclk: value of the clock register to restore
 * @agc_ver: Version of the AGC load procedure
 * @clkctrladdr: Indicates which AGC clock register should be accessed
 */
static void aml_plat_start_agcfsm(struct aml_plat *aml_plat, int agc_reg,
                                   u32 agcctl, u32 memclk, u8 agc_ver,
                                   u32 clkctrladdr)
{

    /* Release clock */
    if (agc_ver > 0)
        /* CLKGATEFCTRL0[AGCCLKFORCE]=0 */
        AML_REG_WRITE(memclk & ~BIT(29), aml_plat, AML_ADDR_SYSTEM,
                       clkctrladdr);
    else
        /* MEMCLKCTRL0[AGCMEMCLKCTRL]=1 */
        AML_REG_WRITE(memclk | BIT(3), aml_plat, AML_ADDR_SYSTEM,
                       clkctrladdr);

    /* Restart state machine: xxAGCCNTL0[AGCFSMRESET]=0 */
    AML_REG_WRITE(agcctl & ~BIT(12), aml_plat, AML_ADDR_SYSTEM, agc_reg);
}

/**
 * aml_plat_get_agc_load_version() - Return the agc load protocol version and the
 * address of the clock control register
 *
 * @aml_plat: platform data
 * @rf: rf in used
 * @clkctrladdr: returned clock control register address
 *
 * c.f Modem UM (AGC/CCA initialization)
 */
#ifndef CONFIG_AML_SDM
static u8 aml_plat_get_agc_load_version(struct aml_plat *aml_plat, u32 rf,
                                         u32 *clkctrladdr)
{
    u8 agc_load_ver = 0;
    u32 agc_ver;
    //u32 regval;

    *clkctrladdr = aml_plat_get_clkctrl_addr(aml_plat);

    /* Trident PHY use old method */
    if (rf ==  MDM_PHY_CONFIG_TRIDENT)
        return 0;

    /* Get the FPGA signature */
    //regval = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_SIGNATURE_ADDR);

    /* Read RIU version register */
    agc_ver = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, RIU_AMLVERSION_ADDR);
    agc_load_ver = __RIU_AGCLOAD_FROM_VERS(agc_ver);

    return agc_load_ver;
}
#endif /* CONFIG_AML_SDM */

/**
 * aml_plat_agc_load() - Load AGC ucode
 *
 * @aml_plat: platform data
 * c.f Modem UM (AGC/CCA initialization)
 */

static int aml_plat_agc_load(struct aml_plat *aml_plat)
{
    int ret = 0;
#ifndef CONFIG_AML_SDM
    u32 agc = 0, agcctl, memclk;
    u32 clkctrladdr;
    u32 rf = aml_plat_get_rf(aml_plat);
    u8 agc_ver;

    switch (rf) {
        case MDM_PHY_CONFIG_TRIDENT:
            agc = AGC_AMLAGCCNTL_ADDR;
            break;
        case MDM_PHY_CONFIG_CATAXIA:
        case MDM_PHY_CONFIG_KARST:
            agc = RIU_AMLAGCCNTL_ADDR;
            break;
        default:
            return -1;
    }

    agc_ver = aml_plat_get_agc_load_version(aml_plat, rf, &clkctrladdr);

    aml_plat_stop_agcfsm(aml_plat, agc, &agcctl, &memclk, agc_ver, clkctrladdr);

    ret = aml_plat_agc_download(aml_plat, AML_ADDR(aml_plat, AML_ADDR_SYSTEM, PHY_AGC_UCODE_ADDR));

    if (!ret && (agc_ver == 1)) {
        /* Run BIST to ensure that the AGC RAM was correctly loaded */
        AML_REG_WRITE(BIT(28), aml_plat, AML_ADDR_SYSTEM,
                       RIU_AMLDYNAMICCONFIG_ADDR);
        while (AML_REG_READ(aml_plat, AML_ADDR_SYSTEM,
                             RIU_AMLDYNAMICCONFIG_ADDR) & BIT(28));

        if (!(AML_REG_READ(aml_plat, AML_ADDR_SYSTEM,
                            RIU_AGCMEMBISTSTAT_ADDR) & BIT(0))) {
            dev_err(aml_platform_get_dev(aml_plat),
                    "AGC RAM not loaded correctly 0x%08x\n",
                    AML_REG_READ(aml_plat, AML_ADDR_SYSTEM,
                                  RIU_AGCMEMSIGNATURESTAT_ADDR));
            ret = -EIO;
        }
    }
    aml_plat_start_agcfsm(aml_plat, agc, agcctl, memclk, agc_ver, clkctrladdr);

#endif
    return ret;
}

/**
 * aml_ldpc_load() - Load LDPC RAM
 *
 * @aml_hw: Main driver data
 * c.f Modem UM (LDPC initialization)
 */
 #if 0
static int aml_ldpc_load(struct aml_hw *aml_hw)
{
#ifndef CONFIG_AML_SDM
    struct aml_plat *aml_plat = aml_hw->plat;
    u32 rf = aml_plat_get_rf(aml_plat);
    u32 phy_feat = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, MDM_HDMCONFIG_ADDR);
    u32 phy_vers = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, MDM_HDMVERSION_ADDR);

    if (((rf !=  MDM_PHY_CONFIG_KARST) && (rf !=  MDM_PHY_CONFIG_CATAXIA)) ||
        (phy_feat & (MDM_LDPCDEC_BIT | MDM_LDPCENC_BIT)) !=
        (MDM_LDPCDEC_BIT | MDM_LDPCENC_BIT)) {
        goto disable_ldpc;
    }

    // No need to load the LDPC RAM anymore on modems starting from version v31
    if (__MDM_VERSION(phy_vers) > 30) {
        return 0;
    }

    if (aml_plat_bin_fw_upload(aml_plat,
                            AML_ADDR(aml_plat, AML_ADDR_SYSTEM, PHY_LDPC_RAM_ADDR),
                            AML_LDPC_RAM_NAME)) {
        goto disable_ldpc;
    }

    return 0;

  disable_ldpc:
    aml_hw->mod_params->ldpc_on = false;

#endif /* CONFIG_AML_SDM */
    return 0;
}
#endif

/**
 * aml_plat_lmac_load() - Load FW code
 *
 * @aml_plat: platform data
 */
int aml_plat_lmac_load(struct aml_plat *aml_plat)
{
    int ret;

    #ifdef CONFIG_AML_TL4
    ret = aml_plat_tl4_fw_upload(aml_plat,
                                  AML_ADDR(aml_plat, AML_ADDR_CPU, RAM_LMAC_FW_ADDR),
                                  AML_MAC_FW_NAME);
    #else
    ret = aml_plat_fw_upload(aml_plat,
            (u8 *)AML_ADDR(aml_plat, AML_ADDR_CPU, RAM_LMAC_FW_ADDR),
            AML_MAC_FW_PCIE);
    #endif

    return ret;
}

/**
 * aml_rf_fw_load() - Load RF FW if any
 *
 * @aml_hw: Main driver data
 */
 #if 0
static int aml_plat_rf_fw_load(struct aml_hw *aml_hw)
{
#ifndef CONFIG_AML_SDM
    struct aml_plat *aml_plat = aml_hw->plat;
    u32 rf = aml_plat_get_rf(aml_plat);
    struct device *dev = aml_platform_get_dev(aml_plat);
    const struct firmware *fw;
    int err = 0;
    u8 const *file_data;
    int remain;
    u32 clkforce;
    u32 clkctrladdr;

    // Today only Cataxia has a FW to load
    if (rf !=  MDM_PHY_CONFIG_CATAXIA)
        return 0;

    err = request_firmware(&fw, AML_CATAXIA_FW_NAME, dev);
    if (err)
    {
        dev_err(dev, "Make sure your board has up-to-date packages.");
        dev_err(dev, "Run \"sudo smart update\" \"sudo smart upgrade\" commands.\n");
        return err;
    }

    file_data = fw->data;
    remain = fw->size;

    // Get address of clock control register
    clkctrladdr = aml_plat_get_clkctrl_addr(aml_plat);

    // Force RC clock
    clkforce = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, clkctrladdr);
    AML_REG_WRITE(clkforce | BIT(27), aml_plat, AML_ADDR_SYSTEM, clkctrladdr);
    mdelay(1);

    // Reset RC
    AML_REG_WRITE(0x00003100, aml_plat, AML_ADDR_SYSTEM, RC_SYSTEM_CONFIGURATION_ADDR);
    mdelay(20);

    // Reset RF
    AML_REG_WRITE(0x00123100, aml_plat, AML_ADDR_SYSTEM, RC_SYSTEM_CONFIGURATION_ADDR);
    mdelay(20);

    // Select trx 2 HB
    AML_REG_WRITE(0x00113100, aml_plat, AML_ADDR_SYSTEM, RC_SYSTEM_CONFIGURATION_ADDR);
    mdelay(50);

    // Set ASP freeze
    AML_REG_WRITE(0xC1010001, aml_plat, AML_ADDR_SYSTEM, RC_ACCES_TO_CATAXIA_REG_ADDR);
    mdelay(1);

    /* Walk through all the lines of the FW file */
    while (remain >= 10) {
        u32 data;

        if (sscanf(file_data, "0x%08X", &data) != 1)
        {
            // Corrupted FW file
            err = -1;
            break;
        }
        file_data += 11;
        remain -= 11;

        AML_REG_WRITE(data, aml_plat, AML_ADDR_SYSTEM, RC_ACCES_TO_CATAXIA_REG_ADDR);
        udelay(50);
    }

    // Clear ASP freeze
    AML_REG_WRITE(0xE0010011, aml_plat, AML_ADDR_SYSTEM, RC_ACCES_TO_CATAXIA_REG_ADDR);
    mdelay(1);

    // Unforce RC clock
    AML_REG_WRITE(clkforce, aml_plat, AML_ADDR_SYSTEM, clkctrladdr);

    release_firmware(fw);

#endif /* CONFIG_AML_SDM */
    return err;
}
#endif

/**
 * aml_plat_mpif_sel() - Select the MPIF according to the FPGA signature
 *
 * @aml_plat: platform data
 */
void aml_plat_mpif_sel(struct aml_plat *aml_plat)
{
#ifndef CONFIG_AML_SDM
    u32 regval;
    u32 type;

    /* Get the FPGA signature */
    regval = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_SIGNATURE_ADDR);
    type = __FPGA_TYPE(regval);

    /* Check if we need to switch to the old MPIF or not */
    if ((type != 0xCAFE) && (type != 0XC0CA) && (regval & 0xF) < 0x3)
    {
        /* A old FPGA A is used, so configure the FPGA B to use the old MPIF */
        AML_REG_WRITE(0x3, aml_plat, AML_ADDR_SYSTEM, FPGAB_MPIF_SEL_ADDR);
    }
#endif
}


/**
 * aml_platform_reset() - Reset the platform
 *
 * @aml_plat: platform data
 */
int aml_platform_reset(struct aml_plat *aml_plat)
{
    u32 regval_aml;
    u32 regval_cpu;
    u32 regval_status;

    /* the doc states that SOFT implies FPGA_B_RESET
     * adding FPGA_B_RESET is clearer */
    regval_aml = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);
    AML_REG_WRITE(SOFT_RESET | FPGA_B_RESET, aml_plat,
                   AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);

    if (aml_bus_type != USB_MODE) {
        regval_cpu = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_PMU_A22);
        regval_cpu |= CPU_RESET;
        AML_REG_WRITE(regval_cpu, aml_plat, AML_ADDR_AON, RG_PMU_A22);
    }

    regval_status = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_PMU_A16);
    regval_status &= ~BIT(30);
    regval_status &= ~BIT(31);
    AML_REG_WRITE(regval_status, aml_plat, AML_ADDR_AON, RG_PMU_A16);

    mdelay(10);

    regval_aml = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);
    regval_cpu = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_PMU_A22);
    regval_status = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_PMU_A16);
    AML_INFO(" regval_aml:%x, regval_cpu:%x, regval_status:%x\n", regval_aml, regval_cpu, regval_status);

    if ((regval_aml & SOFT_RESET) || (!(regval_cpu & CPU_RESET))) {
        AML_INFO("reset: failed\n");
        return -EIO;
    }

    AML_REG_WRITE(regval_aml & ~FPGA_B_RESET, aml_plat,
                   AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);
    mdelay(10);
    return 0;
}

/**
 * rwmx_platform_save_config() - Save hardware config before reload
 *
 * @aml_plat: Pointer to platform data
 *
 * Return configuration registers values.
 */
static void* aml_term_save_config(struct aml_plat *aml_plat)
{
    const u32 *reg_list = NULL;
    u32 *reg_value = NULL, *res = NULL;
    int i, size = 0;

    if (aml_plat->get_config_reg) {
        size = aml_plat->get_config_reg(aml_plat, &reg_list);
    }

    if (size <= 0)
        return NULL;

    res = kmalloc(sizeof(u32) * size, GFP_KERNEL);
    if (!res)
        return NULL;

    reg_value = res;
    for (i = 0; i < size; i++) {
        *reg_value++ = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM,
                                     *reg_list++);
    }

    return res;
}

/**
 * rwmx_platform_restore_config() - Restore hardware config after reload
 *
 * @aml_plat: Pointer to platform data
 * @reg_value: Pointer of value to restore
 * (obtained with rwmx_platform_save_config())
 *
 * Restore configuration registers value.
 */
static void aml_term_restore_config(struct aml_plat *aml_plat,
                                     u32 *reg_value)
{
    const u32 *reg_list = NULL;
    int i, size = 0;

    if (!reg_value || !aml_plat->get_config_reg)
        return;

    size = aml_plat->get_config_reg(aml_plat, &reg_list);

    for (i = 0; i < size; i++) {
        AML_REG_WRITE(*reg_value++, aml_plat, AML_ADDR_SYSTEM,
                       *reg_list++);
    }
}

#ifndef CONFIG_AML_FHOST
int aml_usb_check_fw_compatibility(struct aml_hw *aml_hw)
{
    struct ipc_shared_env_tag *shared = NULL;

    #ifdef CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->hw->wiphy;
    #else //CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->wiphy;
    #endif //CONFIG_AML_SOFTMAC
    #ifdef CONFIG_AML_OLD_IPC
    int ipc_shared_version = 10;
    #else //CONFIG_AML_OLD_IPC
    int ipc_shared_version = 11;
    #endif //CONFIG_AML_OLD_IPC
    int res = 0;

    shared = (struct ipc_shared_env_tag *)kzalloc(sizeof(struct ipc_shared_env_tag), GFP_KERNEL);
    if (!shared) {
        AML_ERR(" alloc shared failed!\n");
        return -ENOMEM;
    }

    aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)&(shared->comp_info),
        (unsigned char *)&(aml_hw->ipc_env->shared->comp_info),
        sizeof(struct compatibility_tag), USB_EP4);

    if (shared->comp_info.ipc_shared_version != ipc_shared_version)
    {
        wiphy_err(wiphy, "Different versions of IPC shared version between driver and FW (%d != %d)\n ",
                  ipc_shared_version, shared->comp_info.ipc_shared_version);
        res = -1;
    }

    if (shared->comp_info.radarbuf_cnt != IPC_RADARBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Radar events handling "\
                  "between driver and FW (%d != %d)\n", IPC_RADARBUF_CNT,
                  shared->comp_info.radarbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.unsuprxvecbuf_cnt != IPC_UNSUPRXVECBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for unsupported Rx vectors "\
                  "handling between driver and FW (%d != %d)\n", IPC_UNSUPRXVECBUF_CNT,
                  shared->comp_info.unsuprxvecbuf_cnt);
        res = -1;
    }

    #ifdef CONFIG_AML_FULLMAC
    if (shared->comp_info.rxdesc_cnt != IPC_RXDESC_CNT)
    {
        wiphy_err(wiphy, "Different number of shared descriptors available for Data RX handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXDESC_CNT,
                  shared->comp_info.rxdesc_cnt);
        res = -1;
    }
    #endif /* CONFIG_AML_FULLMAC */

    if (shared->comp_info.rxbuf_cnt != IPC_RXBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Data Rx handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXBUF_CNT,
                  shared->comp_info.rxbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.msge2a_buf_cnt != IPC_MSGE2A_BUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Emb->App MSGs "\
                  "sending between driver and FW (%d != %d)\n", IPC_MSGE2A_BUF_CNT,
                  shared->comp_info.msge2a_buf_cnt);
        res = -1;
    }

    if (shared->comp_info.dbgbuf_cnt != IPC_DBGBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for debug messages "\
                  "sending between driver and FW (%d != %d)\n", IPC_DBGBUF_CNT,
                  shared->comp_info.dbgbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.bk_txq != NX_TXDESC_CNT0)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BK TX queue (%d != %d)\n",
                  NX_TXDESC_CNT0, shared->comp_info.bk_txq);
        res = -1;
    }

    if (shared->comp_info.be_txq != NX_TXDESC_CNT1)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BE TX queue (%d != %d)\n",
                  NX_TXDESC_CNT1, shared->comp_info.be_txq);
        res = -1;
    }

    if (shared->comp_info.vi_txq != NX_TXDESC_CNT2)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VI TX queue (%d != %d)\n",
                  NX_TXDESC_CNT2, shared->comp_info.vi_txq);
        res = -1;
    }

    if (shared->comp_info.vo_txq != NX_TXDESC_CNT3)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VO TX queue (%d != %d)\n",
                  NX_TXDESC_CNT3, shared->comp_info.vo_txq);
        res = -1;
    }

    #if NX_TXQ_CNT == 5
    if (shared->comp_info.bcn_txq != NX_TXDESC_CNT4)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BCN TX queue (%d != %d)\n",
                NX_TXDESC_CNT4, shared->comp_info.bcn_txq);
        res = -1;
    }
    #else
    if (shared->comp_info.bcn_txq > 0)
    {
        wiphy_err(wiphy, "BCMC enabled in firmware but disabled in driver\n");
        res = -1;
    }
    #endif /* NX_TXQ_CNT == 5 */

    if (shared->comp_info.ipc_shared_size != sizeof(ipc_shared_env))
    {
        wiphy_err(wiphy, "Different sizes of IPC shared between driver and FW (%zd != %d)\n",
                  sizeof(ipc_shared_env), shared->comp_info.ipc_shared_size);
        res = -1;
    }

    if (shared->comp_info.msg_api != MSG_API_VER)
    {
        wiphy_err(wiphy, "Different supported message API versions between "\
                  "driver and FW (%d != %d)\n", MSG_API_VER, shared->comp_info.msg_api);
        res = -1;
    }

    kfree(shared);

    return res;
}

int aml_sdio_check_fw_compatibility(struct aml_hw *aml_hw)
{
    struct ipc_shared_env_tag *shared = NULL;
    #ifdef CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->hw->wiphy;
    #else //CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->wiphy;
    #endif //CONFIG_AML_SOFTMAC
    #ifdef CONFIG_AML_OLD_IPC
    int ipc_shared_version = 10;
    #else //CONFIG_AML_OLD_IPC
    int ipc_shared_version = 11;
    #endif //CONFIG_AML_OLD_IPC
    int res = 0;

    shared = (struct ipc_shared_env_tag *)kzalloc(sizeof(struct ipc_shared_env_tag), GFP_KERNEL);
    if (!shared) {
        AML_ERR(" alloc shared failed!\n");
        return -ENOMEM;
    }

    aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)&(shared->comp_info), (unsigned char *)&(aml_hw->ipc_env->shared->comp_info),
        sizeof(struct compatibility_tag));

    if (shared->comp_info.ipc_shared_version != ipc_shared_version)
    {
        wiphy_err(wiphy, "Different versions of IPC shared version between driver and FW (%d != %d)\n ",
                  ipc_shared_version, shared->comp_info.ipc_shared_version);
        res = -1;
    }

    if (shared->comp_info.radarbuf_cnt != IPC_RADARBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Radar events handling "\
                  "between driver and FW (%d != %d)\n", IPC_RADARBUF_CNT,
                  shared->comp_info.radarbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.unsuprxvecbuf_cnt != IPC_UNSUPRXVECBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for unsupported Rx vectors "\
                  "handling between driver and FW (%d != %d)\n", IPC_UNSUPRXVECBUF_CNT,
                  shared->comp_info.unsuprxvecbuf_cnt);
        res = -1;
    }

    #ifdef CONFIG_AML_FULLMAC
    if (shared->comp_info.rxdesc_cnt != IPC_RXDESC_CNT)
    {
        wiphy_err(wiphy, "Different number of shared descriptors available for Data RX handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXDESC_CNT,
                  shared->comp_info.rxdesc_cnt);
        res = -1;
    }
    #endif /* CONFIG_AML_FULLMAC */

    if (shared->comp_info.rxbuf_cnt != IPC_RXBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Data Rx handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXBUF_CNT,
                  shared->comp_info.rxbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.msge2a_buf_cnt != IPC_MSGE2A_BUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Emb->App MSGs "\
                  "sending between driver and FW (%d != %d)\n", IPC_MSGE2A_BUF_CNT,
                  shared->comp_info.msge2a_buf_cnt);
        res = -1;
    }

    if (shared->comp_info.dbgbuf_cnt != IPC_DBGBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for debug messages "\
                  "sending between driver and FW (%d != %d)\n", IPC_DBGBUF_CNT,
                  shared->comp_info.dbgbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.bk_txq != NX_TXDESC_CNT0)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BK TX queue (%d != %d)\n",
                  NX_TXDESC_CNT0, shared->comp_info.bk_txq);
        res = -1;
    }

    if (shared->comp_info.be_txq != NX_TXDESC_CNT1)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BE TX queue (%d != %d)\n",
                  NX_TXDESC_CNT1, shared->comp_info.be_txq);
        res = -1;
    }

    if (shared->comp_info.vi_txq != NX_TXDESC_CNT2)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VI TX queue (%d != %d)\n",
                  NX_TXDESC_CNT2, shared->comp_info.vi_txq);
        res = -1;
    }

    if (shared->comp_info.vo_txq != NX_TXDESC_CNT3)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VO TX queue (%d != %d)\n",
                  NX_TXDESC_CNT3, shared->comp_info.vo_txq);
        res = -1;
    }

    #if NX_TXQ_CNT == 5
    if (shared->comp_info.bcn_txq != NX_TXDESC_CNT4)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BCN TX queue (%d != %d)\n",
                NX_TXDESC_CNT4, shared->comp_info.bcn_txq);
        res = -1;
    }
    #else
    if (shared->comp_info.bcn_txq > 0)
    {
        wiphy_err(wiphy, "BCMC enabled in firmware but disabled in driver\n");
        res = -1;
    }
    #endif /* NX_TXQ_CNT == 5 */

    if (shared->comp_info.ipc_shared_size != sizeof(ipc_shared_env))
    {
        wiphy_err(wiphy, "Different sizes of IPC shared between driver and FW (%zd != %d)\n",
                  sizeof(ipc_shared_env), shared->comp_info.ipc_shared_size);
        res = -1;
    }

    if (shared->comp_info.msg_api != MSG_API_VER)
    {
        wiphy_err(wiphy, "Different supported message API versions between "\
                  "driver and FW (%d != %d)\n", MSG_API_VER, shared->comp_info.msg_api);
        res = -1;
    }

    kfree(shared);
    return res;
}

static int aml_pci_check_fw_compatibility(struct aml_hw *aml_hw)
{
    struct ipc_shared_env_tag *shared = aml_hw->ipc_env->shared;
    #ifdef CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->hw->wiphy;
    #else //CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = aml_hw->wiphy;
    #endif //CONFIG_AML_SOFTMAC
    #ifdef CONFIG_AML_OLD_IPC
    int ipc_shared_version = 10;
    #else //CONFIG_AML_OLD_IPC
    int ipc_shared_version = 11;
    #endif //CONFIG_AML_OLD_IPC
    int res = 0;

    if (shared->comp_info.ipc_shared_version != ipc_shared_version)
    {
        wiphy_err(wiphy, "Different versions of IPC shared version between driver and FW (%d != %d)\n ",
                  ipc_shared_version, shared->comp_info.ipc_shared_version);
        res = -1;
    }

    if (shared->comp_info.radarbuf_cnt != IPC_RADARBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Radar events handling "\
                  "between driver and FW (%d != %d)\n", IPC_RADARBUF_CNT,
                  shared->comp_info.radarbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.unsuprxvecbuf_cnt != IPC_UNSUPRXVECBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for unsupported Rx vectors "\
                  "handling between driver and FW (%d != %d)\n", IPC_UNSUPRXVECBUF_CNT,
                  shared->comp_info.unsuprxvecbuf_cnt);
        res = -1;
    }

    #ifdef CONFIG_AML_FULLMAC
    if (shared->comp_info.rxdesc_cnt != IPC_RXDESC_CNT)
    {
        wiphy_err(wiphy, "Different number of shared descriptors available for Data RX handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXDESC_CNT,
                  shared->comp_info.rxdesc_cnt);
        res = -1;
    }
    #endif /* CONFIG_AML_FULLMAC */

    if (shared->comp_info.rxbuf_cnt != IPC_RXBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Data Rx handling "\
                  "between driver and FW (%d != %d)\n", IPC_RXBUF_CNT,
                  shared->comp_info.rxbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.msge2a_buf_cnt != IPC_MSGE2A_BUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for Emb->App MSGs "\
                  "sending between driver and FW (%d != %d)\n", IPC_MSGE2A_BUF_CNT,
                  shared->comp_info.msge2a_buf_cnt);
        res = -1;
    }

    if (shared->comp_info.dbgbuf_cnt != IPC_DBGBUF_CNT)
    {
        wiphy_err(wiphy, "Different number of host buffers available for debug messages "\
                  "sending between driver and FW (%d != %d)\n", IPC_DBGBUF_CNT,
                  shared->comp_info.dbgbuf_cnt);
        res = -1;
    }

    if (shared->comp_info.bk_txq != NX_TXDESC_CNT0)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BK TX queue (%d != %d)\n",
                  NX_TXDESC_CNT0, shared->comp_info.bk_txq);
        res = -1;
    }

    if (shared->comp_info.be_txq != NX_TXDESC_CNT1)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BE TX queue (%d != %d)\n",
                  NX_TXDESC_CNT1, shared->comp_info.be_txq);
        res = -1;
    }

    if (shared->comp_info.vi_txq != NX_TXDESC_CNT2)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VI TX queue (%d != %d)\n",
                  NX_TXDESC_CNT2, shared->comp_info.vi_txq);
        res = -1;
    }

    if (shared->comp_info.vo_txq != NX_TXDESC_CNT3)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of VO TX queue (%d != %d)\n",
                  NX_TXDESC_CNT3, shared->comp_info.vo_txq);
        res = -1;
    }

    #if NX_TXQ_CNT == 5
    if (shared->comp_info.bcn_txq != NX_TXDESC_CNT4)
    {
        wiphy_err(wiphy, "Driver and FW have different sizes of BCN TX queue (%d != %d)\n",
                NX_TXDESC_CNT4, shared->comp_info.bcn_txq);
        res = -1;
    }
    #else
    if (shared->comp_info.bcn_txq > 0)
    {
        wiphy_err(wiphy, "BCMC enabled in firmware but disabled in driver\n");
        res = -1;
    }
    #endif /* NX_TXQ_CNT == 5 */

    if (shared->comp_info.ipc_shared_size != sizeof(ipc_shared_env))
    {
        wiphy_err(wiphy, "Different sizes of IPC shared between driver and FW (%zd != %d)\n",
                  sizeof(ipc_shared_env), shared->comp_info.ipc_shared_size);
        res = -1;
    }

    if (shared->comp_info.msg_api != MSG_API_VER)
    {
        wiphy_err(wiphy, "Different supported message API versions between "\
                  "driver and FW (%d != %d)\n", MSG_API_VER, shared->comp_info.msg_api);
        res = -1;
    }

    return res;
}

int aml_check_fw_compatibility(struct aml_hw *aml_hw)
{
    if (aml_bus_type == USB_MODE) {
        return aml_usb_check_fw_compatibility(aml_hw);
    } else if (aml_bus_type == SDIO_MODE) {
        return aml_sdio_check_fw_compatibility(aml_hw);
    } else {
        if (g_pci_shutdown) {
            AML_INFO("pci shutdown");
            return -1;
        }
        else
            return aml_pci_check_fw_compatibility(aml_hw);
    }
}

#endif /* !CONFIG_AML_FHOST */

unsigned int bbpll_init(struct aml_plat *aml_plat)
{
    RG_DPLL_A0_FIELD_T rg_dpll_a0;
    RG_DPLL_A1_FIELD_T rg_dpll_a1;
    RG_DPLL_A2_FIELD_T rg_dpll_a2;
    RG_DPLL_A3_FIELD_T rg_dpll_a3;
    RG_DPLL_A4_FIELD_T rg_dpll_a4;
    RG_DPLL_A5_FIELD_T rg_dpll_a5;
    RG_DPLL_A6_FIELD_T rg_dpll_a6;

    rg_dpll_a0.data = 0x00800060;  //close test path
    AML_REG_WRITE(rg_dpll_a0.data, aml_plat, AML_ADDR_AON, RG_DPLL_A0);

    rg_dpll_a1.data = 0x00000c02;
    AML_REG_WRITE(rg_dpll_a1.data, aml_plat, AML_ADDR_AON, RG_DPLL_A1);

    rg_dpll_a2.data = 0x00021f1f;
    AML_REG_WRITE(rg_dpll_a2.data, aml_plat, AML_ADDR_AON, RG_DPLL_A2);

    rg_dpll_a3.data = 0x00000020;
    AML_REG_WRITE(rg_dpll_a3.data, aml_plat, AML_ADDR_AON, RG_DPLL_A3);

    rg_dpll_a4.data = 0x0000000a;
    AML_REG_WRITE(rg_dpll_a4.data, aml_plat, AML_ADDR_AON, RG_DPLL_A4);

    rg_dpll_a5.data = 0x000000c0;
    AML_REG_WRITE(rg_dpll_a5.data, aml_plat, AML_ADDR_AON, RG_DPLL_A5);

    rg_dpll_a6.data = 0x00000000;
    AML_REG_WRITE(rg_dpll_a6.data, aml_plat, AML_ADDR_AON, RG_DPLL_A6);

    return 0;
}

unsigned int bbpll_start(struct aml_plat *aml_plat)
{
    //RG_DPLL_A0_FIELD_T rg_dpll_a0;
    RG_DPLL_A1_FIELD_T rg_dpll_a1;
    //RG_DPLL_A2_FIELD_T rg_dpll_a2;
    RG_DPLL_A3_FIELD_T rg_dpll_a3;
    //RG_DPLL_A4_FIELD_T rg_dpll_a4;
    //RG_DPLL_A5_FIELD_T rg_dpll_a5;
    RG_DPLL_A6_FIELD_T rg_dpll_a6;

    //1.enable PLL and set PLL configuration
    rg_dpll_a1.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A1);
    rg_dpll_a1.b.rg_bbpll_en = 0x1;
    AML_REG_WRITE(rg_dpll_a1.data, aml_plat, AML_ADDR_AON, RG_DPLL_A1);

    //delay 20us for LDO and Band-gap to establish the working state
    udelay(20);

    //2.disable PLL reset
    rg_dpll_a1.b.rg_bbpll_rst = 0x0;
    AML_REG_WRITE(rg_dpll_a1.data, aml_plat, AML_ADDR_AON, RG_DPLL_A1);

    //delay 20 us for lock detector
    udelay(20);

    //3.enable PLL lock-detecor
    rg_dpll_a3.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A3);
    rg_dpll_a3.b.rg_bbpll_lk_rst = 0;
    AML_REG_WRITE(rg_dpll_a3.data, aml_plat, AML_ADDR_AON, RG_DPLL_A3);

    //4.check PLL status
    rg_dpll_a6.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A6);
    if (rg_dpll_a6.b.ro_bbpll_done == 1)
    {
        AML_INFO("bbpll done !\n");
        return 1;
    }
    else
    {
        AML_ERR("bbpll start failed !\n");
        return 0;
    }
}

unsigned int bbpll_stop(struct aml_plat *aml_plat)
{
    RG_DPLL_A1_FIELD_T rg_dpll_a1;
    RG_DPLL_A3_FIELD_T rg_dpll_a3;

    //1.enable PLL and set PLL configuration
    rg_dpll_a1.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A1);
    rg_dpll_a1.b.rg_bbpll_en = 0x0;
    AML_REG_WRITE(rg_dpll_a1.data, aml_plat, AML_ADDR_AON, RG_DPLL_A1);
    udelay(5);

    rg_dpll_a1.b.rg_bbpll_rst = 0x1;
    AML_REG_WRITE(rg_dpll_a1.data, aml_plat, AML_ADDR_AON, RG_DPLL_A1);
    udelay(5);

    rg_dpll_a3.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A3);
    rg_dpll_a3.b.rg_bbpll_lk_rst = 1;
    AML_REG_WRITE(rg_dpll_a3.data, aml_plat, AML_ADDR_AON, RG_DPLL_A3);

    return 0;
}

void aml_tx_rx_buf_init(struct aml_hw *aml_hw)
{
    int i;
    struct aml_plat *aml_plat = aml_hw->plat;

    if (aml_bus_type != PCIE_MODE) {
        aml_hw->fw_buf_pos = RXBUF_START_ADDR;
        aml_hw->rx_buf_end =
                (aml_bus_type == SDIO_MODE) ?
                        RXBUF_END_ADDR_LARGE : USB_RXBUF_END_ADDR_LARGE;
        aml_hw->rx_buf_state = FW_BUFFER_EXPAND;
    } else {
        for (i = 0; i < 1024; i += 4) {
            AML_REG_WRITE(0, aml_plat, AML_ADDR_MAC_PHY, MAC_SRAM_BASE + i);
        }
    #ifdef CONFIG_LINUXPC_VERSION
        AML_REG_WRITE(UBUNTU_PC_VERSION, aml_plat, AML_ADDR_MAC_PHY, UBUNTU_SYNC_ADDR);
    #endif
    }
}

/* FIXME: move aml_usb_irq_urb_init() into w2_usb.c */
void aml_usb_irq_urb_init(struct aml_hw *aml_hw, struct usb_device *udev)
{
    struct urb *urb = &aml_hw->usb->urb;
    struct usb_ctrlrequest *req = &aml_hw->usb->req;

    /* fill in the devrequest structure */
    req->bRequestType = USB_CTRL_IN_REQTYPE;
    req->bRequest = CMD_USB_IRQ;
    req->wValue = 0;
    req->wIndex = 0;
    req->wLength = cpu_to_le16(sizeof(aml_hw->usb->fw_ptrs));

    usb_fill_control_urb(urb,
        udev,
        usb_rcvctrlpipe(udev, USB_EP0),
        (unsigned char *)req,
        aml_hw->usb->fw_ptrs,
        sizeof(aml_hw->usb->fw_ptrs),
        aml_irq_usb_hdlr,
        aml_hw);
}

int aml_sdio_create_thread(struct aml_hw *aml_hw)
{
    sema_init(&aml_hw->aml_irq_sem, 0);
    aml_hw->aml_irq_task_quit = 0;
    aml_hw->aml_irq_task = kthread_run(aml_irq_task, aml_hw, "aml_irq_task");
    if (IS_ERR(aml_hw->aml_irq_task)) {
        aml_hw->aml_irq_task = NULL;
        ERROR_DEBUG_OUT("create aml_irq_task error!!!!\n");
        return -1;
    }

    sema_init(&aml_hw->aml_rx_sem, 0);
    aml_hw->aml_rx_task_quit = 0;

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)))
        aml_hw->aml_rx_task = kthread_run(aml_rx_task, aml_hw, "aml_rx_task");
        if (IS_ERR(aml_hw->aml_rx_task)) {
            kthread_stop(aml_hw->aml_irq_task);
            aml_hw->aml_rx_task = NULL;
            ERROR_DEBUG_OUT("create aml_rx_task error!!!!\n");
            return -1;
        }
#else // template solution for S905L3A
        {
            aml_hw->aml_rx_task = kthread_create(aml_rx_task, aml_hw, "aml_rx_task", num_online_cpus() - 1);
            if (IS_ERR(aml_hw->aml_rx_task)) {
                kthread_stop(aml_hw->aml_irq_task);
                aml_hw->aml_rx_task = NULL;
                ERROR_DEBUG_OUT("create aml_rx_task error!!!!\n");
                return -1;
            }
            kthread_bind(aml_hw->aml_rx_task, num_online_cpus() - 1);
            wake_up_process(aml_hw->aml_rx_task);
        }
#endif

    sema_init(&aml_hw->aml_tx_sem, 0);
    aml_hw->aml_tx_task_quit = 0;
    aml_hw->aml_tx_task = kthread_run(aml_tx_task, aml_hw, "aml_tx_task");
    if (IS_ERR(aml_hw->aml_tx_task)) {
        kthread_stop(aml_hw->aml_irq_task);
        kthread_stop(aml_hw->aml_rx_task);
        aml_hw->aml_tx_task = NULL;
        ERROR_DEBUG_OUT("create aml_tx_task error!!!!\n");
        return -1;
    }

    sema_init(&aml_hw->aml_msg_sem, 0);
    aml_hw->aml_msg_task_quit = 0;
    aml_hw->aml_msg_task = kthread_run(aml_msg_task, aml_hw, "aml_msg_task");
    if (IS_ERR(aml_hw->aml_msg_task)) {
        kthread_stop(aml_hw->aml_irq_task);
        kthread_stop(aml_hw->aml_rx_task);
        kthread_stop(aml_hw->aml_tx_task);
        aml_hw->aml_msg_task = NULL;
        ERROR_DEBUG_OUT("create aml_msg_task error!!!!\n");
        return -1;
    }


    sema_init(&aml_hw->aml_txcfm_sem, 0);
    aml_hw->aml_txcfm_task_quit = 0;
    aml_hw->aml_txcfm_task = kthread_run(aml_tx_cfm_task, aml_hw, "aml_txcfm_task");
    if (IS_ERR(aml_hw->aml_txcfm_task)) {
        kthread_stop(aml_hw->aml_irq_task);
        kthread_stop(aml_hw->aml_rx_task);
        kthread_stop(aml_hw->aml_tx_task);
        kthread_stop(aml_hw->aml_msg_task);
        aml_hw->aml_txcfm_task = NULL;
        ERROR_DEBUG_OUT("create aml_txcfm_task error!!!!\n");
        return -1;
    }


    return 0;
}
void aml_sdio_destroy_thread(struct aml_hw *aml_hw)
{
    if (aml_hw->aml_irq_task) {
        init_completion(&aml_hw->aml_irq_completion);
        aml_hw->aml_irq_completion_init = 1;
        aml_hw->aml_irq_task_quit = 1;
        up(&aml_hw->aml_irq_sem);
        kthread_stop(aml_hw->aml_irq_task);
        wait_for_completion(&aml_hw->aml_irq_completion);
        aml_hw->aml_irq_task = NULL;
    }

    if (aml_hw->aml_rx_task) {
        init_completion(&aml_hw->aml_rx_completion);
        aml_hw->aml_rx_completion_init = 1;
        aml_hw->aml_rx_task_quit = 1;
        up(&aml_hw->aml_rx_sem);
        kthread_stop(aml_hw->aml_rx_task);
        wait_for_completion(&aml_hw->aml_rx_completion);
        aml_hw->aml_rx_task = NULL;
    }

    if (aml_hw->aml_tx_task) {
        init_completion(&aml_hw->aml_tx_completion);
        aml_hw->aml_tx_completion_init = 1;
        aml_hw->aml_tx_task_quit = 1;
        up(&aml_hw->aml_tx_sem);
        kthread_stop(aml_hw->aml_tx_task);
        wait_for_completion(&aml_hw->aml_tx_completion);
        aml_hw->aml_tx_task = NULL;
    }

    if (aml_hw->aml_msg_task) {
        init_completion(&aml_hw->aml_msg_completion);
        aml_hw->aml_msg_completion_init = 1;
        aml_hw->aml_msg_task_quit = 1;
        up(&aml_hw->aml_msg_sem);
        kthread_stop(aml_hw->aml_msg_task);
        wait_for_completion(&aml_hw->aml_msg_completion);
        aml_hw->aml_msg_task = NULL;
    }

    if (aml_hw->aml_txcfm_task) {
        init_completion(&aml_hw->aml_txcfm_completion);
        aml_hw->aml_txcfm_completion_init = 1;
        aml_hw->aml_txcfm_task_quit = 1;
        up(&aml_hw->aml_txcfm_sem);
        kthread_stop(aml_hw->aml_txcfm_task);
        wait_for_completion(&aml_hw->aml_txcfm_completion);
        aml_hw->aml_txcfm_task = NULL;
    }
}

int aml_cpufreq_boost_update(struct aml_hw *aml_hw)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    struct freq_qos_request *req = &aml_hw->qos_req;
    int ret = 0;

    if (!freq_qos_request_active(req)) {
        struct cpufreq_policy *policy = cpufreq_cpu_get(0);

        if (IS_ERR_OR_NULL(policy)) {
            AML_ERR("cpu0 policy not ready\n");
            return -EINVAL;
        }

        ret = freq_qos_add_request(&policy->constraints, req, FREQ_QOS_MIN,
                       FREQ_QOS_MIN_DEFAULT_VALUE);
        if (ret)
            AML_ERR("Failed to add max-freq constraint (%d)\n", ret);

        cpufreq_cpu_put(policy);
    }

    if (freq_qos_request_active(req)) {
        ret = freq_qos_update_request(req, cpufreq_quick_get_max(0));
        if (ret < 0)
            AML_ERR("freq_qos_update_request fail(%d)\n", ret);
    }
    return ret;
#endif
    return 0;
}

int aml_cpufreq_boost_remove(struct aml_hw *aml_hw)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    struct freq_qos_request *req = &aml_hw->qos_req;

    if (freq_qos_request_active(req))
        freq_qos_remove_request(req);
#endif
    return 0;
}

extern int coex_flag;
int aml_sdio_platform_on(struct aml_hw *aml_hw, void *config)
{
    u8 *shared_ram;
    u8 *shared_host_rxbuf = NULL;
    u8 *shared_host_rxdesc = NULL;
    struct aml_plat *aml_plat = aml_hw->plat;
    int ret;
    RG_DPLL_A6_FIELD_T rg_dpll_a6;
    unsigned int mac_clk_reg;

    if (aml_plat->enabled)
        return 0;

    if (aml_bus_type == SDIO_MODE) {
        aml_sdio_calibration();
    }

    rg_dpll_a6.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A6);

    /*bpll not init*/
    if (rg_dpll_a6.b.ro_bbpll_done != 1) {
        bbpll_init(aml_plat);
        ret = bbpll_start(aml_plat);
#ifdef CONFIG_PT_MODE
    if (!ret) {
        if (aml_bus_type == SDIO_MODE)
            aml_bus_state_detect_deinit();

        return -1;
    }
#endif
        AML_INFO("bbpll init ok!\n");
    } else {
        AML_INFO("bbpll already init,not need to init!\n");
    }

    //change cpu clock to 240M
    AML_REG_WRITE(CPU_CLK_VALUE, aml_plat, AML_ADDR_MAC_PHY, CPU_CLK_REG_ADDR);
    //change mac clock to 240M
    mac_clk_reg = AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);
    mac_clk_reg |= 0x30000;
    AML_REG_WRITE(mac_clk_reg, aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);
    aml_tx_rx_buf_init(aml_hw);
    aml_hw->dynabuf_stop_tx = 0;
    aml_hw->send_tx_stop_to_fw = 0;
    if (aml_platform_reset(aml_plat))
        return -1;

    aml_plat_mpif_sel(aml_plat);

    #ifndef CONFIG_AML_FHOST
    /* By default, we consider that there is only one RF in the system */
    aml_hw->phy.cnt = 1;
    #endif // CONFIG_AML_FHOST

    if ((ret = aml_plat_agc_load(aml_plat)))
        return ret;

    if (aml_bus_type == USB_MODE) {
        if ((ret = wifi_fw_download(AML_MAC_FW_USB)))
            return ret;

        if (ret = start_wifi())
            return ret;
    } else {
        aml_download_wifi_fw_img(AML_MAC_FW_SDIO);
    }

    shared_ram = (u8 *)SHARED_RAM_SDIO_START_ADDR;
    if (ret = aml_ipc_init(aml_hw, shared_ram, shared_host_rxbuf, shared_host_rxdesc))
        return ret;

    AML_REG_WRITE(BOOTROM_ENABLE, aml_plat, AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);

    //start firmware cpu
    AML_REG_WRITE(0x00070000, aml_plat, AML_ADDR_AON, RG_PMU_A22);
    /* wait for chip ready */
    while (!(AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, REG_OF_VENDOR_ID) == W2s_VENDOR_AMLOGIC_EFUSE)) {
        if (bus_state_detect.bus_err) {
            if (aml_hw->plat->disable)
                aml_hw->plat->disable(aml_hw);
            aml_ipc_deinit(aml_hw);
            return -1;
        }
        msleep(5);

#ifdef CONFIG_PT_MODE
        {
            static int wait_cnt = 0;
            wait_cnt++;
            if (wait_cnt > 200) {
                AML_ERR("error found! start FW fail!\n");
                wait_cnt = 0;
                return -1;
            }
        }
#endif
    };

#ifndef CONFIG_PT_MODE
#ifdef CONFIG_AML_DEBUGFS
    aml_fw_trace_config_filters(aml_get_shared_trace_buf(aml_hw),
                                 aml_ipc_fw_trace_desc_get(aml_hw),
                                 aml_hw->mod_params->ftl);
#endif
#endif

#ifndef CONFIG_AML_FHOST
    if (ret = aml_check_fw_compatibility(aml_hw)) {
        if (aml_hw->plat->disable)
            aml_hw->plat->disable(aml_hw);
        aml_ipc_deinit(aml_hw);
        return ret;
    }
#endif /* !CONFIG_AML_FHOST */

    if (config)
        aml_term_restore_config(aml_plat, config);

    aml_ipc_start(aml_hw);

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_IPC_ONGOING)) {
        aml_recy_flags_clr(AML_RECY_IPC_ONGOING);
    }
#endif
    if (aml_bus_type == USB_MODE) {
        aml_hw->usb = kzalloc(sizeof(*aml_hw->usb), GFP_DMA | GFP_ATOMIC);
        if (!aml_hw->usb) {
            AML_ERR("aml_hw->usb malloc fail!\n");
            return -ENOMEM;
        }

        usb_init_urb(&aml_hw->usb->urb);
    }

    if (aml_sdio_create_thread(aml_hw)) {
        if (aml_hw->usb) {
            usb_free_urb(&aml_hw->usb->urb);
            aml_hw->usb = NULL;
        }
        aml_sdio_destroy_thread(aml_hw);
        if (aml_hw->plat->disable)
            aml_hw->plat->disable(aml_hw);
        aml_ipc_deinit(aml_hw);
        return -ENOMEM;
    }

    aml_txbuf_list_init(aml_hw);
    aml_tx_cfmed_list_init(aml_hw);
#ifdef CONFIG_SDIO_TX_ENH
    aml_tx_cfm_param_init(aml_hw);
    aml_hw->irqless_flag = 0;
#endif
    aml_scan_results_list_init(aml_hw);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
    aml_hw->tsq = 10;
#else
    aml_hw->tsq = 0;
#endif
    if (aml_bus_type == SDIO_MODE) {
        aml_hw->g_tx_param.tx_page_once = SDIO_PAGE_MAX;
    } else {
        aml_hw->g_tx_param.tx_page_once = USB_PAGE_MAX;
    }
    aml_hw->g_tx_param.txcfm_trigger_tx_thr = TXCFM_TRIGGER_TX_THR;

    aml_amsdu_buf_list_init(aml_hw);
    if (aml_bus_type == SDIO_MODE) {
#ifdef CONFIG_AML_LA
         aml_hw->g_tx_param.tx_page_free_num = SDIO_TX_PAGE_NUM_LARGE; /* for tx large */
         aml_hw->g_tx_param.tx_page_tot_num = SDIO_TX_PAGE_NUM_LARGE;
#else
         aml_hw->g_tx_param.tx_page_free_num = SDIO_TX_PAGE_NUM_SMALL;
         aml_hw->g_tx_param.tx_page_tot_num = SDIO_TX_PAGE_NUM_SMALL;
#endif
        aml_sdio_scatter_reg_init(aml_hw);
        if (ret = aml_plat->enable(aml_hw)) {
            aml_plat->enabled = true;
            aml_platform_off(aml_hw, NULL);
            return ret;
        }
    }
    if (aml_bus_type == USB_MODE) {
        aml_hw->plat->usb_dev = g_udev;
        aml_usb_irq_urb_init(aml_hw, g_udev);
        aml_usb_irq_urb_submit(aml_hw);
        aml_hw->g_tx_param.tx_page_free_num = USB_TX_PAGE_NUM_SMALL;
        aml_hw->g_tx_param.tx_page_tot_num = USB_TX_PAGE_NUM_SMALL;
        aml_hw->trb_wait_time = USB_SEND_URB_DEFAULT_WAIT_TIME;
        USB_BEGIN_LOCK();
        coex_flag = 1;
        USB_END_LOCK();

    }
    aml_plat->enabled = true;
    aml_scatter_req_init(aml_hw);

    aml_tcp_delay_ack_init(aml_hw);
    AML_INFO("end\n");
    return 0;
}

#ifdef CONFIG_AML_PREALLOC_BUF_SKB
int aml_prealloc_rxbuf_task(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;
    struct sk_buff *skb = NULL;
    uint32_t i = 0;

    aml_sched_rt_set(SCHED_RR, AML_TASK_PRI);
    while (!aml_hw->prealloc_task_quit) {
        if (down_interruptible(&aml_hw->prealloc_rxbuf_sem) != 0) {
            AML_INFO("prealloc: wait semaphore failed");
            break;
        }
        if (aml_hw->prealloc_task_quit) {
            AML_INFO("prealloc: task quit");
            break;
        }

        while (!list_empty(&aml_hw->prealloc_rxbuf_used)) {
            skb = dev_alloc_skb(aml_hw->ipc_env->rxbuf_sz);
            if (unlikely(!skb)) {
                if ((i++ % 20) == 0) {
                    AML_INFO("prealloc: new skb size(%u) failed(=%u)",
                            aml_hw->ipc_env->rxbuf_sz, i);
                }
                continue;
            }
            prealloc_rxbuf = aml_prealloc_get_used_rxbuf(aml_hw);
            if (!prealloc_rxbuf) {
                AML_INFO("prealloc: rxbuf is null");
                dev_kfree_skb(skb);
                continue;
            }
            prealloc_rxbuf->skb = skb;
            spin_lock_bh(&aml_hw->prealloc_rxbuf_lock);
            AML_INFO("prealloc: new skb(%d)=%p", aml_hw->prealloc_rxbuf_count, skb);
            aml_hw->prealloc_rxbuf_count--;
            spin_unlock_bh(&aml_hw->prealloc_rxbuf_lock);
        }
    }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
    complete_and_exit(&aml_hw->prealloc_completion, 0);
#else
    complete(&aml_hw->prealloc_completion);
#endif
    return 0;
}

int aml_pci_create_thread(struct aml_hw *aml_hw)
{
    sema_init(&aml_hw->prealloc_rxbuf_sem, 0);

    aml_hw->prealloc_task_quit = 0;
    aml_hw->prealloc_rxbuf_task =
        kthread_run(aml_prealloc_rxbuf_task, aml_hw, "prealloc_rxbuf_task");
    if (IS_ERR(aml_hw->prealloc_rxbuf_task)) {
        aml_hw->prealloc_rxbuf_task = NULL;
        AML_INFO("prealloc: create task failed!");
        return -1;
    }
    return 0;
}

void aml_pci_destroy_thread(struct aml_hw *aml_hw)
{
    if (aml_hw->prealloc_rxbuf_task) {
        AML_INFO("prealloc: destroy task");
        init_completion(&aml_hw->prealloc_completion);
        aml_hw->prealloc_task_quit = 1;
        up(&aml_hw->prealloc_rxbuf_sem);
        kthread_stop(aml_hw->prealloc_rxbuf_task);
        wait_for_completion(&aml_hw->prealloc_completion);
        aml_hw->prealloc_rxbuf_task = NULL;
    }
}
#endif

/**
 * aml_platform_on() - Start the platform
 *
 * @aml_hw: Main driver data
 * @config: Config to restore (NULL if nothing to restore)
 *
 * It starts the platform :
 * - load fw and ucodes
 * - initialize IPC
 * - boot the fw
 * - enable link communication/IRQ
 *
 * Called by 802.11 part
 */
int aml_pci_platform_on(struct aml_hw *aml_hw, void *config)
{
    u8 *shared_ram;
    struct aml_plat *aml_plat = aml_hw->plat;
    int ret;
    RG_DPLL_A6_FIELD_T rg_dpll_a6;
    unsigned int mac_clk_reg;
    u32 temp_data;
    u8 *shared_host_rxbuf;
    u8 *shared_host_rxdesc;

    if (aml_plat->enabled)
        return 0;

     rg_dpll_a6.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A6);
    /*bpll not init*/
    if (rg_dpll_a6.b.ro_bbpll_done != 1) {
        bbpll_init(aml_plat);
        bbpll_start(aml_plat);
        AML_INFO("bbpll init ok!\n");
    } else {
        AML_INFO("bbpll already init,not need to init!\n");
    }

    //change cpu clock to 240M
    AML_REG_WRITE(CPU_CLK_VALUE, aml_plat,
                   AML_ADDR_MAC_PHY, CPU_CLK_REG_ADDR);

    // pcie Priority adjustment
    AML_REG_WRITE(0xF7468800, aml_plat, AML_ADDR_MAC_PHY, MAC_AHBABT_CONTROL0);
    temp_data = AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, MAC_AHBABT_CONTROL1);
    temp_data &= ~0x7;
    AML_REG_WRITE(temp_data, aml_plat, AML_ADDR_MAC_PHY, MAC_AHBABT_CONTROL1);
    temp_data |= 0x6;
    AML_REG_WRITE(temp_data, aml_plat, AML_ADDR_MAC_PHY, MAC_AHBABT_CONTROL1);

    //change mac clock to 240M
    mac_clk_reg = AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);
    mac_clk_reg |= 0x30000;
    AML_REG_WRITE(mac_clk_reg, aml_plat, AML_ADDR_MAC_PHY, RG_INTF_MACCORE_CLK);

    aml_tx_rx_buf_init(aml_hw);

    if (aml_platform_reset(aml_plat))
        return -1;

    aml_plat_mpif_sel(aml_plat);

#ifndef CONFIG_AML_FHOST
    /* By default, we consider that there is only one RF in the system */
    aml_hw->phy.cnt = 1;
#endif // CONFIG_AML_FHOST

    if ((ret = aml_plat_agc_load(aml_plat)))
        return ret;

    if ((ret = aml_plat_lmac_load(aml_plat)))
        return ret;

    shared_ram = (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_PCI_START_ADDR);
    shared_host_rxbuf = (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_HOST_RXBUF_ADDR);
    shared_host_rxdesc = (u8 *)AML_ADDR(aml_plat, AML_ADDR_SYSTEM, SHARED_RAM_HOST_RXDESC_ADDR);
    if ((ret = aml_ipc_init(aml_hw, shared_ram, shared_host_rxbuf, shared_host_rxdesc)))
        return ret;

#ifdef CONFIG_AML_USE_TASK
    aml_task_init(aml_hw);
#endif

    if ((ret = aml_plat->enable(aml_hw)))
        return ret;
    AML_REG_WRITE(BOOTROM_ENABLE, aml_plat,
                   AML_ADDR_SYSTEM, SYSCTRL_MISC_CNTL_ADDR);

    //start firmware cpu, Bit23 and Bit7 reset efuse
    AML_REG_WRITE(0x00070000, aml_plat, AML_ADDR_AON, RG_PMU_A22);

    //check W2 fw whether is ready
    aml_get_vid(aml_plat);

#ifdef CONFIG_AML_DEBUGFS
    aml_fw_trace_config_filters(aml_get_shared_trace_buf(aml_hw),
                                 aml_ipc_fw_trace_desc_get(aml_hw),
                                 aml_hw->mod_params->ftl);
#endif

    #ifndef CONFIG_AML_FHOST
    if ((ret = aml_check_fw_compatibility(aml_hw)))
    {
        if (aml_hw->plat->disable)
            aml_hw->plat->disable(aml_hw);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
#ifndef CONFIG_AML_USE_TASK
        tasklet_kill(&aml_hw->task);
#endif
#else
        tasklet_kill(&aml_hw->task);
#endif

        aml_ipc_deinit(aml_hw);
        return ret;
    }
    #endif /* !CONFIG_AML_FHOST */

    if (config)
        aml_term_restore_config(aml_plat, config);

    aml_ipc_start(aml_hw);

#ifdef CONFIG_AML_PREALLOC_BUF_SKB
    if (aml_pci_create_thread(aml_hw)) {
        AML_INFO("create thread failed");
        return -1;
    }
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
    aml_hw->tsq = 10;
#else
    aml_hw->tsq = 0;
#endif
    aml_plat->enabled = true;
    aml_tcp_delay_ack_init(aml_hw);

    return 0;
}

int aml_platform_on(struct aml_hw *aml_hw, void *config)
{
    int ret;
    if (aml_bus_type != PCIE_MODE) {
        ret = aml_sdio_platform_on(aml_hw, config);
    } else {
        ret = aml_pci_platform_on(aml_hw, config);
    }
    return ret;
}

/**
 * aml_platform_off() - Stop the platform
 *
 * @aml_hw: Main driver data
 * @config: Updated with pointer to config, to be able to restore it with
 * aml_platform_on(). It's up to the caller to free the config. Set to NULL
 * if configuration is not needed.
 *
 * Called by 802.11 part
 */
void aml_platform_off(struct aml_hw *aml_hw, void **config)
{
    if (!aml_hw->plat->enabled) {
        if (config)
            *config = NULL;
        return;
    }

    aml_ipc_stop(aml_hw);
    if (aml_bus_type != PCIE_MODE) {
         aml_sdio_destroy_thread(aml_hw);
    }

    if (config)
        *config = aml_term_save_config(aml_hw->plat);

    if (aml_hw->plat->disable)
        aml_hw->plat->disable(aml_hw);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
#ifndef CONFIG_AML_USE_TASK
    tasklet_kill(&aml_hw->task);
#endif
#else
    tasklet_kill(&aml_hw->task);
#endif

    aml_ipc_deinit(aml_hw);
#ifdef CONFIG_AML_PREALLOC_BUF_SKB
    if (aml_bus_type == PCIE_MODE) {
        aml_pci_destroy_thread(aml_hw);
    }
#endif

#ifdef CONFIG_AML_USE_TASK
    aml_task_deinit(aml_hw);
#endif

    aml_platform_reset(aml_hw->plat);
    if (aml_bus_type != PCIE_MODE) {
        aml_hw->host_buf = NULL;
        aml_txbuf_list_deinit(aml_hw);
#ifndef CONFIG_AML_PREALLOC_BUF_STATIC
        aml_amsdu_buf_list_deinit(aml_hw);
#endif
    }
    if (aml_hw->usb) {
        usb_kill_urb(&aml_hw->usb->urb);
        usb_free_urb(&aml_hw->usb->urb);
        aml_hw->usb = NULL;
    }
    aml_hw->plat->enabled = false;
}

/**
 * aml_platform_init() - Initialize the platform
 *
 * @aml_plat: platform data (already updated by platform driver)
 * @platform_data: Pointer to store the main driver data pointer (aka aml_hw)
 *                That will be set as driver data for the platform driver
 * Return: 0 on success, < 0 otherwise
 *
 * Called by the platform driver after it has been probed
 */
int aml_platform_init(struct aml_plat *aml_plat, void **platform_data)
{
    AML_DBG(AML_FN_ENTRY_STR);

    aml_plat->enabled = false;

#if defined CONFIG_AML_SOFTMAC
    return aml_mac80211_init(aml_plat, platform_data);
#elif defined CONFIG_AML_FULLMAC
    return aml_cfg80211_init(aml_plat, platform_data);
#elif defined CONFIG_AML_FHOST
    return aml_fhost_init(aml_plat, platform_data);
#endif
}

/**
 * aml_platform_deinit() - Deinitialize the platform
 *
 * @aml_hw: main driver data
 *
 * Called by the platform driver after it is removed
 */
void aml_platform_deinit(struct aml_hw *aml_hw)
{
    AML_DBG(AML_FN_ENTRY_STR);

#if defined CONFIG_AML_SOFTMAC
    aml_mac80211_deinit(aml_hw);
#elif defined CONFIG_AML_FULLMAC
    aml_cfg80211_deinit(aml_hw);
#elif defined CONFIG_AML_FHOST
    aml_fhost_deinit(aml_hw);
#endif
}

#define AML_BASE_ADDR  0x60000000
static unsigned char *aml_get_address(struct aml_plat *aml_plat, int addr_name,
                               unsigned int offset)
{
    unsigned char *addr = NULL;

    if (addr_name == AML_ADDR_SYSTEM) {
        addr = (unsigned char *)(unsigned long)(offset + AML_BASE_ADDR);
    } else {
        addr = (unsigned char *)(unsigned long)offset;
    }

    return addr;
}

static inline int aml_hw_fw_rx_pos_update(struct aml_hw *aml_hw, u32 fw_rx_pos)
{
    unsigned int buf_state = fw_rx_pos & FW_BUFFER_STATUS;
    u32 fw_pos = SHARED_MEM_BASE_ADDR + (fw_rx_pos & ~(FW_BUFFER_STATUS | RX_WRAP_TEMP_FLAG));

    if (fw_rx_pos == 0)
        return 0;

    if (fw_pos < RXBUF_START_ADDR || fw_pos >= 0x60080000) {
        AML_ERR("fw_rx_pos %x fw_pos %x\n", fw_rx_pos, fw_pos);
        /* WARN_ON(1); */
        return -1;
    }

    aml_hw->fw_new_pos = fw_pos;
    if (fw_rx_pos & RX_WRAP_TEMP_FLAG)
        aml_hw->fw_new_pos |= RX_WRAP_FLAG;

    if (buf_state) {
        aml_hw->rx_buf_state &= ~FW_BUFFER_STATUS;
        if (buf_state & FW_BUFFER_NARROW) {
            aml_hw->rx_buf_state |= buf_state | BUFFER_NARROW;
        } else {
            aml_hw->rx_buf_state |= buf_state | BUFFER_EXPAND;
        }
    }
    return 0;
}

static u32 aml_usb_ack_irq(struct aml_hw *aml_hw)
{
    u32 fw_rx_pos = 0;
    u32 istatus = 0;

    if (bus_state_detect.bus_err)
        return 0;

    fw_rx_pos = __le32_to_cpu(aml_hw->usb->fw_ptrs[0]);
    istatus = __le32_to_cpu(aml_hw->usb->fw_ptrs[1]);

    /* reset the cache of interrupt status to prevent the caller dead-loop */
    aml_hw->usb->fw_ptrs[1] = 0;

    if (aml_hw_fw_rx_pos_update(aml_hw, fw_rx_pos))
        return 0;

    return istatus;
}

int aml_sdio_intr_read(struct aml_hw *aml_hw, u32 *fw_rx_pos, u32 *istatus)
{
    u32 regs[2] = { 0 };

    /* FIXME: return -1 if read failed */
    aml_hw->plat->hif_sdio_ops->hi_desc_read(regs,
            (unsigned char *)RG_WIFI_IF_FW2HST_IRQ_CFG, sizeof(regs));

    if (fw_rx_pos)
        *fw_rx_pos = regs[0];
    if (istatus)
        *istatus = regs[1];

    return 0;
}

static u32 aml_sdio_ack_irq(struct aml_hw *aml_hw)
{
    u32 fw_rx_pos = 0;
    u32 istatus = 0;
    int ret;

    if (bus_state_detect.bus_err)
        return 0;

    ret = aml_sdio_intr_read(aml_hw, &fw_rx_pos, &istatus);
    if (ret || aml_hw_fw_rx_pos_update(aml_hw, fw_rx_pos))
        return 0;

    return istatus;
}

static u32 aml_pci_ack_irq(struct aml_hw *aml_hw)
{
    u32 istatus = AML_REG_READ(aml_hw->plat, AML_ADDR_MAC_PHY, ISTATUS_HOST);

    // clean pci irq status
    AML_REG_WRITE(istatus, aml_hw->plat, AML_ADDR_MAC_PHY, ISTATUS_HOST);

    return istatus;
}

int aml_platform_register_usb_drv(void)
{
    int ret = 0;
    struct aml_plat *aml_plat;
    void *drv_data = NULL;

    if (!auc_driver_insmoded) {
        ret = aml_usb_insmod();
    }
    if ((!g_usb_after_probe) || wifi_drv_rmmod_ongoing) {
         AML_INFO("***** please confirm wether the usb is probe or w2_comm.ko rmmod success last time\n");
         return -ENODEV;
    }

    aml_plat = kzalloc(sizeof(struct aml_plat), GFP_KERNEL);
    if (!aml_plat)
        return -ENOMEM;

    AML_FN_ENTRY();

    aml_plat->usb_dev = g_udev;
    aml_plat->hif_ops = &g_auc_hif_ops;

    ipc_basic_address = (u8 *)IPC_BASIC_ADDRESS;
    aml_plat->get_address = aml_get_address;
    aml_plat->ack_irq = aml_usb_ack_irq;

    aml_platform_init(aml_plat, &drv_data);
    dev_set_drvdata(&aml_plat->usb_dev->dev, drv_data);
    // if usb disconnect, system can't get @drv_data from dev, so we save it
    g_aml_hw = drv_data;
    bus_state_detect.is_drv_load_finished = 1;
    aml_log_nl_init();
    return ret;
}

void aml_platform_unregister_usb_drv(void)
{
    struct aml_hw *aml_hw;
    struct aml_plat *aml_plat;

    AML_DBG(AML_FN_ENTRY_STR);
    aml_log_nl_destroy();
    aml_hw = dev_get_drvdata(&g_udev->dev);
    // if usb disconnect, system can't get @drv_data from dev
    if (aml_hw == NULL) {
        if (g_aml_hw) {
            aml_hw = g_aml_hw;
            g_aml_hw = NULL;
        } else {
            AML_INFO("can't get aml_hw, need to check\n");
            goto err_drvdata;
        }
    }

    aml_plat = aml_hw->plat;
    aml_platform_deinit(aml_hw);
    wifi_drv_rmmod_ongoing = 1;
    bus_state_detect.is_drv_load_finished = 0;

err_drvdata:
    kfree(aml_plat);
    dev_set_drvdata(&g_udev->dev, NULL);
}

#ifndef CONFIG_LINUXPC_VERSION
extern int wifi_irq_num(void);
#endif
static int aml_pci_platform_enable(struct aml_hw *aml_hw)
{
    int ret;
#ifdef CONFIG_PT_MODE
    struct sdio_func *func = aml_priv_to_func(SDIO_FUNC1);
#else
    unsigned int irq_flag = 0;
#endif

    if (aml_bus_type == SDIO_MODE) {
#ifdef CONFIG_PT_MODE
        dev_set_drvdata(&func->dev, aml_hw);
        sdio_claim_host(func);
        sdio_claim_irq(func, aml_irq_sdio_hdlr_for_pt);
        sdio_release_host(func);
        AML_INFO(" claim_irq ret=%d\n", ret);
#else
#ifndef CONFIG_LINUXPC_VERSION
        aml_hw->irq = wifi_irq_num();
        irq_flag = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
        AML_INFO(" irq_flag=0x%x  irq=%d\n", irq_flag,  aml_hw->irq);
        ret = request_irq(aml_hw->irq, aml_irq_sdio_hdlr, irq_flag, "aml", aml_hw);
        AML_INFO(" request_irq ret=%d\n", ret);
#endif
#endif
    } else if (aml_bus_type == PCIE_MODE) {
        /* sched_setscheduler on ONESHOT threaded irq handler for BCNs ? */
        ret = request_irq(aml_hw->plat->pci_dev->irq, aml_irq_pcie_hdlr, 0,
                          "aml", aml_hw);
        AML_INFO("irq:%d, ret:%d", aml_hw->plat->pci_dev->irq, ret);
    } else {
        ret = -1;
    }
    return ret;
}

static int aml_pci_platform_disable(struct aml_hw *aml_hw)
{
#ifdef CONFIG_PT_MODE
    struct sdio_func *func = aml_priv_to_func(SDIO_FUNC1);

    sdio_claim_host(func);
    sdio_release_irq(func);
    sdio_release_host(func);
#else
    if (aml_bus_type == SDIO_MODE) {
        free_irq(aml_hw->irq, aml_hw);
    } else if (aml_bus_type == PCIE_MODE) {
        free_irq(aml_hw->plat->pci_dev->irq, aml_hw);
    }
#endif

    return 0;
}

int aml_platform_register_sdio_drv(void)
{
    int ret = 0;
    struct aml_plat *aml_plat;
    void *drv_data = NULL;
    struct sdio_func *func = aml_priv_to_func(SDIO_FUNC7);

    if (!g_sdio_driver_insmoded) {
        ret = aml_sdio_init();
    }
    if ((!g_sdio_after_porbe) || wifi_drv_rmmod_ongoing) {
         AML_INFO("***** please confirm wether the sdio is probe or w2_comm.ko rmmod success last time\n");
         return -ENODEV;
    }

    aml_plat = kzalloc(sizeof(struct aml_plat), GFP_KERNEL);
    if (!aml_plat)
        return -ENOMEM;

    aml_plat->enable = aml_pci_platform_enable;
    aml_plat->disable = aml_pci_platform_disable;
    aml_plat->ack_irq = aml_sdio_ack_irq;

    aml_plat->dev = &func->dev;
    aml_plat->hif_sdio_ops = &g_hif_sdio_ops;
    bus_state_detect.insmod_drv = aml_platform_register_sdio_drv;

    ipc_basic_address = (u8 *)IPC_BASIC_ADDRESS;
    aml_plat->get_address = aml_get_address;

    ret = aml_platform_init(aml_plat, &drv_data);
    if (ret) {
       kfree(aml_plat);
#ifdef CONFIG_PT_MODE
       return ret;
#endif
       if (!bus_state_detect.is_load_by_timer && !bus_state_detect.bus_reset_ongoing) {
           bus_state_detect.bus_err = 2;
       }
       AML_INFO("aml_platform_init error, ret: %d !!!\n", ret);
       return 0;
    }
    dev_set_drvdata(&func->dev, drv_data);
    g_aml_hw = drv_data;

#ifdef CONFIG_AML_RX_SG
    g_mmc_misc = kmalloc(sizeof(struct mmc_misc) * RXDESC_CNT_READ_ONCE, GFP_ATOMIC);
#endif
    bus_state_detect.is_drv_load_finished = 1;
    aml_log_nl_init();

    return ret;
}

void aml_platform_unregister_sdio_drv(void)
{
    struct aml_hw *aml_hw;
    struct aml_plat *aml_plat;
    struct sdio_func *func = aml_priv_to_func(SDIO_FUNC7);

    AML_DBG(AML_FN_ENTRY_STR);
    aml_log_nl_destroy();
    aml_hw = dev_get_drvdata(&func->dev);
    if (aml_hw == NULL) {
        if (g_aml_hw) {
            aml_hw = g_aml_hw;
            g_aml_hw = NULL;
        } else {
            AML_INFO("can't get aml_hw, need to check\n");
            goto err_drvdata;
        }
    }

    aml_plat = aml_hw->plat;
    aml_platform_deinit(aml_hw);
    wifi_drv_rmmod_ongoing = 1;
    bus_state_detect.is_drv_load_finished = 0;
err_drvdata:
    kfree(aml_plat);
    dev_set_drvdata(&func->dev, NULL);
}


u8* aml_pci_get_map_address(struct net_device *dev, unsigned int offset)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_plat *aml_plat = aml_hw->plat;
    struct aml_pci *aml_pci = (struct aml_pci *)aml_plat->priv;

    if (!aml_pci) {
        return NULL;
    }

#ifdef CONFIG_AML_FPGA_PCIE
    //fpga bar0 0x6000_0000~0x603f_ffff 4M
    //fpga bar1 0x0020_0000~0x004f_ffff 4M
    //fpga bar2 0x00c0_0000~0x00ff_ffff 4M
    //fpga bar3 0x00a0_0000~0x00af_ffff 1M
    //fpga bar4 0x0000_0000~0x0007_ffff 512K
    //fpga bar5 0x6080_0000~0x60ff_ffff 8M
    if (offset >= 0x60000000 && offset <= 0x603fffff) {
        return ( aml_pci->pci_bar0_vaddr + (offset - 0x60000000));

    } else if (offset >= 0x00200000 && offset <= 0x004fffff) {
       return ( aml_pci->pci_bar1_vaddr + (offset - 0x00200000));

    } else if (offset >= 0x00c00000 && offset <= 0x00ffffff) {
        return ( aml_pci->pci_bar2_vaddr + (offset - 0x00c00000));

    } else if (offset >= 0x00a00000 && offset <= 0x00afffff) {
        return ( aml_pci->pci_bar3_vaddr + (offset - 0x00a00000));

    } else if (offset <= 0x0007ffff) {
        return ( aml_pci->pci_bar4_vaddr + offset);

    } else if (offset >= 0x60800000 && offset <= 0x60ffffff) {
        return ( aml_pci->pci_bar5_vaddr + (offset - 0x60800000));

    } else {
        AML_ERR("offset error \n");
        return NULL;
    }
#else
    // bar2 table0 address
    if (offset >=PCIE_BAR2_TABLE0_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE0_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE0_OFFSET + (offset - PCIE_BAR2_TABLE0_EP_BASE_ADDR);
    }

    // bar2 table1 address
    if (offset < PCIE_BAR2_TABLE1_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE1_OFFSET + (offset - PCIE_BAR2_TABLE1_EP_BASE_ADDR);
    }

    // bar2 table2 address
    if (offset >=PCIE_BAR2_TABLE2_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE2_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE2_OFFSET + (offset - PCIE_BAR2_TABLE2_EP_BASE_ADDR);
    }

    // bar2 table3 address
    if (offset >=PCIE_BAR2_TABLE3_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE3_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE3_OFFSET + (offset - PCIE_BAR2_TABLE3_EP_BASE_ADDR);
    }

    // bar2 table4 address
    if (offset >=PCIE_BAR2_TABLE4_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE4_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE4_OFFSET + (offset - PCIE_BAR2_TABLE4_EP_BASE_ADDR);
    }

    // bar2 table5 address
    if (offset >=PCIE_BAR2_TABLE5_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE5_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE5_OFFSET + (offset - PCIE_BAR2_TABLE5_EP_BASE_ADDR);
    }

    // bar2 table6 address
    if (offset >=PCIE_BAR2_TABLE6_EP_BASE_ADDR && offset < PCIE_BAR2_TABLE6_EP_END_ADDR) {
        return aml_pci->pci_bar2_vaddr + PCIE_BAR2_TABLE6_OFFSET + (offset - PCIE_BAR2_TABLE6_EP_BASE_ADDR);
    }

    // bar4 table0 address
    if (offset >=PCIE_BAR4_TABLE0_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE0_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE0_OFFSET + (offset - PCIE_BAR4_TABLE0_EP_BASE_ADDR);
    }

    // bar4 table1 address
    if (offset >=PCIE_BAR4_TABLE1_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE1_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE1_OFFSET + (offset - PCIE_BAR4_TABLE1_EP_BASE_ADDR);
    }

    // bar4 table2 address
    if (offset >=PCIE_BAR4_TABLE2_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE2_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE2_OFFSET + (offset - PCIE_BAR4_TABLE2_EP_BASE_ADDR);
    }

    // bar4 table3 address
    if (offset >=PCIE_BAR4_TABLE3_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE3_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE3_OFFSET + (offset - PCIE_BAR4_TABLE3_EP_BASE_ADDR);
    }

    // bar4 table4 address
    if (offset >=PCIE_BAR4_TABLE4_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE4_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE4_OFFSET + (offset - PCIE_BAR4_TABLE4_EP_BASE_ADDR);
    }

    // bar4 table5 address
    if (offset >=PCIE_BAR4_TABLE5_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE5_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE5_OFFSET + (offset - PCIE_BAR4_TABLE5_EP_BASE_ADDR);
    }

    // bar4 table6 address
    if (offset >=PCIE_BAR4_TABLE6_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE6_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE6_OFFSET + (offset - PCIE_BAR4_TABLE6_EP_BASE_ADDR);
    }

    // bar4 table7 address
    if (offset >=PCIE_BAR4_TABLE7_EP_BASE_ADDR && offset < PCIE_BAR4_TABLE7_EP_END_ADDR) {
        return aml_pci->pci_bar4_vaddr + PCIE_BAR4_TABLE7_OFFSET + (offset - PCIE_BAR4_TABLE7_EP_BASE_ADDR);
    }

    AML_ERR("offset error \n");
    return NULL;
#endif
}
static u8* aml_pci_get_address(struct aml_plat *aml_plat, int addr_name,
                               unsigned int offset)
{
#ifndef CONFIG_AML_FPGA_PCIE
    unsigned int i;
    unsigned int addr;
#endif
    struct aml_pci *aml_pci = (struct aml_pci *)aml_plat->priv;

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
            AML_INFO(" bar5 %x, address %x\n", aml_pci->pci_bar5_vaddr, aml_pci->pci_bar5_vaddr + offset - IPC_REG_BASE_ADDR);
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
        if ((addr_name == pcie_ep_addr_range[i].mem_domain) &&
            (addr >= pcie_ep_addr_range[i].pcie_bar_table_base_addr) &&
            (addr <= pcie_ep_addr_range[i].pcie_bar_table_high_addr))
        {
            if (pcie_ep_addr_range[i].pcie_bar_index == PCIE_BAR2)
            {
                return aml_pci->pci_bar2_vaddr + pcie_ep_addr_range[i].pcie_bar_table_offset + (addr - pcie_ep_addr_range[i].pcie_bar_table_base_addr);
            }
            else
            {
                return aml_pci->pci_bar4_vaddr + pcie_ep_addr_range[i].pcie_bar_table_offset + (addr - pcie_ep_addr_range[i].pcie_bar_table_base_addr);
            }
        }
    }

    AML_INFO(" addr(0x%x) or addr_name(0x%x) err\n", offset, addr_name);
    return NULL;

#endif //CONFIG_AML_FPGA_PCIE
}

static const u32 aml_pci_config_reg[] = {
    NXMAC_DEBUG_PORT_SEL_ADDR,
    SYSCTRL_DIAG_CONF_ADDR,
    SYSCTRL_PHYDIAG_CONF_ADDR,
    SYSCTRL_RIUDIAG_CONF_ADDR,
    RF_V7_DIAGPORT_CONF1_ADDR,
};

static const u32 aml_pci_he_config_reg[] = {
    SYSCTRL_DIAG_CONF0,
    SYSCTRL_DIAG_CONF1,
    SYSCTRL_DIAG_CONF2,
    SYSCTRL_DIAG_CONF3,
};

static int aml_pci_get_config_reg(struct aml_plat *aml_plat, const u32 **list)
{
    u32 fpga_sign;

    if (!list)
        return 0;

    fpga_sign = AML_REG_READ(aml_plat, AML_ADDR_SYSTEM, SYSCTRL_SIGNATURE_ADDR);
    if (__FPGA_TYPE(fpga_sign) == 0xc0ca) {
        *list = aml_pci_he_config_reg;
        return ARRAY_SIZE(aml_pci_he_config_reg);
    } else {
        *list = aml_pci_config_reg;
        return ARRAY_SIZE(aml_pci_config_reg);
    }
}

static int wifi_reboot_fn(struct notifier_block *nb, unsigned long action, void *data)
{
    g_pci_msg_suspend = 1;
    AML_INFO(" action: %d =====>\n", action);
    return NOTIFY_OK;
}

static struct notifier_block wifinotifier = {
    .notifier_call = wifi_reboot_fn,
};

/**
 * aml_platform_register_drv() - Register all possible platform drivers
 */
int aml_platform_register_pcie_drv(void)
{
    int ret = 0;
    struct aml_plat *aml_plat = NULL;
    void *drv_data = NULL;
    AML_INFO(" g_pci_driver_insmoded=%d\n", g_pci_driver_insmoded);

    if (!g_pci_driver_insmoded) {
        aml_pci_insmod();
        msleep(100);
    }

    if (!g_pci_after_probe) {
        return -ENODEV;
    }

    aml_plat = kzalloc(sizeof(struct aml_plat) + sizeof(struct aml_pci), GFP_KERNEL);
    if (!aml_plat)
        return -ENOMEM;

    memcpy(aml_plat, g_aml_plat_pci, sizeof(struct aml_plat) + sizeof(struct aml_pci));

    aml_plat->enable = aml_pci_platform_enable;
    aml_plat->disable = aml_pci_platform_disable;
    aml_plat->get_address = aml_pci_get_address;
    aml_plat->ack_irq = aml_pci_ack_irq;
    aml_plat->get_config_reg = aml_pci_get_config_reg;

    g_pci_dev = aml_plat->pci_dev;
    ret = aml_platform_init(aml_plat, &drv_data);
    if (ret != 0)
        return ret;
    pci_set_drvdata(g_pci_dev, drv_data);
    g_aml_hw = drv_data;
    register_reboot_notifier(&wifinotifier);
    return ret;
}

/**
 * aml_platform_unregister_drv() - Unegister all platform drivers
 */
void aml_platform_unregister_pcie_drv(void)
{
    struct aml_hw *aml_hw;
    struct aml_plat *aml_plat;

    AML_DBG(AML_FN_ENTRY_STR);

    aml_hw = pci_get_drvdata(g_pci_dev);
    if (aml_hw == NULL) {
        if (g_aml_hw) {
            aml_hw = g_aml_hw;
            g_aml_hw = NULL;
        } else {
            AML_INFO("can't get aml_hw, need to check\n");
            return;
        }
    }

    aml_plat = aml_hw->plat;

    aml_platform_deinit(aml_hw);
    kfree(aml_plat);
    AML_FN_EXIT();
    pci_set_drvdata(g_pci_dev, NULL);
    unregister_reboot_notifier(&wifinotifier);
}

void aml_get_vid(struct aml_plat *aml_plat)
{
    AML_INFO("vendor_id : %x", readl(aml_plat->get_address(aml_plat, AML_ADDR_MAC_PHY, REG_OF_VENDOR_ID)));
    while (!(AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, REG_OF_VENDOR_ID) == W2p_VENDOR_AMLOGIC_EFUSE))
    {
        msleep(10);
    }
    AML_INFO("vendor_id : %x", readl(aml_plat->get_address(aml_plat, AML_ADDR_MAC_PHY, REG_OF_VENDOR_ID)));
}


#ifndef CONFIG_AML_SDM
MODULE_FIRMWARE(AML_AGC_FW_NAME);
MODULE_FIRMWARE(AML_FCU_FW_NAME);
MODULE_FIRMWARE(AML_LDPC_RAM_NAME);
#endif
MODULE_FIRMWARE(AML_MAC_FW_NAME);
#ifndef CONFIG_AML_TL4
MODULE_FIRMWARE(AML_MAC_FW_NAME2);
#endif
MODULE_FIRMWARE(AML_MAC_FW_SDIO);
MODULE_FIRMWARE(AML_MAC_FW_USB);
MODULE_FIRMWARE(AML_MAC_FW_PCIE);
