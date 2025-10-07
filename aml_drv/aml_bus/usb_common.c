#define AML_MODULE  USB

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/clock.h>
#endif

#include "usb_common.h"
#include "chip_ana_reg.h"
#include "wifi_intf_addr.h"
#include "fi_sdio.h"
#include "w2_usb.h"
#include "aml_interface.h"
#include "fi_w2_sdio.h"
#include "chip_intf_reg.h"
#include "aml_interface.h"
#include "chip_bt_pmu_reg.h"
#include "aml_log.h"

struct auc_hif_ops w2_g_auc_hif_ops;
struct usb_device *w2_g_udev = NULL;
unsigned char w2_auc_driver_insmoded;
unsigned char w2_auc_wifi_in_insmod;
unsigned char w2_g_chip_function_ctrl = 0;
unsigned char w2_g_usb_after_probe;
struct crg_msc_cbw *g_cmd_buf = NULL;
struct mutex w2_auc_usb_mutex;
unsigned char *g_kmalloc_buf;
extern unsigned char w2_wifi_drv_rmmod_ongoing;
extern struct aml_bus_state_detect w2_bus_state_detect;
extern struct aml_pm_type w2_g_wifi_pm;
extern void auc_w2_ops_init(void);

/*for bluetooth get read/write point*/
int w2_bt_wt_ptr = 0;
int w2_bt_rd_ptr = 0;
/*co-exist flag for bt/wifi mode*/
int w2_coex_flag = 0;
struct wakeup_source *w2_aml_wifi_wakeup_source;

static int auc_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    w2_g_udev = usb_get_dev(interface_to_usbdev(interface));
    memset(g_kmalloc_buf,0,1024*20);
    memset(g_cmd_buf,0,sizeof(struct crg_msc_cbw ));

    auc_w2_ops_init();
#ifdef CONFIG_PM
    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt)) {
        atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    }
#endif
    w2_g_aml_device_id = id->idProduct;

    AML_INFO("device id 0x%x\n", w2_g_aml_device_id);
    AML_INFO("done.\n");

    w2_g_usb_after_probe = 1;

#ifdef CONFIG_AML_USB_HOTPLUG
    if (bus_state_detect.auc_wifi_enable_func)
        bus_state_detect.auc_wifi_enable_func();
    bus_state_detect.usb_unplug = 0;
#endif

    return 0;
}

static void auc_disconnect(struct usb_interface *interface)
{
    usb_set_intfdata(interface, NULL);
    usb_put_dev(w2_g_udev);
    w2_g_usb_after_probe = 0;
    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    atomic_set(&w2_g_wifi_pm.drv_suspend_cnt, 0);
    AML_INFO("--------aml_usb:disconnect-------\n");
}

#ifdef CONFIG_PM
static int auc_reset_resume(struct usb_interface *interface)
{
    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    AML_INFO("--------aml_usb:reset done-------\n");
    return 0;
}

