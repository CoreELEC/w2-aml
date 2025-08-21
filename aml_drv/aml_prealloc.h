/**
****************************************************************************************
*
* @file aml_prealloc.h
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022).
*
* @brief Declaration of the preallocing buffer.
*
****************************************************************************************
*/

#ifndef _AML_PREALLOC_H_
#define _AML_PREALLOC_H_

#include "aml_static_buf.h"

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC

static inline void *aml_prealloc_get_ex(enum prealloc_buf_type buf_type,
                                        size_t req_size,
                                        size_t *actual_size)
{
    return __aml_mem_prealloc(buf_type, req_size, actual_size);
}

static inline void *aml_prealloc_get(enum prealloc_buf_type buf_type, size_t req_size)
{
    return aml_prealloc_get_ex(buf_type, req_size, NULL);
}

#endif  // CONFIG_PREALLOC_BUF_STATIC

#ifdef CONFIG_AML_PREALLOC_BUF_SKB
/* prealloc rxbuf definition and structure */
#define PREALLOC_RXBUF_SIZE     (32 + 32)
#define PREALLOC_RXBUF_FACTOR   (16)

struct aml_prealloc_rxbuf {
    struct list_head list;
    struct sk_buff *skb;
};

void aml_prealloc_rxbuf_init(struct aml_hw *aml_hw, uint32_t rxbuf_sz);
void aml_prealloc_rxbuf_deinit(struct aml_hw *aml_hw);
struct aml_prealloc_rxbuf *aml_prealloc_get_free_rxbuf(struct aml_hw *aml_hw);
struct aml_prealloc_rxbuf *aml_prealloc_get_used_rxbuf(struct aml_hw *aml_hw);

#endif  // CONFIG_PREALLOC_BUF_SKB
#endif  // _AML_PREALLOC_H_
