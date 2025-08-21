/**
 ******************************************************************************
 *
 * @file aml_msg_tx.c
 *
 * @brief TX function definitions
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#include "aml_msg_tx.h"
#include "aml_mod_params.h"
#include "reg_access.h"
#ifdef CONFIG_AML_BFMER
#include "aml_bfmer.h"
#endif //(CONFIG_AML_BFMER)
#include "aml_compat.h"
#include "fi_cmd.h"
#include "share_mem_map.h"
#include "aml_scc.h"
#include "aml_recy.h"
#include "aml_wq.h"
#include "aml_sap.h"
#include "chip_intf_reg.h"
#include "aml_compat.h"
#include "aml_mdns_offload.h"

const struct mac_addr mac_addr_bcst = {{0xFFFF, 0xFFFF, 0xFFFF}};

/* Default MAC Rx filters that can be changed by mac80211
 * (via the configure_filter() callback) */
#define AML_MAC80211_CHANGEABLE        (                                       \
                                         NXMAC_ACCEPT_BA_BIT                  | \
                                         NXMAC_ACCEPT_BAR_BIT                 | \
                                         NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT   | \
                                         NXMAC_ACCEPT_PROBE_REQ_BIT           | \
                                         NXMAC_ACCEPT_PS_POLL_BIT               \
                                        )

/* Default MAC Rx filters that cannot be changed by mac80211 */
#define AML_MAC80211_NOT_CHANGEABLE    (                                       \
                                         NXMAC_ACCEPT_QO_S_NULL_BIT           | \
                                         NXMAC_ACCEPT_Q_DATA_BIT              | \
                                         NXMAC_ACCEPT_DATA_BIT                | \
                                         NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT   | \
                                         NXMAC_ACCEPT_MY_UNICAST_BIT          | \
                                         NXMAC_ACCEPT_BROADCAST_BIT           | \
                                         NXMAC_ACCEPT_BEACON_BIT              | \
                                         NXMAC_ACCEPT_PROBE_RESP_BIT            \
                                        )

/* Default MAC Rx filter */
#define AML_DEFAULT_RX_FILTER  (AML_MAC80211_CHANGEABLE | AML_MAC80211_NOT_CHANGEABLE)

const int bw2chnl[] = {
    [NL80211_CHAN_WIDTH_20_NOHT] = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_20]      = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_40]      = PHY_CHNL_BW_40,
    [NL80211_CHAN_WIDTH_80]      = PHY_CHNL_BW_80,
    [NL80211_CHAN_WIDTH_160]     = PHY_CHNL_BW_160,
    [NL80211_CHAN_WIDTH_80P80]   = PHY_CHNL_BW_80P80,
};

const int chnl2bw[] = {
    [PHY_CHNL_BW_20]      = NL80211_CHAN_WIDTH_20,
    [PHY_CHNL_BW_40]      = NL80211_CHAN_WIDTH_40,
    [PHY_CHNL_BW_80]      = NL80211_CHAN_WIDTH_80,
    [PHY_CHNL_BW_160]     = NL80211_CHAN_WIDTH_160,
    [PHY_CHNL_BW_80P80]   = NL80211_CHAN_WIDTH_80P80,
};

#ifdef CONFIG_AML_SOFTMAC
/*****************************************************************************/
/*
 * Parse the ampdu density to retrieve the value in usec, according to the
 * values defined in ieee80211.h
 */
static inline u8 aml_ampdudensity2usec(u8 ampdudensity)
{
    switch (ampdudensity) {
    case IEEE80211_HT_MPDU_DENSITY_NONE:
        return 0;
        /* 1 microsecond is our granularity */
    case IEEE80211_HT_MPDU_DENSITY_0_25:
    case IEEE80211_HT_MPDU_DENSITY_0_5:
    case IEEE80211_HT_MPDU_DENSITY_1:
        return 1;
    case IEEE80211_HT_MPDU_DENSITY_2:
        return 2;
    case IEEE80211_HT_MPDU_DENSITY_4:
        return 4;
    case IEEE80211_HT_MPDU_DENSITY_8:
        return 8;
    case IEEE80211_HT_MPDU_DENSITY_16:
        return 16;
    default:
        return 0;
    }
}
#endif

static inline bool use_pairwise_key(struct cfg80211_crypto_settings *crypto)
{
    if ((crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP40) ||
        (crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP104))
        return false;

    return true;
}

static inline bool is_non_blocking_msg(int id)
{
    return ((id == MM_TIM_UPDATE_REQ) || (id == ME_RC_SET_RATE_REQ) ||
            (id == MM_BFMER_ENABLE_REQ) || (id == ME_TRAFFIC_IND_REQ) ||
            (id == TDLS_PEER_TRAFFIC_IND_REQ) ||
            (id == MESH_PATH_CREATE_REQ) || (id == MESH_PROXY_ADD_REQ) ||
            (id == SM_EXTERNAL_AUTH_REQUIRED_RSP));
}

bool aml_msg_send_mtheod(int id)
{
    return ((id == ME_TRAFFIC_IND_REQ) ||
        (id == SM_EXTERNAL_AUTH_REQUIRED_RSP));
}


#ifdef CONFIG_AML_FULLMAC
/**
 * copy_connect_ies -- Copy Association Elements in the the request buffer
 * send to the firmware
 *
 * @vif: Vif that received the connection request
 * @req: Connection request to send to the firmware
 * @sme: Connection info
 *
 * For driver that do not use userspace SME (like this one) the host connection
 * request doesn't explicitly mentions that the connection can use FT over the
 * air. if FT is possible, send the FT elements (as received in update_ft_ies callback)
 * to the firmware
 *
 * In all other cases simply copy the list povided by the user space in the
 * request buffer
 */
static void copy_connect_ies(struct aml_vif *vif, struct sm_connect_req *req,
                            struct cfg80211_connect_params *sme)
{
    if ((sme->auth_type == NL80211_AUTHTYPE_FT) && !(vif->sta.flags & AML_STA_FT_OVER_DS))
    {
        const struct element *rsne, *fte, *mde;
        uint8_t *pos;
        rsne = cfg80211_find_elem(WLAN_EID_RSN, vif->sta.ft_assoc_ies,
                                    vif->sta.ft_assoc_ies_len);
        fte = cfg80211_find_elem(WLAN_EID_FAST_BSS_TRANSITION, vif->sta.ft_assoc_ies,
                                    vif->sta.ft_assoc_ies_len);
        mde = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN,
                                         vif->sta.ft_assoc_ies, vif->sta.ft_assoc_ies_len);
        pos = (uint8_t *)req->ie_buf;

        // We can use FT over the air
        if (sme->bssid) {
            memcpy(&vif->sta.ft_target_ap, sme->bssid, ETH_ALEN);
        }

        if (rsne) {
            memcpy(pos, rsne, sizeof(struct element) + rsne->datalen);
            pos += sizeof(struct element) + rsne->datalen;
        }

        if (mde) {
            memcpy(pos, mde, sizeof(struct element) + mde->datalen);
            pos += sizeof(struct element) + mde->datalen;
        }

        if (fte) {
            memcpy(pos, fte, sizeof(struct element) + fte->datalen);
            pos += sizeof(struct element) + fte->datalen;
        }

        req->ie_len = pos - (uint8_t *)req->ie_buf;
    }
    else
    {
        memcpy(req->ie_buf, sme->ie, sme->ie_len);
        req->ie_len = sme->ie_len;
    }
}

/**
 * update_connect_req -- Return the length of the association request IEs
 *
 * @vif: Vif that received the connection request
 * @sme: Connection info
 *
 * Return the ft_ie_len in case of FT.
 * FT over the air is possible if:
 * - auth_type = AUTOMATIC (if already set to FT then it means FT over DS)
 * - already associated to a FT BSS
 * - Target Mobility domain is the same as the curent one
 *
 * If FT is not possible return ie length of the connection info
 */
static int update_connect_req(struct aml_vif *vif, struct cfg80211_connect_params *sme)
{
    if ((vif->sta.ap) &&
        (vif->sta.ft_assoc_ies) &&
        (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC))
    {
        const struct element *rsne, *fte, *mde, *mde_req;
        int ft_ie_len = 0;

        mde_req = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN,
                                     sme->ie, sme->ie_len);
        mde = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN,
                                 vif->sta.ft_assoc_ies, vif->sta.ft_assoc_ies_len);
        if (!mde || !mde_req ||
            memcmp(mde, mde_req, sizeof(struct element) + mde->datalen))
        {
            return sme->ie_len;
        }

        ft_ie_len += sizeof(struct element) + mde->datalen;

        rsne = cfg80211_find_elem(WLAN_EID_RSN, vif->sta.ft_assoc_ies,
                                    vif->sta.ft_assoc_ies_len);
        fte = cfg80211_find_elem(WLAN_EID_FAST_BSS_TRANSITION, vif->sta.ft_assoc_ies,
                                    vif->sta.ft_assoc_ies_len);

        if (rsne && fte)
        {
            ft_ie_len += 2 * sizeof(struct element) + rsne->datalen + fte->datalen;
            sme->auth_type = NL80211_AUTHTYPE_FT;
            return ft_ie_len;
        }
        else if (rsne || fte)
        {
            netdev_warn(vif->ndev, "Missing RSNE or FTE element, skip FT over air");
        }
        else
        {
            sme->auth_type = NL80211_AUTHTYPE_FT;
            return ft_ie_len;
        }
    }
    return sme->ie_len;
}
#endif

static inline u8_l get_chan_flags(uint32_t flags)
{
    u8_l chan_flags = 0;
    if (flags & IEEE80211_CHAN_NO_IR)
        chan_flags |= CHAN_NO_IR;
    if (flags & IEEE80211_CHAN_RADAR)
        chan_flags |= CHAN_RADAR;
    return chan_flags;
}

static inline s8_l chan_to_fw_pwr(int power)
{
    return power > 127 ? 127 : (s8_l)power;
}

void cfg80211_to_aml_chan(const struct cfg80211_chan_def *chandef, struct mac_chan_op *chan)
{
    chan->band = chandef->chan->band;
    chan->type = bw2chnl[chandef->width];
    chan->prim20_freq = chandef->chan->center_freq;
    chan->center1_freq = chandef->center_freq1;
    chan->center2_freq = chandef->center_freq2;
    chan->flags = get_chan_flags(chandef->chan->flags);
    chan->tx_power = chan_to_fw_pwr(chandef->chan->max_power);
}

static inline void limit_chan_bw(u8_l *bw, u16_l primary, u16_l *center1)
{
    int oft, new_oft = 10;

    if (*bw <= PHY_CHNL_BW_40)
        return;

    oft = *center1 - primary;
    *bw = PHY_CHNL_BW_40;

    if (oft < 0)
        new_oft = new_oft * -1;
    if (abs(oft) == 10 || abs(oft) == 50)
        new_oft = new_oft * -1;

    *center1 = primary + new_oft;
}

/**
 ******************************************************************************
 * @brief Allocate memory for a message
 *
 * This primitive allocates memory for a message that has to be sent. The memory
 * is allocated dynamically on the heap and the length of the variable parameter
 * structure has to be provided in order to allocate the correct size.
 *
 * Several additional parameters are provided which will be preset in the message
 * and which may be used internally to choose the kind of memory to allocate.
 *
 * The memory allocated will be automatically freed by the kernel, after the
 * pointer has been sent to ke_msg_send(). If the message is not sent, it must
 * be freed explicitly with ke_msg_free().
 *
 * Allocation failure is considered critical and should not happen.
 *
 * @param[in] id        Message identifier
 * @param[in] dest_id   Destination Task Identifier
 * @param[in] src_id    Source Task Identifier
 * @param[in] param_len Size of the message parameters to be allocated
 *
 * @return Pointer to the parameter member of the ke_msg. If the parameter
 *         structure is empty, the pointer will point to the end of the message
 *         and should not be used (except to retrieve the message pointer or to
 *         send the message)
 ******************************************************************************
 */
static inline void *aml_msg_zalloc(lmac_msg_id_t const id,
                                    lmac_task_id_t const dest_id,
                                    lmac_task_id_t const src_id,
                                    uint16_t const param_len)
{
    struct lmac_msg *msg;
    gfp_t flags;
    uint16_t payl_len = param_len;

    if (is_non_blocking_msg(id) && in_softirq())
        flags = GFP_ATOMIC;
    else
        flags = GFP_KERNEL;

    if (aml_bus_type != PCIE_MODE) {
        if (id == ME_TRAFFIC_IND_REQ) {
            flags = GFP_ATOMIC;
        }
    }

    //make param_len ALIGN4 HI
    payl_len = (((payl_len)+3)&~3);
    msg = (struct lmac_msg *)kzalloc(sizeof(struct lmac_msg) + payl_len,
                                     flags);
    if (msg == NULL) {
        printk(KERN_CRIT "%s: msg allocation failed\n", __func__);
        return NULL;
    }

    msg->id = id;
    msg->dest_id = dest_id;
    msg->src_id = src_id;
    msg->param_len = payl_len;

    /* coverity[leaked_storage] - msg will free in @aml_send_msg or in  @cmd_complete*/
    return msg->param;
}

/**
 ******************************************************************************
 * @brief Allocate memory for a sub message
 *
 * @param[in] id        Sub message identifier
 * @param[in] param_len Size of the message parameters to be allocated
 *
 * @return Pointer to the parameter member of the mm_other_cmd. memery
 *          usb direct
 ******************************************************************************
 */
static void *aml_priv_msg_zalloc(lmac_msg_id_t const id,
                                        uint16_t const param_len)
{
    struct mm_other_cmd *mm_sub_cmd = (struct mm_other_cmd *)aml_msg_zalloc(MM_OTHER_REQ, TASK_MM, DRV_TASK_ID,
                                                         sizeof(struct mm_other_cmd) + param_len);
    if (!mm_sub_cmd)
        return NULL;
    mm_sub_cmd->mm_sub_index = id;
    /* coverity[leaked_storage] - mm_sub_cmd will be freed later */
    return mm_sub_cmd->param;
}

static void aml_msg_free(struct aml_hw *aml_hw, const void *msg_params)
{
    struct lmac_msg *msg = container_of((void *)msg_params,
                                        struct lmac_msg, param);

    AML_DBG(AML_FN_ENTRY_STR);

    /* Free the message */
    kfree(msg);
}

static void aml_priv_msg_free(struct aml_hw *aml_hw, const void *msg_params)
{
    void *msg;

    if (!msg_params)
        return;

    msg = container_of((void *)msg_params, struct mm_other_cmd, param);

    aml_msg_free(aml_hw, msg);
}
extern struct aml_bus_state_detect bus_state_detect;
static int aml_send_msg(struct aml_hw *aml_hw, const void *msg_params,
                         int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
    struct lmac_msg *msg;
    struct aml_cmd *cmd;
    struct mm_other_cmd *other_cmd;
    bool nonblock;
    bool call_thread;
    int ret;
    uint32_t id;

    msg = container_of((void *)msg_params, struct lmac_msg, param);
    id = msg->id;
    if (id == MM_OTHER_REQ) {
        other_cmd = (struct mm_other_cmd *)msg_params;
        id = other_cmd->mm_sub_index;
    }

    if (is_mdnsoffload_msg(id))
        MDNS_OFFLOAD_DEBUG(AML_FN_ENTRY_STR);
    else
        AML_DBG(AML_FN_ENTRY_STR);

#ifdef CONFIG_AML_RECOVERY
    if ((aml_bus_type != PCIE_MODE) && (bus_state_detect.bus_err)) {
        kfree(msg);
        return 0;
    }
#endif
    if ((aml_hw->state > WIFI_SUSPEND_STATE_NONE || g_pci_msg_suspend)
        && ((msg->param_len != 0) && (*(msg->param) != MM_SUB_SET_SUSPEND_REQ))
        && ((msg->param_len != 0) && (*(msg->param) != MM_SUB_SCANU_CANCEL_REQ && (*(msg->param) != MM_SUB_SHUTDOWN)))
#ifdef CONFIG_AML_RECOVERY
        && (!aml_recy_flags_chk(AML_RECY_STATE_ONGOING))
#endif

    ) {
        printk("driver in suspend, cmd not allow to send, id:%d,aml_hw->state:%d g_pci_msg_suspend:%d\n",
            msg->id, aml_hw->state, g_pci_msg_suspend);
        kfree(msg);
        return -EBUSY;
    }
#ifdef CONFIG_AML_RECOVERY
    if ((aml_recy != NULL) && (aml_recy_flags_chk(AML_RECY_IPC_ONGOING)))
    {
        printk("ipc recy ongoing, cmd not allow to send, id:%d\n", msg->id);
        kfree(msg);
        return -EBUSY;
    }
#endif

    if (!test_bit(AML_DEV_STARTED, (void*)&aml_hw->flags) &&
        reqid != MM_RESET_CFM && reqid != MM_VERSION_CFM &&
        reqid != MM_START_CFM && reqid != MM_SET_IDLE_CFM &&
        reqid != MM_MSG_BYPASS_ID &&
        reqid != ME_CONFIG_CFM && reqid != MM_SET_PS_MODE_CFM &&
        reqid != ME_CHAN_CONFIG_CFM && reqid != PRIV_EFUSE_READ_RESULT) {
        printk(KERN_CRIT "%s: bypassing (AML_DEV_RESTARTING set) 0x%02x\n",
               __func__, reqid);
        kfree(msg);
        return -EBUSY;
    } else if (!aml_hw->ipc_env) {
        printk(KERN_CRIT "%s: bypassing (restart must have failed)\n", __func__);
        kfree(msg);
        return -EBUSY;
    }

    nonblock = is_non_blocking_msg(msg->id);
    if ((msg->param_len != 0) && (*(msg->param) == MM_SUB_SHUTDOWN) && (msg->id == MM_OTHER_REQ)) {
           nonblock = true;
    }

    call_thread = aml_msg_send_mtheod(msg->id);

    cmd = kzalloc(sizeof(struct aml_cmd), nonblock ? GFP_ATOMIC : GFP_KERNEL);
    if (!cmd) {
        kfree(msg);
        return -ENOMEM;
    }
    memset(cmd, 0, sizeof(struct aml_cmd));

    cmd->result  = -EINTR;
    cmd->id      = msg->id;
    cmd->reqid   = reqid;
    cmd->a2e_msg = msg;
    cmd->e2a_msg = cfm;
    if (cmd->id == MM_OTHER_REQ)
        cmd->mm_sub_id = ((struct mm_other_cmd *)msg_params)->mm_sub_index;
    if (nonblock)
        cmd->flags = AML_CMD_FLAG_NONBLOCK;
    if (reqcfm)
        cmd->flags |= AML_CMD_FLAG_REQ_CFM;
    if (call_thread)
        cmd->flags |= AML_CMD_FLAG_CALL_THREAD;
    if (aml_hw->cmd_mgr.queue)
        ret = aml_hw->cmd_mgr.queue(&aml_hw->cmd_mgr, cmd);
    else
        printk("====> aml_hw->cmd_mgr.queue null xxxxxxx\n");

    if (!ret) {
        if (nonblock) {
            ret = 0;
        } else {
            ret = cmd->result;
        }
    }

    if (!nonblock)
        kfree(cmd);

    /* coverity[leaked_storage] - if nonblock cmd ,cmd will be freed in @cmd_complete */
    return ret;
}

/**
 ******************************************************************************
 * @brief send a sub message
 *
 * get lmac_msg->param and call origin send msg interface.
 *
 * @param[in] msg_params    sub_message addr
 *
 * @return status
 ******************************************************************************
 */
int aml_priv_send_msg(struct aml_hw *aml_hw, const void *msg_params,
                         int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
    void *msg;

    if (msg_params == NULL)
        return -ENOMEM;

    msg = container_of((void *)msg_params, struct mm_other_cmd, param);

    return aml_send_msg(aml_hw, msg, reqcfm, reqid, cfm);
}

/******************************************************************************
 *    Control messages handling functions (SOFTMAC and  FULLMAC)
 *****************************************************************************/
