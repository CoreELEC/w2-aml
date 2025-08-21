/**
 ****************************************************************************************
 *
 * @file aml_regdom.c
 *
 * Copyright (C) Amlogic, Inc. All rights reserved (2022).
 *
 * @brief Ruglatory domain implementation.
 *
 ****************************************************************************************
 */
#include "aml_regdom.h"
#include "aml_utils.h"


static const struct ieee80211_regdomain regdom_global = {
    .n_reg_rules = 7,
    .alpha2 =  "00",
    .reg_rules = {
        /* channels 1..11 */
        REG_RULE(2412-10, 2462+10, 40, 6, 20, 0),
        /* channels 12..13. */
        REG_RULE(2467-10, 2472+10, 40, 6, 20, NL80211_RRF_AUTO_BW),
        /* channel 14. only JP enables this and for 802.11b only */
        REG_RULE(2484-10, 2484+10, 20, 6, 20, NL80211_RRF_NO_OFDM),
        /* channel 36..48 */
        REG_RULE(5180-10, 5240+10, 160, 6, 20, NL80211_RRF_AUTO_BW),
        /* channel 52..64 - DFS required */
        REG_RULE(5260-10, 5320+10, 160, 6, 20, NL80211_RRF_DFS |
                NL80211_RRF_AUTO_BW),
        /* channel 100..144 - DFS required */
        REG_RULE(5500-10, 5720+10, 160, 6, 20, NL80211_RRF_DFS),
        /* channel 149..165 */
        REG_RULE(5745-10, 5825+10, 80, 6, 20, 0),
    }
};

const struct ieee80211_regdomain regdom_cn = {
    .n_reg_rules = 4,
    .alpha2 = "CN",
    .reg_rules = {
        /* channels 1..13 */
        REG_RULE(2412-10, 2472+10, 40, 6, 20, 0),
        /* channel 36..48 */
        REG_RULE(5180-10, 5240+10, 160, 6, 20, NL80211_RRF_AUTO_BW),
        /* channel 52..64 - DFS required */
        REG_RULE(5260-10, 5320+10, 160, 6, 20, NL80211_RRF_DFS |
                NL80211_RRF_AUTO_BW),
        /* channels 149..165 */
        REG_RULE(5745-10, 5825+10, 80, 6, 20, 0),
    }
};

const struct ieee80211_regdomain regdom_us = {
    .n_reg_rules = 5,
    .alpha2 = "US",
    .reg_rules = {
        /* channels 1..11 */
        REG_RULE(2412-10, 2462+10, 40, 6, 20, 0),
        /* channel 36..48 */
        REG_RULE(5180-10, 5240+10, 80, 6, 20, NL80211_RRF_AUTO_BW),
        /* channel 52..64 - DFS required */
        REG_RULE(5260-10, 5320+10, 80, 6, 20, NL80211_RRF_DFS |
                NL80211_RRF_AUTO_BW),
        /* channel 100..140 - DFS required */
        REG_RULE(5500-10, 5720+10, 160, 6, 20, NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
        /* channels 149..165 */
        REG_RULE(5745-10, 5825+10, 80, 6, 20, 0),
    }
};

const struct ieee80211_regdomain regdom_jp = {
    .n_reg_rules = 5,
    .alpha2 = "JP",
    .reg_rules = {
        /* channels 1..13 */
        REG_RULE(2412-10, 2472+10, 40, 6, 20, 0),
        /* channels 14 */
        REG_RULE(2484-10, 2484+10, 20, 6, 20, NL80211_RRF_NO_OFDM),
        /* channels 36..48 */
        REG_RULE(5180-10, 5240+10, 80, 6, 20, NL80211_RRF_AUTO_BW),
        /* channels 52..64 */
        REG_RULE(5260-10, 5320+10, 80, 6, 20, NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
        /* channels 100..140 */
        REG_RULE(5500-10, 5700+10, 160, 6, 20, NL80211_RRF_DFS)
    }
};

const struct aml_regdom aml_regdom_00 = {
    .country_code = "00",
    .regdom = &regdom_global
};

const struct aml_regdom aml_regdom_cn = {
    .country_code = "CN",
    .regdom = &regdom_cn
};

const struct aml_regdom aml_regdom_us = {
    .country_code = "US",
    .regdom = &regdom_us
};

const struct aml_regdom aml_regdom_jp = {
    .country_code = "JP",
    .regdom = &regdom_jp
};

const struct aml_regdom *aml_regdom_tbl[] = {
    &aml_regdom_00,
    &aml_regdom_cn,
    &aml_regdom_us,
    &aml_regdom_jp,
    NULL
};

const struct ieee80211_regdomain *aml_get_regdom(char *alpha2)
{
    const struct aml_regdom *regdom;
    int i = 0;

    while (aml_regdom_tbl[i]) {
        regdom = aml_regdom_tbl[i];
        if ((regdom->country_code[0] == alpha2[0]) &&
                (regdom->country_code[1] == alpha2[1])) {
            return regdom->regdom;
        }
        i++;
    }
    return NULL;
}

void aml_apply_regdom(struct aml_hw *aml_hw, struct wiphy *wiphy, char *alpha2)
{
    u32 band_idx, ch_idx;
    struct ieee80211_supported_band *sband;
    struct ieee80211_channel *chan;
    const struct ieee80211_regdomain *regdom;

    if (aml_hw->mod_params->custregd)
        return;

    AML_INFO("apply regdom alpha=%s", alpha2);

    regdom = aml_get_regdom(alpha2);
    if (regdom) {
        /* reset channel flags */
        for (band_idx = 0; band_idx < 2; band_idx++) {
            sband = wiphy->bands[band_idx];
            if (!sband)
                continue;

            for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
                chan = &sband->channels[ch_idx];
                chan->flags = 0;
            }
        }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
        wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
#else
        wiphy->regulatory_flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 3, 12)
        wiphy->regulatory_flags |= REGULATORY_IGNORE_STALE_KICKOFF;
#else
        wiphy->regulatory_flags |= (REGULATORY_WIPHY_SELF_MANAGED >> 1);
#endif
#endif

        wiphy_apply_custom_regulatory(wiphy, regdom);
    }
}
