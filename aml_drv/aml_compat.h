/**
 ******************************************************************************
 *
 * @file aml_compat.h
 *
 * Ensure driver compilation for linux 4.4 to 5.10
 *
 * To avoid too many #if LINUX_VERSION_CODE if the code, when prototype change
 * between different kernel version:
 * - For external function, define a macro whose name is the function name with
 *   _compat suffix and prototype (actually the number of parameter) of the
 *   latest version. The latest version of this macro simply call the function
 *   and for older kernel version it call the function adapting the api.
 * - For internal function (e.g. cfg80211_ops) do the same but the macro name
 *   doesn't need to have the _compat suffix when the function is not used
 *   directly by the driver
 *
 * Copyright (C) Amlogic 2020
 *
 ******************************************************************************
 */
#ifndef _AML_COMPAT_H_
#define _AML_COMPAT_H_
#include <linux/version.h>
#include <net/cfg80211.h>
#include <linux/fs.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41) && defined (CONFIG_AMLOGIC_KERNEL_VERSION))
#include <linux/upstream_version.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
#error "Minimum kernel version supported is 4.4"
#endif

/******************************************************************************
 * Generic
 *****************************************************************************/
#define UNUSED(x) ((void)x)

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#define __bf_shf(x) (__builtin_ffsll(x) - 1)
#define FIELD_PREP(_mask, _val) \
    (((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask))
#else
#include <linux/bitfield.h>
#endif // 4.9

/******************************************************************************
 * CFG80211
 *****************************************************************************/
#if defined(IEEE80211_MLD_MAX_NUM_LINKS)
  #define CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT 1
#endif

static inline void aml_cfg80211_ch_switch_notify(struct net_device *dev,
	struct cfg80211_chan_def *chandef, unsigned int link_id)
{
#if ((defined (CONFIG_AMLOGIC_KERNEL_VERSION) && defined (AML_KERNEL_VERSION)) && (\
	(CONFIG_AMLOGIC_KERNEL_VERSION == 13515 && AML_KERNEL_VERSION >= 15)\
	|| (CONFIG_AMLOGIC_KERNEL_VERSION == 14515 && AML_KERNEL_VERSION >= 12) ) )\
	|| (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0))
	return cfg80211_ch_switch_notify(dev, chandef, link_id, 0);
#elif defined (CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT)
	return cfg80211_ch_switch_notify(dev, chandef, link_id);
#else
	return cfg80211_ch_switch_notify(dev, chandef);
#endif
}

static inline void aml_cfg80211_ch_switch_started_notify(struct net_device *dev,
	struct cfg80211_chan_def *chandef, unsigned int link_id, u8 count, bool quiet)
{
#if ((defined (CONFIG_AMLOGIC_KERNEL_VERSION) && defined (AML_KERNEL_VERSION)) && (\
	(CONFIG_AMLOGIC_KERNEL_VERSION == 13515 && AML_KERNEL_VERSION >= 15)\
	|| (CONFIG_AMLOGIC_KERNEL_VERSION == 14515 && AML_KERNEL_VERSION >= 12) ) )\
	|| (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0))
	return cfg80211_ch_switch_started_notify(dev, chandef, link_id, count, quiet, 0);
#elif defined (CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT)
	return cfg80211_ch_switch_started_notify(dev, chandef, link_id, count, quiet);
#else
	return cfg80211_ch_switch_started_notify(dev, chandef, count);
#endif
}

#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
  #define LINK_STA_PARAMS(x, y) x.link_sta_params.y
  #define P_LINK_STA_PARAMS(x, y) x->link_sta_params.y
  #define LINK_PARAMS(x, y) x.links[0].y
  #define P_LINK_PARAMS(x, y) x->links[0].y
  #define UNUSED_LINK_ID(x) UNUSED(x)
#else
  #define LINK_STA_PARAMS(x, y) x.y
  #define P_LINK_STA_PARAMS(x, y) x->y
  #define LINK_PARAMS(x, y) x.y
  #define P_LINK_PARAMS(x, y) x->y
  #define UNUSED_LINK_ID(x)
#endif

#if !defined(IEEE80211_HE_MAC_CAP4_SRP_RESP) && \
		(defined  IEEE80211_HE_MAC_CAP4_PSR_RESP)
