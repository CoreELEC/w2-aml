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

#include "aml_msg_tx.h"
#include "aml_mod_params.h"
#include "reg_access.h"
#include "aml_compat.h"
#include "aml_p2p.h"
#include "aml_tx.h"

/*Table 61—P2P public action frame type*/
char p2p_pub_action_trace[][30] = {
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

/*Table 75—P2P action frame type*/
char p2p_action_trace[][30] = {
    "P2P NOA",
    "P2P PRECENCE REQ",
    "P2P PRECENCE RSP",
    "P2P GO DISCOVERY REQ",
    "P2P ACT REV"
};

u32 aml_get_p2p_ie_offset(const u8 *buf,u32 frame_len)
{
    u32 offset = MAC_SHORT_MAC_HDR_LEN + P2P_ACTION_HDR_LEN;
    u8 id;
    u8 len;

    while (offset < frame_len) {
        id = buf[offset];
        len = buf[offset + 1];
        if ((id == P2P_ATTR_VENDOR_SPECIFIC) &&
            (buf[offset + 2] == 0x50) && (buf[offset + 3] == 0x6f) && (buf[offset + 4] == 0x9a)) {
            return offset;
        }
        offset += len + 2;
    }
    return 0;
}

u16 aml_scc_p2p_rewrite_chan_list(u8* buf, u32 offset, u8 target_chan_no, enum nl80211_band target_band)
{
    u32 idx = P2P_ATT_COUNTRY_STR_LEN + P2P_ATT_BODY_OFT;
    u32 i = 0;
    u16 chan_list_ie_len;
    u8 oper_class_len;
    u8 oper_class;
    u8 chan_list_buf[200] = {0,};
    u8 chan_list_idx = 0;

    chan_list_ie_len = buf[offset + 1] | (buf[offset + 2] << 8);
    //AML_INFO("[P2P SCC] target_chan_no:%d", target_chan_no);
    while (idx < chan_list_ie_len) {
        enum nl80211_band band_parse;

        oper_class = buf[offset + idx];
        oper_class_len = buf[offset + idx + 1];
        if (target_band == NL80211_BAND_5GHZ) {
            if (AML_SCC_GET_P2P_PEER_5G_SUPPORT()) {
                for (i = 0; i < oper_class_len; i++) {
                    u8 chan_no_check = buf[offset + idx + 2 + i];
                    if (chan_no_check == target_chan_no) {
                        chan_list_buf[chan_list_idx++] = oper_class;
                        chan_list_buf[chan_list_idx++] = 1;
                        chan_list_buf[chan_list_idx++] = target_chan_no;
                        //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                        break;
                    }
                }
            }
            else {
                if (ieee80211_operating_class_to_band(oper_class, &band_parse) && (band_parse == NL80211_BAND_2GHZ)) {
                    memcpy(&chan_list_buf[chan_list_idx], &buf[offset + idx], oper_class_len + 2);
                    chan_list_idx += buf[offset + idx + 1] + 2;
                }
                else {
                    for (i = 0; i < oper_class_len; i++) {
                        u8 chan_no_check = buf[offset + idx + 2 + i];
                        if (chan_no_check == target_chan_no) {
                            chan_list_buf[chan_list_idx++] = oper_class;
                            chan_list_buf[chan_list_idx++] = 1;
                            chan_list_buf[chan_list_idx++] = target_chan_no;
                            //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                            break;
                        }
                    }
               }
            }
        }
        else {
            for (i = 0; i < oper_class_len; i++) {
                u8 chan_no_check = buf[offset + idx + 2 + i];
                if (chan_no_check == target_chan_no) {
                    chan_list_buf[chan_list_idx++] = oper_class;
                    chan_list_buf[chan_list_idx++] = 1;
                    chan_list_buf[chan_list_idx++] = target_chan_no;
                    //AML_INFO("[P2P SCC] chan match, chan_no:%d class:%d", chan_no_check, oper_class);
                    break;
                }
            }
        }

        idx += oper_class_len + 2;
    }
    memcpy(&buf[offset + P2P_ATT_COUNTRY_STR_LEN + P2P_ATT_BODY_OFT],chan_list_buf,chan_list_idx);
    return chan_list_idx;
}

void aml_change_p2p_chanlist(struct aml_vif *vif, u8 *buf, u32 frame_len, u32* frame_len_offset, struct cfg80211_chan_def chan_def)
{
    u32 offset = aml_get_p2p_ie_offset(buf,frame_len);
    //idx pointer to wifi-direct ie
    if (offset != 0) {
        u8* p2p_ie_len_p;
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

        if (is_found == false)
            return;
        //now offset pointer to channel list ie
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
        buf[offset + 2] = chan_list_len_after >> 0xff;
        *frame_len_offset = len_diff;
        //change p2p ie len
        *p2p_ie_len_p = *p2p_ie_len_p - len_diff;
        //copy rest buffer to front
        memcpy(&buf[offset + P2P_ATT_BODY_OFT + chan_list_len_after], &buf[offset + ie_len + P2P_ATT_BODY_OFT], frame_len - offset - ie_len - P2P_ATT_BODY_OFT);
    }
}

/*check if need replace operating chan class,return true if need replace*/
bool aml_scc_compare_oper_class(u8 org, u8 new)
{
    if (org == new)
        return false;
    if ((new >= 81 && new <= 83) && (org >= 81 && org <= 83))
        return false;
    if ((new >= 115 && new <= 117) && (org >= 115 && org <= 117))
        return false;
    if ((new >= 118 && new <= 120) && (org >= 118 && org <= 120))
        return false;
    if ((new >= 121 && new <= 123) && (org >= 121 && org <= 123))
        return false;
    if ((new >= 124 && new <= 127) && (org >= 124 && org <= 127))
        return false;
    return true;
}

void aml_change_p2p_operchan(struct aml_vif *vif, u8 *buf, u32 frame_len, struct cfg80211_chan_def chan_def)
{
    u8* p_ie_len;
    u32 offset;
    u8 id;
    u16 len;
    u8 chan_no = aml_ieee80211_freq_to_chan(chan_def.chan->center_freq, chan_def.chan->band);

    offset = aml_get_p2p_ie_offset(buf,frame_len);
    //idx pointer to wifi-direct ie
    if (offset != 0) {
        u8 oper_class_org;
        enum nl80211_band band_org;
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

        if (is_found == false)
            return;

        //now offset pointer to oper channel ie
        oper_class_org = buf[offset + 6];
        if (ieee80211_operating_class_to_band(oper_class_org, &band_org)) {
            u8 oper_class_new = aml_get_operation_class(chan_def);
            if ((band_org == chan_def.chan->band) || (chan_def.chan->band == NL80211_BAND_2GHZ)) {
                bool replace = aml_scc_compare_oper_class(buf[offset + 6],oper_class_new);
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

void aml_change_p2p_intent(struct aml_vif *vif, u8 *buf, u32 frame_len,u32* frame_len_offset)
{
    u8* p_ie_len;
    u32 offset;
    u8 id;
    u16 len;
    bool tie_breaker;
    offset = aml_get_p2p_ie_offset(buf,frame_len);
    //idx pointer to wifi-direct ie
    if (offset != 0) {
        p_ie_len = &buf[offset + 1];
        offset += 6;
        while (offset < frame_len) {
            id = buf[offset];
            len = (buf[offset + 2] << 8) | (buf[offset + 1]);
            if (id == P2P_ATTR_GROUP_OWNER_INTENT) {
                break;
            }
            offset += len + 3;
        }
        tie_breaker = buf[offset + 3] & 0x1;
        buf[offset + 3] = (GO_INTENT_H << 1) | tie_breaker;
    }
}

void aml_rx_parse_p2p_chan_list(u8 *buf, u32 frame_len)
{
    u32 offset = aml_get_p2p_ie_offset(buf, frame_len);
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

        if (is_found == false)
            return;

        AML_SCC_SET_P2P_PEER_5G_SUPPORT(false);
        chan_list_ie_len = buf[offset + 1] | (buf[offset + 2] << 8);
        while (idx < chan_list_ie_len) {
            oper_class = buf[offset + idx];
            oper_class_len = buf[offset + idx + 1];
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