int aml_send_reset(struct aml_hw *aml_hw)
{
    void *void_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* RESET REQ has no parameter */
    void_param = aml_msg_zalloc(MM_RESET_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

    printk("%s:%d\n", __func__, __LINE__);
    /* coverity[leaked_storage] - void_param will be free later */
    return aml_send_msg(aml_hw, void_param, 1, MM_RESET_CFM, NULL);
}

int aml_send_start(struct aml_hw *aml_hw)
{
    struct mm_start_req *start_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

#ifdef CONFIG_AML_RECOVERY
    if ((aml_bus_type != PCIE_MODE) && (bus_state_detect.bus_err)) {
        return -EBUSY;
    }
#endif
    /* Build the START REQ message */
    start_req_param = aml_msg_zalloc(MM_START_REQ, TASK_MM, DRV_TASK_ID,
                                      sizeof(struct mm_start_req));
    if (!start_req_param)
        return -ENOMEM;

    /* Set parameters for the START message */
    memcpy(&start_req_param->phy_cfg, &aml_hw->phy.cfg, sizeof(aml_hw->phy.cfg));
    start_req_param->uapsd_timeout = (u32_l)aml_hw->mod_params->uapsd_timeout;
    start_req_param->lp_clk_accuracy = (u16_l)aml_hw->mod_params->lp_clk_ppm;
    start_req_param->tx_timeout[AC_BK] = (u16_l)aml_hw->mod_params->tx_to_bk;
    start_req_param->tx_timeout[AC_BE] = (u16_l)aml_hw->mod_params->tx_to_be;
    start_req_param->tx_timeout[AC_VI] = (u16_l)aml_hw->mod_params->tx_to_vi;
    start_req_param->tx_timeout[AC_VO] = (u16_l)aml_hw->mod_params->tx_to_vo;
    start_req_param->rx_hostbuf_size = (u16_l)aml_hw->ipc_env->rxbuf_sz;

    /* Send the START REQ message to LMAC FW */
    /* coverity[leaked_storage] - start_req_param will be freed later */
    return aml_send_msg(aml_hw, start_req_param, 1, MM_START_CFM, NULL);
}

int aml_send_version_req(struct aml_hw *aml_hw, struct mm_version_cfm *cfm)
{
    void *void_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* VERSION REQ has no parameter */
    void_param = aml_msg_zalloc(MM_VERSION_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

     /* coverity[leaked_storage] - void_param will free later */
    return aml_send_msg(aml_hw, void_param, 1, MM_VERSION_CFM, cfm);
}

int aml_send_add_if(struct aml_hw *aml_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm)
{
    struct mm_add_if_req *add_if_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ADD_IF_REQ message */
    add_if_req_param = aml_msg_zalloc(MM_ADD_IF_REQ, TASK_MM, DRV_TASK_ID,
                                       sizeof(struct mm_add_if_req));
    if (!add_if_req_param)
        return -ENOMEM;

    /* Set parameters for the ADD_IF_REQ message */
    memcpy(&(add_if_req_param->addr.array[0]), mac, ETH_ALEN);
    switch (iftype) {
    #ifdef CONFIG_AML_FULLMAC
    case NL80211_IFTYPE_P2P_CLIENT:
        add_if_req_param->p2p = true;
        // no break
    #endif /* CONFIG_AML_FULLMAC */
    case NL80211_IFTYPE_STATION:
        add_if_req_param->type = MM_STA;
        break;

    case NL80211_IFTYPE_ADHOC:
        add_if_req_param->type = MM_IBSS;
        break;

    #ifdef CONFIG_AML_FULLMAC
    case NL80211_IFTYPE_P2P_GO:
        add_if_req_param->p2p = true;
        // no break
    #endif /* CONFIG_AML_FULLMAC */
    case NL80211_IFTYPE_AP:
        add_if_req_param->type = MM_AP;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        add_if_req_param->type = MM_MESH_POINT;
        break;
    case NL80211_IFTYPE_AP_VLAN:
        aml_msg_free(aml_hw, add_if_req_param);
        /* coverity[leaked_storage] - add_if_req_param have already freed */
        return -1;
    case NL80211_IFTYPE_MONITOR:
        add_if_req_param->type = MM_MONITOR;
        break;
    default:
        add_if_req_param->type = MM_STA;
        break;
    }

    #ifdef CONFIG_AML_SOFTMAC
    add_if_req_param->p2p = p2p;
    #endif /* CONFIG_AML_SOFTMAC */

    /* Send the ADD_IF_REQ message to LMAC FW */
    /* coverity[leaked_storage] - add_if_req_param will be freed later */
    return aml_send_msg(aml_hw, add_if_req_param, 1, MM_ADD_IF_CFM, cfm);
}

int aml_send_remove_if(struct aml_hw *aml_hw, u8 vif_index)
{
    struct mm_remove_if_req *remove_if_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_REMOVE_IF_REQ message */
    remove_if_req = aml_msg_zalloc(MM_REMOVE_IF_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_remove_if_req));
    if (!remove_if_req)
        return -ENOMEM;

    /* Set parameters for the MM_REMOVE_IF_REQ message */
    remove_if_req->inst_nbr = vif_index;

    /* Send the MM_REMOVE_IF_REQ message to LMAC FW */
    /* coverity[leaked_storage] - remove_if_req will be freed later */
    return aml_send_msg(aml_hw, remove_if_req, 1, MM_REMOVE_IF_CFM, NULL);
}

int aml_send_set_channel(struct aml_hw *aml_hw, int phy_idx,
                          struct mm_set_channel_cfm *cfm)
{
#ifdef CONFIG_AML_SOFTMAC
    struct cfg80211_chan_def *chandef = &aml_hw->hw->conf.chandef;
#endif /* CONFIG_AML_SOFTMAC */
    struct mm_set_channel_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    if (phy_idx >= aml_hw->phy.cnt)
        return -ENOTSUPP;

    req = aml_msg_zalloc(MM_SET_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_channel_req));
    if (!req)
        return -ENOMEM;

    if (phy_idx == 0) {
#ifdef CONFIG_AML_SOFTMAC
        cfg80211_to_aml_chan(chandef, &req->chan);
#else
        /* On FULLMAC only setting channel of secondary chain */
        wiphy_err(aml_hw->wiphy, "Trying to set channel of primary chain");
        aml_msg_free(aml_hw, req);
        /* coverity[leaked_storage] - remove_if_req have already freed */
        return 0;
#endif /* CONFIG_AML_SOFTMAC */
    } else {
        req->chan = aml_hw->phy.sec_chan;
    }

    req->index = phy_idx;

    if (aml_hw->phy.limit_bw)
        limit_chan_bw(&req->chan.type, req->chan.prim20_freq, &req->chan.center1_freq);

    //AML_DBG("mac80211:   freq=%d(c1:%d - c2:%d)/width=%d - band=%d\n"
    //         "   hw(%d): prim20=%d(c1:%d - c2:%d)/ type=%d - band=%d\n",
    //         center_freq, center_freq1, center_freq2, width, band,
    //         phy_idx, req->chan.prim20_freq, req->chan.center1_freq,
    //         req->chan.center2_freq, req->chan.type, req->chan.band);

    /* Send the MM_SET_CHANNEL_REQ REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MM_SET_CHANNEL_CFM, cfm);
}


int aml_send_key_add(struct aml_hw *aml_hw, u8 vif_idx, u8 sta_idx, bool pairwise,
                      u8 *key, u8 key_len, u8 key_idx, u8 cipher_suite,
                      struct mm_key_add_cfm *cfm)
{
    struct mm_key_add_req *key_add_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_KEY_ADD_REQ message */
    key_add_req = aml_msg_zalloc(MM_KEY_ADD_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_add_req));
    if (!key_add_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_ADD_REQ message */
    key_add_req->sta_idx = sta_idx;
    key_add_req->key_idx = key_idx;
    key_add_req->pairwise = pairwise;
    key_add_req->inst_nbr = vif_idx;
    key_add_req->key.length = key_len;
    memcpy(&(key_add_req->key.array[0]), key, key_len);

    key_add_req->cipher_suite = cipher_suite;

    AML_DBG("%s: sta_idx:%d key_idx:%d inst_nbr:%d cipher:%d key_len:%d\n", __func__,
             key_add_req->sta_idx, key_add_req->key_idx, key_add_req->inst_nbr,
             key_add_req->cipher_suite, key_add_req->key.length);
#if defined(CONFIG_AML_DBG) || defined(CONFIG_DYNAMIC_DEBUG)
    print_hex_dump_bytes("key: ", DUMP_PREFIX_OFFSET, key_add_req->key.array, key_add_req->key.length);
#endif

    /* Send the MM_KEY_ADD_REQ message to LMAC FW */
    /* coverity[leaked_storage] - key_add_req will be freed later */
    return aml_send_msg(aml_hw, key_add_req, 1, MM_KEY_ADD_CFM, cfm);
}

int aml_send_key_del(struct aml_hw *aml_hw, uint8_t hw_key_idx)
{
    struct mm_key_del_req *key_del_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_KEY_DEL_REQ message */
    key_del_req = aml_msg_zalloc(MM_KEY_DEL_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_del_req));
    if (!key_del_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_DEL_REQ message */
    key_del_req->hw_key_idx = hw_key_idx;

    /* Send the MM_KEY_DEL_REQ message to LMAC FW */
    /* coverity[leaked_storage] - key_del_req will be freed later */
    return aml_send_msg(aml_hw, key_del_req, 1, MM_KEY_DEL_CFM, NULL);
}

int aml_send_bcn_change(struct aml_hw *aml_hw, u8 vif_idx, u32 bcn_addr,
                         u16 bcn_len, u16 tim_oft, u16 tim_len, u16 *csa_oft)
{
    struct mm_bcn_change_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_BCN_CHANGE_REQ message */
    req = aml_msg_zalloc(MM_BCN_CHANGE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_bcn_change_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_BCN_CHANGE_REQ message */
    req->bcn_ptr = bcn_addr;
    req->bcn_len = bcn_len;
    req->tim_oft = tim_oft;
    req->tim_len = tim_len;
    req->inst_nbr = vif_idx;

#if defined(CONFIG_AML_SOFTMAC)
    BUILD_BUG_ON_MSG(IEEE80211_MAX_CNTDWN_COUNTERS_NUM != BCN_MAX_CSA_CPT,
                     "BCN_MAX_CSA_CPT and IEEE80211_MAX_CNTDWN_COUNTERS_NUM "
                     "have different value");
#endif /* CONFIG_AML_SOFTMAC */
    if (csa_oft) {
#ifdef CONIG_AML_CSA_MODE
        req->csa_oft[0] = csa_oft[0] & 0xff;
        req->csa_oft[1] = (csa_oft[0] >> 8) & 0xff;
        AML_INFO("req->csa_oft[0]:%d, req->csa_oft[1]:%d", req->csa_oft[0], req->csa_oft[1]);
#else
        int i;
        for (i = 0; i < BCN_MAX_CSA_CPT; i++) {
            req->csa_oft[i] = csa_oft[i];
        }
#endif
    }

    /* Send the MM_BCN_CHANGE_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MM_BCN_CHANGE_CFM, NULL);
}

int aml_send_roc(struct aml_hw *aml_hw, struct aml_vif *vif,
                  struct ieee80211_channel *chan, unsigned  int duration)
{
    struct mm_remain_on_channel_req *req;
    struct cfg80211_chan_def chandef = {0};

    AML_INFO("op_code:%d,vif_index:%d,dur_ms:%d",MM_ROC_OP_START,vif->vif_index,duration);

    /* Create channel definition structure */
    cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);

    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = aml_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code      = MM_ROC_OP_START;
    req->vif_index    = vif->vif_index;
    req->duration_ms  = duration;
    cfg80211_to_aml_chan(&chandef, &req->chan);

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MM_REMAIN_ON_CHANNEL_CFM, NULL);
}

int aml_send_cancel_roc(struct aml_hw *aml_hw)
{
    struct mm_remain_on_channel_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    aml_tx_cfm_wait_rsp(aml_hw, false, __func__, __LINE__);
    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = aml_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code = MM_ROC_OP_CANCEL;

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_send_set_power(struct aml_hw *aml_hw, u8 vif_idx, s8 pwr,
                        struct mm_set_power_cfm *cfm)
{
    struct mm_set_power_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_POWER_REQ message */
    req = aml_msg_zalloc(MM_SET_POWER_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_power_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_POWER_REQ message */
    req->inst_nbr = vif_idx;
    req->power = pwr;

    /* Send the MM_SET_POWER_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MM_SET_POWER_CFM, cfm);
}

int aml_send_set_edca(struct aml_hw *aml_hw, u8 hw_queue, u32 param,
                       bool uapsd, u8 inst_nbr)
{
    struct mm_set_edca_req *set_edca_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_EDCA_REQ message */
    set_edca_req = aml_msg_zalloc(MM_SET_EDCA_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_set_edca_req));
    if (!set_edca_req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_EDCA_REQ message */
    set_edca_req->ac_param = param;
    set_edca_req->uapsd = uapsd;
    set_edca_req->hw_queue = hw_queue;
    set_edca_req->inst_nbr = inst_nbr;

    /* Send the MM_SET_EDCA_REQ message to LMAC FW */
    /* coverity[leaked_storage] - set_edca_req will be freed later */
    return aml_send_msg(aml_hw, set_edca_req, 1, MM_SET_EDCA_CFM, NULL);
}

#ifdef CONFIG_AML_P2P_DEBUGFS
int aml_send_p2p_oppps_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                            u8 ctw, struct mm_set_p2p_oppps_cfm *cfm)
{
    struct mm_set_p2p_oppps_req *p2p_oppps_req;
    int error;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_P2P_OPPPS_REQ message */
    p2p_oppps_req = aml_msg_zalloc(MM_SET_P2P_OPPPS_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_set_p2p_oppps_req));

    if (!p2p_oppps_req) {
        return -ENOMEM;
    }

    /* Fill the message parameters */
    p2p_oppps_req->vif_index = aml_vif->vif_index;
    p2p_oppps_req->ctwindow = ctw;

    /* Send the MM_P2P_OPPPS_REQ message to LMAC FW */
    error = aml_send_msg(aml_hw, p2p_oppps_req, 1, MM_SET_P2P_OPPPS_CFM, cfm);
    /* coverity[leaked_storage] - p2p_oppps_req will be freed later */
    return (error);
}

int aml_send_p2p_noa_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                          int count, int interval, int duration, bool dyn_noa,
                          struct mm_set_p2p_noa_cfm *cfm)
{
    struct mm_set_p2p_noa_req *p2p_noa_req;
    int error;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Param check */
    if (count > 255)
        count = 255;

    if (duration >= interval) {
        dev_err(aml_hw->dev, "Invalid p2p NOA config: interval=%d <= duration=%d\n",
                interval, duration);
        return -EINVAL;
    }

    /* Build the MM_SET_P2P_NOA_REQ message */
    p2p_noa_req = aml_msg_zalloc(MM_SET_P2P_NOA_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_set_p2p_noa_req));

    if (!p2p_noa_req) {
        return -ENOMEM;
    }

    /* Fill the message parameters */
    p2p_noa_req->vif_index = aml_vif->vif_index;
    p2p_noa_req->noa_inst_nb = 0;
    p2p_noa_req->count = count;

    if (count) {
        p2p_noa_req->duration_us = duration * 1024;
        p2p_noa_req->interval_us = interval * 1024;
        p2p_noa_req->start_offset = (interval - duration - 10) * 1024;
        p2p_noa_req->dyn_noa = dyn_noa;
    }

    /* Send the MM_SET_2P_NOA_REQ message to LMAC FW */
    error = aml_send_msg(aml_hw, p2p_noa_req, 1, MM_SET_P2P_NOA_CFM, cfm);
    /* coverity[leaked_storage] - p2p_noa_req will be freed later */

    return (error);
}
#endif /* CONFIG_AML_P2P_DEBUGFS */

/******************************************************************************
 *    Control messages handling functions (SOFTMAC only)
 *****************************************************************************/