#define IEEE80211_HE_MAC_CAP4_SRP_RESP \
		IEEE80211_HE_MAC_CAP4_PSR_RESP
#endif

#if !defined(IEEE80211_HE_MAC_CAP5_SUBCHAN_SELECVITE_TRANSMISSION) && \
		(defined  IEEE80211_HE_MAC_CAP5_SUBCHAN_SELECTIVE_TRANSMISSION)
#define IEEE80211_HE_MAC_CAP5_SUBCHAN_SELECVITE_TRANSMISSION \
		IEEE80211_HE_MAC_CAP5_SUBCHAN_SELECTIVE_TRANSMISSION
#endif

#if !defined(IEEE80211_HE_PHY_CAP3_RX_HE_MU_PPDU_FROM_NON_AP_STA) && \
	(defined  IEEE80211_HE_PHY_CAP3_RX_PARTIAL_BW_SU_IN_20MHZ_MU)
#define IEEE80211_HE_PHY_CAP3_RX_HE_MU_PPDU_FROM_NON_AP_STA \
	IEEE80211_HE_PHY_CAP3_RX_PARTIAL_BW_SU_IN_20MHZ_MU
#endif

#if !defined(IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMER_FB) && \
	(defined  IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMING_FB)
#define IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMER_FB \
	IEEE80211_HE_PHY_CAP6_TRIG_SU_BEAMFORMING_FB
#endif

#if !defined(IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMER_FB) && \
	(defined  IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMING_PARTIAL_BW_FB)
#define IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMER_FB \
	IEEE80211_HE_PHY_CAP6_TRIG_MU_BEAMFORMING_PARTIAL_BW_FB
#endif

#if !defined(IEEE80211_HE_PHY_CAP7_SRP_BASED_SR) && \
	(defined  IEEE80211_HE_PHY_CAP7_PSR_BASED_SR)
#define IEEE80211_HE_PHY_CAP7_SRP_BASED_SR \
	IEEE80211_HE_PHY_CAP7_PSR_BASED_SR
#endif

#if !defined(IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_AR) && \
	(defined  IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_SUPP)
#define IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_AR \
	IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_SUPP
#endif

#if !defined(IEEE80211_MAX_AMPDU_BUF) && \
	(defined  IEEE80211_MAX_AMPDU_BUF_HE)
#define IEEE80211_MAX_AMPDU_BUF \
	IEEE80211_MAX_AMPDU_BUF_HE
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 12, 0)
#define regulatory_set_wiphy_regd_sync regulatory_set_wiphy_regd_sync_rtnl

#define wiphy_lock(w)
#define wiphy_unlock(w)
#endif // 5.12

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
#define WLAN_EXT_CAPA3_MULTI_BSSID_SUPPORT	0

#define IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_242                    0x00
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_484                    0x40
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_996                    0x80
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_2x996                  0xc0
#define IEEE80211_HE_PHY_CAP8_DCM_MAX_RU_MASK                   0xc0

#define IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_0US           0x00
#define IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_8US           0x40
#define IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_16US          0x80
#define IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_RESERVED      0xc0
#define IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_MASK          0xc0

struct element {
	u8 id;
	u8 datalen;
	u8 data[];
} __packed;

#define for_each_element(_elem, _data, _datalen)			\
	for (_elem = (const struct element *)(_data);			\
	     (const u8 *)(_data) + (_datalen) - (const u8 *)_elem >=	\
		(int)sizeof(*_elem) &&					\
	     (const u8 *)(_data) + (_datalen) - (const u8 *)_elem >=	\
		(int)sizeof(*_elem) + _elem->datalen;			\
	     _elem = (const struct element *)(_elem->data + _elem->datalen))

static inline const struct element *
cfg80211_find_elem(u8 eid, const u8 *ies, int len)
{
    const struct element *elem;
    for_each_element(elem, ies, len) {
        if (elem->id == (eid))
            return elem;
    }
    return NULL;
}

#endif // 5.1

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
#define cfg80211_notify_new_peer_candidate(dev, addr, ie, ie_len, sig_dbm, gfp) \
    cfg80211_notify_new_peer_candidate(dev, addr, ie, ie_len, gfp)

#define WLAN_EXT_CAPA10_TWT_REQUESTER_SUPPORT    BIT(5)
#define WLAN_EXT_CAPA10_TWT_RESPONDER_SUPPORT    BIT(6)

