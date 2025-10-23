/**
 ****************************************************************************************
 *
 * @file aml_msg_rx.c
 *
 * @brief RX function definitions
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#define AML_MODULE                  MSG_RX

#include "aml_defs.h"
#include "aml_prof.h"
#include "aml_tx.h"
#ifdef CONFIG_AML_BFMER
#include "aml_bfmer.h"
#endif //(CONFIG_AML_BFMER)
#ifdef CONFIG_AML_FULLMAC
#include "aml_debugfs.h"
#include "aml_msg_tx.h"
#include "aml_tdls.h"
#endif /* CONFIG_AML_FULLMAC */
#include "aml_events.h"
#include "aml_compat.h"
#include "aml_main.h"
#include <linux/inetdevice.h>
#include <linux/time.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,10,0)
#include <net/addrconf.h>
#endif
#include "aml_defs.h"
#include "aml_recy.h"
#include "aml_interface.h"
#include "aml_msg_rx.h"
#include "aml_scc.h"
#include "aml_recy.h"

extern bool pt_mode;
extern struct aml_pm_type g_wifi_pm;
extern int coex_flag;
extern struct agg_req_t g_agg_parse;

int aml_freq_to_idx(struct aml_hw *aml_hw, int freq)
{
    struct ieee80211_supported_band *sband;
    int band, ch, idx = 0;

    for (band = NL80211_BAND_2GHZ; band < NUM_NL80211_BANDS; band++) {
#ifdef CONFIG_AML_SOFTMAC
        sband = aml_hw->hw->wiphy->bands[band];
#else
        sband = aml_hw->wiphy->bands[band];
#endif /* CONFIG_AML_SOFTMAC */
        if (!sband) {
            continue;
        }

        for (ch = 0; ch < sband->n_channels; ch++, idx++) {
            if (sband->channels[ch].center_freq == freq) {
                goto exit;
            }
        }
    }

    BUG_ON(1);

exit:
    // Channel has been found, return the index
    return idx;
}

/***************************************************************************
 * Messages from MM task
 **************************************************************************/

/*
 * a copy of MM_BA_ADD/DEL_REQ is forwarded by f/w MM task for driver REO
 */

static inline const u8* aml_sta_mac(struct aml_hw *aml_hw, u8 sta_idx)
{
    struct aml_sta *sta = aml_sta_get(aml_hw, sta_idx);
    return sta ? sta->mac_addr : NULL;
}

static inline int aml_rx_mm_ba_add_ind(struct aml_hw *aml_hw,
                                       struct aml_cmd *cmd,
                                       struct ipc_e2a_msg *msg)
{
    struct mm_ba_add_req *req = (struct mm_ba_add_req *)msg->param;

    BUG_ON(cmd);    /* no cmd is associated to */
    BUG_ON(msg->param_len != sizeof(*req));

    AML_M_NOTICE(BA,"ADDBA %s %pM[%d] bufsz %u ssn %u\n",
                 (req->type == BA_AGMT_TX) ? "TX" : "RX",
                 aml_sta_mac(aml_hw, req->sta_idx), req->tid, req->bufsz, req->ssn);

    if (req->type == BA_AGMT_TX) {
        g_agg_parse.def_ampdu_tx = req->bufsz;
        return 0;
    }
    g_agg_parse.ampdu_rx = req->bufsz;

    return aml_reo_session_create(&aml_hw->rx, aml_sta_get(aml_hw, req->sta_idx),
                                  req->tid, req->bufsz, req->ssn);
}

static inline int aml_rx_mm_ba_del_ind(struct aml_hw *aml_hw,
                                       struct aml_cmd *cmd,
                                       struct ipc_e2a_msg *msg)
{
    struct mm_ba_del_req *req = (struct mm_ba_del_req *)msg->param;

    BUG_ON(cmd);    /* no cmd is associated to */
    BUG_ON(msg->param_len != sizeof(*req));

    AML_M_NOTICE(BA, "DELBA %s %pM[%d]\n",
                 (req->type == BA_AGMT_TX) ? "TX" : "RX",
                 aml_sta_mac(aml_hw, req->sta_idx), req->tid);

    if (req->type == BA_AGMT_TX)
        return 0;

    return aml_reo_session_delete(&aml_hw->rx, aml_sta_get(aml_hw, req->sta_idx), req->tid);
}

static int aml_rx_mm_ba_cfm(struct aml_hw *aml_hw, int add, struct ipc_e2a_msg *msg)
{
    struct mm_ba_add_cfm *cfm = (struct mm_ba_add_cfm *)msg->param;

    BUG_ON(msg->param_len != sizeof(*cfm));

    AML_M_NOTICE(BA, "%s -- %pM[%d] status:%d\n", add ? "ADDBA": "DELBA",
                 aml_sta_mac(aml_hw, cfm->sta_idx), cfm->tid, cfm->status);
    return 0;
}

static inline int aml_rx_mm_ba_add_cfm(struct aml_hw *aml_hw,
                                       struct aml_cmd *cmd,
                                       struct ipc_e2a_msg *msg)
{
    BUG_ON(cmd);    /* no cmd is associated to */
    return aml_rx_mm_ba_cfm(aml_hw, 1, msg);
}

static inline int aml_rx_mm_ba_del_cfm(struct aml_hw *aml_hw,
                                       struct aml_cmd *cmd,
                                       struct ipc_e2a_msg *msg)
{
    BUG_ON(cmd);    /* no cmd is associated to */
    BUG_ON(sizeof(struct mm_ba_del_cfm) != sizeof(struct mm_ba_del_cfm));
    return aml_rx_mm_ba_cfm(aml_hw, 0, msg);
}

static inline int aml_rx_chan_pre_switch_ind(struct aml_hw *aml_hw,
                                              struct aml_cmd *cmd,
                                              struct ipc_e2a_msg *msg)
{
#ifdef CONFIG_AML_SOFTMAC
    struct aml_chanctx *chan_ctxt;
#endif
    struct aml_vif *aml_vif;
    int chan_idx = ((struct mm_channel_pre_switch_ind *)msg->param)->chan_index;

    //AML_DBG(AML_FN_ENTRY_STR);

    REG_SW_SET_PROFILING_CHAN(aml_hw, SW_PROF_CHAN_CTXT_PSWTCH_BIT);

#ifdef CONFIG_AML_SOFTMAC
    list_for_each_entry(chan_ctxt, &aml_hw->chan_ctxts, list) {
        if (chan_ctxt->index == chan_idx) {
            chan_ctxt->active = false;
            break;
        }
    }

    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (aml_vif->chanctx && (aml_vif->chanctx->index == chan_idx)) {
            aml_txq_vif_stop(aml_vif, AML_TXQ_STOP_CHAN);
        }
    }
#else
    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (aml_vif->up && aml_vif->ch_index == chan_idx) {
            aml_txq_vif_stop(aml_vif, AML_TXQ_STOP_CHAN);
        }
    }
#endif /* CONFIG_AML_SOFTMAC */

    REG_SW_CLEAR_PROFILING_CHAN(aml_hw, SW_PROF_CHAN_CTXT_PSWTCH_BIT);

    return 0;
}

static inline int aml_rx_chan_switch_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_channel_switch_ind *ind = (void *)msg->param;
    struct aml_vif *aml_vif;

    //AML_INFO("chan_idx:%d, req:%d, tdls:%d", chan_idx, param->roc, param->roc_tdls);

    REG_SW_SET_PROFILING_CHAN(aml_hw, SW_PROF_CHAN_CTXT_SWTCH_BIT);

    if (ind->roc_tdls) {
        aml_vif = aml_vif_get(aml_hw, ind->vif_index);
        if (aml_vif) {
            aml_vif->roc_tdls = true;
            aml_txq_tdls_sta_start(aml_vif, AML_TXQ_STOP_CHAN);
        }
    } else if (!ind->roc) {
        list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
            if (aml_vif->up && aml_vif->ch_index == ind->chan_index) {
                if (aml_hw->show_switch_info > 0) {
                    AML_INFO("txq_start,vif:%d vif_chan:%d chan:%d\n",
                             aml_vif->vif_index, aml_vif->ch_index, ind->chan_index);
                    aml_hw->show_switch_info--;
                }
                aml_txq_vif_start(aml_vif, AML_TXQ_STOP_CHAN);
            }
        }
    } else {
        struct aml_roc *roc;

        spin_lock_bh(&aml_hw->roc_lock);
        roc = aml_hw->roc;
        if (!roc) {
            spin_unlock_bh(&aml_hw->roc_lock);
            return 0;
        }

        aml_vif = roc->vif;
        trace_switch_roc(aml_vif->vif_index);
        if (!roc->internal) {
            // If RoC has been started by the user space, inform it that we have
            // switched on the requested off-channel
            cfg80211_ready_on_channel(&aml_vif->wdev, (u64)(unsigned long)(roc),
                                      roc->chan, roc->duration, GFP_ATOMIC);
        }
        // Keep in mind that we have switched on the channel
        roc->on_chan = true;
        spin_unlock_bh(&aml_hw->roc_lock);

        // Enable traffic on OFF channel queue
        aml_txq_offchan_start(aml_hw);
    }

    aml_hw->cur_chanctx = ind->chan_index;
    aml_radar_detection_enable_on_cur_channel(aml_hw);

    REG_SW_CLEAR_PROFILING_CHAN(aml_hw, SW_PROF_CHAN_CTXT_SWTCH_BIT);

    return 0;
}

static inline int aml_rx_tdls_chan_switch_cfm(struct aml_hw *aml_hw,
                                                struct aml_cmd *cmd,
                                                struct ipc_e2a_msg *msg)
{
    return 0;
}

static inline int aml_rx_tdls_chan_switch_ind(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    // Enable traffic on OFF channel queue
    aml_txq_offchan_start(aml_hw);

    return 0;
}

static inline int aml_rx_tdls_chan_switch_base_ind(struct aml_hw *aml_hw,
                                                    struct aml_cmd *cmd,
                                                    struct ipc_e2a_msg *msg)
{
    struct tdls_chan_switch_base_ind *ind = (void *)msg->param;
    struct aml_vif *aml_vif = aml_vif_get(aml_hw, ind->vif_index);

    AML_DBG(AML_FN_ENTRY_STR);

    if (aml_vif) {
        aml_vif->roc_tdls = false;
        aml_txq_tdls_sta_stop(aml_vif, AML_TXQ_STOP_CHAN);
    }
    return 0;
}

static inline int aml_rx_tdls_peer_ps_ind(struct aml_hw *aml_hw,
                                           struct aml_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    struct tdls_peer_ps_ind *ind = (void *)msg->param;
    struct aml_vif *aml_vif = aml_vif_get(aml_hw, ind->vif_index);

    if (aml_vif) {
        aml_vif->sta.tdls_sta->tdls.ps_on = ind->ps_on;
        // Update PS status for the TDLS station
        aml_ps_bh_enable(aml_hw, aml_vif->sta.tdls_sta, ind->ps_on);
    }

    return 0;
}

