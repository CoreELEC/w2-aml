/**
 ****************************************************************************************
 *
 * @file aml_testmode.h
 *
 * @brief Test mode function declarations
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#ifndef AML_TESTMODE_H_
#define AML_TESTMODE_H_

#include <net/mac80211.h>
#include <net/netlink.h>

/* Commands from user space to kernel space(AML_TM_CMD_APP2DEV_XX) and
 * from and kernel space to user space(AML_TM_CMD_DEV2APP_XX).
 * The command ID is carried with AML_TM_ATTR_COMMAND.
 */
enum aml_tm_cmd_t {
    /* commands from user application to access register */
    AML_TM_CMD_APP2DEV_REG_READ = 1,
    AML_TM_CMD_APP2DEV_REG_WRITE,

    /* commands from user application to select the Debug levels */
    AML_TM_CMD_APP2DEV_SET_DBGMODFILTER,
    AML_TM_CMD_APP2DEV_SET_DBGSEVFILTER,

    /* commands to access registers without sending messages to LMAC layer,
     * this must be used when LMAC FW is stuck. */
    AML_TM_CMD_APP2DEV_REG_READ_DBG,
    AML_TM_CMD_APP2DEV_REG_WRITE_DBG,

    AML_TM_CMD_MAX,
};

enum aml_tm_attr_t {
    AML_TM_ATTR_NOT_APPLICABLE = 0,

    AML_TM_ATTR_COMMAND,

    /* When AML_TM_ATTR_COMMAND is AML_TM_CMD_APP2DEV_REG_XXX,
     * The mandatory fields are:
     * AML_TM_ATTR_REG_OFFSET for the offset of the target register;
     * AML_TM_ATTR_REG_VALUE32 for value */
    AML_TM_ATTR_REG_OFFSET,
    AML_TM_ATTR_REG_VALUE32,

    /* When AML_TM_ATTR_COMMAND is AML_TM_CMD_APP2DEV_SET_DBGXXXFILTER,
     * The mandatory field is AML_TM_ATTR_REG_FILTER. */
    AML_TM_ATTR_REG_FILTER,

    AML_TM_ATTR_MAX,
};

/***********************************************************************/
int aml_testmode_reg(struct ieee80211_hw *hw, struct nlattr **tb);
int aml_testmode_dbg_filter(struct ieee80211_hw *hw, struct nlattr **tb);
int aml_testmode_reg_dbg(struct ieee80211_hw *hw, struct nlattr **tb);

#endif /* AML_TESTMODE_H_ */
