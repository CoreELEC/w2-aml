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
#include "aml_msg_tx.h"

unsigned char regdom_en = 0;
REGDOM_USED_T regdom_used = {"00"};

REGDOM_PWR_TABLE_T regdom_power_table[REGDOM_PWR_MODE_MAX] = {
    {{"00"}}, //global
    {{"US"}}, //fcc
    {{"EU"}}, //ce
    {{"JP"}}, //arib
    {{"CN"}}, //srrc
    {{"BR"}}  //anatel
};


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

static const struct ieee80211_regdomain *aml_get_regdom(char *alpha2)
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
        wiphy->regulatory_flags |= REGULATORY_IGNORE_STALE_KICKOFF;

        wiphy_apply_custom_regulatory(wiphy, regdom);
    }
}

void aml_regdom_table_pwr_get(unsigned char channel, unsigned char band, unsigned char *power_ofdm, unsigned char *power_dsss)
{
    unsigned char index = 0;
    unsigned char channel_map_idx = 0;
    unsigned char limit_pwr;
    unsigned char limit_pwr_dsss_power = 0;

    index = aml_regdom_pwr_table_index_get();
    if (index == 0)
    {
        *power_ofdm = 19;
        *power_dsss = 21;
        return;
    }

    if (band == 1)
    {
        //5g band0 check
        if (channel <= 64)
        {
            //5g band0 80M
            if ((channel == 42) || (channel == 58))
            {
                channel_map_idx = (channel - 42) / 16;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw80_band0[channel_map_idx][index - 1];
            }
            //5g band0 40M
            else if ((channel == 38) || (channel == 46) || (channel == 54) || (channel == 62))
            {
                channel_map_idx = (channel - 38) / 8;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw40_band0[channel_map_idx][index - 1];
            }
            //5g band0 20M
            else
            {
                channel_map_idx = (channel - 36) / 4;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw20_band0[channel_map_idx][index - 1];
            }
        }


        else if (channel <= 144)
        {
            //5g band1 80M
            if ((channel == 106) || (channel == 122) || (channel == 138))
            {
                channel_map_idx = (channel - 106) / 16;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw80_band1[channel_map_idx][index - 1];
            }
            //5g band1 40M
            else if ((channel == 102) || (channel == 110) || (channel == 118) || (channel == 126) || (channel == 134) || (channel == 142))
            {
                channel_map_idx = (channel - 102) / 8;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw40_band1[channel_map_idx][index - 1];
            }
            //5g band1 20M
            else
            {
                channel_map_idx = (channel - 100) / 4;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw20_band1[channel_map_idx][index - 1];
            }
        }
        else
        {
            //5g band2 80M
            if (channel == 155)
            {
                channel_map_idx = (channel - 155) / 16;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw80_band2[channel_map_idx][index - 1];
            }
            //5g band2 40M
            else if ((channel == 151) || (channel == 159) || (channel == 167) || (channel == 175))
            {
                channel_map_idx = (channel - 151) / 8;
                limit_pwr = country_pwr_limit_cfg.wf5g_bw40_band2[channel_map_idx][index - 1];
            }
            //5g band2 20M
            else
            {
                channel_map_idx = (channel - 149) / 4;
                if (channel_map_idx >= 7)
                {
                    channel_map_idx = 6;
                }
                limit_pwr = country_pwr_limit_cfg.wf5g_bw20_band2[channel_map_idx][index - 1];
            }
        }
        *power_ofdm = limit_pwr;
    }
    else
    {
        limit_pwr_dsss_power = country_pwr_limit_cfg.wf2g_dsss_limit[channel-1][index-1];
        limit_pwr = country_pwr_limit_cfg.wf2g_ofdm_limit[channel-1][index -1];
        *power_ofdm = limit_pwr;
        *power_dsss = limit_pwr_dsss_power;
    }
}

unsigned char aml_regdom_pwr_table_index_get(void)
{
    unsigned char index = 0;

    while (index < REGDOM_PWR_MODE_MAX)
    {
        if ((regdom_used.regdom_used_code[0] == regdom_power_table[index].regdom_code[0]) &&
                (regdom_used.regdom_used_code[1] == regdom_power_table[index].regdom_code[1])) {
            break;
        }
        index++;
    }

    if (index == REGDOM_PWR_MODE_MAX)
        index = 0;

    return index;
}

unsigned char aml_regdom_set_pwr(unsigned char power, unsigned char power1)
{
    unsigned char tx_pwr = 0;

    tx_pwr = power > power1 ? power1 : power;

    return tx_pwr;
}

int aml_regdom_doit(struct aml_hw *aml_hw, void *regdom_wq, int len)
{
    struct regdom_set_power_req *req = regdom_wq;
    int ret = 0;
    unsigned char ofdm_power = 0;
    unsigned char dsss_power = 0;

    aml_regdom_table_pwr_get(req->channel, req->band, &ofdm_power, &dsss_power);
    ofdm_power = ofdm_power & 0x7f;

    ret = aml_send_set_power(aml_hw, req->vif_index, ofdm_power, NULL);

    return ret;
}
