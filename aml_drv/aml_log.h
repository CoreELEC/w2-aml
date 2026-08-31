/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _AML_LOG_H_
#define _AML_LOG_H_

#include <linux/types.h>
#include <linux/kernel.h>

#include "aml_prof.h"

#ifndef CONFIG_AML_LOG_PREFIX
#define CONFIG_AML_LOG_PREFIX       /* "[W2]" or "[W2L]", set by Makefile */
#endif

/*
 * the log level is same as kernel, please refer to linux/kern_levels.h.
 *  - the log will be excluded while building if its level is lower than CONFIG_AML_LOG_BUILD_LEVEL
 *  - default module level should be
 *      - interrupt: LOGLEVEL_ERR
 *      - data path: LOGLEVEL_WARNING
 *      - module under debugging: LOGLEVEL_INFO or LOGLEVEL_DEBUG.
 *      - others: LOGLEVEL_WARNING or LOGLEVEL_NOTICE.
 */
#ifdef CONFIG_AML_LOG_BUILD_LEVEL
#if CONFIG_AML_LOG_BUILD_LEVEL < LOGLEVEL_EMERG || CONFIG_AML_LOG_BUILD_LEVEL > LOGLEVEL_DEBUG
#error "CONFIG_AML_LOG_BUILD_LEVEL is out of range!"
#endif
#else
#define CONFIG_AML_LOG_BUILD_LEVEL  LOGLEVEL_DEBUG  /* build all level's log for now */
#endif

#define AML_LOG_MODULES \
        AML_LOG_MODULE(GENERIC,     DEBUG) /* unknown */ \
        AML_LOG_MODULE(RECOVERY,    DEBUG) \
        AML_LOG_MODULE(BA,          NOTICE) \
        AML_LOG_MODULE(REO,         ERR) \
        AML_LOG_MODULE(TX,          NOTICE) \
        AML_LOG_MODULE(RX,          INFO) \
        AML_LOG_MODULE(RX_IRQ,      WARNING) \
        AML_LOG_MODULE(CMD,         DEBUG) \
        AML_LOG_MODULE(TRACE,       INFO) \
        AML_LOG_MODULE(INTERFACE,   INFO) \
        AML_LOG_MODULE(IWPRIV,      INFO) \
        AML_LOG_MODULE(MAIN,        DEBUG) \
        AML_LOG_MODULE(MDNS,        INFO) \
        AML_LOG_MODULE(MSG_RX,      DEBUG) \
        AML_LOG_MODULE(MSG_TX,      DEBUG) \
        AML_LOG_MODULE(PLATF,       DEBUG) \
        AML_LOG_MODULE(TESTM,       DEBUG) \
        AML_LOG_MODULE(PCI,         INFO) \
        AML_LOG_MODULE(COMMON,      INFO) \
        AML_LOG_MODULE(SDIO,        INFO) \
        AML_LOG_MODULE(USB,         INFO) \
        AML_LOG_MODULE(UTILS,       INFO) \
        AML_LOG_MODULE(CSI,         INFO) \
        AML_LOG_MODULE(IRQ,         ERR) \
        AML_LOG_MODULE(P2P,         INFO) \
        AML_LOG_MODULE(TCP,         INFO) \
        AML_LOG_MODULE(RATE,        ERR) \
        AML_LOG_MODULE(SCAN,        WARNING) \

enum aml_log_module {
#define AML_LOG_MODULE(_m, _level)  AML_LOG_MODULE_##_m,
    AML_LOG_MODULES
#undef AML_LOG_MODULE
    AML_LOG_MODULE_MAX,
};

extern s8 aml_log_m_levels[AML_LOG_MODULE_MAX]; /* e.g. LOGLEVEL_ERR ... */

extern const char *aml_log_level_names[];
extern const char *aml_log_module_names[];
int aml_name_index(const char *names[], const char *name);

/* each C file at its beginning may declare a default module that it belongs to */
#ifndef AML_MODULE
#define AML_MODULE                  GENERIC
#endif

/*
 * exported log APIs for the default module: "AML_MODULE"
 */
#define AML_ERR(fmt, ...)           _AML_ERR    (AML_MODULE, true, fmt, ##__VA_ARGS__)
#define AML_WARN(fmt, ...)          _AML_WARN   (AML_MODULE, true, fmt, ##__VA_ARGS__)
#define AML_NOTICE(fmt, ...)        _AML_NOTICE (AML_MODULE, true, fmt, ##__VA_ARGS__)
#define AML_INFO(fmt, ...)          _AML_INFO   (AML_MODULE, true, fmt, ##__VA_ARGS__)
#define AML_DBG(fmt, ...)           _AML_DBG    (AML_MODULE, true, fmt, ##__VA_ARGS__)

/*
 * exported log APIs for the default module: "AML_MODULE" and rate limited.
 */
