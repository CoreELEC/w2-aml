
/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#ifndef _AML_SCC_H_
#define _AML_SCC_H_

#include "aml_defs.h"
#include "aml_wq.h"

enum
{
    BEACON_NO_UPDATE,
    BEACON_UPDATE_WAIT_PROBE,
    BEACON_UPDATE_WAIT_DOWNLOAD
};

#define AML_SCC_BEACON_SET_STATUS(x)        (beacon_need_update = x)
#define AML_SCC_BEACON_WAIT_PROBE()         (beacon_need_update == BEACON_UPDATE_WAIT_PROBE)
#define AML_SCC_BEACON_WAIT_DOWNLOAD()      (beacon_need_update == BEACON_UPDATE_WAIT_DOWNLOAD)
#define AML_SCC_CLEAR_BEACON_UPDATE()       (beacon_need_update = 0)
/*scc for p2p*/
#define AML_SCC_SAVE_P2P_ACTION_FRAME(buf, frame_len) (memcpy(g_scc_p2p_save, buf, frame_len));
#define AML_SCC_SAVE_P2P_ACTION_LEN(frame_len) (g_scc_p2p_len_before = frame_len)
#define AML_SCC_SAVE_P2P_ACTION_LEN_DIFF(len_diff) (g_scc_p2p_len_diff = len_diff)
#define AML_SCC_SET_P2P_PEER_5G_SUPPORT(support) (g_scc_p2p_peer_5g_support = support)
#define AML_SCC_GET_P2P_PEER_5G_SUPPORT() (g_scc_p2p_peer_5g_support)
#define MAX_P2P_SAVE_LEN 500

extern u32 beacon_need_update;
extern u8 g_scc_p2p_save[];
extern u32 g_scc_p2p_len_before;
extern u32 g_scc_p2p_len_diff;
extern bool g_scc_p2p_peer_5g_support;

int aml_scc_change_beacon(struct aml_hw *aml_hw, struct aml_vif *vif);
u8 aml_scc_get_conflict_vif_idx(struct aml_vif *incoming_vif);
int aml_scc_check_chan_conflict(struct aml_hw *aml_hw);
void aml_scc_save_init_band(u32 init_band);
void aml_scc_save_probe_rsp(struct aml_vif *vif, u8 *buf, u32 buf_len);
void aml_scc_deinit(void);
void aml_check_scc(void);
void aml_scc_p2p_action_restore(u8 *buf, u32* len_diff);

#endif /* _AML_SCC_H_ */