#endif // 5.0

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#define IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_MASK IEEE80211_HE_MAC_CAP3_MAX_A_AMPDU_LEN_EXP_MASK
#endif // 4.20

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#define IEEE80211_RADIOTAP_HE 23
#define IEEE80211_RADIOTAP_HE_MU 24

struct ieee80211_radiotap_he {
	__le16 data1, data2, data3, data4, data5, data6;
};

enum ieee80211_radiotap_he_bits {
	IEEE80211_RADIOTAP_HE_DATA1_FORMAT_MASK		= 3,
	IEEE80211_RADIOTAP_HE_DATA1_FORMAT_SU		= 0,
	IEEE80211_RADIOTAP_HE_DATA1_FORMAT_EXT_SU	= 1,
	IEEE80211_RADIOTAP_HE_DATA1_FORMAT_MU		= 2,
	IEEE80211_RADIOTAP_HE_DATA1_FORMAT_TRIG		= 3,

	IEEE80211_RADIOTAP_HE_DATA1_BSS_COLOR_KNOWN	= 0x0004,
	IEEE80211_RADIOTAP_HE_DATA1_BEAM_CHANGE_KNOWN	= 0x0008,
	IEEE80211_RADIOTAP_HE_DATA1_UL_DL_KNOWN		= 0x0010,
	IEEE80211_RADIOTAP_HE_DATA1_DATA_MCS_KNOWN	= 0x0020,
	IEEE80211_RADIOTAP_HE_DATA1_DATA_DCM_KNOWN	= 0x0040,
	IEEE80211_RADIOTAP_HE_DATA1_CODING_KNOWN	= 0x0080,
	IEEE80211_RADIOTAP_HE_DATA1_LDPC_XSYMSEG_KNOWN	= 0x0100,
	IEEE80211_RADIOTAP_HE_DATA1_STBC_KNOWN		= 0x0200,
	IEEE80211_RADIOTAP_HE_DATA1_SPTL_REUSE_KNOWN	= 0x0400,
	IEEE80211_RADIOTAP_HE_DATA1_SPTL_REUSE2_KNOWN	= 0x0800,
	IEEE80211_RADIOTAP_HE_DATA1_SPTL_REUSE3_KNOWN	= 0x1000,
	IEEE80211_RADIOTAP_HE_DATA1_SPTL_REUSE4_KNOWN	= 0x2000,
	IEEE80211_RADIOTAP_HE_DATA1_BW_RU_ALLOC_KNOWN	= 0x4000,
	IEEE80211_RADIOTAP_HE_DATA1_DOPPLER_KNOWN	= 0x8000,

	IEEE80211_RADIOTAP_HE_DATA2_PRISEC_80_KNOWN	= 0x0001,
	IEEE80211_RADIOTAP_HE_DATA2_GI_KNOWN		= 0x0002,
	IEEE80211_RADIOTAP_HE_DATA2_NUM_LTF_SYMS_KNOWN	= 0x0004,
	IEEE80211_RADIOTAP_HE_DATA2_PRE_FEC_PAD_KNOWN	= 0x0008,
	IEEE80211_RADIOTAP_HE_DATA2_TXBF_KNOWN		= 0x0010,
	IEEE80211_RADIOTAP_HE_DATA2_PE_DISAMBIG_KNOWN	= 0x0020,
	IEEE80211_RADIOTAP_HE_DATA2_TXOP_KNOWN		= 0x0040,
	IEEE80211_RADIOTAP_HE_DATA2_MIDAMBLE_KNOWN	= 0x0080,
	IEEE80211_RADIOTAP_HE_DATA2_RU_OFFSET		= 0x3f00,
	IEEE80211_RADIOTAP_HE_DATA2_RU_OFFSET_KNOWN	= 0x4000,
	IEEE80211_RADIOTAP_HE_DATA2_PRISEC_80_SEC	= 0x8000,

