#ifndef _W2_USB_H_
#define _W2_USB_H_
#include "usb_common.h"

#define BT_INTR_TRANS_FLAG 0xc6a780c2

#define W2_PRODUCT  0x4c55
#define W2_VENDOR  0x414D

#define W2u_VENDOR_AMLOGIC_EFUSE 0x1B8E
#define W2u_PRODUCT_A_AMLOGIC_EFUSE 0x0601
#define W2u_PRODUCT_B_AMLOGIC_EFUSE 0x0641
#define USB_TXCMD_CARRY_RXRD_INDEX 401

/*auc--amlogic usb common*/
struct auc_hif_ops {
    int (*hi_send_cmd)(unsigned int addr, unsigned int len);
    void (*hi_write_word)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    void (*hi_rx_buffer_read)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    /*bt use*/
    void (*hi_write_word_for_bt)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word_for_bt)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    int (*hi_enable_scat)(void);
    void (*hi_cleanup_scat)(void);
    struct amlw_hif_scatter_req * (*hi_get_scatreq)(void);
    int (*hi_scat_rw)(struct scatterlist *sg_list, unsigned int sg_num, unsigned int blkcnt,
        unsigned char func_num, unsigned int addr, unsigned char write);

    int (*hi_send_frame)(struct amlw_hif_scatter_req *scat_req);
    void (*hi_rcv_frame)(unsigned char* buf, unsigned char* addr, unsigned long len);
};

int wifi_fw_download(char *firmware_filename);
int start_wifi(void);
void auc_write_word_by_ep_for_wifi(unsigned int addr,unsigned int data, unsigned int ep);
unsigned int auc_read_word_by_ep_for_wifi(unsigned int addr, unsigned int ep);

#endif
