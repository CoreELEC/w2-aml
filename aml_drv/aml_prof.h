/**
 ****************************************************************************************
 *
 * @file aml_prof.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#ifndef _AML_PROF_H_
#define _AML_PROF_H_

#include "linux/gpio.h"

#define AML_PROF_CNT(name, counter)         AML_PROF('C', #name, counter)
#define AML_PROF_HI(name)                   AML_PROF('S', #name, 1000) // 'B' doesn't work
#define AML_PROF_LO(name)                   AML_PROF('F', #name, 1000) // 'E' doesn't work
#define AML_PROF_START(name, cookie)        AML_PROF('S', #name, AML_PROF_COOKIE(cookie))
#define AML_PROF_FIN(name, cookie)          AML_PROF('F', #name, AML_PROF_COOKIE(cookie))

#define AML_PROF(flag, name, value)         do { } while(0)

struct aml_hw;

static inline void aml_prof_set(struct aml_hw *aml_hw, int gpio)
{
    gpio_set_value(gpio, 1);
}

static inline void aml_prof_clear(struct aml_hw *aml_hw, int gpio)
{
    gpio_set_value(gpio, 0);
}

#if 0
/* Defines for SW Profiling registers values */
enum {
    TX_IPC_IRQ,
    TX_IPC_EVT,
    TX_PREP_EVT,
    TX_DMA_IRQ,
    TX_MAC_IRQ,
    TX_PAYL_HDL,
    TX_CFM_EVT,
    TX_IPC_CFM,
    RX_MAC_IRQ,                 // 8
    RX_TRIGGER_EVT,
    RX_DMA_IRQ,
    RX_DMA_EVT,
    RX_IPC_IND,
    RX_MPDU_XFER,
    DBG_PROF_MAX
};
#endif

enum {
    SW_PROF_HOSTBUF_IDX = 12,
    /****** IPC IRQs related signals ******/
    /* E2A direction */
    SW_PROF_IRQ_E2A_RXDESC = 16,    // to make sure we let 16 bits available for LMAC FW
    SW_PROF_IRQ_E2A_TXCFM,
    SW_PROF_IRQ_E2A_DBG,
    SW_PROF_IRQ_E2A_MSG,
    SW_PROF_IPC_MSGPUSH,
    SW_PROF_MSGALLOC,
    SW_PROF_MSGIND,
    SW_PROF_DBGIND,

    /* A2E direction */
    SW_PROF_IRQ_A2E_TXCFM_BACK,

    /****** Driver functions related signals ******/
    SW_PROF_WAIT_QUEUE_STOP,
    SW_PROF_WAIT_QUEUE_WAKEUP,
    SW_PROF_AMLDATAIND,
    SW_PROF_AML_IPC_IRQ_HDLR,
    SW_PROF_AML_IPC_THR_IRQ_HDLR,
    SW_PROF_IEEE80211RX,
    SW_PROF_AML_PATTERN,
    SW_PROF_MAX
};

// [LT]For debug purpose only
#if (0)
#define SW_PROF_CHAN_CTXT_CFM_HDL_BIT       (21)
#define SW_PROF_CHAN_CTXT_CFM_BIT           (22)
#define SW_PROF_CHAN_CTXT_CFM_SWDONE_BIT    (23)
#define SW_PROF_CHAN_CTXT_PUSH_BIT          (24)
#define SW_PROF_CHAN_CTXT_QUEUE_BIT         (25)
#define SW_PROF_CHAN_CTXT_TX_BIT            (26)
#define SW_PROF_CHAN_CTXT_TX_PAUSE_BIT      (27)
#define SW_PROF_CHAN_CTXT_PSWTCH_BIT        (28)
#define SW_PROF_CHAN_CTXT_SWTCH_BIT         (29)

// TO DO: update this

#define REG_SW_SET_PROFILING_CHAN(env, bit)             \
    aml_prof_set((struct aml_hw*)env, BIT(bit))

#define REG_SW_CLEAR_PROFILING_CHAN(env, bit) \
    aml_prof_clear((struct aml_hw*)env, BIT(bit))

#else
#define SW_PROF_CHAN_CTXT_CFM_HDL_BIT       (0)
#define SW_PROF_CHAN_CTXT_CFM_BIT           (0)
#define SW_PROF_CHAN_CTXT_CFM_SWDONE_BIT    (0)
#define SW_PROF_CHAN_CTXT_PUSH_BIT          (0)
#define SW_PROF_CHAN_CTXT_QUEUE_BIT         (0)
#define SW_PROF_CHAN_CTXT_TX_BIT            (0)
#define SW_PROF_CHAN_CTXT_TX_PAUSE_BIT      (0)
#define SW_PROF_CHAN_CTXT_PSWTCH_BIT        (0)
#define SW_PROF_CHAN_CTXT_SWTCH_BIT         (0)

#define REG_SW_SET_PROFILING_CHAN(env, bit)            do {} while (0)
#define REG_SW_CLEAR_PROFILING_CHAN(env, bit)          do {} while (0)
#endif

#ifdef CONFIG_AML_SW_PROFILING
/* Macros for SW PRofiling registers access */
#define REG_SW_SET_PROFILING(env, bit)                  \
    aml_prof_set((struct aml_hw*)env, BIT(bit))

#define REG_SW_SET_HOSTBUF_IDX_PROFILING(env, val)      \
    aml_prof_set((struct aml_hw*)env, val<<(SW_PROF_HOSTBUF_IDX))

#define REG_SW_CLEAR_PROFILING(env, bit)                \
    aml_prof_clear((struct aml_hw*)env, BIT(bit))

#define REG_SW_CLEAR_HOSTBUF_IDX_PROFILING(env)                         \
    aml_prof_clear((struct aml_hw*)env,0x0F<<(SW_PROF_HOSTBUF_IDX))

#else
#define REG_SW_SET_PROFILING(env, value)            do {} while (0)
#define REG_SW_CLEAR_PROFILING(env, value)          do {} while (0)
#define REG_SW_SET_HOSTBUF_IDX_PROFILING(env, val)  do {} while (0)
#define REG_SW_CLEAR_HOSTBUF_IDX_PROFILING(env)     do {} while (0)
#endif

#endif /* _AML_PROF_H_ */
