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

extern unsigned int g_aml_device_id;
typedef void (*bt_shutdown_func)(void);
typedef void (*lp_shutdown_func)(void);
typedef void (*bt_pm_func)(void);
void aml_wifi_power_on(int on);

void aml_bus_state_detect_deinit(void);

#endif
