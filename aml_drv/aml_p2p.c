/**
 ******************************************************************************
 *
 * @file aml_p2p.c
 *
 * @brief
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#define AML_MODULE          P2P
#include <linux/tcp.h>
#include <linux/ip.h>
#include "aml_msg_tx.h"
#include "aml_mod_params.h"
#include "reg_access.h"
#include "aml_compat.h"
#include "aml_p2p.h"
#include "aml_tx.h"

const char *p2p_pub_action_trace_name(int type)
{
   /*Table 61—P2P public action frame type*/
   static const char *p2p_pub_action_trace[] = {
       "P2P NEG REQ",
       "P2P NEG RSP",
       "P2P NEG CFM",
       "P2P INV REQ",
       "P2P INV RSP",
       "P2P DEV DISCOVERY REQ",
       "P2P DEV DISCOVERY RSP",
       "P2P PROVISION REQ",
       "P2P PROVISION RSP",
       "P2P PUBLIC ACT REV"
   };

   return type < ARRAY_SIZE(p2p_pub_action_trace) ? p2p_pub_action_trace[type] : "p2p_pub NULL";
}



const char *p2p_action_trace_name(int type)
{
   /*Table 75—P2P action frame type*/
   static const char *p2p_action_trace[] = {
       "P2P NOA",
       "P2P PRESENCE REQ",
       "P2P PRESENCE RSP",
       "P2P GO DISCOVERY REQ",
       "P2P ACT REV"
   };

   return type < ARRAY_SIZE(p2p_action_trace) ? p2p_action_trace[type] : "p2p_action NULL";
}

u32 aml_get_p2p_ie_offset(const u8 *buf, u32 frame_len, u8 element_offset)
{
    u32 offset = element_offset;
    u8 id;
    u8 len;

    while (offset < frame_len) {
        id = buf[offset];
        len = buf[offset + 1];

        if ((id == P2P_ATTR_VENDOR_SPECIFIC) &&
                (buf[offset + 2] == 0x50) && (buf[offset + 3] == 0x6f) && (buf[offset + 4] == 0x9a) && (buf[offset + 5] == 0x09)) {
            return offset;
        }

        offset += len + 2;
    }

    return 0;
}

u32 aml_get_wfd_ie_offset(const u8 *buf, u32 frame_len, u8 element_offset)
{
    u32 offset = element_offset;
    u8 id;
    u8 len;

    while (offset < frame_len) {
        id = buf[offset];
        len = buf[offset + 1];

        if ((id == P2P_ATTR_VENDOR_SPECIFIC) &&
                (buf[offset + 2] == 0x50) && (buf[offset + 3] == 0x6f) && (buf[offset + 4] == 0x9a) && (buf[offset + 5] == 0x0a)) {
            return offset;
        }

        offset += len + 2;
    }

    return 0;
}

