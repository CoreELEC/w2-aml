/**
 ******************************************************************************
 *
 * @file aml_main.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_MAIN_H_
#define _AML_MAIN_H_

#include "aml_defs.h"
#include "aml_tx.h"
#include "aml_sap.h"

int aml_cfg80211_init(struct aml_plat *aml_plat, void **platform_data);
void aml_cfg80211_deinit(struct aml_hw *aml_hw);
int aml_cfg80211_change_iface(struct wiphy *wiphy,
        struct net_device *dev, enum nl80211_iftype type, struct vif_params *params);
void aml_get_version(void);
void aml_cfg80211_sched_scan_results(struct wiphy *wiphy, uint64_t reqid);
int aml_cancel_scan(struct aml_hw *aml_hw, struct aml_vif *vif);
void aml_tx_rx_buf_init(struct aml_hw *aml_hw);
int aml_cfg80211_start_ap(struct wiphy *wiphy,
        struct net_device *dev, struct cfg80211_ap_settings *settings);
int aml_cfg80211_del_station(struct wiphy *wiphy,
        struct net_device *dev, struct station_del_parameters *params);
int aml_config_cali_param(struct aml_hw *aml_hw);
void aml_set_scan_hang(struct aml_vif *aml_vif, int scan_hang, u8* func, u32 line);
u32 aml_pci_readl(u8* addr);
void aml_pci_writel(u32 data, u8* addr);
int aml_ps_wow_resume(struct aml_hw *aml_hw);

#endif /* _AML_MAIN_H_ */
