/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#define AML_MODULE          USB
#define AML_FMT             AML_FMT_M

#include "usb_common.h"
#include "chip_ana_reg.h"
#include "wifi_intf_addr.h"
#include "fi_sdio.h"
#include "w2_usb.h"
#include "aml_static_buf.h"
#include "w2_sdio.h"
#include "aml_interface.h"
#include "aml_compat.h"
#include "aml_log.h"

#define ICCM_ROM_LEN (256 * 1024)
#define ICCM_RAM_LEN (256 * 1024)
#define ICCM_ALL_LEN (ICCM_ROM_LEN + ICCM_RAM_LEN)
#define DCCM_ALL_LEN (256 * 1024)
#define ICCM_ROM_ADDR (0x00100000)
#define ICCM_RAM_ADDR (0x00100000 + ICCM_ROM_LEN)
#define DCCM_RAM_ADDR (0x00d00000)
#define DCCM_RAM_OFFSET (0x00700000) //0x00800000 - 0x00100000, in fw_flash
#define BYTE_IN_LINE (9)

#define ICCM_ROM

#define AML_SIG_CBW                 0x43425355
#define AML_TAG_CBW                 0x5da729a0

#define BT_INTR_TRANS_FLAG          0xc6a780c2

#define UPDATE_FLAG                 0x11223344
#define USB_TXCMD_CARRY_RXRD_INDEX  401
#define WRITE_SRAM_DATA_LEN         477

enum aml_usb_dir {
    AML_XFER_TO_DEVICE = 0,
    AML_XFER_TO_HOST = 0x80,
};

#define WIFI_READ_CMD               0   // EP4
#define BT_READ_CMD                 1   // EP2

extern struct auc_hif_ops g_auc_hif_ops;
extern struct usb_device *g_udev;
extern unsigned char auc_driver_insmoded;
extern struct crg_msc_cbw *g_cmd_buf;
extern struct aml_pm_type g_wifi_pm;

unsigned char *g_auc_kmalloc_buf = NULL;

static inline void __auc_cmd_rxrd_set(u32 flag, u32 rxrd)
{
    unsigned char *p = &g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX];

    *p++ = rxrd & 0xff;
    *p++ = (rxrd >> 8) & 0xff;
    *p++ = (rxrd >> 16) & 0xff;
    *p++ = (rxrd >> 24) & 0xff;

    *p++ = flag & 0xff;
    *p++ = (flag >> 8) & 0xff;
    *p++ = (flag >> 16) & 0xff;
    *p++ = (flag >> 24) & 0xff;
}

static inline void auc_cmd_rxrd_clear(void)
{
    __auc_cmd_rxrd_set(0, 0);
}

int auc_cmd_rxrd_set(u32 rxrd)
{
    USB_BEGIN_LOCK();
    /* RX read pointer (confirm) is already embedded in command? */
    if (*(u32 *)&g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 4]) {
        USB_END_LOCK();
        return -1;
    }

    /* later send it to firmware with the next command */
    __auc_cmd_rxrd_set(UPDATE_FLAG, rxrd);
    USB_END_LOCK();
    return 0;
}
EXPORT_SYMBOL(auc_cmd_rxrd_set);

static void auc_build_cbw_add_data(struct crg_msc_cbw *cbw_buf,
                                   enum aml_usb_dir dir, unsigned int len,
                                   enum wifi_cmd cmd, u32 addr, u32 flag, u32 data_len,
                                   const unsigned char *data)
{
    cbw_buf->sig = AML_SIG_CBW;
    cbw_buf->tag = AML_TAG_CBW;
    cbw_buf->data_len = len;
    cbw_buf->flag = dir; //direction
    cbw_buf->len = 16; //command length
    cbw_buf->lun = 0;

    cbw_buf->cdb[0] = cmd;
    cbw_buf->cdb[1] = addr; // read or write addr
    cbw_buf->cdb[2] = flag;
    cbw_buf->cdb[3] = data_len; //read or write data length

    if (!data)
        return;

    BUG_ON(dir != AML_XFER_TO_DEVICE);
    BUG_ON(len >= USB_TXCMD_CARRY_RXRD_INDEX);
    memcpy(cbw_buf->resv + 1, (unsigned char *) data, len);
    /*in case call cmd and data mode but fw call cmd+data stage*/
    cbw_buf->resv[479] = cbw_buf->resv[480] = 0xFF;
}

static inline void auc_build_cbw(struct crg_msc_cbw *cbw_buf,
                                 enum aml_usb_dir dir, unsigned int len,
                                 enum wifi_cmd cmd, u32 addr, u32 flag, u32 data_len)
{
    return auc_build_cbw_add_data(cbw_buf, dir, len, cmd, addr, flag, data_len, NULL);
}

