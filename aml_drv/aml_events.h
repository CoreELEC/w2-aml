/**
 ******************************************************************************
 *
 * @file aml_events.h
 *
 * @brief Trace events definition
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM aml

#if !defined(_AML_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _AML_EVENTS_H

#include <linux/tracepoint.h>
#ifndef CONFIG_AML_FHOST
#include "aml_tx.h"
#endif
#include "aml_compat.h"
#include "aml_p2p.h"

/*****************************************************************************
 * TRACE function for MGMT TX (FULLMAC)
 ****************************************************************************/
#ifdef CONFIG_AML_FULLMAC
#include "linux/ieee80211.h"
#if defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS)
#include <linux/trace_seq.h>

/* P2P Public Action Frames Definitions (see WiFi P2P Technical Specification, section 4.2.8) */
/* IEEE 802.11 Public Action Usage Category - Define P2P public action frames */
#define MGMT_ACTION_PUBLIC_CAT              (0x04)
/* Offset of OUI Subtype field in Vendor Public Action Frame format */
#define MGMT_ACTION_OUI_SUBTYPE_OFFSET (5)

const char *ftrace_print_mgmt_info(struct trace_seq *p, u16 frame_control, u8 cat, u8 type, u8 vendor) {
    const char *ret = trace_seq_buffer_ptr(p);

    switch (frame_control & IEEE80211_FCTL_STYPE) {
        case (IEEE80211_STYPE_ASSOC_REQ): trace_seq_printf(p, "Association Request"); break;
        case (IEEE80211_STYPE_ASSOC_RESP): trace_seq_printf(p, "Association Response"); break;
        case (IEEE80211_STYPE_REASSOC_REQ): trace_seq_printf(p, "Reassociation Request"); break;
        case (IEEE80211_STYPE_REASSOC_RESP): trace_seq_printf(p, "Reassociation Response"); break;
        case (IEEE80211_STYPE_PROBE_REQ): trace_seq_printf(p, "Probe Request"); break;
        case (IEEE80211_STYPE_PROBE_RESP): trace_seq_printf(p, "Probe Response"); break;
        case (IEEE80211_STYPE_BEACON): trace_seq_printf(p, "Beacon"); break;
        case (IEEE80211_STYPE_ATIM): trace_seq_printf(p, "ATIM"); break;
        case (IEEE80211_STYPE_DISASSOC): trace_seq_printf(p, "Disassociation"); break;
        case (IEEE80211_STYPE_AUTH): trace_seq_printf(p, "Authentication"); break;
        case (IEEE80211_STYPE_DEAUTH): trace_seq_printf(p, "Deauthentication"); break;
        case (IEEE80211_STYPE_ACTION):
            trace_seq_printf(p, "Action");
            if (vendor)
                // vendor != 0 means the frame is a Public Action frame, with type set to 'Vendor Specific'
                // In this case 'vendor' is the OUI subtype (assuming OUI==WFA),
                // 'cat' is the 1 first after OUI and 'type' the second byte
                if (vendor == 0x09) {
                    switch (cat) {
                        case (P2P_ACTION_GO_NEG_REQ): trace_seq_printf(p, ": GO Negociation Request"); break;
                        case (P2P_ACTION_GO_NEG_RSP): trace_seq_printf(p, ": GO Negociation Response"); break;
                        case (P2P_ACTION_GO_NEG_CFM): trace_seq_printf(p, ": GO Negociation Confirmation"); break;
                        case (P2P_ACTION_INVIT_REQ): trace_seq_printf(p, ": P2P Invitation Request"); break;
                        case (P2P_ACTION_INVIT_RSP): trace_seq_printf(p, ": P2P Invitation Response"); break;
                        case (P2P_ACTION_DEV_DISC_REQ): trace_seq_printf(p, ": Device Discoverability Request"); break;
                        case (P2P_ACTION_DEV_DISC_RSP): trace_seq_printf(p, ": Device Discoverability Response"); break;
                        case (P2P_ACTION_PROV_DISC_REQ): trace_seq_printf(p, ": Provision Discovery Request"); break;
                        case (P2P_ACTION_PROV_DISC_RSP): trace_seq_printf(p, ": Provision Discovery Response"); break;
                        default: trace_seq_printf(p, "Unknown p2p %d", type); break;
                    }
                } else if (vendor == 0x1a) {
                    trace_seq_printf(p, ": DPP %d", type); break;
                } else {
                    trace_seq_printf(p, "Unknown vendor Public action  0x%x:%d", vendor, type); break;
                }
            else {
                switch (cat) {
                    case 0: trace_seq_printf(p, ":Spectrum %d", type); break;
                    case 1: trace_seq_printf(p, ":QOS %d", type); break;
                    case 2: trace_seq_printf(p, ":DLS %d", type); break;
                    case 3: trace_seq_printf(p, ":BA %d", type); break;
                    case 4: trace_seq_printf(p, ":Public %d", type); break;
                    case 5: trace_seq_printf(p, ":Radio Measure %d", type); break;
                    case 6: trace_seq_printf(p, ":Fast BSS %d", type); break;
                    case 7: trace_seq_printf(p, ":HT Action %d", type); break;
                    case 8: trace_seq_printf(p, ":SA Query %d", type); break;
                    case 9: trace_seq_printf(p, ":Protected Public %d", type); break;
                    case 10: trace_seq_printf(p, ":WNM %d", type); break;
                    case 11: trace_seq_printf(p, ":Unprotected WNM %d", type); break;
                    case 12: trace_seq_printf(p, ":TDLS %d", type); break;
                    case 13: trace_seq_printf(p, ":Mesh %d", type); break;
                    case 14: trace_seq_printf(p, ":MultiHop %d", type); break;
                    case 15: trace_seq_printf(p, ":Self Protected %d", type); break;
                    case 126: trace_seq_printf(p, ":Vendor protected"); break;
                    case 127: trace_seq_printf(p, ":Vendor"); break;
                    default: trace_seq_printf(p, ":Unknown category %d", cat); break;
                }
            }
            break;
        default: trace_seq_printf(p, "Unknown subtype %d", frame_control & IEEE80211_FCTL_STYPE); break;
    }

    trace_seq_putc(p, 0);

    return ret;
}
#endif /* defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS) */

