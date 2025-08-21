#include "usb_common.h"
#include "chip_ana_reg.h"
#include "wifi_intf_addr.h"
#include "sg_common.h"
#include "fi_sdio.h"
#include "w2_usb.h"
#include "aml_static_buf.h"
#include "w2_sdio.h"
#include "aml_interface.h"

 /* memory mapping for wifi space */
#define MAC_ICCM_AHB_BASE    0x00000000
#define MAC_SRAM_BASE        0x00a10000
#define MAC_REG_BASE         0x00a00000
#define MAC_DCCM_AHB_BASE    0x00d00000

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
#define USB_TXCMD_CARRY_RXRD_START_INDEX 401
uint8_t rx_need_update = 0;

#define WIFI_TOP (0xa07000)
#define RG_WIFI_RST_CTRL (WIFI_TOP + 0x00)

#define WIFI_READ_CMD   0
#define BT_READ_CMD     1
#define WRITE_SRAM_DATA_LEN 477
extern struct auc_hif_ops g_auc_hif_ops;
extern struct aml_hwif_usb g_hwif_usb;
extern struct usb_device *g_udev;
extern unsigned char auc_driver_insmoded;
extern struct crg_msc_cbw *g_cmd_buf;
extern unsigned char *g_kmalloc_buf;
extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_pm_type g_wifi_pm;
unsigned char *g_auc_kmalloc_buf = NULL;


void auc_build_cbw(struct crg_msc_cbw *cbw_buf,
                               unsigned char dir,
                               unsigned int len,
                               unsigned char cdb1,
                               unsigned int cdb2,
                               unsigned long cdb3,
                               SYS_TYPE cdb4)
{
    cbw_buf->sig = AML_SIG_CBW;
    cbw_buf->tag = 0x5da729a0;
    cbw_buf->data_len = len;
    cbw_buf->flag = dir; //direction
    cbw_buf->len = 16; //command length
    cbw_buf->lun = 0;

    cbw_buf->cdb[0] = cdb1;
    cbw_buf->cdb[1] = cdb2; // read or write addr
    cbw_buf->cdb[2] = (unsigned int)(unsigned long)cdb3;
    cbw_buf->cdb[3] = cdb4; //read or write data length
}