	IEEE80211_RADIOTAP_HE_DATA3_BSS_COLOR		= 0x003f,
	IEEE80211_RADIOTAP_HE_DATA3_BEAM_CHANGE		= 0x0040,
	IEEE80211_RADIOTAP_HE_DATA3_UL_DL		= 0x0080,
	IEEE80211_RADIOTAP_HE_DATA3_DATA_MCS		= 0x0f00,
	IEEE80211_RADIOTAP_HE_DATA3_DATA_DCM		= 0x1000,
	IEEE80211_RADIOTAP_HE_DATA3_CODING		= 0x2000,
	IEEE80211_RADIOTAP_HE_DATA3_LDPC_XSYMSEG	= 0x4000,
	IEEE80211_RADIOTAP_HE_DATA3_STBC		= 0x8000,

	IEEE80211_RADIOTAP_HE_DATA4_SU_MU_SPTL_REUSE	= 0x000f,
	IEEE80211_RADIOTAP_HE_DATA4_MU_STA_ID		= 0x7ff0,
	IEEE80211_RADIOTAP_HE_DATA4_TB_SPTL_REUSE1	= 0x000f,
	IEEE80211_RADIOTAP_HE_DATA4_TB_SPTL_REUSE2	= 0x00f0,
	IEEE80211_RADIOTAP_HE_DATA4_TB_SPTL_REUSE3	= 0x0f00,
	IEEE80211_RADIOTAP_HE_DATA4_TB_SPTL_REUSE4	= 0xf000,

	IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC	= 0x000f,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_20MHZ	= 0,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_40MHZ	= 1,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_80MHZ	= 2,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_160MHZ	= 3,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_26T	= 4,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_52T	= 5,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_106T	= 6,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_242T	= 7,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_484T	= 8,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_996T	= 9,
		IEEE80211_RADIOTAP_HE_DATA5_DATA_BW_RU_ALLOC_2x996T	= 10,

	IEEE80211_RADIOTAP_HE_DATA5_GI			= 0x0030,
		IEEE80211_RADIOTAP_HE_DATA5_GI_0_8			= 0,
		IEEE80211_RADIOTAP_HE_DATA5_GI_1_6			= 1,
		IEEE80211_RADIOTAP_HE_DATA5_GI_3_2			= 2,

	IEEE80211_RADIOTAP_HE_DATA5_LTF_SIZE		= 0x00c0,
		IEEE80211_RADIOTAP_HE_DATA5_LTF_SIZE_UNKNOWN		= 0,
		IEEE80211_RADIOTAP_HE_DATA5_LTF_SIZE_1X			= 1,
		IEEE80211_RADIOTAP_HE_DATA5_LTF_SIZE_2X			= 2,
		IEEE80211_RADIOTAP_HE_DATA5_LTF_SIZE_4X			= 3,
	IEEE80211_RADIOTAP_HE_DATA5_NUM_LTF_SYMS	= 0x0700,
	IEEE80211_RADIOTAP_HE_DATA5_PRE_FEC_PAD		= 0x3000,
	IEEE80211_RADIOTAP_HE_DATA5_TXBF		= 0x4000,
	IEEE80211_RADIOTAP_HE_DATA5_PE_DISAMBIG		= 0x8000,

	IEEE80211_RADIOTAP_HE_DATA6_NSTS		= 0x000f,
	IEEE80211_RADIOTAP_HE_DATA6_DOPPLER		= 0x0010,
	IEEE80211_RADIOTAP_HE_DATA6_TXOP		= 0x7f00,
	IEEE80211_RADIOTAP_HE_DATA6_MIDAMBLE_PDCTY	= 0x8000,
};

struct ieee80211_radiotap_he_mu {
	__le16 flags1, flags2;
	u8 ru_ch1[4];
	u8 ru_ch2[4];
};

enum ieee80211_radiotap_he_mu_bits {
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_MCS		= 0x000f,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_MCS_KNOWN		= 0x0010,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_DCM		= 0x0020,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_DCM_KNOWN		= 0x0040,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_CH2_CTR_26T_RU_KNOWN	= 0x0080,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_CH1_RU_KNOWN		= 0x0100,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_CH2_RU_KNOWN		= 0x0200,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_CH1_CTR_26T_RU_KNOWN	= 0x1000,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_CH1_CTR_26T_RU		= 0x2000,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_COMP_KNOWN	= 0x4000,
	IEEE80211_RADIOTAP_HE_MU_FLAGS1_SIG_B_SYMS_USERS_KNOWN	= 0x8000,

	IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW	= 0x0003,
		IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW_20MHZ	= 0x0000,
		IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW_40MHZ	= 0x0001,
		IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW_80MHZ	= 0x0002,
		IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW_160MHZ	= 0x0003,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_BW_FROM_SIG_A_BW_KNOWN	= 0x0004,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_SIG_B_COMP		= 0x0008,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_SIG_B_SYMS_USERS	= 0x00f0,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_PUNC_FROM_SIG_A_BW	= 0x0300,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_PUNC_FROM_SIG_A_BW_KNOWN= 0x0400,
	IEEE80211_RADIOTAP_HE_MU_FLAGS2_CH2_CTR_26T_RU		= 0x0800,
};
#ifndef CONFIG_KERNEL_AX_PATCH
enum {
    IEEE80211_HE_MCS_SUPPORT_0_7    = 0,
    IEEE80211_HE_MCS_SUPPORT_0_9    = 1,
    IEEE80211_HE_MCS_SUPPORT_0_11   = 2,
    IEEE80211_HE_MCS_NOT_SUPPORTED  = 3,
};
#endif
#endif // 4.19

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)
#define cfg80211_probe_status(ndev, addr, cookie, ack, ack_pwr, pwr_valid, gfp) \
    cfg80211_probe_status(ndev, addr, cookie, ack, gfp)
#endif // 4.17

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
#define aml_cfg80211_add_iface(wiphy, name, name_assign_type, type, params) \
    aml_cfg80211_add_iface(wiphy, name, name_assign_type, type, u32 *flags, params)

#define aml_cfg80211_change_iface(wiphy, dev, type, params) \
    aml_cfg80211_change_iface(wiphy, dev, type, u32 *flags, params)

#define nla_parse(tb, maxtype, head, len, policy )       \
    nla_parse(tb, maxtype, head, len, policy)

#endif
#define CCFS0(vht) vht->center_freq_seg1_idx
#define CCFS1(vht) vht->center_freq_seg2_idx


struct cfg80211_roam_info {
	struct ieee80211_channel *channel;
	struct cfg80211_bss *bss;
	const u8 *bssid;
	const u8 *req_ie;
	size_t req_ie_len;
	const u8 *resp_ie;
	size_t resp_ie_len;
};

#define cfg80211_roamed(_dev, _info, _gfp) \
    cfg80211_roamed(_dev, (_info)->channel, (_info)->bssid, (_info)->req_ie, \
                    (_info)->req_ie_len, (_info)->resp_ie, (_info)->resp_ie_len, _gfp)

#else // 4.12

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
#define CCFS0(vht) vht->center_freq_seg0_idx
#else
#define CCFS0(vht) vht->center_freq_seg1_idx
#endif
#define CCFS1(vht) vht->center_freq_seg1_idx
#endif // 4.12

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
#define cfg80211_cqm_rssi_notify(dev, event, level, gfp) \
    cfg80211_cqm_rssi_notify(dev, event, gfp)
#endif // 4.11

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#define ieee80211_amsdu_to_8023s(skb, list, addr, iftype, extra_headroom, check_da, check_sa) \
    ieee80211_amsdu_to_8023s(skb, list, addr, iftype, extra_headroom, false)
#endif // 4.9

#if LINUX_VERSION_CODE  < KERNEL_VERSION(4, 7, 0)
#define NUM_NL80211_BANDS IEEE80211_NUM_BANDS
#endif // 4.7

#define SURVEY_TIME(s) s->time
#define SURVEY_TIME_BUSY(s) s->time_busy
#define STA_TDLS_INITIATOR(sta) sta->tdls_initiator

