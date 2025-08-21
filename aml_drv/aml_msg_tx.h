/**
 ****************************************************************************************
 *
 * @file aml_msg_tx.h
 *
 * @brief TX function declarations
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#ifndef _AML_MSG_TX_H_
#define _AML_MSG_TX_H_

#include "aml_defs.h"
#include "aml_mdns_offload.h"

extern unsigned char g_pci_shutdown;
extern unsigned char g_pci_msg_suspend;
int aml_send_reset(struct aml_hw *aml_hw);
int aml_send_start(struct aml_hw *aml_hw);
int aml_send_version_req(struct aml_hw *aml_hw, struct mm_version_cfm *cfm);
int aml_send_add_if(struct aml_hw *aml_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm);
int aml_send_remove_if(struct aml_hw *aml_hw, u8 vif_index);
int aml_send_set_channel(struct aml_hw *aml_hw, int phy_idx,
                          struct mm_set_channel_cfm *cfm);
int aml_send_key_add(struct aml_hw *aml_hw, u8 vif_idx, u8 sta_idx, bool pairwise,
                      u8 *key, u8 key_len, u8 key_idx, u8 cipher_suite,
                      struct mm_key_add_cfm *cfm);
int aml_send_key_del(struct aml_hw *aml_hw, uint8_t hw_key_idx);
int aml_send_bcn_change(struct aml_hw *aml_hw, u8 vif_idx, u32 bcn_addr,
                         u16 bcn_len, u16 tim_oft, u16 tim_len, u16 *csa_oft);
int aml_send_tim_update(struct aml_hw *aml_hw, u8 vif_idx, u16 aid,
                         u8 tx_status);
int aml_send_roc(struct aml_hw *aml_hw, struct aml_vif *vif,
                  struct ieee80211_channel *chan, unsigned int duration);
int aml_send_cancel_roc(struct aml_hw *aml_hw);
int aml_send_set_power(struct aml_hw *aml_hw,  u8 vif_idx, s8 pwr,
                        struct mm_set_power_cfm *cfm);
int aml_send_set_edca(struct aml_hw *aml_hw, u8 hw_queue, u32 param,
                       bool uapsd, u8 inst_nbr);
int aml_send_tdls_chan_switch_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                                   struct aml_sta *aml_sta, bool sta_initiator,
                                   u8 oper_class, struct cfg80211_chan_def *chandef,
                                   struct tdls_chan_switch_cfm *cfm);
int aml_send_tdls_cancel_chan_switch_req(struct aml_hw *aml_hw,
                                          struct aml_vif *aml_vif,
                                          struct aml_sta *aml_sta,
                                          struct tdls_cancel_chan_switch_cfm *cfm);

#ifdef CONFIG_AML_P2P_DEBUGFS
int aml_send_p2p_oppps_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                            u8 ctw, struct mm_set_p2p_oppps_cfm *cfm);
int aml_send_p2p_noa_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                          int count, int interval, int duration,
                          bool dyn_noa, struct mm_set_p2p_noa_cfm *cfm);
#endif /* CONFIG_AML_P2P_DEBUGFS */

#ifdef CONFIG_AML_SOFTMAC
int aml_send_sta_add(struct aml_hw *aml_hw, struct ieee80211_sta *sta,
                      u8 inst_nbr, struct mm_sta_add_cfm *cfm);
int aml_send_sta_del(struct aml_hw *aml_hw, u8 sta_idx);
int aml_send_set_filter(struct aml_hw *aml_hw, uint32_t filter);
int aml_send_add_chanctx(struct aml_hw *aml_hw,
                          struct ieee80211_chanctx_conf *ctx,
                          struct mm_chan_ctxt_add_cfm *cfm);
int aml_send_del_chanctx(struct aml_hw *aml_hw, u8 index);
int aml_send_link_chanctx(struct aml_hw *aml_hw, u8 vif_idx, u8 chan_idx,
                           u8 chan_switch);
int aml_send_unlink_chanctx(struct aml_hw *aml_hw, u8 vif_idx);
int aml_send_update_chanctx(struct aml_hw *aml_hw,
                             struct ieee80211_chanctx_conf *ctx);
int aml_send_sched_chanctx(struct aml_hw *aml_hw, u8 vif_idx, u8 chan_idx,
                            u8 type);

