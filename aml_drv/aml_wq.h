/**
****************************************************************************************
*
* @file aml_wq.h
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief Declaration of the workqueue API mechanism.
*
****************************************************************************************
*/

#ifndef __AML_WQ_H__
#define __AML_WQ_H__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#include "aml_utils.h"
#include "aml_defs.h"

enum aml_wq_type {
    AML_WQ_NONE,
    AML_WQ_RECY,
    AML_WQ_SYNC_TRACE,
    AML_WQ_SYNC_BEACON,
    AML_WQ_SHOW_TX_MSG,
    AML_WQ_CHECK_SCC,
    AML_WQ_RECY_CONNECT_RETRY,
#ifndef CONFIG_AML_DEBUGFS
    AML_WQ_ALLOC_RX_RATE,
#endif
    AML_WQ_IPV6,
    AML_WQ_CANCEL_SCAN,
    AML_WQ_WAIT_USB,
    AML_WQ_MAX,
};

struct aml_wq {
    struct list_head list;
    struct aml_vif *aml_vif;
    struct aml_hw *aml_hw;
    enum aml_wq_type id;
    uint8_t len;
    uint8_t data[];
};

struct aml_wq *aml_wq_alloc(int len);
void aml_wq_add(struct aml_hw *aml_hw, struct aml_wq *aml_wq);
void aml_wq_del(struct aml_hw *aml_hw);
int aml_wq_init(struct aml_hw *aml_hw);
void aml_wq_deinit(struct aml_hw *aml_hw);

#endif