static int auc_suspend(struct usb_interface *interface,pm_message_t state)
{
    u64 start_time_ns;
    u64 elapsed_time_ns = 0;
    u64 wait_bt_time_ns = 8000000000; //wait bt 8s
    u64 wait_wifi_time_ns = 12000000000; //wait wifi 12s

    AML_INFO("auc_suspend!! \n");

    //bt open
    if ((auc_read_word_by_ep_for_bt(RG_BT_PMU_A16, USB_EP1) & BIT(31)))
    {
        start_time_ns = sched_clock();
        //bt drv suspend set bit25
        while ((auc_read_word_by_ep_for_bt(RG_AON_A24, USB_EP1) & BIT(25)) &&
                (w2_bus_state_detect.bus_err == 0) &&
                (w2_bus_state_detect.is_recy_ongoing == 0) &&
                (elapsed_time_ns < wait_bt_time_ns))
        {
            elapsed_time_ns = sched_clock() - start_time_ns;
            msleep(10);
        }

        if (elapsed_time_ns >= wait_bt_time_ns)
        {
            AML_INFO("bt suspend fail, return\n");
        }

        // Detect a bus error or ongoing recovery,
        // exit immediately to prevent blocking the kernel USB resume call.
        if (w2_bus_state_detect.bus_err || w2_bus_state_detect.is_recy_ongoing)
        {
            AML_INFO("Detect a bus error or ongoing recovery, return\n");
            return 0;
        }
    }

    elapsed_time_ns = 0;
    if (atomic_read(&w2_g_wifi_pm.wifi_enable))
    {
        start_time_ns = sched_clock();
        while ((atomic_read(&w2_g_wifi_pm.drv_suspend_cnt) == 0) &&
                (w2_bus_state_detect.bus_err == 0) &&
                (w2_bus_state_detect.is_recy_ongoing == 0) &&
                (atomic_read(&w2_g_wifi_pm.wifi_suspend_state) == 0) &&
                (elapsed_time_ns < wait_wifi_time_ns))
        {
            elapsed_time_ns = sched_clock() - start_time_ns;
            msleep(10);
        }

        if (elapsed_time_ns >= wait_wifi_time_ns)
        {
            AML_INFO("wifi suspend fail, return\n");
        }

        if (atomic_read(&w2_g_wifi_pm.wifi_suspend_state) != 0)
        {
            AML_INFO("Detect wifi suspend fail\n");
            return 0;
        }

        // Detect a bus error or ongoing recovery,
        // exit immediately to prevent blocking the kernel USB resume call.
        if (w2_bus_state_detect.bus_err || w2_bus_state_detect.is_recy_ongoing)
        {
            AML_INFO("Detect a bus error or ongoing recovery, return\n");
            return 0;
        }
    }

    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 1);
    AML_INFO("---------aml_usb suspend-------\n");
    return 0;
}

static int auc_resume(struct usb_interface *interface)
{
    AML_INFO("auc_resume!! \n");

    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    return 0;
}
#endif

extern lp_shutdown_func w2_g_lp_shutdown_func;
extern bt_shutdown_func w2_g_bt_shutdown_func;

void auc_shutdown(struct device *dev)
{
    AML_INFO("auc_shutdown begin \n");

    //Mask interrupt reporting to the host
    atomic_set(&w2_g_wifi_pm.is_shut_down, 2);

    // Notify fw to enter shutdown mode
    if (w2_g_bt_shutdown_func != NULL)
    {
        w2_g_bt_shutdown_func();
    }

    if (w2_g_lp_shutdown_func != NULL)
    {
        w2_g_lp_shutdown_func();
    }

    //notify fw shutdown
    //notify bt wifi will go shutdown
    auc_write_word_by_ep_for_wifi(RG_AON_A16, auc_read_word_by_ep_for_wifi(RG_AON_A16, USB_EP4)|BIT(28) ,USB_EP4);

    atomic_set(&w2_g_wifi_pm.is_shut_down, 1);
}

static const struct usb_device_id auc_devices[] =
{
    {USB_DEVICE(W2_VENDOR,W2_PRODUCT)},
    {USB_DEVICE(W2u_VENDOR_AMLOGIC_EFUSE,W2u_PRODUCT_A_AMLOGIC_EFUSE)},
    {USB_DEVICE(W2u_VENDOR_AMLOGIC_EFUSE,W2u_PRODUCT_B_AMLOGIC_EFUSE)},
    {USB_DEVICE(W2u_VENDOR_AMLOGIC_EFUSE,W2u_PRODUCT_C_AMLOGIC_EFUSE)},
    {}
};

//MODULE_DEVICE_TABLE(usb, auc_devices);

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

/**
 * w2_aml_usb_set_bus_err - Set the bus error state and handle system wakeup
 *
 * Updates the bus error state. If `bus_err` is non-zero, and if the
 * wakeup source is initialized but not active, the system is kept awake
 * to prevent suspend during recovery.
 *
 * @bus_err: The bus error state. A non-zero value indicates an error.
 */
void w2_aml_usb_set_bus_err(unsigned char bus_err)
{
    if (bus_err) {
        // Wake up the system and prevent it from entering
        // suspend during the upcoming recovery process.
        if (w2_aml_wifi_wakeup_source && (!w2_aml_wifi_wakeup_source->active)) {
            __pm_stay_awake(w2_aml_wifi_wakeup_source);
        } else {
            AML_INFO("w2_aml_wifi_wakeup_source is not initialized or active already\n");
        }
    }

    w2_bus_state_detect.bus_err = bus_err;

    AML_INFO("Bus error state updated: %d\n", bus_err);
}
EXPORT_SYMBOL(w2_aml_usb_set_bus_err);

