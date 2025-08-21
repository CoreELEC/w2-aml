/**
******************************************************************************
*
* @file aml_mod_params.c
*
* @brief Set configuration according to modules parameters
*
* Copyright (C) Amlogic 2012-2021
*
******************************************************************************
*/
#include <linux/module.h>
#include <linux/rtnetlink.h>

#include "aml_defs.h"
#include "aml_tx.h"
#include "hal_desc.h"
#include "aml_cfg.h"
#include "aml_dini.h"
#include "reg_access.h"
#include "aml_compat.h"

#ifdef CONFIG_AML_SOFTMAC
#define COMMON_PARAM(name, default_softmac, default_fullmac)    \
    .name = default_softmac,
#define SOFTMAC_PARAM(name, default) .name = default,
#define FULLMAC_PARAM(name, default)
#else
#define COMMON_PARAM(name, default_softmac, default_fullmac)    \
    .name = default_fullmac,
#define SOFTMAC_PARAM(name, default)
#define FULLMAC_PARAM(name, default) .name = default,
#endif /* CONFIG_AML_SOFTMAC */

struct aml_mod_params aml_mod_params = {
    /* common parameters */
    COMMON_PARAM(ht_on, true, true)
    COMMON_PARAM(vht_on, true, true)
    COMMON_PARAM(he_on, true, true)
    COMMON_PARAM(mcs_map, IEEE80211_VHT_MCS_SUPPORT_0_9, IEEE80211_VHT_MCS_SUPPORT_0_9)
    COMMON_PARAM(he_mcs_map, IEEE80211_HE_MCS_SUPPORT_0_11, IEEE80211_HE_MCS_SUPPORT_0_11)
    COMMON_PARAM(he_ul_on, false, false)
    COMMON_PARAM(ldpc_on, true, true)
    COMMON_PARAM(stbc_on, true, true)
    COMMON_PARAM(gf_rx_on, true, true)
    COMMON_PARAM(phy_cfg, 0, 0)
    COMMON_PARAM(uapsd_timeout, 300, 300)
    COMMON_PARAM(ap_uapsd_on, true, true)
    COMMON_PARAM(sgi, true, true)
    COMMON_PARAM(sgi80, true, true)
    COMMON_PARAM(use_2040, 1, 1)
    COMMON_PARAM(nss, 2, 2)
    COMMON_PARAM(amsdu_rx_max, 2, 2)
    COMMON_PARAM(bfmee, true, true)
    COMMON_PARAM(bfmer, true, true)
    COMMON_PARAM(mesh, true, true)
    COMMON_PARAM(murx, true, true)
    COMMON_PARAM(mutx, true, true)
    COMMON_PARAM(mutx_on, true, true)
    COMMON_PARAM(use_80, true, true)
    COMMON_PARAM(custregd, false, false)
    COMMON_PARAM(custchan, false, false)
    COMMON_PARAM(roc_dur_max, 500, 500)
    COMMON_PARAM(listen_itv, 0, 0)
    COMMON_PARAM(listen_bcmc, true, true)
    COMMON_PARAM(lp_clk_ppm, 20, 20)
    COMMON_PARAM(ps_on, true, true)
    COMMON_PARAM(tx_lft, AML_TX_LIFETIME_MS, AML_TX_LIFETIME_MS)
    COMMON_PARAM(amsdu_maxnb, NX_TX_PAYLOAD_MAX, NX_TX_PAYLOAD_MAX)
    // By default, only enable UAPSD for Voice queue (see IEEE80211_DEFAULT_UAPSD_QUEUE comment)
//#ifdef CONFIG_AML_POWER_SAVE_MODE
#if 1
    COMMON_PARAM(uapsd_queues, 0, 0)
#else
    COMMON_PARAM(uapsd_queues, IEEE80211_WMM_IE_STA_QOSINFO_AC_VO, IEEE80211_WMM_IE_STA_QOSINFO_AC_VO)
#endif
    COMMON_PARAM(tdls, true, true)
    COMMON_PARAM(uf, true, true)
    COMMON_PARAM(ftl, "", "")
    COMMON_PARAM(dpsm, true, true)
    COMMON_PARAM(tx_to_bk, 0, 0)
    COMMON_PARAM(tx_to_be, 0, 0)
    COMMON_PARAM(tx_to_vi, 0, 0)
    COMMON_PARAM(tx_to_vo, 0, 0)
    COMMON_PARAM(amsdu_tx, 0, 0)

    /* SOFTMAC only parameters */
    SOFTMAC_PARAM(mfp_on, false)
    SOFTMAC_PARAM(gf_on, false)
    SOFTMAC_PARAM(bwsig_on, true)
    SOFTMAC_PARAM(dynbw_on, true)
    SOFTMAC_PARAM(agg_tx, true)
    SOFTMAC_PARAM(rc_probes_on, false)

    /* FULLMAC only parameters */
    FULLMAC_PARAM(ant_div, false)
};

#ifdef CONFIG_AML_SOFTMAC
/* SOFTMAC specific parameters */
module_param_named(mfp_on, aml_mod_params.mfp_on, bool, S_IRUGO);
MODULE_PARM_DESC(mfp_on, "Enable MFP (11w) (Default: 0)");

module_param_named(gf_on, aml_mod_params.gf_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gf_on, "Try TXing Green Field if peer supports it (Default: 0)");

module_param_named(bwsig_on, aml_mod_params.bwsig_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bwsig_on, "Enable bandwidth signaling (VHT tx) (Default: 1)");

module_param_named(dynbw_on, aml_mod_params.dynbw_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dynbw_on, "Enable dynamic bandwidth (VHT tx) (Default: 1)");

module_param_named(agg_tx, aml_mod_params.agg_tx, bool, S_IRUGO);
MODULE_PARM_DESC(agg_tx, "Use A-MPDU in TX (Default: 1)");

module_param_named(rc_probes_on, aml_mod_params.rc_probes_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rc_probes_on, "IEEE80211_TX_CTL_RATE_CTRL_PROBE is 1st in AMPDU (Default: 0)");

#else
/* FULLMAC specific parameters*/
module_param_named(ant_div, aml_mod_params.ant_div, bool, S_IRUGO);
MODULE_PARM_DESC(ant_div, "Enable Antenna Diversity (Default: 0)");
#endif /* CONFIG_AML_SOFTMAC */

