/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _AML_INTERFACE_H_
#define _AML_INTERFACE_H_

#include <linux/version.h>
#include <linux/atomic.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

/* for sched_clock() */
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/clock.h>
#endif

// SDIO PID/VID
#define W2s_PRODUCT_AMLOGIC  0x8888
#define W2s_VENDOR_AMLOGIC  0x8888
#define W2s_C_PRODUCT_AMLOGIC  0x8881
#define W2s_C_VENDOR_AMLOGIC  0x8881

//sdio manufacturer code, usually vendor ID, 'a'=0x61, 'm'=0x6d
#define W2_VENDOR_AMLOGIC_EFUSE ('a'|('m'<<8))
//sdio manufacturer info, usually product ID
#define W2_PRODUCT_AMLOGIC_EFUSE (0x9007)

#define W2s_VENDOR_AMLOGIC_EFUSE 0x1B8E
#define W2s_A_PRODUCT_AMLOGIC_EFUSE 0x0600
#define W2s_B_PRODUCT_AMLOGIC_EFUSE 0x0640
#define W2s_C_PRODUCT_AMLOGIC_EFUSE 0x0680

// USB PID/VID
#define W2u_VENDOR_AMLOGIC  0x414D
#define W2u_PRODUCT_AMLOGIC  0x4c55

#define W2u_VENDOR_AMLOGIC_EFUSE 0x1B8E
#define W2u_A_PRODUCT_AMLOGIC_EFUSE 0x0601
#define W2u_B_PRODUCT_AMLOGIC_EFUSE 0x0641
#define W2u_C_PRODUCT_AMLOGIC_EFUSE 0x0681

// PCIE PID/VID
#define W2p_VENDOR_AMLOGIC_EFUSE 0x1F35
#define W2p_A_PRODUCT_AMLOGIC_EFUSE 0x0602
#define W2p_B_PRODUCT_AMLOGIC_EFUSE 0x0642
#define W2p_C_PRODUCT_AMLOGIC_EFUSE 0x0682


#define AML_SDIO_STATE_MON_INTERVAL   (5 *HZ)
enum interface_type {
    SDIO_MODE,
    USB_MODE,
    PCIE_MODE
};

struct aml_bus_state_detect {
  unsigned char bus_err;
  unsigned char is_drv_load_finished;
  unsigned char bus_reset_ongoing;
  unsigned char is_load_by_timer;
  unsigned char is_recy_ongoing;
  struct timer_list timer;
  struct work_struct detect_work;
  int (*insmod_drv)(void);

#ifdef CONFIG_AML_USB_HOTPLUG
  unsigned char usb_unplug;
  void (*auc_wifi_enable_func)(void);
  void (*auc_wifi_disable_func)(void);
#endif
};

extern struct aml_bus_state_detect bus_state_detect;

struct aml_pm_type {
    atomic_t bus_suspend_cnt;
    atomic_t drv_suspend_cnt;
    atomic_t is_shut_down;
    atomic_t wifi_enable;
    atomic_t wifi_suspend_state;
};

extern struct aml_pm_type g_wifi_pm;
extern struct wakeup_source *aml_wifi_wakeup_source;

extern unsigned int g_aml_device_id;
typedef void (*bt_shutdown_func)(void);
typedef void (*lp_shutdown_func)(void);
typedef void (*bt_pm_func)(void);
void aml_wifi_power_on(int on);

void aml_bus_state_detect_deinit(void);

#endif