#ifdef CONFIG_AML_SOFTMAC
int aml_send_sta_add(struct aml_hw *aml_hw, struct ieee80211_sta *sta,
                      u8 inst_nbr, struct mm_sta_add_cfm *cfm)
{
    struct mm_sta_add_req *sta_add_req;
    union aml_thd_phy_ctrl_info *phy;
    struct aml_sta *aml_sta;
    int error;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_STA_ADD_REQ message */
    sta_add_req = aml_msg_zalloc(MM_STA_ADD_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_sta_add_req));
    if (!sta_add_req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_ADD_REQ message */
    memcpy(&(sta_add_req->mac_addr.array[0]), &(sta->addr[0]), ETH_ALEN);

    if (sta->wme)
        sta_add_req->capa_flags |= STA_QOS_CAPA;

    /* TODO: check if a density of 0 microseconds is OK or not for LMAC */
    if (sta->ht_cap.ht_supported) {
        int ht_exp = sta->ht_cap.ampdu_factor;
        int vht_exp = 0;

        sta_add_req->capa_flags |= STA_HT_CAPA;
        sta_add_req->ampdu_size_max_ht =
            (1 << (IEEE80211_HT_MAX_AMPDU_FACTOR + ht_exp)) - 1;
        sta_add_req->ampdu_spacing_min =
            aml_ampdudensity2usec(sta->ht_cap.ampdu_density);

        if (sta->vht_cap.vht_supported) {
            sta_add_req->capa_flags |= STA_VHT_CAPA;
            vht_exp = (sta->vht_cap.cap &
                IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK) >>
                IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT;
            sta_add_req->ampdu_size_max_vht =
                (1 << (IEEE80211_HT_MAX_AMPDU_FACTOR + vht_exp)) - 1;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)  || (defined CONFIG_KERNEL_AX_PATCH)
        if (sta->he_cap.has_he) {
            int he_exp_ext = (sta->he_cap.he_cap_elem.mac_cap_info[3] &
                              IEEE80211_HE_MAC_CAP3_MAX_AMPDU_LEN_EXP_MASK) >> 3;

            sta_add_req->capa_flags |= STA_HE_CAPA;
            if (sta->vht_cap.vht_supported) {
                if ((vht_exp == 7) && he_exp_ext)
                    sta_add_req->ampdu_size_max_he =
                        (1 << (IEEE80211_HT_MAX_AMPDU_FACTOR + vht_exp + he_exp_ext)) - 1;
                else
                    sta_add_req->ampdu_size_max_he = sta_add_req->ampdu_size_max_vht;
            } else {
                if ((ht_exp == 3) && he_exp_ext)
                    sta_add_req->ampdu_size_max_he =
                        (1 << (IEEE80211_HT_MAX_AMPDU_FACTOR + ht_exp + he_exp_ext)) - 1;
                else
                    sta_add_req->ampdu_size_max_he = sta_add_req->ampdu_size_max_ht;
            }
        }
#endif
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
    if (sta->mfp)
        sta_add_req->capa_flags |= STA_MFP_CAPA;
#endif

    aml_sta = (struct aml_sta *)sta->drv_priv;
    /* TODO Set the interface index from the vif structure */
    sta_add_req->inst_nbr = inst_nbr;
    sta_add_req->tdls_sta = sta->tdls;
    sta_add_req->tdls_sta_initiator = STA_TDLS_INITIATOR(sta);
    sta_add_req->bssid_index = 0;
    sta_add_req->max_bssid_ind = 0;
    phy = (union aml_thd_phy_ctrl_info *)&sta_add_req->paid_gid;
    phy->partialAIDTx = aml_sta->paid;
    phy->groupIDTx = aml_sta->gid;

    /* Send the MM_STA_ADD_REQ message to LMAC FW */
    error = aml_send_msg(aml_hw, sta_add_req, 1, MM_STA_ADD_CFM, cfm);

    return (error);
}

int aml_send_sta_del(struct aml_hw *aml_hw, u8 sta_idx)
{
    struct mm_sta_del_req *sta_del_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_STA_DEL_REQ message */
    sta_del_req = aml_msg_zalloc(MM_STA_DEL_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_sta_del_req));
    if (!sta_del_req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_DEL_REQ message */
    sta_del_req->sta_idx = sta_idx;

    /* Send the MM_STA_DEL_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, sta_del_req, 1, MM_STA_DEL_CFM, NULL);
}

int aml_send_set_filter(struct aml_hw *aml_hw, uint32_t filter)
{
    struct mm_set_filter_req *set_filter_req_param;
    uint32_t rx_filter = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_FILTER_REQ message */
    set_filter_req_param =
        aml_msg_zalloc(MM_SET_FILTER_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_filter_req));
    if (!set_filter_req_param)
        return -ENOMEM;

    /* Set parameters for the MM_SET_FILTER_REQ message */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
    if (filter & FIF_PROMISC_IN_BSS)
        rx_filter |= NXMAC_ACCEPT_UNICAST_BIT;
#endif
    if (filter & FIF_ALLMULTI)
        rx_filter |= NXMAC_ACCEPT_MULTICAST_BIT;

    if (filter & (FIF_FCSFAIL | FIF_PLCPFAIL))
        rx_filter |= NXMAC_ACCEPT_ERROR_FRAMES_BIT;

    if (filter & FIF_BCN_PRBRESP_PROMISC)
        rx_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;

    if (filter & FIF_CONTROL)
        rx_filter |= NXMAC_ACCEPT_OTHER_CNTRL_FRAMES_BIT |
                     NXMAC_ACCEPT_CF_END_BIT |
                     NXMAC_ACCEPT_ACK_BIT |
                     NXMAC_ACCEPT_CTS_BIT |
                     NXMAC_ACCEPT_RTS_BIT |
                     NXMAC_ACCEPT_BA_BIT | NXMAC_ACCEPT_BAR_BIT;

    if (filter & FIF_OTHER_BSS)
        rx_filter |= NXMAC_ACCEPT_OTHER_BSSID_BIT;

    if (filter & FIF_PSPOLL) {
        /* TODO: check if the MAC filters apply to our BSSID or is general */
        rx_filter |= NXMAC_ACCEPT_PS_POLL_BIT;
    }

    if (filter & FIF_PROBE_REQ) {
        rx_filter |= NXMAC_ACCEPT_PROBE_REQ_BIT;
        rx_filter |= NXMAC_ACCEPT_ALL_BEACON_BIT;
    }

    /* Add the filter flags that are set by default and cannot be changed here */
    rx_filter |= AML_MAC80211_NOT_CHANGEABLE;

    /* XXX */
    if (ieee80211_hw_check(aml_hw->hw, AMPDU_AGGREGATION))
        rx_filter |= NXMAC_ACCEPT_BA_BIT;

    /* Now copy all the flags into the message parameter */
    set_filter_req_param->filter = rx_filter;

    AML_DBG("new total_flags = 0x%08x\nrx filter set to  0x%08x\n",
             filter, rx_filter);

    /* Send the MM_SET_FILTER_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_filter_req_param, 1, MM_SET_FILTER_CFM, NULL);
}


int aml_send_add_chanctx(struct aml_hw *aml_hw,
                          struct ieee80211_chanctx_conf *ctx,
                          struct mm_chan_ctxt_add_cfm *cfm)
{
    struct mm_chan_ctxt_add_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_ADD_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_ADD_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_chan_ctxt_add_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the CHAN_CTXT_ADD_REQ message */
    cfg80211_to_aml_chan(&ctx->def, &req->chan);

    if (aml_hw->phy.limit_bw)
        limit_chan_bw(&req->chan.type, req->chan.prim20_freq,
                      &req->chan.center1_freq);

    AML_DBG("mac80211:   freq=%d(c1:%d - c2:%d)/width=%d - band=%d\n"
             "          prim20=%d(c1:%d - c2:%d)/ type=%d - band=%d\n",
             ctx->def.chan->center_freq, ctx->def.center_freq1,
             ctx->def.center_freq2, ctx->def.width, ctx->def.chan->band,
             req->chan.prim20_freq, req->chan.center1_freq,
             req->chan.center2_freq, req->chan.type, req->chan.band);
    /* Send the CHAN_CTXT_ADD_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_ADD_CFM, cfm);
}

int aml_send_del_chanctx(struct aml_hw *aml_hw, u8 index)
{
    struct mm_chan_ctxt_del_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_DEL_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_DEL_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_chan_ctxt_del_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMOVE_IF_REQ message */
    req->index = index;

    /* Send the MM_CHAN_CTXT_DEL_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_DEL_CFM, NULL);
}

int aml_send_link_chanctx(struct aml_hw *aml_hw, u8 vif_idx, u8 chan_idx,
                           u8 chan_switch)
{
    struct mm_chan_ctxt_link_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_LINK_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_LINK_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_chan_ctxt_link_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CHAN_CTXT_LINK_REQ message */
    req->vif_index = vif_idx;
    req->chan_index = chan_idx;
    req->chan_switch = chan_switch;

    /* Send the MM_CHAN_CTXT_LINK_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_LINK_CFM, NULL);
}

int aml_send_unlink_chanctx(struct aml_hw *aml_hw, u8 vif_idx)
{
    struct mm_chan_ctxt_unlink_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_UNLINK_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_UNLINK_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_chan_ctxt_unlink_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CHAN_CTXT_UNLINK_REQ message */
    req->vif_index = vif_idx;

    /* Send the MM_CHAN_CTXT_UNLINK_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_UNLINK_CFM, NULL);
}

int aml_send_update_chanctx(struct aml_hw *aml_hw,
                             struct ieee80211_chanctx_conf *ctx)
{
    struct mm_chan_ctxt_update_req *req;
    struct aml_chanctx *aml_chanctx;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_UPDATE_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_UPDATE_REQ, TASK_MM, DRV_TASK_ID,
                                       sizeof(struct mm_chan_ctxt_update_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CHAN_CTXT_UPDATE_REQ message */
    aml_chanctx = (struct aml_chanctx *)ctx->drv_priv;
    req->chan_index = aml_chanctx->index;
    cfg80211_to_aml_chan(&ctx->def, &req->chan);

    if (aml_hw->phy.limit_bw)
        limit_chan_bw(&req->chan.type, req->chan.prim20_freq, &req->chan.center1_freq);

    AML_DBG("mac80211:   freq=%d(c1:%d - c2:%d)/width=%d - band=%d\n"
             "          prim20=%d(c1:%d - c2:%d)/ type=%d - band=%d\n",
             ctx->def.chan->center_freq, ctx->def.center_freq1,
             ctx->def.center_freq2, ctx->def.width, ctx->def.chan->band,
             req->chan.prim20_freq, req->chan.center1_freq,
             req->chan.center2_freq, req->chan.type, req->chan.band);
    /* Send the MM_CHAN_CTXT_UPDATE_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_UPDATE_CFM, NULL);
}

int aml_send_sched_chanctx(struct aml_hw *aml_hw, u8 vif_idx, u8 chan_idx, u8 type)
{
    struct mm_chan_ctxt_sched_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CHAN_CTXT_SCHED_REQ message */
    req = aml_msg_zalloc(MM_CHAN_CTXT_SCHED_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_chan_ctxt_sched_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CHAN_CTXT_SCHED_REQ message */
    req->vif_index = vif_idx;
    req->chan_index = chan_idx;
    req->type = type;

    /* Send the MM_CHAN_CTXT_SCHED_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_CHAN_CTXT_SCHED_CFM, NULL);
}

int aml_send_dtim_req(struct aml_hw *aml_hw, u8 dtim_period)
{
    struct mm_set_dtim_req *set_dtim_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_DTIM_REQ message */
    set_dtim_req = aml_msg_zalloc(MM_SET_DTIM_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_set_dtim_req));
    if (!set_dtim_req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_DTIM_REQ message */
    set_dtim_req->dtim_period = dtim_period;

    /* Send the MM_SET_DTIM_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_dtim_req, 1, MM_SET_DTIM_CFM, NULL);
}

int aml_send_set_br(struct aml_hw *aml_hw, u32 basic_rates, u8 vif_idx, u8 band)
{
    struct mm_set_basic_rates_req *set_basic_rates_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_BASIC_RATES_REQ message */
    set_basic_rates_req_param =
        aml_msg_zalloc(MM_SET_BASIC_RATES_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_basic_rates_req));
    if (!set_basic_rates_req_param)
        return -ENOMEM;

    /* Set parameters for the MM_SET_BASIC_RATES_REQ message */
    set_basic_rates_req_param->rates = basic_rates;
    set_basic_rates_req_param->inst_nbr = vif_idx;
    set_basic_rates_req_param->band = band;

    /* Send the MM_SET_BASIC_RATES_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_basic_rates_req_param, 1, MM_SET_BASIC_RATES_CFM, NULL);
}

int aml_send_set_beacon_int(struct aml_hw *aml_hw, u16 beacon_int, u8 vif_idx)
{
    struct mm_set_beacon_int_req *set_beacon_int_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_BEACON_INT_REQ message */
    set_beacon_int_req_param =
        aml_msg_zalloc(MM_SET_BEACON_INT_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_beacon_int_req));
    if (!set_beacon_int_req_param)
        return -ENOMEM;

    /* Set parameters for the MM_SET_BEACON_INT_REQ message */
    set_beacon_int_req_param->beacon_int = beacon_int;
    set_beacon_int_req_param->inst_nbr = vif_idx;

    /* Send the MM_SET_BEACON_INT_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_beacon_int_req_param, 1,
                         MM_SET_BEACON_INT_CFM, NULL);
}

int aml_send_set_bssid(struct aml_hw *aml_hw, const u8 *bssid, u8 vif_idx)
{
    struct mm_set_bssid_req *set_bssid_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_BSSID_REQ message */
    set_bssid_req_param = aml_msg_zalloc(MM_SET_BSSID_REQ, TASK_MM, DRV_TASK_ID,
                                          sizeof(struct mm_set_bssid_req));
    if (!set_bssid_req_param)
        return -ENOMEM;

    /* Set parameters for the MM_SET_BSSID_REQ message */
    memcpy(&(set_bssid_req_param->bssid.array[0]), bssid, ETH_ALEN);
    set_bssid_req_param->inst_nbr = vif_idx;

    /* Send the MM_SET_BSSID_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_bssid_req_param, 1, MM_SET_BSSID_CFM, NULL);
}

int aml_send_set_vif_state(struct aml_hw *aml_hw, bool active,
                            u16 aid, u8 vif_idx)
{
    struct mm_set_vif_state_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_VIF_STATE_REQ message */
    req = aml_msg_zalloc(MM_SET_VIF_STATE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_vif_state_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_VIF_STATE_REQ message */
    req->active = active;
    req->aid = aid;
    req->inst_nbr = vif_idx;

    /* Send the MM_SET_VIF_STATE_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_SET_VIF_STATE_CFM, NULL);
}

int aml_send_set_mode(struct aml_hw *aml_hw, u8 abgnmode)
{
    struct mm_set_mode_req *set_mode_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_MODE_REQ message */
    set_mode_req_param = aml_msg_zalloc(MM_SET_MODE_REQ, TASK_MM, DRV_TASK_ID,
                                         sizeof(struct mm_set_mode_req));
    if (!set_mode_req_param)
        return -ENOMEM;

    set_mode_req_param->abgnmode = abgnmode;

    /* Send the MM_SET_MODE_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_mode_req_param, 1, MM_SET_MODE_CFM, NULL);
}

int aml_send_set_idle(struct aml_hw *aml_hw, int idle)
{
    struct mm_set_idle_req *set_idle_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    set_idle_req_param = aml_msg_zalloc(MM_SET_IDLE_REQ, TASK_MM, DRV_TASK_ID,
                                         sizeof(struct mm_set_idle_req));
    if (!set_idle_req_param)
        return -ENOMEM;

    set_idle_req_param->hw_idle = idle;

    return aml_send_msg(aml_hw, set_idle_req_param, 1, MM_SET_IDLE_CFM, NULL);
}

int aml_send_set_ps_mode(struct aml_hw *aml_hw, u8 ps_mode)
{
    struct mm_set_ps_mode_req *set_ps_mode_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    set_ps_mode_req_param =
        aml_msg_zalloc(MM_SET_PS_MODE_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_ps_mode_req));
    if (!set_ps_mode_req_param)
        return -ENOMEM;

    set_ps_mode_req_param->new_state = ps_mode;

    return aml_send_msg(aml_hw, set_ps_mode_req_param, 1, MM_SET_PS_MODE_CFM, NULL);
}

int aml_send_set_ps_options(struct aml_hw *aml_hw, bool listen_bcmc,
                             u16 listen_interval, u8 vif_idx)
{
    struct mm_set_ps_options_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_PS_OPTIONS_REQ message */
    req = aml_msg_zalloc(MM_SET_PS_OPTIONS_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_ps_options_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_VIF_STATE_REQ message */
    req->listen_interval = listen_interval;
    req->dont_listen_bc_mc = !listen_bcmc;
    req->vif_index = vif_idx;

    /* Send the MM_SET_PS_OPTIONS_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_SET_PS_OPTIONS_CFM, NULL);
}

int aml_send_set_slottime(struct aml_hw *aml_hw, int use_short_slot)
{
    struct mm_set_slottime_req *set_slottime_req_param;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SET_SLOTTIME_REQ message */
    set_slottime_req_param =
        aml_msg_zalloc(MM_SET_SLOTTIME_REQ, TASK_MM, DRV_TASK_ID,
                        sizeof(struct mm_set_slottime_req));
    if (!set_slottime_req_param)
        return -ENOMEM;

    /* Set parameters for the MM_SET_SLOTTIME_REQ message */
    set_slottime_req_param->slottime = use_short_slot ? 9 : 20;

    /* Send the MM_SET_SLOTTIME_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, set_slottime_req_param, 1, MM_SET_SLOTTIME_CFM, NULL);
}

int aml_send_ba_add(struct aml_hw *aml_hw, uint8_t type, uint8_t sta_idx,
                     u16 tid, uint8_t bufsz, uint16_t ssn,
                     struct mm_ba_add_cfm *cfm)
{
    struct mm_ba_add_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_BA_ADD_REQ message */
    req = aml_msg_zalloc(MM_BA_ADD_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_ba_add_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_BA_ADD_REQ message */
    req->type = type;
    req->sta_idx = sta_idx;
    req->tid = (u8_l)tid;
    req->bufsz = bufsz;
    req->ssn = ssn;

    /* Send the MM_BA_ADD_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_BA_ADD_CFM, cfm);
}

int aml_send_ba_del(struct aml_hw *aml_hw, uint8_t sta_idx, u16 tid,
                     struct mm_ba_del_cfm *cfm)
{
    struct mm_ba_del_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_BA_DEL_REQ message */
    req = aml_msg_zalloc(MM_BA_DEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_ba_del_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters */
    req->type = 0;
    req->sta_idx = sta_idx;
    req->tid = (u8_l)tid;

    /* Send the MM_BA_DEL_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_BA_DEL_CFM, cfm);
}

int aml_send_scan_req(struct aml_hw *aml_hw, struct ieee80211_vif *vif,
                       struct cfg80211_scan_request *param,
                       struct scan_start_cfm *cfm)
{
    struct scan_start_req *req;
    int i;
    struct aml_vif *aml_vif;
    uint8_t chan_flags = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    aml_vif = (struct aml_vif *)vif->drv_priv;

    /* Build the SCAN_START_REQ message */
    req = aml_msg_zalloc(SCAN_START_REQ, TASK_SCAN, DRV_TASK_ID,
                          sizeof(struct scan_start_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters */
    req->vif_idx = aml_vif->vif_index;
    req->chan_cnt = (u8)min_t(int, SCAN_CHANNEL_MAX, param->n_channels);
    req->ssid_cnt = (u8)min_t(int, SCAN_SSID_MAX, param->n_ssids);
    req->bssid = mac_addr_bcst;
    req->no_cck = param->no_cck;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    if (param->duration_mandatory)
        req->duration = ieee80211_tu_to_usec(param->duration);
#endif

    if (req->ssid_cnt == 0)
        chan_flags |= CHAN_NO_IR;
    for (i = 0; i < req->ssid_cnt; i++) {
        int j;
        for (j = 0; j < param->ssids[i].ssid_len; j++)
            req->ssid[i].array[j] = param->ssids[i].ssid[j];
        req->ssid[i].length = param->ssids[i].ssid_len;
    }

    if (param->ie) {
        if (aml_ipc_buf_a2e_alloc(aml_hw, &aml_hw->scan_ie,
                                   param->ie_len, param->ie)) {
            dev_err(aml_hw->dev, "Failed to allocate IPC buffer for Scan IEs\n");
            goto error;
        }

        req->add_ie_len = param->ie_len;
        req->add_ies = aml_hw->scan_ie.dma_addr;
    } else {
        req->add_ie_len = 0;
        req->add_ies = 0;
    }

    for (i = 0; i < req->chan_cnt; i++) {
        struct ieee80211_channel *chan = param->channels[i];

        req->chan[i].band = chan->band;
        req->chan[i].freq = chan->center_freq;
        req->chan[i].flags = chan_flags | get_chan_flags(chan->flags);
        req->chan[i].tx_power = chan_to_fw_pwr(chan->max_reg_power);
    }

    /* Send the SCAN_START_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, SCAN_START_CFM, cfm);
error:
    if (req != NULL)
        aml_msg_free(aml_hw, req);
    return -ENOMEM;
}

int aml_send_scan_cancel_req(struct aml_hw *aml_hw,
                              struct scan_cancel_cfm *cfm)
{
    struct scan_cancel_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the SCAN_CANCEL_REQ message */
    req = aml_msg_zalloc(SCAN_CANCEL_REQ, TASK_SCAN, DRV_TASK_ID,
                          sizeof(struct scan_cancel_req));
    if (!req)
        return -ENOMEM;

    /* Send the SCAN_CANCEL_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, SCAN_CANCEL_CFM, cfm);
}

void aml_send_tdls_ps(struct aml_hw *aml_hw, bool ps_mode)
{
    if (!aml_hw->mod_params->ps_on)
        return;

    aml_send_set_ps_mode(aml_hw, ps_mode);
}

int aml_send_tim_update(struct aml_hw *aml_hw, u8 vif_idx, u16 aid,
                         u8 tx_status)
{
    struct mm_tim_update_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_TIM_UPDATE_REQ message */
    req = aml_msg_zalloc(MM_TIM_UPDATE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_tim_update_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_TIM_UPDATE_REQ message */
    req->aid = aid;
    req->tx_avail = tx_status;
    req->inst_nbr = vif_idx;

    /* Send the MM_TIM_UPDATE_REQ message to LMAC FW */
    return aml_send_msg(aml_hw, req, 1, MM_TIM_UPDATE_CFM, NULL);
}
#endif /* CONFIG_AML_SOFTMAC */

/******************************************************************************
 *    Control messages handling functions (FULLMAC only)
 *****************************************************************************/
#ifdef CONFIG_AML_FULLMAC
int aml_send_me_config_req(struct aml_hw *aml_hw)
{
    struct me_config_req *req;
    struct wiphy *wiphy = aml_hw->wiphy;
    struct ieee80211_sta_ht_cap *ht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->ht_cap;
    struct ieee80211_sta_vht_cap *vht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->vht_cap;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    struct ieee80211_sta_he_cap const *he_cap;
#endif
    uint8_t *ht_mcs = (uint8_t *)&ht_cap->mcs;
    int i;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_CONFIG_REQ message */
    req = aml_msg_zalloc(ME_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_config_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_CONFIG_REQ message */
    req->ht_supp = ht_cap->ht_supported;
    req->vht_supp = vht_cap->vht_supported;
    req->ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap);
    req->ht_cap.a_mpdu_param = ht_cap->ampdu_factor |
                                     (ht_cap->ampdu_density <<
                                         IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
    for (i = 0; i < sizeof(ht_cap->mcs); i++)
        req->ht_cap.mcs_rate[i] = ht_mcs[i];
    req->ht_cap.ht_extended_capa = 0;
    req->ht_cap.tx_beamforming_capa = 0;
    req->ht_cap.asel_capa = 0;

    req->vht_cap.vht_capa_info = cpu_to_le32(vht_cap->cap);
    req->vht_cap.rx_highest = cpu_to_le16(vht_cap->vht_mcs.rx_highest);
    req->vht_cap.rx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.rx_mcs_map);
    req->vht_cap.tx_highest = cpu_to_le16(vht_cap->vht_mcs.tx_highest);
    req->vht_cap.tx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.tx_mcs_map);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    if (wiphy->bands[NL80211_BAND_5GHZ]->iftype_data != NULL) {
        he_cap = &wiphy->bands[NL80211_BAND_5GHZ]->iftype_data->he_cap;

        req->he_supp = he_cap->has_he;
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.mac_cap_info); i++) {
            req->he_cap.mac_cap_info[i] = he_cap->he_cap_elem.mac_cap_info[i];
        }
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.phy_cap_info); i++) {
            req->he_cap.phy_cap_info[i] = he_cap->he_cap_elem.phy_cap_info[i];
        }
        req->he_cap.mcs_supp.rx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80);
        req->he_cap.mcs_supp.tx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80);
        req->he_cap.mcs_supp.rx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_160);
        req->he_cap.mcs_supp.tx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_160);
        req->he_cap.mcs_supp.rx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80p80);
        req->he_cap.mcs_supp.tx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80p80);
        for (i = 0; i < MAC_HE_PPE_THRES_MAX_LEN; i++) {
            req->he_cap.ppe_thres[i] = he_cap->ppe_thres[i];
        }
        req->he_ul_on = aml_hw->mod_params->he_ul_on;
    }
