
#define AML_MODULE  COMMON

#include <linux/mutex.h>
#include "chip_ana_reg.h"
#include "chip_pmu_reg.h"
#include "chip_intf_reg.h"
#include "wifi_intf_addr.h"
#include "wifi_top_addr.h"
#include "wifi_sdio_cfg_addr.h"
#include "wifi_w2_shared_mem_cfg.h"
#include "sdio_common.h"
#include "sg_common.h"
#include "aml_interface.h"
#include "w2_sdio.h"
#include "aml_log.h"

#ifdef CONFIG_PT_MODE
unsigned char g_sdio_is_probe = 0;
#endif
struct aml_hwif_sdio g_hwif_sdio;
unsigned char g_sdio_wifi_bt_alive;
unsigned char g_sdio_driver_insmoded;
unsigned char g_sdio_after_porbe;
unsigned char g_wifi_in_insmod;
extern unsigned int chip_id;
unsigned char *g_func_kmalloc_buf = NULL;
unsigned char wifi_irq_enable = 0;
unsigned int  shutdown_i = 0;
unsigned char wifi_sdio_shutdown = 0;
unsigned char wifi_in_insmod;
unsigned char wifi_in_rmmod;
unsigned char  chip_en_access;
extern unsigned char wifi_sdio_shutdown;
extern unsigned char wifi_drv_rmmod_ongoing;
extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_pm_type g_wifi_pm;

static DEFINE_MUTEX(wifi_bt_sdio_mutex);
static DEFINE_MUTEX(wifi_ipc_mutex);

unsigned char (*host_wake_req)(void);
int (*host_suspend_req)(struct device *device);
int (*host_resume_req)(struct device *device);
extern void extern_wifi_set_enable(int is_on);
extern void aml_sdio_random_word_write(unsigned int addr, unsigned int data);
extern unsigned int aml_sdio_random_word_read(unsigned int addr);

struct sdio_func *aml_priv_to_func(int func_n)
{
    ASSERT(func_n >= 0 &&  func_n < SDIO_FUNCNUM_MAX);
    return g_hwif_sdio.sdio_func_if[func_n];
}

int aml_sdio_suspend(unsigned int suspend_enable)
{
    mmc_pm_flag_t flags;
    struct sdio_func *func = NULL;
    int ret = 0, i;

    if (suspend_enable == 0)
    {
        /* when enable == 0 that's resume operation
        and we do nothing for resume now. */
        return ret;
    }

    /* just clear sdio clock value for emmc init when resume */
    //amlwifi_set_sdio_host_clk(0);

    AML_BT_WIFI_MUTEX_ON();
    /* we shall suspend all card for sdio. */
    for (i = SDIO_FUNC1; i <= FUNCNUM_SDIO_LAST; i++)
    {
        func = aml_priv_to_func(i);
        if (func == NULL)
            continue;
        flags = sdio_get_host_pm_caps(func);

        if ((flags & MMC_PM_KEEP_POWER) != 0)
            ret = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);

        if (ret != 0) {
            AML_BT_WIFI_MUTEX_OFF();
            return -1;
        }

        /*
         * if we don't use sdio irq, we can't get functions' capability with
         * MMC_PM_WAKE_SDIO_IRQ, so we don't need set irq for wake up
         * sdio for upcoming suspend.
         */
        if ((flags & MMC_PM_WAKE_SDIO_IRQ) != 0)
            ret = sdio_set_host_pm_flags(func, MMC_PM_WAKE_SDIO_IRQ);

        if (ret != 0) {
            AML_BT_WIFI_MUTEX_OFF();
            return -1;
        }
    }

    AML_BT_WIFI_MUTEX_OFF();
    return ret;
}