int auc_bulk_msg(struct usb_device *usb_dev, unsigned int pipe,
    void *data, int len, int *actual_length, int timeout)
{
    int ret = 0;
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        ERROR_DEBUG_OUT("bus suspend (%d) ongoing, do not read/write now!\n",
            atomic_read(&g_wifi_pm.bus_suspend_cnt));
        return -ENOMEM;
    }
#endif
    if (atomic_read(&g_wifi_pm.is_shut_down) == 1) {
        ERROR_DEBUG_OUT("fw shut down(%d) , do not read/write now!\n",
            atomic_read(&g_wifi_pm.is_shut_down));
        return -ENOMEM;
    }

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return -ENOMEM;
    }
#endif
    AML_PROF_CNT(BULK, usb_pipein(pipe) ? len : -len);
    ret = usb_bulk_msg(usb_dev, pipe, data, len, actual_length, timeout);
    AML_PROF_CNT(BULK, 0);
#ifdef CONFIG_AML_RECOVERY
    if (ret && !bus_state_detect.bus_err) {
#ifdef CONFIG_AML_USB_HOTPLUG
        if (bus_state_detect.usb_unplug)
            aml_usb_set_bus_err(1);
#endif
        if ((bus_state_detect.is_drv_load_finished) && (!bus_state_detect.is_recy_ongoing)) {
            aml_usb_set_bus_err(1);
            ERROR_DEBUG_OUT("bus error(%d), will do reovery later\n", ret);
        }
    }
#endif

    return ret;
}

int auc_send_cmd_ep1(unsigned int addr, unsigned int len)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_DOWNLOAD_BT, addr, 0, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP2), g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        USB_END_LOCK();
        return ret;
    }

    USB_END_LOCK();
    return 0;
}

unsigned int auc_read_reg_ep1(unsigned int addr, unsigned int len)
{
    int ret = 0;
    int actual_length = 0;
    unsigned int reg_data;
    struct usb_device *udev = g_udev;
    unsigned char *data = NULL;

    USB_BEGIN_LOCK();

    data = (unsigned char *)ZMALLOC(len,"reg tmp",GFP_DMA | GFP_ATOMIC);
    if (!data) {
        ERROR_DEBUG_OUT("data malloc fail, len: %d\n", len);
        USB_END_LOCK();
        return -ENOMEM;
    }

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, BT_INTR_TRANS_FLAG, len);

    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP2),(void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(data,"reg tmp");
        USB_END_LOCK();
        return ret;
    }
    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvintpipe(udev, USB_EP1), (void *)data, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(data,"reg tmp");
        USB_END_LOCK();
        return ret;
    }

    memcpy(&reg_data, data, actual_length);
    FREE(data,"reg tmp");
    USB_END_LOCK();

    return reg_data;
}

void auc_read_sram_ep1(unsigned char *pdata, unsigned int addr, unsigned int len)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();

    auc_build_cbw(g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, BT_INTR_TRANS_FLAG, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP2), (void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        USB_END_LOCK();
        return;
    }

    if (g_auc_kmalloc_buf) {
        kmalloc_buf = g_auc_kmalloc_buf;
    } else {
        kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_read_sram", GFP_DMA|GFP_ATOMIC);
        if (kmalloc_buf == NULL)
        {
            ERROR_DEBUG_OUT("kmalloc buf fail, len: %d\n", len);
            USB_END_LOCK();
            return;
        }
    }

    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvintpipe(udev, USB_EP1),(void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d,  addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        USB_END_LOCK();
        return;
    }

    memcpy(pdata, kmalloc_buf, actual_length);
    if (g_auc_kmalloc_buf != kmalloc_buf) {
        FREE(kmalloc_buf, "usb_read_sram");
    }

    USB_END_LOCK();
}

void usb_isoc_callback(struct urb * urb)
{
    AML_INFO("urb: 0x%p; iso callback is called!\n", urb);
}

int auc_write_reg_ep3(unsigned int addr, unsigned int value, unsigned int len)
{
    int ret = 0;
    struct usb_device *udev = g_udev;
    struct urb *urb;

    USB_BEGIN_LOCK();

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        USB_END_LOCK();
        return -ENOMEM;
    }

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);

    urb->dev = udev;
    urb->pipe = usb_sndisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = g_cmd_buf;
    urb->transfer_buffer_length = sizeof(*g_cmd_buf);

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = sizeof(*g_cmd_buf);

    /* cmd stage */
    ret = usb_submit_urb(urb, GFP_ATOMIC);//GFP_KERNEL
    if (ret) {
        ERROR_DEBUG_OUT("Failed to submit urb, ret %d,  addr: 0x%x, len: %d, value: 0x%x\n", ret, addr, len, value);
        usb_free_urb(urb);
        USB_END_LOCK();
        return ret;
    }
    usb_free_urb(urb);
    USB_END_LOCK();

    return ret;
}