#else
    req->he_supp = false;
    req->he_ul_on = false;
#endif
    req->ps_on = aml_hw->mod_params->ps_on;
    req->dpsm = aml_hw->mod_params->dpsm;
    req->tx_lft = aml_hw->mod_params->tx_lft;
    req->amsdu_tx = aml_hw->mod_params->amsdu_tx;
    req->ant_div_on = aml_hw->mod_params->ant_div;
    if (aml_hw->mod_params->use_80)
        req->phy_bw_max = PHY_CHNL_BW_80;
    else if (aml_hw->mod_params->use_2040)
        req->phy_bw_max = PHY_CHNL_BW_40;
    else
        req->phy_bw_max = PHY_CHNL_BW_20;

    wiphy_info(wiphy, "HT supp %d, VHT supp %d, HE supp %d\n", req->ht_supp,
                                                               req->vht_supp,
                                                               req->he_supp);

    /* Send the ME_CONFIG_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_CONFIG_CFM, NULL);
}

int aml_send_me_chan_config_req(struct aml_hw *aml_hw)
{
    struct me_chan_config_req *req;
    struct wiphy *wiphy = aml_hw->wiphy;
    int i;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_CHAN_CONFIG_REQ message */
    req = aml_msg_zalloc(ME_CHAN_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                            sizeof(struct me_chan_config_req));
    if (!req)
        return -ENOMEM;

    req->chan2G4_cnt=  0;
    if (wiphy->bands[NL80211_BAND_2GHZ] != NULL) {
        struct ieee80211_supported_band *b = wiphy->bands[NL80211_BAND_2GHZ];
        for (i = 0; i < b->n_channels; i++) {
            req->chan2G4[req->chan2G4_cnt].flags = 0;
            if (b->channels[i].flags & IEEE80211_CHAN_DISABLED)
                req->chan2G4[req->chan2G4_cnt].flags |= CHAN_DISABLED;
            req->chan2G4[req->chan2G4_cnt].flags |= get_chan_flags(b->channels[i].flags);
            req->chan2G4[req->chan2G4_cnt].band = NL80211_BAND_2GHZ;
            req->chan2G4[req->chan2G4_cnt].freq = b->channels[i].center_freq;
            req->chan2G4[req->chan2G4_cnt].tx_power = chan_to_fw_pwr(b->channels[i].max_power);
            req->chan2G4_cnt++;
            if (req->chan2G4_cnt == MAC_DOMAINCHANNEL_24G_MAX)
                break;
        }
    }

    req->chan5G_cnt = 0;
    if (wiphy->bands[NL80211_BAND_5GHZ] != NULL) {
        struct ieee80211_supported_band *b = wiphy->bands[NL80211_BAND_5GHZ];
        for (i = 0; i < b->n_channels; i++) {
            req->chan5G[req->chan5G_cnt].flags = 0;
            if (b->channels[i].flags & IEEE80211_CHAN_DISABLED)
                req->chan5G[req->chan5G_cnt].flags |= CHAN_DISABLED;
            req->chan5G[req->chan5G_cnt].flags |= get_chan_flags(b->channels[i].flags);
            req->chan5G[req->chan5G_cnt].band = NL80211_BAND_5GHZ;
            req->chan5G[req->chan5G_cnt].freq = b->channels[i].center_freq;
            req->chan5G[req->chan5G_cnt].tx_power = chan_to_fw_pwr(b->channels[i].max_power);
            req->chan5G_cnt++;
            if (req->chan5G_cnt == MAC_DOMAINCHANNEL_5G_MAX)
                break;
        }
    }

    /* Send the ME_CHAN_CONFIG_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_CHAN_CONFIG_CFM, NULL);
}

int aml_send_me_set_control_port_req(struct aml_hw *aml_hw, bool opened, u8 sta_idx)
{
    struct me_set_control_port_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_SET_CONTROL_PORT_REQ message */
    req = aml_msg_zalloc(ME_SET_CONTROL_PORT_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_set_control_port_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_SET_CONTROL_PORT_REQ message */
    req->sta_idx = sta_idx;
    req->control_port_open = opened;

    /* Send the ME_SET_CONTROL_PORT_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_SET_CONTROL_PORT_CFM, NULL);
}

int aml_send_me_sta_add(struct aml_hw *aml_hw, struct station_parameters *params,
                         const u8 *mac, u8 inst_nbr, struct me_sta_add_cfm *cfm)
{
    struct me_sta_add_req *req;
    u8 *ht_mcs = (u8 *)&(P_LINK_STA_PARAMS(params, ht_capa->mcs));
    int i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0) || (defined CONFIG_KERNEL_AX_PATCH)
    AML_INFO("sta params has ht=%d, vht=%d he=%d capabilities",
            P_LINK_STA_PARAMS(params, ht_capa) ? 1 : 0, P_LINK_STA_PARAMS(params, vht_capa) ? 1 : 0
            ,P_LINK_STA_PARAMS(params, he_capa) ? 1 : 0);
#else
    AML_INFO("sta params has ht=%d, vht=%d he capabilities",
            P_LINK_STA_PARAMS(params, ht_capa) ? 1 : 0, P_LINK_STA_PARAMS(params, vht_capa) ? 1 : 0
            );
#endif

    /* Build the MM_STA_ADD_REQ message */
    req = aml_msg_zalloc(ME_STA_ADD_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_sta_add_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_ADD_REQ message */
    memcpy(&(req->mac_addr.array[0]), mac, ETH_ALEN);

    req->rate_set.length = P_LINK_STA_PARAMS(params, supported_rates_len);
    for (i = 0; i < P_LINK_STA_PARAMS(params, supported_rates_len); i++)
        req->rate_set.array[i] = P_LINK_STA_PARAMS(params, supported_rates)[i];

    req->flags = 0;

    if (params->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
        req->flags |= STA_SHORT_PREAMBLE_CAPA;

    if (P_LINK_STA_PARAMS(params, ht_capa)) {
        const struct ieee80211_ht_cap *ht_capa = P_LINK_STA_PARAMS(params, ht_capa);
        req->flags |= STA_HT_CAPA;
        req->ht_cap.ht_capa_info = cpu_to_le16(ht_capa->cap_info);
        if (!memcmp(mac, aml_hw->rx_assoc_info.addr, ETH_ALEN)) {
            req->ht_cap.ht_capa_info &= ~(IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40);
            req->ht_cap.ht_capa_info |= (aml_hw->mod_params->sgi
                && (aml_hw->rx_assoc_info.htcap & IEEE80211_HT_CAP_SGI_20)) ? IEEE80211_HT_CAP_SGI_20 : 0;
            req->ht_cap.ht_capa_info |= (aml_hw->mod_params->use_2040
                && (aml_hw->rx_assoc_info.htcap & IEEE80211_HT_CAP_SGI_40)) ? IEEE80211_HT_CAP_SGI_40 : 0;
        }
        req->ht_cap.a_mpdu_param = ht_capa->ampdu_params_info;
        for (i = 0; i < sizeof(ht_capa->mcs); i++)
            req->ht_cap.mcs_rate[i] = ht_mcs[i];
        req->ht_cap.ht_extended_capa = cpu_to_le16(ht_capa->extended_ht_cap_info);
        req->ht_cap.tx_beamforming_capa = cpu_to_le32(ht_capa->tx_BF_cap_info);
        req->ht_cap.asel_capa = ht_capa->antenna_selection_info;
    }

    if (P_LINK_STA_PARAMS(params, vht_capa)) {
        const struct ieee80211_vht_cap *vht_capa = P_LINK_STA_PARAMS(params, vht_capa);

        req->flags |= STA_VHT_CAPA;
        req->vht_cap.vht_capa_info = cpu_to_le32(vht_capa->vht_cap_info);
        req->vht_cap.rx_highest = cpu_to_le16(vht_capa->supp_mcs.rx_highest);
        req->vht_cap.rx_mcs_map = cpu_to_le16(vht_capa->supp_mcs.rx_mcs_map);
        req->vht_cap.tx_highest = cpu_to_le16(vht_capa->supp_mcs.tx_highest);
        req->vht_cap.tx_mcs_map = cpu_to_le16(vht_capa->supp_mcs.tx_mcs_map);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    if (P_LINK_STA_PARAMS(params, he_capa)) {
        const struct ieee80211_he_cap_elem *he_capa = P_LINK_STA_PARAMS(params, he_capa);
        struct ieee80211_he_mcs_nss_supp *mcs_nss_supp =
                                (struct ieee80211_he_mcs_nss_supp *)(he_capa + 1);

        req->flags |= STA_HE_CAPA;
        for (i = 0; i < ARRAY_SIZE(he_capa->mac_cap_info); i++) {
            req->he_cap.mac_cap_info[i] = he_capa->mac_cap_info[i];
        }
        for (i = 0; i < ARRAY_SIZE(he_capa->phy_cap_info); i++) {
            req->he_cap.phy_cap_info[i] = he_capa->phy_cap_info[i];
        }
        req->he_cap.mcs_supp.rx_mcs_80 = mcs_nss_supp->rx_mcs_80;
        req->he_cap.mcs_supp.tx_mcs_80 = mcs_nss_supp->tx_mcs_80;
        req->he_cap.mcs_supp.rx_mcs_160 = mcs_nss_supp->rx_mcs_160;
        req->he_cap.mcs_supp.tx_mcs_160 = mcs_nss_supp->tx_mcs_160;
        req->he_cap.mcs_supp.rx_mcs_80p80 = mcs_nss_supp->rx_mcs_80p80;
        req->he_cap.mcs_supp.tx_mcs_80p80 = mcs_nss_supp->tx_mcs_80p80;
    }
#endif

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME))
        req->flags |= STA_QOS_CAPA;

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_MFP))
        req->flags |= STA_MFP_CAPA;

    if (P_LINK_STA_PARAMS(params, opmode_notif_used)) {
        req->flags |= STA_OPMOD_NOTIF;
        req->opmode = P_LINK_STA_PARAMS(params, opmode_notif);
    }

    req->aid = cpu_to_le16(params->aid);
    req->uapsd_queues = params->uapsd_queues;
    req->max_sp_len = params->max_sp * 2;
    req->vif_idx = inst_nbr;

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER)) {
        struct aml_vif *aml_vif = aml_hw->vif_table[inst_nbr];
        req->tdls_sta = true;
        if ((params->ext_capab[3] & WLAN_EXT_CAPA4_TDLS_CHAN_SWITCH) &&
            !aml_vif->tdls_chsw_prohibited)
            req->tdls_chsw_allowed = true;
        if (aml_vif->tdls_status == TDLS_SETUP_RSP_TX)
            req->tdls_sta_initiator = true;
    }

    /* Send the ME_STA_ADD_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_STA_ADD_CFM, cfm);
}

int aml_send_me_sta_del(struct aml_hw *aml_hw, u8 sta_idx, bool tdls_sta)
{
    struct me_sta_del_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_STA_DEL_REQ message */
    req = aml_msg_zalloc(ME_STA_DEL_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_sta_del_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_DEL_REQ message */
    req->sta_idx = sta_idx;
    req->tdls_sta = tdls_sta;

    /* Send the ME_STA_DEL_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_STA_DEL_CFM, NULL);
}

int aml_send_me_traffic_ind(struct aml_hw *aml_hw, u8 sta_idx, bool uapsd, u8 tx_status)
{
    struct me_traffic_ind_req *req;

    AML_INFO("sta_idx:%d \n",sta_idx);

    /* Build the ME_UTRAFFIC_IND_REQ message */
    req = aml_msg_zalloc(ME_TRAFFIC_IND_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_traffic_ind_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_TRAFFIC_IND_REQ message */
    req->sta_idx = sta_idx;
    req->tx_avail = tx_status;
    req->uapsd = uapsd;

    /* Send the ME_TRAFFIC_IND_REQ to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_TRAFFIC_IND_CFM, NULL);
}

int aml_send_twt_request(struct aml_hw *aml_hw,
                          u8 setup_type, u8 vif_idx,
                          struct twt_conf_tag *conf,
                          struct twt_setup_cfm *cfm)
{
    struct twt_setup_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the TWT_SETUP_REQ message */
    req = aml_msg_zalloc(TWT_SETUP_REQ, TASK_TWT, DRV_TASK_ID,
                          sizeof(struct twt_setup_req));
    if (!req)
        return -ENOMEM;

    memcpy(&req->conf, conf, sizeof(req->conf));
    req->setup_type = setup_type;
    req->vif_idx = vif_idx;

    /* Send the TWT_SETUP_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, TWT_SETUP_CFM, cfm);
}

int _aml_send_twt_teardown(struct aml_hw *aml_hw,
                           struct twt_teardown_req *twt_teardown,
                           struct twt_teardown_cfm *cfm)
{
    struct twt_teardown_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the TWT_TEARDOWN_REQ message */
    req = aml_msg_zalloc(TWT_TEARDOWN_REQ, TASK_TWT, DRV_TASK_ID,
                          sizeof(struct twt_teardown_req));
    if (!req)
        return -ENOMEM;

    memcpy(req, twt_teardown, sizeof(struct twt_teardown_req));

    /* Send the TWT_TEARDOWN_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, TWT_TEARDOWN_CFM, cfm);
}

int aml_send_me_rc_stats(struct aml_hw *aml_hw,
                          u8 sta_idx,
                          struct me_rc_stats_cfm *cfm)
{
    struct me_rc_stats_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_RC_STATS_REQ message */
    req = aml_msg_zalloc(ME_RC_STATS_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_rc_stats_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_STATS_REQ message */
    req->sta_idx = sta_idx;

    /* Send the ME_RC_STATS_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_RC_STATS_CFM, cfm);
}

int aml_send_me_rc_set_rate(struct aml_hw *aml_hw,
                             u8 sta_idx,
                             u16 rate_cfg)
{
    struct me_rc_set_rate_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_RC_SET_RATE_REQ message */
    req = aml_msg_zalloc(ME_RC_SET_RATE_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_rc_set_rate_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_SET_RATE_REQ message */
    req->sta_idx = sta_idx;
    req->fixed_rate_cfg = rate_cfg;

    /* Send the ME_RC_SET_RATE_REQ message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_send_me_set_ps_mode(struct aml_hw *aml_hw, u8 ps_mode)
{
    struct me_set_ps_mode_req *req;
    int ret = 0;

    AML_INFO("set power save mode:%d", ps_mode);

    /* Build the ME_SET_PS_MODE_REQ message */
    req = aml_msg_zalloc(ME_SET_PS_MODE_REQ, TASK_ME, DRV_TASK_ID,
                          4);
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_SET_PS_MODE_REQ message */
    req->ps_state = ps_mode;

    /* Send the ME_SET_PS_MODE_REQ message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    ret = aml_send_msg(aml_hw, req, 1, ME_SET_PS_MODE_CFM, NULL);
#ifdef CONFIG_AML_RECOVERY
    if ((aml_recy != NULL) && !ret) {
        aml_recy->ps_state = ps_mode;
    }
#endif

    return ret;
}

struct element * aml_get_md_ie(struct aml_vif *vif, struct cfg80211_connect_params *sme)
{
    if ((sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM) && (vif->sta.ft_assoc_ies)) {
        /*get mdie*/
        const struct element *mde, *mde_req;
        mde_req = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN,
                                 sme->ie, sme->ie_len);
        mde = cfg80211_find_elem(WLAN_EID_MOBILITY_DOMAIN,
                             vif->sta.ft_assoc_ies, vif->sta.ft_assoc_ies_len);
        if ((!mde_req) && (mde)) {
            printk("add md ie\n");
            return mde;
        }
    }
    return NULL;
}

int aml_send_sm_connect_req(struct aml_hw *aml_hw,
                             struct aml_vif *aml_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm)
{
    struct sm_connect_req *req;
    int i, ie_len;
    const struct element *mde = NULL;

    AML_DBG(AML_FN_ENTRY_STR);

    ie_len = update_connect_req(aml_vif, sme);

    mde = aml_get_md_ie(aml_vif, sme);
    if (mde) {
        ie_len += (sizeof(struct element) + mde->datalen);
    }
    /* Build the SM_CONNECT_REQ message */
    req = aml_msg_zalloc(SM_CONNECT_REQ, TASK_SM, DRV_TASK_ID,
                     (sizeof(struct sm_connect_req) + ie_len));

    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_CONNECT_REQ message */
    if (sme->crypto.n_ciphers_pairwise &&
        ((sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_TKIP) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104)))
        req->flags |= DISABLE_HT;

    if (sme->crypto.control_port)
        req->flags |= CONTROL_PORT_HOST;

    if (sme->crypto.control_port_no_encrypt)
        req->flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&sme->crypto))
        req->flags |= WPA_WPA2_IN_USE;

    if (sme->mfp == NL80211_MFP_REQUIRED)
        req->flags |= MFP_IN_USE;

    req->ctrl_port_ethertype = sme->crypto.control_port_ethertype;

    if (sme->bssid)
        memcpy(&req->bssid, sme->bssid, ETH_ALEN);
    else
        req->bssid = mac_addr_bcst;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
    if (sme->prev_bssid)
        req->flags |= REASSOCIATION;
#else
    if (aml_vif->sta.ap)
        req->flags |= REASSOCIATION;
#endif

    if ((sme->auth_type == NL80211_AUTHTYPE_FT) && (aml_vif->sta.flags & AML_STA_FT_OVER_DS))
        req->flags |= (REASSOCIATION | FT_OVER_DS);

    req->vif_idx = aml_vif->vif_index;
    if (sme->channel) {
        req->chan.band = sme->channel->band;
        req->chan.freq = sme->channel->center_freq;
        req->chan.flags = get_chan_flags(sme->channel->flags);
    } else {
        req->chan.freq = (u16_l)-1;
    }
    for (i = 0; i < sme->ssid_len; i++)
        req->ssid.array[i] = sme->ssid[i];
    req->ssid.length = sme->ssid_len;

    req->listen_interval = aml_mod_params.listen_itv;
    req->dont_wait_bcmc = !aml_mod_params.listen_bcmc;

    /* Set auth_type */
    if (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) {
        if ((sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40 || sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104)
            && sme->key_len)
            req->auth_type = WLAN_AUTH_SHARED_KEY;
        else
            req->auth_type = WLAN_AUTH_OPEN;
    } else if (sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM)
        req->auth_type = WLAN_AUTH_OPEN;
    else if (sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY)
        req->auth_type = WLAN_AUTH_SHARED_KEY;
    else if (sme->auth_type == NL80211_AUTHTYPE_FT)
        req->auth_type = WLAN_AUTH_FT;
    else if (sme->auth_type == NL80211_AUTHTYPE_SAE)
        req->auth_type = WLAN_AUTH_SAE;
    else
        goto invalid_param;

    if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_SAE) {
        req->flags |= WPA3_SAE_IN_USE;
    }
    copy_connect_ies(aml_vif, req, sme);
    if (mde) {
        u8 * ie = (u8 *)req->ie_buf;
        ie += req->ie_len;
        memcpy(ie, mde, (sizeof(struct element) + mde->datalen));
        req->ie_len += (sizeof(struct element) + mde->datalen);
    }

    /* Set UAPSD queues */
    req->uapsd_queues = aml_mod_params.uapsd_queues;

    /* Send the SM_CONNECT_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, SM_CONNECT_CFM, cfm);

invalid_param:
    aml_msg_free(aml_hw, req);
    return -EINVAL;
}

int aml_send_sm_disconnect_req(struct aml_hw *aml_hw,
                                struct aml_vif *aml_vif,
                                u16 reason)
{
    struct sm_disconnect_req *req;
    int ret;

    AML_INFO("vif:%d",aml_vif->vif_index);

    /* Build the SM_DISCONNECT_REQ message */
    req = aml_msg_zalloc(SM_DISCONNECT_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_disconnect_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_DISCONNECT_REQ message */
    req->reason_code = reason;
    req->vif_idx = aml_vif->vif_index;

    /* Send the SM_DISCONNECT_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    ret = aml_send_msg(aml_hw, req, 1, SM_DISCONNECT_CFM, NULL);
#ifdef CONFIG_AML_RECOVERY
    if ((!ret)  && (!aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) && (aml_recy->reconnect_rest == 0)) {
       if ((aml_recy != NULL) && (aml_vif->vif_index == aml_recy->assoc_info.vif_idx)) {
            aml_recy_flags_clr(AML_RECY_ASSOC_INFO_SAVED);
       }
    }
#endif
    return ret;
}

int aml_send_sm_external_auth_required_rsp(struct aml_hw *aml_hw,
                                            struct aml_vif *aml_vif,
                                            u16 status)
{
    struct sm_external_auth_required_rsp *rsp;

    /* Build the SM_EXTERNAL_AUTH_CFM message */
    rsp = aml_msg_zalloc(SM_EXTERNAL_AUTH_REQUIRED_RSP, TASK_SM, DRV_TASK_ID,
                          sizeof(struct sm_external_auth_required_rsp));
    if (!rsp)
        return -ENOMEM;

    rsp->status = status;
    rsp->vif_idx = aml_vif->vif_index;

    /* send the SM_EXTERNAL_AUTH_REQUIRED_RSP message UMAC FW */
    /* coverity[leaked_storage] - rsp will be freed later */
    return aml_send_msg(aml_hw, rsp, 0, 0, NULL);
}