#undef __print_mgmt_info
#define __print_mgmt_info(frame_control, cat, type, p2p) ftrace_print_mgmt_info(p, frame_control, cat, type, p2p)

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    roc,
    TP_PROTO(u8 vif_idx, u16 freq, unsigned int duration),
    TP_ARGS(vif_idx, freq, duration),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
        __field(u16, freq)
        __field(unsigned int, duration)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
        __entry->freq = freq;
        __entry->duration = duration;
                   ),
    TP_printk("f=%d vif=%d dur=%d",
            __entry->freq, __entry->vif_idx, __entry->duration)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    cancel_roc,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    roc_exp,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    switch_roc,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    mgmt_template,
    TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
    TP_ARGS(freq, vif_idx, sta_idx, mgmt),
    TP_STRUCT__entry(
        __field(u16, freq)
        __field(u8, vif_idx)
        __field(u8, sta_idx)
        __field(u16, frame_control)
        __field(u8, action_cat)
        __field(u8, action_type)
        __field(u8, action_vendor)
                     ),
    TP_fast_assign(
        __entry->freq = freq;
        __entry->vif_idx = vif_idx;
        __entry->sta_idx = sta_idx;
        __entry->frame_control = mgmt->frame_control;
        if ((mgmt->u.action.category == MGMT_ACTION_PUBLIC_CAT) &&
            (mgmt->u.action.u.wme_action.action_code == 0x9)) {
            __entry->action_vendor = *((u8 *)&mgmt->u.action.category
                                       + MGMT_ACTION_OUI_SUBTYPE_OFFSET);
            __entry->action_cat = *((u8 *)&mgmt->u.action.category
                                     + MGMT_ACTION_OUI_SUBTYPE_OFFSET + 1);
            __entry->action_type = *((u8 *)&mgmt->u.action.category
                                     + MGMT_ACTION_OUI_SUBTYPE_OFFSET + 2);
        } else {
            __entry->action_cat = mgmt->u.action.category;
            __entry->action_type = mgmt->u.action.u.wme_action.action_code;
            __entry->action_vendor = 0;
        }
                   ),
    TP_printk("f=%d vif=%d sta=%d -> %s",
            __entry->freq, __entry->vif_idx, __entry->sta_idx,
              __print_mgmt_info(__entry->frame_control, __entry->action_cat,
                                __entry->action_type, __entry->action_vendor))
);

DEFINE_EVENT(mgmt_template, mgmt_tx,
             TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
             TP_ARGS(freq, vif_idx, sta_idx, mgmt));

DEFINE_EVENT(mgmt_template, mgmt_rx,
             TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
             TP_ARGS(freq, vif_idx, sta_idx, mgmt));

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    mgmt_cfm,
    TP_PROTO(u8 vif_idx, u8 sta_idx, bool acked),
    TP_ARGS(vif_idx, sta_idx, acked),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
        __field(u8, sta_idx)
        __field(bool, acked)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
        __entry->sta_idx = sta_idx;
        __entry->acked = acked;
                   ),
    TP_printk("vif=%d sta=%d ack=%d",
            __entry->vif_idx, __entry->sta_idx, __entry->acked)
);
#endif /* CONFIG_AML_FULLMAC */

/*****************************************************************************
 * TRACE function for TXQ
 ****************************************************************************/
#ifndef CONFIG_AML_FHOST
#if defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS)

#include <linux/trace_seq.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
#include <linux/trace_events.h>
#else
#include <linux/ftrace_event.h>
#endif

const char *
ftrace_print_txq(struct trace_seq *p, int txq_idx) {
    const char *ret = trace_seq_buffer_ptr(p);

    if (txq_idx == TXQ_INACTIVE) {
        trace_seq_printf(p, "[INACTIVE]");
    } else if (txq_idx < NX_FIRST_VIF_TXQ_IDX) {
        trace_seq_printf(p, "[STA %d/%d]",
                         txq_idx / NX_NB_TXQ_PER_STA,
                         txq_idx % NX_NB_TXQ_PER_STA);
#ifdef CONFIG_AML_FULLMAC
    } else if (txq_idx < NX_FIRST_UNK_TXQ_IDX) {
        trace_seq_printf(p, "[BC/MC %d]",
                         txq_idx - NX_FIRST_BCMC_TXQ_IDX);
    } else if (txq_idx < NX_OFF_CHAN_TXQ_IDX) {
        trace_seq_printf(p, "[UNKNOWN %d]",
                         txq_idx - NX_FIRST_UNK_TXQ_IDX);
    } else if (txq_idx == NX_OFF_CHAN_TXQ_IDX) {
        trace_seq_printf(p, "[OFFCHAN]");
#else
    } else if (txq_idx < NX_NB_TXQ) {
        txq_idx -= NX_FIRST_VIF_TXQ_IDX;
        trace_seq_printf(p, "[VIF %d/%d]",
                         txq_idx / NX_NB_TXQ_PER_VIF,
                         txq_idx % NX_NB_TXQ_PER_VIF);
#endif
    } else {
        trace_seq_printf(p, "[ERROR %d]", txq_idx);
    }

    trace_seq_putc(p, 0);

    return ret;
}

const char *
ftrace_print_sta(struct trace_seq *p, int sta_idx) {
    const char *ret = trace_seq_buffer_ptr(p);

    if (sta_idx < NX_REMOTE_STA_MAX) {
        trace_seq_printf(p, "[STA %d]", sta_idx);
    } else {
        trace_seq_printf(p, "[BC/MC %d]", sta_idx - NX_REMOTE_STA_MAX);
    }

    trace_seq_putc(p, 0);

    return ret;
}

const char *
ftrace_print_hwq(struct trace_seq *p, int hwq_idx) {

    static const struct trace_print_flags symbols[] =
        {{AML_HWQ_BK, "BK"},
         {AML_HWQ_BE, "BE"},
         {AML_HWQ_VI, "VI"},
         {AML_HWQ_VO, "VO"},
#ifdef CONFIG_AML_FULLMAC
         {AML_HWQ_BCMC, "BCMC"},
#else
         {AML_HWQ_BCN, "BCN"},
#endif
         { -1, NULL }};
    return trace_print_symbols_seq(p, hwq_idx, symbols);
}