static inline int aml_rx_remain_on_channel_exp_ind(struct aml_hw *aml_hw,
                                                    struct aml_cmd *cmd,
                                                    struct ipc_e2a_msg *msg)
{
    struct aml_vif *aml_vif;
    u8 vif_index = ((struct mm_remain_on_channel_exp_ind *)msg->param)->vif_index;

    aml_vif = aml_hw->vif_table[vif_index];
    if (aml_vif)
        aml_txq_offchan_deinit(aml_vif);

    spin_lock_bh(&aml_hw->roc_lock);
    if (!aml_hw->roc) {
        spin_unlock_bh(&aml_hw->roc_lock);
        return 0;
    }

    if ((aml_hw->roc->vif) && (aml_hw->roc->vif != aml_vif)) {
        AML_INFO("err roc vif index not %d\n",vif_index);
        aml_vif = aml_hw->roc->vif;
    }

    aml_tx_cfm_wait_rsp(aml_hw, false, (u8 *)__func__, __LINE__);
    trace_roc_exp(aml_vif->vif_index);

    AML_INFO("roc internal=%d, on_chan=%d cookie:0x%p\n", aml_hw->roc->internal, aml_hw->roc->on_chan, aml_hw->roc);
    if (!aml_hw->roc->internal && aml_hw->roc->on_chan) {
        // If RoC has been started by the user space and hasn't been cancelled,
        // inform it that off-channel period has expired
        cfg80211_remain_on_channel_expired(&aml_vif->wdev, (u64)(unsigned long)(aml_hw->roc),
                                           aml_hw->roc->chan, GFP_ATOMIC);
    }
    kfree(aml_hw->roc);
    aml_hw->roc = NULL;
    spin_unlock_bh(&aml_hw->roc_lock);

    aml_hw->roc_is_canceling = false;
    return 0;
}

static inline int aml_rx_p2p_vif_ps_change_ind(struct aml_hw *aml_hw,
                                                struct aml_cmd *cmd,
                                                struct ipc_e2a_msg *msg)
{
    struct mm_p2p_vif_ps_change_ind *ind = (void *)msg->param;
    struct aml_vif *aml_vif = aml_vif_get(aml_hw, ind->vif_index);

    if (aml_vif) {
        if (ind->ps_state == MM_PS_MODE_OFF)
            aml_txq_vif_start(aml_vif, AML_TXQ_STOP_VIF_PS);
        else
            aml_txq_vif_stop(aml_vif, AML_TXQ_STOP_VIF_PS);
    }
    return 0;
}

static inline int aml_rx_channel_survey_ind(struct aml_hw *aml_hw,
                                             struct aml_cmd *cmd,
                                             struct ipc_e2a_msg *msg)
{
    struct mm_channel_survey_ind *ind = (struct mm_channel_survey_ind *)msg->param;
    // Get the channel index
    int idx = aml_freq_to_idx(aml_hw, ind->freq);
    // Get the survey
    struct aml_survey_info *aml_survey;

    //AML_DBG(AML_FN_ENTRY_STR);

    if (idx >= SCAN_CHANNEL_MAX)
        return 0;

    aml_survey = &aml_hw->survey[idx];

    // Store the received parameters
    aml_survey->chan_time_ms = ind->chan_time_ms;
    aml_survey->chan_time_busy_ms = ind->chan_time_busy_ms;
    aml_survey->noise_dbm = ind->noise_dbm;
    aml_survey->filled = (SURVEY_INFO_TIME |
                           SURVEY_INFO_TIME_BUSY);

    if (ind->noise_dbm != 0) {
        aml_survey->filled |= SURVEY_INFO_NOISE_DBM;
    }

    return 0;
}

static inline int aml_rx_p2p_noa_upd_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    return 0;
}

static inline int aml_rx_rssi_status_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_rssi_status_ind *ind = (struct mm_rssi_status_ind *)msg->param;
    int vif_idx  = ind->vif_index;
    bool rssi_status = ind->rssi_status;

    struct aml_vif *vif_entry;

    AML_DBG(AML_FN_ENTRY_STR);

    vif_entry = aml_hw->vif_table[vif_idx];
    if (vif_entry) {
        cfg80211_cqm_rssi_notify(vif_entry->ndev,
                                 rssi_status ? NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW :
                                               NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH,
                                 ind->rssi, GFP_ATOMIC);
    }

    return 0;
}

static inline int aml_rx_pktloss_notify_ind(struct aml_hw *aml_hw,
                                             struct aml_cmd *cmd,
                                             struct ipc_e2a_msg *msg)
{
#ifdef CONFIG_AML_FULLMAC
    struct mm_pktloss_ind *ind = (struct mm_pktloss_ind *)msg->param;
    struct aml_vif *vif_entry;
    int vif_idx  = ind->vif_index;

    AML_DBG(AML_FN_ENTRY_STR);

    vif_entry = aml_hw->vif_table[vif_idx];
    if (vif_entry) {
        cfg80211_cqm_pktloss_notify(vif_entry->ndev, (const u8 *)ind->mac_addr.array,
                                    ind->num_packets, GFP_ATOMIC);
    }
#endif /* CONFIG_AML_FULLMAC */

    return 0;
}

static inline int aml_rx_csa_counter_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_csa_counter_ind *ind = (void *)msg->param;
    struct aml_vif *aml_vif = aml_vif_get(aml_hw, ind->vif_index);

    if (aml_vif) {
        if (aml_vif->ap.csa)
            aml_vif->ap.csa->count = ind->csa_count;
        else
            AML_ERR("%s: CSA counter update but no active CSA\n", aml_vif->ndev->name);
    }

    return 0;
}

static inline int aml_rx_csa_finish_ind(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct mm_csa_finish_ind *ind = (struct mm_csa_finish_ind *)msg->param;
    struct aml_vif *vif = aml_vif_get(aml_hw, ind->vif_index);

    AML_DBG(AML_FN_ENTRY_STR);

    if (vif) {
        if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP ||
            AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_GO) {
            if (vif->ap.csa) {
                vif->ap.csa->status = ind->status;
                vif->ap.csa->ch_idx = ind->chan_idx;
                schedule_work(&vif->ap.csa->work);
            } else
                AML_ERR("%s: CSA finish indication but no active CSA!\n", vif->ndev->name);
        } else if (ind->status == 0) {
            aml_chanctx_unlink(vif);
            aml_chanctx_link(vif, ind->chan_idx, NULL);
            if (aml_hw->cur_chanctx == ind->chan_idx) {
                aml_radar_detection_enable_on_cur_channel(aml_hw);
                aml_txq_vif_start(vif, AML_TXQ_STOP_CHAN);
            } else
                aml_txq_vif_stop(vif, AML_TXQ_STOP_CHAN);
        }
    }

    return 0;
}

void aml_sta_notify_csa_ch_switch(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg)
{
    struct ieee80211_channel *notify_channel = NULL;
    struct cfg80211_chan_def chandef = {0};
    struct mm_csa_finish_ind *ind = (struct mm_csa_finish_ind *)msg->param;
    struct aml_vif *vif;
    bool found = false;
    struct aml_chanctx *ctxt;

    // Look for VIF entry
    list_for_each_entry(vif, &aml_hw->vifs, list) {
        if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_AP) || (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_GO))
            continue;
        if (vif->vif_index == ind->vif_index) {
            found=true;
            break;
        }
    }

    AML_INFO("csa_status:%d, csa_chanidx:%d, csa_vif:%d, found:%d, vif_type:%d",
             ind->status, ind->chan_idx, ind->vif_index, found, AML_VIF_TYPE(vif));
    if (found && (ind->status == 0)) {
        notify_channel = ieee80211_get_channel(aml_hw->wiphy, ind->chan.prim20_freq);
        cfg80211_chandef_create(&chandef, notify_channel, NL80211_CHAN_NO_HT);
        if (!aml_hw->mod_params->ht_on)
            chandef.width = NL80211_CHAN_WIDTH_20_NOHT;
        else
            chandef.width = (enum nl80211_chan_width)chnl2bw[ind->chan.type];
        chandef.center_freq1 = ind->chan.center1_freq;
        chandef.center_freq2 = ind->chan.center2_freq;

        AML_INFO("chandef.center_freq1:%d, chandef.center_freq2:%d", chandef.center_freq1, chandef.center_freq2);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0)
        mutex_lock(&vif->wdev.wiphy->mtx);
        __acquire(&vif->wdev.wiphy->mtx);
        aml_cfg80211_ch_switch_notify(vif->ndev, &chandef, 0);
        mutex_unlock(&vif->wdev.wiphy->mtx);
        __release(&vif->wdev.wiphy->mtx);
#else
        mutex_lock(&vif->wdev.mtx);
        __acquire(&vif->wdev.mtx);
        aml_cfg80211_ch_switch_notify(vif->ndev, &chandef, 0);
        mutex_unlock(&vif->wdev.mtx);
        __release(&vif->wdev.mtx);
#endif

        ctxt = &vif->aml_hw->chanctx_table[vif->ch_index];
        if (!ctxt->chan_def.chan) {
            ctxt->chan_def = chandef;
        }

        if (AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) {
#ifdef CONFIG_AML_RECOVERY
            aml_recy_flags_set(AML_RECY_CHECK_SCC);
#endif
            aml_check_scc();
        }

        if ((AML_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) ||
             (AML_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT)) {
            if (vif->sta.ap)
                vif->sta.ap->stats.bw_max = ind->chan.type;
        }
    }

    return;
}

static inline int aml_rx_csa_traffic_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_csa_traffic_ind *ind = (struct mm_csa_traffic_ind *)msg->param;
    struct aml_vif *vif = aml_vif_get(aml_hw, ind->vif_index);

    if (vif) {
        if (ind->enable)
            aml_txq_vif_start(vif, AML_TXQ_STOP_CSA);
        else
            aml_txq_vif_stop(vif, AML_TXQ_STOP_CSA);
    }

    return 0;
}

static inline int aml_rx_ps_change_ind(struct aml_hw *aml_hw,
                                        struct aml_cmd *cmd,
                                        struct ipc_e2a_msg *msg)
{
    struct mm_ps_change_ind *ind = (struct mm_ps_change_ind *)msg->param;
    struct aml_sta *sta = aml_hw->sta_table + ind->sta_idx;

    //AML_DBG(AML_FN_ENTRY_STR);

