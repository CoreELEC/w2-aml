/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#define AML_MODULE                  INTERFACE

#include <linux/init.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#ifdef CONFIG_AML_PLATFORM_ANDROID
#include <linux/amlogic/wifi_dt.h>  /* for extern_wifi_set_enable() */
#endif

#include "aml_static_buf.h"
#include "aml_interface.h"
#include "usb_common.h"
#include "aml_compat.h"
#include "aml_log.h"

char *bus_type = "pci";
unsigned int aml_bus_type;
unsigned char wifi_drv_rmmod_ongoing = 0;
struct aml_bus_state_detect bus_state_detect;
struct aml_pm_type g_wifi_pm = {0};
unsigned char aml_wifi_detect_bt_status = 0;
unsigned int g_aml_device_id;

const char *aml_log_level_names[] = {
#define AML_LOG_LEVEL_NAME(_level)  [LOGLEVEL_##_level] = #_level
    AML_LOG_LEVEL_NAME(EMERG),
    AML_LOG_LEVEL_NAME(ALERT),
    AML_LOG_LEVEL_NAME(CRIT),
    AML_LOG_LEVEL_NAME(ERR),
    AML_LOG_LEVEL_NAME(WARNING),
    AML_LOG_LEVEL_NAME(NOTICE),
    AML_LOG_LEVEL_NAME(INFO),
    AML_LOG_LEVEL_NAME(DEBUG),
#undef AML_LOG_LEVEL_NAME
    NULL,
};
EXPORT_SYMBOL(aml_log_level_names);

const char *aml_log_module_names[] = {
#define AML_LOG_MODULE(_m, _level)  [AML_LOG_MODULE_##_m] = #_m,
    AML_LOG_MODULES
#undef AML_LOG_MODULE
    NULL,
};
EXPORT_SYMBOL(aml_log_module_names);

s8 aml_log_m_levels[AML_LOG_MODULE_MAX] = {
#define AML_LOG_MODULE(_m, _level)  [AML_LOG_MODULE_##_m] = LOGLEVEL_##_level,
    AML_LOG_MODULES
#undef AML_LOG_MODULE
};
EXPORT_SYMBOL(aml_log_m_levels);

int aml_name_index(const char *names[], const char *name)
{
    int i;

    if (!names || !name)
        return -1;

    for (i = 0; names[i]; i++) {
        if (strcasecmp(name, names[i]) == 0)
            return i;
    }
    return -1;
}
EXPORT_SYMBOL(aml_name_index);

void aml_wifi_power_on(int on)
{
#ifdef CONFIG_AML_PLATFORM_ANDROID
    extern_wifi_set_enable(on);
#endif
}
EXPORT_SYMBOL(aml_wifi_power_on);

EXPORT_SYMBOL(bus_state_detect);
EXPORT_SYMBOL(wifi_drv_rmmod_ongoing);
EXPORT_SYMBOL(bus_type);
EXPORT_SYMBOL(aml_bus_type);
EXPORT_SYMBOL(g_wifi_pm);
EXPORT_SYMBOL(aml_wifi_detect_bt_status);

extern int aml_sdio_insmod(void);
extern int aml_sdio_rmmod(void);
extern int aml_pci_insmod(void);
extern int aml_pci_rmmod(void);
extern void aml_sdio_reset(void);

void bus_detect_work(struct work_struct *p_work)
{
    AML_FN_ENTRY();
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
        AML_ERR("stop bus detected state timer\n");
    }
}

void aml_bus_state_detect_init(void)
{
    bus_state_detect.bus_err = 0;
    bus_state_detect.bus_reset_ongoing = 0;
    bus_state_detect.is_drv_load_finished = 0;
    bus_state_detect.is_load_by_timer = 0;
    INIT_WORK(&bus_state_detect.detect_work, bus_detect_work);
    timer_setup(&bus_state_detect.timer, state_detect_cb, 0);
    mod_timer(&bus_state_detect.timer, jiffies + AML_SDIO_STATE_MON_INTERVAL);
}

void aml_bus_state_detect_deinit(void)
{
    del_timer_sync(&bus_state_detect.timer);
    bus_state_detect.bus_err = 0;
    bus_state_detect.bus_reset_ongoing = 0;
    bus_state_detect.is_drv_load_finished = 0;
}
EXPORT_SYMBOL(aml_bus_state_detect_deinit);

int aml_bus_intf_insmod(void)
{
    int ret;

    AML_NOTICE("CONFIG_AML_LOG_BUILD_LEVEL=%s\n", aml_log_level_names[CONFIG_AML_LOG_BUILD_LEVEL]);

    if (aml_init_wlan_mem()) {
        AML_ERR("aml_init_wlan_mem fail\n");
        return -EPERM;
    }
    if (strncmp(bus_type,"usb",3) == 0) {
        aml_bus_type = USB_MODE;
        ret = aml_usb_insmod();
        if (ret) {
            AML_ERR("aml usb bus init fail\n");
            goto err;
        }
    } else if (strncmp(bus_type,"sdio",4) == 0) {
        aml_bus_type = SDIO_MODE;
        ret = aml_sdio_insmod();
        if (ret) {
            AML_ERR("aml sdio bus init fail\n");
            goto err;
        }
    } else if (strncmp(bus_type,"pci",3) == 0) {
        aml_bus_type = PCIE_MODE;
        ret = aml_pci_insmod();
        if (ret) {
            AML_ERR("aml pcie bus init fail\n");
            goto err;
        }
    }
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.drv_suspend_cnt, 0);
    atomic_set(&g_wifi_pm.is_shut_down, 0);
    atomic_set(&g_wifi_pm.wifi_suspend_state, 0);

#ifndef CONFIG_PT_MODE
    if (aml_bus_type == SDIO_MODE) {
        aml_bus_state_detect_init();
    }
#endif

    return 0;
err:
    aml_deinit_wlan_mem();
    return ret;
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

bt_shutdown_func g_bt_shutdown_func = NULL;
lp_shutdown_func g_lp_shutdown_func = NULL;
bt_pm_func g_bt_suspend_func = NULL;
bt_pm_func g_bt_resume_func = NULL;

module_param(bus_type, charp,S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(bus_type,"A string variable to adjust pci or sdio or usb bus interface");
module_init(aml_bus_intf_insmod);
module_exit(aml_bus_intf_rmmod);

MODULE_LICENSE("GPL");

EXPORT_SYMBOL(g_bt_shutdown_func);
EXPORT_SYMBOL(g_lp_shutdown_func);
EXPORT_SYMBOL(g_bt_suspend_func);
EXPORT_SYMBOL(g_bt_resume_func);
EXPORT_SYMBOL(g_aml_device_id);