static u16 aml_scc_p2p_rewrite_chan_list(u8 *buf, u32 offset, u8 target_chan_no, enum nl80211_band target_band)
{
#define MAX_CHAN_LIST_BUF_LEN 200
#define OPERATION_CLASS_HRD_LEN 2 // oper_class(1), len(1)
#define MIN_OPERATION_CLASS_LEN (OPERATION_CLASS_HRD_LEN + 1) // oper_class(1), len(1), chan_no(min 1)
#define MIN_CHAN_LIST_IE_LEN 6 //id(1) + len(2) + country(3)
#define ATTRIBUTE_HDR_LEN 3
    u32 idx = P2P_ATT_COUNTRY_STR_LEN + P2P_ATT_BODY_OFT;
    u32 i = 0;
    u16 chan_list_ie_len;
    u32 oper_class_len;
    u8 oper_class;
    u8 chan_list_buf[200] = {0,};
    u32 chan_list_idx = 0;
    chan_list_ie_len = buf[offset + 1] | (buf[offset + 2] << 8);

    if (chan_list_ie_len < MIN_CHAN_LIST_IE_LEN || chan_list_ie_len > MAX_CHAN_LIST_BUF_LEN) {
        return 0;
    }

    //AML_INFO("[P2P SCC] target_chan_no:%d", target_chan_no);
    while (idx < chan_list_ie_len) {
        enum nl80211_band band_parse = NL80211_BAND_2GHZ;
        oper_class = buf[offset + idx];
        oper_class_len = buf[offset + idx + 1];

        if (idx + OPERATION_CLASS_HRD_LEN + oper_class_len > chan_list_ie_len + ATTRIBUTE_HDR_LEN) {
            return 0;
        }

        if (target_band == NL80211_BAND_5GHZ) {
            if (AML_SCC_GET_P2P_PEER_5G_SUPPORT()) {
                /* coverity[tainted_data]*/
                for (i = 0; i < oper_class_len; i++) {
                    u8 chan_no_check = buf[offset + idx + OPERATION_CLASS_HRD_LEN + i];

                    if (chan_no_check == target_chan_no) {
                        if (chan_list_idx + MIN_OPERATION_CLASS_LEN > MAX_CHAN_LIST_BUF_LEN) {
                            // overflow
                            return 0;
                        }

                        chan_list_buf[chan_list_idx++] = oper_class;
                        chan_list_buf[chan_list_idx++] = 1;
                        chan_list_buf[chan_list_idx++] = target_chan_no;
                        //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                        break;
                    }
                }
            }
            else {
                bool is_2g = false;

                if (ieee80211_operating_class_to_band(oper_class, &band_parse)) {
                    is_2g = (band_parse == NL80211_BAND_2GHZ);
                }

                if (is_2g) {
                    if (chan_list_idx + oper_class_len + OPERATION_CLASS_HRD_LEN > MAX_CHAN_LIST_BUF_LEN) {
                        //overflow
                        return 0;
                    }

                    /* coverity[tainted_data]*/
                    memcpy(&chan_list_buf[chan_list_idx], &buf[offset + idx], oper_class_len + OPERATION_CLASS_HRD_LEN);
                    chan_list_idx += oper_class_len + OPERATION_CLASS_HRD_LEN;
                }
                else {
                    /* coverity[tainted_data]*/
                    for (i = 0; i < oper_class_len; i++) {
                        u8 chan_no_check = buf[offset + idx + OPERATION_CLASS_HRD_LEN + i];

                        if (chan_no_check == target_chan_no) {
                            if (chan_list_idx + MIN_OPERATION_CLASS_LEN > MAX_CHAN_LIST_BUF_LEN) {
                                //overflow
                                return 0;
                            }

                            chan_list_buf[chan_list_idx++] = oper_class;
                            chan_list_buf[chan_list_idx++] = 1; //one channel
                            chan_list_buf[chan_list_idx++] = target_chan_no;
                            //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                            break;
                        }
                    }
                }
            }
        }
        else {
            /* coverity[tainted_data]*/
            for (i = 0; i < oper_class_len; i++) {
                u8 chan_no_check = buf[offset + idx + OPERATION_CLASS_HRD_LEN + i];

                if (chan_no_check == target_chan_no) {
                    if (chan_list_idx + MIN_OPERATION_CLASS_LEN > MAX_CHAN_LIST_BUF_LEN) {
                        //overflow
                        return 0;
                    }

                    chan_list_buf[chan_list_idx++] = oper_class;
                    chan_list_buf[chan_list_idx++] = 1; //one channel
                    chan_list_buf[chan_list_idx++] = target_chan_no;
                    //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                    break;
                }
            }
        }

        idx += oper_class_len + OPERATION_CLASS_HRD_LEN;
    }

    /*coverity[tainted_data]*/
    memcpy(&buf[offset + P2P_ATT_COUNTRY_STR_LEN + P2P_ATT_BODY_OFT], chan_list_buf, chan_list_idx);
    return chan_list_idx;
}

void aml_change_p2p_chanlist(struct aml_vif *vif, u8 *buf, u32 frame_len, u32 *frame_len_offset, struct cfg80211_chan_def chan_def)
{
    u32 offset = aml_get_p2p_ie_offset(buf, frame_len, MAC_SHORT_MAC_HDR_LEN + P2P_ACTION_HDR_LEN);

    //idx pointer to wifi-direct ie
    if (offset != 0) {
        u8 *p2p_ie_len_p;
        u8 id;
        u16 ie_len;
        u16 chan_list_len_after;
        u16 chan_list_len_before;
        u16 len_diff;
        u8 target_chan_no = aml_ieee80211_freq_to_chan(chan_def.chan->center_freq, chan_def.chan->band);
        bool is_found = false;
        p2p_ie_len_p = &buf[offset + 1];
        offset += P2P_ELEMENT_HDR_LEN;

        while (offset < frame_len) {
            id = buf[offset];
            ie_len = (buf[offset + 2] << 8) | (buf[offset + 1]);

            if (id == P2P_ATTR_CHANNEL_LIST) {
                is_found = true;
                break;
            }

            offset += ie_len + P2P_ATT_BODY_OFT;
        }

        if (is_found == false) {
            return;
        }

        //now offset pointer to channel list ie
        /* coverity[tainted_data]*/
        chan_list_len_after = aml_scc_p2p_rewrite_chan_list(buf, offset, target_chan_no, chan_def.chan->band);

        if (chan_list_len_after == 0) {
            //no chan found,return
            return;
        }

        chan_list_len_after += P2P_ATT_COUNTRY_STR_LEN;
        chan_list_len_before = buf[offset + 1] | (buf[offset + 2] << 8);
        len_diff = chan_list_len_before - chan_list_len_after;
        //change change list ie len
        buf[offset + 1] = chan_list_len_after & 0xff;
        buf[offset + 2] = chan_list_len_after >> 8;
        *frame_len_offset = len_diff;
        //change p2p ie len
        *p2p_ie_len_p = *p2p_ie_len_p - len_diff;
        //copy rest buffer to front
        /* coverity[tainted_data]*/
        memmove(&buf[offset + P2P_ATT_BODY_OFT + chan_list_len_after], &buf[offset + ie_len + P2P_ATT_BODY_OFT], frame_len - offset - ie_len - P2P_ATT_BODY_OFT);
    }
}