    if (ind->sta_idx >= (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
        wiphy_err(aml_hw->wiphy, "Invalid sta index reported by fw %d\n",
                  ind->sta_idx);
        return 1;
    }

    /*
    AML_INFO("Sta %d, valid:%d, change PS mode to %s,mac[%02x %02x %02x %02x %02x %02x]", sta->sta_idx, sta->valid,
               ind->ps_state ? "ON" : "OFF",
               sta->mac_addr[0],sta->mac_addr[1],sta->mac_addr[2],sta->mac_addr[3],sta->mac_addr[4],sta->mac_addr[5]);
    */

    if (sta->valid) {
        aml_ps_bh_enable(aml_hw, sta, ind->ps_state);
    } else if (test_bit(AML_DEV_ADDING_STA, (void*)&aml_hw->flags)) {
        sta->ps.active = ind->ps_state ? true : false;
    } else {
        AML_INFO("Ignore PS mode change on invalid sta\n");
    }

    return 0;
}


static inline int aml_rx_traffic_req_ind(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    struct mm_traffic_req_ind *ind = (struct mm_traffic_req_ind *)msg->param;
    struct aml_sta *sta = aml_hw->sta_table + ind->sta_idx;

    AML_DBG(AML_FN_ENTRY_STR);

    netdev_dbg(aml_hw->vif_table[sta->vif_idx]->ndev,
               "Sta %d, asked for %d pkt", sta->sta_idx, ind->pkt_cnt);

    aml_ps_bh_traffic_req(aml_hw, sta, ind->pkt_cnt,
                           ind->uapsd ? UAPSD_ID : LEGACY_PS_ID);

    return 0;
}

/***************************************************************************
 * Messages from SCAN task
 **************************************************************************/
#ifdef CONFIG_AML_SOFTMAC
static inline int aml_rx_scan_done_ind(struct aml_hw *aml_hw,
                                        struct aml_cmd *cmd,
                                        struct ipc_e2a_msg *msg)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    struct cfg80211_scan_info info = {
        .aborted = false,
    };
#endif
    AML_DBG(AML_FN_ENTRY_STR);

    aml_ipc_buf_dealloc(aml_hw, &aml_hw->scan_ie);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    ieee80211_scan_completed(aml_hw->hw, &info);
#else
    ieee80211_scan_completed(aml_hw->hw, false);
#endif

    return 0;
}
#endif /* CONFIG_AML_SOFTMAC */

/***************************************************************************
 * Messages from SCANU task
 **************************************************************************/
#ifdef CONFIG_AML_FULLMAC
extern int aml_sdio_rx_scanu_result_ind(struct aml_hw *aml_hw);
static inline int aml_rx_scanu_start_cfm(struct aml_hw *aml_hw,
                                          struct aml_cmd *cmd,
                                          struct ipc_e2a_msg *msg)
{
    AML_INFO("%s, cur_chan:%d, rx beacon and probe rsp count:%d\n",
             __func__, aml_hw->cur_chanctx, aml_hw->misc.scan_result_cnt);

    aml_ipc_buf_dealloc(aml_hw, &aml_hw->scan_ie);
    spin_lock_bh(&aml_hw->scan_req_lock);
    if (aml_hw->scan_request) {
        // sdio and usb flush list@aml_hw->scan_res_list in scan-end
        // because event@SCANU_RESULT_IND and payload are out-sync
        if (aml_bus_type != PCIE_MODE)
            aml_sdio_rx_scanu_result_ind(aml_hw);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        {
            struct cfg80211_scan_info info = {
                .aborted = false,
            };

            cfg80211_scan_done(aml_hw->scan_request, &info);
        }
#else
        cfg80211_scan_done(aml_hw->scan_request, false);
#endif
    }

    aml_hw->scan_request = NULL;
    spin_unlock_bh(&aml_hw->scan_req_lock);
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy && aml_recy->link_loss.is_enabled
        && aml_recy->link_loss.is_happened
        && !aml_recy->link_loss.scan_result_cnt
        && aml_recy->link_loss.scan_cnt++ >= 3) {
        aml_recy->link_loss.is_requested = 1;
        aml_recy->link_loss.scan_cnt = 0;
        aml_recy->link_loss.is_happened = 0;
    }
    if (aml_recy->link_loss.scan_result_cnt) {
        aml_recy->link_loss.is_happened = 0;
        aml_recy->link_loss.scan_result_cnt = 0;
        aml_recy->link_loss.scan_cnt = 0;
    }
#endif
    aml_hw->misc.scan_result_cnt = 0;

    AML_M_INFO(SCAN, "happened %d, scan_result_cnt %d, scan_cnt %d, request %d\n",
        aml_recy->link_loss.is_happened,
        aml_recy->link_loss.scan_result_cnt,
        aml_recy->link_loss.scan_cnt,
        aml_recy->link_loss.is_requested);
    return 0;
}

static int aml_get_freq_from_ie(struct wiphy *wiphy, const u8 *ie, size_t ielen)
{
    const u8 *tmp;
    int band;
    int channel_number = -1;

    tmp = cfg80211_find_ie(WLAN_EID_DS_PARAMS, ie, ielen);
    if (tmp && tmp[1] == 1) {
        channel_number = tmp[2];
    } else {
        tmp = cfg80211_find_ie(WLAN_EID_HT_OPERATION, ie, ielen);
        if (tmp && tmp[1] >= sizeof(struct ieee80211_ht_operation)) {
            struct ieee80211_ht_operation *htop = (void *)(tmp + 2);

            channel_number = htop->primary_chan;
        }
    }

    if (channel_number < 0) {
        /* No channel information in frame payload */
        return 0;
    }
    if (channel_number <= 14)
        band = NL80211_BAND_2GHZ;
    else if (channel_number <= 177)
        band = NL80211_BAND_5GHZ;
    else
        return 0;

    return ieee80211_channel_to_frequency(channel_number, band);
}

int aml_sdio_rx_scanu_result_ind(struct aml_hw *aml_hw)
{
    struct cfg80211_bss *bss = NULL;
    struct ieee80211_channel *chan;
    u64 timestamp;
    struct ieee80211_mgmt *mgmt = NULL;
    struct scan_results *scan_res, *next;
    const u8 *ie;
    size_t ielen;
    int freq;

    spin_lock_bh(&aml_hw->scan_lock);
    list_for_each_entry_safe(scan_res, next, &aml_hw->scan_res_list, list) {
        struct sdio_scanu_result_ind *ind = &scan_res->scanu_res_ind;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
        struct timespec ts;
        get_monotonic_boottime(&ts);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
        timestamp = ktime_to_us(ktime_get_boottime());
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
        timestamp = ((u64)ts.tv_sec * 1000000) + ts.tv_nsec / 1000;
#endif

        mgmt = (struct ieee80211_mgmt *)ind->payload;

        if ((uint8_t *)mgmt < aml_hw->scanres_payload_buf || (uint8_t *)mgmt > (aml_hw->scanres_payload_buf + (SCAN_RESULTS_MAX_CNT * 500) - 21)) {
            AML_INFO("buf start:%p buf end:%p mgmt:%p\n", aml_hw->scanres_payload_buf, (aml_hw->scanres_payload_buf + SCAN_RESULTS_MAX_CNT * 500), mgmt);
            AML_INFO("ind:%p len:%d\n", ind, ind->length);

            list_del(&scan_res->list);
            list_add_tail(&scan_res->list, &aml_hw->scan_res_available_list);
            continue;
        }

        mgmt->u.probe_resp.timestamp = timestamp;
        ie = mgmt->u.probe_resp.variable;
        ielen = ind->length - offsetof(struct ieee80211_mgmt, u.probe_resp.variable);

        freq = aml_get_freq_from_ie(aml_hw->wiphy, ie, ielen);
        if (freq == 0) {
            freq = ind->center_freq;
        }

        if ((ie[0] == WLAN_EID_SSID) && (ie + 2 + ie[1] < (u8 *) mgmt + ind->length)) {
            AML_M_INFO(SCAN, "ind freq:%d, get from ie:%d, ssid:%-32.32s bssid:%02x:%02x:%02x:%02x:%02x:%02x, frm ctrl 0x%x, rssi %d",
                ind->center_freq, freq,
                ssid_sprintf(&ie[2], ie[1]),
                mgmt->bssid[0],
                mgmt->bssid[1],
                mgmt->bssid[2],
                mgmt->bssid[3],
                mgmt->bssid[4],
                mgmt->bssid[5], mgmt->frame_control, ind->rssi);
        }

        chan = ieee80211_get_channel(aml_hw->wiphy, freq);
        if (chan != NULL)
            bss = cfg80211_inform_bss_frame(aml_hw->wiphy, chan, mgmt,
                    ind->length, ind->rssi * 100, GFP_ATOMIC);
        if (bss != NULL)
            cfg80211_put_bss(aml_hw->wiphy, bss);

        list_del(&scan_res->list);
        scan_res->scanu_res_ind.payload = NULL;
        list_add_tail(&scan_res->list, &aml_hw->scan_res_available_list);
    }
    spin_unlock_bh(&aml_hw->scan_lock);

    return 0;
}


static inline int aml_pcie_rx_scanu_result_ind(struct aml_hw *aml_hw,
                                           struct aml_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    struct cfg80211_bss *bss = NULL;
    struct ieee80211_channel *chan;
    u64 timestamp;
    struct ieee80211_mgmt *mgmt = NULL;
    struct scanu_result_ind *ind = (struct scanu_result_ind *)msg->param;
    u8* ie = NULL;
    size_t ielen;
    int freq;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
    struct timespec ts;
#else
    struct timeval ts;
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
    timestamp = ktime_to_us(ktime_get_boottime());
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
    get_monotonic_boottime(&ts);
    timestamp = ((u64)ts.tv_sec * 1000000) + ts.tv_nsec / 1000;
#else
    do_gettimeofday(&ts);
    timestamp = ((u64)ts.tv_sec * 1000000) + ts.tv_nsec / 1000;
#endif
    mgmt = (struct ieee80211_mgmt *)ind->payload;
    mgmt->u.probe_resp.timestamp = timestamp;
    ie = mgmt->u.probe_resp.variable;
    ielen = ind->length - offsetof(struct ieee80211_mgmt, u.probe_resp.variable);

    freq = aml_get_freq_from_ie(aml_hw->wiphy, ie, ielen);
    if (freq == 0) {
        freq = ind->center_freq;
    }

    if ((ie[0] == WLAN_EID_SSID) && (ie + 2 + ie[1] < (u8 *) mgmt + ind->length)) {
        AML_M_INFO(SCAN, "ind freq:%d, get from ie:%d, ssid:%-32.32s bssid:%02x:%02x:%02x:%02x:%02x:%02x, frm ctrl 0x%x,",
                ind->center_freq, freq,
                ssid_sprintf(&ie[2], ie[1]),
                mgmt->bssid[0],
                mgmt->bssid[1],
                mgmt->bssid[2],
                mgmt->bssid[3],
                mgmt->bssid[4],
                mgmt->bssid[5], mgmt->frame_control);
    }

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy && aml_recy->link_loss.is_enabled
        && aml_recy->link_loss.is_happened) {
        aml_recy->link_loss.scan_result_cnt++;
    }
#endif
    aml_hw->misc.scan_result_cnt++;

    chan = ieee80211_get_channel(aml_hw->wiphy, freq);
    if (chan != NULL)
        bss = cfg80211_inform_bss_frame(aml_hw->wiphy, chan, mgmt,
                ind->length, ind->rssi * 100, GFP_ATOMIC);
    if (bss != NULL)
        cfg80211_put_bss(aml_hw->wiphy, bss);

    return 0;
}

