/**
 ****************************************************************************************
 *
 * @file aml_strs.h
 *
 * @brief Miscellaneous debug strings
 *
 * Copyright (C) Amlogic 2014-2021
 *
 ****************************************************************************************
 */

#ifndef _AML_STRS_H_
#define _AML_STRS_H_

#ifdef CONFIG_AML_FHOST

#define AML_ID2STR(tag) "Cmd"

#else
#include "lmac_msg.h"

extern const char *const *aml_id2str[TASK_LAST_EMB + 1];
extern const char *const aml_mm_other_id2str[MM_SUB_A2E_MAX];

#define AML_MM_OTHER_CMD2STR(cmd) ((cmd && cmd->mm_sub_id < ARRAY_SIZE(aml_mm_other_id2str)) ? \
                                    aml_mm_other_id2str[cmd->mm_sub_id] : "unknown")

#define AML_ID2STR(tag) (((MSG_T(tag) < ARRAY_SIZE(aml_id2str)) &&        \
                           (aml_id2str[MSG_T(tag)]) &&          \
                           ((aml_id2str[MSG_T(tag)])[MSG_I(tag)])) ?   \
                          (aml_id2str[MSG_T(tag)])[MSG_I(tag)] : "unknown")

#define MSG2STR_FORMANT "cmd:%4d-%-24s"
#define AML_MSG2STR(msg) ((msg->id == MM_OTHER_REQ) ? *(msg->param) : msg->id), ((msg->id == MM_OTHER_REQ) ?  \
                ((*(msg->param) < ARRAY_SIZE(aml_mm_other_id2str)) ? \
                (aml_mm_other_id2str[*(msg->param)] ? aml_mm_other_id2str[*(msg->param)] : "MM_OTHER_REQ") : \
                "unknown") : AML_ID2STR(msg->id))

#endif /* CONFIG_AML_FHOST */

#endif /* _AML_STRS_H_ */