int aml_send_dtim_req(struct aml_hw *aml_hw, u8 dtim_period);
int aml_send_set_br(struct aml_hw *aml_hw, u32 basic_rates, u8 vif_idx, u8 band);
int aml_send_set_beacon_int(struct aml_hw *aml_hw, u16 beacon_int, u8 vif_idx);
int aml_send_set_bssid(struct aml_hw *aml_hw, const u8 *bssid, u8 vif_idx);
int aml_send_set_vif_state(struct aml_hw *aml_hw, bool active,
                            u16 aid, u8 vif_idx);
int aml_send_set_mode(struct aml_hw *aml_hw, u8 abgmode);
int aml_send_set_idle(struct aml_hw *aml_hw, int idle);
int aml_send_set_ps_mode(struct aml_hw *aml_hw, u8 ps_mode);
int aml_send_set_ps_options(struct aml_hw *aml_hw, bool listen_bcmc,
                             u16 listen_interval, u8 vif_idx);
int aml_send_set_slottime(struct aml_hw *aml_hw, int use_short_slot);
int aml_send_ba_add(struct aml_hw *aml_hw, uint8_t type, uint8_t sta_idx,
                     u16 tid, uint8_t bufsz, uint16_t ssn,
                     struct mm_ba_add_cfm *cfm);
int aml_send_ba_del(struct aml_hw *aml_hw, uint8_t sta_idx, u16 tid,
                     struct mm_ba_del_cfm *cfm);
int aml_send_scan_req(struct aml_hw *aml_hw, struct ieee80211_vif *vif,
                       struct cfg80211_scan_request *param,
                       struct scan_start_cfm *cfm);
int aml_send_scan_cancel_req(struct aml_hw *aml_hw,
                              struct scan_cancel_cfm *cfm);
void aml_send_tdls_ps(struct aml_hw *aml_hw, bool ps_mode);
#endif /* CONFIG_AML_SOFTMAC */

#ifdef CONFIG_AML_FULLMAC
int aml_send_me_config_req(struct aml_hw *aml_hw);
int aml_send_me_chan_config_req(struct aml_hw *aml_hw);
int aml_send_me_set_control_port_req(struct aml_hw *aml_hw, bool opened,
                                      u8 sta_idx);
int aml_send_me_sta_add(struct aml_hw *aml_hw, struct station_parameters *params,
                         const u8 *mac, u8 inst_nbr, struct me_sta_add_cfm *cfm);
int aml_send_me_sta_del(struct aml_hw *aml_hw, u8 sta_idx, bool tdls_sta);
int aml_send_me_traffic_ind(struct aml_hw *aml_hw, u8 sta_idx, bool uapsd, u8 tx_status);
int aml_send_twt_request(struct aml_hw *aml_hw,
                          u8 setup_type, u8 vif_idx,
                          struct twt_conf_tag *conf,
                          struct twt_setup_cfm *cfm);
int _aml_send_twt_teardown(struct aml_hw *aml_hw,
                           struct twt_teardown_req *twt_teardown,
                           struct twt_teardown_cfm *cfm);
int aml_send_me_rc_stats(struct aml_hw *aml_hw, u8 sta_idx,
                          struct me_rc_stats_cfm *cfm);
int aml_send_me_rc_set_rate(struct aml_hw *aml_hw,
                             u8 sta_idx,
                             u16 rate_idx);
int aml_send_me_set_ps_mode(struct aml_hw *aml_hw, u8 ps_mode);
int aml_send_sm_connect_req(struct aml_hw *aml_hw,
                             struct aml_vif *aml_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm);
int aml_send_sm_disconnect_req(struct aml_hw *aml_hw,
                                struct aml_vif *aml_vif,
                                u16 reason);
int aml_send_sm_external_auth_required_rsp(struct aml_hw *aml_hw,
                                            struct aml_vif *aml_vif,
                                            u16 status);
int aml_send_sm_ft_auth_rsp(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                             uint8_t *ie, int ie_len);
int aml_send_apm_start_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm);
int aml_send_apm_stop_req(struct aml_hw *aml_hw, struct aml_vif *vif);
int aml_send_apm_probe_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct aml_sta *sta, struct apm_probe_client_cfm *cfm);
int aml_send_scanu_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                        struct cfg80211_scan_request *param);
int aml_send_apm_start_cac_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm);
int aml_send_apm_stop_cac_req(struct aml_hw *aml_hw, struct aml_vif *vif);
int aml_send_tdls_peer_traffic_ind_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif);
int aml_send_config_monitor_req(struct aml_hw *aml_hw,
                                 struct cfg80211_chan_def *chandef,
                                 struct me_config_monitor_cfm *cfm);
