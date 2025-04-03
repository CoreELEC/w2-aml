/**
 ******************************************************************************
 *
 * @file aml_cfgvendor.h
 *
 * @brief Linux cfg80211 Vendor Extension Code
 *        New vendor interface addition to nl80211/cfg80211 to allow vendors
 *        to implement proprietary features over the cfg80211 stack.
 *
 * Copyright (C) Amlogic 2012-2024
 *
 ******************************************************************************
 */

#ifndef __AML_CFGVENDOR_H__
#define __AML_CFGVENDOR_H__
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <linux/wait.h>
#include <net/cfg80211.h>
#include <net/rtnetlink.h>


#define GOOGLE_VENDOR_OUI 0x1A11
#ifdef CONFIG_AML_APF
extern struct mutex apf_mutex;

enum andr_vendor_subcmd {
    APF_SUBCMD_GET_CAPABILITIES = 0x1800,
    APF_SUBCMD_SET_FILTER,
    APF_SUBCMD_READ_FILTER_DATA,
    VENDOR_SUBCMD_MAX,
};

enum apf_attributes {
    APF_ATTRIBUTE_VERSION,
    APF_ATTRIBUTE_MAX_LEN,
    APF_ATTRIBUTE_PROGRAM,
    APF_ATTRIBUTE_PROGRAM_LEN,
    APF_ATTRIBUTE_MAX
};

#define ATTRIBUTE_U32_LEN                  (NLA_HDRLEN  + 4)
#define VENDOR_ID_OVERHEAD                 ATTRIBUTE_U32_LEN
#define VENDOR_SUBCMD_OVERHEAD             ATTRIBUTE_U32_LEN
#define VENDOR_DATA_OVERHEAD               (NLA_HDRLEN)

#define VENDOR_REPLY_OVERHEAD       (VENDOR_ID_OVERHEAD + \
                                     VENDOR_SUBCMD_OVERHEAD + \
                                     VENDOR_DATA_OVERHEAD)

extern int aml_cfgvendor_apf_get_capabilities(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void *data, int len);

extern int aml_cfgvendor_apf_set_filter(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void  *data, int len);

extern int aml_cfgvendor_apf_read_filter_data(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void *data, int len);
#endif /* CONFIG_AML_APF */

#endif /* __AML_CFGVENDOR_H__ */