int aml_send_sm_ft_auth_rsp(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                             uint8_t *ie, int ie_len)
{
    struct sm_connect_req *rsp;

    rsp = aml_msg_zalloc(SM_FT_AUTH_RSP, TASK_SM, DRV_TASK_ID,
                         (sizeof(struct sm_connect_req) + ie_len));
    if (!rsp)
        return -ENOMEM;

    rsp->vif_idx = aml_vif->vif_index;
    rsp->ie_len = ie_len;
    memcpy(rsp->ie_buf, ie, rsp->ie_len);
    /* coverity[leaked_storage] - rsp will be freed later */
    return aml_send_msg(aml_hw, rsp, 0, 0, NULL);
}

int aml_send_apm_start_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm)
{
    struct apm_start_req *req;
    struct aml_bcn *bcn = &vif->ap.bcn;
    struct aml_ipc_buf buf;
    u8 *bcn_buf;
    u32 flags = 0;
    const u8 *rate_ie;
    const u8 *ht_cap_ie;
    u8 rate_len = 0;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    const u8 *var_pos;
    int len, i, error;
    unsigned int addr;
    enum nl80211_band band;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the APM_START_REQ message */
    req = aml_msg_zalloc(APM_START_REQ, TASK_APM, DRV_TASK_ID,
                                   sizeof(struct apm_start_req));
    if (!req)
        return -ENOMEM;

    // Build the beacon
    bcn->dtim = (u8)settings->dtim_period;
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy_flags_chk(AML_RECY_STATE_ONGOING) &&
        aml_recy_flags_chk(AML_RECY_AP_INFO_SAVED)) {
        bcn_buf = kmalloc(bcn->len, GFP_KERNEL);
        if (!bcn_buf) {
            aml_msg_free(aml_hw, req);
            /* coverity[leaked_storage] - req have already freed */
            return -ENOMEM;
        }
        memcpy(bcn_buf, bcn_save, bcn->len);
        band = aml_recy->ap_info.band;
    } else
#endif
    {
        bcn_buf = aml_build_bcn(bcn, &settings->beacon);
        if (!bcn_buf) {
            aml_msg_free(aml_hw, req);
            /* coverity[leaked_storage] - req have already freed */
            return -ENOMEM;
        }
        aml_save_bcn_buf(bcn_buf, bcn->len);
        band = settings->chandef.chan->band;
    }

    // Retrieve the basic rate set from the beacon buffer
    len = bcn->len - var_offset;
    var_pos = bcn_buf + var_offset;

// Assume that rate higher that 54 Mbps are BSS membership
#define IS_BASIC_RATE(r) (r & 0x80) && ((r & ~0x80) <= (54 * 2))

    rate_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const u8 *rates = rate_ie + 2;
        for (i = 0; (i < rate_ie[1]) && (rate_len < MAC_RATESET_LEN); i++) {
            if (IS_BASIC_RATE(rates[i]))
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    rate_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const u8 *rates = rate_ie + 2;
        for (i = 0; (i < rate_ie[1]) && (rate_len < MAC_RATESET_LEN); i++) {
            if (IS_BASIC_RATE(rates[i]))
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    req->basic_rates.length = rate_len;
#undef IS_BASIC_RATE

    ht_cap_ie = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, var_pos, len);
    if (ht_cap_ie) {
        struct ieee80211_ht_cap *ht_cap = ht_cap_ie + 2;
        ht_cap->cap_info &= ~(IEEE80211_HT_CAP_SGI_20 | IEEE80211_HT_CAP_SGI_40);
        ht_cap->cap_info |= aml_hw->mod_params->sgi ? IEEE80211_HT_CAP_SGI_20 : 0;
        ht_cap->cap_info |= aml_hw->mod_params->use_2040 ? IEEE80211_HT_CAP_SGI_40 : 0;
    }

#ifdef SCC_STA_SOFTAP
    aml_scc_save_init_band(band);
#endif

    // Sync buffer for FW
    if (aml_bus_type == PCIE_MODE) {
        if ((error = aml_ipc_buf_a2e_init(aml_hw, &buf, bcn_buf, bcn->len))) {
            netdev_err(vif->ndev, "Failed to allocate IPC buf for AP Beacon\n");
            kfree(bcn_buf);
            aml_msg_free(aml_hw, req);
            /* coverity[leaked_storage] - req have already freed */
            return -EIO;
        }
    } else if (aml_bus_type == USB_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len, USB_EP4);
        kfree(bcn_buf);
    } else if (aml_bus_type == SDIO_MODE) {
        addr = TXL_BCN_POOL  + (vif->vif_index * (BCN_TXLBUF_TAG_LEN + NX_BCNFRAME_LEN)) + BCN_TXLBUF_TAG_LEN;
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)bcn_buf, (unsigned char *)(unsigned long)addr, bcn->len);
        kfree(bcn_buf);
    }

    /* Set parameters for the APM_START_REQ message */
    req->vif_idx = vif->vif_index;
    req->bcn_addr = buf.dma_addr;
    req->bcn_len = bcn->len;
    req->tim_oft = bcn->head_len;
    req->tim_len = bcn->tim_len;
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy_flags_chk(AML_RECY_AP_INFO_SAVED)
            && aml_recy_flags_chk(AML_RECY_STATE_ONGOING)) {
        memcpy(&req->chan, &aml_recy->ap_info.chan, sizeof(struct mac_chan_op));
    } else
#endif
    {
        cfg80211_to_aml_chan(&settings->chandef, &req->chan);
    }
    req->bcn_int = settings->beacon_interval;
    if (settings->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    if (settings->crypto.control_port_no_encrypt)
        flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&settings->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (settings->crypto.control_port_ethertype)
        req->ctrl_port_ethertype = settings->crypto.control_port_ethertype;
    else
        req->ctrl_port_ethertype = ETH_P_PAE;
    req->flags = flags;

    /* Send the APM_START_REQ message to LMAC FW */
    error = aml_send_msg(aml_hw, req, 1, APM_START_CFM, cfm);

    aml_ipc_buf_dealloc(aml_hw, &buf);
    /* coverity[leaked_storage] - req will be freed later */
    return error;
}

int aml_send_apm_stop_req(struct aml_hw *aml_hw, struct aml_vif *vif)
{
    struct apm_stop_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the APM_STOP_REQ message */
    req = aml_msg_zalloc(APM_STOP_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_stop_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, APM_STOP_CFM, NULL);
}

int aml_send_apm_probe_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct aml_sta *sta, struct apm_probe_client_cfm *cfm)
{
    struct apm_probe_client_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    req = aml_msg_zalloc(APM_PROBE_CLIENT_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_probe_client_req));
    if (!req)
        return -ENOMEM;

    req->vif_idx = vif->vif_index;
    req->sta_idx = sta->sta_idx;

    /* Send the APM_PROBE_CLIENT_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, APM_PROBE_CLIENT_CFM, cfm);
}

int aml_send_scanu_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                        struct cfg80211_scan_request *param)
{
    struct scanu_start_req *req;
    int i;
    uint8_t chan_flags = 0;

    AML_INFO("vif:%d", aml_vif->vif_index);

    /* Build the SCANU_START_REQ message */
    req = aml_msg_zalloc(SCANU_START_REQ, TASK_SCANU, DRV_TASK_ID,
                          sizeof(struct scanu_start_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters */
    req->vif_idx = aml_vif->vif_index;
    req->chan_cnt = (u8)min_t(int, SCAN_CHANNEL_MAX, param->n_channels);
    req->ssid_cnt = (u8)min_t(int, SCAN_SSID_MAX, param->n_ssids);
    req->bssid = mac_addr_bcst;
    req->no_cck = param->no_cck;

    if (aml_vif->sta.scan_duration)
        req->duration = aml_vif->sta.scan_duration;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    if (param->duration_mandatory)
        req->duration = ieee80211_tu_to_usec(param->duration);
#endif

    if (req->ssid_cnt == 0)
        chan_flags |= CHAN_NO_IR;
    for (i = 0; i < req->ssid_cnt; i++) {
        int j;
        for (j = 0; j < param->ssids[i].ssid_len; j++)
            req->ssid[i].array[j] = param->ssids[i].ssid[j];
        req->ssid[i].length = param->ssids[i].ssid_len;
        if (req->ssid[i].length) {
            AML_INFO("len:%d ssid:%s", req->ssid[i].length,req->ssid[i].array);
        }
    }

    if (param->ie) {
        if (aml_bus_type == PCIE_MODE) {
            if (aml_ipc_buf_a2e_alloc(aml_hw, &aml_hw->scan_ie,
                                      param->ie_len, param->ie)) {
                netdev_err(aml_vif->ndev, "Failed to allocate IPC buf for SCAN IEs\n");
                goto error;
            }
        }

        req->add_ie_len = param->ie_len;
        req->add_ies = aml_hw->scan_ie.dma_addr;

        if (aml_bus_type == USB_MODE) {
            aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)param->ie, (unsigned char *)SCANU_ADD_IE + SCANU_ADD_IE_OFFSET, param->ie_len, USB_EP4);
        }
        if (aml_bus_type == SDIO_MODE) {
            aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)param->ie, (unsigned char *)SCANU_ADD_IE + SCANU_ADD_IE_OFFSET, param->ie_len);
        }

    } else {
        req->add_ie_len = 0;
        req->add_ies = 0;
    }

    for (i = 0; i < req->chan_cnt; i++) {
        struct ieee80211_channel *chan = param->channels[i];

        req->chan[i].band = chan->band;
        req->chan[i].freq = chan->center_freq;
        req->chan[i].flags = chan_flags | get_chan_flags(chan->flags);
        req->chan[i].tx_power = chan_to_fw_pwr(chan->max_reg_power);
    }

    /* Send the SCANU_START_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 0, 0, NULL);

error:
    if (req != NULL)
        aml_msg_free(aml_hw, req);

    /* coverity[leaked_storage] - req have already freed */
    return -ENOMEM;
}

int aml_send_apm_start_cac_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm)
{
    struct apm_start_cac_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the APM_START_CAC_REQ message */
    req = aml_msg_zalloc(APM_START_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_start_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_START_CAC_REQ message */
    req->vif_idx = vif->vif_index;
    cfg80211_to_aml_chan(chandef, &req->chan);

    /* Send the APM_START_CAC_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, APM_START_CAC_CFM, cfm);
}

int aml_send_apm_stop_cac_req(struct aml_hw *aml_hw, struct aml_vif *vif)
{
    struct apm_stop_cac_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the APM_STOP_CAC_REQ message */
    req = aml_msg_zalloc(APM_STOP_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_stop_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_CAC_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_CAC_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, APM_STOP_CAC_CFM, NULL);
}

int aml_send_mesh_start_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                             const struct mesh_config *conf, const struct mesh_setup *setup,
                             struct mesh_start_cfm *cfm)
{
    // Message to send
    struct mesh_start_req *req;
    // Supported basic rates
    struct ieee80211_supported_band *band = aml_hw->wiphy->bands[setup->chandef.chan->band];
    /* Counter */
    int i;
    /* Return status */
    int status;
    /* DMA Address to be unmapped after confirmation reception */
    u32 dma_addr = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_START_REQ message */
    req = aml_msg_zalloc(MESH_START_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_start_req));
    if (!req) {
        return -ENOMEM;
    }

    req->vif_index = vif->vif_index;
    req->bcn_int = setup->beacon_interval;
    req->dtim_period = setup->dtim_period;
    req->mesh_id_len = setup->mesh_id_len;

    for (i = 0; i < setup->mesh_id_len; i++) {
        req->mesh_id[i] = *(setup->mesh_id + i);
    }

    req->user_mpm = setup->user_mpm;
    req->is_auth = setup->is_authenticated;
    req->auth_id = setup->auth_id;
    req->ie_len = setup->ie_len;

    if (setup->ie_len) {
        /*
         * Need to provide a Virtual Address to the MAC so that it can download the
         * additional information elements.
         */
        req->ie_addr = dma_map_single(aml_hw->dev, (void *)setup->ie,
                                      setup->ie_len, DMA_FROM_DEVICE);

        /* Check DMA mapping result */
        if (dma_mapping_error(aml_hw->dev, req->ie_addr)) {
            printk(KERN_CRIT "%s - DMA Mapping error on additional IEs\n", __func__);

            /* Consider there is no Additional IEs */
            req->ie_len = 0;
        } else {
            /* Store DMA Address so that we can unmap the memory section once MESH_START_CFM is received */
            dma_addr = req->ie_addr;
        }
    }

    /* Provide rate information */
    req->basic_rates.length = 0;
    for (i = 0; i < band->n_bitrates; i++) {
        u16 rate = band->bitrates[i].bitrate;

        /* Read value is in in units of 100 Kbps, provided value is in units
         * of 1Mbps, and multiplied by 2 so that 5.5 becomes 11 */
        rate = (rate << 1) / 10;

        if (setup->basic_rates & CO_BIT(i)) {
            rate |= 0x80;
        }

        req->basic_rates.array[i] = (u8)rate;
        req->basic_rates.length++;
    }

    /* Provide channel information */
    cfg80211_to_aml_chan(&setup->chandef, &req->chan);

    /* Send the MESH_START_REQ message to UMAC FW */
    status = aml_send_msg(aml_hw, req, 1, MESH_START_CFM, cfm);

    /* Unmap DMA area */
    if (setup->ie_len) {
        dma_unmap_single(aml_hw->dev, dma_addr, setup->ie_len, DMA_TO_DEVICE);
    }

    /* Return the status */
    /* coverity[leaked_storage] - req will be freed later */
    return (status);
}

int aml_send_mesh_stop_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                            struct mesh_stop_cfm *cfm)
{
    // Message to send
    struct mesh_stop_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_STOP_REQ message */
    req = aml_msg_zalloc(MESH_STOP_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_stop_req));
    if (!req) {
        return -ENOMEM;
    }

    req->vif_idx = vif->vif_index;

    /* Send the MESH_STOP_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MESH_STOP_CFM, cfm);
}

int aml_send_mesh_update_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                              u32 mask, const struct mesh_config *p_mconf, struct mesh_update_cfm *cfm)
{
    // Message to send
    struct mesh_update_req *req;
    // Keep only bit for fields which can be updated
    u32 supp_mask = (mask << 1) & (CO_BIT(NL80211_MESHCONF_GATE_ANNOUNCEMENTS)
                                   | CO_BIT(NL80211_MESHCONF_HWMP_ROOTMODE)
                                   | CO_BIT(NL80211_MESHCONF_FORWARDING)
                                   | CO_BIT(NL80211_MESHCONF_POWER_MODE));


    AML_DBG(AML_FN_ENTRY_STR);

    if (!supp_mask) {
        return -ENOENT;
    }

    /* Build the MESH_UPDATE_REQ message */
    req = aml_msg_zalloc(MESH_UPDATE_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_update_req));

    if (!req) {
        return -ENOMEM;
    }

    req->vif_idx = vif->vif_index;

    if (supp_mask & CO_BIT(NL80211_MESHCONF_GATE_ANNOUNCEMENTS))
    {
        req->flags |= CO_BIT(MESH_UPDATE_FLAGS_GATE_MODE_BIT);
        req->gate_announ = p_mconf->dot11MeshGateAnnouncementProtocol;
    }

    if (supp_mask & CO_BIT(NL80211_MESHCONF_HWMP_ROOTMODE))
    {
        req->flags |= CO_BIT(MESH_UPDATE_FLAGS_ROOT_MODE_BIT);
        req->root_mode = p_mconf->dot11MeshHWMPRootMode;
    }

    if (supp_mask & CO_BIT(NL80211_MESHCONF_FORWARDING))
    {
        req->flags |= CO_BIT(MESH_UPDATE_FLAGS_MESH_FWD_BIT);
        req->mesh_forward = p_mconf->dot11MeshForwarding;
    }

    if (supp_mask & CO_BIT(NL80211_MESHCONF_POWER_MODE))
    {
        req->flags |= CO_BIT(MESH_UPDATE_FLAGS_LOCAL_PSM_BIT);
        req->local_ps_mode = p_mconf->power_mode;
    }

    /* Send the MESH_UPDATE_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MESH_UPDATE_CFM, cfm);
}

int aml_send_mesh_peer_info_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                                 u8 sta_idx, struct mesh_peer_info_cfm *cfm)
{
    // Message to send
    struct mesh_peer_info_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_PEER_INFO_REQ message */
    req = aml_msg_zalloc(MESH_PEER_INFO_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_peer_info_req));
    if (!req) {
        return -ENOMEM;
    }

    req->sta_idx = sta_idx;

    /* Send the MESH_PEER_INFO_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MESH_PEER_INFO_CFM, cfm);
}

void aml_send_mesh_peer_update_ntf(struct aml_hw *aml_hw, struct aml_vif *vif,
                                    u8 sta_idx, u8 mlink_state)
{
    // Message to send
    struct mesh_peer_update_ntf *ntf;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_PEER_UPDATE_NTF message */
    ntf = aml_msg_zalloc(MESH_PEER_UPDATE_NTF, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_peer_update_ntf));

    if (ntf) {
        ntf->vif_idx = vif->vif_index;
        ntf->sta_idx = sta_idx;
        ntf->state = mlink_state;

        /* Send the MESH_PEER_INFO_REQ message to UMAC FW */
        aml_send_msg(aml_hw, ntf, 0, 0, NULL);
        /* coverity[leaked_storage] - ntf will be freed later */
    }
}

void aml_send_mesh_path_create_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 *tgt_addr)
{
    struct mesh_path_create_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Check if we are already waiting for a confirmation */
    if (vif->ap.flags & AML_AP_CREATE_MESH_PATH)
        return;

    /* Build the MESH_PATH_CREATE_REQ message */
    req = aml_msg_zalloc(MESH_PATH_CREATE_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_path_create_req));
    if (!req)
        return;

    req->vif_idx = vif->vif_index;
    memcpy(&req->tgt_mac_addr, tgt_addr, ETH_ALEN);

    vif->ap.flags |= AML_AP_CREATE_MESH_PATH;

    /* Send the MESH_PATH_CREATE_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    aml_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_send_mesh_path_update_req(struct aml_hw *aml_hw, struct aml_vif *vif, const u8 *tgt_addr,
                                   const u8 *p_nhop_addr, struct mesh_path_update_cfm *cfm)
{
    // Message to send
    struct mesh_path_update_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_PATH_UPDATE_REQ message */
    req = aml_msg_zalloc(MESH_PATH_UPDATE_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_path_update_req));
    if (!req) {
        return -ENOMEM;
    }

    req->delete = (p_nhop_addr == NULL);
    req->vif_idx = vif->vif_index;
    memcpy(&req->tgt_mac_addr, tgt_addr, ETH_ALEN);

    if (p_nhop_addr) {
        memcpy(&req->nhop_mac_addr, p_nhop_addr, ETH_ALEN);
    }

    /* Send the MESH_PATH_UPDATE_REQ message to UMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MESH_PATH_UPDATE_CFM, cfm);
}

void aml_send_mesh_proxy_add_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 *ext_addr)
{
    // Message to send
    struct mesh_proxy_add_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MESH_PROXY_ADD_REQ message */
    req = aml_msg_zalloc(MESH_PROXY_ADD_REQ, TASK_MESH, DRV_TASK_ID,
                          sizeof(struct mesh_proxy_add_req));

    if (req) {
        req->vif_idx = vif->vif_index;
        memcpy(&req->ext_sta_addr, ext_addr, ETH_ALEN);

        /* Send the MESH_PROXY_ADD_REQ message to UMAC FW */
        aml_send_msg(aml_hw, req, 0, 0, NULL);
        /* coverity[leaked_storage] - req will be freed later */
    }
}

