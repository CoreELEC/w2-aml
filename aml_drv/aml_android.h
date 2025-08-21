/**
 ****************************************************************************************
 *
 * @file aml_android.h
 *
 * Copyright (C) Amlogic, Inc. All rights reserved (2022).
 *
 * @brief Declaration of Android WPA driver API implementation.
 *
 ****************************************************************************************
 */
#include <linux/compat.h>
#include "aml_defs.h"

#ifndef __AML_ANDROID_H__
#define __AML_ANDROID_H__

#define AML_ANDROID_CMD_MAX_LEN    8192
#define AML_ANDROID_CMD(_id, _str) {   \
    .id     = (_id),    \
    .str    = (_str),   \
}

struct aml_android_priv_cmd {
    char *buf;
    int used_len;
    int total_len;
};

#ifdef CONFIG_COMPAT
struct aml_android_compat_priv_cmd {
    compat_caddr_t buf;
    int used_len;
    int total_len;
};
#endif

enum aml_android_cmdid {
    CMDID_RSSI = 4,
    CMDID_COUNTRY = 17,
    CMDID_P2P_SET_NOA,
    CMDID_P2P_GET_NOA,
    CMDID_P2P_SET_PS,
    CMDID_SET_AP_WPS_P2P_IE,
    CMDID_BTCOEXSCAN,
    CMDID_SETSUSPENDMODE,
};

struct aml_android_cmd {
    int id;
    char *str;
};

int aml_android_priv_ioctl(struct aml_vif *aml_vif, void __user *data);

#endif
