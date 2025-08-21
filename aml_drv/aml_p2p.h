#ifndef _AML_P2P_H_
#define _AML_P2P_H_

#include "aml_defs.h"
#include "aml_scc.h"

#define P2P_ACTION_HDR_LEN          8
#define GO_INTENT_H                 15
#define GO_INTENT_L                 0
#define P2P_ELEMENT_HDR_LEN         6
#define P2P_ATT_COUNTRY_STR_LEN     3
#define P2P_ATT_BODY_OFT            3

enum p2p_attr_id
{
    P2P_ATTR_STATUS = 0,
    P2P_ATTR_MINOR_REASON_CODE = 1,
    P2P_ATTR_CAPABILITY = 2,
    P2P_ATTR_DEVICE_ID = 3,
    P2P_ATTR_GROUP_OWNER_INTENT = 4,
    P2P_ATTR_CONFIGURATION_TIMEOUT = 5,
    P2P_ATTR_LISTEN_CHANNEL = 6,
    P2P_ATTR_GROUP_BSSID = 7,
    P2P_ATTR_EXT_LISTEN_TIMING = 8,
    P2P_ATTR_INTENDED_INTERFACE_ADDR = 9,
    P2P_ATTR_MANAGEABILITY = 10,
    P2P_ATTR_CHANNEL_LIST = 11,
    P2P_ATTR_NOTICE_OF_ABSENCE = 12,
    P2P_ATTR_DEVICE_INFO = 13,
    P2P_ATTR_GROUP_INFO = 14,
    P2P_ATTR_GROUP_ID = 15,
    P2P_ATTR_INTERFACE = 16,
    P2P_ATTR_OPERATING_CHANNEL = 17,
    P2P_ATTR_INVITATION_FLAGS = 18,
    P2P_ATTR_VENDOR_SPECIFIC = 221
};

/* P2P Public Action Frame Types */
enum p2p_action_type {
    P2P_ACTION_GO_NEG_REQ   = 0,    /* GO Negociation Request */
    P2P_ACTION_GO_NEG_RSP,          /* GO Negociation Response */
    P2P_ACTION_GO_NEG_CFM,          /* GO Negociation Confirmation */
    P2P_ACTION_INVIT_REQ,           /* P2P Invitation Request */
    P2P_ACTION_INVIT_RSP,           /* P2P Invitation Response */
    P2P_ACTION_DEV_DISC_REQ,        /* Device Discoverability Request */
    P2P_ACTION_DEV_DISC_RSP,        /* Device Discoverability Response */
    P2P_ACTION_PROV_DISC_REQ,       /* Provision Discovery Request */
    P2P_ACTION_PROV_DISC_RSP,       /* Provision Discovery Response */
};

extern char p2p_pub_action_trace[][30];
extern char p2p_action_trace[][30];
extern u32 aml_get_p2p_ie_offset(const u8 *buf,u32 frame_len);
extern void aml_change_p2p_chanlist(struct aml_vif *vif, u8 *buf, u32 frame_len,u32* frame_len_offset,struct cfg80211_chan_def chan_def);
extern void aml_change_p2p_intent(struct aml_vif *vif, u8 *buf, u32 frame_len,u32* frame_len_offset);
extern void aml_change_p2p_operchan(struct aml_vif *vif, u8 *buf, u32 frame_len, struct cfg80211_chan_def chan_def);
void aml_rx_parse_p2p_chan_list(u8 *buf, u32 frame_len);


#endif /* _AML_P2P_H_ */
