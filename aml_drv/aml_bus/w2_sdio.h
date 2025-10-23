/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _W2_SDIO_H_
#define _W2_SDIO_H_
#include "sdio_common.h"

#define SDIO_MAX_BLK_CNT    511

#define SDIO_BLKSIZE 512
#define FUNC4_BLKSIZE 512

#define MAC_ICCM_AHB_BASE    0x00000000
#define MAC_SRAM_BASE        0x00a10000
#define MAC_REG_BASE         0x00a00000
#define MAC_DCCM_AHB_BASE    0x00d00000

#ifdef CONFIG_PT_MODE
    #define SRAM_MAX_LEN (1024 * 64)
#else
    #define SRAM_MAX_LEN (1024 * 128)
#endif
#define SRAM_LEN (32 * 1024)
#define ICCM_ROM_LEN (256 * 1024)
#define ICCM_RAM_LEN (256 * 1024)
#define ICCM_ALL_LEN (ICCM_ROM_LEN + ICCM_RAM_LEN)
#define DCCM_ALL_LEN (256 * 1024)
#define ICCM_ROM_ADDR (0x00100000)
#define ICCM_RAM_ADDR (0x00100000 + ICCM_ROM_LEN)
#define DCCM_RAM_ADDR (0x00d00000)
#define DCCM_RAM_OFFSET (0x00700000) //0x00800000 - 0x00100000, in fw_flash
#define BYTE_IN_LINE (9)
#define ICCM_BUFFER_RD_LEN  (ICCM_RAM_LEN)
#define ICCM_CHECK_LEN      (ICCM_RAM_LEN)
#define DCCM_CHECK_LEN      (DCCM_ALL_LEN)

#define ICCM_CHECK
#define ICCM_ROM

#define WIFI_SDIO_IF    (0xa05000)
                 /*BIT(0): TX DONE intr, BIT(1): RX DONE intr*/
#define RG_SDIO_IF_INTR2CPU_ENABLE    (WIFI_SDIO_IF+0x30)

struct aml_hif_sdio_ops {
    //sdio func0 for self define domain, cmd52
    int (*hi_self_define_domain_func0_write8)(int addr, unsigned char data);
    unsigned char (*hi_self_define_domain_func0_read8)(int addr);
    //sdio func1 for self define domain, cmd52
    int (*hi_self_define_domain_write8)(int addr, unsigned char data);
    unsigned char (*hi_self_define_domain_read8)(int addr);
    int (*hi_self_define_domain_write32)(unsigned long sram_addr, unsigned long sramdata);
    unsigned long (*hi_self_define_domain_read32)(unsigned long sram_addr);

    //sdio func2 for random ram
    void (*hi_random_word_write)(unsigned int addr, unsigned int data);
    unsigned int (*hi_random_word_read)(unsigned int addr);
    void (*hi_random_ram_write)(unsigned char *buf, unsigned char *addr, size_t len);
    void (*hi_random_ram_read)(unsigned char *buf, unsigned char *addr, size_t len);

    //sdio func3 for sram
    void (*hi_sram_word_write)(unsigned int addr, unsigned int data);
    unsigned int (*hi_sram_word_read)(unsigned int addr);
    void (*hi_sram_write)(unsigned char *buf, unsigned char *addr, size_t len);
    void (*hi_sram_read)(unsigned char *buf, unsigned char *addr, size_t len);

    //sdio func4 for tx buffer
    void (*hi_tx_buffer_write)(unsigned char *buf, unsigned char *addr, size_t len);
    void (*hi_tx_buffer_read)(unsigned char *buf, unsigned char *addr, size_t len);

    //sdio func5 for rxdesc
    int (*hi_desc_read)(void *buf, u32 addr, size_t len);

    //sdio func6 for rx buffer
    int (*hi_rx_buffer_read)(void *buf, u32 addr, unsigned int len, unsigned int unused);

    //scatter list operation
    int (*hi_enable_scat)(struct aml_hwif_sdio *hif_sdio);
    void (*hi_cleanup_scat)(struct aml_hwif_sdio *hif_sdio);
    void *(*hi_get_scatreq)(struct aml_hwif_sdio *hif_sdio);
    int (*hi_scat_rw)(struct scatterlist *sg_list, unsigned int sg_num, unsigned int blkcnt,
        unsigned char func_num, unsigned int addr, unsigned char write);
    int (*hi_send_frame)(struct scatterlist *scat_list, int n);
    int (*hi_recv_frame)(void *scat_req);

    //sdio func7 for bt
    void (*bt_hi_write_sram)(unsigned char* buf, unsigned char* addr, SYS_TYPE len);
    void (*bt_hi_read_sram)(unsigned char* buf, unsigned char* addr, SYS_TYPE len);
    void (*bt_hi_write_word)(unsigned int addr,unsigned int data);
    unsigned int (*bt_hi_read_word)(unsigned int addr);

    //suspend & resume
    int (*hif_suspend)(unsigned int suspend_enable);
};


extern struct aml_hif_sdio_ops g_hif_sdio_ops;
extern unsigned char g_sdio_driver_insmoded;

unsigned char aml_download_wifi_fw_img(char *firmware_filename);
void aml_sdio_init_base_addr(void);
void aml_sdio_init_w2_ops(void);


#endif
