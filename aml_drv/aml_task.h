/**
****************************************************************************************
*
* @file aml_task.h
*
* Copyright (C) Amlogic, Inc. All rights reserved (2023).
*
* @brief Declaration of the task API mechanism.
*
****************************************************************************************
*/

#ifndef __AML_TASK_H__
#define __AML_TASK_H__

#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/kthread.h>

/*
 * Wi-Fi task's priority should not be higher than video decoder,
 * PS: the priority of surfaceflinger is 98 in S905L3Y (kernel 4.9).
 */
#define AML_TASK_PRI   98

#undef AML_TASK_SCH_BY_SEM

struct aml_task {
    struct task_struct *task;
    int (*fn)(struct aml_task *t);

    int quit;
#ifdef AML_TASK_SCH_BY_SEM
    struct semaphore sem;
#else
    struct completion completion;
#endif

    /* private data */
    void *data[];
};

#define AML_TASK_INIT(_task, fn_name, name, cpu)   \
        aml_task_init(_task, fn_name, "amlw_"#name, cpu)

#define AML_TASK_DEINIT(_task) \
        aml_task_deinit(_task)

int aml_task_init(struct aml_task *task, int (*fn)(struct aml_task *t), const char *name, int cpu);
void aml_task_deinit(struct aml_task *task);

static inline void aml_task_schedule(struct aml_task *task)
{
#ifdef AML_TASK_SCH_BY_SEM
    up(&task->sem);
#else
    complete(&task->completion);
#endif
}

#endif //__AML_TASK_H__
