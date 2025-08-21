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

struct aml_hw;

struct aml_wq {
    struct list_head list;
    int (*fn)(struct aml_hw *aml_hw, void *data, int len);
    int (*fn_none)(struct aml_hw *aml_hw);
    int (*fn_ptr)(struct aml_hw *aml_hw, void *ptr);
    int len;
    void *data[];
};

int aml_wq_init(struct aml_hw *aml_hw);
void aml_wq_deinit(struct aml_hw *aml_hw);

int aml_wq_do(int (*fn_none)(struct aml_hw *aml_hw),
              struct aml_hw *aml_hw);
int aml_wq_do_ptr(int (*fn_ptr)(struct aml_hw *aml_hw, void *ptr),
                  struct aml_hw *aml_hw, void *ptr);
int aml_wq_do_data(int (*fn)(struct aml_hw *aml_hw, void *data, int len),
                   struct aml_hw *aml_hw, void *data, int len);

#endif
