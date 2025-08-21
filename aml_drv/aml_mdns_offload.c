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

#include "aml_mdns_offload.h"
#include "lmac_msg.h"
#include "aml_msg_tx.h"

/// The maximum number of response data that can be added
#define MDNS_INDEX_ERR              (-1)
#define MDNS_INDEX_MAX              (3)
#define MDNS_RAW_DATA_LENGTH_MAX    (492)

extern struct auc_hif_ops g_auc_hif_ops;
extern void aml_pci_writel(u32 data, u8* addr);

int g_mdns_offload_debug = 0;

static u32_boolean setOffloadState(struct aml_hw *aml_hw, u32_boolean enabled)
{
    uint32_t ret;

#ifdef MDNS_OFFLOAD_FEATURE
    if (aml_mdns_set_offload_state(aml_hw, enabled) != 0) {
        ret = false;
        goto exit;
    }
    ret = true;
#else
    printk("%s: MDNS_OFFLOAD_FEATURE is disabled!\n", __func__);
    aml_mdns_set_offload_state(aml_hw, 0);
    ret = false;
#endif

exit:
    printk("%s: enabled:%d,ret:%d\n", __func__, enabled, ret);
    return ret;
}

static void resetAll(struct aml_hw *aml_hw)
{
    aml_mdns_reset_all(aml_hw);
}

static int addProtocolResponses(struct aml_hw *aml_hw, char *networkInterface,
    mdnsProtocolData *offloadData)
{
    matchCriteria *list = offloadData->matchCriteriaList;
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
        printk("%s mdns frame size err\n", __func__);
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

ANDROID_MDNS_OFFLOAD_VENDOR_IMPL = {
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
#endif
};

