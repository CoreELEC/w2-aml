/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef __WIFI_TOP_ADDR_H__
#define __WIFI_TOP_ADDR_H__

#define WIFI_TOP_BASE                   (0xa07000)

#define RG_WIFI_RST_CTRL                (WIFI_TOP_BASE + 0x00)
#define RG_WIFI_RST_TIMER0              (WIFI_TOP_BASE + 0x04)
#define RG_WIFI_RST_TIMER1              (WIFI_TOP_BASE + 0x08)
#define RG_WIFI_MAC_ARC_CTRL            (WIFI_TOP_BASE + 0x20)
#define MAC_AHBABT_CONTROL0             (WIFI_TOP_BASE + 0x28)
#define MAC_AHBABT_CONTROL1             (WIFI_TOP_BASE + 0x2c)

#define RG_WIFI_IF_FW2HST_STATUS        (WIFI_TOP_BASE + 0x60)
#define RG_WIFI_IF_FW2HST_CLR           (WIFI_TOP_BASE + 0x64)
#define RG_WIFI_IF_FW2HST_MASK          (WIFI_TOP_BASE + 0x68)
#define RG_WIFI_IF_FW2HST_IRQ_CFG       (WIFI_TOP_BASE + 0x6c)
#define RG_WIFI_IF_HOST_IRQ_ST          (WIFI_TOP_BASE + 0x70)

#define RG_WIFI_IF_INT_CIRCLE           (WIFI_TOP_BASE + 0x74)//TODO
#define RG_WIFI_IF_SDIO_FW2HST_CTRL_REG (WIFI_TOP_BASE + 0x78)//TODO
#define RG_WIFI_IF_GPIO_IRQ_CNF         (WIFI_TOP_BASE + 0x7c)//TODO

#define RG_WIFI_IF_RXPAGE_BUF_RDPTR     (WIFI_TOP_BASE + 0x80)
#define RG_WIFI_IF_MAC_TXTABLE_BSADDR   (WIFI_TOP_BASE + 0x90)
#define RG_WIFI_IF_MAC_TXPAGE_BSADDR    (WIFI_TOP_BASE + 0x94)
#define RG_WIFI_IF_MAC_TXTABLE_WT_ID    (WIFI_TOP_BASE + 0x9c)
#define RG_WIFI_IF_MAC_TXTABLE_RD_ID    (WIFI_TOP_BASE + 0xa0)
#define RG_WIFI_IF_MAC_TXTABLE_PAGE_NUM (WIFI_TOP_BASE + 0x98)
#define RG_WIFI_IF_MAC_TXTABLE_OBSERVE  (WIFI_TOP_BASE + 0xa8)

#define RG_WIFI_CPU_CTRL                (WIFI_TOP_BASE + 0xb0)

#endif