int _aml_sdio_request_buffer(unsigned char func_num,
    unsigned int fix_incr, unsigned char write, unsigned int addr, void *buf, unsigned int nbytes)
{
    int err_ret = 0;
    int align_nbytes = nbytes;
    struct sdio_func * func = aml_priv_to_func(func_num);
    bool fifo = (fix_incr == SDIO_OPMODE_FIXED);

    if (!func) {
        AML_ERR("func is NULL!\n");
        return -1;
    }

    ASSERT(fix_incr == SDIO_OPMODE_FIXED|| fix_incr == SDIO_OPMODE_INCREMENT);
    ASSERT(func->num == func_num);

    /* Claim host controller */
    sdio_claim_host(func);

    if (write && !fifo)
    {
        /* write, increment */
        align_nbytes = sdio_align_size(func, nbytes);
        err_ret = sdio_memcpy_toio(func, addr, buf, align_nbytes);
    }
    else if (write)
    {
        /* write, fifo */
        err_ret = sdio_writesb(func, addr, buf, align_nbytes);
    }
    else if (fifo)
    {
        /* read */
        err_ret = sdio_readsb(func, buf, addr, align_nbytes);
    }
    else
    {
        /* read */
        align_nbytes = sdio_align_size(func, nbytes);
        err_ret = sdio_memcpy_fromio(func, buf, addr, align_nbytes);
    }

    /* Release host controller */
    sdio_release_host(func);

    return (err_ret == 0) ? SDIOH_API_RC_SUCCESS : SDIOH_API_RC_FAIL;
}


void aml_sdio_init_ops(void)
{
    aml_sdio_init_w2_ops();
    return;
}

#ifdef CONFIG_PT_MODE
void *g_drv_data = NULL;
#endif

int aml_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    int ret = 0;
    static struct sdio_func sdio_func_0;

    sdio_claim_host(func);
    ret = sdio_enable_func(func);
    if (ret)
        goto sdio_enable_error;

    if (func->num == 4)
        sdio_set_block_size(func, 512);
    else
        sdio_set_block_size(func, 512);

    AML_DBG("func->num %d sdio block size=%d, \n",
        func->num,  func->cur_blksize);

    if (func->num == 1)
    {
        sdio_func_0.num = 0;
        sdio_func_0.card = func->card;
        g_hwif_sdio.sdio_func_if[0] = &sdio_func_0;
    }
    g_hwif_sdio.sdio_func_if[func->num] = func;
    AML_DBG(" func->num %d sdio_func=%p, \n", func->num,  func);

    sdio_release_host(func);
    sdio_set_drvdata(func, (void *)(&g_hwif_sdio));
    if (func->num != FUNCNUM_SDIO_LAST)
    {
        AML_DBG("func_num=%d, last func num=%d\n",
            func->num, FUNCNUM_SDIO_LAST);
        return 0;
    }
    AML_INFO(" sdio probe success\n");
    aml_sdio_init_base_addr();
    aml_sdio_init_ops();
    g_hif_sdio_ops.hi_enable_scat(&g_hwif_sdio);
   // g_hif_sdio_ops.hi_enable_scat(&g_hwif_rx_sdio);

#ifdef CONFIG_PT_MODE
    dev_set_drvdata(&func->dev, g_drv_data);
    g_sdio_is_probe = 1;
#endif

    return ret;

sdio_enable_error:
    AML_ERR("sdio_enable_error:  line \n");
    sdio_release_host(func);

    return ret;
}

static void  aml_sdio_remove(struct sdio_func *func)
{
    if (func== NULL)
    {
        return ;
    }

    AML_INFO("aml_sdio_remove++ func->num =%d \n",func->num);

    sdio_claim_host(func);
    sdio_disable_func(func);
    sdio_release_host(func);
#ifdef CONFIG_PT_MODE
    g_drv_data = dev_get_drvdata(&func->dev);
#endif

    host_wake_req = NULL;
    host_suspend_req = NULL;
    host_resume_req = NULL;
}

 int aml_sdio_pm_suspend(struct device *device)
{
    int ret = 0;
    int cnt = 0;

    if (atomic_read(&g_wifi_pm.wifi_enable))
    {
        while (atomic_read(&g_wifi_pm.drv_suspend_cnt) == 0)
        {
            msleep(50);
            cnt++;
            if (cnt > 40)
            {
                AML_ERR("wifi suspend fail \n");
                return -1;
            }
        }
    }

    if (host_suspend_req != NULL)
        ret = host_suspend_req(device);
    else
        ret = aml_sdio_suspend(1);
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 1);
    return ret;
}

 int aml_sdio_pm_resume(struct device *device)
{
    int ret = 0;

    if (host_resume_req != NULL)
        ret = host_resume_req(device);
    atomic_set(&g_wifi_pm.bus_suspend_cnt, 0);

    return ret;
}