const char *
ftrace_print_hwq_cred(struct trace_seq *p, u8 *cred) {
    const char *ret = trace_seq_buffer_ptr(p);

#if CONFIG_USER_MAX == 1
    trace_seq_printf(p, "%d", cred[0]);
#else
    int i;

    for (i = 0; i < CONFIG_USER_MAX - 1; i++)
        trace_seq_printf(p, "%d-", cred[i]);
    trace_seq_printf(p, "%d", cred[i]);
#endif

    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_mu_info(struct trace_seq *p, u8 mu_info) {
    const char *ret = trace_seq_buffer_ptr(p);

    if (mu_info)
        trace_seq_printf(p, "MU: %d-%d", (mu_info & 0x3f), (mu_info >> 6));

    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_mu_group(struct trace_seq *p, int nb_user, u8 *users) {
    const char *ret = trace_seq_buffer_ptr(p);
#if CONFIG_USER_MAX > 1
    int i;
#endif

    if (users[0] != 0xff)
        trace_seq_printf(p, "(%d", users[0]);
    else
        trace_seq_printf(p, "(-");
#if CONFIG_USER_MAX > 1
    for (i = 1; i < CONFIG_USER_MAX ; i++) {
        if (users[i] != 0xff)
            trace_seq_printf(p, ",%d", users[i]);
        else
            trace_seq_printf(p, ",-");
    }
#endif
    trace_seq_printf(p, ")");
    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_amsdu(struct trace_seq *p, u16 nb_pkt) {
    const char *ret = trace_seq_buffer_ptr(p);

    if (nb_pkt > 1)
        trace_seq_printf(p, "(AMSDU %d)", nb_pkt);

    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_sn(struct trace_seq *p, u16 sn) {
    const char *ret = trace_seq_buffer_ptr(p);

    if (sn < 4096)
        trace_seq_printf(p, "(SN %d) ", sn);

    trace_seq_putc(p, 0);
    return ret;
}

#ifdef CONFIG_AML_SOFTMAC
#include <net/mac80211.h>
const char *
ftrace_print_mac80211_flag(struct trace_seq *p, u32 flags) {

    static const struct trace_print_flags mac80211_flags[] = {
        {IEEE80211_TX_CTL_REQ_TX_STATUS       , "CTL_REQ_TX_STATUS"},
        {IEEE80211_TX_CTL_ASSIGN_SEQ          , "CTL_ASSIGN_SEQ"},
        {IEEE80211_TX_CTL_NO_ACK              , "CTL_NO_ACK"},
        {IEEE80211_TX_CTL_CLEAR_PS_FILT       , "CTL_CLEAR_PS_FILT"},
        {IEEE80211_TX_CTL_FIRST_FRAGMENT      , "CTL_FIRST_FRAGMENT"},
        {IEEE80211_TX_CTL_SEND_AFTER_DTIM     , "CTL_SEND_AFTER_DTIM"},
        {IEEE80211_TX_CTL_AMPDU               , "CTL_AMPDU"},
        {IEEE80211_TX_CTL_INJECTED            , "CTL_INJECTED"},
        {IEEE80211_TX_STAT_TX_FILTERED        , "STAT_TX_FILTERED"},
        {IEEE80211_TX_STAT_ACK                , "STAT_ACK"},
        {IEEE80211_TX_STAT_AMPDU              , "STAT_AMPDU"},
        {IEEE80211_TX_STAT_AMPDU_NO_BACK      , "STAT_AMPDU_NO_BACK"},
        {IEEE80211_TX_CTL_RATE_CTRL_PROBE     , "CTL_RATE_CTRL_PROBE"},
        {IEEE80211_TX_INTFL_OFFCHAN_TX_OK     , "INTFL_OFFCHAN_TX_OK"},
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
        {IEEE80211_TX_INTFL_NEED_TXPROCESSING , "INTFL_NEED_TXPROCESSING"},
#else
        {IEEE80211_TX_CTL_HW_80211_ENCAP      , "CTL_HW_80211_ENCAP"},
#endif
        {IEEE80211_TX_INTFL_RETRIED           , "INTFL_RETRIED"},
        {IEEE80211_TX_INTFL_DONT_ENCRYPT      , "INTFL_DONT_ENCRYPT"},
        {IEEE80211_TX_CTL_NO_PS_BUFFER        , "CTL_NO_PS_BUFFER"},
        {IEEE80211_TX_CTL_MORE_FRAMES         , "CTL_MORE_FRAMES"},
        {IEEE80211_TX_INTFL_RETRANSMISSION    , "INTFL_RETRANSMISSION"},
        {IEEE80211_TX_INTFL_MLME_CONN_TX      , "INTFL_MLME_CONN_TX"},
        {IEEE80211_TX_INTFL_NL80211_FRAME_TX  , "INTFL_NL80211_FRAME_TX"},
        {IEEE80211_TX_CTL_LDPC                , "CTL_LDPC"},
        {IEEE80211_TX_CTL_STBC                , "CTL_STBC"},
        {IEEE80211_TX_CTL_TX_OFFCHAN          , "CTL_TX_OFFCHAN"},
        {IEEE80211_TX_INTFL_TKIP_MIC_FAILURE  , "INTFL_TKIP_MIC_FAILURE"},
        {IEEE80211_TX_CTL_NO_CCK_RATE         , "CTL_NO_CCK_RATE"},
        {IEEE80211_TX_STATUS_EOSP             , "STATUS_EOSP"},
        {IEEE80211_TX_CTL_USE_MINRATE         , "CTL_USE_MINRATE"},
        {IEEE80211_TX_CTL_DONTFRAG            , "CTL_DONTFRAG"},
        {IEEE80211_TX_STAT_NOACK_TRANSMITTED  , "STATUS_NOACK_TRANSMIT"},
        { -1, NULL }};

    return trace_print_flags_seq(p, "|", flags, mac80211_flags);
}

#undef __print_mac_flags
#define __print_mac_flags(flags) ftrace_print_mac80211_flag(p, flags)

#endif /* CONFIG_AML_SOFTMAC */

#undef __print_txq
#define __print_txq(txq_idx) ftrace_print_txq(p, txq_idx)

#undef __print_sta
#define __print_sta(sta_idx) ftrace_print_sta(p, sta_idx)

#undef __print_hwq
#define __print_hwq(hwq) ftrace_print_hwq(p, hwq)

#undef __print_hwq_cred
#define __print_hwq_cred(cred) ftrace_print_hwq_cred(p, cred)

#undef __print_mu_info
#define __print_mu_info(mu_info) ftrace_print_mu_info(p, mu_info)

#undef __print_mu_group
#define __print_mu_group(nb, users) ftrace_print_mu_group(p, nb, users)

#undef __print_amsdu
#define __print_amsdu(nb_pkt) ftrace_print_amsdu(p, nb_pkt)

#undef __print_sn
#define __print_sn(sn) ftrace_print_sn(p, sn)
#endif /* defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS) */

#ifdef CONFIG_AML_FULLMAC

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_select,
    TP_PROTO(int txq_idx, u16 pkt_ready_up, struct sk_buff *skb),
    TP_ARGS(txq_idx, pkt_ready_up, skb),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, pkt_ready)
        __field(struct sk_buff *, skb)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq_idx;
        __entry->pkt_ready = pkt_ready_up;
        __entry->skb = skb;
                   ),
    TP_printk("%s pkt_ready_up=%d skb=%p", __print_txq(__entry->txq_idx),
              __entry->pkt_ready, __entry->skb)
);

#endif /* CONFIG_AML_FULLMAC */

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    hwq_template,
    TP_PROTO(u8 hwq_idx),
    TP_ARGS(hwq_idx),
    TP_STRUCT__entry(
        __field(u8, hwq_idx)
                     ),
    TP_fast_assign(
        __entry->hwq_idx = hwq_idx;
                   ),
    TP_printk("%s", __print_hwq(__entry->hwq_idx))
);

