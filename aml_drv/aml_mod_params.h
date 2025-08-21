/**
 ******************************************************************************
 *
 * @file aml_mod_params.h
 *
 * @brief Declaration of module parameters
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_MOD_PARAM_H_
#define _AML_MOD_PARAM_H_

struct aml_mod_params {
    bool ht_on;
    bool vht_on;
    bool he_on;
    int mcs_map;
    int he_mcs_map;
    bool he_ul_on;
    bool ldpc_on;
    bool stbc_on;
    bool gf_rx_on;
    int phy_cfg;
    int uapsd_timeout;
    bool ap_uapsd_on;
    bool sgi;
    bool sgi80;
    bool use_2040;
    bool use_80;
    bool custregd;
    bool custchan;
    int nss;
    int amsdu_rx_max;
    bool bfmee;
    bool bfmer;
    bool mesh;
    bool murx;
    bool mutx;
    bool mutx_on;
    int roc_dur_max;
    int listen_itv;
    bool listen_bcmc;
    int lp_clk_ppm;
    bool ps_on;
    int tx_lft;
    int amsdu_maxnb;
    int uapsd_queues;
    bool tdls;
    bool uf;
    char *ftl;
    bool dpsm;
    int tx_to_bk;
    int tx_to_be;
    int tx_to_vi;
    int tx_to_vo;
    int amsdu_tx;
#ifdef CONFIG_AML_SOFTMAC
    bool mfp_on;
    bool gf_on;
    bool bwsig_on;
    bool dynbw_on;
    bool agg_tx;
    bool rc_probes_on;
#else
    bool ant_div;
#endif /* CONFIG_AML_SOFTMAC */
};

extern struct aml_mod_params aml_mod_params;

struct aml_hw;
struct wiphy;

int aml_handle_dynparams(struct aml_hw *aml_hw, struct wiphy *wiphy);
void aml_custregd(struct aml_hw *aml_hw, struct wiphy *wiphy);
void aml_enable_wapi(struct aml_hw *aml_hw);
void aml_enable_mfp(struct aml_hw *aml_hw);
void aml_enable_gcmp(struct aml_hw *aml_hw);
void aml_adjust_amsdu_maxnb(struct aml_hw *aml_hw);
void aml_set_vht_capa(struct aml_hw *aml_hw, struct wiphy *wiphy);
void aml_set_ht_capa(struct aml_hw *aml_hw, struct wiphy *wiphy);
void aml_set_he_capa(struct aml_hw *aml_hw, struct wiphy *wiphy);

#endif /* _AML_MOD_PARAM_H_ */
