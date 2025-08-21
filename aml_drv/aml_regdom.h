/**
 ****************************************************************************************
 *
 * @file aml_regdom.h
 *
 * Copyright (C) Amlogic, Inc. All rights reserved (2022).
 *
 * @brief Declaration of the preallocing buffer.
 *
 ****************************************************************************************
 */

#ifndef __AML_REGDOM_H__
#define __AML_REGDOM_H__

#include <net/cfg80211.h>
#include <linux/types.h>
#include "aml_defs.h"

struct aml_regdom {
    char country_code[2];
    const struct ieee80211_regdomain *regdom;
};

void aml_apply_regdom(struct aml_hw *aml_hw, struct wiphy *wiphy, char *alpha2);

#endif