int aml_send_mesh_start_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                             const struct mesh_config *conf, const struct mesh_setup *setup,
                             struct mesh_start_cfm *cfm);
int aml_send_mesh_stop_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct mesh_stop_cfm *cfm);
int aml_send_mesh_update_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                              u32 mask, const struct mesh_config *p_mconf, struct mesh_update_cfm *cfm);
int aml_send_mesh_peer_info_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                                 u8 sta_idx, struct mesh_peer_info_cfm *cfm);
void aml_send_mesh_peer_update_ntf(struct aml_hw *aml_hw, struct aml_vif *vif,
                                    u8 sta_idx, u8 mlink_state);
void aml_send_mesh_path_create_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 *tgt_addr);
int aml_send_mesh_path_update_req(struct aml_hw *aml_hw, struct aml_vif *vif, const u8 *tgt_addr,
                                   const u8 *p_nhop_addr, struct mesh_path_update_cfm *cfm);
void aml_send_mesh_proxy_add_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 *ext_addr);
#endif /* CONFIG_AML_FULLMAC */

#ifdef CONFIG_AML_BFMER
#ifdef CONFIG_AML_SOFTMAC
void aml_send_bfmer_enable(struct aml_hw *aml_hw, struct ieee80211_sta *sta);
#else
void aml_send_bfmer_enable(struct aml_hw *aml_hw, struct aml_sta *aml_sta,
                            const struct ieee80211_vht_cap *vht_cap);
#endif /* CONFIG_AML_SOFTMAC*/
#ifdef CONFIG_AML_MUMIMO_TX
int aml_send_mu_group_update_req(struct aml_hw *aml_hw, struct aml_sta *aml_sta);
#endif /* CONFIG_AML_MUMIMO_TX */
#endif /* CONFIG_AML_BFMER */

/* Debug messages */
int aml_send_dbg_trigger_req(struct aml_hw *aml_hw, char *msg);
int aml_send_dbg_mem_read_req(struct aml_hw *aml_hw, u32 mem_addr,
                               struct dbg_mem_read_cfm *cfm);
int aml_send_dbg_mem_write_req(struct aml_hw *aml_hw, u32 mem_addr,
                                u32 mem_data);
int aml_send_dbg_set_mod_filter_req(struct aml_hw *aml_hw, u32 filter);
int aml_send_dbg_set_sev_filter_req(struct aml_hw *aml_hw, u32 filter);
int aml_send_dbg_get_sys_stat_req(struct aml_hw *aml_hw,
                                   struct dbg_get_sys_stat_cfm *cfm);
int aml_send_cfg_rssi_req(struct aml_hw *aml_hw, u8 vif_index, int rssi_thold, u32 rssi_hyst);
#ifdef TEST_MODE
int aml_pcie_prssr_test(struct net_device *dev, int start_addr, int len, u32_l payload);
int aml_pcie_dl_malloc_test(struct aml_hw *aml_hw, int start_addr, int len, u32_l payload);
int aml_pcie_ul_malloc_test(struct aml_hw *aml_hw, int start_addr, int len, u32_l payload);
#endif
int aml_rf_reg_write(struct net_device *dev, int addr, int value);
int aml_rf_reg_read(struct net_device *dev, int addr);
int aml_csi_status_sp_read(struct net_device *dev, int csi_mode, int csi_time_interval, struct csi_status_sp_get_ind *ind);
int aml_csi_status_com_read(struct net_device *dev, struct csi_status_com_get_ind *ind);
unsigned int aml_efuse_read(struct aml_hw *aml_hw, u32 addr);
int aml_scan_hang(struct aml_vif *aml_vif, int scan_hang);
int aml_send_suspend_req(struct aml_hw *aml_hw, u8_l filter, enum wifi_suspend_state state);
int aml_send_wow_pattern (struct aml_hw *aml_hw, struct aml_vif *vif,
                        struct cfg80211_pkt_pattern *param, int id);
