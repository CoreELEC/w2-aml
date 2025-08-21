/**
 ****************************************************************************************
 *
 * @file aml_msg_rx.h
 *
 * @brief RX function declarations
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#ifndef _AML_MSG_RX_H_
#define _AML_MSG_RX_H_

struct aml_ft_auth_timeout {
    uint8_t vif_idx;
};

void aml_rx_handle_msg(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);
void aml_rx_sdio_ind_msg_handle(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);
void aml_del_sta(struct aml_vif *aml_vif, const u8 *mac_addr, u32 freq);
int aml_send_me_shutdown(struct aml_hw *aml_hw);
void aml_sta_notify_csa_ch_switch(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);

#endif /* _AML_MSG_RX_H_ */
