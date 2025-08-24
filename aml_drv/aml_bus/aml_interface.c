#include <linux/init.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include "aml_static_buf.h"
#include "aml_interface.h"
#include "aml_compat.h"

char *w2_bus_type = "sdio";
unsigned int w2_aml_bus_type;
unsigned char w2_wifi_drv_rmmod_ongoing = 0;
struct aml_bus_state_detect w2_bus_state_detect;
struct aml_pm_type w2_g_wifi_pm = {0};

EXPORT_SYMBOL(w2_bus_state_detect);
EXPORT_SYMBOL(w2_wifi_drv_rmmod_ongoing);
EXPORT_SYMBOL(w2_bus_type);
EXPORT_SYMBOL(w2_aml_bus_type);
EXPORT_SYMBOL(w2_g_wifi_pm);

extern int w2_aml_usb_insmod(void);
extern int w2_aml_usb_rmmod(void);
extern int w2_aml_sdio_insmod(void);
extern int w2_aml_sdio_rmmod(void);
extern int w2_aml_pci_insmod(void);
extern int w2_aml_pci_rmmod(void);
extern void w2_aml_sdio_reset(void);
extern void w2_aml_usb_reset(void);

void bus_detect_work(struct work_struct *p_work)
{
    printk("%s: enter\n", __func__);
    if (w2_aml_bus_type == SDIO_MODE) {
        w2_aml_sdio_reset();
    } else if (w2_aml_bus_type == USB_MODE) {
        w2_aml_usb_reset();
    }
    w2_bus_state_detect.bus_err = 0;
    if (w2_bus_state_detect.insmod_drv) {
        w2_bus_state_detect.is_load_by_timer = 1;
        w2_bus_state_detect.insmod_drv();
    }
    w2_bus_state_detect.bus_reset_ongoing = 0;

    return;
}
static void state_detect_cb(struct timer_list* t)
{

    if ((w2_bus_state_detect.bus_err == 2) && (!w2_bus_state_detect.bus_reset_ongoing)) {
        w2_bus_state_detect.bus_reset_ongoing = 1;
        schedule_work(&w2_bus_state_detect.detect_work);
    }
    if (!w2_bus_state_detect.is_drv_load_finished || (w2_bus_state_detect.bus_err == 2)) {
        mod_timer(&w2_bus_state_detect.timer, jiffies + AML_SDIO_STATE_MON_INTERVAL);
    } else {

        printk("%s: stop bus detected state timer\n", __func__);
    }
}

void aml_bus_state_detect_init()
{
    w2_bus_state_detect.bus_err = 0;
    w2_bus_state_detect.bus_reset_ongoing = 0;
    w2_bus_state_detect.is_drv_load_finished = 0;
    w2_bus_state_detect.is_load_by_timer = 0;
    INIT_WORK(&w2_bus_state_detect.detect_work, bus_detect_work);
    timer_setup(&w2_bus_state_detect.timer, state_detect_cb, 0);
    mod_timer(&w2_bus_state_detect.timer, jiffies + AML_SDIO_STATE_MON_INTERVAL);
}
void w2_aml_bus_state_detect_deinit()
{
    del_timer_sync(&w2_bus_state_detect.timer);
    w2_bus_state_detect.bus_err = 0;
    w2_bus_state_detect.bus_reset_ongoing = 0;
    w2_bus_state_detect.is_drv_load_finished = 0;
}

int aml_bus_intf_insmod(void)
{
    int ret;
    if (aml_init_wlan_mem()) {
        printk("aml_init_wlan_mem fail\n");
        return -EPERM;
    }
    if (strncmp(w2_bus_type,"usb",3) == 0) {
        w2_aml_bus_type = USB_MODE;
        ret = w2_aml_usb_insmod();
        if (ret) {
            printk("aml usb bus init fail\n");
        }
    } else if (strncmp(w2_bus_type,"sdio",4) == 0) {
        w2_aml_bus_type = SDIO_MODE;
        ret = w2_aml_sdio_insmod();
        if (ret) {
            printk("aml sdio bus init fail\n");
#ifdef CONFIG_PT_MODE
            return ret;
#endif
        }
    } else if (strncmp(w2_bus_type,"pci",3) == 0) {
        w2_aml_bus_type = PCIE_MODE;
        ret = w2_aml_pci_insmod();
        if (ret) {
            printk("aml sdio bus init fail\n");
        }
    }
    atomic_set(&w2_g_wifi_pm.bus_suspend_cnt, 0);
    atomic_set(&w2_g_wifi_pm.drv_suspend_cnt, 0);
    atomic_set(&w2_g_wifi_pm.is_shut_down, 0);
#ifndef CONFIG_PT_MODE
    if (w2_aml_bus_type == SDIO_MODE) {
        aml_bus_state_detect_init();
    }
#endif

    return 0;
}
void aml_bus_intf_rmmod(void)
{
    if (strncmp(w2_bus_type,"usb",3) == 0) {
        w2_aml_usb_rmmod();
    } else if (strncmp(w2_bus_type,"sdio",4) == 0) {
        w2_aml_sdio_rmmod();
    } else if (strncmp(w2_bus_type,"pci",3) == 0) {
        w2_aml_pci_rmmod();
    }
#ifndef CONFIG_PT_MODE
    if (w2_aml_bus_type == SDIO_MODE) {
        w2_aml_bus_state_detect_deinit();
    }
#endif
    aml_deinit_wlan_mem();
}

lp_shutdown_func w2_g_lp_shutdown_func = NULL;

EXPORT_SYMBOL(w2_aml_bus_state_detect_deinit);
module_param(w2_bus_type, charp,S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(w2_bus_type,"A string variable to adjust pci or sdio or usb bus interface");
module_init(aml_bus_intf_insmod);
module_exit(aml_bus_intf_rmmod);

MODULE_LICENSE("GPL");

EXPORT_SYMBOL(w2_g_lp_shutdown_func);
