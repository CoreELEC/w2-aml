#ifndef _AML_INTERFACE_H_
#define _AML_INTERFACE_H_

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

struct aml_pm_type {
    atomic_t bus_suspend_cnt;
    atomic_t drv_suspend_cnt;
    atomic_t is_shut_down;
    atomic_t wifi_enable;
};

typedef void (*lp_shutdown_func)(void);
#endif