void auc_build_cbw_add_data(struct crg_msc_cbw *cbw_buf,
                               unsigned char dir,
                               unsigned int len,
                               unsigned char cdb1,
                               unsigned int cdb2,
                               unsigned long cdb3,
                               SYS_TYPE cdb4,unsigned char *data)
{
    cbw_buf->sig = AML_SIG_CBW;
    cbw_buf->tag = 0x5da729a0;
    cbw_buf->data_len = len;
    cbw_buf->flag = dir; //direction
    cbw_buf->len = 16; //command length
    cbw_buf->lun = 0;

    cbw_buf->cdb[0] = cdb1;
    cbw_buf->cdb[1] = cdb2; // read or write addr
    cbw_buf->cdb[2] = (unsigned int)(unsigned long)cdb3;
    cbw_buf->cdb[3] = cdb4; //read or write data length
    memcpy(cbw_buf->resv + 1, (unsigned char *) data, len);
    /*in case call cmd and data mode but fw call cmd+data stage*/
    cbw_buf->resv[479] = cbw_buf->resv[480] = 0xFF;
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
    ret = usb_bulk_msg(usb_dev, pipe, data, len, actual_length, timeout);
#ifdef CONFIG_AML_RECOVERY
    if (ret && !bus_state_detect.bus_err) {
        if ((bus_state_detect.is_drv_load_finished) && (!bus_state_detect.is_recy_ongoing)) {
            bus_state_detect.bus_err = 1;
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

    kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_read_sram", GFP_DMA|GFP_ATOMIC);
    if (kmalloc_buf == NULL)
    {
        ERROR_DEBUG_OUT("kmalloc buf fail, len: %d\n", len);
        USB_END_LOCK();
        return;
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
    FREE(kmalloc_buf, "usb_read_sram");

    USB_END_LOCK();
}

void usb_isoc_callback(struct urb * urb)
{
    printk("urb: 0x%p; iso callback is called!\n", urb);
}

int auc_write_reg_ep3(unsigned int addr, unsigned int value, unsigned int len)
{
    int ret = 0;
    struct usb_device *udev = g_udev;
    struct urb *urb;

    USB_BEGIN_LOCK();

    urb = usb_alloc_urb(1, GFP_ATOMIC);

    if (!urb) {
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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
        printk("alloc urb failed!\n");
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

    data = (unsigned char *)ZMALLOC(len,"reg tmp",GFP_DMA | GFP_ATOMIC);

    if (!data) {
        ERROR_DEBUG_OUT("data malloc fail, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ep, addr, len, mode);
        USB_END_LOCK();
        return -ENOMEM;
    }

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep),(void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        FREE(data,"reg tmp");
        USB_END_LOCK();
        return ret;
    }

    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, ep), (void *)data, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, mode: %d\n", ret ,ep, addr, len, mode);
        FREE(data,"reg tmp");
        USB_END_LOCK();
        return ret;
    }

    memcpy(&reg_data, data, actual_length);
    FREE(data,"reg tmp");
    USB_END_LOCK();

    return reg_data;
}

extern int coex_flag;
void auc_write_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();
    if (coex_flag && len < WRITE_SRAM_DATA_LEN) {
        auc_build_cbw_add_data(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len,pdata);
        /* cmd stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void*)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
            ERROR_DEBUG_OUT("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
            USB_END_LOCK();
            return;
        }
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

        kmalloc_buf = (unsigned char *)ZMALLOC(len, "usb_write_sram", GFP_DMA | GFP_ATOMIC);//virt_to_phys(fwICCM);
        if (kmalloc_buf == NULL)
        {
            ERROR_DEBUG_OUT("kmalloc buf fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
            USB_END_LOCK();
            return;
        }

        memcpy(kmalloc_buf, pdata, len);
        /* data stage */
        ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d\n", ret, ep, addr, len);
            FREE(kmalloc_buf, "usb_read_sram");
            USB_END_LOCK();
            return;
        }
        FREE(kmalloc_buf, "usb_read_sram");
    }

    if (addr == CMD_DOWN_FIFO_FDH_ADDR) {
        rx_need_update = 0;
        g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 1] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 2] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 3] = 0;
        g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 4] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 5] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 6] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_START_INDEX + 7] = 0;
    }
    USB_END_LOCK();
}

void auc_read_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep, unsigned int mode)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;
    unsigned char *kmalloc_buf = NULL;

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

    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
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

void rx_read(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep, unsigned int mode)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    USB_BEGIN_LOCK();
    auc_build_cbw(g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        USB_END_LOCK();
        return;
    }
    if (mode == WIFI_READ_CMD)
        ep = USB_EP4;

    /* data stage */
    ret = auc_bulk_msg(udev, usb_rcvbulkpipe(udev, ep),(void *)pdata, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d, mode: %d\n", ret, ep, addr, len, mode);
        USB_END_LOCK();
        return;
    }

    USB_END_LOCK();
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
        printk("write_word: ep-%d unsupported\n", ep);
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
        printk("Read_word: ep-%d unsupported!\n", ep);
    }

    return value;
}

void auc_write_sram_by_ep_for_wifi(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
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
        printk("write_word: ep-%d unsupported\n", ep);
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
        printk("write_word: ep-%d unsupported\n", ep);
    }
}

void auc_rx_buffer_read(unsigned char *buf,unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    if ((ep == USB_EP4) || (ep == USB_EP5) || (ep == USB_EP6) || (ep == USB_EP7)) {
        ep = USB_EP1;
        rx_read(buf, (unsigned int)(unsigned long)sram_addr, len, ep, WIFI_READ_CMD);
    } else {
        printk("write_word: ep-%d unsupported\n", ep);
    }
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
            printk("EP-%d unsupported!\n", ep);
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
            printk("EP-%d unsupported!\n", ep);
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
            printk("EP-%d unsupported!\n", ep);
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
            printk("EP-%d unsupported!\n", ep);
            break;
    }
}

