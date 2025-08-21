/**
****************************************************************************************
*
* @file aml_wq.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief workqueue API implementation.
*
****************************************************************************************
*/
#include <linux/types.h>

#include "aml_wq.h"
#include "aml_defs.h"

static inline struct aml_wq *aml_wq_get(struct aml_hw *aml_hw)
{
    struct aml_wq *aml_wq;

    spin_lock_bh(&aml_hw->wq_lock);
    aml_wq = list_first_entry_or_null(&aml_hw->work_list, struct aml_wq, list);
    if (aml_wq)
        list_del(&aml_wq->list);
    spin_unlock_bh(&aml_hw->wq_lock);

    return aml_wq;
}

static void aml_wq_doit(struct work_struct *work)
{
    struct aml_hw *aml_hw = container_of(work, struct aml_hw, work);
    struct aml_wq *aml_wq;

    while ((aml_wq = aml_wq_get(aml_hw))) {
        aml_wq->fn(aml_hw, aml_wq->data, aml_wq->len);
        kfree(aml_wq);
    }
}

static int __aml_wq_do(struct aml_hw *aml_hw, void *data, int len,
                       int (*fn)(struct aml_hw *aml_hw, void *data, int len),
                       int (*fn_none)(struct aml_hw *aml_hw),
                       int (*fn_ptr)(struct aml_hw *aml_hw, void *ptr))
{
    int size = sizeof(struct aml_wq) + len;
    struct aml_wq *wq = kzalloc(size, GFP_ATOMIC);

    if (!wq) {
        AML_ERR("no memory for aml_wq (%d size)!\n", len);
        return -ENOMEM;
    }

    INIT_LIST_HEAD(&wq->list);
    wq->len = len;
    wq->fn = fn;
    wq->fn_none = fn_none;
    wq->fn_ptr = fn_ptr;
    if (len) {
        BUG_ON(!data);
        memcpy(wq->data, data, len);
    }

    spin_lock_bh(&aml_hw->wq_lock);
    list_add_tail(&wq->list, &aml_hw->work_list);
    spin_unlock_bh(&aml_hw->wq_lock);

    if (!work_pending(&aml_hw->work))
        queue_work(aml_hw->wq, &aml_hw->work);

    /* coverity[leaked_storage] - wq will be freed at aml_wq_doit or deinit */
    return 0;
}

static int aml_wq_doit_none(struct aml_hw *aml_hw, void *data, int len)
{
    struct aml_wq *wq = container_of(data, struct aml_wq, data);

    return wq->fn_none(aml_hw);
}

int aml_wq_do(int (*fn_none)(struct aml_hw *aml_hw),
              struct aml_hw *aml_hw)
{
    BUG_ON(!fn_none);
    return __aml_wq_do(aml_hw, NULL, 0, aml_wq_doit_none, fn_none, NULL);
}

static int aml_wq_doit_ptr(struct aml_hw *aml_hw, void *data, int len)
{
    struct aml_wq *wq = container_of(data, struct aml_wq, data);

    return wq->fn_ptr(aml_hw, *(void **)data);
}

int aml_wq_do_ptr(int (*fn_ptr)(struct aml_hw *aml_hw, void *ptr),
                  struct aml_hw *aml_hw, void *ptr)
{
    BUG_ON(!fn_ptr);
    return __aml_wq_do(aml_hw, &ptr, sizeof(ptr), aml_wq_doit_ptr, NULL, fn_ptr);
}

int aml_wq_do_data(int (*fn)(struct aml_hw *aml_hw, void *data, int len),
                   struct aml_hw *aml_hw, void *data, int len)
{
    return __aml_wq_do(aml_hw, data, len, fn, NULL, NULL);
}

int aml_wq_init(struct aml_hw *aml_hw)
{
    /* coverity[USELESS_CALL] - spinlock_check(_lock) */
    spin_lock_init(&aml_hw->wq_lock);
    INIT_LIST_HEAD(&aml_hw->work_list);
    INIT_WORK(&aml_hw->work, aml_wq_doit);

    aml_hw->wq = alloc_ordered_workqueue("w2_wq",
            WQ_HIGHPRI | WQ_CPU_INTENSIVE |
            WQ_MEM_RECLAIM);
    if (!aml_hw->wq) {
        AML_INFO("wq create failed");
        return -ENOMEM;
    }

    /* coverity[RESOURCE_LEAK] , wq will be freed in aml_wq_doit */
    return 0;
}

void aml_wq_deinit(struct aml_hw *aml_hw)
{
    struct aml_wq *aml_wq, *tmp;

    AML_FN_ENTRY();
    cancel_work_sync(&aml_hw->work);

    spin_lock_bh(&aml_hw->wq_lock);
    list_for_each_entry_safe(aml_wq, tmp, &aml_hw->work_list, list) {
        list_del(&aml_wq->list);
        kfree(aml_wq);
    }
    spin_unlock_bh(&aml_hw->wq_lock);

    flush_workqueue(aml_hw->wq);
    destroy_workqueue(aml_hw->wq);
}