/*check if need replace operating chan class,return true if need replace*/
static bool aml_scc_compare_oper_class(u8 org, u8 new)
{
    if (org == new) {
        return false;
    }

    if ((new >= 81 && new <= 83) && (org >= 81 && org <= 83)) {
        return false;
    }

    if ((new >= 115 && new <= 117) && (org >= 115 && org <= 117)) {
        return false;
    }

    if ((new >= 118 && new <= 120) && (org >= 118 && org <= 120)) {
        return false;
    }

    if ((new >= 121 && new <= 123) && (org >= 121 && org <= 123)) {
        return false;
    }

    if ((new >= 124 && new <= 127) && (org >= 124 && org <= 127)) {
        return false;
    }

    return true;
}

void aml_change_p2p_operchan(struct aml_vif *vif, u8 *buf, u32 frame_len, struct cfg80211_chan_def chan_def)
{
    u8 *p_ie_len;
    u32 offset;
    u8 id;
    u16 len;
    u8 chan_no = aml_ieee80211_freq_to_chan(chan_def.chan->center_freq, chan_def.chan->band);
    offset = aml_get_p2p_ie_offset(buf, frame_len, MAC_SHORT_MAC_HDR_LEN + P2P_ACTION_HDR_LEN);

    //idx pointer to wifi-direct ie
    if (offset != 0) {
        u8 oper_class_org;
        enum nl80211_band band_org = NL80211_BAND_2GHZ;
        bool is_found = false;
        p_ie_len = &buf[offset + 1];
        offset += 6;

        while (offset < frame_len) {
            id = buf[offset];
            len = (buf[offset + 2] << 8) | (buf[offset + 1]);

            if (id == P2P_ATTR_OPERATING_CHANNEL) {
                is_found = true;
                break;
            }

            offset += len + 3;
        }

        if (is_found == false) {
            return;
        }

        //now offset pointer to oper channel ie
        oper_class_org = buf[offset + 6];

        if (ieee80211_operating_class_to_band(oper_class_org, &band_org)) {
            u8 oper_class_new = 0;

            if (!ieee80211_chandef_to_operating_class(&chan_def, &oper_class_new)) {
                AML_INFO("[P2P SCC] operating class not support");
                return;
            }

            if ((band_org == chan_def.chan->band) || (chan_def.chan->band == NL80211_BAND_2GHZ)) {
                bool replace = aml_scc_compare_oper_class(buf[offset + 6], oper_class_new);
                AML_INFO("[P2P SCC] operating chan  %d ->  %d,oper_class:[%d %d]",  buf[offset + 7], chan_no, buf[offset + 6], oper_class_new);

                if (replace) {
                    //AML_INFO("[P2P SCC] operating class  %d ->  %d",  buf[offset + 6], oper_class_new);
                    buf[offset + 6] = oper_class_new;
                }

                buf[offset + 7] = chan_no;
            }
            else {
                AML_INFO("[P2P SCC] do not change operating chan, class:%d no:%d", buf[offset + 6], buf[offset + 7]);
            }
        }
        else {
            AML_INFO("[P2P SCC] get oper class error, oper_class:%d", oper_class_org);
        }
    }
}

void aml_change_p2p_intent(struct aml_vif *vif, u8 *buf, u32 frame_len, u32 *frame_len_offset)
{
    u8 *p_ie_len;
    u32 offset;
    u8 id;
    u16 len;
    bool tie_breaker;
    offset = aml_get_p2p_ie_offset(buf, frame_len, MAC_SHORT_MAC_HDR_LEN + P2P_ACTION_HDR_LEN);

    //idx pointer to wifi-direct ie
    if (offset != 0) {
        p_ie_len = &buf[offset + 1];
        offset += 6;

        while (offset < frame_len) {
            id = buf[offset];
            len = (buf[offset + 2] << 8) | (buf[offset + 1]);

            if (id == P2P_ATTR_GROUP_OWNER_INTENT) {
                tie_breaker = buf[offset + 3] & 0x1;
                buf[offset + 3] = (GO_INTENT_H << 1) | tie_breaker;
                break;
            }

            offset += len + 3;
        }
    }
}