struct aml_hwif_usb *aml_usb_priv(void)
{
    return &g_hwif_usb;
}

int w2_usb_enable_scatter(void)
{
    struct aml_hwif_usb *hif_usb = aml_usb_priv();
    struct amlw_hif_scatter_req *scat_req = NULL;

    ASSERT(hif_usb != NULL);

    if (hif_usb->scatter_enabled) {
        return 0;
    }

    hif_usb->scatter_enabled = true;

    /* allocate the scatter request */
    scat_req = ZMALLOC(sizeof(struct amlw_hif_scatter_req), "usb_alloc_prep_scat_req", GFP_ATOMIC|GFP_DMA);
    if (scat_req == NULL)
    {
        ERROR_DEBUG_OUT("[usb sg alloc_scat_req]: no mem\n");
        return 1;
    }

    scat_req->free = true;
    hif_usb->scat_req = scat_req;

    return 0;

}

struct amlw_hif_scatter_req *aml_usb_scatter_req_get(void)
{
    struct aml_hwif_usb *hif_usb = aml_usb_priv();
    struct amlw_hif_scatter_req *scat_req = NULL;

    ASSERT(hif_usb != NULL);

    scat_req = hif_usb->scat_req;

    if (scat_req->free)
    {
        scat_req->free = false;
    }
    else if (scat_req->scat_count != 0) // get scat_req, but not build scatter list
    {
        scat_req = NULL;
    }

    return scat_req;
}

void aml_usb_cleanup_scatter(void)
{
    struct aml_hwif_usb *hif_usb = aml_usb_priv();
    PRINT("[usb sg cleanup]: enter\n");

    ASSERT(hif_usb != NULL);

    if (!hif_usb->scatter_enabled)
        return;

    hif_usb->scatter_enabled = false;

    /* empty the free list */
     FREE(hif_usb->scat_req, "usb_alloc_prep_scat_req");

    PRINT("[usb sg cleanup]: exit\n");

    return;
}

void w2_usb_scat_complete (struct amlw_hif_scatter_req * scat_req)
{
    int  i;

    ASSERT(scat_req != NULL);

    if (scat_req->complete)
    {
        for (i = 0; i < scat_req->scat_count; i++)
        {
            (scat_req->complete)(scat_req->scat_list[i].skbbuf);
            scat_req->scat_list[i].skbbuf = NULL;
        }
    }
    scat_req->free = true;
    scat_req->scat_count = 0;
    scat_req->len = 0;
    scat_req->addr = 0;
    memset(scat_req->sgentries, 0, MAX_SG_ENTRIES * sizeof(struct scatterlist));

}

void aml_usb_build_tx_packet_info(struct crg_msc_cbw *cbw_buf, unsigned char cdb1,
    struct tx_trb_info_ex * trb_info)
{
    cbw_buf->sig = trb_info->buffer_size[0] | trb_info->buffer_size[1] << 16;
    cbw_buf->tag = trb_info->buffer_size[2] | trb_info->buffer_size[3] << 16;
    cbw_buf->data_len = trb_info->buffer_size[4] | trb_info->buffer_size[5] << 16;
    cbw_buf->flag = trb_info->packet_num; //packet nums 1byte
    cbw_buf->len = trb_info->buffer_size[13] & 0xff;
    cbw_buf->lun = (trb_info->buffer_size[13] >> 8) & 0xff;
    cbw_buf->cdb[0] = cdb1 | trb_info->buffer_size[12] << 16;
    cbw_buf->cdb[1] = trb_info->buffer_size[6] | trb_info->buffer_size[7] << 16;
    cbw_buf->cdb[2] = trb_info->buffer_size[8] | trb_info->buffer_size[9] << 16;
    cbw_buf->cdb[3] = trb_info->buffer_size[10] | trb_info->buffer_size[11] << 16;