int aml_send_scanu_cancel_req(struct aml_hw *aml_hw, struct aml_vif *vif, struct scanu_cancel_cfm *cfm);
int aml_send_arp_agent_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 enable, u32 ipv4, u8 *ipv6);
int aml_set_rekey_data(struct aml_vif *aml_vif, const u8 *kek, const u8 *kck, const u8 *replay_ctr);
int aml_tko_config_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                        u16 interval, u16 retry_interval, u16 retry_count);
int aml_set_cali_param_req(struct aml_hw *aml_hw, struct Cali_Param *cali_param);
int aml_fw_reset(struct aml_vif *aml_vif);
int _aml_get_efuse(struct aml_vif *aml_vif, u32 addr);
int _aml_set_efuse(struct aml_vif *aml_vif, u32 addr, u32 value);
int _aml_set_macbypass(struct aml_vif *aml_vif, int format_type, int bandwidth, int rate, int siso_or_mimo);
int _aml_set_stop_macbypass(struct aml_vif *aml_vif);

int aml_send_sched_scan_req(struct aml_vif *aml_vif,
    struct cfg80211_sched_scan_request *request);
int aml_send_sched_scan_stop_req(struct aml_vif *aml_vif, u64 reqid);
int _aml_set_amsdu_tx(struct aml_hw *aml_hw, u8 amsdu_tx);
int _aml_set_tx_lft(struct aml_hw *aml_hw, u32 tx_lft);
int aml_set_ldpc_tx(struct aml_hw *aml_hw, struct aml_vif *aml_vif);
int _aml_set_stbc(struct aml_hw *aml_hw, u8 vif_idx, u8 stbc_on);
int aml_set_temp_start(struct aml_hw *aml_hw);
int aml_coex_cmd(struct net_device *dev, u32_l coex_cmd, u32_l cmd_ctxt_1, u32_l cmd_ctxt_2);
int aml_tko_activate(struct aml_hw *aml_hw, struct aml_vif *vif, u8 active);
int _aml_set_pt_calibration(struct aml_vif *aml_vif, int pt_cali_val);
int aml_send_notify_ip(struct aml_vif *aml_vif,u8_l ip_ver,u8_l*ip_addr);
int _aml_enable_wf(struct aml_vif *aml_vif, u32 addr);
int aml_send_fwlog_cmd(struct aml_vif *aml_vif, int mode);
int aml_send_scc_conflict_notify(struct aml_vif *ap_vif, u8 sta_vif_idx, struct mm_scc_cfm *scc_cfm);
int aml_send_sync_trace(struct aml_hw *aml_hw);
int aml_send_dhcp_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif, uint8_t work);
int aml_send_extcapab_req(struct aml_hw *aml_hw);

int aml_sync_trace_init(struct aml_hw *aml_hw);
int aml_sync_trace_deinit(struct aml_hw *aml_hw);
int aml_set_limit_power(struct aml_hw *aml_hw, int limit_power_switch);
int aml_txq_unexpection(struct net_device *dev);
void cfg80211_to_aml_chan(const struct cfg80211_chan_def *chandef, struct mac_chan_op *chan);
int aml_send_set_buf_state_req(struct aml_hw *aml_hw, int buf_state);
int _aml_set_la_capture(struct aml_vif *aml_vif, u32 bus1, u32 bus2);
int _aml_fix_txpwr(struct aml_vif *aml_vif, int pwr);
int _aml_set_usb_trace_enable(struct aml_hw *aml_hw, int value);
int _aml_putv_trace_switch(struct aml_hw *aml_hw, int value);
int _aml_set_la_enable(struct aml_hw *aml_hw, int value);
int aml_set_rssi_reg(struct aml_vif *aml_vif, int flag);
int aml_mdns_set_offload_state(struct aml_hw *aml_hw, int enable);
int aml_mdns_reset_all(struct aml_hw *aml_hw);
int aml_coex_get_status(struct net_device *dev);
int aml_mdns_add_protocol_data_status(struct aml_hw *aml_hw, void *list_param, mdnsProtocolData *offloadData, int *index);
int aml_mdns_add_protocol_data(struct aml_hw *aml_hw, uint8_t *raw_data, uint8_t index, uint16_t data_len);
int aml_mdns_remove_protocol_data(struct aml_hw *aml_hw, int index);
int aml_mdns_get_reset_hit_counter(struct aml_hw *aml_hw, int index);
int aml_mdns_get_reset_miss_counter(struct aml_hw *aml_hw);
int aml_mdns_add_passthrough_list(struct aml_hw *aml_hw, uint8_t *qname, int length);
int aml_mdns_remove_passthrough_list(struct aml_hw *aml_hw, uint8_t *qname, int length);
int aml_mdns_set_passthrough_behavior(struct aml_hw *aml_hw, int behavior);

#endif /* _AML_MSG_TX_H_ */