#define AML_RLMT_ERR(fmt, ...)      _AML_ERR    (AML_MODULE, net_ratelimit(), fmt, ##__VA_ARGS__)
#define AML_RLMT_WARN(fmt, ...)     _AML_WARN   (AML_MODULE, net_ratelimit(), fmt, ##__VA_ARGS__)
#define AML_RLMT_NOTICE(fmt, ...)   _AML_NOTICE (AML_MODULE, net_ratelimit(), fmt, ##__VA_ARGS__)
#define AML_RLMT_INFO(fmt, ...)     _AML_INFO   (AML_MODULE, net_ratelimit(), fmt, ##__VA_ARGS__)
#define AML_RLMT_DBG(fmt, ...)      _AML_DBG    (AML_MODULE, net_ratelimit(), fmt, ##__VA_ARGS__)

/*
 * exported log APIs should specify the particular module
 */
#define AML_M_ERR(_m, fmt, ...)     _AML_ERR    (_m, true, fmt, ##__VA_ARGS__)
#define AML_M_WARN(_m, fmt, ...)    _AML_WARN   (_m, true, fmt, ##__VA_ARGS__)
#define AML_M_NOTICE(_m, fmt, ...)  _AML_NOTICE (_m, true, fmt, ##__VA_ARGS__)
#define AML_M_INFO(_m, fmt, ...)    _AML_INFO   (_m, true, fmt, ##__VA_ARGS__)
#define AML_M_DBG(_m, fmt, ...)     _AML_DBG    (_m, true, fmt, ##__VA_ARGS__)

#define AML_FMT_M(_level, _m, fmt)  CONFIG_AML_LOG_PREFIX "[%8s] " fmt, #_m

#define AML_FMT_M_FN_LN(_level, _m, fmt) \
        CONFIG_AML_LOG_PREFIX "[%8s] [%s %d]" fmt, #_m, __func__, __LINE__

#ifndef AML_FMT
#define AML_FMT                     AML_FMT_M_FN_LN
#endif

#define AML_FN_ENTRY_STR            ">>> %s(%d)\n", __func__, __LINE__
#define AML_FN_EXIT_STR             "<<< %s(%d)\n", __func__, __LINE__
#define AML_FN_ENTRY()              AML_INFO(AML_FN_ENTRY_STR)
#define AML_FN_EXIT()               AML_INFO(AML_FN_EXIT_STR)

/*
 * exported log API to check the log is enabled or not.
 */
#define AML_LOG_EN(_level, _m)      (LOGLEVEL_##_level <= aml_log_m_levels[AML_LOG_MODULE_##_m])

/*
 * the following log APIs should not be used directly.
 */
#define _AML_LOG(_level, _m, _rlmt, fmt, ...)  do { \
            if (LOGLEVEL_##_level <= aml_log_m_levels[AML_LOG_MODULE_##_m] && _rlmt) { \
                printk(AML_FMT(_level, _m, fmt), ##__VA_ARGS__); \
            } \
        } while (0)

#if CONFIG_AML_LOG_BUILD_LEVEL >= LOGLEVEL_ERR
#define _AML_ERR(_m, _rt, fmt, ...)     _AML_LOG(ERR,     _m, _rt, fmt, ##__VA_ARGS__)
#else
#define _AML_ERR(_m, _rt, fmt, ...)     do {} while(0)
#endif
#if CONFIG_AML_LOG_BUILD_LEVEL >= LOGLEVEL_WARNING
#define _AML_WARN(_m, _rt, fmt, ...)    _AML_LOG(WARNING, _m, _rt, fmt, ##__VA_ARGS__)
#else
#define _AML_WARN(_m, _rt, fmt, ...)    do {} while(0)
#endif
#if CONFIG_AML_LOG_BUILD_LEVEL >= LOGLEVEL_NOTICE
#define _AML_NOTICE(_m, _rt, fmt, ...)  _AML_LOG(NOTICE,  _m, _rt, fmt, ##__VA_ARGS__)
#else
#define _AML_NOTICE(_m, _rt, fmt, ...)  do {} while(0)
#endif
#if CONFIG_AML_LOG_BUILD_LEVEL >= LOGLEVEL_INFO
#define _AML_INFO(_m, _rt, fmt, ...)    _AML_LOG(INFO,    _m, _rt, fmt, ##__VA_ARGS__)
#else
#define _AML_INFO(_m, _rt, fmt, ...)    do {} while(0)
#endif
#if CONFIG_AML_LOG_BUILD_LEVEL >= LOGLEVEL_DEBUG
#define _AML_DBG(_m, _rt, fmt, ...)     _AML_LOG(DEBUG,   _m, _rt, fmt, ##__VA_ARGS__)
#else
#define _AML_DBG(_m, _rt, fmt, ...)     do {} while(0)
#endif

/* FIXME: all ERROR_DEBUG_OUT should be replaced as AML_ERR */
#define ERROR_DEBUG_OUT(fmt,...)        AML_ERR(fmt, ##__VA_ARGS__)

#define HERE                            AML_ERR("here\n")

#endif /* _AML_LOG_H_ */
