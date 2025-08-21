/*
****************************************************************************************
*
* @file aml_hif.h
*
* @brief Implementation of the aml hif
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#ifndef AML_HIF_H_
#define AML_HIF_H_

#include "aml_interface.h"
#include "usb_common.h"
#include "sdio_common.h"
#include "w2_sdio.h"
#include "w2_usb.h"
#include "share_mem_map.h"
#include "wifi_top_addr.h"
#include "aml_log.h"

#define AML_RANGE_CHECK(name, addr, len)    do {} while(0)

#define __AML_CASTED_DEV_ADDR               (unsigned char *)(uintptr_t)addr

static inline int hi_random_read(struct aml_hw *aml_hw, void *buf, addr32_t addr, unsigned int len)
{
    if (aml_bus_type == USB_MODE)
        aml_hw->plat->hif_ops->hi_read_sram(buf, __AML_CASTED_DEV_ADDR, len, USB_EP4);
    else if (aml_bus_type == SDIO_MODE)
        aml_hw->plat->hif_sdio_ops->hi_random_ram_read(buf, __AML_CASTED_DEV_ADDR, len);
    return 0;
}

static inline int hi_random_write(struct aml_hw *aml_hw, addr32_t addr, void *buf, unsigned int len)
{
    if (aml_bus_type == USB_MODE)
        aml_hw->plat->hif_ops->hi_write_sram(buf, __AML_CASTED_DEV_ADDR, len, USB_EP4);
    else if (aml_bus_type == SDIO_MODE)
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write(buf, __AML_CASTED_DEV_ADDR, len);
    return 0;
}

static inline u32 hi_reg_read(struct aml_hw *aml_hw, addr32_t addr)
{
    u32 val = ~0;

    hi_random_read(aml_hw, &val, addr, sizeof(val));
    return val;
}

static inline int hi_reg_write(struct aml_hw *aml_hw, addr32_t addr, u32 val)
{
    hi_random_write(aml_hw, addr, &val, sizeof(val));
    return 0;
}

static inline int hi_sram_read(struct aml_hw *aml_hw, void *buf, addr32_t addr, unsigned int len)
{
    AML_RANGE_CHECK(SRAM, addr, len);
    if (aml_bus_type == USB_MODE)
        aml_hw->plat->hif_ops->hi_read_sram(buf, __AML_CASTED_DEV_ADDR, len, USB_EP4);
    else if (aml_bus_type == SDIO_MODE)
        aml_hw->plat->hif_sdio_ops->hi_sram_read(buf, __AML_CASTED_DEV_ADDR, len);
    return 0;
}

static inline int hi_sram_write(struct aml_hw *aml_hw, addr32_t addr, void *buf, unsigned int len)
{
    AML_RANGE_CHECK(SRAM, addr, len);
    if (aml_bus_type == USB_MODE)
        aml_hw->plat->hif_ops->hi_write_sram(buf, __AML_CASTED_DEV_ADDR, len, USB_EP4);
    else if (aml_bus_type == SDIO_MODE)
        aml_hw->plat->hif_sdio_ops->hi_sram_write(buf, __AML_CASTED_DEV_ADDR, len);
    return 0;
}

static inline int hi_rx_buffer_read(struct aml_hw *aml_hw, void *buf, addr32_t addr, unsigned int len)
{
    AML_RANGE_CHECK(RXBUF, addr, len);
    if (aml_bus_type == USB_MODE)
        return aml_hw->plat->hif_ops->hi_rx_buffer_read(buf, addr, len, USB_EP4);
    else if (aml_bus_type == SDIO_MODE)
        return aml_hw->plat->hif_sdio_ops->hi_rx_buffer_read(buf, addr, len, 0);
    return 0;
}
#undef __AML_CASTED_DEV_ADDR

#endif /* AML_HIF_H_ */
