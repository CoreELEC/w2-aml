
/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _FI_W2_SDIO_H
#define _FI_W2_SDIO_H

 /* start address of sram_cfm in SRAM  */
#define SRAM_TXCFM_START_ADDR   (0xa17000)
#define SRAM_TXCFM_SIZE         (2 << 10)
#define SRAM_SYNC_FW_CFM_IDX ((SRAM_TXCFM_START_ADDR) + (SRAM_TXCFM_SIZE))

#define howmanypage(x,y) (((x - 12) + ((y - 12) -1) )/ (y - 12))

#define SDIO_PAGE_MAX    65
#define SDIO_PAGE_LEN    1024

#define SDIO_DATA_OFFSET        (12 + 72)

#define SDIO_TX_PAGE_SMALL_SKIP_NUM 166
#define SDIO_TX_PAGE_NUM_SMALL 90
#define SDIO_TX_PAGE_NUM_LARGE 255
#define SDIO_DYNA_PAGE_NUM  165

#if defined (USB_TX_USE_LARGE_PAGE) || defined (CONFIG_AML_USB_LARGE_PAGE)
#define USB_PAGE_MAX    25

#define USB_PAGE_LEN    4624

#define USB_TX_PAGE_SMALL_SKIP_NUM 56
#define USB_TX_PAGE_NUM_SMALL 21   //250 * 1024 / 1880
#define USB_TX_PAGE_NUM_LARGE 77
#define USB_SEND_URB_DEFAULT_WAIT_TIME 1000   //1000us

#define USB_LA_PAGE_NUM  15
#else
#define USB_PAGE_MAX    40
#define USB_SEND_URB_DEFAULT_WAIT_TIME 1000   //1000us
#define USB_TX_PAGE_SMALL_SKIP_NUM 147
#define USB_TX_PAGE_NUM_SMALL 53   //250 * 1024 / 1880
#define USB_TX_PAGE_NUM_LARGE 200

#define USB_PAGE_LEN    1880
#define USB_LA_PAGE_NUM  35
#endif

#define SDIO_LA_PAGE_NUM  64

//When setting bit4 to 1, enable auto tx of func4, otherwise disable
#define RG_SCFG_FUNC1_AUTO_TX  0x8181


#define WIFI_SDIO_IF    (0xa05000)

/*BIT(0): TX DONE intr, BIT(1): RX DONE intr*/
#define RG_SDIO_IF_INTR2CPU_ENABLE    (WIFI_SDIO_IF+0x30)

#endif