extern lp_shutdown_func g_lp_shutdown_func;
extern bt_shutdown_func g_bt_shutdown_func;

//The shutdown interface will be called 7 times by the driver, and msg only needs to send once
int g_sdio_shutdown_cnt = 0;
void aml_sdio_shutdown(struct device *device)
{
    //sdio the shutdown interface will be called 7 times by the driver, and msg only needs to send once
    if (g_sdio_shutdown_cnt++)
    {
        return;
    }

    AML_INFO("aml_sdio_shutdown begin \n");

    //Mask interrupt reporting to the host
    atomic_set(&g_wifi_pm.is_shut_down, 2);

    // Notify fw to enter shutdown mode
    if (g_bt_shutdown_func != NULL)
    {
        g_bt_shutdown_func();
    }

    //send msg only once
    if (g_lp_shutdown_func != NULL)
    {
        g_lp_shutdown_func();
    }

    //notify fw shutdown
    //notify bt wifi will go shutdown
    aml_sdio_random_word_write(RG_AON_A16, aml_sdio_random_word_read(RG_AON_A16) | BIT(28));

    //prevrnt msg_send & reg read_write
    atomic_set(&g_wifi_pm.is_shut_down, 1);
}

bool aml_sdio_block_bus_opt(void)
{
    if (atomic_read(&g_wifi_pm.is_shut_down) == 1)
    {
       ERROR_DEBUG_OUT("fw shut down(%d) , do not read/write now!\n",
       atomic_read(&g_wifi_pm.is_shut_down));
       return true;
    }
    else
    {
       return false;
    }
}
static SIMPLE_DEV_PM_OPS(aml_sdio_pm_ops, aml_sdio_pm_suspend,
                     aml_sdio_pm_resume);

static const struct sdio_device_id aml_sdio[] =
{
    {SDIO_DEVICE(W2_VENDOR_AMLOGIC,W2_PRODUCT_AMLOGIC) },
    {SDIO_DEVICE(W2_VENDOR_AMLOGIC_EFUSE,W2_PRODUCT_AMLOGIC_EFUSE)},
    {SDIO_DEVICE(W2s_VENDOR_AMLOGIC_EFUSE,W2s_A_PRODUCT_AMLOGIC_EFUSE)},
    {SDIO_DEVICE(W2s_VENDOR_AMLOGIC_EFUSE,W2s_B_PRODUCT_AMLOGIC_EFUSE)},
    {}
};

static struct sdio_driver aml_sdio_driver =
{
    .name = "aml_sdio",
    .id_table = aml_sdio,
    .probe = aml_sdio_probe,
    .remove = aml_sdio_remove,
    .drv.pm = &aml_sdio_pm_ops,
    .drv.shutdown = aml_sdio_shutdown,
};

int  aml_sdio_init(void)
{
    int err = 0;

    //amlwifi_set_sdio_host_clk(200000000);//200MHZ

    err = sdio_register_driver(&aml_sdio_driver);
    g_sdio_driver_insmoded = 1;
    g_wifi_in_insmod = 0;

    wifi_in_insmod = 0;
    wifi_in_rmmod = 0;
    chip_en_access = 0;
    wifi_sdio_shutdown = 0;
    AML_INFO("*****************aml sdio common driver is insmoded********************\n");
    if (err)
        AML_ERR("failed to register sdio driver: %d \n", err);

    return err;
}