unsigned int auc_read_reg_ep3(unsigned int addr, unsigned int len)
{
    unsigned int reg_data;
    int ret = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;
    struct urb *urb;

    USB_BEGIN_LOCK();

    urb= usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        USB_END_LOCK();
        return -ENOMEM;
    }

    kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_read_sram", GFP_DMA|GFP_ATOMIC);

    if (kmalloc_buf == NULL)
    {
        ERROR_DEBUG_OUT("kmalloc buf fail, len: %d\n", len);
        USB_END_LOCK();
        return -1;
    }

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

    urb->dev = udev;
    urb->pipe = usb_sndisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = g_cmd_buf;
    urb->transfer_buffer_length = sizeof(*g_cmd_buf);

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = sizeof(*g_cmd_buf);

    ret = usb_submit_urb(urb, GFP_ATOMIC); //GFP_KERNEL
    if (ret) {
        ERROR_DEBUG_OUT("EP3: Failed to submit urb, ret %d,  addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return ret;
    }

    usb_free_urb(urb);
    usleep_range(1000,1200);

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        FREE(kmalloc_buf, "usb_read_sram");
        USB_END_LOCK();
        return -ENOMEM;
    }

    urb->dev= udev;
    urb->pipe = usb_rcvisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = kmalloc_buf;
    urb->transfer_buffer_length = len;
    urb->number_of_packets = 1;
    urb->interval = 8;
    urb->complete = usb_isoc_callback;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = len;

    ret = usb_submit_urb(urb, GFP_ATOMIC); //GFP_KERNEL
    if (ret) {
        ERROR_DEBUG_OUT("EP3: Failed to submit urb, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return ret;
    }

    memcpy(&reg_data, kmalloc_buf, len);
    FREE(kmalloc_buf, "usb_read_sram");
    usb_free_urb(urb);
    USB_END_LOCK();

    return reg_data;
}

void auc_write_sram_ep3(unsigned char *pdata, unsigned int addr, unsigned int len)
{
    int ret = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;
    struct urb *urb;

    USB_BEGIN_LOCK();

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        USB_END_LOCK();
        return;
    }

    kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_write_sram", GFP_DMA | GFP_ATOMIC);//virt_to_phys(fwICCM);
    if (kmalloc_buf == NULL)
    {
        ERROR_DEBUG_OUT("kmalloc buf fail, len: %d\n", len);
        USB_END_LOCK();
        return;
    }

    memcpy(kmalloc_buf, pdata, len);

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len + 4, CMD_WRITE_SRAM, addr, 0, len + 4);

    urb->dev = udev;
    urb->pipe = usb_sndisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = g_cmd_buf;
    urb->transfer_buffer_length = sizeof(*g_cmd_buf);

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = sizeof(*g_cmd_buf);

    /* cmd stage */
    ret = usb_submit_urb(urb, GFP_ATOMIC);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return;
    }

    usb_free_urb(urb);
    usleep_range(1000,1200);
    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        FREE(kmalloc_buf, "usb_read_sram");
        USB_END_LOCK();
        return;
    }

    urb->dev = udev;
    urb->pipe = usb_sndisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = kmalloc_buf;
    urb->transfer_buffer_length = len;

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = len;

    /* data stage */
    ret = usb_submit_urb(urb, GFP_ATOMIC);//GFP_KERNEL
    if (ret) {
        ERROR_DEBUG_OUT("EP3: Failed to submit urb, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return;
    }

    FREE(kmalloc_buf, "usb_write_sram");
    usb_free_urb(urb);
    USB_END_LOCK();
}

void auc_read_sram_ep3(unsigned char *pdata, unsigned int addr, unsigned int len)
{
    int ret = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;
    struct urb *urb;

    USB_BEGIN_LOCK();

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        USB_END_LOCK();
        return;
    }

    kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_read_sram", GFP_DMA|GFP_ATOMIC);
    if (kmalloc_buf == NULL)
    {
        ERROR_DEBUG_OUT("kmalloc buf fail, len: %d\n", len);
        USB_END_LOCK();
        return;
    }

    auc_build_cbw(g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);

    urb->dev = udev;
    urb->pipe = usb_sndisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;

    urb->transfer_buffer = g_cmd_buf;
    urb->transfer_buffer_length = sizeof(*g_cmd_buf);

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;

    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = sizeof(*g_cmd_buf);

    /* cmd stage */
    ret = usb_submit_urb(urb, GFP_ATOMIC);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return;
    }

    usb_free_urb(urb);
    usleep_range(1000,1200);

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        AML_ERR("alloc urb failed!\n");
        FREE(kmalloc_buf, "usb_read_sram");
        USB_END_LOCK();
        return;
    }

    urb->dev = udev;
    urb->pipe = usb_rcvisocpipe(udev, USB_EP3);
    urb->transfer_flags = URB_ISO_ASAP;
    urb->transfer_buffer = kmalloc_buf;
    urb->transfer_buffer_length = len;

    urb->complete = usb_isoc_callback;
    urb->number_of_packets = 1;
    urb->interval = 8;
    urb->iso_frame_desc[0].offset = 0;
    urb->iso_frame_desc[0].length = len;

    /* data stage */
    ret = usb_submit_urb(urb, GFP_ATOMIC); //GFP_KERNEL
    if (ret) {
        ERROR_DEBUG_OUT("Failed to submit urb, ret %d, addr: 0x%x, len: %d\n", ret, addr, len);
        FREE(kmalloc_buf, "usb_read_sram");
        usb_free_urb(urb);
        USB_END_LOCK();
        return;
    }

    usleep_range(1000,1200);
    memcpy(pdata, kmalloc_buf, /*urb->actual_length*/len);
    usleep_range(1000,1200);

    FREE(kmalloc_buf, "usb_read_sram");
    usb_free_urb(urb);

    USB_END_LOCK();
}