int w2_aml_usb_insmod(void)
{
    int err;

    g_cmd_buf = ZMALLOC(sizeof(*g_cmd_buf), "cmd stage", GFP_DMA | GFP_ATOMIC);
    if (!g_cmd_buf) {
        AML_INFO("g_cmd_buf malloc fail\n");
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
        AML_INFO("failed to register usb driver: %d \n", err);
    }
    w2_auc_driver_insmoded = 1;
    w2_auc_wifi_in_insmod = 0;
    USB_LOCK_INIT();
    AML_INFO("aml common driver insmod\n");

    w2_aml_wifi_wakeup_source = wakeup_source_register(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
             NULL,
#endif
             "bus_wakeup_source");
    if (!w2_aml_wifi_wakeup_source) {
        AML_INFO("Failed to create wakeup source\n");
        return -ENOMEM;
    }

    return 0;
}
EXPORT_SYMBOL(w2_aml_usb_insmod);

void w2_aml_usb_rmmod(void)
{
    usb_deregister(&aml_usb_common_driver);
    w2_auc_driver_insmoded = 0;
    w2_wifi_drv_rmmod_ongoing = 0;
    FREE(g_cmd_buf, "cmd stage");
    FREE(g_kmalloc_buf, "reg tmp");
    USB_LOCK_DESTROY();

    w2_aml_wifi_power_on(0);
    msleep(100);
    w2_aml_wifi_power_on(1);

    if (w2_aml_wifi_wakeup_source) {
        wakeup_source_unregister(w2_aml_wifi_wakeup_source);
        w2_aml_wifi_wakeup_source = NULL;
    } else {
        AML_INFO("w2_aml_wifi_wakeup_source is not initialized, unregistering is not required.\n");
    }
    AML_INFO("aml common driver rmsmod\n");
}
EXPORT_SYMBOL(w2_aml_usb_rmmod);

void w2_aml_usb_reset(void)
{
#ifndef CONFIG_PT_MODE
    uint32_t count = 0;
    uint32_t try_cnt = 0;

try_again:
    AML_INFO("******* usb reset begin *******\n");

#ifndef CONFIG_PT_MODE
    w2_aml_wifi_power_on(0);
    while ((w2_g_usb_after_probe) && (try_cnt <= 3)) {
        msleep(5);
        count++;
        if (count > 200 && try_cnt < 1) {
            count = 0;
            try_cnt++;
            w2_aml_wifi_power_on(1);
            msleep(50);
            AML_ERR("usb reset fail, try again(%d)\n", try_cnt);
            goto try_again;
        }
    }
    w2_aml_wifi_power_on(1);

    count = 0;
    while ((!w2_g_usb_after_probe) && (try_cnt < 1)) {
        msleep(5);
        count++;
        if (count > 200) {
            count = 0;
            try_cnt++;
            AML_ERR("usb reset fail, try again(%d)\n", try_cnt);
            goto try_again;
        }
    };

    if ((w2_g_usb_after_probe == 0) || (try_cnt >= 1)) {
        AML_ERR("usb reset fail, usb may be unplug\n");
        return -ETIMEDOUT;
    }

    w2_bus_state_detect.bus_reset_ongoing = 0;
    w2_bus_state_detect.bus_err = 0;
    AML_INFO("******* usb reset end *******\n");
#endif
    return 0;
}
EXPORT_SYMBOL(w2_aml_usb_reset);

EXPORT_SYMBOL(w2_g_auc_hif_ops);
EXPORT_SYMBOL(w2_g_udev);
EXPORT_SYMBOL(w2_auc_driver_insmoded);
EXPORT_SYMBOL(w2_auc_wifi_in_insmod);
EXPORT_SYMBOL(w2_auc_usb_mutex);
EXPORT_SYMBOL(w2_g_usb_after_probe);
EXPORT_SYMBOL(w2_bt_wt_ptr);
EXPORT_SYMBOL(w2_bt_rd_ptr);
EXPORT_SYMBOL(w2_coex_flag);
EXPORT_SYMBOL(w2_g_chip_function_ctrl);
EXPORT_SYMBOL(w2_aml_wifi_wakeup_source);