DEFINE_EVENT(hwq_template, hwq_flowctrl_stop,
             TP_PROTO(u8 hwq_idx),
             TP_ARGS(hwq_idx));

DEFINE_EVENT(hwq_template, hwq_flowctrl_start,
             TP_PROTO(u8 hwq_idx),
             TP_ARGS(hwq_idx));


/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    txq_template,
    TP_PROTO(struct aml_txq *txq),
    TP_ARGS(txq),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
                   ),
    TP_printk("%s", __print_txq(__entry->txq_idx))
);

DEFINE_EVENT(txq_template, txq_add_to_hw,
             TP_PROTO(struct aml_txq *txq),
             TP_ARGS(txq));

DEFINE_EVENT(txq_template, txq_del_from_hw,
             TP_PROTO(struct aml_txq *txq),
             TP_ARGS(txq));

#ifdef CONFIG_AML_FULLMAC

DEFINE_EVENT(txq_template, txq_flowctrl_stop,
             TP_PROTO(struct aml_txq *txq),
             TP_ARGS(txq));

DEFINE_EVENT(txq_template, txq_flowctrl_restart,
             TP_PROTO(struct aml_txq *txq),
             TP_ARGS(txq));

#endif  /* CONFIG_AML_FULLMAC */

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    process_txq,
    TP_PROTO(struct aml_txq *txq),
    TP_ARGS(txq),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, len)
        __field(u16, len_retry)
        __field(s8, credit)
        #ifdef CONFIG_AML_FULLMAC
        __field(u16, limit)
        #endif /* CONFIG_AML_FULLMAC*/
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->len = skb_queue_len(&txq->sk_list);
        #ifdef CONFIG_MAC80211_TXQ
        __entry->len += txq->nb_ready_mac80211;
        #endif
        __entry->len_retry = txq->nb_retry;
        __entry->credit = txq->credits;
        #ifdef CONFIG_AML_FULLMAC
        __entry->limit = txq->push_limit;
        #endif /* CONFIG_AML_FULLMAC*/
                   ),

    #ifdef CONFIG_AML_FULLMAC
    TP_printk("%s txq_credits=%d, len=%d, retry_len=%d, push_limit=%d",
              __print_txq(__entry->txq_idx), __entry->credit,
              __entry->len, __entry->len_retry, __entry->limit)
    #else
    TP_printk("%s txq_credits=%d, len=%d, retry_len=%d",
              __print_txq(__entry->txq_idx), __entry->credit,
              __entry->len, __entry->len_retry)
    #endif /* CONFIG_AML_FULLMAC*/
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    txq_reason_template,
    TP_PROTO(struct aml_txq *txq, u16 reason),
    TP_ARGS(txq, reason),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, reason)
        __field(u16, status)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->reason = reason;
        __entry->status = txq->status;
                   ),
    TP_printk("%s reason=%s status=%s",
              __print_txq(__entry->txq_idx),
              __print_symbolic(__entry->reason,
                               {AML_TXQ_STOP_FULL, "FULL"},
                               {AML_TXQ_STOP_CSA, "CSA"},
                               {AML_TXQ_STOP_STA_PS, "PS"},
                               {AML_TXQ_STOP_VIF_PS, "VPS"},
                               {AML_TXQ_STOP_CHAN, "CHAN"},
                               {AML_TXQ_STOP_MU_POS, "MU"}),
              __print_flags(__entry->status, "|",
                            {AML_TXQ_IN_HWQ_LIST, "IN LIST"},
                            {AML_TXQ_STOP_FULL, "FULL"},
                            {AML_TXQ_STOP_CSA, "CSA"},
                            {AML_TXQ_STOP_STA_PS, "PS"},
                            {AML_TXQ_STOP_VIF_PS, "VPS"},
                            {AML_TXQ_STOP_CHAN, "CHAN"},
                            {AML_TXQ_STOP_MU_POS, "MU"},
                            {AML_TXQ_NDEV_FLOW_CTRL, "FLW_CTRL"}))
);

DEFINE_EVENT(txq_reason_template, txq_start,
             TP_PROTO(struct aml_txq *txq, u16 reason),
             TP_ARGS(txq, reason));

DEFINE_EVENT(txq_reason_template, txq_stop,
             TP_PROTO(struct aml_txq *txq, u16 reason),
             TP_ARGS(txq, reason));

#ifdef CONFIG_AML_SOFTMAC
TRACE_EVENT(
    tx_ops,
    TP_PROTO(struct sk_buff *skb, u16 sn, u32 flags),
    TP_ARGS(skb, sn, flags),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u32, flags)
        __field(u16, sn)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->flags = flags;
        __entry->sn = sn;
                   ),
    TP_printk("%p sn=%d flags=[%s]", __entry->skb, __entry->sn,
              __print_mac_flags(__entry->flags))
);
#endif /* CONFIG_AML_SOFTMAC */

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    push_desc,
    TP_PROTO(struct sk_buff *skb, struct aml_sw_txhdr *sw_txhdr, int push_flags),

    TP_ARGS(skb, sw_txhdr, push_flags),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(unsigned int, len)
        __field(u16, tx_queue)
        __field(u8, hw_queue)
        __field(u8, push_flag)
        __field(u32, flag)
        __field(s8, txq_cred)
        __field(u8, hwq_cred)
        __field(u8, txq_length)
#ifdef CONFIG_AML_SOFTMAC
        __field(u16, sn)