module_param_named(amsdu_tx, aml_mod_params.amsdu_tx, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(amsdu_tx, "Use A-MSDU in TX: 0-if advertised, 1-yes, 2-no (Default: 0)");

module_param_named(ht_on, aml_mod_params.ht_on, bool, S_IRUGO);
MODULE_PARM_DESC(ht_on, "Enable HT (Default: 1)");

module_param_named(vht_on, aml_mod_params.vht_on, bool, S_IRUGO);
MODULE_PARM_DESC(vht_on, "Enable VHT (Default: 1)");

module_param_named(he_on, aml_mod_params.he_on, bool, S_IRUGO);
MODULE_PARM_DESC(he_on, "Enable HE (Default: 1)");

module_param_named(mcs_map, aml_mod_params.mcs_map, int, S_IRUGO);
MODULE_PARM_DESC(mcs_map,  "VHT MCS map value  0: MCS0_7, 1: MCS0_8, 2: MCS0_9"
                 " (Default: 2)");

module_param_named(he_mcs_map, aml_mod_params.he_mcs_map, int, S_IRUGO);
MODULE_PARM_DESC(he_mcs_map,  "HE MCS map value  0: MCS0_7, 1: MCS0_9, 2: MCS0_11"
                 " (Default: 2)");

module_param_named(he_ul_on, aml_mod_params.he_ul_on, bool, S_IRUGO);
MODULE_PARM_DESC(he_ul_on, "Enable HE OFDMA UL (Default: 0)");

module_param_named(amsdu_maxnb, aml_mod_params.amsdu_maxnb, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(amsdu_maxnb, "Maximum number of MSDUs inside an A-MSDU in TX: (Default: NX_TX_PAYLOAD_MAX)");

module_param_named(ps_on, aml_mod_params.ps_on, bool, S_IRUGO);
MODULE_PARM_DESC(ps_on, "Enable PowerSaving (Default: 1-Enabled)");

module_param_named(tx_lft, aml_mod_params.tx_lft, int, 0644);
MODULE_PARM_DESC(tx_lft, "Tx lifetime (ms) - setting it to 0 disables retries "
                 "(Default: "__stringify(AML_TX_LIFETIME_MS)")");

module_param_named(ldpc_on, aml_mod_params.ldpc_on, bool, S_IRUGO);
MODULE_PARM_DESC(ldpc_on, "Enable LDPC (Default: 1)");

module_param_named(stbc_on, aml_mod_params.stbc_on, bool, S_IRUGO);
MODULE_PARM_DESC(stbc_on, "Enable STBC in RX (Default: 1)");

module_param_named(gf_rx_on, aml_mod_params.gf_rx_on, bool, S_IRUGO);
MODULE_PARM_DESC(gf_rx_on, "Enable HT greenfield in reception (Default: 1)");

module_param_named(phycfg, aml_mod_params.phy_cfg, int, S_IRUGO);
MODULE_PARM_DESC(phycfg, "Main RF Path (Default: 0)");

module_param_named(uapsd_timeout, aml_mod_params.uapsd_timeout, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uapsd_timeout,
                 "UAPSD Timer timeout, in ms (Default: 300). If 0, UAPSD is disabled");

module_param_named(uapsd_queues, aml_mod_params.uapsd_queues, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uapsd_queues, "UAPSD Queues, integer value, must be seen as a bitfield\n"
                 "        Bit 0 = VO\n"
                 "        Bit 1 = VI\n"
                 "        Bit 2 = BK\n"
                 "        Bit 3 = BE\n"
                 "     -> uapsd_queues=7 will enable uapsd for VO, VI and BK queues");

module_param_named(ap_uapsd_on, aml_mod_params.ap_uapsd_on, bool, S_IRUGO);
MODULE_PARM_DESC(ap_uapsd_on, "Enable UAPSD in AP mode (Default: 1)");

module_param_named(sgi, aml_mod_params.sgi, bool, S_IRUGO);
MODULE_PARM_DESC(sgi, "Advertise Short Guard Interval support (Default: 1)");

module_param_named(sgi80, aml_mod_params.sgi80, bool, S_IRUGO);
MODULE_PARM_DESC(sgi80, "Advertise Short Guard Interval support for 80MHz (Default: 1)");

module_param_named(use_2040, aml_mod_params.use_2040, bool, S_IRUGO);
MODULE_PARM_DESC(use_2040, "Enable 40MHz (Default: 1)");

module_param_named(use_80, aml_mod_params.use_80, bool, S_IRUGO);
MODULE_PARM_DESC(use_80, "Enable 80MHz (Default: 1)");

module_param_named(custregd, aml_mod_params.custregd, bool, S_IRUGO);
MODULE_PARM_DESC(custregd,
                 "Use permissive custom regulatory rules (for testing ONLY) (Default: 0)");

module_param_named(custchan, aml_mod_params.custchan, bool, S_IRUGO);
MODULE_PARM_DESC(custchan,
                 "Extend channel set to non-standard channels (for testing ONLY) (Default: 0)");

module_param_named(nss, aml_mod_params.nss, int, S_IRUGO);
MODULE_PARM_DESC(nss, "1 <= nss <= 2 : Supported number of Spatial Streams (Default: 2)");

module_param_named(amsdu_rx_max, aml_mod_params.amsdu_rx_max, int, S_IRUGO);
MODULE_PARM_DESC(amsdu_rx_max, "0 <= amsdu_rx_max <= 2 : Maximum A-MSDU size supported in RX\n"
                 "        0: 3895 bytes\n"
                 "        1: 7991 bytes\n"
                 "        2: 11454 bytes\n"
                 "        This value might be reduced according to the FW capabilities.\n"
                 "        Default: 2");

module_param_named(bfmee, aml_mod_params.bfmee, bool, S_IRUGO);
MODULE_PARM_DESC(bfmee, "Enable Beamformee Capability (Default: 1-Enabled)");

module_param_named(bfmer, aml_mod_params.bfmer, bool, S_IRUGO);
MODULE_PARM_DESC(bfmer, "Enable Beamformer Capability (Default: 1-Enabled)");

module_param_named(mesh, aml_mod_params.mesh, bool, S_IRUGO);
MODULE_PARM_DESC(mesh, "Enable Meshing Capability (Default: 1-Enabled)");

module_param_named(murx, aml_mod_params.murx, bool, S_IRUGO);
MODULE_PARM_DESC(murx, "Enable MU-MIMO RX Capability (Default: 1-Enabled)");

module_param_named(mutx, aml_mod_params.mutx, bool, S_IRUGO);
MODULE_PARM_DESC(mutx, "Enable MU-MIMO TX Capability (Default: 1-Enabled)");

module_param_named(mutx_on, aml_mod_params.mutx_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mutx_on, "Enable MU-MIMO transmissions (Default: 1-Enabled)");

module_param_named(roc_dur_max, aml_mod_params.roc_dur_max, int, S_IRUGO);
MODULE_PARM_DESC(roc_dur_max, "Maximum Remain on Channel duration");

module_param_named(listen_itv, aml_mod_params.listen_itv, int, S_IRUGO);
MODULE_PARM_DESC(listen_itv, "Maximum listen interval");

module_param_named(listen_bcmc, aml_mod_params.listen_bcmc, bool, S_IRUGO);
MODULE_PARM_DESC(listen_bcmc, "Wait for BC/MC traffic following DTIM beacon");

module_param_named(lp_clk_ppm, aml_mod_params.lp_clk_ppm, int, S_IRUGO);
MODULE_PARM_DESC(lp_clk_ppm, "Low Power Clock accuracy of the local device");

module_param_named(tdls, aml_mod_params.tdls, bool, S_IRUGO);
MODULE_PARM_DESC(tdls, "Enable TDLS (Default: 1-Enabled)");

module_param_named(uf, aml_mod_params.uf, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uf, "Enable Unsupported HT Frame Logging (Default: 1-Enabled)");

module_param_named(ftl, aml_mod_params.ftl, charp, S_IRUGO);
MODULE_PARM_DESC(ftl, "Firmware trace level  (Default: \"\")");

module_param_named(dpsm, aml_mod_params.dpsm, bool, S_IRUGO);
MODULE_PARM_DESC(dpsm, "Enable Dynamic PowerSaving (Default: 1-Enabled)");

module_param_named(tx_to_bk, aml_mod_params.tx_to_bk, int, S_IRUGO);
MODULE_PARM_DESC(tx_to_bk,
     "TX timeout for BK, in ms (Default: 0, Max: 65535). If 0, default value is applied");

module_param_named(tx_to_be, aml_mod_params.tx_to_be, int, S_IRUGO);
MODULE_PARM_DESC(tx_to_be,
     "TX timeout for BE, in ms (Default: 0, Max: 65535). If 0, default value is applied");

module_param_named(tx_to_vi, aml_mod_params.tx_to_vi, int, S_IRUGO);
MODULE_PARM_DESC(tx_to_vi,
     "TX timeout for VI, in ms (Default: 0, Max: 65535). If 0, default value is applied");

module_param_named(tx_to_vo, aml_mod_params.tx_to_vo, int, S_IRUGO);
MODULE_PARM_DESC(tx_to_vo,
     "TX timeout for VO, in ms (Default: 0, Max: 65535). If 0, default value is applied");

/* Regulatory rules */
static struct ieee80211_regdomain aml_regdom = {
    .n_reg_rules = 2,
    .alpha2 = "99",
    .reg_rules = {
        REG_RULE(2390 - 10, 2510 + 10, 40, 0, 1000, 0),
        REG_RULE(5150 - 10, 5970 + 10, 80, 0, 1000, 0),
    }
};

static const int mcs_map_to_rate[4][3] = {
    [PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_7] = 65,
    [PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_8] = 78,
    [PHY_CHNL_BW_20][IEEE80211_VHT_MCS_SUPPORT_0_9] = 78,
    [PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_7] = 135,
    [PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_8] = 162,
    [PHY_CHNL_BW_40][IEEE80211_VHT_MCS_SUPPORT_0_9] = 180,
    [PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_7] = 292,
    [PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_8] = 351,
    [PHY_CHNL_BW_80][IEEE80211_VHT_MCS_SUPPORT_0_9] = 390,
    [PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_7] = 585,
    [PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_8] = 702,
    [PHY_CHNL_BW_160][IEEE80211_VHT_MCS_SUPPORT_0_9] = 780,
};

#define MAX_VHT_RATE(map, nss, bw) (mcs_map_to_rate[bw][map] * (nss))

/**
 * Do some sanity check
 *
 */
static int aml_check_fw_hw_feature(struct aml_hw *aml_hw,
                                    struct wiphy *wiphy)
{
    u32_l sys_feat = aml_hw->version_cfm.features;
    u32_l mac_feat = aml_hw->version_cfm.version_machw_1;
    u32_l phy_feat = aml_hw->version_cfm.version_phy_1;
    u32_l phy_vers = aml_hw->version_cfm.version_phy_2;
    u16_l max_sta_nb = aml_hw->version_cfm.max_sta_nb;
    u8_l max_vif_nb = aml_hw->version_cfm.max_vif_nb;
    int bw, res = 0;
    int amsdu_rx;

    if (!aml_hw->mod_params->custregd)
        aml_hw->mod_params->custchan = false;

    if (aml_hw->mod_params->custchan) {
        aml_hw->mod_params->mesh = false;
        aml_hw->mod_params->tdls = false;
    }

#ifdef CONFIG_AML_SOFTMAC
    if (sys_feat & BIT(MM_FEAT_UMAC_BIT)) {
        wiphy_err(wiphy, "Loading fullmac firmware with softmac driver\n");
        res = -1;
    }

    /* AMPDU (non)support implies different shared structure definition
       so insure that fw and drv have consistent compilation option */
    if (sys_feat & BIT(MM_FEAT_AMPDU_BIT)) {
#ifndef CONFIG_AML_AGG_TX
        wiphy_err(wiphy,
                  "AMPDU enabled in firmware but support not compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_AGG_TX */
    } else {
#ifdef CONFIG_AML_AGG_TX
        wiphy_err(wiphy,
                  "AMPDU disabled in firmware but support compiled in driver\n");
        res = -1;
#else
        aml_hw->mod_params->agg_tx = false;
#endif /* CONFIG_AML_AGG_TX */
    }
#else /* check for FULLMAC */

    if (!(sys_feat & BIT(MM_FEAT_UMAC_BIT))) {
        wiphy_err(wiphy,
                  "Loading softmac firmware with fullmac driver\n");
        res = -1;
    }

    if (!(sys_feat & BIT(MM_FEAT_ANT_DIV_BIT))) {
        aml_hw->mod_params->ant_div = false;
    }

#endif /* CONFIG_AML_SOFTMAC */

    if (!(sys_feat & BIT(MM_FEAT_VHT_BIT))) {
        aml_hw->mod_params->vht_on = false;
    }

    // Check if HE is supported
    if (!(sys_feat & BIT(MM_FEAT_HE_BIT))) {
        aml_hw->mod_params->he_on = false;
        aml_hw->mod_params->he_ul_on = false;
    }

    if (!(sys_feat & BIT(MM_FEAT_PS_BIT))) {
        aml_hw->mod_params->ps_on = false;
    }

    /* AMSDU (non)support implies different shared structure definition
       so insure that fw and drv have consistent compilation option */
    if (sys_feat & BIT(MM_FEAT_AMSDU_BIT)) {
#ifndef CONFIG_AML_SPLIT_TX_BUF
        wiphy_err(wiphy,
                  "AMSDU enabled in firmware but support not compiled in driver\n");
        res = -1;
#else
        /* Adjust amsdu_maxnb so that it stays in allowed bounds */
        aml_adjust_amsdu_maxnb(aml_hw);
#endif /* CONFIG_AML_SPLIT_TX_BUF */
    } else {
#ifdef CONFIG_AML_SPLIT_TX_BUF
        wiphy_err(wiphy,
                  "AMSDU disabled in firmware but support compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_SPLIT_TX_BUF */
    }

    if (!(sys_feat & BIT(MM_FEAT_UAPSD_BIT))) {
        aml_hw->mod_params->uapsd_timeout = 0;
    }

    if (!(sys_feat & BIT(MM_FEAT_BFMEE_BIT))) {
        aml_hw->mod_params->bfmee = false;
    }

    if ((sys_feat & BIT(MM_FEAT_BFMER_BIT))) {
#ifndef CONFIG_AML_BFMER
        wiphy_err(wiphy,
                  "BFMER enabled in firmware but support not compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_BFMER */
        // Check PHY and MAC HW BFMER support and update parameter accordingly
        if (!(phy_feat & MDM_BFMER_BIT) || !(mac_feat & NXMAC_BFMER_BIT)) {
            aml_hw->mod_params->bfmer = false;
            // Disable the feature in the bitfield so that it won't be displayed
            sys_feat &= ~BIT(MM_FEAT_BFMER_BIT);
        }
    } else {
#ifdef CONFIG_AML_BFMER
        wiphy_err(wiphy,
                  "BFMER disabled in firmware but support compiled in driver\n");
        res = -1;
#else
        aml_hw->mod_params->bfmer = false;
#endif /* CONFIG_AML_BFMER */
    }

    if (!(sys_feat & BIT(MM_FEAT_MESH_BIT))) {
        aml_hw->mod_params->mesh = false;
    }

    if (!(sys_feat & BIT(MM_FEAT_TDLS_BIT))) {
        aml_hw->mod_params->tdls = false;
    }

    if (!(sys_feat & BIT(MM_FEAT_UF_BIT))) {
        aml_hw->mod_params->uf = false;
    }

#ifdef CONFIG_AML_FULLMAC
    if ((sys_feat & BIT(MM_FEAT_MON_DATA_BIT))) {
#ifndef CONFIG_AML_MON_DATA
        wiphy_err(wiphy,
                  "Monitor+Data interface support (MON_DATA) is enabled in firmware but support not compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_MON_DATA */
    } else {
#ifdef CONFIG_AML_MON_DATA
        wiphy_err(wiphy,
                  "Monitor+Data interface support (MON_DATA) disabled in firmware but support compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_MON_DATA */
    }
#endif

    // Check supported AMSDU RX size
    amsdu_rx = (sys_feat >> MM_AMSDU_MAX_SIZE_BIT0) & 0x03;
    if (amsdu_rx < aml_hw->mod_params->amsdu_rx_max) {
        aml_hw->mod_params->amsdu_rx_max = amsdu_rx;
    }

    // Check supported BW
    bw = (phy_feat & MDM_CHBW_MASK) >> MDM_CHBW_LSB;
    // Check if 80MHz BW is supported
    if (bw < 2) {
        aml_hw->mod_params->use_80 = false;
    }
    // Check if 40MHz BW is supported
    if (bw < 1)
        aml_hw->mod_params->use_2040 = false;

    // 80MHz BW shall be disabled if 40MHz is not enabled
    if (!aml_hw->mod_params->use_2040)
        aml_hw->mod_params->use_80 = false;

    // Check if HT is supposed to be supported. If not, disable VHT/HE too
    if (!aml_hw->mod_params->ht_on)
    {
        aml_hw->mod_params->vht_on = false;
        aml_hw->mod_params->he_on = false;
        aml_hw->mod_params->he_ul_on = false;
        aml_hw->mod_params->use_80 = false;
        aml_hw->mod_params->use_2040 = false;
    }

    // LDPC is mandatory for HE40 and above, so if LDPC is not supported, then disable
    // support for 40 and 80MHz
    if (aml_hw->mod_params->he_on && !aml_hw->mod_params->ldpc_on)
    {
        aml_hw->mod_params->use_80 = false;
        aml_hw->mod_params->use_2040 = false;
    }

    // HT greenfield is not supported in modem >= 3.0
    if (__MDM_MAJOR_VERSION(phy_vers) > 0) {
#ifdef CONFIG_AML_SOFTMAC
        aml_hw->mod_params->gf_on = false;
#endif
        aml_hw->mod_params->gf_rx_on = false;
    }

    if (!(sys_feat & BIT(MM_FEAT_MU_MIMO_RX_BIT)) ||
        !aml_hw->mod_params->bfmee) {
        aml_hw->mod_params->murx = false;
    }

    if ((sys_feat & BIT(MM_FEAT_MU_MIMO_TX_BIT))) {
#ifndef CONFIG_AML_MUMIMO_TX
        wiphy_err(wiphy,
                  "MU-MIMO TX enabled in firmware but support not compiled in driver\n");
        res = -1;
#endif /* CONFIG_AML_MUMIMO_TX */
        if (!aml_hw->mod_params->bfmer)
            aml_hw->mod_params->mutx = false;
        // Check PHY and MAC HW MU-MIMO TX support and update parameter accordingly
        else if (!(phy_feat & MDM_MUMIMOTX_BIT) || !(mac_feat & NXMAC_MU_MIMO_TX_BIT)) {
                aml_hw->mod_params->mutx = false;
                // Disable the feature in the bitfield so that it won't be displayed
                sys_feat &= ~BIT(MM_FEAT_MU_MIMO_TX_BIT);
        }
    } else {
#ifdef CONFIG_AML_MUMIMO_TX
        wiphy_err(wiphy,
                  "MU-MIMO TX disabled in firmware but support compiled in driver\n");
        res = -1;
#else
        aml_hw->mod_params->mutx = false;
#endif /* CONFIG_AML_MUMIMO_TX */
    }

    if (sys_feat & BIT(MM_FEAT_WAPI_BIT)) {
        aml_enable_wapi(aml_hw);
    }

#ifdef CONFIG_AML_FULLMAC
    if (sys_feat & BIT(MM_FEAT_MFP_BIT)) {
        aml_enable_mfp(aml_hw);
    }

    if (mac_feat & NXMAC_GCMP_BIT) {
        aml_enable_gcmp(aml_hw);
    }
#endif

#ifdef CONFIG_AML_SOFTMAC
#define QUEUE_NAME "BEACON queue "
#else
#define QUEUE_NAME "Broadcast/Multicast queue "
#endif /* CONFIG_AML_SOFTMAC */

    if (sys_feat & BIT(MM_FEAT_BCN_BIT)) {
#if NX_TXQ_CNT == 4
        wiphy_err(wiphy, QUEUE_NAME
                  "enabled in firmware but support not compiled in driver\n");
        res = -1;
#endif /* NX_TXQ_CNT == 4 */
    } else {
#if NX_TXQ_CNT == 5
        wiphy_err(wiphy, QUEUE_NAME
                  "disabled in firmware but support compiled in driver\n");
        res = -1;
#endif /* NX_TXQ_CNT == 5 */
    }
#undef QUEUE_NAME

#ifdef CONFIG_AML_RADAR
    if (sys_feat & BIT(MM_FEAT_RADAR_BIT)) {
        /* Enable combination with radar detection */
        wiphy->n_iface_combinations++;
    }
#endif /* CONFIG_AML_RADAR */

#ifndef CONFIG_AML_SDM
    switch (__MDM_PHYCFG_FROM_VERS(phy_feat)) {
        case MDM_PHY_CONFIG_TRIDENT:
            aml_hw->mod_params->nss = 1;
            if ((aml_hw->mod_params->phy_cfg < 0) || (aml_hw->mod_params->phy_cfg > 2))
                aml_hw->mod_params->phy_cfg = 2;
            break;
        case MDM_PHY_CONFIG_KARST:
        case MDM_PHY_CONFIG_CATAXIA:
            {
                int nss_supp = (phy_feat & MDM_NSS_MASK) >> MDM_NSS_LSB;
                if (aml_hw->mod_params->nss > nss_supp)
                    aml_hw->mod_params->nss = nss_supp;
                if ((aml_hw->mod_params->phy_cfg < 0) || (aml_hw->mod_params->phy_cfg > 1))
                    aml_hw->mod_params->phy_cfg = 0;
            }
            break;
        default:
            WARN_ON(1);
            break;
    }
#endif /* CONFIG_AML_SDM */

    if ((aml_hw->mod_params->nss < 1) || (aml_hw->mod_params->nss > 2))
        aml_hw->mod_params->nss = 1;

    if ((aml_hw->mod_params->mcs_map < 0) || (aml_hw->mod_params->mcs_map > 2))
        aml_hw->mod_params->mcs_map = 0;

#define PRINT_AML_PHY_FEAT(feat)                                   \
    (phy_feat & MDM_##feat##_BIT ? "["#feat"]" : "")

    wiphy_info(wiphy, "PHY features: [NSS=%d][CHBW=%d]%s%s%s%s%s%s%s\n",
               (phy_feat & MDM_NSS_MASK) >> MDM_NSS_LSB,
               20 * (1 << ((phy_feat & MDM_CHBW_MASK) >> MDM_CHBW_LSB)),
               (phy_feat & (MDM_LDPCDEC_BIT | MDM_LDPCENC_BIT)) ==
                       (MDM_LDPCDEC_BIT | MDM_LDPCENC_BIT) ? "[LDPC]" : "",
               PRINT_AML_PHY_FEAT(VHT),
               PRINT_AML_PHY_FEAT(HE),
               PRINT_AML_PHY_FEAT(BFMER),
               PRINT_AML_PHY_FEAT(BFMEE),
               PRINT_AML_PHY_FEAT(MUMIMOTX),
               PRINT_AML_PHY_FEAT(MUMIMORX)
               );

#define PRINT_AML_FEAT(feat)                                   \
    (sys_feat & BIT(MM_FEAT_##feat##_BIT) ? "["#feat"]" : "")

    wiphy_info(wiphy, "FW features: %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
               PRINT_AML_FEAT(BCN),
               PRINT_AML_FEAT(RADAR),
               PRINT_AML_FEAT(PS),
               PRINT_AML_FEAT(UAPSD),
               PRINT_AML_FEAT(AMPDU),
               PRINT_AML_FEAT(AMSDU),
               PRINT_AML_FEAT(P2P),
               PRINT_AML_FEAT(P2P_GO),
               PRINT_AML_FEAT(UMAC),
               PRINT_AML_FEAT(VHT),
               PRINT_AML_FEAT(HE),
               PRINT_AML_FEAT(BFMEE),
               PRINT_AML_FEAT(BFMER),
               PRINT_AML_FEAT(WAPI),
               PRINT_AML_FEAT(MFP),
               PRINT_AML_FEAT(MU_MIMO_RX),
               PRINT_AML_FEAT(MU_MIMO_TX),
               PRINT_AML_FEAT(MESH),
               PRINT_AML_FEAT(TDLS),
               PRINT_AML_FEAT(ANT_DIV),
               PRINT_AML_FEAT(UF),
               PRINT_AML_FEAT(TWT),
               PRINT_AML_FEAT(FTM_INIT),
               PRINT_AML_FEAT(FAKE_FTM_RSP));
#undef PRINT_AML_FEAT

    if(max_sta_nb != NX_REMOTE_STA_MAX)
    {
        wiphy_err(wiphy, "Different number of supported stations between driver and FW (%d != %d)\n",
                  NX_REMOTE_STA_MAX, max_sta_nb);
        res = -1;
    }

    if(max_vif_nb != NX_VIRT_DEV_MAX)
    {
        wiphy_err(wiphy, "Different number of supported virtual interfaces between driver and FW (%d != %d)\n",
                  NX_VIRT_DEV_MAX, max_vif_nb);
        res = -1;
    }

    return res;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
static void aml_set_ppe_threshold(struct aml_hw *aml_hw,
                                   struct ieee80211_sta_he_cap *he_cap)
{
    const u8_l PPE_THRES_INFO_OFT = 7;
    const u8_l PPE_THRES_INFO_BIT_LEN = 6;
    struct ppe_thres_info_tag
    {
        u8_l ppet16 : 3;
        u8_l ppet8 : 3;
    }__packed;

    struct ppe_thres_field_tag
    {
        u8_l nsts : 3;
        u8_l ru_idx_bmp : 4;
    };
    int nss = aml_hw->mod_params->nss;
    struct ppe_thres_field_tag* ppe_thres_field = (struct ppe_thres_field_tag*) he_cap->ppe_thres;
    struct ppe_thres_info_tag ppe_thres_info = {.ppet16 = 0, //BSPK
                                                .ppet8 = 7 //None
                                               };
    u8_l* ppe_thres_info_ptr = (u8_l*) &ppe_thres_info;
    u16_l* ppe_thres_ptr = NULL;
    u8_l  j, cnt, offset;
    int i = 0;

    if (aml_hw->mod_params->use_80)
    {
        ppe_thres_field->ru_idx_bmp = 7;
        cnt = 3;
    }
    else if (aml_hw->mod_params->use_2040)
    {
        ppe_thres_field->ru_idx_bmp = 3;
        cnt = 2;
    }
    else
    {
        ppe_thres_field->ru_idx_bmp = 1;
        cnt = 1;
    }
    ppe_thres_field->nsts = nss - 1;
    for (i = 0; i < nss ; i++)
    {
        for (j = 0; j < cnt; j++){
            offset = (i * cnt + j) * PPE_THRES_INFO_BIT_LEN + PPE_THRES_INFO_OFT;
            ppe_thres_ptr = (u16_l*)&he_cap->ppe_thres[offset / 8];
            *ppe_thres_ptr |= *ppe_thres_info_ptr << (offset % 8);
        }
    }
}
#endif // LINUX_VERSION_CODE >= 4.20

#ifdef CONFIG_AML_SOFTMAC
static void aml_set_softmac_flags(struct aml_hw *aml_hw)
{
    struct ieee80211_hw *hw = aml_hw->hw;
    int nss;

#ifdef CONFIG_MAC80211_AMSDUS_TX
    ieee80211_hw_set(hw, TX_AMSDU);
    ieee80211_hw_set(hw, TX_FRAG_LIST);
    hw->max_tx_fragments = aml_hw->mod_params->amsdu_maxnb;
#endif

    if (aml_hw->mod_params->agg_tx)
        ieee80211_hw_set(hw, AMPDU_AGGREGATION);

    if (aml_hw->mod_params->ps_on) {
        ieee80211_hw_set(hw, SUPPORTS_PS);
    }
    /* To disable the dynamic PS we say to the stack that we support it in
     * HW. This will force mac80211 rely on us to handle this. */
    ieee80211_hw_set(hw, SUPPORTS_DYNAMIC_PS);

    if (aml_hw->mod_params->mfp_on)
        ieee80211_hw_set(hw, MFP_CAPABLE);

    nss = aml_hw->mod_params->nss;
    aml_hw->phy.ctrlinfo_1.value = 0;
    aml_hw->phy.ctrlinfo_2.value = 0;
    if (nss == 1) {
        aml_hw->phy.ctrlinfo_2.antennaSet = 1;
    } else {
        aml_hw->phy.ctrlinfo_1.fecCoding = 0;
        aml_hw->phy.ctrlinfo_1.nTx = 1;
        aml_hw->phy.ctrlinfo_2.antennaSet = 3;
        aml_hw->phy.ctrlinfo_2.smmIndex = 1;
    }
    aml_hw->phy.stbc_nss = nss >> 1;
}
#endif // CONFIG_AML_SOFTMAC

void aml_set_vht_capa(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
    struct ieee80211_supported_band *band_5GHz = wiphy->bands[NL80211_BAND_5GHZ];
    int i;
    int nss = aml_hw->mod_params->nss;
    int mcs_map;
    int mcs_map_max;
    int mcs_map_max_2ss_rx = IEEE80211_VHT_MCS_SUPPORT_0_9;
    int mcs_map_max_2ss_tx = IEEE80211_VHT_MCS_SUPPORT_0_9;
    int bw_max;

    if (!aml_hw->mod_params->vht_on)
        return;

    band_5GHz->vht_cap.vht_supported = true;
    if (aml_hw->mod_params->sgi80)
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_SHORT_GI_80;
    if (aml_hw->mod_params->stbc_on) {
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_RXSTBC_1;
    } else {
        band_5GHz->vht_cap.cap &= ~IEEE80211_VHT_CAP_RXSTBC_1;
    }
    if (aml_hw->mod_params->ldpc_on)
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_RXLDPC;
    if (aml_hw->mod_params->bfmee) {
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE;
        band_5GHz->vht_cap.cap |= 3 << IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT;
    }
    if (nss > 1)
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_TXSTBC;

    // Update the AMSDU max RX size (not shifted as located at offset 0 of the VHT cap)
    band_5GHz->vht_cap.cap |= aml_hw->mod_params->amsdu_rx_max;

    if (aml_hw->mod_params->bfmer) {
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE;
        /* Set number of sounding dimensions */
        band_5GHz->vht_cap.cap |= (nss - 1) << IEEE80211_VHT_CAP_SOUNDING_DIMENSIONS_SHIFT;
    }
    if (aml_hw->mod_params->murx)
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE;
    if (aml_hw->mod_params->mutx)
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE;

    // Get max supported BW
    if (aml_hw->mod_params->use_80) {
        bw_max = PHY_CHNL_BW_80;
        mcs_map_max_2ss_rx = IEEE80211_VHT_MCS_SUPPORT_0_9;
        mcs_map_max_2ss_tx = IEEE80211_VHT_MCS_SUPPORT_0_9;
    } else if (aml_hw->mod_params->use_2040)
        bw_max = PHY_CHNL_BW_40;
    else
        bw_max = PHY_CHNL_BW_20;

    // Check if MCS map should be limited to MCS0_8 due to the standard. Indeed in BW20,
    // MCS9 is not supported in 1 and 2 SS
    if (aml_hw->mod_params->use_2040)
        mcs_map_max = IEEE80211_VHT_MCS_SUPPORT_0_9;
    else
        mcs_map_max = IEEE80211_VHT_MCS_SUPPORT_0_8;

    mcs_map = min_t(int, aml_hw->mod_params->mcs_map, mcs_map_max);
    band_5GHz->vht_cap.vht_mcs.rx_mcs_map = cpu_to_le16(0);
    for (i = 0; i < nss; i++) {
        band_5GHz->vht_cap.vht_mcs.rx_mcs_map |= cpu_to_le16(mcs_map << (i*2));
        band_5GHz->vht_cap.vht_mcs.rx_highest = MAX_VHT_RATE(mcs_map, nss, bw_max);
        mcs_map = min_t(int, mcs_map, mcs_map_max_2ss_rx);
    }
    for (; i < 8; i++) {
        band_5GHz->vht_cap.vht_mcs.rx_mcs_map |= cpu_to_le16(
            IEEE80211_VHT_MCS_NOT_SUPPORTED << (i*2));
    }

    mcs_map = min_t(int, aml_hw->mod_params->mcs_map, mcs_map_max);
    band_5GHz->vht_cap.vht_mcs.tx_mcs_map = cpu_to_le16(0);
    for (i = 0; i < nss; i++) {
        band_5GHz->vht_cap.vht_mcs.tx_mcs_map |= cpu_to_le16(mcs_map << (i*2));
        band_5GHz->vht_cap.vht_mcs.tx_highest = MAX_VHT_RATE(mcs_map, nss, bw_max);
        mcs_map = min_t(int, mcs_map, mcs_map_max_2ss_tx);
    }
    for (; i < 8; i++) {
        band_5GHz->vht_cap.vht_mcs.tx_mcs_map |= cpu_to_le16(
            IEEE80211_VHT_MCS_NOT_SUPPORTED << (i*2));
    }

    if (!aml_hw->mod_params->use_80) {
#ifdef CONFIG_VENDOR_AML_VHT_NO80
        band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_NOT_SUP_WIDTH_80;
#endif
        band_5GHz->vht_cap.cap &= ~IEEE80211_VHT_CAP_SHORT_GI_80;
    }
}

void aml_set_ht_capa(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
    struct ieee80211_supported_band *band_5GHz = wiphy->bands[NL80211_BAND_5GHZ];
    struct ieee80211_supported_band *band_2GHz = wiphy->bands[NL80211_BAND_2GHZ];
    int i;
    int nss = aml_hw->mod_params->nss;

    if (!aml_hw->mod_params->ht_on) {
        band_2GHz->ht_cap.ht_supported = false;
        band_5GHz->ht_cap.ht_supported = false;
        return;
    }

    if (aml_hw->mod_params->stbc_on) {
        band_2GHz->ht_cap.cap |= 1 << IEEE80211_HT_CAP_RX_STBC_SHIFT;
    } else {
        band_2GHz->ht_cap.cap &= ~(1 << IEEE80211_HT_CAP_RX_STBC_SHIFT);
    }
    if (aml_hw->mod_params->ldpc_on)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_LDPC_CODING;
    if (aml_hw->mod_params->use_2040) {
        band_2GHz->ht_cap.mcs.rx_mask[4] = 0x1; /* MCS32 */
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(135 * nss);
    } else {
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(65 * nss);
    }
    if (nss > 1)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_TX_STBC;

    // Update the AMSDU max RX size
    if (aml_hw->mod_params->amsdu_rx_max)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_MAX_AMSDU;

    if (aml_hw->mod_params->sgi) {
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
        if (aml_hw->mod_params->use_2040) {
            band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(150 * nss);
        } else
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(72 * nss);
    }
    if (aml_hw->mod_params->gf_rx_on)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_GRN_FLD;

    for (i = 0; i < nss; i++) {
        band_2GHz->ht_cap.mcs.rx_mask[i] = 0xFF;
    }

    band_5GHz->ht_cap = band_2GHz->ht_cap;
}

void aml_set_he_capa(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    struct ieee80211_supported_band *band_5GHz = wiphy->bands[NL80211_BAND_5GHZ];
    struct ieee80211_supported_band *band_2GHz = wiphy->bands[NL80211_BAND_2GHZ];
    int i;
    int nss = aml_hw->mod_params->nss;
    struct ieee80211_sta_he_cap *he_cap;
    int mcs_map, mcs_map_max_2ss = IEEE80211_HE_MCS_SUPPORT_0_11;
    u8 dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_242;
    u32_l phy_vers = aml_hw->version_cfm.version_phy_2;

    if (!aml_hw->mod_params->he_on) {
        band_2GHz->iftype_data = NULL;
        band_2GHz->n_iftype_data = 0;
        band_5GHz->iftype_data = NULL;
        band_5GHz->n_iftype_data = 0;
        return;
    }

    he_cap = (struct ieee80211_sta_he_cap *) &band_2GHz->iftype_data[0].he_cap;
    he_cap->has_he = true;

    he_cap->he_cap_elem.mac_cap_info[2] |= IEEE80211_HE_MAC_CAP2_ALL_ACK;
    aml_set_ppe_threshold(aml_hw, he_cap);
    if (aml_hw->mod_params->use_2040) {
        he_cap->he_cap_elem.phy_cap_info[0] |=
                        IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G;
        dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_484;
    }
    if (aml_hw->mod_params->use_80) {
        he_cap->he_cap_elem.phy_cap_info[0] |=
                        IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G;
        mcs_map_max_2ss = IEEE80211_HE_MCS_SUPPORT_0_11;
        dcm_max_ru = IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_996;
    }
    if (aml_hw->mod_params->ldpc_on) {
        he_cap->he_cap_elem.phy_cap_info[1] |= IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD;
    } else {
        // If no LDPC is supported, we have to limit to MCS0_9, as LDPC is mandatory
        // for MCS 10 and 11
        aml_hw->mod_params->he_mcs_map = min_t(int, aml_hw->mod_params->he_mcs_map,
                                                IEEE80211_HE_MCS_SUPPORT_0_9);
    }
    he_cap->he_cap_elem.phy_cap_info[1] |= IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US |
                                           IEEE80211_HE_PHY_CAP1_MIDAMBLE_RX_TX_MAX_NSTS;
    he_cap->he_cap_elem.phy_cap_info[2] |= IEEE80211_HE_PHY_CAP2_MIDAMBLE_RX_TX_MAX_NSTS |
                                           IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US |
                                           IEEE80211_HE_PHY_CAP2_DOPPLER_RX;
    if (aml_hw->mod_params->stbc_on) {
        he_cap->he_cap_elem.phy_cap_info[2] |= IEEE80211_HE_PHY_CAP2_STBC_RX_UNDER_80MHZ;
    } else {
        he_cap->he_cap_elem.phy_cap_info[2] &= ~IEEE80211_HE_PHY_CAP2_STBC_RX_UNDER_80MHZ;
    }
    he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_CONST_RX_16_QAM |
                                           IEEE80211_HE_PHY_CAP3_RX_HE_MU_PPDU_FROM_NON_AP_STA;
    if (nss > 0) {
        he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_2;
    } else {
        he_cap->he_cap_elem.phy_cap_info[3] |= IEEE80211_HE_PHY_CAP3_DCM_MAX_RX_NSS_1;
    }

    if (aml_hw->mod_params->bfmee) {
        he_cap->he_cap_elem.phy_cap_info[4] |= IEEE80211_HE_PHY_CAP4_SU_BEAMFORMEE;
        he_cap->he_cap_elem.phy_cap_info[4] |=
                     IEEE80211_HE_PHY_CAP4_BEAMFORMEE_MAX_STS_UNDER_80MHZ_4;
    }
    he_cap->he_cap_elem.phy_cap_info[5] |= IEEE80211_HE_PHY_CAP5_NG16_SU_FEEDBACK |
                                           IEEE80211_HE_PHY_CAP5_NG16_MU_FEEDBACK;
    he_cap->he_cap_elem.phy_cap_info[6] |= IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_42_SU |
                                           IEEE80211_HE_PHY_CAP6_CODEBOOK_SIZE_75_MU |
                                           IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMER_FB |
                                           IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMER_FB |
                                           IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT |
                                           IEEE80211_HE_PHY_CAP6_PARTIAL_BANDWIDTH_DL_MUMIMO;
    he_cap->he_cap_elem.phy_cap_info[7] |= IEEE80211_HE_PHY_CAP7_HE_SU_MU_PPDU_4XLTF_AND_08_US_GI;
    he_cap->he_cap_elem.phy_cap_info[8] |= IEEE80211_HE_PHY_CAP8_20MHZ_IN_40MHZ_HE_PPDU_IN_2G |
                                           dcm_max_ru;
    he_cap->he_cap_elem.phy_cap_info[9] |= IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_COMP_SIGB |
                                           IEEE80211_HE_PHY_CAP9_RX_FULL_BW_SU_USING_MU_WITH_NON_COMP_SIGB |
                                           #if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
                                           IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_16US;
                                           #else
                                           IEEE80211_HE_PHY_CAP9_NOMINAL_PKT_PADDING_16US;
                                           #endif

    // Starting from version v31 more HE_ER_SU modulations is supported
    if (__MDM_VERSION(phy_vers) > 30) {

        he_cap->he_cap_elem.phy_cap_info[6] |= IEEE80211_HE_PHY_CAP6_PARTIAL_BW_EXT_RANGE;
        he_cap->he_cap_elem.phy_cap_info[8] |= IEEE80211_HE_PHY_CAP8_HE_ER_SU_1XLTF_AND_08_US_GI |
                                               IEEE80211_HE_PHY_CAP8_HE_ER_SU_PPDU_4XLTF_AND_08_US_GI;
    }

    mcs_map = aml_hw->mod_params->he_mcs_map;
    memset(&he_cap->he_mcs_nss_supp, 0, sizeof(he_cap->he_mcs_nss_supp));
    for (i = 0; i < nss; i++) {
        __le16 unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i*2));
        he_cap->he_mcs_nss_supp.rx_mcs_80 |= cpu_to_le16(mcs_map << (i*2));
        he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
        mcs_map = min_t(int, aml_hw->mod_params->he_mcs_map,
                        mcs_map_max_2ss);
    }
    for (; i < 8; i++) {
        __le16 unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i*2));
        he_cap->he_mcs_nss_supp.rx_mcs_80 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.rx_mcs_80p80 |= unsup_for_ss;
    }
    mcs_map = aml_hw->mod_params->he_mcs_map;
    for (i = 0; i < nss; i++) {
        __le16 unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i*2));
        he_cap->he_mcs_nss_supp.tx_mcs_80 |= cpu_to_le16(mcs_map << (i*2));
        he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
        mcs_map = min_t(int, aml_hw->mod_params->he_mcs_map,
                        mcs_map_max_2ss);
    }
    for (; i < 8; i++) {
        __le16 unsup_for_ss = cpu_to_le16(IEEE80211_HE_MCS_NOT_SUPPORTED << (i*2));
        he_cap->he_mcs_nss_supp.tx_mcs_80 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_160 |= unsup_for_ss;
        he_cap->he_mcs_nss_supp.tx_mcs_80p80 |= unsup_for_ss;
    }

    for (i = 0; i < band_2GHz->n_iftype_data; i++) {
        memcpy((void *)&band_2GHz->iftype_data[i].he_cap, he_cap, sizeof(*he_cap));
    }

#ifdef CONFIG_AML_FULLMAC
    // HE capabilities only for STA interfaces
    if (aml_hw->version_cfm.features & BIT(MM_FEAT_TWT_BIT)) {
        aml_hw->ext_capa[9] = WLAN_EXT_CAPA10_TWT_REQUESTER_SUPPORT;
        he_cap->he_cap_elem.mac_cap_info[0] |= IEEE80211_HE_MAC_CAP0_TWT_REQ;
    }
#endif // CONFIG_AML_FULLMAC

    // HE capabilities only for AP interfaces
    he_cap = (struct ieee80211_sta_he_cap *)&band_2GHz->iftype_data[1].he_cap;
#ifdef CONFIG_AML_FULLMAC
#ifdef CONFIG_AML_WFA_CERT_MODE
    /* WFA CERT:
     * Disabled by default, should enable it when doing WFA test.
     * Cause the WFA brcm98 STA will inspect these two fileds to determine
     * whether carry HE capabilities IE in ASSOC request frame or not.
     * */
    he_cap->he_cap_elem.mac_cap_info[0] |= IEEE80211_HE_MAC_CAP0_HTC_HE;
    he_cap->he_cap_elem.mac_cap_info[3] |= IEEE80211_HE_MAC_CAP3_OMI_CONTROL;
#endif // CONFIG_AML_WFA_CERT_MODE
    if (aml_hw->version_cfm.features & BIT(MM_FEAT_TWT_BIT)) {
        aml_hw->ext_capa[9] |= WLAN_EXT_CAPA10_TWT_RESPONDER_SUPPORT;
        he_cap->he_cap_elem.mac_cap_info[0] |= IEEE80211_HE_MAC_CAP0_TWT_RES;
    }
#endif // CONFIG_AML_FULLMAC

#endif // LINUX >= 4.20.0
}

static void aml_set_wiphy_params(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
    if (aml_hw->mod_params->tdls) {
        /* TDLS support */
        wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
        wiphy->features |= NL80211_FEATURE_TDLS_CHANNEL_SWITCH;
#ifdef CONFIG_AML_FULLMAC
        /* TDLS external setup support */
        wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
#endif
    }

    if (aml_hw->mod_params->ap_uapsd_on)
        wiphy->flags |= WIPHY_FLAG_AP_UAPSD;

#ifdef CONFIG_AML_FULLMAC
    if (aml_hw->mod_params->ps_on)
        wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
    else
        wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
    if (aml_hw->version_cfm.features & BIT(MM_FEAT_FAKE_FTM_RSP_BIT))
        wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_ENABLE_FTM_RESPONDER);
#endif
#endif

    if (aml_hw->mod_params->custregd) {
        // Check if custom channel set shall be enabled. In such case only monitor mode is
        // supported
        if (aml_hw->mod_params->custchan) {
            wiphy->interface_modes = BIT(NL80211_IFTYPE_MONITOR);

            // Enable "extra" channels
            wiphy->bands[NL80211_BAND_2GHZ]->n_channels += 13;
            wiphy->bands[NL80211_BAND_5GHZ]->n_channels += 59;
        }
    }
}

static void aml_set_rf_params(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
#ifndef CONFIG_AML_SDM
    struct ieee80211_supported_band *band_5GHz = wiphy->bands[NL80211_BAND_5GHZ];
    u32 mdm_phy_cfg = __MDM_PHYCFG_FROM_VERS(aml_hw->version_cfm.version_phy_1);
    struct aml_cfg_phy phy_conf;

    /*
     * Get configuration file depending on the RF
     */
    if (mdm_phy_cfg == MDM_PHY_CONFIG_TRIDENT) {
        // Retrieve the Trident configuration
        aml_cfg_parse_phy(aml_hw, AML_PHY_CONFIG_TRD_NAME,
                                  &phy_conf, aml_hw->mod_params->phy_cfg);
        memcpy(&aml_hw->phy.cfg, &phy_conf.trd, sizeof(phy_conf.trd));
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_CATAXIA) {
        memset(&phy_conf.cataxia, 0, sizeof(phy_conf.cataxia));
        phy_conf.cataxia.path_used = aml_hw->mod_params->phy_cfg;
        memcpy(&aml_hw->phy.cfg, &phy_conf.cataxia, sizeof(phy_conf.cataxia));
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_KARST) {
        // We use the NSS parameter as is
        // Retrieve the Karst configuration
        //aml_cfg_parse_phy(aml_hw, AML_PHY_CONFIG_KARST_NAME,
        //                          &phy_conf, aml_hw->mod_params->phy_cfg);

        memcpy(&aml_hw->phy.cfg, &phy_conf.karst, sizeof(phy_conf.karst));
    } else {
        WARN_ON(1);
    }

    /*
     * adjust caps depending on the RF
     */
    switch (mdm_phy_cfg) {
        case MDM_PHY_CONFIG_TRIDENT:
        {
            wiphy_dbg(wiphy, "found Trident PHY .. limit BW to 40MHz\n");
            aml_hw->phy.limit_bw = true;
#ifdef CONFIG_VENDOR_AML_VHT_NO80
            band_5GHz->vht_cap.cap |= IEEE80211_VHT_CAP_NOT_SUP_WIDTH_80;
#endif
            band_5GHz->vht_cap.cap &= ~(IEEE80211_VHT_CAP_SHORT_GI_80 |
                                        IEEE80211_VHT_CAP_RXSTBC_MASK);
            break;
        }
        case MDM_PHY_CONFIG_CATAXIA:
        {
            wiphy_dbg(wiphy, "found CATAXIA PHY\n");
            break;
        }
        case MDM_PHY_CONFIG_KARST:
        {
            wiphy_dbg(wiphy, "found KARST PHY\n");
            break;
        }
        default:
            WARN_ON(1);
            break;
    }
#endif /* CONFIG_AML_SDM */
}

int aml_handle_dynparams(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
    int ret;

    /* Check compatibility between requested parameters and HW/SW features */
    ret = aml_check_fw_hw_feature(aml_hw, wiphy);
    if (ret)
        return ret;

#ifdef CONFIG_AML_SOFTMAC
    /* SOFTMAC specific parameters*/
    aml_set_softmac_flags(aml_hw);
#endif /* CONFIG_AML_SOFTMAC */

    /* Set wiphy parameters */
    aml_set_wiphy_params(aml_hw, wiphy);

    /* Set VHT capabilities */
    aml_set_vht_capa(aml_hw, wiphy);

    /* Set HE capabilities */
    aml_set_he_capa(aml_hw, wiphy);

    /* Set HT capabilities */
    aml_set_ht_capa(aml_hw, wiphy);

    /* Set RF specific parameters (shall be done last as it might change some
       capabilities previously set) */
    aml_set_rf_params(aml_hw, wiphy);

    return 0;
}

void aml_custregd(struct aml_hw *aml_hw, struct wiphy *wiphy)
{
    if (!aml_hw->mod_params->custregd)
        return;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 3, 12)
    wiphy->regulatory_flags |= REGULATORY_IGNORE_STALE_KICKOFF;
#else
    wiphy->regulatory_flags |= (REGULATORY_WIPHY_SELF_MANAGED >> 1);
#endif
    wiphy->regulatory_flags |= REGULATORY_WIPHY_SELF_MANAGED;

    rtnl_lock();
    wiphy_lock(wiphy);
    if (regulatory_set_wiphy_regd_sync(wiphy, &aml_regdom))
        wiphy_err(wiphy, "Failed to set custom regdomain\n");
    else
        wiphy_err(wiphy,"\n"
                  "*******************************************************\n"
                  "** CAUTION: USING PERMISSIVE CUSTOM REGULATORY RULES **\n"
                  "*******************************************************\n");
    wiphy_unlock(wiphy);
    rtnl_unlock();
}

void aml_adjust_amsdu_maxnb(struct aml_hw *aml_hw)
{
    if (aml_hw->mod_params->amsdu_maxnb > NX_TX_PAYLOAD_MAX)
        aml_hw->mod_params->amsdu_maxnb = NX_TX_PAYLOAD_MAX;
    else if (aml_hw->mod_params->amsdu_maxnb == 0)
        aml_hw->mod_params->amsdu_maxnb = 1;
}