    {
        int i=0,j;
        if (trb_info->packet_num >= 15) {
            for (j=14;j<trb_info->packet_num;j++) {
                cbw_buf->resv[i] = trb_info->buffer_size[j] & 0xff;
                cbw_buf->resv[i+1] = (trb_info->buffer_size[j]>> 8) & 0xff;
                i=i+2;
            }
        }
    }

    if (rx_need_update != 0)
        rx_need_update = 0;
}
int w2_usb_send_packet(struct amlw_hif_scatter_req * scat_req)
{
    struct usb_device *udev = g_udev;
    struct scatterlist *sg;
    struct usb_sg_request sgr;
    int sg_count, sgitem_count;
    unsigned int max_req_size;
    int ttl_len, pkt_offset, page_num;
    //struct txdesc_host *txdesc_host;

    unsigned int last_page_size ;
    unsigned int ttl_page_num = 0;
    int ret;
    //int i;

    /* fill SG entries */
    sg = scat_req->sgentries;
    pkt_offset = 0; // reminder
    sgitem_count = 0; // count of scatterlist
    max_req_size = USB_MAX_TRANS_SIZE;
    udev->bus->sg_tablesize = MAXSG_SIZE;

    while (sgitem_count < scat_req->scat_count)
    {
        ttl_len = 0;
        sg_count = 0;
        page_num = 0;
        sg_init_table(sg, MAXSG_SIZE);
        /* assemble SG list */
        while (sgitem_count < scat_req->scat_count)
        {
            int packet_len = 0;
            unsigned char *pdata = NULL;
            packet_len = scat_req->scat_list[sgitem_count].len;

            pdata = scat_req->scat_list[sgitem_count].packet;
            page_num = scat_req->scat_list[sgitem_count].page_num;

            if (sg_count > (MAXSG_SIZE - page_num))
            {
                printk("sg_count > MAXSG_SIZE, sg_count:%d, page_num:%d, scat_count:%d\n", sg_count, page_num, scat_req->scat_count);
                break;
            }
            ttl_page_num += page_num;
            last_page_size = packet_len - (page_num - 1) * USB_PAGE_LEN;

            if (page_num == 1)
            {
                //printk("sg_count:%d, page_num:%d, scat_count:%d\n", sg_count, page_num, scat_req->scat_count);
                sg_set_buf(&scat_req->sgentries[sg_count], pdata, packet_len);
                sg_count++;
                ttl_len += packet_len;
            }
            sgitem_count++;
        }

        ret = usb_sg_init(&sgr, udev, usb_sndbulkpipe(udev, USB_EP1), 0, scat_req->sgentries,
            sg_count, 0, GFP_NOIO);

        if (ret)
        {
            printk("usb_sg_init fail ret = %d\n", ret);
            return ret;
        }

        usb_sg_wait(&sgr);
        if (sgr.status != 0)
        {
            printk("usb_sg_wait fail  %d\n", sgr.status);
            return -1;
        }

    }
    return 0;
}

int w2_usb_send_frame(struct amlw_hif_scatter_req * pframe)
{
    int ret;
    int i;
    unsigned int actual_length;
    struct usb_device *udev = g_udev;


    memset(&pframe->page, 0, sizeof(struct tx_trb_info_ex));
#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing) {
        ERROR_DEBUG_OUT("bus err or reset is on going(bus err %d, bus reset ongoing: %d)\n",
            bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
        w2_usb_scat_complete(pframe);
        return 0;
    }
#endif
    USB_BEGIN_LOCK();
    /* build page_info array */
    pframe->page.packet_num = pframe->scat_count;

    for (i = 0; i < pframe->scat_count; i++)
    {
        pframe->page.buffer_size[i] = pframe->scat_list[i].len;
    }
    aml_usb_build_tx_packet_info(g_cmd_buf, CMD_WRITE_PACKET, &(pframe->page));
    /* cmd stage */
    ret = auc_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),
        g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        ERROR_DEBUG_OUT("Failed to usb_bulk_msg, ret %d\n",ret);
        USB_END_LOCK();
        return 1;
    }

    g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 1] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 2] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 3] = 0;
    g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 4] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 5] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 6] = g_cmd_buf->resv[USB_TXCMD_CARRY_RXRD_INDEX + 7] = 0;
    w2_usb_send_packet(pframe);

    w2_usb_scat_complete(pframe);

    USB_END_LOCK();
    return 0;
}