#endif
        __field(u16, pkt_cnt)
        __field(u8, mu_info)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->tx_queue = sw_txhdr->txq->idx;
        __entry->push_flag = push_flags;
        __entry->hw_queue = sw_txhdr->txq->hwq->id;
        __entry->txq_cred = sw_txhdr->txq->credits;
        __entry->hwq_cred = sw_txhdr->txq->hwq->credits[AML_TXQ_POS_ID(sw_txhdr->txq)];
#ifdef CONFIG_AML_SPLIT_TX_BUF
        __entry->pkt_cnt =  sw_txhdr->desc.api.host.packet_cnt;
#else
        __entry->pkt_cnt =  1;
#endif
        __entry->txq_length = skb_queue_len(&sw_txhdr->txq->sk_list);
#ifdef CONFIG_AML_FULLMAC
        __entry->flag = sw_txhdr->desc.api.host.flags;
#ifdef CONFIG_AML_AMSDUS_TX
        if (sw_txhdr->amsdu.len)
            __entry->len = sw_txhdr->amsdu.len;
        else
#endif /* CONFIG_AML_AMSDUS_TX */
            __entry->len = sw_txhdr->desc.api.host.packet_len[0];

#else /* !CONFIG_AML_FULLMAC */
        __entry->flag = sw_txhdr->desc.api.umac.flags;
        __entry->len = sw_txhdr->frame_len;
        __entry->sn = sw_txhdr->sn;
#endif /* CONFIG_AML_FULLMAC */
#ifdef CONFIG_AML_MUMIMO_TX
        __entry->mu_info = sw_txhdr->desc.api.host.mumimo_info;
#else
        __entry->mu_info = 0;
#endif
                   ),

#ifdef CONFIG_AML_FULLMAC
    TP_printk("%s skb=%p (len=%d) hw_queue=%s txq_length=%d cred_txq=%d cred_hwq=%d %s flag=%s %s%s%s",
              __print_txq(__entry->tx_queue), __entry->skb, __entry->len,
              __print_hwq(__entry->hw_queue), __entry->txq_length,
              __entry->txq_cred, __entry->hwq_cred,
              __print_mu_info(__entry->mu_info),
              __print_flags(__entry->flag, "|",
                            {TXU_CNTRL_RETRY, "RETRY"},
                            {TXU_CNTRL_MORE_DATA, "MOREDATA"},
                            {TXU_CNTRL_MGMT, "MGMT"},
                            {TXU_CNTRL_MGMT_NO_CCK, "NO_CCK"},
                            {TXU_CNTRL_MGMT_ROBUST, "ROBUST"},
                            {TXU_CNTRL_AMSDU, "AMSDU"},
                            {TXU_CNTRL_USE_4ADDR, "4ADDR"},
                            {TXU_CNTRL_EOSP, "EOSP"},
                            {TXU_CNTRL_MESH_FWD, "MESH_FWD"},
                            {TXU_CNTRL_TDLS, "TDLS"},
                            {TXU_CNTRL_REUSE_SN, "SN"}),
              (__entry->push_flag & AML_PUSH_IMMEDIATE) ? "(IMMEDIATE)" : "",
              (!(__entry->flag & TXU_CNTRL_RETRY) &&
               (__entry->push_flag & AML_PUSH_RETRY)) ? "(SW_RETRY)" : "",
              __print_amsdu(__entry->pkt_cnt))
#else
    TP_printk("%s skb=%p (len=%d) hw_queue=%s txq_length=%d cred_txq=%d cred_hwq=%d %s flag=%x (%s) sn=%d %s",
              __print_txq(__entry->tx_queue), __entry->skb, __entry->len,
              __print_hwq(__entry->hw_queue), __entry->txq_length, __entry->txq_cred,
              __entry->hwq_cred,
              __print_mu_info(__entry->mu_info),
              __entry->flag,
              __print_flags(__entry->push_flag, "|",
                            {AML_PUSH_RETRY, "RETRY"},
                            {AML_PUSH_IMMEDIATE, "IMMEDIATE"}),
              __entry->sn, __print_amsdu(__entry->pkt_cnt))
#endif /* CONFIG_AML_FULLMAC */
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_queue_skb,
    TP_PROTO(struct sk_buff *skb, struct aml_txq *txq, bool retry),
    TP_ARGS(skb, txq, retry),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(s8, credit)
        __field(u16, q_len)
        __field(u16, q_len_retry)
        __field(bool, retry)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->credit = txq->credits;
        __entry->q_len = skb_queue_len(&txq->sk_list);
        __entry->q_len_retry = txq->nb_retry;
        __entry->retry = retry;
                   ),

    TP_printk("%s skb=%p retry=%d txq_credits=%d queue_len=%d (retry = %d)",
              __print_txq(__entry->txq_idx), __entry->skb, __entry->retry,
              __entry->credit, __entry->q_len, __entry->q_len_retry)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_drop_skb,
    TP_PROTO(struct sk_buff *skb, struct aml_txq *txq, unsigned long queued_time),
    TP_ARGS(skb, txq, queued_time),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(unsigned long, queued_time)
        __field(u16, q_len)
        __field(u16, q_len_retry)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->q_len = skb_queue_len(&txq->sk_list);
        __entry->q_len_retry = txq->nb_retry;
        __entry->queued_time = queued_time;
                   ),

    TP_printk("%s skb=%p time_queued=%dms queue_len=%d (retry = %d)",
              __print_txq(__entry->txq_idx), __entry->skb,
              jiffies_to_msecs(__entry->queued_time), __entry->q_len, __entry->q_len_retry)
);

#ifdef CONFIG_MAC80211_TXQ
/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_wake,
    TP_PROTO(struct aml_txq *txq),
    TP_ARGS(txq),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, q_len)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->q_len = txq->nb_ready_mac80211;
                   ),

    TP_printk("%s mac80211_queue_len=%d", __print_txq(__entry->txq_idx), __entry->q_len)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_drop,
    TP_PROTO(struct aml_txq *txq, unsigned long nb_drop),
    TP_ARGS(txq, nb_drop),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, nb_drop)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->nb_drop = nb_drop;
                   ),

    TP_printk("%s %u pkt have been dropped by codel in mac80211 txq",
              __print_txq(__entry->txq_idx), __entry->nb_drop)
);

#endif


/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    idx_template,
    TP_PROTO(u16 idx),
    TP_ARGS(idx),
    TP_STRUCT__entry(
        __field(u16, idx)
                     ),
    TP_fast_assign(
        __entry->idx = idx;
                   ),
    TP_printk("idx=%d", __entry->idx)
);