void  aml_sdio_exit(void)
{
    AML_INFO("aml_sdio_exit++ \n");
    sdio_unregister_driver(&aml_sdio_driver);
    g_sdio_driver_insmoded = 0;
    g_sdio_after_porbe = 0;

    AML_INFO("*****************aml sdio common driver is rmmoded********************\n");
}

void aml_sdio_reset(void)
{
#ifndef CONFIG_PT_MODE
    int reg = 0;
    int try_count = 0;

    AML_INFO("******* sdio reset begin *******\n");
Try_again:
#ifndef CONFIG_PT_MODE
#ifndef CONFIG_LINUXPC_VERSION
    extern_wifi_set_enable(0);
#endif
#endif
    aml_sdio_exit();
    while (g_sdio_driver_insmoded == 1) {
        msleep(5);
    }
#ifndef CONFIG_PT_MODE
#ifndef CONFIG_LINUXPC_VERSION
    extern_wifi_set_enable(1);
    msleep(100);
    sdio_reinit();
#endif
#endif
    aml_sdio_init();
    while (g_sdio_driver_insmoded == 0) {
        msleep(5);
    }
    if (bus_state_detect.is_drv_load_finished) {
        bus_state_detect.bus_err = 0;
        reg = g_hif_sdio_ops.hi_random_word_read(0xf0101c);
        if ((bus_state_detect.bus_err) && try_count <= 3) {
            try_count++;
            AML_ERR(" *******sdio reset failed, try again(%d)", try_count);
            goto Try_again;
        }
        bus_state_detect.bus_reset_ongoing = 0;
    }

    AML_INFO(" ******* sdio reset end *******\n");
    return;
#endif
}

/*set_wifi_bt_sdio_driver_bit() is used to determine whether to unregister sdio power driver.
  *Only when g_sdio_wifi_bt_alive is 0, then call aml_sdio_exit().
*/
void set_wifi_bt_sdio_driver_bit(bool is_register, int shift)
{
    AML_BT_WIFI_MUTEX_ON();
    if (is_register) {
        g_sdio_wifi_bt_alive |= (1 << shift);
        AML_INFO("Insmod %s sdio driver!\n", (shift ? "WiFi":"BT"));
    } else {
        AML_INFO("Rmmod %s sdio driver!\n", (shift ? "WiFi":"BT"));
        g_sdio_wifi_bt_alive &= ~(1 << shift);
        if (!g_sdio_wifi_bt_alive) {
            aml_sdio_exit();
        }
    }
    AML_BT_WIFI_MUTEX_OFF();
}


int aml_sdio_insmod(void)
{
    aml_sdio_init();

#ifdef CONFIG_PT_MODE
    if (!g_sdio_is_probe) {
        aml_sdio_exit();
        AML_ERR(" err found! g_sdio_is_probe: %d\n", g_sdio_is_probe);
        return -1;
    }
#endif

    AML_INFO(" start...\n");
    return 0;
}

void aml_sdio_rmmod(void)
{
    aml_sdio_exit();
    g_hif_sdio_ops.hi_cleanup_scat(&g_hwif_sdio);
    wifi_drv_rmmod_ongoing = 0;
}

EXPORT_SYMBOL(aml_sdio_reset);
EXPORT_SYMBOL(wifi_irq_enable);
EXPORT_SYMBOL(aml_sdio_insmod);
EXPORT_SYMBOL(aml_sdio_rmmod);
EXPORT_SYMBOL(set_wifi_bt_sdio_driver_bit);
EXPORT_SYMBOL(g_hwif_sdio);
EXPORT_SYMBOL(aml_sdio_exit);
EXPORT_SYMBOL(aml_sdio_init);
EXPORT_SYMBOL(g_sdio_driver_insmoded);
EXPORT_SYMBOL(g_wifi_in_insmod);
EXPORT_SYMBOL(g_sdio_after_porbe);
EXPORT_SYMBOL(host_wake_req);
EXPORT_SYMBOL(host_suspend_req);
EXPORT_SYMBOL(host_resume_req);
EXPORT_SYMBOL(aml_priv_to_func);