int auc_write_reg_by_ep(unsigned int addr, unsigned int value, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, (unsigned int)usb_sndbulkpipe(udev, ep),(void *) g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, value: 0x%x\n", ret, ep, addr, len, value);
        USB_END_LOCK();
        return ret;
    }
    USB_END_LOCK();

    return actual_length; //bt write maybe use the value
}

unsigned int auc_read_reg_by_ep(unsigned int addr, unsigned int len, unsigned int ep, unsigned int mode)
{
    int ret = 0;
    int actual_length = 0;
    unsigned int reg_data;
    struct usb_device *udev = g_udev;
    unsigned char *data = NULL;

    USB_BEGIN_LOCK();

    if (g_auc_kmalloc_buf) {
        data = g_auc_kmalloc_buf;
    } else {
        data = (unsigned char *)ZMALLOC(len,"reg tmp", GFP_DMA | GFP_ATOMIC);
        if (!data) {
            ERROR_DEBUG_OUT("data malloc fail, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ep, addr, len, mode);
            USB_END_LOCK();
            return -ENOMEM;
        }
    }

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep),(void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        if (data != g_auc_kmalloc_buf) {
            FREE(data,"reg tmp");
        }
        USB_END_LOCK();
        return ret;
    }

    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, ep), (void *)data, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret ,ep, addr, len, mode);
        if (data != g_auc_kmalloc_buf) {
            FREE(data,"reg tmp");
        }
        USB_END_LOCK();
        return ret;
    }

    memcpy(&reg_data, data, actual_length);
    if (data != g_auc_kmalloc_buf) {
        FREE(data,"reg tmp");
    }
    USB_END_LOCK();

    return reg_data;
}

extern int coex_flag;
void auc_write_sram_by_ep(const unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();
    /* NB: original code may overwrite RXRD at USB_TXCMD_CARRY_RXRD_INDEX(401) */
    if (coex_flag && len < min(USB_TXCMD_CARRY_RXRD_INDEX, WRITE_SRAM_DATA_LEN)) {
        auc_build_cbw_add_data(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len, pdata);
        /* cmd stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void*)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
            ERROR_DEBUG_OUT("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
            USB_END_LOCK();
            return;
        }
        auc_cmd_rxrd_clear();
        g_cmd_buf->resv[479] = g_cmd_buf->resv[480] = 0;
    } else {
        auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len);
        /* cmd stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void*)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
            ERROR_DEBUG_OUT("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
            USB_END_LOCK();
            return;
        }

        auc_cmd_rxrd_clear();

        if (g_auc_kmalloc_buf) {
            kmalloc_buf = g_auc_kmalloc_buf;
        } else {
            kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_write_sram", GFP_DMA | GFP_ATOMIC);//virt_to_phys(fwICCM);
            if (kmalloc_buf == NULL)
            {
                ERROR_DEBUG_OUT("kmalloc buf fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
                USB_END_LOCK();
                return;
            }
        }

        memcpy(kmalloc_buf, pdata, len);
        /* data stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d\n", ret, ep, addr, len);
            if (g_auc_kmalloc_buf != kmalloc_buf) {
                FREE(kmalloc_buf, "usb_read_sram");
            }
            USB_END_LOCK();
            return;
        }
        if (g_auc_kmalloc_buf != kmalloc_buf) {
            FREE(kmalloc_buf, "usb_read_sram");
        }
    }

    USB_END_LOCK();
}

void auc_read_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep, unsigned int mode)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;

    if (mode == WIFI_READ_CMD && addr == SRAM_TXCFM_START_ADDR && len == SRAM_TXCFM_SIZE) {
        /* EP5 is designed to directly read TX confirmation w/o a pre-command, no lock is required. */
        ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, USB_EP5), pdata, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        }
        return;
    }

    USB_BEGIN_LOCK();

    auc_build_cbw(g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        ERROR_DEBUG_OUT("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n",addr,len);
        USB_END_LOCK();
        return;
    }

    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
    if (g_auc_kmalloc_buf) {
        kmalloc_buf = g_auc_kmalloc_buf;
    } else {
        kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_read_sram", GFP_DMA|GFP_ATOMIC);
        if (kmalloc_buf == NULL)
        {
            ERROR_DEBUG_OUT("kmalloc buf fail, ep: %d, len: %d\n", ep, len);
            USB_END_LOCK();
            return;
        }
    }

    ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, ep),(void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        if (g_auc_kmalloc_buf != kmalloc_buf) {
            FREE(kmalloc_buf, "usb_read_sram");
        }
        USB_END_LOCK();
        return;
    }

    memcpy(pdata, kmalloc_buf, actual_length);
    if (g_auc_kmalloc_buf != kmalloc_buf) {
        FREE(kmalloc_buf, "usb_read_sram");
    }

    USB_END_LOCK();
}

