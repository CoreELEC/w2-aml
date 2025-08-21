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

#define AML_MODULE          GENERIC

#include <linux/kthread.h>

#include "aml_task.h"

#include "aml_compat.h"
#include "aml_log.h"

static inline int aml_task_threadfn(void *data)
{
    struct aml_task *task = data;
    int ret = -EINTR;

    /* FIXME: set_user_nice(task->task, 5); */
    aml_sched_rt_set(SCHED_FIFO, AML_TASK_PRI);

    while (1) {
#ifdef AML_TASK_SCH_BY_SEM
        if (down_interruptible(&task->sem) != 0)
#else
        if (wait_for_completion_interruptible(&task->completion))
#endif
        {
            AML_WARN("terminating task \"%s\"!\n", task->task->comm);
            break;
        }

        ret = task->fn(task);

        if (task->quit)
            break;
    }

    while (!kthread_should_stop()) {
        msleep(10);
    }

    return ret;
}

int aml_task_init(struct aml_task *task, int (*fn)(struct aml_task *t), const char *name, int cpu)
{
    struct task_struct *t;

    BUG_ON(!task);
    BUG_ON(!fn);
    BUG_ON(task->task);

#ifdef AML_TASK_SCH_BY_SEM
    sema_init(&task->sem, 0);
#else
    init_completion(&task->completion);
#endif
    task->quit = 0;
    task->fn = fn;

    if (cpu >= 0)
        t = kthread_create(aml_task_threadfn, task, "%s@%u", name, cpu);
    else
        t = kthread_create(aml_task_threadfn, task, "%s", name);
    if (IS_ERR_OR_NULL(t)) {
        AML_ERR("create %s on cpu %d error %ld!!!\n", name, cpu, PTR_ERR(t));
        return t ? IS_ERR(t) : -ENOMEM;
    }

    task->task = t;

    if (cpu >= 0)
        kthread_bind(t, cpu);

    wake_up_process(t);

    AML_INFO("\"%s\" is created.\n", task->task->comm);

    return 0;
}

void aml_task_deinit(struct aml_task *task)
{
    if (!task->task)
        return;

    AML_INFO("\"%s\" is terminating.\n", task->task->comm);

    task->quit = 1;
    aml_task_schedule(task);
    kthread_stop(task->task);
    task->task = NULL;
}
