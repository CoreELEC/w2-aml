/**
****************************************************************************************
*
* @file aml_mdns_offload.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief android mDNS offload
*
****************************************************************************************
*/
#define AML_MODULE  MDNS

#include "aml_mdns_offload.h"
#include "lmac_msg.h"
#include "aml_msg_tx.h"

/// The maximum number of response data that can be added
#define MDNS_INDEX_ERR (-1)
#define MDNS_INDEX_MAX (3)

static u32_boolean setOffloadState(struct aml_hw *aml_hw, u32_boolean enabled)
{
    uint32_t ret;

#ifdef MDNS_OFFLOAD_FEATURE
    if (aml_mdns_set_offload_state(aml_hw, enabled) != 0) {
        AML_ERR(" MDNS_OFFLOAD_FEATURE is enabled!\n");
        ret = false;
        goto exit;
    }
    ret = true;
#else
    AML_ERR(" MDNS_OFFLOAD_FEATURE is disabled!\n");
    aml_mdns_set_offload_state(aml_hw, 0);
    ret = false;
#endif

exit:
    AML_ERR(" enabled:%d,ret:%d\n", enabled, ret);
    return ret;
}

static void resetAll(struct aml_hw *aml_hw)
{
    aml_mdns_reset_all(aml_hw);
}

static int addProtocolResponses(struct aml_hw *aml_hw, char *networkInterface,
    mdnsProtocolData *offloadData)
{
    struct match_criteria list_lmac[MDNS_LIST_CRITERIA_MAX] = {0};
    int i = 0;
    int ret;
    int index = MDNS_INDEX_ERR;

    // change type to reduce fw mem
    for (i = 0; (i < offloadData->matchCriteriaListNum) && (i < MDNS_LIST_CRITERIA_MAX); ++i) {
        list_lmac[i].offset = offloadData->matchCriteriaList[i].nameOffset;
        list_lmac[i].type = offloadData->matchCriteriaList[i].type;
    }

    if (offloadData->rawOffloadPacketLen <= MDNS_RAW_DATA_LENGTH_MAX)
    {
        ret = aml_mdns_add_protocol_data_status(aml_hw, list_lmac, offloadData, &index); //data size err
        if (ret == 0)
            aml_mdns_add_protocol_data(aml_hw, offloadData->rawOffloadPacket, index, offloadData->rawOffloadPacketLen);
    }
    else
    {
        AML_ERR(" mdns frame size err\n");
    }

    return index;
}

static void removeProtocolResponses(struct aml_hw *aml_hw, int recordKey)
{
    aml_mdns_remove_protocol_data(aml_hw, recordKey);
}

static int getAndResetHitCounter(struct aml_hw *aml_hw, int recordKey)
{
    return aml_mdns_get_reset_hit_counter(aml_hw, recordKey);
}

static int getAndResetMissCounter(struct aml_hw *aml_hw)
{
    return aml_mdns_get_reset_miss_counter(aml_hw);
}

static u32_boolean addToPassthroughList(struct aml_hw *aml_hw, char *networkInterface, char *qname)
{
    if (aml_mdns_add_passthrough_list(aml_hw, qname, strlen(qname)) != 0)
        return false;
    return true;
}

static void removeFromPassthroughList(struct aml_hw *aml_hw, char *networkInterface, char *qname)
{
    aml_mdns_remove_passthrough_list(aml_hw, qname, strlen(qname));
}

static void setPassthroughBehavior(struct aml_hw *aml_hw, char *networkInterface,
    passthroughBehavior behavior)
{
    aml_mdns_set_passthrough_behavior(aml_hw, behavior);
}

static void setWakePorts(struct aml_hw *aml_hw, wakePort_set *ports)
{
    int ret = aml_mdns_set_wake_ports(aml_hw, ports);

    if (ret)
        AML_ERR("set wake port fail\n");
}

const struct s_mdns_offload_ops mdns_offload_ops = {
    .setOffloadState = setOffloadState,
#ifdef MDNS_OFFLOAD_FEATURE
    .resetAll = resetAll,
    .addProtocolResponses = addProtocolResponses,
    .removeProtocolResponses = removeProtocolResponses,
    .getAndResetHitCounter = getAndResetHitCounter,
    .getAndResetMissCounter = getAndResetMissCounter,
    .addToPassthroughList = addToPassthroughList,
    .removeFromPassthroughList = removeFromPassthroughList,
    .setPassthroughBehavior = setPassthroughBehavior,
    .setWakePorts = setWakePorts,
#else
    .setOffloadState = NULL,
    .resetAll = NULL,
    .addProtocolResponses = NULL,
    .removeProtocolResponses = NULL,
    .getAndResetHitCounter = NULL,
    .getAndResetMissCounter = NULL,
    .addToPassthroughList = NULL,
    .removeFromPassthroughList = NULL,
    .setPassthroughBehavior = NULL,
    .setWakePorts = NULL,
#endif
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
const struct nla_policy mdns_offload_attr_policy[WIFI_MDNS_OFFLOAD_ATTRIBUTE_MAX] = {
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_STATE]                = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_NETWORK_INTERFACE]    = { .type = NLA_NUL_STRING },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_LEN]      = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_OFFLOAD_PKT_DATA]     = { .type = NLA_BINARY },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_NUM]   = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_MATCH_CRITERIA_DATA]  = { .type = NLA_BINARY },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_RECORD_KEY]           = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_QNAME]                = { .type = NLA_NUL_STRING },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_PASSTHROUGH_BEHAVIOR] = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_WAKE_PORTS_NUM]       = { .type = NLA_U32, .len = sizeof(uint32_t) },
    [WIFI_MDNS_OFFLOAD_ATTRIBUTE_WAKE_PORTS]           = { .type = NLA_BINARY },
};
#endif