static int rx_read(unsigned char *pdata, u32 addr, unsigned int len, unsigned int ep, unsigned int mode)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        USB_END_LOCK();
        return ret;
    }
    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, ep),(void *)pdata, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        USB_END_LOCK();
        return ret;
    }

    USB_END_LOCK();
    return actual_length;
}

void auc_write_word_by_ep_for_wifi(unsigned int addr,unsigned int data, unsigned int ep)
{
    int len = 4;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif

    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7)) {
        ep = USB_EP1;
        auc_write_reg_by_ep(addr, data, len, ep);
    } else {
        AML_ERR("write_word: ep-%d unsupported\n", ep);
    }
}

unsigned int auc_read_word_by_ep_for_wifi(unsigned int addr, unsigned int ep)
{
    int len = 4;
    unsigned int value = 0;
    static int cnt = 0;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        if (cnt++ < 3)
            ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
                ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return 0;
    }
#endif
    cnt = 0;
    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7)) {
        ep = USB_EP1;
        value = auc_read_reg_by_ep(addr, len, ep, WIFI_READ_CMD);
    } else {
        AML_ERR("Read_word: ep-%d unsupported!\n", ep);
    }

    return value;
}

void auc_write_sram_by_ep_for_wifi(const unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    if (len == 0) {
        ERROR_DEBUG_OUT("EP-%d write len err!\n", ep);
        return;
    }
#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif
    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7)) {
        ep = USB_EP1;
        auc_write_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
    } else {
        AML_ERR("write_word: ep-%d unsupported\n", ep);
    }
}

void auc_read_sram_by_ep_for_wifi(unsigned char *buf,unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    if (len == 0) {
        ERROR_DEBUG_OUT("EP-%d read len err!\n", ep);
        return;
    }
#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif
    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7)) {
        ep = USB_EP1;
        auc_read_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep, WIFI_READ_CMD);
    } else {
        AML_ERR("write_word: ep-%d unsupported\n", ep);
    }
}

static int auc_rx_buffer_read(void *buf, u32 sram_addr, unsigned int len, unsigned int ep)
{
    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7))
        return rx_read(buf, sram_addr, len, USB_EP1, WIFI_READ_CMD);

    AML_ERR("write_word: ep-%d unsupported\n", ep);
    return -1;
}

void auc_write_word_by_ep_for_bt(unsigned int addr,unsigned int data, unsigned int ep)
{
    int len = 4;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif
    switch (ep) {
        case USB_EP2:
            auc_write_reg_by_ep(addr, data, len, ep);
            break;
        case USB_EP3:
            auc_write_reg_ep3(addr, data, len);
            break;
        default:
            AML_ERR("EP-%d unsupported!\n", ep);
            break;
    }
}

unsigned int auc_read_word_by_ep_for_bt(unsigned int addr, unsigned int ep)
{
    int len = 4;
    unsigned int value = 0;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return 0;
    }
#endif
    switch (ep) {
        case USB_EP1:
            value = auc_read_reg_ep1(addr, len);
            break;
        case USB_EP2:
            value = auc_read_reg_by_ep(addr, len, ep, BT_READ_CMD);
            break;
        case USB_EP3:
            value = auc_read_reg_ep3(addr, len);
            break;
        default:
            AML_ERR("EP-%d unsupported!\n", ep);
            break;
    }
    return value;
}

void auc_write_sram_by_ep_for_bt(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    if (len == 0) {
        ERROR_DEBUG_OUT("EP-%d write len err!\n", ep);
        return;
    }
#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif
    switch (ep) {
        case USB_EP2:
            auc_write_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
            break;
        case USB_EP3:
            auc_write_sram_ep3(buf, (unsigned int)(unsigned long)sram_addr, len);
            break;
        default:
            AML_ERR("EP-%d unsupported!\n", ep);
            break;
    }
}
void auc_read_sram_by_ep_for_bt(unsigned char *buf,unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    if (len == 0) {
        ERROR_DEBUG_OUT("EP-%d read len err!\n", ep);
        return;
    }
#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("EP-%d bus reset is ongoing(bus err:%d, reset on going: %d:), do not read/write now!\n",
            ep, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return;
    }
