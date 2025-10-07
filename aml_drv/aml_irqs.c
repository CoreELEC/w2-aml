/**
 ******************************************************************************
 *
 * @file aml_irqs.c
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define AML_MODULE     IRQ

#include <linux/interrupt.h>
#include "aml_defs.h"
#include "ipc_host.h"
#include "aml_prof.h"
#include "reg_ipc_app.h"
#include "aml_irqs.h"
#include "wifi_top_addr.h"
#include "aml_recy.h"

static int aml_sdio_usb_irq_task(struct aml_hw *aml_hw)
{
    u32 status;

    while ((status = aml_hw->plat->ack_irq(aml_hw))) {
        if (aml_hw->aml_irq_task_quit)
            return -1;

        /* process high priority interrupts */
        ipc_host_irq(aml_hw->ipc_env, status & ~(IPC_IRQ_E2A_RXDESC | IPC_IRQ_E2A_TXCFM));

        /* process RX interrupt */
        if ((status & IPC_IRQ_E2A_RXDESC) && aml_sdio_usb_rxdataind(&aml_hw->rx) < 0)
            break;

        /* process TX confirmation */
        if (status & IPC_IRQ_E2A_TXCFM) {
            hi_sram_read(aml_hw, aml_hw->read_cfm, SRAM_TXCFM_START_ADDR, sizeof(aml_hw->read_cfm));
            up(&aml_hw->aml_txcfm_sem);
        }
    }

    return 0;
}

#ifdef CONFIG_AML_SDIO_IRQ_VIA_GPIO

static irqreturn_t aml_irq_sdio_thread(int irq, void *dev_id)
{
    struct aml_hw *aml_hw = (struct aml_hw *)dev_id;

    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || atomic_read(&w2_g_wifi_pm.is_shut_down))
        return IRQ_HANDLED;

    if (aml_hw->irq_done) {
        aml_hw->irq_done = 0;
        AML_PROF_HI(irq_thread);
        aml_sdio_usb_irq_task(aml_hw);
        aml_enable_sdio_irq(aml_hw);
        AML_PROF_LO(irq_thread);
    }

    return IRQ_HANDLED;
}

void aml_sdio_irq_release(struct aml_hw *aml_hw)
{
    if (aml_hw->irq) {
        free_irq(aml_hw->irq, aml_hw);
        aml_hw->irq = 0;
    }
}

int aml_sdio_irq_claim(struct aml_hw *aml_hw)
{
    extern int wifi_irq_num(void);

    unsigned int irq_flag = IRQF_ONESHOT | \
                            IORESOURCE_IRQ | \
                            IORESOURCE_IRQ_LOWLEVEL | \
                            IORESOURCE_IRQ_SHAREABLE;
    int ret;

    aml_hw->irq = wifi_irq_num();
    ret = request_threaded_irq(aml_hw->irq, NULL, aml_irq_sdio_thread,
                               irq_flag, "aml_sdio", aml_hw);
    AML_INFO("request_threaded_irq(irq=%d, irq_flag=0x%x) = %d\n",
             aml_hw->irq, irq_flag, ret);
    if (ret)
        aml_hw->irq = 0;
    return ret;
}

#else

static int sdio_irq_handler_done;
static void aml_irq_sdio_hdlr(struct sdio_func *func)
{
    struct aml_hw *aml_hw = dev_get_drvdata(&func->dev);

    sdio_irq_handler_done = 0;
    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || atomic_read(&w2_g_wifi_pm.is_shut_down))
    {
        sdio_irq_handler_done = 1;
        return;
    }

    if (aml_hw->irq_done) {
        aml_hw->irq_done = 0;

        REG_SW_SET_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);
        sdio_release_host(func);

        aml_sdio_usb_irq_task(aml_hw);

        aml_enable_sdio_irq(aml_hw);

        sdio_claim_host(func);
        REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);
    }

    sdio_irq_handler_done = 1;
    return;
}

void aml_sdio_irq_release(struct aml_hw *aml_hw)
{
    struct sdio_func *func = w2_aml_priv_to_func(SDIO_FUNC1);
    int wait_cnt = 0;

    while (!sdio_irq_handler_done) {
        AML_RLMT_ERR("irq release need wait !!!\n");
        usleep_range(2, 3);
        wait_cnt++;
        if (wait_cnt > 500) {
            AML_ERR("irq release wait timeout.\n");
            break;
        }
    }

    sdio_claim_host(func);
    sdio_release_irq(func);
    sdio_release_host(func);
    AML_RLMT_INFO("irq release\n");
}

int aml_sdio_irq_claim(struct aml_hw *aml_hw)
{
    struct sdio_func *func = w2_aml_priv_to_func(SDIO_FUNC1);
    int ret;

    dev_set_drvdata(&func->dev, aml_hw);
    sdio_claim_host(func);
    ret = sdio_claim_irq(func, aml_irq_sdio_hdlr);
    sdio_release_host(func);
    AML_RLMT_INFO("irq claim\n");
    return ret;
}

#endif

void aml_enable_sdio_irq(struct aml_hw *aml_hw)
{
    aml_hw->irq_done = 1;

#ifdef CONFIG_AML_W2L_RX_MINISIZE
    /*
     * for W2, interrupt has been cleared by firmware.
     * please refer to patch_fi_irq_to_host() / register RG_WIFI_IF_FW2HST_CLR.
     */
    aml_hw->plat->hif_sdio_ops->hi_random_word_write(RG_WIFI_IF_FW2HST_CLR, BIT(31));
#endif
}

