/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _W2_USB_H_
#define _W2_USB_H_
#include "usb_common.h"

/*auc--amlogic usb common*/
struct auc_hif_ops {
    int (*hi_send_cmd)(unsigned int addr, unsigned int len);
    void (*hi_write_word)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram)(const unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    int (*hi_rx_buffer_read)(void *buf, u32 addr, unsigned int len, unsigned int ep);

    /*bt use*/
    void (*hi_write_word_for_bt)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word_for_bt)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    int (*hi_enable_scat)(void);
    void (*hi_cleanup_scat)(void);
    void *(*hi_get_scatreq)(void);
    int (*hi_scat_rw)(struct scatterlist *sg_list, unsigned int sg_num, unsigned int blkcnt,
        unsigned char func_num, unsigned int addr, unsigned char write);

    int (*hi_send_frame)(struct scatterlist *scat_list, int n);
    void (*hi_rcv_frame)(unsigned char* buf, unsigned char* addr, unsigned long len);
};

int auc_cmd_rxrd_set(u32 rxrd);

int wifi_fw_download(char *firmware_filename);
int start_wifi(void);
void auc_write_word_by_ep_for_wifi(unsigned int addr,unsigned int data, unsigned int ep);
unsigned int auc_read_word_by_ep_for_wifi(unsigned int addr, unsigned int ep);
unsigned int auc_read_word_by_ep_for_bt(unsigned int addr, unsigned int ep);
#endif