#endif
    switch (ep) {
        case USB_EP1:
            auc_read_sram_ep1(buf, (unsigned int)(unsigned long)sram_addr, len);
            break;
        case USB_EP2:
            auc_read_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep, BT_READ_CMD);
            break;
        case USB_EP3:
            auc_read_sram_ep3(buf, (unsigned int)(unsigned long)sram_addr, len);
            break;
        default:
            AML_ERR("EP-%d unsupported!\n", ep);
            break;
    }
}

struct usb_sg_request_ex {
    struct usb_sg_request sgr;
    struct timer_list timer;
    int timed_out;
};

static void aml_usb_sg_cancel(struct usb_sg_request *io)
{
#if defined(CFG80211_VANILLA) || (LINUX_VERSION_CODE & 0xffffff00) != KERNEL_VERSION(6, 12, 0)
    usb_sg_cancel(io);
#else
    /*
     * WAR: The API usb_sg_cancel() is hidden by Android-16/Linux-6.12
     * the following code is copied from drivers/usb/core/message.c
     * this WAR can be removed after usb_sg_cancel is inserted into gki/aarch64/abi.stg
     */
    unsigned long flags;
    int i, retval;

    spin_lock_irqsave(&io->lock, flags);
    if (io->status || io->count == 0) {
        spin_unlock_irqrestore(&io->lock, flags);
        return;
    }
    /* shut everything down */
    io->status = -ECONNRESET;
    io->count++;        /* Keep the request alive until we're done */
    spin_unlock_irqrestore(&io->lock, flags);

    for (i = io->entries - 1; i >= 0; --i) {
        atomic_inc(&io->urbs[i]->reject);    /* = usb_block_urb(io->urbs[i]); */

        retval = usb_unlink_urb(io->urbs[i]);
        if (retval != -EINPROGRESS
            && retval != -ENODEV
            && retval != -EBUSY
            && retval != -EIDRM)
            dev_warn(&io->dev->dev, "%s, unlink --> %d\n",
                 __func__, retval);
    }

    spin_lock_irqsave(&io->lock, flags);
    io->count--;
    if (!io->count)
        complete(&io->complete);
    spin_unlock_irqrestore(&io->lock, flags);
#endif
}

static void w2_usb_sg_timed_out(struct timer_list *t)
{
    struct usb_sg_request_ex *ctx = from_timer(ctx, t, timer);

    AML_RLMT_ERR("USB SG timeout!\n");
    ctx->timed_out = 1;
    aml_usb_sg_cancel(&ctx->sgr);
}

static int w2_usb_send_packet(struct usb_device *udev, unsigned int pipe,
                              struct scatterlist *sg_list, int n_sg)
{
    struct usb_sg_request_ex ctx = {};
    int ret;

    BUG_ON(!udev->bus->sg_tablesize);

    AML_PROF_CNT(SG, -n_sg);
    ret = usb_sg_init(&ctx.sgr, udev, pipe, 0, sg_list, n_sg, 0, GFP_NOIO);
    if (ret) {
        AML_RLMT_ERR("usb_sg_init fail ret = %d\n", ret);
        return ret;
    }

    timer_setup(&ctx.timer, w2_usb_sg_timed_out, 0);
    ctx.timer.expires = jiffies + msecs_to_jiffies(AML_USB_CONTROL_MSG_TIMEOUT);
    add_timer(&ctx.timer);
    usb_sg_wait(&ctx.sgr);
    del_timer(&ctx.timer);
    AML_PROF_CNT(SG, 0);
    if (ctx.sgr.status) {
        AML_RLMT_ERR("usb_sg_wait fail %d timed out %d\n", ctx.sgr.status, ctx.timed_out);
        return -1;
    }
    return 0;
}

int w2_usb_send_frame(struct scatterlist *scat_list, int n_sg)
{
    struct scatterlist *sg;
    int ret;
    int i;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    uint16_t buffer_size[128];
    int len = sizeof(*buffer_size) * n_sg;

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("bus err or reset is on going(bus err %d, bus reset ongoing: %d)\n",
            bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        return 0;
    }
#endif

    BUG_ON(n_sg >= ARRAY_SIZE(buffer_size));
    for_each_sg(scat_list, sg, n_sg, i) {
        buffer_size[i] = sg->length;
        actual_length += sg->length;
    }

    USB_BEGIN_LOCK();

    AML_PROF_CNT(SG, - actual_length);

    auc_build_cbw_add_data(g_cmd_buf, AML_XFER_TO_DEVICE, len,
                           CMD_WRITE_PACKET, 0, 0, len, (unsigned char *)buffer_size);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),
                       g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n",ret);
        USB_END_LOCK();
        return 1;
    }

    auc_cmd_rxrd_clear();
    w2_usb_send_packet(udev, usb_sndbulkpipe(udev, USB_EP1), scat_list, n_sg);

    AML_PROF_CNT(SG, 0);
    USB_END_LOCK();
    return actual_length;
}

