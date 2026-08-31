/**
 ******************************************************************************
 *
 * aml_cmds.h
 *
 * Copyright (C) Amlogic 2014-2021
 *
 ******************************************************************************
 */

#ifndef _AML_CMDS_H_
#define _AML_CMDS_H_

#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include "lmac_msg.h"
#include "usb_common.h"
#include "sdio_common.h"

#ifdef CONFIG_PT_MODE
    #define AML_80211_CMD_TIMEOUT_MS    1000
#else
    #define AML_80211_CMD_TIMEOUT_MS    3000
#endif

#define AML_CMD_FLAG_NONBLOCK      BIT(0)
#define AML_CMD_FLAG_REQ_CFM       BIT(1)
#define AML_CMD_FLAG_WAIT_PUSH     BIT(2)
#define AML_CMD_FLAG_WAIT_ACK      BIT(3)
#define AML_CMD_FLAG_WAIT_CFM      BIT(4)
#define AML_CMD_FLAG_DONE          BIT(5)
#define AML_CMD_FLAG_CALL_THREAD   BIT(6)
/* ATM IPC design makes it possible to get the CFM before the ACK,
 * otherwise this could have simply been a state enum */
#define AML_CMD_WAIT_COMPLETE(flags) \
    (!(flags & (AML_CMD_FLAG_WAIT_ACK | AML_CMD_FLAG_WAIT_CFM)))

#define AML_CMD_MAX_QUEUED         8
#define AML_FW_PC_POINTER   0x00a070b4
#define CMD_CRASH_FW_PC_NUM 5

#ifdef CONFIG_AML_FHOST
#include "ipc_fhost.h"
#define aml_cmd_e2amsg ipc_fhost_msg
#define aml_cmd_a2emsg ipc_fhost_msg
#define AML_CMD_A2EMSG_LEN(m) (m->param_len)
#define AML_CMD_E2AMSG_LEN_MAX IPC_FHOST_MSG_BUF_SIZE
struct aml_term_stream;

#else /* !CONFIG_AML_FHOST*/
#include "ipc_shared.h"
#define aml_cmd_e2amsg ipc_e2a_msg
#define aml_cmd_a2emsg lmac_msg
#define AML_CMD_A2EMSG_LEN(m) (uint16_t)(sizeof(struct lmac_msg) + m->param_len)
#define AML_CMD_E2AMSG_LEN_MAX (IPC_E2A_MSG_PARAM_SIZE * 4)

#endif /* CONFIG_AML_FHOST*/

struct aml_hw;
struct aml_cmd;
typedef int (*msg_cb_fct)(struct aml_hw *aml_hw, struct aml_cmd *cmd,
                          struct aml_cmd_e2amsg *msg);

enum aml_cmd_mgr_state {
    AML_CMD_MGR_STATE_DEINIT,
    AML_CMD_MGR_STATE_INITED,
    AML_CMD_MGR_STATE_CRASHED,
};

struct aml_cmd {
    struct list_head list;
    lmac_msg_id_t id;
    lmac_msg_id_t reqid;
    struct aml_cmd_a2emsg *a2e_msg;
    char *e2a_msg;
    u32 tkn;
    u16 flags;
    u16 mm_sub_id;
    struct completion complete;
    u32 result;
    #ifdef CONFIG_AML_FHOST
    struct aml_term_stream *stream;
    #endif
};

struct aml_cmd_mgr {
    enum aml_cmd_mgr_state state;
    spinlock_t lock;
    u32 next_tkn;
    u32 queue_sz;
    u32 max_queue_sz;

    struct list_head cmds;

    int  (*queue)(struct aml_cmd_mgr *, struct aml_cmd *);
    int  (*llind)(struct aml_cmd_mgr *, struct aml_cmd *);
    int  (*msgind)(struct aml_cmd_mgr *, struct aml_cmd_e2amsg *, msg_cb_fct);
    void (*print)(struct aml_cmd_mgr *);
    void (*drain)(struct aml_cmd_mgr *);
};

void aml_cmd_mgr_init(struct aml_cmd_mgr *cmd_mgr);
void aml_cmd_mgr_deinit(struct aml_cmd_mgr *cmd_mgr);

#endif /* _AML_CMDS_H_ */