static inline int aml_sdio_rx_scanu_result_for_join_ind(struct aml_hw *aml_hw,
                                                        struct aml_cmd *cmd,
                                                        struct ipc_e2a_msg *msg)
{
    // scanu_result_ind upload by msg for join, it is same with pcie
    aml_pcie_rx_scanu_result_ind(aml_hw, cmd, msg);
    return 0;
}


static inline int aml_rx_scanu_result_ind(struct aml_hw *aml_hw,
                                           struct aml_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    if (aml_bus_type == PCIE_MODE)
        aml_pcie_rx_scanu_result_ind(aml_hw, cmd, msg);
    else
        aml_sdio_rx_scanu_result_ind(aml_hw);

    return 0;
}

#endif /* CONFIG_AML_FULLMAC */

/***************************************************************************
 * Messages from ME task
 **************************************************************************/
#ifdef CONFIG_AML_FULLMAC
static inline int aml_rx_me_tkip_mic_failure_ind(struct aml_hw *aml_hw,
                                                  struct aml_cmd *cmd,
                                                  struct ipc_e2a_msg *msg)
{
    struct me_tkip_mic_failure_ind *ind = (struct me_tkip_mic_failure_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct net_device *dev = aml_vif->ndev;

    if (ind->vif_idx > NX_ITF_MAX) {
        AML_ERR("vif_idx:%d\n", ind->vif_idx);
        return 0;
    }

    AML_DBG(AML_FN_ENTRY_STR);

    cfg80211_michael_mic_failure(dev, (u8 *)&ind->addr, (ind->ga?NL80211_KEYTYPE_GROUP:
                                 NL80211_KEYTYPE_PAIRWISE), ind->keyid,
                                 (u8 *)&ind->tsc, GFP_ATOMIC);

    return 0;
}

static inline int aml_rx_me_tx_credits_update_ind(struct aml_hw *aml_hw,
                                                   struct aml_cmd *cmd,
                                                   struct ipc_e2a_msg *msg)
{
    struct me_tx_credits_update_ind *ind = (struct me_tx_credits_update_ind *)msg->param;

    AML_DBG(AML_FN_ENTRY_STR);

    aml_txq_credit_update(aml_hw, ind->sta_idx, ind->tid, ind->credits);