void aml_rx_parse_p2p_chan_list(u8 *buf, u32 frame_len)
{
    u32 offset = aml_get_p2p_ie_offset(buf, frame_len, MAC_SHORT_MAC_HDR_LEN + P2P_ACTION_HDR_LEN);

    //idx pointer to wifi-direct ie
    if (offset != 0) {
        u8 id;
        u16 ie_len;
        u16 chan_list_ie_len;
        u8 oper_class_len;
        u8 oper_class;
        u8 i;
        u8 chan_parse;
        u32 idx = P2P_ATT_COUNTRY_STR_LEN + P2P_ATT_BODY_OFT;
        bool is_found = false;
        offset += P2P_ELEMENT_HDR_LEN;

        while (offset < frame_len) {
            id = buf[offset];
            ie_len = (buf[offset + 2] << 8) | (buf[offset + 1]);

            if (id == P2P_ATTR_CHANNEL_LIST) {
                is_found = true;
                break;
            }

            offset += ie_len + P2P_ATT_BODY_OFT;
        }

        if (is_found == false) {
            return;
        }

        AML_SCC_SET_P2P_PEER_5G_SUPPORT(false);
        chan_list_ie_len = buf[offset + 1] | (buf[offset + 2] << 8);

        if ((chan_list_ie_len + offset) >= frame_len) {
            return;
        }

        /* coverity[tainted_data]*/
        while (idx < chan_list_ie_len) {
            oper_class = buf[offset + idx];
            oper_class_len = buf[offset + idx + 1];

            /* coverity[tainted_data]*/
            for (i = 1; i <= oper_class_len; i++) {
                chan_parse = buf[offset + idx + 1 + i];

                if (chan_parse >= 36) {
                    AML_SCC_SET_P2P_PEER_5G_SUPPORT(true);
                    AML_INFO("[P2P SCC] rx action, chan no parse:%d ,set 5g support \n", chan_parse);
                    return;
                }
            }

            idx += oper_class_len + 2;
        }
    }
}

bool aml_filter_rtsp_frame(struct aml_vif *vif, u32 len, const u8 *data, AML_SP_STATUS_E sp_status)
{
#define MAX_RTSP_LEN 300
#define CHECK_LEN 40 //first 40 bytes check rtsp string RTSP/1.0
#define TRACE_INFO_LEN 30
    u32 i = 0;
    u32 ip_hdr_len;
    u32 tcp_hdr_len;
    struct aml_hw *aml_hw = vif->aml_hw;
    struct ethhdr *ethhdr = (struct ethhdr *)data;
    struct iphdr *iphdr = NULL;
    struct tcphdr *tcphdr = NULL;
    u8 *payload = NULL;
    s32 payload_len = 0;
    const char *rtsp_str = "RTSP/1.0";

    if ((sp_status != SP_STATUS_TX_START)
            || (vif->vif_index != AML_P2P_VIF_IDX)
            || (len > MAX_RTSP_LEN)
            || !aml_hw->wfd_present) {
        return false;
    }

    iphdr = (struct iphdr *)(ethhdr + 1);

    if ((iphdr->version != 4) || (iphdr->protocol != IPPROTO_TCP)) {
        return 0;
    }

    ip_hdr_len = iphdr->ihl * 4;
    tcphdr = (struct tcphdr *)((u8 *)(iphdr) + ip_hdr_len);
    tcp_hdr_len = tcphdr->doff * 4;
    payload_len = (s32)(ntohs(iphdr->tot_len) - ip_hdr_len - tcp_hdr_len);

    if (payload_len < (s32)(strlen(rtsp_str))) {
        return false;
    }

    payload = (u8 *)(tcphdr) + tcp_hdr_len;

    for (i = 0; i <= MIN(payload_len - strlen(rtsp_str), CHECK_LEN); i++) {
        if (strncmp(&payload[i], rtsp_str, strlen(rtsp_str)) == 0) {
            u8 str[TRACE_INFO_LEN];
            u32 len = MIN(payload_len, TRACE_INFO_LEN);
            strncpy(str, payload, len - 1);
            str[len - 1] = '\0';
            AML_INFO("[SP FRAME RTSP], ip_id:%d, src_ip:%pI4, dst_ip:%pI4, info:%s",
                     ntohs(iphdr->id), &iphdr->saddr, &iphdr->daddr, str);
            return true;
        }
    }

    return false;
}