DEFINE_EVENT(idx_template, txq_vif_start,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

DEFINE_EVENT(idx_template, txq_vif_stop,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    process_hw_queue,
    TP_PROTO(struct aml_hwq *hwq),
    TP_ARGS(hwq),
    TP_STRUCT__entry(
        __field(u16, hwq)
        __array(u8, credits, CONFIG_USER_MAX)
                     ),
    TP_fast_assign(
        int i;
        __entry->hwq = hwq->id;
        for (i=0; i < CONFIG_USER_MAX; i ++)
            __entry->credits[i] = hwq->credits[i];
                   ),
    TP_printk("hw_queue=%s hw_credits=%s",
              __print_hwq(__entry->hwq), __print_hwq_cred(__entry->credits))
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    sta_idx_template,
    TP_PROTO(u16 idx),
    TP_ARGS(idx),
    TP_STRUCT__entry(
        __field(u16, idx)
                     ),
    TP_fast_assign(
        __entry->idx = idx;
                   ),
    TP_printk("%s", __print_sta(__entry->idx))
);

DEFINE_EVENT(sta_idx_template, txq_sta_start,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

DEFINE_EVENT(sta_idx_template, txq_sta_stop,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

#ifdef CONFIG_AML_FULLMAC

DEFINE_EVENT(sta_idx_template, ps_disable,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

#else
DEFINE_EVENT(sta_idx_template, ps_enable,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

DEFINE_EVENT(sta_idx_template, ps_disable,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

#endif  /* CONFIG_AML_FULLMAC */

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    skb_confirm,
    TP_PROTO(struct sk_buff *skb, struct aml_txq *txq, struct aml_hwq *hwq,
#ifdef CONFIG_AML_FULLMAC
             struct tx_cfm_tag *cfm
#else
             u8 cfm, u32 flags
#endif
             ),

    TP_ARGS(skb, txq, hwq, cfm
#ifdef CONFIG_AML_SOFTMAC
            , flags
#endif
            ),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(u8, hw_queue)
        __array(u8, hw_credit, CONFIG_USER_MAX)
        __field(s8, sw_credit)
        __field(s8, sw_credit_up)
#ifdef CONFIG_AML_FULLMAC
        __field(u8, ampdu_size)
#ifdef CONFIG_AML_SPLIT_TX_BUF
        __field(u16, amsdu)
#endif /* CONFIG_AML_SPLIT_TX_BUF */
        __field(u16, sn)
#else
        __field(u32, flags)
#endif /* CONFIG_AML_FULLMAC*/
                     ),

    TP_fast_assign(
        int i;
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->hw_queue = hwq->id;
        for (i = 0 ; i < CONFIG_USER_MAX ; i++)
            __entry->hw_credit[i] = hwq->credits[i];
        __entry->sw_credit = txq->credits;
#if defined CONFIG_AML_FULLMAC
        __entry->sw_credit_up = cfm->credits;
        __entry->ampdu_size = cfm->ampdu_size;
        __entry->sn = cfm->status.sn;
#ifdef CONFIG_AML_SPLIT_TX_BUF
        __entry->amsdu = cfm->amsdu_size;
#endif
#else /* CONFIG_AML_SOFTMAC */
        __entry->sw_credit_up = cfm;
        __entry->flags = flags;
#endif /* CONFIG_AML_FULLMAC */
                   ),

    TP_printk("%s skb=%p hw_queue=%s, hw_credits=%s, txq_credits=%d (+%d)"
#ifdef CONFIG_AML_FULLMAC
              " sn=%u ampdu=%d"
#ifdef CONFIG_AML_SPLIT_TX_BUF
              " amsdu=%u"
#endif
#else /* CONFIG_AML_SOFTMAC */
              " flags=%s"
#endif

              , __print_txq(__entry->txq_idx), __entry->skb,
              __print_hwq(__entry->hw_queue),
              __print_hwq_cred(__entry->hw_credit),
               __entry->sw_credit, __entry->sw_credit_up
#ifdef CONFIG_AML_FULLMAC
              , __entry->sn, __entry->ampdu_size
#ifdef CONFIG_AML_SPLIT_TX_BUF
              , __entry->amsdu
#endif
#else /* CONFIG_AML_SOFTMAC */
              , __print_mac_flags(__entry->flags)
#endif
              )
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    skb_retry,
    TP_PROTO(struct sk_buff *skb, struct aml_txq *txq, uint16_t sn),

    TP_ARGS(skb, txq, sn),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(s8, txq_credit)
        __field(u16, sn)
                     ),

    TP_fast_assign(
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->txq_credit = txq->credits;
        __entry->sn = sn;
                   ),

    TP_printk("%s skb=%p txq_credits=%d %s",
               __print_txq(__entry->txq_idx), __entry->skb,
              __entry->txq_credit, __print_sn(__entry->sn))
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    credit_update,
    TP_PROTO(struct aml_txq *txq, s8_l cred_up),

    TP_ARGS(txq, cred_up),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(s8, sw_credit)
        __field(s8, sw_credit_up)
                     ),

    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->sw_credit = txq->credits;
        __entry->sw_credit_up = cred_up;
                   ),

    TP_printk("%s txq_credits=%d (%+d)", __print_txq(__entry->txq_idx),
              __entry->sw_credit, __entry->sw_credit_up)
)

#ifdef CONFIG_AML_FULLMAC

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    ps_template,
    TP_PROTO(struct aml_sta *sta),
    TP_ARGS(sta),
    TP_STRUCT__entry(
        __field(u16, idx)
        __field(u16, ready_ps)
        __field(u16, sp_ps)
        __field(u16, ready_uapsd)
        __field(u16, sp_uapsd)
                     ),
    TP_fast_assign(
        __entry->idx  = sta->sta_idx;
        __entry->ready_ps = sta->ps.pkt_ready[LEGACY_PS_ID];
        __entry->sp_ps = sta->ps.sp_cnt[LEGACY_PS_ID];
        __entry->ready_uapsd = sta->ps.pkt_ready[UAPSD_ID];
        __entry->sp_uapsd = sta->ps.sp_cnt[UAPSD_ID];
                   ),

    TP_printk("%s [PS] ready=%d sp=%d [UAPSD] ready=%d sp=%d",
              __print_sta(__entry->idx), __entry->ready_ps, __entry->sp_ps,
              __entry->ready_uapsd, __entry->sp_uapsd)
);

DEFINE_EVENT(ps_template, ps_queue,
             TP_PROTO(struct aml_sta *sta),
             TP_ARGS(sta));

