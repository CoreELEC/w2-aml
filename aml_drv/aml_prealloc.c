/**
****************************************************************************************
*
* @file aml_prealloc.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022).
*
* @brief Preallocing buffer implementation.
*
****************************************************************************************
*/

#define AML_MODULE      GENERIC

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/skbuff.h>
#include <linux/version.h>

#include "aml_defs.h"
#include "aml_prealloc.h"

#ifdef CONFIG_AML_PREALLOC_BUF_SKB
void aml_prealloc_rxbuf_init(struct aml_hw *aml_hw, uint32_t rxbuf_sz)
{
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;
    int i = 0;

    aml_hw->prealloc_rxbuf_count = 0;
    /* coverity[USELESS_CALL] - standard kernel interface */
    spin_lock_init(&aml_hw->prealloc_rxbuf_lock);
    INIT_LIST_HEAD(&aml_hw->prealloc_rxbuf_free);
    INIT_LIST_HEAD(&aml_hw->prealloc_rxbuf_used);

    for (i = 0; i < PREALLOC_RXBUF_SIZE; i++) {
        prealloc_rxbuf = kmalloc(sizeof(struct aml_prealloc_rxbuf), GFP_ATOMIC);
        if (!prealloc_rxbuf) {
            ASSERT_ERR(0);
            return;
        }
        prealloc_rxbuf->skb = dev_alloc_skb(rxbuf_sz);
        if (prealloc_rxbuf->skb) {
            list_add_tail(&prealloc_rxbuf->list, &aml_hw->prealloc_rxbuf_free);
        } else {
            kfree(prealloc_rxbuf);
            i--;
        }
    }
}

void aml_prealloc_rxbuf_deinit(struct aml_hw *aml_hw)
{
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;
    struct aml_prealloc_rxbuf *prealloc_rxbuf_tmp = NULL;

    spin_lock_bh(&aml_hw->prealloc_rxbuf_lock);
    list_for_each_entry_safe(prealloc_rxbuf,
            prealloc_rxbuf_tmp, &aml_hw->prealloc_rxbuf_free, list) {
        if (prealloc_rxbuf->skb) {
            dev_kfree_skb(prealloc_rxbuf->skb);
        }
        kfree(prealloc_rxbuf);
    }
    spin_unlock_bh(&aml_hw->prealloc_rxbuf_lock);
}

struct aml_prealloc_rxbuf *aml_prealloc_get_free_rxbuf(struct aml_hw *aml_hw)
{
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;

    spin_lock_bh(&aml_hw->prealloc_rxbuf_lock);
    if (!list_empty(&aml_hw->prealloc_rxbuf_free)) {
        prealloc_rxbuf = list_first_entry(&aml_hw->prealloc_rxbuf_free,
                struct aml_prealloc_rxbuf, list);
        list_del(&prealloc_rxbuf->list);
        list_add_tail(&prealloc_rxbuf->list, &aml_hw->prealloc_rxbuf_used);
        AML_INFO("prealloc: get free skb=%p", prealloc_rxbuf->skb);
    }
    spin_unlock_bh(&aml_hw->prealloc_rxbuf_lock);
    return prealloc_rxbuf;
}

struct aml_prealloc_rxbuf *aml_prealloc_get_used_rxbuf(struct aml_hw *aml_hw)
{
    struct aml_prealloc_rxbuf *prealloc_rxbuf = NULL;
    struct aml_prealloc_rxbuf *prealloc_rxbuf_tmp = NULL;

    list_for_each_entry_safe(prealloc_rxbuf,
            prealloc_rxbuf_tmp, &aml_hw->prealloc_rxbuf_used, list) {
        spin_lock_bh(&aml_hw->prealloc_rxbuf_lock);
        list_del(&prealloc_rxbuf->list);
        list_add_tail(&prealloc_rxbuf->list, &aml_hw->prealloc_rxbuf_free);
        spin_unlock_bh(&aml_hw->prealloc_rxbuf_lock);
        AML_INFO("prealloc: get used skb=%p", prealloc_rxbuf->skb);
        return prealloc_rxbuf;
    }
    return NULL;
}
#endif
