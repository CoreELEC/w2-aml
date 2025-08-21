#include <linux/init.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include "aml_static_buf.h"
#include "aml_interface.h"
#include "aml_compat.h"

char *bus_type = "pci";
unsigned int aml_bus_type;
unsigned char wifi_drv_rmmod_ongoing = 0;
struct aml_bus_state_detect bus_state_detect;
struct aml_pm_type g_wifi_pm = {0};

EXPORT_SYMBOL(bus_state_detect);
EXPORT_SYMBOL(wifi_drv_rmmod_ongoing);
EXPORT_SYMBOL(bus_type);
EXPORT_SYMBOL(aml_bus_type);
EXPORT_SYMBOL(g_wifi_pm);

extern int aml_usb_insmod(void);
extern int aml_usb_rmmod(void);
extern int aml_sdio_insmod(void);
extern int aml_sdio_rmmod(void);
extern int aml_pci_insmod(void);
extern int aml_pci_rmmod(void);
extern void aml_sdio_reset(void);
extern void aml_usb_reset(void);

void bus_detect_work(struct work_struct *p_work)
{
    printk("%s: enter\n", __func__);
    if (aml_bus_type == SDIO_MODE) {
        aml_sdio_reset();
    } else if (aml_bus_type == USB_MODE) {
        aml_usb_reset();
    }
    bus_state_detect.bus_err = 0;
    if (bus_state_detect.insmod_drv) {
        bus_state_detect.is_load_by_timer = 1;
        bus_state_detect.insmod_drv();
    }
    bus_state_detect.bus_reset_ongoing = 0;

    return;
}
static void state_detect_cb(struct timer_list* t)
{

    if ((bus_state_detect.bus_err == 2) && (!bus_state_detect.bus_reset_ongoing)) {
        bus_state_detect.bus_reset_ongoing = 1;
        schedule_work(&bus_state_detect.detect_work);
    }
    if (!bus_state_detect.is_drv_load_finished || (bus_state_detect.bus_err == 2)) {
        mod_timer(&bus_state_detect.timer, jiffies + AML_SDIO_STATE_MON_INTERVAL);
    } else {

        printk("%s: stop bus detected state timer\n", __func__);
    }
}

void aml_bus_state_detect_init()
{
    bus_state_detect.bus_err = 0;
    bus_state_detect.bus_reset_ongoing = 0;
    bus_state_detect.is_drv_load_finished = 0;
    bus_state_detect.is_load_by_timer = 0;
    INIT_WORK(&bus_state_detect.detect_work, bus_detect_work);
    timer_setup(&bus_state_detect.timer, state_detect_cb, 0);
    mod_timer(&bus_state_detect.timer, jiffies + AML_SDIO_STATE_MON_INTERVAL);
}
void aml_bus_state_detect_deinit()
{
    del_timer_sync(&bus_state_detect.timer);
    bus_state_detect.bus_err = 0;
    bus_state_detect.bus_reset_ongoing = 0;
    bus_state_detect.is_drv_load_finished = 0;
}

int aml_bus_intf_insmod(void)
{
    int ret;
    if (aml_init_wlan_mem()) {
        printk("aml_init_wlan_mem fail\n");
        return -EPERM;
    }
    if (strncmp(bus_type,"usb",3) == 0) {
        aml_bus_type = USB_MODE;
        ret = aml_usb_insmod();
        if (ret) {
            printk("aml usb bus init fail\n");
        }
    } else if (strncmp(bus_type,"sdio",4) == 0) {
        aml_bus_type = SDIO_MODE;
        ret = aml_sdio_insmod();
        if (ret) {
            printk("aml sdio bus init fail\n");
#ifdef CONFIG_PT_MODE
            return ret;
#endif
        }
    } else if (strncmp(bus_type,"pci",3) == 0) {
        aml_bus_type = PCIE_MODE;
        ret = aml_pci_insmod();
        if (ret) {
            printk("aml sdio bus init fail\n");
        }
    }
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.drv_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.is_shut_down, 0);
#ifndef CONFIG_PT_MODE
    if (aml_bus_type == SDIO_MODE) {
        aml_bus_state_detect_init();
    }
#endif

    return 0;
}
void aml_bus_intf_rmmod(void)
{
    if (strncmp(bus_type,"usb",3) == 0) {
        aml_usb_rmmod();
    } else if (strncmp(bus_type,"sdio",4) == 0) {
        aml_sdio_rmmod();
    } else if (strncmp(bus_type,"pci",3) == 0) {
        aml_pci_rmmod();
    }
#ifndef CONFIG_PT_MODE
    if (aml_bus_type == SDIO_MODE) {
        aml_bus_state_detect_deinit();
    }
#endif
    aml_deinit_wlan_mem();
}

lp_shutdown_func g_lp_shutdown_func = NULL;

EXPORT_SYMBOL(aml_bus_state_detect_deinit);
module_param(bus_type, charp,S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(bus_type,"A string variable to adjust pci or sdio or usb bus interface");
module_init(aml_bus_intf_insmod);
module_exit(aml_bus_intf_rmmod);

MODULE_LICENSE("GPL");

EXPORT_SYMBOL(g_lp_shutdown_func);
