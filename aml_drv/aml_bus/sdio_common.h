#ifndef SDIO_COMMON_H
#define SDIO_COMMON_H

#include <linux/version.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>    /* udelay */
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/moduleparam.h>
#include <linux/irqreturn.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h> /* printk() */
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/gpio.h> //mach
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/firmware.h>
#include "wifi_sdio_cfg_addr.h"


#define AML_BT_WIFI_MUTEX_ON() do {\
                    mutex_lock(&wifi_bt_sdio_mutex);\
                } while (0)

#define AML_BT_WIFI_MUTEX_OFF() do {\
                    mutex_unlock(&wifi_bt_sdio_mutex);\
                } while (0)

#define AML_WIFI_IPC_MUTEX_ON() do {\
                    mutex_lock(&wifi_ipc_mutex);\
                } while (0)

#define AML_WIFI_IPC_MUTEX_OFF() do {\
                    mutex_unlock(&wifi_ipc_mutex);\
                } while (0)


#ifndef ASSERT
#define ASSERT(exp) do{    \
                if (!(exp)) {   \
                        printk("=>=>=>=>=>assert %s,%d\n",__func__,__LINE__);   \
                        /*BUG();        while(1);   */  \
                }                       \
        } while (0);
#endif

#define ERROR_DEBUG_OUT(format,...) do {    \
                                  printk("FUNCTION: %s LINE: %d:"format"",__FUNCTION__, __LINE__, ##__VA_ARGS__); \
                         } while (0)


#define OS_LOCK spinlock_t

#define SDIO_ADDR_MASK (128 * 1024 - 1)
#define SDIO_OPMODE_INCREMENT 1
#define SDIO_OPMODE_FIXED 0

#define SDIO_WRITE 1
#define SDIO_READ 0

#define SDIOH_API_RC_SUCCESS (0x00)
#define SDIOH_API_RC_FAIL (0x01)

#define FUNCNUM_SDIO_LAST SDIO_FUNC7
#define SDIO_FUNCNUM_MAX (FUNCNUM_SDIO_LAST+1)

#define WIFI_SDIO_IF    (0xa05000)
                 /* APB domain, checksum error status, checksum enable, frame flag bypass*/
#define RG_SDIO_IF_MISC_CTRL (WIFI_SDIO_IF+0x80)
#define RG_SDIO_IF_MISC_CTRL2 (WIFI_SDIO_IF+0x84)


#define ZMALLOC(size, name, gfp) kzalloc(size, gfp)
#define FREE(a, name) kfree(a)
#define LEN_128K (128 * 1024)
#define LEN_256K (256 * 1024)


/*sdio max block count when we use scatter/gather list.*/

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef unsigned long SYS_TYPE;

enum SDIO_STD_FUNNUM {
    SDIO_FUNC0=0,
    SDIO_FUNC1,
    SDIO_FUNC2,
    SDIO_FUNC3,
    SDIO_FUNC4,
    SDIO_FUNC5,
    SDIO_FUNC6,
    SDIO_FUNC7,
};

extern struct aml_hwif_sdio g_hwif_sdio;

struct aml_hwif_sdio {
    struct sdio_func * sdio_func_if[SDIO_FUNCNUM_MAX];
    bool scatter_enabled;

    /* protects access to scat_req */
    OS_LOCK scat_lock;

    /* scatter request list head */
    struct amlw_hif_scatter_req *scat_req;
};

extern unsigned char g_sdio_driver_insmoded;
extern struct sdio_func *aml_priv_to_func(int func_n);

int aml_sdio_init(void);
void aml_sdio_calibration(void);
extern void sdio_reinit(void);
extern void amlwifi_set_sdio_host_clk(int clk);
extern void set_usb_bt_power(int is_on);
struct sdio_func *aml_priv_to_func(int func_n);
int aml_sdio_pm_suspend(struct device *device);
int aml_sdio_pm_resume(struct device *device);
bool aml_sdio_block_bus_opt(void);

#endif