void auc_w2_ops_init(void)
{
    struct auc_hif_ops *ops = &g_auc_hif_ops;
    g_auc_kmalloc_buf = (unsigned char *)aml_mem_prealloc(PREALLOC_BUF_BUS, PREALLOC_BUF_BUS_SIZE);
    if (!g_auc_kmalloc_buf) {
         AML_ERR(">>>usb kmalloc failed!");
    }

    ops->hi_send_cmd = auc_send_cmd_ep1;
    ops->hi_write_word = auc_write_word_by_ep_for_wifi;
    ops->hi_read_word = auc_read_word_by_ep_for_wifi;
    ops->hi_write_sram = auc_write_sram_by_ep_for_wifi;
    ops->hi_read_sram = auc_read_sram_by_ep_for_wifi;
    ops->hi_rx_buffer_read = auc_rx_buffer_read;
    ops->hi_write_word_for_bt = auc_write_word_by_ep_for_bt;
    ops->hi_read_word_for_bt = auc_read_word_by_ep_for_bt;
    ops->hi_write_sram_for_bt = auc_write_sram_by_ep_for_bt;
    ops->hi_read_sram_for_bt = auc_read_sram_by_ep_for_bt;
    ops->hi_send_frame = w2_usb_send_frame;
    auc_driver_insmoded = 1;
}
#ifdef ICCM_CHECK
extern unsigned char buf_iccm_rd[ICCM_BUFFER_RD_LEN];
#endif