void aml_irq_usb_hdlr(struct urb *urb)
{
    extern int w2_bt_wt_ptr;
    extern int w2_bt_rd_ptr;

    struct aml_hw *aml_hw = (struct aml_hw *)(urb->context);

    AML_PROF_CNT(urb_irq, 0);
    AML_PROF_CNT(urb_status, urb->status);
    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || atomic_read(&w2_g_wifi_pm.is_shut_down))
    {
        return;
    }

    if (w2_bus_state_detect.bus_err)
        return;

    if (!aml_hw->usb) {
        AML_ERR("aml_hw->usb is NULL\n");
        return;
    }

    w2_bt_rd_ptr = __le32_to_cpu(aml_hw->usb->fw_ptrs[2]);
    w2_bt_wt_ptr = __le32_to_cpu(aml_hw->usb->fw_ptrs[3]);

    urb->status = 0;    /* FIXME: it's dangerous, usb core may still refer to this status */
    up(&aml_hw->aml_irq_sem);
}

/* FIXME: move aml_usb_irq_urb_submit() into w2_usb.c */
int aml_usb_irq_urb_submit(struct aml_hw *aml_hw)
{
    struct urb *urb;
    int ret = 0;

    if (!aml_hw->usb) {
        AML_ERR("aml_hw->usb is NULL\n");
        return -1;
    }

    urb = &aml_hw->usb->urb;

    USB_BEGIN_LOCK();
    if (urb->status != -EINPROGRESS) {
        if (urb->status)
            AML_NOTICE("need submit urb %d\n", urb->status);
        AML_PROF_CNT(urb_irq, 3);
        ret = usb_submit_urb(urb, GFP_ATOMIC);
        AML_PROF_CNT(urb_irq, ret ? 1 : 2);
    }
    USB_END_LOCK();
    if (ret < 0)
        AML_ERR("failed %d\n", ret);
    else if (urb->status && urb->status != -EINPROGRESS)
        AML_NOTICE("urb.status %d\n", urb->status);

    return ret;
}

static inline int aml_usb_irq_task(struct aml_hw *aml_hw)
{
    int try_cnt = 0;

    BUG_ON(w2_aml_bus_type != USB_MODE);

    aml_sched_rt_set(SCHED_FIFO, AML_TASK_PRI);
    while (!aml_hw->aml_irq_task_quit) {
        /* wait for work */
        if (down_interruptible(&aml_hw->aml_irq_sem) != 0) {
            /* interrupted, exit */
            AML_RLMT_ERR("wait aml_task_sem fail!\n");
            break;
        }

        REG_SW_SET_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);

        aml_sdio_usb_irq_task(aml_hw);

#ifdef CONFIG_AML_RECOVERY
        if (!w2_bus_state_detect.bus_err)
#endif
        {
            int ret = 0;

            usleep_range(aml_hw->trb_wait_time, aml_hw->trb_wait_time + 10);
            if ((atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) == 0) && (atomic_read(&w2_g_wifi_pm.is_shut_down) == 0) &&
                (atomic_read(&w2_g_wifi_pm.drv_suspend_cnt) == 0)) {
                aml_usb_irq_urb_incr(aml_hw);
                ret = aml_usb_irq_urb_submit(aml_hw);
            }
            if (ret < 0) {
                try_cnt++;
                AML_ERR("aml_usb_irq_urb_submit failed %d, bus_supend: %d, drv_suspend: %d\n",
                        ret, atomic_read(&w2_g_wifi_pm.bus_suspend_cnt), atomic_read(&w2_g_wifi_pm.drv_suspend_cnt));
                if (try_cnt < 5) {
                    if ((atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) == 0) && (atomic_read(&w2_g_wifi_pm.is_shut_down) == 0) &&
                        (atomic_read(&w2_g_wifi_pm.drv_suspend_cnt) == 0))
                        up(&aml_hw->aml_irq_sem);
                } else {
#ifdef CONFIG_AML_RECOVERY
                    if ((atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) == 0) && (atomic_read(&w2_g_wifi_pm.is_shut_down) == 0) &&
                        (atomic_read(&w2_g_wifi_pm.drv_suspend_cnt) == 0))
                        w2_aml_usb_set_bus_err(1);
#endif
                    AML_ERR("aml_usb_irq_urb_submit failed(%d), try cnt %d\n", ret, try_cnt);
                }
            } else {
                try_cnt = 0;
            }
        }

        REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);
    }
    if (aml_hw->aml_irq_completion_init) {
        aml_hw->aml_irq_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
        complete_and_exit(&aml_hw->aml_irq_completion, 0);
#else
        complete(&aml_hw->aml_irq_completion);
#endif
    }

    return 0;
}

int aml_irq_task(void *data)
{
    return aml_usb_irq_task((struct aml_hw *)data);
}

/**
 * aml_irq_hdlr - IRQ handler
 *
 * Handler registerd by the platform driver
 */
irqreturn_t aml_irq_pcie_hdlr(int irq, void *dev_id)
{
    struct aml_hw *aml_hw = (struct aml_hw *)dev_id;

    if (atomic_read(&w2_g_wifi_pm.bus_suspend_cnt) || atomic_read(&w2_g_wifi_pm.is_shut_down))
    {
        return IRQ_HANDLED;
    }
    disable_irq_nosync(irq);

#ifdef CONFIG_AML_USE_TASK
    aml_task_schedule(&aml_hw->pcie.task_irqhdlr);
#else
    tasklet_schedule(&aml_hw->task);
#endif
    return IRQ_HANDLED;
}