DEFINE_EVENT(ps_template, ps_drop,
             TP_PROTO(struct aml_sta *sta),
             TP_ARGS(sta));

DEFINE_EVENT(ps_template, ps_push,
             TP_PROTO(struct aml_sta *sta),
             TP_ARGS(sta));

DEFINE_EVENT(ps_template, ps_enable,
             TP_PROTO(struct aml_sta *sta),
             TP_ARGS(sta));

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    ps_traffic_update,
    TP_PROTO(u16 sta_idx, u8 traffic, bool uapsd),

    TP_ARGS(sta_idx, traffic, uapsd),

    TP_STRUCT__entry(
        __field(u16, sta_idx)
        __field(u8, traffic)
        __field(bool, uapsd)
                     ),

    TP_fast_assign(
        __entry->sta_idx = sta_idx;
        __entry->traffic = traffic;
        __entry->uapsd = uapsd;
                   ),

    TP_printk("%s %s%s traffic available ", __print_sta(__entry->sta_idx),
              __entry->traffic ? "" : "no more ",
              __entry->uapsd ? "U-APSD" : "legacy PS")
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    ps_traffic_req,
    TP_PROTO(struct aml_sta *sta, u16 pkt_req, u8 ps_id),
    TP_ARGS(sta, pkt_req, ps_id),
    TP_STRUCT__entry(
        __field(u16, idx)
        __field(u16, pkt_req)
        __field(u8, ps_id)
        __field(u16, ready)
        __field(u16, sp)
                     ),
    TP_fast_assign(
        __entry->idx  = sta->sta_idx;
        __entry->pkt_req  = pkt_req;
        __entry->ps_id  = ps_id;
        __entry->ready = sta->ps.pkt_ready[ps_id];
        __entry->sp = sta->ps.sp_cnt[ps_id];
                   ),

    TP_printk("%s %s traffic request %d pkt (ready=%d, sp=%d)",
              __print_sta(__entry->idx),
              __entry->ps_id == UAPSD_ID ? "U-APSD" : "legacy PS" ,
              __entry->pkt_req, __entry->ready, __entry->sp)
);


#ifdef CONFIG_AML_AMSDUS_TX
/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    amsdu_subframe,
    TP_PROTO(struct aml_sw_txhdr *sw_txhdr),
    TP_ARGS(sw_txhdr),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(u8, nb)
        __field(u32, len)
                     ),
    TP_fast_assign(
        __entry->skb = sw_txhdr->skb;
        __entry->nb = sw_txhdr->amsdu.nb;
        __entry->len = sw_txhdr->amsdu.len;
        __entry->txq_idx = sw_txhdr->txq->idx;
                   ),

    TP_printk("%s skb=%p %s nb_subframe=%d, len=%u",
              __print_txq(__entry->txq_idx), __entry->skb,
              (__entry->nb == 2) ? "Start new AMSDU" : "Add subframe",
              __entry->nb, __entry->len)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    amsdu_dismantle,
    TP_PROTO(struct aml_sw_txhdr *sw_txhdr),
    TP_ARGS(sw_txhdr),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(u8, nb)
        __field(u32, len)
                     ),
    TP_fast_assign(
        __entry->skb = sw_txhdr->skb;
        __entry->nb = sw_txhdr->amsdu.nb;
        __entry->len = sw_txhdr->amsdu.len;
        __entry->txq_idx = sw_txhdr->txq->idx;
                   ),

    TP_printk("%s skb=%p nb_subframe=%d, len=%u",
              __print_txq(__entry->txq_idx), __entry->skb,
              __entry->nb, __entry->len)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    amsdu_len_update,
    TP_PROTO(struct aml_sta *sta, int amsdu_len),
    TP_ARGS(sta, amsdu_len),
    TP_STRUCT__entry(
        __field(u8, sta_idx)
        __field(u16, amsdu_len)
                     ),
    TP_fast_assign(
        __entry->sta_idx = sta->sta_idx;
        __entry->amsdu_len = amsdu_len;
                   ),

    TP_printk("[Sta %d] A-MSDU len = %d", __entry->sta_idx, __entry->amsdu_len)
);
#endif

#endif /* CONFIG_AML_FULLMAC */

#ifdef CONFIG_AML_MUMIMO_TX
/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    mu_group_update,
    TP_PROTO(struct aml_mu_group *group),
    TP_ARGS(group),
    TP_STRUCT__entry(
        __field(u8, nb_user)
        __field(u8, group_id)
        __array(u8, users, CONFIG_USER_MAX)
                     ),
    TP_fast_assign(
        int i;
        __entry->nb_user = group->user_cnt;
        for (i = 0; i < CONFIG_USER_MAX ; i++) {
            if (group->users[i]) {
                __entry->users[i] = group->users[i]->sta_idx;
            } else {
                __entry->users[i] = 0xff;
            }
        }

        __entry->group_id = group->group_id;
                   ),

    TP_printk("Group-id = %d, Users = %s",
              __entry->group_id,
              __print_mu_group(__entry->nb_user, __entry->users))
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    mu_group_delete,
    TP_PROTO(int group_id),
    TP_ARGS(group_id),
    TP_STRUCT__entry(
        __field(u8, group_id)
                     ),
    TP_fast_assign(
        __entry->group_id = group_id;
                   ),

    TP_printk("Group-id = %d", __entry->group_id)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    mu_group_selection,
    TP_PROTO(struct aml_sta *sta, int group_id),
    TP_ARGS(sta, group_id),
    TP_STRUCT__entry(
        __field(u8, sta_idx)
        __field(u8, group_id)
                     ),
    TP_fast_assign(
        __entry->sta_idx = sta->sta_idx;
        __entry->group_id = group_id;
                   ),

    TP_printk("[Sta %d] Group-id = %d", __entry->sta_idx, __entry->group_id)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    txq_select_mu_group,
    TP_PROTO(struct aml_txq *txq, int group_id, int pos),

    TP_ARGS(txq, group_id, pos),

    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u8, group_id)
        __field(u8, pos)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->group_id = group_id;
        __entry->pos = pos;
                   ),

    TP_printk("%s: group=%d pos=%d", __print_txq(__entry->txq_idx),
              __entry->group_id, __entry->pos)
);

#endif /* CONFIG_AML_MUMIMO_TX */
#endif /* ! CONFIG_AML_FHOST */

/*****************************************************************************
 * TRACE functions for MESH
 ****************************************************************************/