void auc_w2_ops_init(void)
{
    struct auc_hif_ops *ops = &g_auc_hif_ops;
    g_auc_kmalloc_buf = (unsigned char *)aml_mem_prealloc(AML_PREALLOC_SDIO, WLAN_AML_SDIO_SIZE);
    if (!g_auc_kmalloc_buf) {
         printk(">>>usb kmalloc failed!");
    }

    ops->hi_send_cmd = auc_send_cmd_ep1;
    ops->hi_write_word = auc_write_word_by_ep_for_wifi;
    ops->hi_read_word = auc_read_word_by_ep_for_wifi;
    ops->hi_write_sram = auc_write_sram_by_ep_for_wifi;
    ops->hi_read_sram = auc_read_sram_by_ep_for_wifi;
    ops->hi_rx_buffer_read = auc_rx_buffer_read;
    ops->hi_get_scatreq = aml_usb_scatter_req_get;
    ops->hi_cleanup_scat = aml_usb_cleanup_scatter;
    ops->hi_enable_scat = w2_usb_enable_scatter;
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
    struct auc_hif_ops *hif_ops = &g_auc_hif_ops;
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

        PRINT("wifi_iccm_download actual_length = 0x%x; len: 0x%x; offset: 0x%x\n", actual_length, len, offset);
        offset += actual_length;
    }

    USB_END_LOCK();
#ifdef ICCM_CHECK
    hif_ops->hi_read_sram(buf_tmp,
                    (unsigned char*)(SYS_TYPE)(base_addr), len, USB_EP4);

    if (memcmp(buf_tmp, addr, len)) {
        PRINT("%s, %d: write ICCM ERROR!!!! \n", __func__, __LINE__);
    } else {
        PRINT("%s, %d: write ICCM SUCCESS!!!! \n", __func__, __LINE__);
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
    struct auc_hif_ops *hif_ops = &g_auc_hif_ops;
#ifdef ICCM_CHECK
    unsigned char *buf_tmp = buf_iccm_rd;
    memset(buf_iccm_rd, 0, ICCM_BUFFER_RD_LEN);
#endif

    PRINT("dccm_downed, addr 0x%p, len %d \n", addr, len);
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

        PRINT("wifi_dccm_download actual_length = 0x%x; len: 0x%x; offset: 0x%x\n", actual_length, len, offset);
        offset += actual_length;
    }

    USB_END_LOCK();
#ifdef ICCM_CHECK
    hif_ops->hi_read_sram(buf_tmp,
            (unsigned char*)(SYS_TYPE)base_addr, len, USB_EP4);
    if (memcmp(buf_tmp, addr, len)) {
        PRINT("%s, %d: write DCCM ERROR!!!! \n", __func__, __LINE__);
    } else {
        PRINT("%s, %d: write DCCM SUCCESS!!!! \n", __func__, __LINE__);
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

    printk("%s: %d\n", __func__, __LINE__);
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

    kmalloc_buf = (unsigned char *)aml_mem_prealloc(AML_PREALLOC_DOWNLOAD_FW, len);
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

    printk("start iccm download!\n");
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

    printk("start dccm download!\n");
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

    printk("finished fw downloading!\n");
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

    printk("start_wifi finished!\n");

    return 0;
}

EXPORT_SYMBOL(wifi_fw_download);
EXPORT_SYMBOL(start_wifi);
EXPORT_SYMBOL(rx_need_update);