/******************************************************************************
 * MAC80211
 *****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#define IEEE80211_MAX_CNTDWN_COUNTERS_NUM IEEE80211_MAX_CSA_COUNTERS_NUM
#define ieee80211_beacon_update_cntdwn ieee80211_csa_update_counter
#define ieee80211_beacon_cntdwn_is_complete ieee80211_csa_is_complete
#define cntdwn_counter_offs csa_counter_offs
#endif // 5.10

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
#define aml_ops_cancel_remain_on_channel(hw, vif) \
    aml_ops_cancel_remain_on_channel(hw)
#endif // 5.3

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
#define aml_ops_mgd_prepare_tx(hw, vif, duration) \
    aml_ops_mgd_prepare_tx(hw, vif)
#endif // 4.18

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)

#define RX_ENC_HT(s) s->flag |= RX_FLAG_HT
#define RX_ENC_HT_GF(s) s->flag |= (RX_FLAG_HT | RX_FLAG_HT_GF)
#define RX_ENC_VHT(s) s->flag |= RX_FLAG_HT
#define RX_ENC_HE(s) s->flag |= RX_FLAG_HT
#define RX_ENC_FLAG_SHORT_GI(s) s->flag |= RX_FLAG_SHORT_GI
#define RX_ENC_FLAG_SHORT_PRE(s) s->flag |= RX_FLAG_SHORTPRE
#define RX_ENC_FLAG_LDPC(s) s->flag |= RX_FLAG_LDPC
#define RX_BW_40MHZ(s) s->flag |= RX_FLAG_40MHZ
#define RX_BW_80MHZ(s) s->vht_flag |= RX_VHT_FLAG_80MHZ
#define RX_BW_160MHZ(s) s->vht_flag |= RX_VHT_FLAG_160MHZ
#define RX_NSS(s) s->vht_nss

#else
#define RX_ENC_HT(s) s->encoding = RX_ENC_HT
#define RX_ENC_HT_GF(s) { s->encoding = RX_ENC_HT;      \
        s->enc_flags |= RX_ENC_FLAG_HT_GF; }
#define RX_ENC_VHT(s) s->encoding = RX_ENC_VHT
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 19, 0)
#define RX_ENC_HE(s) s->encoding = RX_ENC_HE
#else
#define RX_ENC_HE(s) s->encoding = RX_ENC_VHT
#endif
#define RX_ENC_FLAG_SHORT_GI(s) s->enc_flags |= RX_ENC_FLAG_SHORT_GI
#define RX_ENC_FLAG_SHORT_PRE(s) s->enc_flags |= RX_ENC_FLAG_SHORTPRE
#define RX_ENC_FLAG_LDPC(s) s->enc_flags |= RX_ENC_FLAG_LDPC
#define RX_BW_40MHZ(s) s->bw = RATE_INFO_BW_40
#define RX_BW_80MHZ(s) s->bw = RATE_INFO_BW_80
#define RX_BW_160MHZ(s) s->bw = RATE_INFO_BW_160
#define RX_NSS(s) s->nss

#endif // 4.12

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
#define ieee80211_cqm_rssi_notify(vif, event, level, gfp) \
    ieee80211_cqm_rssi_notify(vif, event, gfp)
#endif // 4.11

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
#define RX_FLAG_MIC_STRIPPED 0
#endif // 4.7

#ifndef CONFIG_VENDOR_AML_AMSDUS_TX
#if  (LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0))
#define aml_ops_ampdu_action(hw, vif, params) \
    aml_ops_ampdu_action(hw, vif, enum ieee80211_ampdu_mlme_action action, \
                          struct ieee80211_sta *sta, u16 tid, u16 *ssn, u8 buf_size, \
                          bool amsdu)
#endif // 4.6
#endif /* CONFIG_VENDOR_AML_AMSDUS_TX */

/******************************************************************************
 * NET
 *****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#define aml_select_queue(dev, skb, sb_dev) \
    aml_select_queue(dev, skb, void *accel_priv, select_queue_fallback_t fallback)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
#define aml_select_queue(dev, skb, sb_dev) \
    aml_select_queue(dev, skb, sb_dev, select_queue_fallback_t fallback)
#endif //4.19

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)) && !(defined CONFIG_VENDOR_AML)
#define sk_pacing_shift_update(sk, shift)
#endif // 4.16

/******************************************************************************
 * TRACE
 *****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
#define trace_print_symbols_seq ftrace_print_symbols_seq
#endif // 4.2

/******************************************************************************
 * TIME
 *****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
#define time64_to_tm(t, o, tm) time_to_tm((time_t)t, o, tm)
#endif // 4.8

/******************************************************************************
 * timer
 *****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

#define timer_setup(timer, callback, flags) \
    __setup_timer(timer, (void (*)(unsigned long))callback, (unsigned long)timer, flags)
#endif // 4.14

/******************************************************************************
 * PCIe
 *****************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
  #define D3HOT_DELAY d3hot_delay
#else
  #define D3HOT_DELAY d3_delay
#endif

/******************************************************************************
 * Android GKI
 *****************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static inline int CALL_USERMODEHELPER(const char *path, char **argv, char **envp, int wait)
#else
static inline int CALL_USERMODEHELPER(char *path, char **argv, char **envp, int wait)
#endif
{
#ifdef CONFIG_ANDROID_GKI
	UNUSED(path);
	UNUSED(argv);
	UNUSED(envp);
	UNUSED(wait);
	return -ENOTSUPP;
#else
	return call_usermodehelper(path, argv, envp, wait);
#endif
}

static inline void SKB_APPEND(struct sk_buff *old, struct sk_buff *newsk, struct sk_buff_head *list)
{
#ifdef CONFIG_ANDROID_GKI
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);
	__skb_queue_after(list, old, newsk);
	spin_unlock_irqrestore(&list->lock, flags);
#else
	skb_append(old, newsk, list);
#endif
}

/******************************************************************************
 * File
 *****************************************************************************/
