/**
 ******************************************************************************
 *
 * @file aml_scc.c
 *
 * @brief handle sta + softap scc modes
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#include "aml_msg_tx.h"
#include "fi_cmd.h"
#include "share_mem_map.h"
#include "aml_sap.h"
#include "aml_wq.h"

u8 bcn_save[AML_SCC_FRMBUF_MAXLEN];

/**
 * This function is called when start ap,and save bcn_buf
 *
 */
void aml_save_bcn_buf(u8 *bcn_buf, size_t len)
{
    memcpy(bcn_save, bcn_buf, len);
}

u8* aml_get_beacon_ie_addr(u8* buf, int frame_len,u8 eid)
{
    int len;
    u8 *var_pos;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    len = frame_len - var_offset;
    var_pos = buf + var_offset;
    return (u8*)cfg80211_find_ie(eid, var_pos, len);
}

u8* aml_get_probe_rsp_ie_addr(u8* buf, int frame_len,u8 eid)
{
    int len;
    u8 *var_pos;
    int var_offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
    len = frame_len - var_offset;
    var_pos = buf + var_offset;
    return (u8*)cfg80211_find_ie(eid, var_pos, len);
}