#ifdef CONFIG_AML_FULLMAC

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    mesh_path_template,
    TP_PROTO(struct aml_mesh_path *mesh_path),
    TP_ARGS(mesh_path),
    TP_STRUCT__entry(
        __field(u8, idx)
        __field(u8, next_hop_sta)
        __array(u8, tgt_mac, ETH_ALEN)
                     ),

    TP_fast_assign(
        __entry->idx = mesh_path->path_idx;
        memcpy(__entry->tgt_mac, &mesh_path->tgt_mac_addr, ETH_ALEN);
        if (mesh_path->nhop_sta)
            __entry->next_hop_sta = mesh_path->nhop_sta->sta_idx;
        else
            __entry->next_hop_sta = 0xff;
                   ),

    TP_printk("Mpath(%d): target=%pM next_hop=STA-%d",
              __entry->idx, __entry->tgt_mac, __entry->next_hop_sta)
);

DEFINE_EVENT(mesh_path_template, mesh_create_path,
             TP_PROTO(struct aml_mesh_path *mesh_path),
             TP_ARGS(mesh_path));

DEFINE_EVENT(mesh_path_template, mesh_delete_path,
             TP_PROTO(struct aml_mesh_path *mesh_path),
             TP_ARGS(mesh_path));

DEFINE_EVENT(mesh_path_template, mesh_update_path,
             TP_PROTO(struct aml_mesh_path *mesh_path),
             TP_ARGS(mesh_path));

#endif /* CONFIG_AML_FULLMAC */

/*****************************************************************************
 * TRACE functions for RADAR
 ****************************************************************************/
#ifdef CONFIG_AML_RADAR

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    radar_pulse,
    TP_PROTO(u8 chain, struct radar_pulse *pulse),
    TP_ARGS(chain, pulse),
    TP_STRUCT__entry(
        __field(u8, chain)
        __field(s16, freq)
        __field(u16, pri)
        __field(u8, len)
        __field(u8, fom)
                     ),
    TP_fast_assign(
        __entry->freq = pulse->freq * 2;
        __entry->len = pulse->len * 2;
        __entry->fom = pulse->fom * 6;
        __entry->pri = pulse->rep;
        __entry->chain = chain;
                   ),

    TP_printk("%s: PRI=%.5d LEN=%.3d FOM=%.2d%% freq=%dMHz ",
              __print_symbolic(__entry->chain,
                               {AML_RADAR_RIU, "RIU"},
                               {AML_RADAR_FCU, "FCU"}),
              __entry->pri, __entry->len, __entry->fom, __entry->freq)
            );

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    radar_detected,
    TP_PROTO(u8 chain, u8 region, s16 freq, u8 type, u16 pri),
    TP_ARGS(chain, region, freq, type, pri),
    TP_STRUCT__entry(
        __field(u8, chain)
        __field(u8, region)
        __field(s16, freq)
        __field(u8, type)
        __field(u16, pri)
                     ),
    TP_fast_assign(
        __entry->chain = chain;
        __entry->region = region;
        __entry->freq = freq;
        __entry->type = type;
        __entry->pri = pri;
                   ),
    TP_printk("%s: region=%s type=%d freq=%dMHz (pri=%dus)",
              __print_symbolic(__entry->chain,
                               {AML_RADAR_RIU, "RIU"},
                               {AML_RADAR_FCU, "FCU"}),
              __print_symbolic(__entry->region,
                               {NL80211_DFS_UNSET, "UNSET"},
                               {NL80211_DFS_FCC, "FCC"},
                               {NL80211_DFS_ETSI, "ETSI"},
                               {NL80211_DFS_JP, "JP"}),
              __entry->type, __entry->freq, __entry->pri)
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    radar_set_region,
    TP_PROTO(u8 region),
    TP_ARGS(region),
    TP_STRUCT__entry(
        __field(u8, region)
                     ),
    TP_fast_assign(
        __entry->region = region;
                   ),
    TP_printk("region=%s",
              __print_symbolic(__entry->region,
                               {NL80211_DFS_UNSET, "UNSET"},
                               {NL80211_DFS_FCC, "FCC"},
                               {NL80211_DFS_ETSI, "ETSI"},
                               {NL80211_DFS_JP, "JP"}))
);

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
TRACE_EVENT(
    radar_enable_detection,
    TP_PROTO(u8 region, u8 enable, u8 chain),
    TP_ARGS(region, enable, chain),
    TP_STRUCT__entry(
        __field(u8, region)
        __field(u8, chain)
        __field(u8, enable)
                     ),
    TP_fast_assign(
        __entry->chain = chain;
        __entry->enable = enable;
        __entry->region = region;
                   ),
    TP_printk("%s: %s radar detection %s",
               __print_symbolic(__entry->chain,
                               {AML_RADAR_RIU, "RIU"},
                               {AML_RADAR_FCU, "FCU"}),
              __print_symbolic(__entry->enable,
                               {AML_RADAR_DETECT_DISABLE, "Disable"},
                               {AML_RADAR_DETECT_ENABLE, "Enable (no report)"},
                               {AML_RADAR_DETECT_REPORT, "Enable"}),
              __entry->enable == AML_RADAR_DETECT_DISABLE ? "" :
              __print_symbolic(__entry->region,
                               {NL80211_DFS_UNSET, "UNSET"},
                               {NL80211_DFS_FCC, "FCC"},
                               {NL80211_DFS_ETSI, "ETSI"},
                               {NL80211_DFS_JP, "JP"}))
);
#endif /* CONFIG_AML_RADAR */

/*****************************************************************************
 * TRACE functions for IPC message
 ****************************************************************************/
#include "aml_strs.h"

/* coverity[uninit_use_in_call] - __regs will be initialized in the calling function*/
DECLARE_EVENT_CLASS(
    ipc_msg_template,
    TP_PROTO(u16 id),
    TP_ARGS(id),
    TP_STRUCT__entry(
        __field(u16, id)
                     ),
    TP_fast_assign(
        __entry->id  = id;
                   ),

    TP_printk("%s (%d - %d)", AML_ID2STR(__entry->id),
              MSG_T(__entry->id), MSG_I(__entry->id))
);

DEFINE_EVENT(ipc_msg_template, msg_send,
             TP_PROTO(u16 id),
             TP_ARGS(id));

DEFINE_EVENT(ipc_msg_template, msg_recv,
             TP_PROTO(u16 id),
             TP_ARGS(id));



#endif /* !defined(_AML_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ) */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE aml_events
#include <trace/define_trace.h>