#if defined(__ANDROID_COMMON_KERNEL__) || defined(CONFIG_AML_ANDROID) || defined(CONFIG_BUILDROOT)
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    #define DISABLE_FILE_OPS
  #endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
  #if !defined(CONFIG_SET_FS)
    #define GET_FS(fs)
	#define SET_FS(fs)
  #else
    #define GET_FS(fs) mm_segment_t fs = get_fs()
	#define SET_FS(fs) set_fs(fs)
  #endif
#else
  #define GET_FS(fs) mm_segment_t fs = get_fs()
  #define SET_FS(fs) set_fs(fs)
#endif

static inline struct file *FILE_OPEN(const char *filename, int flags, umode_t mode)
{
#ifdef DISABLE_FILE_OPS
	UNUSED(filename);
	UNUSED(flags);
	UNUSED(mode);
	return NULL;
#else
    return filp_open(filename, flags, mode);
#endif
}

static inline int FILE_CLOSE(struct file *filp, fl_owner_t id)
{
#ifdef DISABLE_FILE_OPS
	UNUSED(filp);
	UNUSED(id);
	return -ENOTSUPP;
#else
    return filp_close(filp, id);
#endif
}

static inline int FILE_STAT(const char __user *filename, struct kstat *stat)
{
#if defined (CONFIG_ANDROID_GKI) || defined (CONFIG_BUILDROOT)
	UNUSED(filename);
	UNUSED(stat);
	return -ENOTSUPP;
#else
    int ret;
    GET_FS(oldfs);
    SET_FS(KERNEL_DS);
    ret=vfs_stat(filename, stat);
    SET_FS(oldfs);
    return ret;
#endif
}

static inline ssize_t FILE_READ(struct file *file, void *buf, size_t count, loff_t *pos)
{
#ifdef DISABLE_FILE_OPS
	UNUSED(file);
	UNUSED(buf);
	UNUSED(count);
	UNUSED(pos);
	return -ENOTSUPP;
#else
	ssize_t ret;
	GET_FS(oldfs);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	if (!(file->f_mode & FMODE_CAN_READ)) {
#else
	if (!file->f_op || !file->f_op->read) {
#endif
		return -EPERM;
	}

	SET_FS(KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret = kernel_read(file, buf, count, pos);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	ret = __vfs_read(file, buf, count, pos);
#else
	ret = fp->f_op->read(file, buf, count, pos);
#endif
	SET_FS(oldfs);

	return ret;
#endif
}

static inline ssize_t FILE_WRITE(struct file *file, const void *buf, size_t count, loff_t *pos)
{
#ifdef DISABLE_FILE_OPS
	UNUSED(file);
	UNUSED(buf);
	UNUSED(count);
	UNUSED(pos);
	return -ENOTSUPP;
#else
	ssize_t ret;
	GET_FS(oldfs);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	if (!(file->f_mode & FMODE_CAN_WRITE)) {
#else
	if (!file->f_op || !file->f_op->write) {
#endif
		return -EPERM;
	}

	SET_FS(KERNEL_DS);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret = kernel_write(file, buf, count, pos);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	ret = __vfs_write(file, buf, count, pos);
#else
	ret = fp->f_op->write(file, buf, count, pos);
#endif
	SET_FS(oldfs);

	return ret;
#endif
}

#endif /* _AML_COMPAT_H_ */