    return 0;
}
#endif /* CONFIG_AML_FULLMAC */

/***************************************************************************
 * Messages from SM task
 **************************************************************************/
#ifdef CONFIG_AML_FULLMAC
extern const struct aml_legrate legrates_lut[16];

char format_mod_trace[][20] = {
    "NON_HT",
    "NON_HT_DUP_OFDM",
    "HT_MF",
    "HT_GF",
    "VHT",
    "HE_SU",
    "HE_MU",
    "HE_ER"
};
char no_ss_trace[][5] = {
    "1ss",
    "2ss"
};
char bw_width_trace[][15] = {
    "BW_20",
    "BW_40",
    "BW_80",
    "BW_160",
    "BW_80P80",
    "BW_OTHER"
};

static inline int aml_rx_sm_connect_ind_ex(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct sm_connect_ind_ex *ind = (struct sm_connect_ind_ex *)msg->param;
    struct aml_sta *sta = aml_hw->sta_table + ind->ap_idx;
    int i;

    sta->stats.bcn_interval = ind->bcn_interval;
    sta->stats.bw_max = ind->bw_max;
    //sta->stats.dtim = ind->dtim; //TODO dtim info
    sta->stats.mcs_max = ind->mcs_max;
    sta->stats.no_ss = ind->no_ss;
    sta->stats.format_mod = ind->format_mod;
    sta->stats.short_gi = ind->short_gi;
    AML_INFO("freq[%d %d %d], bw:%s max_mcs:%d format_mode:%s no_ss:%s short_gi:%d\n",
            sta->center_freq, sta->center_freq1, sta->center_freq2,
            bw_width_trace[sta->width], sta->stats.mcs_max,
            format_mod_trace[sta->stats.format_mod],
            no_ss_trace[sta->stats.no_ss], sta->stats.short_gi);

    for (i = 0; i < sizeof (legrates_lut) / sizeof(struct aml_legrate); i++) {
        if (ind->r_idx_max == legrates_lut[i].idx) {
            sta->stats.leg_rate = legrates_lut[i].rate;
            break;
        }
    }
    return 0;
}

void aml_del_sta(struct aml_vif *aml_vif, const u8 *mac_addr, u32 freq)
{
    struct net_device *dev = aml_vif->ndev;
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    AML_INFO("flags:%d", aml_hw->wiphy->flags & WIPHY_FLAG_HAVE_AP_SME);
    if (aml_hw->wiphy->flags & WIPHY_FLAG_HAVE_AP_SME) {
        cfg80211_del_sta(dev, mac_addr, GFP_ATOMIC);
    }
    else {
        #define DEAUTH_FRAME_LEN 26
        #define DEAUTH_TYPE 0XC0
        #define DEAUTH_REASON 0X3
        u8 *pos;
        struct sk_buff *skb = alloc_skb(DEAUTH_FRAME_LEN, GFP_ATOMIC);

        if (!skb) {
            AML_INFO("deauth alloc fail");
            return;
        }

        pos = (void *)skb_put(skb, DEAUTH_FRAME_LEN);
        *pos = DEAUTH_TYPE;
        pos += 2;
        memset(pos, 0, 2);//dur
        pos += 2;
        memcpy(pos, dev->dev_addr, MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        memcpy(pos, mac_addr, MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        memcpy(pos, dev->dev_addr, MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        memset(pos, 0, 2);//seq
        pos += 2;
        *pos = DEAUTH_REASON;

        cfg80211_rx_mgmt(&aml_vif->wdev, freq, 0, skb->data, skb->len, 0);
        kfree_skb(skb);
        /* coverity[leaked_storage] */
    }
}

static inline int aml_apm_handle_disconnect_sta(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct apm_disconnect_sta_ind *ind = (struct apm_disconnect_sta_ind *)msg->param;
    struct aml_vif *aml_vif;
    struct net_device *dev;

    if (!aml_hw->vif_table[ind->vif_idx] || !aml_hw->vif_table[ind->vif_idx]->ndev)
        return -1;

    aml_vif = aml_hw->vif_table[ind->vif_idx];
    dev = aml_vif->ndev;

    AML_INFO("sta ageout --> %pM \n", ind->sta_mac);

    aml_del_sta(aml_vif, ind->sta_mac, ind->pri_freq);
    return 0;
}

static inline int aml_rx_sm_connect_ind(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct sm_connect_ind *ind = (struct sm_connect_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct net_device *dev = aml_vif->ndev;
    const u8 *req_ie, *rsp_ie;
    const u8 *extcap_ie;
    const struct ieee_types_extcap *extcap;
    unsigned char ipv4_addr[IPV4_ADDR_LEN] = {0};

    AML_INFO("vif_idx:%d, status_code:%d, sta_idx:%d,"
            "center_freq:%d, center_freq1:%d, roamed:%d, mac:%pM",
            ind->vif_idx, ind->status_code, ind->ap_idx,
            ind->chan.prim20_freq, ind->chan.center1_freq, ind->roamed,
            (const u8 *)ind->bssid.array);
    aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
    /* Retrieve IE addresses and lengths */
    req_ie = (const u8 *)ind->assoc_ie_buf;
    rsp_ie = req_ie + ind->assoc_req_ie_len;

    // Fill-in the AP information
    if (ind->status_code == 0)
    {
        struct aml_sta *sta = aml_hw->sta_table + ind->ap_idx;
        u8 txq_status;
        struct ieee80211_channel *chan;
        struct cfg80211_chan_def chandef = {0};

        sta->valid = true;
        sta->sta_idx = ind->ap_idx;
        sta->ch_idx = ind->ch_idx;
        sta->vif_idx = ind->vif_idx;
        sta->vlan_idx = sta->vif_idx;
        sta->qos = ind->qos;
        sta->acm = ind->acm;
        sta->ps.active = false;
        sta->aid = ind->aid;
        sta->band = (enum nl80211_band)ind->chan.band;
        sta->width = (enum nl80211_chan_width)ind->chan.type;
        sta->center_freq = ind->chan.prim20_freq;
        sta->center_freq1 = ind->chan.center1_freq;
        sta->center_freq2 = ind->chan.center2_freq;
        aml_vif->sta.ap = sta;
        aml_vif->generation++;

        if (aml_vif->sta.flags & AML_STA_FT_OVER_DS)
            aml_vif->sta.flags &= ~AML_STA_FT_OVER_DS;

        if (aml_vif->sta.flags & AML_STA_FT_OVER_AIR)
            aml_vif->sta.flags &= ~AML_STA_FT_OVER_AIR;

        chan = ieee80211_get_channel(aml_hw->wiphy, ind->chan.prim20_freq);

        cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);
        if (!aml_hw->mod_params->ht_on)
            chandef.width = NL80211_CHAN_WIDTH_20_NOHT;
        else
            chandef.width = (enum nl80211_chan_width)chnl2bw[ind->chan.type];
        chandef.center_freq1 = ind->chan.center1_freq;
        chandef.center_freq2 = ind->chan.center2_freq;
        aml_chanctx_link(aml_vif, ind->ch_idx, &chandef);
        memcpy(sta->mac_addr, ind->bssid.array, ETH_ALEN);
        if (ind->ch_idx == aml_hw->cur_chanctx) {
            txq_status = 0;
        } else {
            txq_status = AML_TXQ_STOP_CHAN;
        }
        memcpy(sta->ac_param, ind->ac_param, sizeof(sta->ac_param));
        aml_sta_init(aml_hw, sta, txq_status);
        aml_txq_tdls_vif_init(aml_vif);
        aml_mu_group_sta_init(sta, NULL);
        /* Look for TDLS Channel Switch Prohibited flag in the Extended Capability
         * Information Element*/
        extcap_ie = cfg80211_find_ie(WLAN_EID_EXT_CAPABILITY, rsp_ie, ind->assoc_rsp_ie_len);
        if (extcap_ie && extcap_ie[1] >= 5) {
            extcap = (void *)(extcap_ie);
            aml_vif->tdls_chsw_prohibited = extcap->ext_capab[4] & WLAN_EXT_CAPA5_TDLS_CH_SW_PROHIBITED;
        }

#ifdef CONFIG_AML_BFMER
        /* If Beamformer feature is activated, check if features can be used
         * with the new peer device
         */
        if (aml_hw->mod_params->bfmer) {
            const u8 *vht_capa_ie;
            const struct ieee80211_vht_cap *vht_cap;

            do {
                /* Look for VHT Capability Information Element */
                vht_capa_ie = cfg80211_find_ie(WLAN_EID_VHT_CAPABILITY, rsp_ie,
                                               ind->assoc_rsp_ie_len);

                /* Stop here if peer device does not support VHT */
                if (!vht_capa_ie) {
                    break;
                }

                vht_cap = (const struct ieee80211_vht_cap *)(vht_capa_ie + 2);

                /* Send MM_BFMER_ENABLE_REQ message if needed */
                aml_send_bfmer_enable(aml_hw, sta, vht_cap);
            } while (0);
        }
#endif //(CONFIG_AML_BFMER)

#ifdef CONFIG_AML_MON_DATA
        // If there are 1 sta and 1 monitor interface active at the same time then
        // monitor interface channel context is always the same as the STA interface.
        // This doesn't work with 2 STA interfaces but we don't want to support it.
        if (aml_hw->monitor_vif != AML_INVALID_VIF) {
            struct aml_vif *aml_mon_vif = aml_hw->vif_table[aml_hw->monitor_vif];
            aml_chanctx_unlink(aml_mon_vif);
            aml_chanctx_link(aml_mon_vif, ind->ch_idx, NULL);
        }
#endif

#ifdef CONFIG_AML_RECOVERY
        /*recovery connect has no inetaddr_event,so check scc here */
        aml_check_scc();
#endif
        if (pt_mode)
            aml_set_scan_hang(aml_vif, 1, __func__, __LINE__);

        if (sta->band == NL80211_BAND_2GHZ)
            coex_flag |= (1U << 30);
        else
            coex_flag &= ~(1U << 30);
    } else {
        aml_external_auth_disable(aml_vif);
    }

    if (ind->roamed) {
        struct cfg80211_roam_info info;
        memset(&info, 0, sizeof(info));

        if (aml_vif->ch_index < NX_CHAN_CTXT_CNT)
            LINK_PARAMS(info, channel) = aml_hw->chanctx_table[aml_vif->ch_index].chan_def.chan;
        LINK_PARAMS(info, bssid) = (const u8 *)ind->bssid.array;
        info.req_ie = req_ie;
        info.req_ie_len = ind->assoc_req_ie_len;
        info.resp_ie = rsp_ie;
        info.resp_ie_len = ind->assoc_rsp_ie_len;
        cfg80211_roamed(dev, &info, GFP_ATOMIC);
    }
    else {
#ifdef CONFIG_AML_RECOVERY
            if (ind->status_code == 0) {
                aml_recy->reconnect_rest = 0;
            }
            /*recovery connect fail,try again */
            if (ind->status_code && aml_recy->reconnect_rest && aml_recy_connect_retry() == 0) {
                aml_recy->reconnect_rest--;
                AML_INFO("recy retry connect,rest cnt:%d", aml_recy->reconnect_rest);
            }
            else
#endif
            {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
                struct cfg80211_bss *bss = NULL;
                struct ieee80211_channel *notify_channel = NULL;
                notify_channel = ieee80211_get_channel(aml_hw->wiphy, ind->chan.prim20_freq);
                if (notify_channel != NULL) {
                    bss = cfg80211_get_bss(aml_hw->wiphy, notify_channel,
                            (const u8 *)ind->bssid.array, (const u8 *)aml_vif->sta.assoc_ssid,
                            aml_vif->sta.assoc_ssid_len, IEEE80211_BSS_TYPE_ANY, IEEE80211_PRIVACY_ANY);
                } else {
                    AML_INFO("channel is null");
                }
                if (bss != NULL) {
                    /* coverity[overrun-buffer-val] - the string 'ssid' is configured without a null terminator */
                    AML_INFO("ssid:%s, bss_freq:%d", ssid_sprintf(aml_vif->sta.assoc_ssid, aml_vif->sta.assoc_ssid_len),
                            bss->channel->center_freq);
                    // if vif has ip, flag should not be set. eg: recover and roam
                    if ((aml_connect_flags_chk(aml_vif, AML_CONNECTING))
                        && (ind->status_code == 0)
                        && !memcmp(aml_vif->ipv4_addr, ipv4_addr, IPV4_ADDR_LEN))
                        aml_connect_flags_set(aml_vif, AML_GETTING_IP);
                } else {
                    AML_INFO("can't find bss in kernel");
                }

                cfg80211_connect_bss(dev, (const u8 *)ind->bssid.array, bss, req_ie, ind->assoc_req_ie_len, rsp_ie,
                    #if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 12, 0)
                                    ind->assoc_rsp_ie_len, ind->status_code, GFP_ATOMIC);
                    #else
                                    ind->assoc_rsp_ie_len, ind->status_code, GFP_ATOMIC,NL80211_TIMEOUT_UNSPECIFIED);
                    #endif
#else
                cfg80211_connect_result(dev, (const u8 *)ind->bssid.array, req_ie,
                                        ind->assoc_req_ie_len, rsp_ie,
                                        ind->assoc_rsp_ie_len, ind->status_code,
                                        GFP_ATOMIC);

#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
                if (ind->status_code != 0 && aml_vif->wdev.connected) {
                    AML_INFO("roam connect fail, need upload disconnect to kernel to clear wdev.connected\n");
                    cfg80211_disconnected(dev, 0, NULL, 0, false, GFP_ATOMIC);
                }
#endif
                /* normal connect, but fail */
                if (ind->status_code != 0) {
                    aml_wake_source_relax(aml_hw);
                }
            }
    }
    netif_tx_start_all_queues(dev);
    netif_carrier_on(dev);
    aml_connect_flags_clr(aml_vif, AML_CONNECTING);
    return 0;
}

static inline int aml_rx_sm_disconnect_ind(struct aml_hw *aml_hw,
                                            struct aml_cmd *cmd,
                                            struct ipc_e2a_msg *msg)
{
    struct sm_disconnect_ind *ind = (struct sm_disconnect_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct net_device *dev;

    if (aml_vif == NULL)
        return 0;

    AML_INFO("vif_idx:%d, reason_code: %d, reassoc: %d", aml_vif->vif_index,
        (int)(ind->reason_code & ~HOST_REQUEST_DISCONNECT), ind->reassoc);

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy && aml_recy->link_loss.is_enabled
        && ind->reason_code == AML_RECY_REASON_CODE_LINK_LOSS) {
        AML_INFO("link loss disconnect happen, statistics scan and evaluates for recovery");
        aml_recy->link_loss.is_happened = true;
        aml_recy->link_loss.scan_cnt = 0;
        ind->reason_code = HOST_REQUEST_DISCONNECT | MAC_RS_DEAUTH_SENDER_LEFT_IBSS_ESS;
    }
#endif

    dev = aml_vif->ndev;

    /* if vif is not up, aml_close has already been called */
    if (aml_vif->up) {
        if (!ind->reassoc) {
            cfg80211_disconnected(dev, (ind->reason_code & ~HOST_REQUEST_DISCONNECT), NULL, 0,
                                  ((ind->reason_code & HOST_REQUEST_DISCONNECT) != 0), GFP_ATOMIC);

            if (aml_vif->sta.ft_assoc_ies) {
                kfree(aml_vif->sta.ft_assoc_ies);
                aml_vif->sta.ft_assoc_ies = NULL;
                aml_vif->sta.ft_assoc_ies_len = 0;
            }
        }
        if (aml_connect_flags_chk(aml_vif, AML_CONNECTING | AML_GETTING_IP)) {
            aml_wake_source_relax(aml_hw);
        }
        netif_tx_stop_all_queues(dev);
        netif_carrier_off(dev);
    }
    spin_lock_bh(&aml_vif->vif_lock);
    if (aml_vif->sta.ap) {
#ifdef CONFIG_AML_BFMER
        /* Disable Beamformer if supported */
        aml_bfmer_report_del(aml_hw, aml_vif->sta.ap);
#endif //(CONFIG_AML_BFMER)

        aml_sta_deinit(aml_hw, aml_vif->sta.ap);
        aml_txq_tdls_vif_deinit(aml_vif);
        AML_INFO("sta assoc ap info was cleared, sta_idx:%d", aml_vif->sta.ap->sta_idx);
        aml_vif->sta.ap->valid = false;
        aml_vif->sta.ap = NULL;
    }
    spin_unlock_bh(&aml_vif->vif_lock);
    aml_vif->generation++;
    aml_external_auth_disable(aml_vif);
    aml_chanctx_unlink(aml_vif);
    aml_set_scan_hang(aml_vif, 0, __func__, __LINE__);
#ifdef CONFIG_AML_RECOVERY
    if ((aml_recy) && (aml_vif->vif_index == aml_recy->assoc_info.vif_idx)) {
        aml_recy_flags_clr(AML_RECY_ASSOC_INFO_SAVED);
    }
#endif
    aml_connect_flags_clr(aml_vif, AML_DISCONNECTING);
    aml_connect_flags_clr(aml_vif, AML_GETTING_IP);
    coex_flag &= ~(1U << 30);

    return 0;
}

static inline int aml_rx_sm_external_auth_required_ind(struct aml_hw *aml_hw,
                                                        struct aml_cmd *cmd,
                                                        struct ipc_e2a_msg *msg)
{
    struct sm_external_auth_required_ind *ind =
        (struct sm_external_auth_required_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    struct net_device *dev = aml_vif->ndev;
    struct cfg80211_external_auth_params params;

    AML_DBG(AML_FN_ENTRY_STR);

    params.action = NL80211_EXTERNAL_AUTH_START;
    memcpy(params.bssid, ind->bssid.array, ETH_ALEN);
    params.ssid.ssid_len = ind->ssid.length;
    memcpy(params.ssid.ssid, ind->ssid.array,
           min_t(size_t, ind->ssid.length, sizeof(params.ssid.ssid)));
    params.key_mgmt_suite = ind->akm;

    AML_INFO("ind->vif_idx > NX_VIRT_DEV_MAX %d, !aml_vif->up %d, (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_STATION) %d",
        ind->vif_idx > NX_VIRT_DEV_MAX, !aml_vif->up, (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_STATION));
    if ((ind->vif_idx >= NX_VIRT_DEV_MAX) || !aml_vif->up ||
        (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_STATION) ||
        cfg80211_external_auth_request(dev, &params, GFP_ATOMIC)) {
        wiphy_err(aml_hw->wiphy, "Failed to start external auth on vif %d",
                  ind->vif_idx);
        aml_send_sm_external_auth_required_rsp(aml_hw, aml_vif,
                                                WLAN_STATUS_UNSPECIFIED_FAILURE);
        return 0;
    }

    aml_external_auth_enable(aml_vif);
#ifdef CONFIG_AML_RECOVERY
    if (aml_vif->sta.ap) {
        aml_vif->sta.ap = NULL;
    }
#endif

#else
    aml_send_sm_external_auth_required_rsp(aml_hw, aml_vif,
                                            WLAN_STATUS_UNSPECIFIED_FAILURE);
#endif
    return 0;
}

static inline int aml_rx_sm_ft_auth_ind(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct sm_ft_auth_ind *ind = (struct sm_ft_auth_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct sk_buff *skb;
    size_t data_len = (offsetof(struct ieee80211_mgmt, u.auth.variable) +
                       ind->ft_ie_len);

    skb = dev_alloc_skb(data_len);
    if (skb) {
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb_put(skb, data_len);
        mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_AUTH);
        memcpy(mgmt->u.auth.variable, ind->ft_ie_buf, ind->ft_ie_len);
        aml_rx_defer_skb(aml_hw, aml_vif, skb);
        dev_kfree_skb(skb);
    } else {
        netdev_warn(aml_vif->ndev, "Allocation failed for FT auth ind\n");
    }

    return 0;
}

static inline int aml_rx_sm_ft_auth_rsp_timeout_ind(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct aml_ft_auth_timeout *ft_auth_timeout = (struct aml_ft_auth_timeout *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ft_auth_timeout->vif_idx];
    aml_vif->sta.flags &= ~(AML_STA_FT_OVER_AIR | AML_STA_FT_OVER_DS);
    AML_INFO(" vif idx %d, flags 0x%08x\n", ft_auth_timeout->vif_idx, aml_vif->sta.flags);
    return 0;
}

/***************************************************************************
 * Messages from TWT task
 **************************************************************************/
static inline int aml_rx_twt_setup_ind(struct aml_hw *aml_hw,
                                        struct aml_cmd *cmd,
                                        struct ipc_e2a_msg *msg)
{
    struct twt_setup_ind *ind = (struct twt_setup_ind *)msg->param;
    struct aml_sta *aml_sta = aml_hw->sta_table + ind->sta_idx;

    AML_DBG(AML_FN_ENTRY_STR);

    memcpy(&aml_sta->twt_ind, ind, sizeof(struct twt_setup_ind));
    return 0;
}

static inline int aml_rx_mesh_path_create_cfm(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    struct mesh_path_create_cfm *cfm = (struct mesh_path_create_cfm *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[cfm->vif_idx];

    AML_DBG(AML_FN_ENTRY_STR);

    /* Check we well have a Mesh Point Interface */
    if (aml_vif && (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_MESH_POINT))
        aml_vif->ap.flags &= ~AML_AP_CREATE_MESH_PATH;

    return 0;
}

static inline int aml_rx_mesh_peer_update_ind(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    struct mesh_peer_update_ind *ind = (struct mesh_peer_update_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct aml_sta *aml_sta = aml_hw->sta_table + ind->sta_idx;

    AML_DBG(AML_FN_ENTRY_STR);

    if ((ind->vif_idx >= NX_VIRT_DEV_MAX) ||
        (aml_vif && (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT)) ||
        (ind->sta_idx >= NX_REMOTE_STA_MAX))
        return 1;

    if (aml_vif && (aml_vif->ap.flags & AML_AP_USER_MESH_PM))
    {
        if (!ind->estab && aml_sta->valid) {
            /* There is no way to inform upper layer for lost of peer, still
               clean everything in the driver */
            aml_sta->ps.active = false;
            aml_sta->valid = false;

            /* Remove the station from the list of VIF's station */
            list_del_init(&aml_sta->list);

            aml_sta_deinit(aml_hw, aml_sta);
        } else {
            WARN_ON(0);
        }
    } else {
        /* Check if peer link has been established or lost */
        if (ind->estab) {
            if (aml_vif && (!aml_sta->valid)) {
                u8 txq_status;

                aml_sta->valid = true;
                aml_sta->sta_idx = ind->sta_idx;
                aml_sta->ch_idx = aml_vif->ch_index;
                aml_sta->vif_idx = ind->vif_idx;
                aml_sta->vlan_idx = aml_sta->vif_idx;
                aml_sta->ps.active = false;
                aml_sta->qos = true;
                aml_sta->aid = ind->sta_idx + 1;
                //aml_sta->acm = ind->acm;
                memcpy(aml_sta->mac_addr, ind->peer_addr.array, ETH_ALEN);

                aml_chanctx_link(aml_vif, aml_sta->ch_idx, NULL);

                /* Add the station in the list of VIF's stations */
                INIT_LIST_HEAD(&aml_sta->list);
                list_add_tail(&aml_sta->list, &aml_vif->ap.sta_list);

                /* Initialize the TX queues */
                if (aml_sta->ch_idx == aml_hw->cur_chanctx) {
                    txq_status = 0;
                } else {
                    txq_status = AML_TXQ_STOP_CHAN;
                }

                aml_txq_sta_init(aml_hw, aml_sta, txq_status);
                aml_dbgfs_register_sta(aml_hw, aml_sta);

#ifdef CONFIG_AML_BFMER
                // TODO: update indication to contains vht capabilities
                if (aml_hw->mod_params->bfmer)
                    aml_send_bfmer_enable(aml_hw, aml_sta, NULL);

                aml_mu_group_sta_init(aml_sta, NULL);
#endif /* CONFIG_AML_BFMER */

            } else {
                WARN_ON(0);
            }
        } else {
            if (aml_sta->valid) {
                aml_sta->ps.active = false;
                aml_sta->valid = false;

                /* Remove the station from the list of VIF's station */
                list_del_init(&aml_sta->list);

                aml_sta_deinit(aml_hw, aml_sta);
            } else {
                WARN_ON(0);
            }
        }
    }

    return 0;
}

static inline int aml_rx_mesh_path_update_ind(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    struct mesh_path_update_ind *ind = (struct mesh_path_update_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct aml_mesh_path *mesh_path;
    bool found = false;

    AML_DBG(AML_FN_ENTRY_STR);

    if (ind->vif_idx >= NX_VIRT_DEV_MAX)
        return 1;

    if (!aml_vif || (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT))
        return 0;

    /* Look for path with provided target address */
    list_for_each_entry(mesh_path, &aml_vif->ap.mpath_list, list) {
        if (mesh_path->path_idx == ind->path_idx) {
            found = true;
            break;
        }
    }

    /* Check if element has been deleted */
    if (ind->delete) {
        if (found) {
            trace_mesh_delete_path(mesh_path);
            /* Remove element from list */
            list_del_init(&mesh_path->list);
            /* Free the element */
            kfree(mesh_path);
        }
    }
    else {
        if (found) {
            // Update the Next Hop STA
            mesh_path->nhop_sta = aml_hw->sta_table + ind->nhop_sta_idx;
            trace_mesh_update_path(mesh_path);
        } else {
            // Allocate a Mesh Path structure
            mesh_path = kmalloc(sizeof(struct aml_mesh_path), GFP_ATOMIC);

            if (mesh_path) {
                INIT_LIST_HEAD(&mesh_path->list);

                mesh_path->path_idx = ind->path_idx;
                mesh_path->nhop_sta = aml_hw->sta_table + ind->nhop_sta_idx;
                memcpy(&mesh_path->tgt_mac_addr, &ind->tgt_mac_addr, MAC_ADDR_LEN);

                // Insert the path in the list of path
                list_add_tail(&mesh_path->list, &aml_vif->ap.mpath_list);
                trace_mesh_create_path(mesh_path);
            }
        }
    }
    /* coverity[leaked_storage] - mesh_path have added to list */
    return 0;
}

static inline int aml_rx_mesh_proxy_update_ind(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    struct mesh_proxy_update_ind *ind = (struct mesh_proxy_update_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct aml_mesh_proxy *mesh_proxy;
    bool found = false;

    AML_DBG(AML_FN_ENTRY_STR);

    if (ind->vif_idx >= NX_VIRT_DEV_MAX)
        return 1;

    if (!aml_vif || (AML_VIF_TYPE(aml_vif) != NL80211_IFTYPE_MESH_POINT))
        return 0;

    /* Look for path with provided external STA address */
    list_for_each_entry(mesh_proxy, &aml_vif->ap.proxy_list, list) {
        if (!memcmp(&ind->ext_sta_addr, &mesh_proxy->ext_sta_addr, ETH_ALEN)) {
            found = true;
            break;
        }
    }

    if (ind->delete && found) {
        /* Delete mesh path */
        list_del_init(&mesh_proxy->list);
        kfree(mesh_proxy);
    } else if (!ind->delete && !found) {
        /* Allocate a Mesh Path structure */
        mesh_proxy = (struct aml_mesh_proxy *)kmalloc(sizeof(*mesh_proxy),
                                                       GFP_ATOMIC);

        if (mesh_proxy) {
            INIT_LIST_HEAD(&mesh_proxy->list);

            memcpy(&mesh_proxy->ext_sta_addr, &ind->ext_sta_addr, MAC_ADDR_LEN);
            mesh_proxy->local = ind->local;

            if (!ind->local) {
                memcpy(&mesh_proxy->proxy_addr, &ind->proxy_mac_addr, MAC_ADDR_LEN);
            }

            /* Insert the path in the list of path */
            list_add_tail(&mesh_proxy->list, &aml_vif->ap.proxy_list);
        }
    }
    /* coverity[leaked_storage] - mesh_proxy have added to list */
    return 0;
}

/***************************************************************************
 * Messages from APM task
 **************************************************************************/
static inline int aml_rx_apm_probe_client_ind(struct aml_hw *aml_hw,
                                               struct aml_cmd *cmd,
                                               struct ipc_e2a_msg *msg)
{
    struct apm_probe_client_ind *ind = (struct apm_probe_client_ind *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[ind->vif_idx];
    struct aml_sta *aml_sta = aml_hw->sta_table + ind->sta_idx;

    aml_sta->stats.last_act = jiffies;
    cfg80211_probe_status(aml_vif->ndev, aml_sta->mac_addr, (u64)ind->probe_id,
                          ind->client_present, 0, false, GFP_ATOMIC);

    return 0;
}

#endif /* CONFIG_AML_FULLMAC */

/***************************************************************************
 * Messages from DEBUG task
 **************************************************************************/
static inline int aml_rx_dbg_error_ind(struct aml_hw *aml_hw,
                                        struct aml_cmd *cmd,
                                        struct ipc_e2a_msg *msg)
{
    AML_DBG(AML_FN_ENTRY_STR);

#ifndef CONFIG_PT_MODE
    aml_error_ind(aml_hw);
#endif

    return 0;
}

static inline int aml_rx_rf_regvalue(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct rf_read_result_ind *ind = (struct rf_read_result_ind *)msg->param;

    AML_INFO(" rf_regaddr:%x, rf_regvalue:%x\n", ind->rf_addr, ind->rf_data);

    return 0;
}

#ifdef TEST_MODE
#define RX_PAYLOAD_DESC_PATTERN 0xC0DEDBAD
static inline int aml_dma_ul_result_ind(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct dma_ul_result_ind *ind = (struct dma_ul_result_ind *)msg->param;
    u32_l * read_addr;
    u8_l result = 1;
    u32_l read_cnt = 0;
    read_addr = (u32_l *)aml_hw->pcie_prssr_ul_addr;
    AML_INFO(" dma_ul_info: scr_addr: 0x%x, buffer_len:%d, payload:%x",
             ind->dest_addr, ind->len, ind->payload);

    AML_INFO("dest_addr: %x, upload: %x", ((u32_l)(read_addr)), (*read_addr));

    while (*(read_addr + (ind->len/4 -1)) != RX_PAYLOAD_DESC_PATTERN)
    {
        read_cnt++;
        if (read_cnt > 20480)
        {
            AML_ERR(" dma upload incomplete!");
            break;
        }
        msleep(1);
    }

    while (*(read_addr) != RX_PAYLOAD_DESC_PATTERN)
    {
        if (*(read_addr) != ind->payload)
        {
            AML_INFO("wrong dma_add:%x, wrong_payload:%x", read_addr, *(read_addr));
            result = false;
        }
        read_addr = read_addr + 1;
    }

    aml_ipc_buf_dealloc(aml_hw, &aml_hw->pcie_prssr_test);
    AML_INFO("pcie dma ul test_done, result: %d",result);

    return 0;
}

static inline int aml_dma_dl_result_ind(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct dma_dl_result_ind *ind = (struct dma_dl_result_ind *)msg->param;

    AML_INFO("dma_dl_result:%x, start_addr: 0x%x, buffer_len:%d, payload:%x",
            ind->result, ind->start_addr, ind->len, ind->payload);

    aml_ipc_buf_dealloc(aml_hw, &aml_hw->pcie_prssr_test);
    AML_INFO("pcie dma dl test_done");

    return 0;
}
#endif

static inline int aml_coex_get_status_ind(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct coex_get_status *ind = (struct coex_get_status *)msg->param;
    int wifi_act_sum = 0;
    int wifi_inactive_sum = 0;
    int time_sum = 0;
    int ratio = 0;

    AML_INFO("\nCoex Status Info: \n");

    if (ind->coex_state == 1)
    {
        if (ind->wifi_act_sum > 0)
        {
            wifi_act_sum = (ind->wifi_act_sum) / 1000;
            wifi_inactive_sum = (ind->wifi_inactive_sum) / 1000;
            time_sum = wifi_act_sum + wifi_inactive_sum;
        }
        if (wifi_act_sum > 0)
        {
            ratio = (wifi_act_sum * 100) / time_sum;
        }
        AML_INFO("coex work on TDD, work mode: %x; \n", ind->work_mode);
        AML_INFO("In %dms; wifi_time: %dms; bt_time: %dms; wifi ratio: %d%%\n", time_sum, wifi_act_sum, wifi_inactive_sum, ratio);

        if (ind->null_data_enable)
        {
            if (ind->null_data_send_cnt > 0)
            {
                ratio = (ind->null_data_send_succ_cnt_before_bt_s * 100) / ind->null_data_send_cnt;
            }
            AML_INFO("null data status: send cnt: %d, success cnt: %d, success ratio: %d%%\n", ind->null_data_send_cnt, ind->null_data_send_succ_cnt_before_bt_s, ratio);
        }
        else
        {
            AML_INFO("Do not need send null data now; \n");
        }
    }
    else
    {
        AML_INFO("coex work on FDD, work mode: %x; \n", ind->work_mode);
    }

    AML_INFO("\nWiFi Calibration Info: \n");
    AML_INFO("%-15s %-10d\n", "POC Cali Status:", ind->poc_cali_status);

    AML_INFO("\nLink Cali Status: \n");
    AML_INFO("%-15s %-10s %-10s\n", "     ", "is exist", "is done");
    AML_INFO("%-15s %-10d %-10d\n", "STA Vif", ((ind->link_cali_status & BIT(1))? 1 : 0), ((ind->link_cali_status & BIT(2))? 1 : 0));
    AML_INFO("%-15s %-10d %-10d\n", "SAP Vif", ((ind->link_cali_status & BIT(3))? 1 : 0), ((ind->link_cali_status & BIT(4))? 1 : 0));


    AML_INFO("\nBT Link Info: \n");
    if (ind->bt_work_status & BIT(21))
    {
        AML_INFO("BT work on ESCO mode; \n");
    }
    if (ind->bt_work_status & BIT(22))
    {
        AML_INFO("BT work on SLAVE mode; \n");
    }
    if (ind->bt_work_status & BIT(24))
    {
        AML_INFO("BT work with CLASSIC; \n");
    }
    if (ind->bt_work_status & BIT(25))
    {
        AML_INFO("BT work with BLE; \n");
    }
    if (ind->bt_work_status & BIT(26))
    {
        AML_INFO("BT REQ TDD; \n");
    }

    return 0;
}

static inline int aml_scanu_cancel_cfm(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct scanu_cancel_cfm *cfm = (struct scanu_cancel_cfm *)msg->param;
    struct aml_vif *aml_vif = aml_hw->vif_table[cfm->vif_idx];

    AML_INFO(" status:%x, vif_id=%x\n", cfm->status, cfm->vif_idx);
    aml_ipc_buf_dealloc(aml_hw, &aml_hw->scan_ie);
    spin_lock_bh(&aml_hw->scan_req_lock);
    if (aml_hw->scan_request) {
        if (aml_bus_type != PCIE_MODE)
            aml_sdio_rx_scanu_result_ind(aml_hw);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        {
            struct cfg80211_scan_info info = {
                .aborted = true,
            };

            cfg80211_scan_done(aml_hw->scan_request, &info);
        }
#else
        cfg80211_scan_done(aml_hw->scan_request, true);
#endif
    }
    aml_hw->scan_request = NULL;
    spin_unlock_bh(&aml_hw->scan_req_lock);
    if (aml_vif != NULL) {
        aml_vif->sta.cancel_scan_cfm = 1;
    }
    return 0;
}

static inline int aml_tko_conn_dead_ind(struct aml_hw *aml_hw,
                                    struct aml_cmd *cmd,
                                    struct ipc_e2a_msg *msg)
{
    struct tko_conn_dead_ind *ind = (struct tko_conn_dead_ind *)msg->param;

    AML_INFO(" vif_id=%u src_ip=%u src_port=%u dst_ip=%u dst_port=%u\n",
            ind->vif_idx, ind->src_ip, ind->src_port,
            ind->dst_ip, ind->dst_port);

    return 0;
}

static inline int aml_sched_scan_cfm(struct aml_hw *aml_hw,
                                      struct aml_cmd *cmd,
                                      struct ipc_e2a_msg *msg)
{
    struct scanu_start_cfm *cfm = (struct scanu_start_cfm *)msg->param;

    AML_INFO("status:%x, vif_id=%x, cfm->result_cnt %d, aml_hw->pno_scan_reqid %lld \n",
            cfm->status, cfm->vif_idx, cfm->result_cnt, aml_hw->pno_scan_reqid);

    if (cfm->result_cnt) {
        if (aml_bus_type != PCIE_MODE)
            aml_sdio_rx_scanu_result_ind(aml_hw);
        aml_cfg80211_sched_scan_results(aml_hw->wiphy, aml_hw->pno_scan_reqid);
    }

    return 0;
}

extern struct aml_pm_type g_wifi_pm;
static inline int aml_suspend_ind(struct aml_hw *aml_hw,
                                  struct aml_cmd *cmd,
                                  struct ipc_e2a_msg *msg)
{
    aml_hw->suspend_ind = SUSPEND_IND_RECV;
    if (aml_bus_type == USB_MODE)
    {
        atomic_set(&g_wifi_pm.drv_suspend_cnt, 1);
    }
    AML_FN_EXIT();
    return 0;
}

static int aml_dhcp_offload_ind(struct aml_hw *aml_hw,
                                           struct aml_cmd *cmd,
                                           struct ipc_e2a_msg *msg)
{
    struct dhcp_to_host_ind *fw_event = (struct dhcp_to_host_ind *)msg->param;
    struct in_device *in_dev;
    struct in_ifaddr *ifa_v4;
    unsigned int local_ip = 0;
    struct aml_vif *aml_vif;
    bool found = false;

    if (fw_event == NULL) {
        return -1;
    }

    list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
        if (!aml_vif->up || aml_vif->ndev == NULL) {
            continue;
        }

        if (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_STATION) {
            found = true;
            break;
        }
    }

    if (!found) {
        AML_ERR("%s %d\n", __func__, __LINE__);
        return -1;
    }

    local_ip = *(__be32 *)(&fw_event->local_ip);
    in_dev = __in_dev_get_rtnl(aml_vif->ndev);

    if (in_dev) {
        ifa_v4 = in_dev->ifa_list;
        if (ifa_v4 != NULL && local_ip != 0) {
            ifa_v4->ifa_local = local_ip;
            ifa_v4->ifa_address = *(__be32 *)(&fw_event->server_ip);
            ifa_v4->ifa_mask = *(__be32 *)(&fw_event->subnet_mask);
            AML_INFO(" %x %x %x\n", ifa_v4->ifa_local,ifa_v4->ifa_address,ifa_v4->ifa_mask);
        }
    }

    return 0;
}

static inline int aml_traffic_busy_ind(struct aml_hw *aml_hw,
                                         struct aml_cmd *cmd,
                                         struct ipc_e2a_msg *msg)
{
    struct traffic_busy *traffic = (struct traffic_busy *)msg->param;

    switch (traffic->td_flag) {
    case TRAFFIC_CPU_FLAG:
        aml_hw->traffic_busy = traffic->traffic_busy_flag;
        if (traffic->traffic_busy_flag)
            aml_cpufreq_boost_update(aml_hw);
        else
            aml_cpufreq_boost_remove(aml_hw);
        aml_set_tcp_ack_auto(aml_hw);
        break;
    case TRAFFIC_SCAN_FLAG:
        if (traffic->traffic_busy_flag) {
            aml_hw->scan_abort_flag = 1;
            aml_hw->trb_wait_time = (USB_SEND_URB_DEFAULT_WAIT_TIME / 50);
            coex_flag |= BIT(31);
            AML_INFO("traffic busy!!\n");
        } else {
            aml_hw->scan_abort_flag = 0;
            aml_hw->trb_wait_time = USB_SEND_URB_DEFAULT_WAIT_TIME;
            coex_flag &= ~BIT(31);
            AML_INFO("traffic idle!!\n");
        }
        break;
    default:
        break;
    }

    return 0;
}

//can stop or restore all alive vif txq
static inline int aml_rx_coexist_stop_restore_txq_ind(struct aml_hw *aml_hw,
                                              struct aml_cmd *cmd,
                                              struct ipc_e2a_msg *msg)
{

    struct aml_vif *aml_vif;

    int wifi_inactive_status = ((struct coex_stop_restore_txq_ind *)msg->param)->wifi_inactive_flag;

    if (wifi_inactive_status == 1)
    {
        list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
            if (aml_vif->up) {
                aml_txq_vif_stop(aml_vif, AML_TXQ_STOP_COEX_INACTIVE);
            }
        }
    }
    else
    {
        list_for_each_entry(aml_vif, &aml_hw->vifs, list) {
            if (aml_vif->up) {
                if (aml_hw->show_switch_info > 0) {
                    AML_INFO("coex txq_start,vif:%d vif_chan:%d",aml_vif->vif_index,aml_vif->ch_index);
                    aml_hw->show_switch_info--;
                }
                aml_txq_vif_start(aml_vif, AML_TXQ_STOP_COEX_INACTIVE);
            }
        }
    }

    return 0;
}

#ifdef CONFIG_AML_SOFTMAC

static msg_cb_fct mm_hdlrs[MSG_I(MM_MAX)] = {
    [MSG_I(MM_CONNECTION_LOSS_IND)]       = aml_rx_connection_loss_ind,
    [MSG_I(MM_CHANNEL_SWITCH_IND)]        = aml_rx_chan_switch_ind,
    [MSG_I(MM_CHANNEL_PRE_SWITCH_IND)]    = aml_rx_chan_pre_switch_ind,
    [MSG_I(MM_REMAIN_ON_CHANNEL_EXP_IND)] = aml_rx_remain_on_channel_exp_ind,
#ifdef CONFIG_AML_BCN
    [MSG_I(MM_PRIMARY_TBTT_IND)]          = aml_rx_prm_tbtt_ind,
#endif
    [MSG_I(MM_P2P_VIF_PS_CHANGE_IND)]     = aml_rx_p2p_vif_ps_change_ind,
    [MSG_I(MM_CSA_COUNTER_IND)]           = aml_rx_csa_counter_ind,
    [MSG_I(MM_CHANNEL_SURVEY_IND)]        = aml_rx_channel_survey_ind,
    [MSG_I(MM_RSSI_STATUS_IND)]           = aml_rx_rssi_status_ind,
};

static msg_cb_fct scan_hdlrs[MSG_I(SCAN_MAX)] = {
    [MSG_I(SCAN_DONE_IND)]                = aml_rx_scan_done_ind,
};

#else  /* CONFIG_AML_FULLMAC */

static msg_cb_fct mm_hdlrs[MSG_I(MM_MAX)] = {
    [MSG_I(MM_BA_ADD_REQ)]             = aml_rx_mm_ba_add_ind,
    [MSG_I(MM_BA_ADD_CFM)]             = aml_rx_mm_ba_add_cfm,
    [MSG_I(MM_BA_DEL_REQ)]             = aml_rx_mm_ba_del_ind,
    [MSG_I(MM_BA_DEL_CFM)]             = aml_rx_mm_ba_del_cfm,
    [MSG_I(MM_CHANNEL_SWITCH_IND)]     = aml_rx_chan_switch_ind,
    [MSG_I(MM_CHANNEL_PRE_SWITCH_IND)] = aml_rx_chan_pre_switch_ind,
    [MSG_I(MM_REMAIN_ON_CHANNEL_EXP_IND)] = aml_rx_remain_on_channel_exp_ind,
    [MSG_I(MM_PS_CHANGE_IND)]          = aml_rx_ps_change_ind,
    [MSG_I(MM_TRAFFIC_REQ_IND)]        = aml_rx_traffic_req_ind,
    [MSG_I(MM_P2P_VIF_PS_CHANGE_IND)]  = aml_rx_p2p_vif_ps_change_ind,
    [MSG_I(MM_CSA_COUNTER_IND)]        = aml_rx_csa_counter_ind,
    [MSG_I(MM_CSA_FINISH_IND)]         = aml_rx_csa_finish_ind,
    [MSG_I(MM_CSA_TRAFFIC_IND)]        = aml_rx_csa_traffic_ind,
    [MSG_I(MM_CHANNEL_SURVEY_IND)]     = aml_rx_channel_survey_ind,
    [MSG_I(MM_P2P_NOA_UPD_IND)]        = aml_rx_p2p_noa_upd_ind,
    [MSG_I(MM_RSSI_STATUS_IND)]        = aml_rx_rssi_status_ind,
    [MSG_I(MM_PKTLOSS_IND)]            = aml_rx_pktloss_notify_ind,
};

static msg_cb_fct scan_hdlrs[MSG_I(SCANU_MAX)] = {
    [MSG_I(SCANU_START_CFM)]           = aml_rx_scanu_start_cfm,
    [MSG_I(SCANU_RESULT_IND)]          = aml_rx_scanu_result_ind,
};

static msg_cb_fct me_hdlrs[MSG_I(ME_MAX)] = {
    [MSG_I(ME_TKIP_MIC_FAILURE_IND)] = aml_rx_me_tkip_mic_failure_ind,
    [MSG_I(ME_TX_CREDITS_UPDATE_IND)] = aml_rx_me_tx_credits_update_ind,
};

static msg_cb_fct sm_hdlrs[MSG_I(SM_MAX)] = {
    [MSG_I(SM_CONNECT_IND)]    = aml_rx_sm_connect_ind,
    [MSG_I(SM_DISCONNECT_IND)] = aml_rx_sm_disconnect_ind,
    [MSG_I(SM_EXTERNAL_AUTH_REQUIRED_IND)] = aml_rx_sm_external_auth_required_ind,
    [MSG_I(SM_FT_AUTH_IND)] = aml_rx_sm_ft_auth_ind,
};

static msg_cb_fct apm_hdlrs[MSG_I(APM_MAX)] = {
    [MSG_I(APM_PROBE_CLIENT_IND)] = aml_rx_apm_probe_client_ind,
};

static msg_cb_fct twt_hdlrs[MSG_I(TWT_MAX)] = {
    [MSG_I(TWT_SETUP_IND)]    = aml_rx_twt_setup_ind,
};

static msg_cb_fct mesh_hdlrs[MSG_I(MESH_MAX)] = {
    [MSG_I(MESH_PATH_CREATE_CFM)]  = aml_rx_mesh_path_create_cfm,
    [MSG_I(MESH_PEER_UPDATE_IND)]  = aml_rx_mesh_peer_update_ind,
    [MSG_I(MESH_PATH_UPDATE_IND)]  = aml_rx_mesh_path_update_ind,
    [MSG_I(MESH_PROXY_UPDATE_IND)] = aml_rx_mesh_proxy_update_ind,
};

#endif /* CONFIG_AML_SOFTMAC */

static msg_cb_fct dbg_hdlrs[MSG_I(DBG_MAX)] = {
    [MSG_I(DBG_ERROR_IND)]                = aml_rx_dbg_error_ind,
};

static msg_cb_fct tdls_hdlrs[MSG_I(TDLS_MAX)] = {
    [MSG_I(TDLS_CHAN_SWITCH_CFM)] = aml_rx_tdls_chan_switch_cfm,
    [MSG_I(TDLS_CHAN_SWITCH_IND)] = aml_rx_tdls_chan_switch_ind,
    [MSG_I(TDLS_CHAN_SWITCH_BASE_IND)] = aml_rx_tdls_chan_switch_base_ind,
    [MSG_I(TDLS_PEER_PS_IND)] = aml_rx_tdls_peer_ps_ind,
};

static msg_cb_fct priv_hdlrs[MSG_I(PRIV_SUB_E2A_MAX)] = {
    [MSG_I(PRIV_SCANU_CANCEL_CFM)]  = aml_scanu_cancel_cfm,
    [MSG_I(PRIV_TKO_CONN_DEAD_IND)] = aml_tko_conn_dead_ind,
    [MSG_I(PRIV_SCHED_SCAN_CFM)]    = aml_sched_scan_cfm,
    [MSG_I(PRIV_DHCP_OFFLOAD_IND)]  = aml_dhcp_offload_ind,
#ifdef TEST_MODE
    [MSG_I(PRIV_DMA_UL_RESULT)]     = aml_dma_ul_result_ind,
    [MSG_I(PRIV_DMA_DL_RESULT)]     = aml_dma_dl_result_ind,
#endif
    [MSG_I(PRIV_CONNECT_INFO)]      = aml_rx_sm_connect_ind_ex,
    [MSG_I(PRIV_SET_SUSPEND_IND)]   = aml_suspend_ind,
    [MSG_I(PRIV_APM_DIS_STA_IND)]   = aml_apm_handle_disconnect_sta,
    [MSG_I(PRIV_FT_AUTH_RSP_TIMEOUT_IND)] = aml_rx_sm_ft_auth_rsp_timeout_ind,
    [MSG_I(PRIV_TRAFFIC_BUSY_IND)]    = aml_traffic_busy_ind,
    [MSG_I(PRIV_SCANU_RESULT_IND)]    = aml_sdio_rx_scanu_result_for_join_ind,
    [MSG_I(PRIV_COEX_STOP_RESTORE_TXQ_IND)] = aml_rx_coexist_stop_restore_txq_ind,
    [MSG_I(PRIV_COEX_GET_STATUS)]     = aml_coex_get_status_ind,
};

#define MSG_SUB_HDLRS(_sub_hdlrs)       { ARRAY_SIZE(_sub_hdlrs), _sub_hdlrs, }
static const struct msg_hdlr_info {
    uint32_t max_index;
    msg_cb_fct *hdlrs;
} msg_hdlrs[] = {
    [TASK_MM]    = MSG_SUB_HDLRS(mm_hdlrs),
    [TASK_PRIV]  = MSG_SUB_HDLRS(priv_hdlrs),
    [TASK_DBG]   = MSG_SUB_HDLRS(dbg_hdlrs),
#ifdef CONFIG_AML_SOFTMAC
    [TASK_SCAN]  = MSG_SUB_HDLRS(scan_hdlrs),
    [TASK_TDLS]  = MSG_SUB_HDLRS(tdls_hdlrs),
#else
    [TASK_TDLS]  = MSG_SUB_HDLRS(tdls_hdlrs),
    [TASK_SCANU] = MSG_SUB_HDLRS(scan_hdlrs),
    [TASK_ME]    = MSG_SUB_HDLRS(me_hdlrs),
    [TASK_SM]    = MSG_SUB_HDLRS(sm_hdlrs),
    [TASK_APM]   = MSG_SUB_HDLRS(apm_hdlrs),
    [TASK_MESH]  = MSG_SUB_HDLRS(mesh_hdlrs),
    [TASK_TWT]   = MSG_SUB_HDLRS(twt_hdlrs),
#endif /* CONFIG_AML_SOFTMAC */
};

/**
 *
 */
void aml_rx_handle_msg(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg)
{
    int type = MSG_T(msg->id);
    int index = MSG_I(msg->id);

    if (WARN_ON(type >= ARRAY_SIZE(msg_hdlrs)) ||
        WARN_ON(index >= msg_hdlrs[type].max_index)) {
        pr_err("%s: msg id %x (type %d, index %d) is out of rang!\n",
                __func__, msg->id, type, index);
        return;
    }
    aml_hw->cmd_mgr.msgind(&aml_hw->cmd_mgr, msg, msg_hdlrs[type].hdlrs[index]);
}

void aml_rx_sdio_ind_msg_handle(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg)
{
    if (msg->id == MM_CHANNEL_PRE_SWITCH_IND) {
        aml_rx_chan_pre_switch_ind(aml_hw, NULL, msg);
    } else if (msg->id == MM_CHANNEL_SWITCH_IND) {
        aml_rx_chan_switch_ind(aml_hw, NULL, msg);
    }

    return;
}