int aml_send_tdls_peer_traffic_ind_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif)
{
    struct tdls_peer_traffic_ind_req *tdls_peer_traffic_ind_req;

    if (!aml_vif->sta.tdls_sta)
        return -ENOLINK;

    /* Build the TDLS_PEER_TRAFFIC_IND_REQ message */
    tdls_peer_traffic_ind_req = aml_msg_zalloc(TDLS_PEER_TRAFFIC_IND_REQ, TASK_TDLS, DRV_TASK_ID,
                                           sizeof(struct tdls_peer_traffic_ind_req));

    if (!tdls_peer_traffic_ind_req)
        return -ENOMEM;

    /* Set parameters for the TDLS_PEER_TRAFFIC_IND_REQ message */
    tdls_peer_traffic_ind_req->vif_index = aml_vif->vif_index;
    tdls_peer_traffic_ind_req->sta_idx = aml_vif->sta.tdls_sta->sta_idx;
    memcpy(&(tdls_peer_traffic_ind_req->peer_mac_addr.array[0]),
           aml_vif->sta.tdls_sta->mac_addr, ETH_ALEN);
    tdls_peer_traffic_ind_req->dialog_token = 0; // check dialog token value
    tdls_peer_traffic_ind_req->last_tid = aml_vif->sta.tdls_sta->tdls.last_tid;
    tdls_peer_traffic_ind_req->last_sn = aml_vif->sta.tdls_sta->tdls.last_sn;

    /* Send the TDLS_PEER_TRAFFIC_IND_REQ message to LMAC FW */
    /* coverity[leaked_storage] - tdls_peer_traffic_ind_req will be freed later */
    return aml_send_msg(aml_hw, tdls_peer_traffic_ind_req, 0, 0, NULL);
}

int aml_send_config_monitor_req(struct aml_hw *aml_hw,
                                 struct cfg80211_chan_def *chandef,
                                 struct me_config_monitor_cfm *cfm)
{
    struct me_config_monitor_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_CONFIG_MONITOR_REQ message */
    req = aml_msg_zalloc(ME_CONFIG_MONITOR_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_config_monitor_req));
    if (!req)
        return -ENOMEM;

    if (chandef) {
        req->chan_set = true;
        cfg80211_to_aml_chan(chandef, &req->chan);

        if (aml_hw->phy.limit_bw)
            limit_chan_bw(&req->chan.type, req->chan.prim20_freq, &req->chan.center1_freq);
    } else {
         req->chan_set = false;
    }

    req->uf = aml_hw->mod_params->uf;

    /* Send the ME_CONFIG_MONITOR_REQ message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, ME_CONFIG_MONITOR_CFM, cfm);
}
#endif /* CONFIG_AML_FULLMAC */

int aml_send_tdls_chan_switch_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif,
                                   struct aml_sta *aml_sta, bool sta_initiator,
                                   u8 oper_class, struct cfg80211_chan_def *chandef,
                                   struct tdls_chan_switch_cfm *cfm)
{
    struct tdls_chan_switch_req *tdls_chan_switch_req;

#ifdef CONFIG_AML_SOFTMAC
    /* Check if channel switch already enabled on a TDLS peer */
    if (aml_hw->tdls_info.chsw_en) {
        pr_err("TDLS channel switch already enabled for another TDLS station\n");
        return -ENOTSUPP;
    }
#endif

    /* Build the TDLS_CHAN_SWITCH_REQ message */
    tdls_chan_switch_req = aml_msg_zalloc(TDLS_CHAN_SWITCH_REQ, TASK_TDLS, DRV_TASK_ID,
                                           sizeof(struct tdls_chan_switch_req));

    if (!tdls_chan_switch_req)
        return -ENOMEM;

    /* Set parameters for the TDLS_CHAN_SWITCH_REQ message */
    tdls_chan_switch_req->vif_index = aml_vif->vif_index;
    tdls_chan_switch_req->sta_idx = aml_sta->sta_idx;
    memcpy(&(tdls_chan_switch_req->peer_mac_addr.array[0]),
           aml_sta_addr(aml_sta), ETH_ALEN);
    tdls_chan_switch_req->initiator = sta_initiator;
    cfg80211_to_aml_chan(chandef, &tdls_chan_switch_req->chan);
    tdls_chan_switch_req->op_class = oper_class;

    /* Send the TDLS_CHAN_SWITCH_REQ message to LMAC FW */
    /* coverity[leaked_storage] - tdls_chan_switch_req will be freed later */
    return aml_send_msg(aml_hw, tdls_chan_switch_req, 1, TDLS_CHAN_SWITCH_CFM, cfm);
}

int aml_send_tdls_cancel_chan_switch_req(struct aml_hw *aml_hw,
                                          struct aml_vif *aml_vif,
                                          struct aml_sta *aml_sta,
                                          struct tdls_cancel_chan_switch_cfm *cfm)
{
    struct tdls_cancel_chan_switch_req *tdls_cancel_chan_switch_req;

    /* Build the TDLS_CHAN_SWITCH_REQ message */
    tdls_cancel_chan_switch_req = aml_msg_zalloc(TDLS_CANCEL_CHAN_SWITCH_REQ, TASK_TDLS, DRV_TASK_ID,
                                           sizeof(struct tdls_cancel_chan_switch_req));
    if (!tdls_cancel_chan_switch_req)
        return -ENOMEM;

    /* Set parameters for the TDLS_CHAN_SWITCH_REQ message */
    tdls_cancel_chan_switch_req->vif_index = aml_vif->vif_index;
    tdls_cancel_chan_switch_req->sta_idx = aml_sta->sta_idx;
    memcpy(&(tdls_cancel_chan_switch_req->peer_mac_addr.array[0]),
           aml_sta_addr(aml_sta), ETH_ALEN);

    /* Send the TDLS_CHAN_SWITCH_REQ message to LMAC FW */
    /* coverity[leaked_storage] - tdls_cancel_chan_switch_req will be freed later */
    return aml_send_msg(aml_hw, tdls_cancel_chan_switch_req, 1, TDLS_CANCEL_CHAN_SWITCH_CFM, cfm);
}

#ifdef CONFIG_AML_BFMER
#ifdef CONFIG_AML_SOFTMAC
void aml_send_bfmer_enable(struct aml_hw *aml_hw, struct ieee80211_sta *sta)
#else
void aml_send_bfmer_enable(struct aml_hw *aml_hw, struct aml_sta *aml_sta,
                            const struct ieee80211_vht_cap *vht_cap)
#endif /* CONFIG_AML_SOFTMAC*/
{
    struct mm_bfmer_enable_req *bfmer_en_req;
#ifdef CONFIG_AML_SOFTMAC
    struct aml_sta *aml_sta = (struct aml_sta *)&sta->drv_priv;
    u32 vht_capability;
#else
    __le32 vht_capability;
    u8 rx_nss = 0;
#endif /* CONFIG_AML_SOFTMAC */

    AML_DBG(AML_FN_ENTRY_STR);

#ifdef CONFIG_AML_SOFTMAC
    if (!sta->vht_cap.vht_supported) {
#else
    if (!vht_cap) {
#endif /* CONFIG_AML_SOFTMAC */
        goto end;
    }

#ifdef CONFIG_AML_SOFTMAC
    vht_capability = sta->vht_cap.cap;
#else
    vht_capability = vht_cap->vht_cap_info;
#endif /* CONFIG_AML_SOFTMAC */

    if (!(vht_capability & IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE)) {
        goto end;
    }

#ifdef CONFIG_AML_FULLMAC
    rx_nss = aml_bfmer_get_rx_nss(vht_cap);
#endif /* CONFIG_AML_FULLMAC */

    /* Allocate a structure that will contain the beamforming report */
    if (aml_bfmer_report_add(aml_hw, aml_sta, AML_BFMER_REPORT_SPACE_SIZE))
    {
        goto end;
    }

    /* Build the MM_BFMER_ENABLE_REQ message */
    bfmer_en_req = aml_msg_zalloc(MM_BFMER_ENABLE_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_bfmer_enable_req));

    /* Check message allocation */
    if (!bfmer_en_req) {
        /* Free memory allocated for the report */
        aml_bfmer_report_del(aml_hw, aml_sta);

        /* Do not use beamforming */
        goto end;
    }

    /* Provide DMA address to the MAC */
    bfmer_en_req->host_bfr_addr = aml_sta->bfm_report->dma_addr;
    bfmer_en_req->host_bfr_size = AML_BFMER_REPORT_SPACE_SIZE;
    bfmer_en_req->sta_idx = aml_sta->sta_idx;
#ifdef CONFIG_AML_SOFTMAC
    bfmer_en_req->aid = sta->aid;
    bfmer_en_req->rx_nss = sta->rx_nss;
#else
    bfmer_en_req->aid = aml_sta->aid;
    bfmer_en_req->rx_nss = rx_nss;
#endif /* CONFIG_AML_SOFTMAC */

    if (vht_capability & IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE) {
        bfmer_en_req->vht_mu_bfmee = true;
    } else {
        bfmer_en_req->vht_mu_bfmee = false;
    }

    /* Send the MM_BFMER_EN_REQ message to LMAC FW */
    /* coverity[leaked_storage] - bfmer_en_req will be freed later */
    aml_send_msg(aml_hw, bfmer_en_req, 0, 0, NULL);

end:
    /* coverity[leaked_storage] - bfmer_en_req don't be freed */
    return;
}

#ifdef CONFIG_AML_MUMIMO_TX
int aml_send_mu_group_update_req(struct aml_hw *aml_hw, struct aml_sta *aml_sta)
{
    struct mm_mu_group_update_req *req;
    int group_id, i = 0;
    u64 map;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_MU_GROUP_UPDATE_REQ message */
    req = aml_msg_zalloc(MM_MU_GROUP_UPDATE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_mu_group_update_req) +
                          aml_sta->group_info.cnt * sizeof(req->groups[0]));

    /* Check message allocation */
    if (!req)
        return -ENOMEM;

    /* Go through the groups the STA belongs to */
    group_sta_for_each(aml_sta, group_id, map) {
        int user_pos = aml_mu_group_sta_get_pos(aml_hw, aml_sta, group_id);

        if (WARN((i >= aml_sta->group_info.cnt),
                 "STA%d: Too much group (%d)\n",
                 aml_sta->sta_idx, i + 1))
            break;

        req->groups[i].group_id = group_id;
        req->groups[i].user_pos = user_pos;

        i++;
    }

    req->group_cnt = aml_sta->group_info.cnt;
    req->sta_idx = aml_sta->sta_idx;

    /* Send the MM_MU_GROUP_UPDATE_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 1, MM_MU_GROUP_UPDATE_CFM, NULL);
}
#endif /* CONFIG_AML_MUMIMO_TX */
#endif /* CONFIG_AML_BFMER */

/**********************************************************************
 *    Debug Messages
 *********************************************************************/
int aml_send_dbg_trigger_req(struct aml_hw *aml_hw, char *msg)
{
    struct mm_dbg_trigger_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_DBG_TRIGGER_REQ message */
    req = aml_msg_zalloc(MM_DBG_TRIGGER_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_dbg_trigger_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_DBG_TRIGGER_REQ message */
    strncpy(req->error, msg, sizeof(req->error)-1);

    /* Send the MM_DBG_TRIGGER_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 0, -1, NULL);
}

int aml_send_dbg_mem_read_req(struct aml_hw *aml_hw, u32 mem_addr,
                               struct dbg_mem_read_cfm *cfm)
{
    struct dbg_mem_read_req *mem_read_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the DBG_MEM_READ_REQ message */
    mem_read_req = aml_msg_zalloc(DBG_MEM_READ_REQ, TASK_DBG, DRV_TASK_ID,
                                   sizeof(struct dbg_mem_read_req));
    if (!mem_read_req)
        return -ENOMEM;

    /* Set parameters for the DBG_MEM_READ_REQ message */
    mem_read_req->memaddr = mem_addr;

    /* Send the DBG_MEM_READ_REQ message to LMAC FW */
    /* coverity[leaked_storage] - mem_read_req will be freed later */
    return aml_send_msg(aml_hw, mem_read_req, 1, DBG_MEM_READ_CFM, cfm);
}

int aml_send_dbg_mem_write_req(struct aml_hw *aml_hw, u32 mem_addr,
                                u32 mem_data)
{
    struct dbg_mem_write_req *mem_write_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the DBG_MEM_WRITE_REQ message */
    mem_write_req = aml_msg_zalloc(DBG_MEM_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_mem_write_req));
    if (!mem_write_req)
        return -ENOMEM;

    /* Set parameters for the DBG_MEM_WRITE_REQ message */
    mem_write_req->memaddr = mem_addr;
    mem_write_req->memdata = mem_data;

    /* Send the DBG_MEM_WRITE_REQ message to LMAC FW */
    /* coverity[leaked_storage] - mem_write_req will be freed later */
    return aml_send_msg(aml_hw, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int aml_send_dbg_set_mod_filter_req(struct aml_hw *aml_hw, u32 filter)
{
    struct dbg_set_mod_filter_req *set_mod_filter_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the DBG_SET_MOD_FILTER_REQ message */
    set_mod_filter_req =
        aml_msg_zalloc(DBG_SET_MOD_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                        sizeof(struct dbg_set_mod_filter_req));
    if (!set_mod_filter_req)
        return -ENOMEM;

    /* Set parameters for the DBG_SET_MOD_FILTER_REQ message */
    set_mod_filter_req->mod_filter = filter;

    /* Send the DBG_SET_MOD_FILTER_REQ message to LMAC FW */
    /* coverity[leaked_storage] - set_mod_filter_req will be freed later */
    return aml_send_msg(aml_hw, set_mod_filter_req, 1, DBG_SET_MOD_FILTER_CFM, NULL);
}

int aml_send_dbg_set_sev_filter_req(struct aml_hw *aml_hw, u32 filter)
{
    struct dbg_set_sev_filter_req *set_sev_filter_req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the DBG_SET_SEV_FILTER_REQ message */
    set_sev_filter_req =
        aml_msg_zalloc(DBG_SET_SEV_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                        sizeof(struct dbg_set_sev_filter_req));
    if (!set_sev_filter_req)
        return -ENOMEM;

    /* Set parameters for the DBG_SET_SEV_FILTER_REQ message */
    set_sev_filter_req->sev_filter = filter;

    /* Send the DBG_SET_SEV_FILTER_REQ message to LMAC FW */
    /* coverity[leaked_storage] - set_sev_filter_req will be freed later */
    return aml_send_msg(aml_hw, set_sev_filter_req, 1, DBG_SET_SEV_FILTER_CFM, NULL);
}

int aml_send_dbg_get_sys_stat_req(struct aml_hw *aml_hw,
                                   struct dbg_get_sys_stat_cfm *cfm)
{
    void *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Allocate the message */
    req = aml_msg_zalloc(DBG_GET_SYS_STAT_REQ, TASK_DBG, DRV_TASK_ID, 0);
    if (!req)
        return -ENOMEM;

    /* Send the DBG_MEM_READ_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be free later */
    return aml_send_msg(aml_hw, req, 1, DBG_GET_SYS_STAT_CFM, cfm);
}

int aml_send_cfg_rssi_req(struct aml_hw *aml_hw, u8 vif_index, int rssi_thold, u32 rssi_hyst)
{
    struct mm_cfg_rssi_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_CFG_RSSI_REQ message */
    req = aml_msg_zalloc(MM_CFG_RSSI_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_cfg_rssi_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CFG_RSSI_REQ message */
    req->vif_index = vif_index;
    req->rssi_thold = (s8)rssi_thold;
    req->rssi_hyst = (u8)rssi_hyst;

    /* Send the MM_CFG_RSSI_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_rf_reg_write(struct net_device *dev, int addr, int value)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct rf_write_req *rf_write_param;

    rf_write_param = aml_priv_msg_zalloc(MM_SUB_RF_WRITE, sizeof(struct rf_write_req));
    if (!rf_write_param)
        return -ENOMEM;

    rf_write_param->rf_addr = addr;
    rf_write_param->rf_data = value;
    /* coverity[leaked_storage] - rf_write_param will be freed later */
    return aml_priv_send_msg(aml_hw, rf_write_param, 0, 0, NULL);
}

int aml_rf_reg_read(struct net_device *dev, int addr)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct rf_read_req *rf_read_param;
    struct rf_read_result_ind ind;

    memset(&ind, 0, sizeof(struct rf_read_result_ind));

    rf_read_param = aml_priv_msg_zalloc(MM_SUB_RF_READ, sizeof(struct rf_read_req));
    if (!rf_read_param)
        return -ENOMEM;

    rf_read_param->rf_addr = addr;
    aml_priv_send_msg(aml_hw, rf_read_param, 1, PRIV_RF_READ_RESULT, &ind);

    if (ind.rf_addr != addr) {
         printk("get_rf_reg:0x%x erro!\n", ind.rf_addr);
    }

    /* coverity[leaked_storage] - rf_read_param will be freed later */
    return ind.rf_data;
}

int aml_send_me_shutdown(struct aml_hw *aml_hw)
{
    void *shutdown_req = NULL;
    int ret;
    int count = 0;
    bool msg_recv;
    unsigned int value;
    printk("%s %d, aml_send_me_shutdown begin \n",__func__, __LINE__);

    shutdown_req = aml_priv_msg_zalloc(MM_SUB_SHUTDOWN, 0);
    if (!shutdown_req)
        return -ENOMEM;

    ret = aml_priv_send_msg(aml_hw, shutdown_req, 0, MM_MSG_BYPASS_ID, NULL);

    //wait fw set msg recv flag
    do
    {
        value = AML_REG_READ(aml_hw->plat, AML_ADDR_AON, RG_AON_A55);
        if (value != 0xffffffff) {
            msg_recv = value & BIT(21);
        }
        msleep(10);
        if (count++ > 100) {
            printk("%s %d, ERROR wait shutdown_ind timeout:%d \n",
                __func__, __LINE__, msg_recv );
            return ret;
        }
    }while (!msg_recv);

    printk("%s %d, shutdown_msg_send_ok! \n",__func__, __LINE__);

    return ret;
}

int aml_csi_status_com_read(struct net_device *dev, struct csi_status_com_get_ind *ind)
{
    void *void_param;
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    void_param = aml_priv_msg_zalloc(MM_SUB_CSI_STATUS_COM_GET, 0);
    if (!void_param)
        return -ENOMEM;

    aml_priv_send_msg(aml_hw, void_param, 1, PRIV_CSI_STATUS_COM_CFM, ind);

    return 0;
}

int aml_csi_status_sp_read(struct net_device *dev, int csi_mode, int csi_time_interval, struct csi_status_sp_get_ind *ind)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct csi_status_sp_get_req *csi_get_param;

    csi_get_param = aml_priv_msg_zalloc(MM_SUB_CSI_STATUS_SP_GET, sizeof(struct csi_status_sp_get_req));
    if (!csi_get_param)
        return -ENOMEM;

    csi_get_param->csi_mode = csi_mode;
    csi_get_param->csi_time_interval = csi_time_interval;
    printk("csi mode:0x%x csi_time_interval:%d \n", csi_mode, csi_time_interval);
    aml_priv_send_msg(aml_hw, csi_get_param, 1, PRIV_CSI_STATUS_SP_CFM, ind);

    return 0;
}

unsigned int aml_efuse_read(struct aml_hw *aml_hw, u32 addr)
{
    struct get_efuse_req *req = NULL;
    struct efuse_read_result_ind ind;

    memset(&ind, 0, sizeof(struct efuse_read_result_ind));

    req = aml_priv_msg_zalloc(MM_SUB_READ_EFUSE, sizeof(struct get_efuse_req));
    if (!req)
        return -ENOMEM;

    memset((void *)req, 0,sizeof(struct get_efuse_req));
    req->addr = addr;

    /* coverity[leaked_storage] - req will be freed later */
    aml_priv_send_msg(aml_hw, req, 1, PRIV_EFUSE_READ_RESULT, &ind);
    if (ind.addr != addr) {
         printk("read_efuse:%x erro!\n", ind.addr);
    }
    printk("read_efuse:value = %x\n", ind.value);

    return ind.value;
}

int aml_scan_hang(struct aml_vif *aml_vif, int scan_hang)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct scan_hang_req *scan_hang_param;

    scan_hang_param = aml_priv_msg_zalloc(MM_SCAN_HANG, sizeof(struct scan_hang_req));
    if (!scan_hang_param)
        return -ENOMEM;

    scan_hang_param->scan_hang = scan_hang;
    /* coverity[leaked_storage] - scan_hang_param will be freed later */
    return aml_priv_send_msg(aml_hw, scan_hang_param, 0, 0, NULL);
}

