#include "usb_common.h"
#include "chip_ana_reg.h"
#include "wifi_intf_addr.h"
#include "sg_common.h"
#include "fi_sdio.h"
#include "w2_usb.h"
#include "aml_interface.h"
#include "fi_w2_sdio.h"
#include "chip_intf_reg.h"
#include "aml_interface.h"

struct auc_hif_ops g_auc_hif_ops;
struct usb_device *g_udev = NULL;
struct aml_hwif_usb g_hwif_usb;
unsigned char auc_driver_insmoded;
unsigned char auc_wifi_in_insmod;
unsigned char g_chip_function_ctrl = 0;
unsigned char g_usb_after_probe;
struct crg_msc_cbw *g_cmd_buf = NULL;
struct mutex auc_usb_mutex;
unsigned char *g_kmalloc_buf;
extern unsigned char wifi_drv_rmmod_ongoing;
extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_pm_type g_wifi_pm;
extern void auc_w2_ops_init(void);
extern void extern_wifi_set_enable(int is_on);
/*for bluetooth get read/write point*/
int bt_wt_ptr = 0;
int bt_rd_ptr = 0;
/*co-exist flag for bt/wifi mode*/
int coex_flag = 0;
static int auc_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    g_udev = usb_get_dev(interface_to_usbdev(interface));
    memset(g_kmalloc_buf,0,1024*20);
    memset(g_cmd_buf,0,sizeof(struct crg_msc_cbw ));
    g_usb_after_probe = 1;

    auc_w2_ops_init();
    g_auc_hif_ops.hi_enable_scat();
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    }
#endif
    PRINT("%s(%d)\n",__func__,__LINE__);
    return 0;
}


static void auc_disconnect(struct usb_interface *interface)
{
    usb_set_intfdata(interface, NULL);
    usb_put_dev(g_udev);
    g_usb_after_probe = 0;
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    PRINT("--------aml_usb:disconnect-------\n");
}

#ifdef CONFIG_PM
static int auc_reset_resume(struct usb_interface *interface)
{
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    PRINT("--------aml_usb:reset done-------\n");
    return 0;
}

static int auc_suspend(struct usb_interface *interface,pm_message_t state)
{
    int cnt = 0;

    if (atomic_read(&g_wifi_pm.wifi_enable))
    {
        while (atomic_read(&g_wifi_pm.drv_suspend_cnt) == 0)
        {
            msleep(50);
            cnt++;
            if (cnt > 40)
            {
                PRINT("wifi suspend fail \n");
                return -1;
            }
        }
    }

    atomic_set(&g_wifi_pm.bus_suspend_cnt, 1);
    PRINT("---------aml_usb suspend-------\n");
    return 0;
}

static int auc_resume(struct usb_interface *interface)
{
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    return 0;
}
#endif

extern lp_shutdown_func g_lp_shutdown_func;
void auc_shutdown(struct device *dev)
{
    //Mask interrupt reporting to the host
    atomic_set(&g_wifi_pm.is_shut_down, 2);

    // Notify fw to enter shutdown mode
    if (g_lp_shutdown_func != NULL)
    {
        g_lp_shutdown_func();
    }

    //notify fw shutdown
    //notify bt wifi will go shutdown
    auc_write_word_by_ep_for_wifi(RG_AON_A55, auc_read_word_by_ep_for_wifi(RG_AON_A55, USB_EP4)|BIT(28) ,USB_EP4);

    atomic_set(&g_wifi_pm.is_shut_down, 1);
}

static const struct usb_device_id auc_devices[] =
{
    {USB_DEVICE(W2_VENDOR,W2_PRODUCT)},
    {USB_DEVICE(W2u_VENDOR_AMLOGIC_EFUSE,W2u_PRODUCT_A_AMLOGIC_EFUSE)},
    {USB_DEVICE(W2u_VENDOR_AMLOGIC_EFUSE,W2u_PRODUCT_B_AMLOGIC_EFUSE)},
    {}
};

MODULE_DEVICE_TABLE(usb, auc_devices);

static struct usb_driver aml_usb_common_driver = {

