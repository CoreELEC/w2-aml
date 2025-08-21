/**
****************************************************************************************
*
* @file aml_task.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief task API implementation.
*
****************************************************************************************
*/

#include "aml_task.h"
#include "aml_wq.h"
#include "aml_scc.h"
#include "ipc_host.h"
#include "aml_msg_tx.h"

#ifdef CONFIG_AML_USE_TASK

/* use set_user_nice(x, nice) priority to alleviate
 * preempt cpu loading:
 *  PR  = 20 + nice
 * where nice range is -20 to 19
 * PR = 0 to 39 which is same as 100 to 139.
 * */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
#define AML_TASK_FUNC(data, name, func) do { \
    struct aml_hw *aml_hw = (struct aml_hw *)data; \
    set_user_nice(current, 5); \
    while (!aml_hw->name->task_quit) { \
        if (down_interruptible(&aml_hw->name->task_sem) != 0) break; \
        if (aml_hw->name->task_quit) break; \
        func(aml_hw); \
    } \
    complete_and_exit(&aml_hw->name->task_cmpl, 0); \
} while (0);
#else
#define AML_TASK_FUNC(data, name, func) do { \
    struct aml_hw *aml_hw = (struct aml_hw *)data; \
    set_user_nice(current, 5); \
    while (!aml_hw->name->task_quit) { \
        if (down_interruptible(&aml_hw->name->task_sem) != 0) break; \
        if (aml_hw->name->task_quit) break; \
        func(aml_hw); \
    } \
    complete(&aml_hw->name->task_cmpl); \
} while (0);
#endif

#define AML_TASK_INIT(aml_hw, name)  do { \
    aml_hw->name = kmalloc(sizeof(struct aml_task), GFP_KERNEL); \
    spin_lock_init(&aml_hw->name->lock); \
    sema_init(&aml_hw->name->task_sem, 0); \
    aml_hw->name->task_quit = 0; \
    aml_hw->name->task = kthread_run(aml_task_##name, aml_hw, "aml_task_"#name); \
    if (IS_ERR(aml_hw->name->task)) { kfree(aml_hw->name); } \
} while (0);

#define AML_TASK_DEINIT(aml_hw, name) do { \
    init_completion(&aml_hw->name->task_cmpl); \
    aml_hw->name->task_quit = 1; \
    up(&aml_hw->name->task_sem); \
    kthread_stop(aml_hw->name->task); \
    wait_for_completion(&aml_hw->name->task_cmpl); \
    kfree(aml_hw->name); \
} while (0);


int aml_task_irqhdlr(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;
    struct sched_param param = {0};
    u32 status;

    param.sched_priority = AML_TASK_PRI;
#ifndef CONFIG_PT_MODE
    sched_setscheduler(current, SCHED_FIFO, &param);
#endif
    while (!aml_hw->irqhdlr->task_quit) {
        if (down_interruptible(&aml_hw->irqhdlr->task_sem) != 0) {
            enable_irq(aml_platform_get_irq(aml_hw->plat));
            break;
        }
        if (aml_hw->irqhdlr->task_quit)
            break;
        while ((status = ipc_host_get_status(aml_hw->ipc_env))) {
            ipc_host_irq_ext(aml_hw->ipc_env, status);
        }
        aml_spin_lock(&aml_hw->tx_lock);
        aml_hwq_process_all(aml_hw);
        aml_spin_unlock(&aml_hw->tx_lock);
        enable_irq(aml_platform_get_irq(aml_hw->plat));
        aml_hw->plat->ack_irq(aml_hw);
    }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
    complete_and_exit(&aml_hw->irqhdlr->task_cmpl, 0);
#else
    complete(&aml_hw->irqhdlr->task_cmpl);
#endif
    return 0;
}

void aml_rxdesc_hdlr(struct aml_hw *aml_hw)
{
    ipc_host_rxdesc_handler(aml_hw->ipc_env);
}

int aml_task_rxdesc(void *data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;

    set_user_nice(current, 5);
    while (!aml_hw->rxdesc->task_quit) {
        if (down_interruptible(&aml_hw->rxdesc->task_sem) != 0)
            break;
        if (aml_hw->rxdesc->task_quit)
            break;
        aml_rxdesc_hdlr(aml_hw);
    }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
    complete_and_exit(&aml_hw->rxdesc->task_cmpl, 0);
#else
    complete(&aml_hw->rxdesc->task_cmpl);
#endif
    return 0;
}

void aml_task_init(struct aml_hw *aml_hw)
{
    if (aml_bus_type != PCIE_MODE)
        return;

    AML_INFO("aml task init");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
    AML_TASK_INIT(aml_hw, irqhdlr);
#else
#endif
    AML_TASK_INIT(aml_hw, rxdesc);
}

void aml_task_deinit(struct aml_hw *aml_hw)
{
    if (aml_bus_type != PCIE_MODE)
        return;

    AML_INFO("aml task deinit");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) // template solution for S905L3A
    AML_TASK_DEINIT(aml_hw, irqhdlr);
#else
#endif
    AML_TASK_DEINIT(aml_hw, rxdesc);
}
#endif