int aml_set_limit_power(struct aml_hw *aml_hw, int limit_power_switch)
{
    int *fw_limit_power_switch = NULL;

    fw_limit_power_switch = aml_priv_msg_zalloc(MM_SUB_LIMIT_POWER, sizeof(int));
    if (!fw_limit_power_switch)
        return -ENOMEM;

    *fw_limit_power_switch = limit_power_switch;

    return aml_priv_send_msg(aml_hw, fw_limit_power_switch, 0, 0, NULL);
}


int aml_send_suspend_req(struct aml_hw *aml_hw, u8_l filter, enum wifi_suspend_state state)
{
    struct mm_set_suspend_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SUB_SET_SUSPEND_REQ message */
    req = aml_priv_msg_zalloc(MM_SUB_SET_SUSPEND_REQ, sizeof(struct mm_set_suspend_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_SET_SUSPEND_REQ message */
    req->suspend_state = (u8_l)state;
    req->filter = filter;

    /* Send the MM_SUB_SET_SUSPEND_REQ message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    // MSG_BYPASS: insmod module, not open interface
    return aml_priv_send_msg(aml_hw, req, 0, MM_MSG_BYPASS_ID, NULL);
}

int aml_send_wow_pattern (struct aml_hw *aml_hw, struct aml_vif *vif,
    struct cfg80211_pkt_pattern *param, int id)
{
    struct mm_set_wow_pattern *req;
    int mask_len = 0;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SUB_SET_WOW_PATTERN message */
    req = aml_priv_msg_zalloc(MM_SUB_SET_WOW_PATTERN, sizeof(struct mm_set_wow_pattern));
    if (!req)
        return -ENOMEM;

    mask_len = ALIGN(param->pattern_len, WOW_PATTERN_MASK_SIZE) / WOW_PATTERN_MASK_SIZE;
    req->vif_idx = vif->vif_index;
    req->pattern_len = param->pattern_len > WOW_PATTERN_SIZE ? WOW_PATTERN_SIZE : param->pattern_len;
    req->pkt_offset = 0;
    req->pattern_id = id;
    memcpy(req->pattern, (unsigned char*)param->pattern, req->pattern_len);
    memcpy(req->mask, (unsigned char*)param->mask, mask_len);

    /* Send the ME_SET_WOW_PATTERN message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_send_scanu_cancel_req(struct aml_hw *aml_hw, struct aml_vif *vif, struct scanu_cancel_cfm *cfm)
{
    struct scanu_cancel_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the MM_SUB_SCANU_CANCEL_REQ message */
    req = aml_priv_msg_zalloc(MM_SUB_SCANU_CANCEL_REQ, sizeof(struct scanu_cancel_req));
    if (!req)
        return -ENOMEM;

    req->vif_idx = vif->vif_index;
    /* Send the MM_SUB_SCANU_CANCEL_REQ message to LMAC FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 1, PRIV_SCANU_CANCEL_CFM, cfm);
}

int aml_send_arp_agent_req(struct aml_hw *aml_hw, struct aml_vif *vif, u8 enable, u32 ipv4, u8 *ipv6)
{
    struct arp_agent_req *req;

    AML_DBG(AML_FN_ENTRY_STR);
    /* Build the MM_SUB_ARP_AGENT_REQ message */
    req = aml_priv_msg_zalloc(MM_SUB_ARP_AGENT_REQ, sizeof(struct arp_agent_req));
    if (!req)
        return -ENOMEM;

    req->enable = enable;
    req->vif_idx = vif->vif_index;
    req->ipv4 = ipv4;
    if (ipv6) {
        memcpy(req->ipv6, ipv6, IPV6_ADDR_BUF_LEN);
    }
    /* Send the MM_SUB_ARP_AGENT_REQ message to FW */
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_get_sched_scan_req(struct aml_vif *aml_vif, struct cfg80211_sched_scan_request *request,
    struct scanu_sched_scan_start_req *sched_start_req)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct scanu_start_req *req = &sched_start_req->scanu_req;
    int i;
    uint8_t chan_flags = 0;

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,12,0)
    sched_start_req->reqid = request->reqid;
#endif
    sched_start_req->min_rssi_thold = request->match_sets->rssi_thold;

     /* Set parameters */
    req->vif_idx = aml_vif->vif_index;
    req->chan_cnt = (u8)min_t(int, SCAN_CHANNEL_MAX, request->n_channels);
    req->ssid_cnt = (u8)min_t(int, SCAN_SSID_MAX, request->n_ssids);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,12,0)
    memcpy(&req->bssid, request->match_sets->bssid, 6);
#else
    req->bssid = mac_addr_bcst;
#endif
    if (req->ssid_cnt == 0)
        chan_flags |= CHAN_NO_IR;
    for (i = 0; i < req->ssid_cnt; i++) {
        int j;
        for (j = 0; j < request->ssids[i].ssid_len; j++)
            req->ssid[i].array[j] = request->ssids[i].ssid[j];
        req->ssid[i].length = request->ssids[i].ssid_len;
    }

    if (request->ie) {
        if (aml_ipc_buf_a2e_alloc(aml_hw, &aml_hw->scan_ie,
                                   request->ie_len, request->ie)) {
            netdev_err(aml_vif->ndev, "Failed to allocate IPC buf for SCAN IEs\n");
            return -ENOMEM;
        }

        req->add_ie_len = request->ie_len;
        req->add_ies = aml_hw->scan_ie.dma_addr;
    } else {
        req->add_ie_len = 0;
        req->add_ies = 0;
    }

    for (i = 0; i < req->chan_cnt; i++) {
        struct ieee80211_channel *chan = request->channels[i];

        req->chan[i].band = chan->band;
        req->chan[i].freq = chan->center_freq;
        req->chan[i].flags = chan_flags | get_chan_flags(chan->flags);
        req->chan[i].tx_power = chan_to_fw_pwr(chan->max_reg_power);
    }

    sched_start_req->n_scan_plans = (u8)min_t(int, MAX_SCHED_SCAN_PLANS,request->n_scan_plans);
    sched_start_req->match_count = (u8)min_t(int, MAX_MATCH_COUNT,request->n_match_sets);

    for (i = 0; i < sched_start_req->n_scan_plans; i++) {
        sched_start_req->scan_plans[i].interval = request->scan_plans[i].interval;
        sched_start_req->scan_plans[i].iterations = request->scan_plans[i].iterations;
    }

     for (i = 0; i < sched_start_req->match_count; i++) {
        sched_start_req->match_sets[i].rssiThreshold = request->match_sets[i].rssi_thold;

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,12,0)
        memcpy(&sched_start_req->match_sets[i].bssid.array[0], request->match_sets[i].bssid, ETH_ALEN);
#endif

        sched_start_req->match_sets[i].ssId.length = request->match_sets[i].ssid.ssid_len;
        memcpy(&sched_start_req->match_sets[i].ssId.array[0],
               request->match_sets[i].ssid.ssid,
               request->match_sets[i].ssid.ssid_len);
    }

    return 0;

}

int aml_set_rekey_data(struct aml_vif *aml_vif, const u8 *kek, const u8 *kck, const u8 *replay_ctr)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_rekey_data_req *rekey_data;

    rekey_data = aml_priv_msg_zalloc(MM_SUB_SET_REKEY_DATA, sizeof(struct set_rekey_data_req));
    if (!rekey_data)
        return -ENOMEM;

    memset((void *)rekey_data, 0, 40);
    memcpy(rekey_data->kek, kek, 16);
    memcpy(rekey_data->kck, kck, 16);
    memcpy(rekey_data->replay_counter, replay_ctr, 8);
    rekey_data->vif_idx = aml_vif->vif_index;

    /* coverity[leaked_storage] - rekey_data will be freed later */
    return aml_priv_send_msg(aml_hw, rekey_data, 0, 0, NULL);
}

int aml_tko_config_req(struct aml_hw *aml_hw, struct aml_vif *vif,
                u16 interval, u16 retry_interval, u16 retry_count)
{
    struct tko_config_req *req;

    req = aml_priv_msg_zalloc(MM_SUB_TKO_CONFIG_REQ, sizeof(struct tko_config_req));
    if (!req)
        return -ENOMEM;

    req->interval = interval;
    req->retry_interval = retry_interval;
    req->retry_count = retry_count;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_set_cali_param_req(struct aml_hw *aml_hw, struct Cali_Param *cali_param)
{
    struct Cali_Param *req;
    req = aml_priv_msg_zalloc(MM_SUB_SET_CALI_REQ, sizeof(struct Cali_Param));

    if (!req)
        return -ENOMEM;

    memcpy(req, cali_param, sizeof(struct Cali_Param));
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_enable_wf(struct aml_vif *aml_vif, u32 wfflag)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct enable_wf_req *req = NULL;
    printk("aml_enable_wf: 0x%08x\n", wfflag);
    req = aml_priv_msg_zalloc(MM_SUB_ENABLE_WF, sizeof(struct enable_wf_req));
    if (!req)
        return -ENOMEM;
    memset((void *)req, 0,sizeof(struct enable_wf_req));
    req->wfflag= wfflag;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_fix_txpwr(struct aml_vif *aml_vif, int pwr)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct fix_txpwr *req = NULL;
    printk("_aml_fix_txpwr: 0x%08x\n", pwr);
    req = aml_priv_msg_zalloc(MM_SUB_FIX_TXPWR, sizeof(struct fix_txpwr));
    if (!req)
        return -ENOMEM;
    memset((void *)req, 0,sizeof(struct fix_txpwr));
    req->pwr= pwr;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_set_rssi_reg(struct aml_vif *aml_vif, int flag)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct enable_rssi_req *req = NULL;
    printk("%s: %x\n", __func__, flag);
    req = aml_priv_msg_zalloc(MM_SUB_ENABLE_RSSI_REG, sizeof(struct enable_rssi_req));
    if (!req)
        return -ENOMEM;
    memset((void *)req, 0,sizeof(struct enable_rssi_req));
    req->flag= flag;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_get_efuse(struct aml_vif *aml_vif, u32 addr)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct get_efuse_req *req = NULL;
    struct efuse_read_result_ind ind;

    memset(&ind, 0, sizeof(struct efuse_read_result_ind));
    req = aml_priv_msg_zalloc(MM_SUB_GET_EFUSE, sizeof(struct get_efuse_req));
    if (!req)
        return -ENOMEM;

    memset((void *)req, 0,sizeof(struct get_efuse_req));
    req->addr = addr;

    /* coverity[leaked_storage] - req will be freed later */
    aml_priv_send_msg(aml_hw, req, 1, PRIV_EFUSE_GET_RESULT, &ind);

    if (ind.addr != addr) {
         printk("get_efuse:0x%x erro!\n", ind.addr);
    }

    return ind.value;
}

int _aml_set_efuse(struct aml_vif *aml_vif, u32 addr, u32 value)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_efuse_req *req = NULL;

    req = aml_priv_msg_zalloc(MM_SUB_SET_EFUSE, sizeof(struct set_efuse_req));
    if (!req)
        return -ENOMEM;

    memset((void *)req, 0,sizeof(struct set_efuse_req));
    req->addr = addr;
    req->value = value;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_fw_reset(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct fw_reset_req *req = NULL;

    req = aml_priv_msg_zalloc(MM_SUB_FW_RESET, sizeof(struct fw_reset_req));
    if (!req)
        return -ENOMEM;

    memset((void *)req, 0, sizeof(struct fw_reset_req));
    req->vif_idx = aml_vif->vif_index;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_set_macbypass(struct aml_vif *aml_vif, int format_type, int bandwidth, int rate, int siso_or_mimo)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_macbypass *macbypass = NULL;

    macbypass = aml_priv_msg_zalloc(MM_SUB_SET_MACBYPASS, sizeof(struct set_macbypass));
    if (!macbypass)
        return -ENOMEM;

    memset((void *)macbypass, 0,sizeof(struct set_macbypass));
    macbypass->format_type = (u8_l)format_type;
    macbypass->bandwidth = (u8_l)bandwidth;
    macbypass->rate = (u8_l)rate;
    macbypass->siso_or_mimo = (u8_l)siso_or_mimo;

    /* coverity[leaked_storage] - macbypass will be freed later */
    return aml_priv_send_msg(aml_hw, macbypass, 0, 0, NULL);
}

int _aml_set_stop_macbypass(struct aml_vif *aml_vif)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_stop_macbypass *stop_macbypass = NULL;

    stop_macbypass = aml_priv_msg_zalloc(MM_SUB_SET_STOP_MACBYPASS, sizeof(struct set_stop_macbypass));
    if (!stop_macbypass)
        return -ENOMEM;

    memset((void *)stop_macbypass, 0,sizeof(struct set_stop_macbypass));
    stop_macbypass->vif_idx = aml_vif->vif_index;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, stop_macbypass, 0, 0, NULL);

}


int aml_send_sched_scan_req(struct aml_vif *aml_vif,
    struct cfg80211_sched_scan_request *request)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct scanu_sched_scan_start_req *sched_start_req;

    sched_start_req =  aml_priv_msg_zalloc(SCANU_SCHED_START_REQ, sizeof(struct scanu_sched_scan_start_req));
    if (!sched_start_req)
        return -ENOMEM;

    if (aml_get_sched_scan_req(aml_vif, request, sched_start_req)) {
        /* coverity[leaked_storage] - sched_start_req have be freed*/
        aml_priv_msg_free(aml_hw, sched_start_req);
        return -ENOMEM;
    }

    /* coverity[leaked_storage] - sched_start_req will be freed later */
    return aml_priv_send_msg(aml_hw, sched_start_req, 0, 0, NULL);
}

int aml_send_sched_scan_stop_req(struct aml_vif *aml_vif, u64 reqid)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct scanu_sched_scan_stop_req *sched_scan_stop_req;

    sched_scan_stop_req =  aml_priv_msg_zalloc(SCANU_SCHED_STOP_REQ, sizeof(struct scanu_sched_scan_stop_req));
    if (!sched_scan_stop_req)
        return -ENOMEM;

    sched_scan_stop_req->vif_idx = aml_vif->vif_index;
    sched_scan_stop_req->reqid = reqid;

    /* coverity[leaked_storage] - sched_start_req will be freed later */
    return aml_priv_send_msg(aml_hw, sched_scan_stop_req, 0, 0, NULL);
}
int _aml_set_amsdu_tx(struct aml_hw *aml_hw, u8 amsdu_tx)
{
    struct set_amsdu_tx_req *req = NULL;
    req = aml_priv_msg_zalloc(MM_SUB_SET_AMSDU_TX, sizeof(struct set_efuse_req));
    if (!req)
      return -ENOMEM;

    memset(req, 0, sizeof(struct set_amsdu_tx_req));
    req->amsdu_tx = amsdu_tx;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_set_tx_lft(struct aml_hw *aml_hw, u32 tx_lft)
{
    struct set_tx_lft_req *req = NULL;
    req = aml_priv_msg_zalloc(MM_SUB_SET_TX_LFT, sizeof(struct set_tx_lft_req));
    if (!req)
        return -ENOMEM;

    memset(req, 0, sizeof(struct set_tx_lft_req));
    req->tx_lft = tx_lft;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}


int aml_set_ldpc_tx(struct aml_hw *aml_hw, struct aml_vif *aml_vif)
{
    struct ldpc_config_req *req;
    struct wiphy *wiphy = aml_hw->wiphy;
    struct ieee80211_sta_ht_cap *ht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->ht_cap;
    struct ieee80211_sta_vht_cap *vht_cap = &wiphy->bands[NL80211_BAND_5GHZ]->vht_cap;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    struct ieee80211_sta_he_cap const *he_cap;
#endif
    uint8_t *ht_mcs = (uint8_t *)&ht_cap->mcs;
    int i;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Build the ME_CONFIG_REQ message */
    req = aml_priv_msg_zalloc(MM_SUB_SET_LDPC, sizeof(struct ldpc_config_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_CONFIG_REQ message */
    req->ht_supp = ht_cap->ht_supported;
    req->vht_supp = vht_cap->vht_supported;
    req->ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap);
    req->ht_cap.a_mpdu_param = ht_cap->ampdu_factor |
                                     (ht_cap->ampdu_density <<
                                         IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
    for (i = 0; i < sizeof(ht_cap->mcs); i++)
        req->ht_cap.mcs_rate[i] = ht_mcs[i];
    req->ht_cap.ht_extended_capa = 0;
    req->ht_cap.tx_beamforming_capa = 0;
    req->ht_cap.asel_capa = 0;

    req->vht_cap.vht_capa_info = cpu_to_le32(vht_cap->cap);
    req->vht_cap.rx_highest = cpu_to_le16(vht_cap->vht_mcs.rx_highest);
    req->vht_cap.rx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.rx_mcs_map);
    req->vht_cap.tx_highest = cpu_to_le16(vht_cap->vht_mcs.tx_highest);
    req->vht_cap.tx_mcs_map = cpu_to_le16(vht_cap->vht_mcs.tx_mcs_map);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0) || (defined CONFIG_KERNEL_AX_PATCH)
    if (wiphy->bands[NL80211_BAND_5GHZ]->iftype_data != NULL) {
        he_cap = &wiphy->bands[NL80211_BAND_5GHZ]->iftype_data->he_cap;

        req->he_supp = he_cap->has_he;
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.mac_cap_info); i++) {
            req->he_cap.mac_cap_info[i] = he_cap->he_cap_elem.mac_cap_info[i];
        }
        for (i = 0; i < ARRAY_SIZE(he_cap->he_cap_elem.phy_cap_info); i++) {
            req->he_cap.phy_cap_info[i] = he_cap->he_cap_elem.phy_cap_info[i];
        }
        req->he_cap.mcs_supp.rx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80);
        req->he_cap.mcs_supp.tx_mcs_80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80);
        req->he_cap.mcs_supp.rx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_160);
        req->he_cap.mcs_supp.tx_mcs_160 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_160);
        req->he_cap.mcs_supp.rx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.rx_mcs_80p80);
        req->he_cap.mcs_supp.tx_mcs_80p80 = cpu_to_le16(he_cap->he_mcs_nss_supp.tx_mcs_80p80);
        for (i = 0; i < MAC_HE_PPE_THRES_MAX_LEN; i++) {
            req->he_cap.ppe_thres[i] = he_cap->ppe_thres[i];
        }
    }
#else
    req->he_supp = false;