    .name = "aml_usb_common",
    .id_table = auc_devices,
    .probe = auc_probe,
    .disconnect = auc_disconnect,
#ifdef CONFIG_PM
    .reset_resume = auc_reset_resume,
    .suspend = auc_suspend,
    .resume = auc_resume,
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 8, 0)
    .drvwrap.driver.shutdown = auc_shutdown,
#endif
};


int aml_usb_insmod(void)
{
    int err = 0;

    g_cmd_buf = ZMALLOC(sizeof(*g_cmd_buf), "cmd stage", GFP_DMA | GFP_ATOMIC);
    if (!g_cmd_buf) {
        PRINT("g_cmd_buf malloc fail\n");
        return -ENOMEM;
    }
    g_kmalloc_buf = (unsigned char *)ZMALLOC(20*1024, "reg tmp", GFP_DMA | GFP_ATOMIC);
    if (!g_kmalloc_buf) {
        ERROR_DEBUG_OUT("data malloc fail\n");
        FREE(g_cmd_buf, "cmd stage");
        return -ENOMEM;
    }
    err = usb_register(&aml_usb_common_driver);
    if (err) {
        PRINT("failed to register usb driver: %d \n", err);
    }
    auc_driver_insmoded = 1;
    auc_wifi_in_insmod = 0;
    USB_LOCK_INIT();
    PRINT("%s(%d) aml common driver insmod\n", __func__, __LINE__);

    return err;
}

void aml_usb_rmmod(void)
{
    usb_deregister(&aml_usb_common_driver);
    auc_driver_insmoded = 0;
    wifi_drv_rmmod_ongoing = 0;
    g_auc_hif_ops.hi_cleanup_scat();
    FREE(g_cmd_buf, "cmd stage");
    FREE(g_kmalloc_buf, "reg tmp");
    USB_LOCK_DESTROY();
#ifndef CONFIG_PT_MODE
#ifndef CONFIG_LINUXPC_VERSION
    extern_wifi_set_enable(0);
    msleep(100);
    extern_wifi_set_enable(1);
#endif
#endif

   PRINT("%s(%d) aml common driver rmsmod\n",__func__, __LINE__);
}
void aml_usb_reset(void)
{
    uint32_t count = 0;
    uint32_t try_cnt = 0;

Try_again:
    printk("%s: ******* usb reset begin *******\n", __func__);

#ifndef CONFIG_PT_MODE

#ifndef CONFIG_LINUXPC_VERSION
    extern_wifi_set_enable(0);
    while (g_usb_after_probe) {
        msleep(5);
        count++;
        if (count > 40 && try_cnt <= 3) {
            count = 0;
            try_cnt++;
            extern_wifi_set_enable(1);
            msleep(50);
            printk("%s: %d usb reset fail, try again(%d)\n", __func__, __LINE__, try_cnt);
            goto Try_again;
        }
    }
    extern_wifi_set_enable(1);
#endif

    count = 0;
    try_cnt = 0;
    while ((!g_usb_after_probe) && try_cnt <= 3) {
        msleep(5);
        count++;
        if (count > 200) {
            count = 0;
            try_cnt++;
            printk("%s: %d usb reset fail, try again(%d)\n", __func__, __LINE__, try_cnt);
            goto Try_again;
        }
    };
    bus_state_detect.bus_reset_ongoing = 0;
    bus_state_detect.bus_err = 0;
    printk("%s: ******* usb reset end *******\n", __func__);

    return;
#endif
}
EXPORT_SYMBOL(aml_usb_reset);
EXPORT_SYMBOL(aml_usb_insmod);
EXPORT_SYMBOL(aml_usb_rmmod);
EXPORT_SYMBOL(g_cmd_buf);
EXPORT_SYMBOL(g_auc_hif_ops);
EXPORT_SYMBOL(g_udev);
EXPORT_SYMBOL(auc_driver_insmoded);
EXPORT_SYMBOL(auc_wifi_in_insmod);
EXPORT_SYMBOL(auc_usb_mutex);
EXPORT_SYMBOL(g_usb_after_probe);
EXPORT_SYMBOL(bt_wt_ptr);
EXPORT_SYMBOL(bt_rd_ptr);
EXPORT_SYMBOL(coex_flag);
EXPORT_SYMBOL(g_chip_function_ctrl);

