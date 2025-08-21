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

enum
{
    GLOBAL_PWR_LIMIT = 0, //00
    FCC_PWR_LIMIT,        //US
    CE_PWR_LIMIT,         //EU
    ARIB_PWR_LIMIT,       //JP
    SRRC_PWR_LIMIT,       //CN
    ANATEL_PWR_LIMIT,     //BR
    REGDOM_PWR_MODE_MAX,
};

typedef struct REGDOM_USED
{
    char regdom_used_code[3];
}REGDOM_USED_T;

//wf2g:ch1~ch14, wf5g_band0:ch34~ch64, wf5g band1:ch100~ch144, wf5g band2:ch149~ch165
typedef struct REGDOM_PWR_TABLE
{
    char regdom_code[3];

}REGDOM_PWR_TABLE_T;

struct regdom_set_power_req
{
    unsigned char vif_index;
    unsigned char width;
    unsigned char band;
    unsigned char channel;
};

extern unsigned char regdom_en;
extern REGDOM_USED_T regdom_used;
extern REGDOM_PWR_TABLE_T regdom_power_table[];
extern struct COUNTRY_PWR_LIMIT_CFG country_pwr_limit_cfg;

void aml_apply_regdom(struct aml_hw *aml_hw, struct wiphy *wiphy, char *alpha2);
unsigned char aml_regdom_pwr_table_index_get(void);
void aml_regdom_table_pwr_get(unsigned char channel, unsigned char band, unsigned char *power_ofdm, unsigned char *power_dsss);
unsigned char aml_regdom_set_pwr(unsigned char power, unsigned char power1);
int aml_regdom_doit(struct aml_hw *aml_hw, void *regdom_wq, int len);

#endif