#endif
    if (aml_hw->mod_params->use_80)
        req->phy_bw_max = PHY_CHNL_BW_80;
    else if (aml_hw->mod_params->use_2040)
        req->phy_bw_max = PHY_CHNL_BW_40;
    else
        req->phy_bw_max = PHY_CHNL_BW_20;

    req->vif_idx = aml_vif->vif_index;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_set_stbc(struct aml_hw *aml_hw, u8 vif_idx, u8 stbc_on)
{
    struct set_stbc_req *req = NULL;
    req = aml_priv_msg_zalloc(MM_SUB_SET_STBC_ON, sizeof(struct set_stbc_req));

    if (!req)
      return -ENOMEM;

    memset(req, 0, sizeof(struct set_stbc_req));
    req->stbc_on = stbc_on;
    req->vif_idx = vif_idx;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_set_temp_start(struct aml_hw *aml_hw)
{
    void *void_param;
    void_param = aml_priv_msg_zalloc(MM_SUB_SET_TEMP_START, 0);
    if (!void_param)
        return -ENOMEM;
    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, void_param ,0 ,MM_MSG_BYPASS_ID, NULL);
}

int aml_tko_activate(struct aml_hw *aml_hw, struct aml_vif *vif, u8 active)
{
    struct tko_activate_req *req;

    req = aml_priv_msg_zalloc(MM_SUB_TKO_ACTIVATE_REQ, sizeof(struct tko_activate_req));
    if (!req)
        return -ENOMEM;

    req->active = active;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_mdns_set_offload_state(struct aml_hw *aml_hw, int enable)
{
    struct mm_mdns_offload_state *req = NULL;

    req = aml_priv_msg_zalloc(MDNS_SET_STATE, sizeof(struct mm_mdns_offload_state));
    if (!req)
        return -ENOMEM;

    req->enable = enable;

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_mdns_set_passthrough_behavior(struct aml_hw *aml_hw, int behavior)
{
    struct mm_mdns_offload_behavior *req = NULL;

    req = aml_priv_msg_zalloc(MDNS_SET_BEHAVIOR, sizeof(struct mm_mdns_offload_behavior));
    if (!req)
        return -ENOMEM;

    req->behavior = behavior;

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_mdns_reset_all(struct aml_hw *aml_hw)
{
    void *req = NULL;

    req = aml_priv_msg_zalloc(MDNS_RESET_ALL, 0);
    if (!req)
        return -ENOMEM;

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_mdns_add_protocol_data_status(struct aml_hw *aml_hw, void *list_param, mdnsProtocolData *offloadData, int *index)
{
    struct mdns_adddata_cfm cfm;
    int ret;
    struct mm_mdns_add_data_status *req = aml_priv_msg_zalloc(MDNS_ADD_PROTOCOL_STATUS, sizeof(struct mm_mdns_add_data_status));

    if (!req)
        return -ENOMEM;

    MDNS_OFFLOAD_DEBUG("%s ok exit   %d\n", __func__, __LINE__);

    if ((offloadData->matchCriteriaListNum > MDNS_LIST_CRITERIA_MAX)
        || (offloadData->rawOffloadPacketLen > MDNS_RAW_DATA_LENGTH_MAX)) {
        aml_priv_msg_free(aml_hw, req);
        return -ENOMEM;
    }

    req->list_len = offloadData->matchCriteriaListNum;
    req->data_len = offloadData->rawOffloadPacketLen;

    memcpy(req->list_criteria, list_param, req->list_len * sizeof(struct match_criteria));

    ret = aml_priv_send_msg(aml_hw, req, 1, PRIV_MDNS_ADDDATA_CFM, &cfm);
    if (ret != 0)
        return ret;

    if (cfm.state != CO_OK)
        return -EPERM;

    *index = cfm.index;

    return 0;
}

int aml_mdns_add_protocol_data(struct aml_hw *aml_hw, uint8_t *raw_data, uint8_t index, uint16_t data_len)
{

    int ret;
    struct mm_mdns_add_data *req = aml_priv_msg_zalloc(MDNS_ADD_PROTOCOL, sizeof(struct mm_mdns_add_data));

    if (!req)
        return -ENOMEM;
    MDNS_OFFLOAD_DEBUG("%s ok exit   %d\n", __func__, __LINE__);

    req->index = index;
    memcpy(req->raw_offload_packet, raw_data, data_len);
    ret = aml_priv_send_msg(aml_hw, req, 0, 0, NULL);

    return ret;
}


int aml_mdns_remove_protocol_data(struct aml_hw *aml_hw, int index)
{
    struct mm_mdns_remove_data *req = NULL;

    req = aml_priv_msg_zalloc(MDNS_REMOVE_PROTOCOL, sizeof(struct mm_mdns_offload_behavior));
    if (!req)
        return -ENOMEM;

    req->index = index;

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_mdns_get_reset_hit_counter(struct aml_hw *aml_hw, int index)
{
    struct mdns_get_hit_cfm cfm;
    struct mm_mdns_get_hit *req;
    int ret;

    if (index >= MDNS_DATA_MAX)
        return -ENOMEM;

    req = aml_priv_msg_zalloc(MDNS_GET_HIT, sizeof(struct mm_mdns_get_hit));
    if (!req)
        return -ENOMEM;

    req->index = index;

    ret = aml_priv_send_msg(aml_hw, req, 1, PRIV_MDNS_GET_HIT_CFM, &cfm);
    if (ret != 0)
        return ret;

    return cfm.cnt;
}

int aml_mdns_get_reset_miss_counter(struct aml_hw *aml_hw)
{
    struct mdns_get_miss_cfm cfm;
    struct mm_mdns_get_miss *req;
    int ret;

    req = aml_priv_msg_zalloc(MDNS_GET_MISS, sizeof(struct mm_mdns_get_miss));
    if (!req)
        return -ENOMEM;

    //req->index = index;

    ret = aml_priv_send_msg(aml_hw, req, 1, PRIV_MDNS_GET_MISS_CFM, &cfm);
    if (ret != 0)
        return -1;  // need -1 for upper check

    return cfm.cnt;
}

int aml_mdns_add_passthrough_list(struct aml_hw *aml_hw, uint8_t *qname, int length)
{
    struct mm_mdns_passthrough_list *req;
    struct mdns_adddpassthrough_cfm cfm;
    int ret;

    req = aml_priv_msg_zalloc(MDNS_ADD_PASS_LIST, sizeof(struct mm_mdns_passthrough_list));
    if (!req)
        return -ENOMEM;

    // TODO param check
    req->length = length;
    memcpy(req->qname, qname, length);

    ret = aml_priv_send_msg(aml_hw, req, 1, PRIV_MDNS_ADDPASSTHROUGH_CFM, &cfm);
    if (ret != 0)
        return -1;

    MDNS_OFFLOAD_DEBUG("state :%d", cfm.state);

    return cfm.state;
}

int aml_mdns_remove_passthrough_list(struct aml_hw *aml_hw, uint8_t *qname, int length)
{
    struct mm_mdns_passthrough_list *req;

    req = aml_priv_msg_zalloc(MDNS_REMOVE_PASS_LIST, sizeof(struct mm_mdns_passthrough_list));
    if (!req)
        return -ENOMEM;

    // TODO param check
    req->length = length;
    memcpy(req->qname, qname, length);

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}


#ifdef TEST_MODE
int aml_pcie_dl_malloc_test(struct aml_hw *aml_hw, int start_addr, int len, u32_l payload)
{
    u32_l * read_addr;
    struct pcie_dl_req *pcie_dl_param = NULL;

    aml_ipc_buf_alloc(aml_hw, &aml_hw->pcie_prssr_test, len, DMA_TO_DEVICE, NULL);
    memset(aml_hw->pcie_prssr_test.addr, payload , len);
    pcie_dl_param = aml_priv_msg_zalloc(MM_SUB_PCIE_DL_TEST, sizeof(struct pcie_dl_req));
    if (!pcie_dl_param)
        return -ENOMEM;
    pcie_dl_param->dma_addr = aml_hw->pcie_prssr_test.dma_addr;
    pcie_dl_param->start_addr = start_addr;
    pcie_dl_param->len = len - 4;
    read_addr = (u32_l *)aml_hw->pcie_prssr_test.addr;
    pcie_dl_param->payload = *(read_addr);
    printk("%s,%d---dma_addr:%lx, start_addr:%lx \n",__func__,__LINE__,pcie_dl_param->dma_addr,pcie_dl_param->start_addr);
    printk("%s---dma_addr_payload:%x \n",__func__,*(read_addr));
    return aml_priv_send_msg(aml_hw, pcie_dl_param, 0, 0, NULL);
}

int aml_pcie_ul_malloc_test(struct aml_hw *aml_hw, int start_addr, int len, u32_l payload)
{
    struct pcie_ul_req *pcie_ul_param = NULL;

    aml_ipc_buf_alloc(aml_hw, &aml_hw->pcie_prssr_test, len, DMA_FROM_DEVICE, NULL);
    pcie_ul_param = aml_priv_msg_zalloc(MM_SUB_PCIE_UL_TEST, sizeof(struct pcie_ul_req));
    if (!pcie_ul_param)
        return -ENOMEM;
    aml_hw->pcie_prssr_ul_addr = aml_hw->pcie_prssr_test.addr;
    pcie_ul_param->dest_addr = aml_hw->pcie_prssr_test.dma_addr;
    pcie_ul_param->src_addr = start_addr;
    pcie_ul_param->len = len;
    pcie_ul_param->payload = payload;
    printk("%s,%d---dest_addr:%lx, src_addr:%lx",__func__,__LINE__,pcie_ul_param->dest_addr,pcie_ul_param->src_addr);
    printk("%s---dma_addr_payload:%x",__func__, payload);
    return aml_priv_send_msg(aml_hw, pcie_ul_param, 0, 0, NULL);
}

int aml_pcie_prssr_test(struct net_device *dev, int start_addr, int len, u32_l payload)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    //len:
    //bit0: meaning dma direction, 0 is upload, 1 is download;
    //bit1 ~ bit31:meaning dma download or upload length.
    u8_l dir = len & BIT(0);
    len = len >> 1;

    printk("%s,%d, dir: %d, length: %d \n",__func__, __LINE__, dir, len);
    if(dir == 1)
    {
        aml_pcie_dl_malloc_test(aml_hw, start_addr, len, payload);
    }
    else if (dir == 0)
    {
        aml_pcie_ul_malloc_test(aml_hw, start_addr, len, payload);
    }
    return 0;
}
#endif

int aml_coex_cmd(struct net_device *dev, u32_l coex_cmd, u32_l cmd_ctxt_1, u32_l cmd_ctxt_2)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct coex_cmd_req *coex_cmd_param = NULL;

    coex_cmd_param = aml_priv_msg_zalloc(MM_SUB_COEX_CMD, sizeof(struct coex_cmd_req));
    if (!coex_cmd_param)
        return -ENOMEM;
    coex_cmd_param->coex_cmd = coex_cmd;
    coex_cmd_param->cmd_txt_1 = cmd_ctxt_1;
    coex_cmd_param->cmd_txt_2 = cmd_ctxt_2;
    printk("%s,%d, coex_cmd: %d, cmd_ctxt_1:%X, cmd_ctxt_2:%X",__func__, __LINE__, coex_cmd, cmd_ctxt_1, cmd_ctxt_2);
    /* coverity[leaked_storage] - coex_cmd_param will be freed later */
    return aml_priv_send_msg(aml_hw, coex_cmd_param, 0, 0, NULL);
}

int aml_coex_get_status(struct net_device *dev)
{
    void *void_param;
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    void_param = aml_priv_msg_zalloc(MM_SUB_COEX_GET_STATUS, 0);

    if (!void_param)
        return -ENOMEM;

    printk("%s,%d, coex_get_status;",__func__, __LINE__);
    /* coverity[leaked_storage] - coex_cmd_param will be freed later */
    return aml_priv_send_msg(aml_hw, void_param, 0, 0, NULL);
}

int _aml_set_pt_calibration(struct aml_vif *aml_vif, int pt_cali_val)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_pt_calibration *pt_calibration;

    pt_calibration = aml_priv_msg_zalloc(MM_SUB_SET_PT_CALIBRATION, sizeof(struct set_pt_calibration));

    if (!pt_calibration)
        return -ENOMEM;

    memset((void *)pt_calibration, 0,sizeof(struct set_pt_calibration));
    pt_calibration->pt_cali_cfg = (u32_l)pt_cali_val;

    /* coverity[leaked_storage] - pt_calibration will be freed later */
    return aml_priv_send_msg(aml_hw, pt_calibration, 0, 0, NULL);
}

int aml_send_notify_ip(struct aml_vif *aml_vif, u8_l ip_ver, u8_l *ip_addr)
{
    struct aml_hw *aml_hw;
    notify_ip_addr_t *notify_ip_addr;

    if (!aml_vif) {
        printk("aml vif param invalid\n");
        return -EINVAL;
    }
    aml_hw = aml_vif->aml_hw;
    notify_ip_addr = aml_priv_msg_zalloc(MM_SUB_NOTIFY_IP, sizeof(notify_ip_addr_t));
    if (!notify_ip_addr) {
        return -ENOMEM;
    }

    notify_ip_addr->vif_idx = aml_vif->vif_index;
    notify_ip_addr->ip_ver = ip_ver;
    if (ip_ver == IPV4_VER) {
        memcpy(notify_ip_addr->ipv4_addr, ip_addr, IPV4_ADDR_LEN);
    } else if (ip_ver == IPV6_VER) {
        memcpy(notify_ip_addr->ipv6_addr, ip_addr, IPV6_ADDR_LEN);
    }
    /* coverity[leaked_storage] - notify_ip_addr will be freed later */
    return aml_priv_send_msg(aml_hw, notify_ip_addr, 0, 0, NULL);
}

int aml_send_fwlog_cmd(struct aml_vif *aml_vif, int mode)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct Fwlog_Mode_Control *fwlog_param;

    fwlog_param = aml_priv_msg_zalloc(MM_SUB_SEND_FWLOG, sizeof(struct Fwlog_Mode_Control));
    if (!fwlog_param)
        return -ENOMEM;

    fwlog_param->mode = mode;

    /* coverity[leaked_storage] - fwlog_param will be freed later */
    return aml_priv_send_msg(aml_hw, fwlog_param, 0, 0, NULL);
}

int aml_send_scc_conflict_notify(struct aml_vif *ap_vif, u8 sta_vif_idx, struct mm_scc_cfm *scc_cfm)
{
    struct aml_hw *aml_hw = ap_vif->aml_hw;
    scc_conflict_t *scc_conflict;
    scc_conflict =  aml_priv_msg_zalloc(MM_SCC_CONFLICT, sizeof(scc_conflict_t));
    if (!scc_conflict) {
        return -ENOMEM;
    }
    scc_conflict->ap_idx = ap_vif->vif_index;
    scc_conflict->sta_idx = sta_vif_idx;
    return aml_priv_send_msg(aml_hw, scc_conflict, 1, PRIV_SCC_CONFLICT_CFM, scc_cfm);
}

int aml_send_sync_trace(struct aml_hw *aml_hw)
{
    static u32 sync_token = 0;
    sync_trace_t *sync_trace;

    if (sync_token == 0) {
        get_random_bytes(&sync_token, 2);
    }
    sync_trace =  aml_priv_msg_zalloc(MM_SYNC_TRACE, sizeof(sync_trace_t));
    if (!sync_trace) {
        return -ENOMEM;
    }
    AML_INFO("sync token[%d]", sync_token);
    sync_trace->token = sync_token++;
    return aml_priv_send_msg(aml_hw, sync_trace, 0, 0, NULL);
}

int aml_send_dhcp_req(struct aml_hw *aml_hw, struct aml_vif *aml_vif, uint8_t work)
{
    struct mm_dhcpoffload_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    req = aml_priv_msg_zalloc(MM_SUB_DHCP_REQ, sizeof(struct mm_dhcpoffload_req));
    if (!req)
        return -ENOMEM;

    req->dhcp_enable = work;
    req->vif_idx = aml_vif->vif_index;

    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int aml_send_extcapab_req(struct aml_hw *aml_hw)
{
    struct extcapab_req *req;

    AML_DBG(AML_FN_ENTRY_STR);

    req = aml_priv_msg_zalloc(MM_EXT_CAPAB, sizeof(struct extcapab_req));
    if (!req)
        return -ENOMEM;

    memcpy(&req->ext_capab[0], &aml_hw->ext_capa[0], 10);

    return aml_priv_send_msg(aml_hw, req, 0, MM_MSG_BYPASS_ID, NULL);
}

#define AML_SYNC_TRACE_MON_INTERVAL    (60 * HZ)

void aml_sync_trace_cb(struct timer_list *t)
{
    struct aml_hw *aml_hw = from_timer(aml_hw, t, sync_trace_timer);
    enum aml_wq_type type = AML_WQ_SYNC_TRACE;
    struct aml_wq *aml_wq;

    aml_wq = aml_wq_alloc(1);
    if (!aml_wq) {
        AML_INFO("alloc workqueue out of memory");
        return;
    }
    aml_wq->id = AML_WQ_SYNC_TRACE;
    memcpy(aml_wq->data, &type, 1);
    aml_wq_add(aml_hw, aml_wq);
    mod_timer(&aml_hw->sync_trace_timer, jiffies + AML_SYNC_TRACE_MON_INTERVAL);
}

int aml_sync_trace_init(struct aml_hw *aml_hw)
{
    timer_setup(&aml_hw->sync_trace_timer, aml_sync_trace_cb, 0);
    mod_timer(&aml_hw->sync_trace_timer, jiffies + AML_SYNC_TRACE_MON_INTERVAL);
    aml_send_sync_trace(aml_hw);
    return 0;
}

int aml_sync_trace_deinit(struct aml_hw *aml_hw)
{
    del_timer_sync(&aml_hw->sync_trace_timer);
    return 0;
}

int aml_txq_unexpection(struct net_device *dev)
{
    static u32 token = 0;
    struct show_tx_msg_req_t *show_tx_msg_req;
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;

    if (token == 0) {
        get_random_bytes(&token, 2);
    }
    show_tx_msg_req =  aml_priv_msg_zalloc(MM_SUB_SHOW_TX_MSG, sizeof(struct show_tx_msg_req_t));
    if (!show_tx_msg_req) {
        return -ENOMEM;
    }
    AML_INFO("token[%d]", token);
    show_tx_msg_req->token = token++;
    aml_hw->show_switch_info = 50;
    return aml_priv_send_msg(aml_hw, show_tx_msg_req, 0, 0, NULL);
}

int aml_send_set_buf_state_req(struct aml_hw *aml_hw, int buf_state)
{
    int *fw_buf_state = NULL;

    fw_buf_state = aml_priv_msg_zalloc(MM_SUB_SET_DYNAMIC_BUF_STATE, sizeof(int));
    if (!fw_buf_state)
        return -ENOMEM;

    *fw_buf_state = buf_state;

    return aml_priv_send_msg(aml_hw, fw_buf_state, 0, 0, NULL);
}

int _aml_set_la_capture(struct aml_vif *aml_vif, u32 bus1, u32 bus2)
{
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct set_la_capture_req *req = NULL;

    req = aml_priv_msg_zalloc(MM_SUB_SET_LA_CAPTURE, sizeof(struct set_la_capture_req));
    if (!req)
        return -ENOMEM;

    memset((void *)req, 0,sizeof(struct set_la_capture_req));
    req->bus1 = bus1;
    req->bus2 = bus2;

    /* coverity[leaked_storage] - req will be freed later */
    return aml_priv_send_msg(aml_hw, req, 0, 0, NULL);
}

int _aml_set_la_enable(struct aml_hw *aml_hw, int value)
{
    int *la_status = NULL;

    la_status = aml_priv_msg_zalloc(MM_SUB_SET_LA_STATE, sizeof(int));
    if (!la_status)
        return -ENOMEM;

    *la_status = value;

    return aml_priv_send_msg(aml_hw, la_status, 0, 0, NULL);
}

int _aml_set_usb_trace_enable(struct aml_hw *aml_hw, int value)
{
    int *trace_status = NULL;

    trace_status = aml_priv_msg_zalloc(MM_SUB_SET_USB_TRACE_STATE, sizeof(int));
    if (!trace_status)
        return -ENOMEM;

    *trace_status = value;

    return aml_priv_send_msg(aml_hw, trace_status, 0, 0, NULL);
}

int _aml_putv_trace_switch(struct aml_hw *aml_hw, int value)
{
    int *switch_status = NULL;
    switch_status = aml_priv_msg_zalloc(MM_SUB_SET_PUTV_TRACE_SWITCH, sizeof(int));
    if (!switch_status)
        return -ENOMEM;

    *switch_status = value;
    return aml_priv_send_msg(aml_hw, switch_status, 0, 0, NULL);
}