int wifi_iccm_download(unsigned char* addr, unsigned int len)
{
#ifdef ICCM_ROM
    unsigned int base_addr = MAC_ICCM_AHB_BASE + ICCM_ROM_LEN;
#else
    unsigned int base_addr = MAC_ICCM_AHB_BASE;
#endif
    unsigned int offset = 0;
    unsigned int trans_len = 0;
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
#ifdef ICCM_CHECK
    unsigned char *buf_tmp = buf_iccm_rd;
    memset(buf_iccm_rd, 0, ICCM_BUFFER_RD_LEN);
#endif

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_DOWNLOAD_WIFI, base_addr, 0, len);

    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n", ret);
        USB_END_LOCK();
        return 1;
    }

    while (offset < len) {
        if ((len - offset) > USB_MAX_TRANS_SIZE) {
            trans_len = USB_MAX_TRANS_SIZE;
        } else {
            trans_len = len - offset;
        }

        /* data stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void*)addr+offset, trans_len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n", ret);
            USB_END_LOCK();
            return 1;
        }

        AML_INFO("wifi_iccm_download actual_length = 0x%x; len: 0x%x; offset: 0x%x\n", actual_length, len, offset);
        offset += actual_length;
    }

    USB_END_LOCK();
#ifdef ICCM_CHECK
    auc_read_sram_by_ep_for_wifi(buf_tmp, (void*)(uintptr_t)base_addr, len, USB_EP4);

    if (memcmp(buf_tmp, addr, len)) {
        AML_ERR("write ICCM ERROR!!!! \n");
    } else {
        AML_INFO("write ICCM SUCCESS!!!! \n");
    }
#endif
    return 0;
}

int wifi_dccm_download(unsigned char* addr, unsigned int len, unsigned int start)
{
    unsigned int base_addr = 0x00d00000 + start;
    unsigned int offset = 0;
    unsigned int trans_len = 0;
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
#ifdef ICCM_CHECK
    unsigned char *buf_tmp = buf_iccm_rd;
    memset(buf_iccm_rd, 0, ICCM_BUFFER_RD_LEN);
#endif

    AML_INFO("dccm_downed, addr 0x%p, len %d \n", addr, len);
    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_DOWNLOAD_WIFI, base_addr, 0, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void*)g_cmd_buf,sizeof(*g_cmd_buf),&actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n", ret);
        USB_END_LOCK();
        return 1;
    }

    while (offset < len) {
        if ((len - offset) > USB_MAX_TRANS_SIZE) {
            trans_len = USB_MAX_TRANS_SIZE;
        } else {
            trans_len = len - offset;
        }

        /* data stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),(void *)addr+offset, trans_len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n",ret);
            USB_END_LOCK();
            return 1;
        }

        AML_INFO("wifi_dccm_download actual_length = 0x%x; len: 0x%x; offset: 0x%x\n", actual_length, len, offset);
        offset += actual_length;
    }

    USB_END_LOCK();
#ifdef ICCM_CHECK
    auc_read_sram_by_ep_for_wifi(buf_tmp, (void*)(uintptr_t)base_addr, len, USB_EP4);
    if (memcmp(buf_tmp, addr, len)) {
        AML_ERR("write DCCM ERROR!!!! \n");
    } else {
        AML_INFO("write DCCM SUCCESS!!!! \n");
    }
#endif

    return 0;
}

int wifi_fw_download(char * firmware_filename)
{
    int i = 0, err = 0;
    unsigned int tmp_val = 0;
    unsigned int len = ICCM_RAM_LEN;
    char tmp_buf[9] = {0};
    unsigned char *src = NULL;
    unsigned char *kmalloc_buf = NULL;
    //unsigned int buf[10] = {0};
    const struct firmware *fw = NULL;
#ifndef ICCM_ROM
    unsigned int offset = 0;
#else
    unsigned int offset = ICCM_ROM_LEN;
#endif

    AML_FN_ENTRY();
    err =request_firmware(&fw, firmware_filename, &g_udev->dev);
    if (err) {
        ERROR_DEBUG_OUT("request firmware fail!\n");
        return err;
    }

    src = (unsigned char *)fw->data + (offset / 4) * BYTE_IN_LINE;

#if 0
    if (fw->size < 1024 * 512 * 2) {
        len = ICCM_RAM_LEN;
    } else {
        len = ICCM_ALL_LEN;
    }
#endif

    kmalloc_buf = (unsigned char *)aml_mem_prealloc(PREALLOC_BUF_FW_DL, len);
    if (kmalloc_buf == NULL) {
        ERROR_DEBUG_OUT("kmalloc buf fail\n");
        release_firmware(fw);
        return -ENOMEM;
    }

    for (i = 0; i < len /4; i++) {
        tmp_buf[8] = 0;
        strncpy(tmp_buf, (char *)src, 8);
        if ((err = kstrtouint(tmp_buf, 16, &tmp_val))) {
            release_firmware(fw);
            //FREE(kmalloc_buf, "usb_write");
            return err;
        }
        *(unsigned int *)&kmalloc_buf[4 * i] = __swab32(tmp_val);
        src += BYTE_IN_LINE;
    }

    AML_INFO("start iccm download!\n");
    wifi_iccm_download(kmalloc_buf, len);

    memset(kmalloc_buf, 0, len);
    src = (unsigned char *)fw->data + (ICCM_ALL_LEN / 4) * BYTE_IN_LINE;

    /* download dccm section1, 0 - usb_data_start */
#define USB_ROM_DATA_LEN (2 * 1024)
#define STACK_LEN (6 * 1024)
#define DCCM_SECTION1_LEN (160 * 1024 - (STACK_LEN + USB_ROM_DATA_LEN))
#define DCCM_SECTION2_LEN (96 * 1024)

    //len = ALIGN(DCCM_ALL_LEN, 4) - (6 * 1024/*stack*/ + 2 * 1024/*usb data*/);
    len = DCCM_SECTION1_LEN;
    for (i = 0; i < len / 4; i++) {
        tmp_buf[8] = 0;
        strncpy(tmp_buf, (char *)src, 8);
        if ((err = kstrtouint(tmp_buf, 16, &tmp_val))) {
            release_firmware(fw);
            //FREE(kmalloc_buf, "usb_write");
            return err;
        }
        *(unsigned int *)&kmalloc_buf[4 * i] = __swab32(tmp_val);
        src += BYTE_IN_LINE;
    }

    AML_INFO("start dccm download!\n");
    wifi_dccm_download(kmalloc_buf, len, 0);

    /* download dccm section2, stack end to dccm end */
    memset(kmalloc_buf, 0, len);
    src += ((USB_ROM_DATA_LEN + STACK_LEN) / 4) * BYTE_IN_LINE;
    offset = len + USB_ROM_DATA_LEN + STACK_LEN;
    len = DCCM_SECTION2_LEN;

    for (i = 0; i < len / 4; i++) {
        tmp_buf[8] = 0;
        strncpy(tmp_buf, (char *)src, 8);
        if ((err = kstrtouint(tmp_buf, 16, &tmp_val))) {
            release_firmware(fw);
            //FREE(kmalloc_buf, "usb_write");
            return err;
        }
        *(unsigned int *)&kmalloc_buf[4 * i] = __swab32(tmp_val);
        src += BYTE_IN_LINE;
    }

    wifi_dccm_download(kmalloc_buf, len, offset);

    release_firmware(fw);
    //FREE(kmalloc_buf, "usb_write");

    AML_INFO("finished fw downloading!\n");
    return 0;
}

int start_wifi(void)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_START_WIFI, 0, 0, 0);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),(void *) g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    USB_END_LOCK();
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n", ret);
        return 1;
    }

    AML_INFO("start_wifi finished!\n");

    return 0;
}

EXPORT_SYMBOL(wifi_fw_download);
EXPORT_SYMBOL(start_wifi);
