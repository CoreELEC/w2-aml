/**
 ******************************************************************************
 *
 * @file ipc_host.c
 *
 * @brief IPC module.
 *
 * Copyright (C) Amlogic 2011-2021
 *
 ******************************************************************************
 */

/*
 * INCLUDE FILES
 ******************************************************************************
 */

#define AML_MODULE      GENERIC

#include <linux/spinlock.h>
#include "aml_defs.h"
#include "aml_prof.h"
#include "reg_ipc_app.h"
#include "ipc_host.h"
#include "share_mem_map.h"
#include "ipc_shared.h"
#include "aml_msg_rx.h"
#include "aml_recy.h"
#include "aml_log.h"
#include "aml_msg_tx.h"

/*
 * TYPES DEFINITION
 ******************************************************************************
 */

const int nx_txdesc_cnt[] =
{
    NX_TXDESC_CNT0,
    NX_TXDESC_CNT1,
    NX_TXDESC_CNT2,
    NX_TXDESC_CNT3,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4,
    #endif
};

const int nx_txuser_cnt[] =
{
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    #if NX_TXQ_CNT == 5
    1,
    #endif
};


/*
 * FUNCTIONS DEFINITIONS
 ******************************************************************************
 */
/**
 * ipc_host_rxdesc_handler() - Handle the reception of a Rx Descriptor
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_RXDESC is set
 */
extern struct aml_pm_type g_wifi_pm;
static void ipc_host_rxdesc_handler(struct ipc_host_env_tag *env)
{
    struct aml_hw *aml_hw = env->pthis;
    // For profiling
    REG_SW_SET_PROFILING(env->pthis, SW_PROF_IRQ_E2A_RXDESC);

    /* NB: this function is for PCIe only */
    ipc_host_disable_irq(env, IPC_IRQ_E2A_RXDESC);

    // LMAC has triggered an IT saying that a reception has occurred.
    // Then we first need to check the validity of the current hostbuf, and the validity
    // of the next hostbufs too, because it is likely that several hostbufs have been
    // filled within the time needed for this irq handling
    while (1) {
        #ifdef CONFIG_AML_FULLMAC
        // call the external function to indicate that a RX descriptor is received
        if (aml_pci_rxdataind(env->pthis, env->rxdesc[env->rxdesc_idx]) != 0)
        #else
        // call the external function to indicate that a RX packet is received
        if (aml_pci_rxdataind(env->pthis, env->rxbuf[env->rxbuf_idx]) != 0)
        #endif //(CONFIG_AML_FULLMAC)
            break;
    }
#ifdef CONFIG_AML_NAPI
    if ((aml_hw->napi_enable) && (!skb_queue_empty(&aml_hw->napi_rx_pending_queue))) {
        unsigned long flags;
        spin_lock_irqsave(&aml_hw->napi_rx_upload_queue.lock, flags);
        skb_queue_splice_tail_init(&aml_hw->napi_rx_pending_queue, &aml_hw->napi_rx_upload_queue);
        spin_unlock_irqrestore(&aml_hw->napi_rx_upload_queue.lock, flags);
        napi_schedule(&aml_hw->napi);
    }
#endif
    ipc_host_enable_irq(env,IPC_IRQ_E2A_RXDESC);
    // Signal to the embedded CPU that at least one buffer is available
    ipc_app2emb_trigger_set(env->pthis, IPC_IRQ_A2E_RXBUF_BACK);
    // Signal to the embedded CPU that at least one descriptor is available
    ipc_app2emb_trigger_set(env->pthis, IPC_IRQ_A2E_RXDESC_BACK);

    // For profiling
    REG_SW_CLEAR_PROFILING(env->pthis, SW_PROF_IRQ_E2A_RXDESC);
}

/**
 * ipc_host_radar_handler() - Handle the reception of radar events
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_RADAR is set
 */
static void ipc_host_radar_handler(struct ipc_host_env_tag *env)
{
#ifdef CONFIG_AML_RADAR
    struct aml_hw *aml_hw = env->pthis;
    struct radar_pulse_array_desc *pulses = NULL;

    if (aml_bus_type != PCIE_MODE) {
        pulses = &aml_hw->g_pulses;
    }

    if (aml_bus_type == USB_MODE) {
        aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)pulses,
            (unsigned char *)(unsigned long)(RADAR_EVENT_DESC_ARRAY + (unsigned long)aml_hw->radar_pulse_index * 56),
            sizeof(struct radar_pulse_array_desc), USB_EP4);
    } else if (aml_bus_type == SDIO_MODE) {
        aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)pulses,
            (unsigned char *)(unsigned long)(RADAR_EVENT_DESC_ARRAY + (unsigned long)aml_hw->radar_pulse_index * 56),
            sizeof(struct radar_pulse_array_desc));
    }

    aml_spin_lock(&((struct aml_hw *)env->pthis)->radar.lock);
    while (env->cb.recv_radar_ind(env->pthis,
                                  env->radar[env->radar_idx]) == 0)
        ;
    aml_spin_unlock(&((struct aml_hw *)env->pthis)->radar.lock);
#endif /* CONFIG_AML_RADAR */
}

/**
 * ipc_host_unsup_rx_vec_handler() - Handle the reception of unsupported rx vector
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_UNSUP_RX_VEC is set
 */
static void ipc_host_unsup_rx_vec_handler(struct ipc_host_env_tag *env)
{
    while (env->cb.recv_unsup_rx_vec_ind(env->pthis,
                                         env->unsuprxvec[env->unsuprxvec_idx]) == 0)
        ;
}

static void ipc_host_chan_switch_ind_handler(void *pthis)
{
    struct aml_hw *aml_hw = pthis;
    struct chan_switch_ind_info ind_info = {0};
    struct ipc_e2a_msg msg;
    struct mm_channel_pre_switch_ind *pre_ind = NULL;
    struct mm_channel_switch_ind *ind = NULL;

    BUG_ON(aml_bus_type == PCIE_MODE);
    hi_sram_read(aml_hw, &ind_info, CHAN_SWITCH_IND_MSG_ADDR, sizeof(struct chan_switch_ind_info));

    switch (ind_info.msg_id) {
    case MM_CHANNEL_PRE_SWITCH_IND:
        msg.id = MM_CHANNEL_PRE_SWITCH_IND;
        pre_ind = (struct mm_channel_pre_switch_ind *)msg.param;
        pre_ind->chan_index = ind_info.chan_index;
        break;
    case MM_CHANNEL_SWITCH_IND:
        msg.id = MM_CHANNEL_SWITCH_IND;
        ind = (struct mm_channel_switch_ind *)msg.param;
        ind->chan_index = ind_info.chan_index;
        ind->roc = ind_info.roc;
        ind->roc_tdls = ind_info.roc_tdls;
        ind->vif_index = ind_info.vif_index;
        break;
    default:
        return;
    }
    aml_rx_sdio_ind_msg_handle(aml_hw, &msg);
}
/**
 * ipc_host_msg_handler() - Handler for firmware message
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_MSG is set
 */
static void ipc_host_msg_handler(struct ipc_host_env_tag *env)
{
    if (aml_bus_type != PCIE_MODE) {
        env->cb.recv_msg_ind(env->pthis, env->msgbuf[env->msgbuf_idx]);
    } else {
        while (env->cb.recv_msg_ind(env->pthis, env->msgbuf[env->msgbuf_idx]) == 0) {
            ;
        }
    }
}

static void ipc_usb_host_msgack_handler(struct ipc_host_env_tag *env)
{
    void *hostid = env->msga2e_hostid;
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
    volatile struct ipc_a2e_msg msg_a2e_buf = {0};

    aml_hw->plat->hif_ops->hi_read_sram((unsigned char *)&msg_a2e_buf, (unsigned char *)&env->shared->msg_a2e_buf, sizeof(struct ipc_a2e_msg), USB_EP4);

    ASSERT_ERR(hostid);
    ASSERT_ERR(env->msga2e_cnt == (((struct lmac_msg *)(&msg_a2e_buf.msg))->src_id & 0xFF));
    if (!hostid) {
        AML_INFO("error: a2e msg hostid is null\n");
        return;
    }

    env->msga2e_hostid = NULL;
    env->msga2e_cnt++;
    env->cb.recv_msgack_ind(env->pthis, hostid);
}

static void ipc_sdio_host_msgack_handler(struct ipc_host_env_tag *env)
{
    void *hostid = env->msga2e_hostid;
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
    volatile struct ipc_a2e_msg msg_a2e_buf = {0};
    aml_hw->plat->hif_sdio_ops->hi_random_ram_read((unsigned char *)&msg_a2e_buf, (unsigned char *)&env->shared->msg_a2e_buf, sizeof(struct ipc_a2e_msg));
    ASSERT_ERR(hostid);
    ASSERT_ERR(env->msga2e_cnt == (((struct lmac_msg *)(&msg_a2e_buf.msg))->src_id & 0xFF));
    if (!hostid) {
        AML_INFO("error: a2e msg hostid is null\n");
        return;
    }
    env->msga2e_hostid = NULL;
    env->msga2e_cnt++;
    env->cb.recv_msgack_ind(env->pthis, hostid);
}

/**
 * ipc_host_msgack_handler() - Handle the reception of message acknowledgement
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_MSG_ACK is set
 */
static void ipc_pci_host_msgack_handler(struct ipc_host_env_tag *env)
{
    void *hostid = env->msga2e_hostid;

    if (!hostid) {
        struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
        struct aml_vif *aml_vif = aml_hw->vif_table[0];
        AML_INFO("error: a2e msg hostid is null\n");
        if (aml_vif != NULL) {
            struct net_device * dev = aml_hw->vif_table[0]->ndev;
            AML_INFO("0x6080001c 0x%x\n", aml_read_reg(dev, 0x6080001c));
            AML_INFO("0x60800100 0x%x\n", aml_read_reg(dev, 0x60800100));
            AML_INFO("0x6080000c 0x%x\n", aml_read_reg(dev, 0x6080000c));
            AML_INFO("0x60000004 0x%x\n", aml_read_reg(dev, 0x60000004));
        }
        return;
    }

    env->msga2e_hostid = NULL;
    env->msga2e_cnt++;
    env->cb.recv_msgack_ind(env->pthis, hostid);
}

static void ipc_host_msgack_handler(struct ipc_host_env_tag *env)
{
    if (aml_bus_type == USB_MODE) {
        ipc_usb_host_msgack_handler(env);
    } else if (aml_bus_type == SDIO_MODE) {
        ipc_sdio_host_msgack_handler(env);
    } else {
        ipc_pci_host_msgack_handler(env);
    }
}

/**
 * ipc_host_dbg_handler() - Handle the reception of Debug event
 *
 * @env: pointer to the IPC Host environment
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_DBG is set
 */
static void ipc_host_dbg_handler(struct ipc_host_env_tag *env)
{
    if (aml_bus_type != PCIE_MODE) {
       env->cb.recv_dbg_ind(env->pthis, env->dbgbuf[env->dbgbuf_idx]);
    } else {
        while(env->cb.recv_dbg_ind(env->pthis,
            env->dbgbuf[env->dbgbuf_idx]) == 0);
    }
}

static void ipc_host_trace_handler(struct aml_hw *aml_hw)
{
    if (aml_bus_type != PCIE_MODE) {
       aml_traceind(aml_hw);
    }
}

/**
 * ipc_host_tx_cfm_handler() - Handle the reception of TX confirmation
 *
 * @env: pointer to the IPC Host environment
 * @queue_idx: index of the hardware on which the confirmation has been received
 * @user_pos: index of the user position
 *
 * Called from general IRQ handler when status %IPC_IRQ_E2A_TXCFM is set.
 * Process confirmations in order until:
 * - There is no more buffer pushed (no need to check confirmation in this case)
 * - The confirmation has not been updated by firmware
 */
static void ipc_host_tx_cfm_handler(struct ipc_host_env_tag *env,
                                    const int queue_idx, const int user_pos)
{
    while (!list_empty(&env->tx_hostid_pushed)) {
        if (env->cb.send_data_cfm(env->pthis, env->txcfm[env->txcfm_idx]))
            break;
        env->txcfm_idx++;
        if (env->txcfm_idx == IPC_TXCFM_CNT)
            env->txcfm_idx = 0;
    }
}

static void ipc_host_tx_cfm_handler_pcie(struct ipc_host_env_tag *env, uint32_t status)
{
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
    int i;

    aml_spin_lock(&aml_hw->tx_lock);
    for (i = 0; i < IPC_TXQUEUE_CNT; i++) {
        int j = 0;

#ifdef CONFIG_AML_MUMIMO_TX
        for (; j < nx_txuser_cnt[i]; j++)
#endif
        {
            uint32_t q_bit = BIT(j + i * CONFIG_USER_MAX + IPC_IRQ_E2A_TXCFM_POS);

            if (status & q_bit)
                ipc_host_tx_cfm_handler(env, i, j);
        }
    }
    aml_spin_unlock(&aml_hw->tx_lock);
}

/**
 ******************************************************************************
 */
bool ipc_host_tx_frames_pending(struct ipc_host_env_tag *env)
{
    return !list_empty(&env->tx_hostid_pushed);
}

/**
 ******************************************************************************
 */
void *ipc_host_tx_flush(struct ipc_host_env_tag *env)
{
    struct ipc_hostid *tx_hostid;
    tx_hostid = list_first_entry_or_null(&env->tx_hostid_pushed,
                                         struct ipc_hostid, list);

    if (!tx_hostid)
        return NULL;

    list_del(&tx_hostid->list);
    list_add_tail(&tx_hostid->list, &env->tx_hostid_available);
    return tx_hostid->hostptr;
}

struct ipc_shared_rx_buf *g_host_rxbuf = NULL;
static void ipc_host_rxbuf_ext_init(struct ipc_shared_rx_buf *shared_host_rxbuf)
{
    unsigned int i, size, *dst;

    if (!shared_host_rxbuf)
        return;

    size = (unsigned int)sizeof(struct ipc_shared_rx_buf) * (IPC_RXBUF_CNT_EXT + IPC_RXBUF_CNT);
    dst = (unsigned int *)shared_host_rxbuf;

    for (i = 0; i < size; i += 4) {
        aml_pci_writel(0, (u8 *)dst);
        dst++;
    }
    g_host_rxbuf = (struct ipc_shared_rx_buf *)shared_host_rxbuf;
}

struct ipc_shared_rx_desc *g_host_rxdesc = NULL;
static void ipc_host_rxdesc_ext_init(struct ipc_shared_rx_desc *shared_host_rxdesc)
{
    unsigned int i, size, *dst;

    if (!shared_host_rxdesc)
        return;

    size = (unsigned int)sizeof(struct ipc_shared_rx_desc) * IPC_RXDESC_CNT_EXT;
    dst = (unsigned int *)shared_host_rxdesc;

    for (i = 0; i < size; i += 4) {
        aml_pci_writel(0, (u8 *)dst);
        dst++;
    }
    g_host_rxdesc = (struct ipc_shared_rx_desc *)shared_host_rxdesc;
}


/**
 ******************************************************************************
 */
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  struct ipc_shared_env_tag *shared_env_ptr,
                  struct ipc_shared_rx_buf *shared_host_rxbuf,
                  struct ipc_shared_rx_desc *shared_host_rxdesc,
                  void *pthis)
{
    unsigned int i;
    struct ipc_hostid *tx_hostid;
    struct aml_hw *aml_hw = NULL;

    // Reset the environments
#if 0
    /* check potential platform bug on multiple stores */
    memset(shared_env_ptr, 0, sizeof(struct ipc_shared_env_tag));
#else
    // Reset the IPC Shared memory
    if (aml_bus_type == PCIE_MODE) {
        unsigned int size = (unsigned int)sizeof(struct ipc_shared_env_tag);
        unsigned int *dst = (unsigned int *)shared_env_ptr;
        for (i=0; i < size; i+=4) {
            aml_pci_writel(0, (u8 *)dst++);
        }
    }
#endif

    ipc_host_rxbuf_ext_init(shared_host_rxbuf);
    ipc_host_rxdesc_ext_init(shared_host_rxdesc);

    // Reset the IPC Host environment
    memset(env, 0, sizeof(struct ipc_host_env_tag));

    // Initialize the shared environment pointer
    env->shared = shared_env_ptr;

    // Save the callbacks in our own environment
    env->cb = *cb;

    // Save the pointer to the register base
    env->pthis = pthis;

    // Initialize buffers numbers and buffers sizes needed for DMA Receptions
    env->rxbuf_nb = (aml_bus_type == PCIE_MODE) ? (IPC_RXBUF_CNT + IPC_RXBUF_CNT_EXT) : IPC_RXBUF_CNT;
#ifdef CONFIG_AML_FULLMAC
    env->rxdesc_nb = (aml_bus_type == PCIE_MODE) ? (IPC_RXDESC_CNT + IPC_RXDESC_CNT_EXT) : IPC_RXDESC_CNT;
#endif //(CONFIG_AML_FULLMAC)
    env->unsuprxvec_sz = max(sizeof(struct rx_vector_desc), (size_t) RADIOTAP_HDR_MAX_LEN) +
        RADIOTAP_HDR_VEND_MAX_LEN +  UNSUP_RX_VEC_DATA_LEN;

    // Initialize the pointer to the TX DMA descriptor arrays
    env->txdmadesc = shared_env_ptr->txdmadesc;

    INIT_LIST_HEAD(&env->tx_hostid_available);
    INIT_LIST_HEAD(&env->tx_hostid_pushed);
    aml_hw = (struct aml_hw *)env->pthis;
    if (aml_bus_type == PCIE_MODE) {
        tx_hostid = env->tx_hostid;

        /* start from 1 (host id can't be 0) */
        for (i = 1; i <= ARRAY_SIZE(env->tx_hostid); i++, tx_hostid++) {
            tx_hostid->hostid = i;
            list_add_tail(&tx_hostid->list, &env->tx_hostid_available);
        }
    } else {
        tx_hostid = env->tx_hostid_sdio_usb;

        /* start from 1 (host id can't be 0) */
        for (i = 1; i <= ARRAY_SIZE(env->tx_hostid_sdio_usb); i++, tx_hostid++) {
            tx_hostid->hostid = aml_compact_tx_host_id(i, i);
            list_add_tail(&tx_hostid->list, &env->tx_hostid_available);
        }
        aml_hw->hostid_prefix = ARRAY_SIZE(env->tx_hostid_sdio_usb) + 1;
    }
}

/**
 ******************************************************************************
 */
void ipc_host_pattern_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_pattern_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
        }
    }
    env->shared->pattern_addr = buf->dma_addr;
}

/**
 ******************************************************************************
 */
#ifdef DEBUG_CODE
extern uint32_t addr_null_happen;
struct debug_push_rxbuff_info debug_push_rxbuff[DEBUG_RX_BUF_CNT];
u16 debug_push_rxbuff_idx = 0;
static void record_push_rx_buf(u32 dma_addr,u32 host_id, u16 rxbuf_idx)
{
    debug_push_rxbuff[debug_push_rxbuff_idx].addr = dma_addr;
    debug_push_rxbuff[debug_push_rxbuff_idx].idx = rxbuf_idx;
    debug_push_rxbuff[debug_push_rxbuff_idx].hostid = host_id;
    debug_push_rxbuff[debug_push_rxbuff_idx].time = jiffies;
    debug_push_rxbuff_idx++;
    if (debug_push_rxbuff_idx == DEBUG_RX_BUF_CNT) {
        debug_push_rxbuff_idx = 0;
    }
}
#endif

int ipc_host_rxbuf_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_rx_buf *host_rxbuf;
    unsigned int hostid = AML_RXBUFF_HOSTID_GET(buf);

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_pattern_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    if (aml_bus_type == PCIE_MODE) {
        host_rxbuf = g_host_rxbuf + env->rxbuf_idx;
    } else {
        host_rxbuf = (struct ipc_shared_rx_buf *)&env->shared->host_rxbuf[env->rxbuf_idx];
    }

    host_rxbuf->hostid = hostid;
    host_rxbuf->dma_addr = buf->dma_addr;
    if ((hostid == 0) || (hostid > AML_RXBUFF_MAX)) {
        AML_INFO("hostid invalid:%x", hostid);
    }

#ifdef DEBUG_CODE
    if (addr_null_happen) {
        AML_INFO("push rxbuf, idx:%d, host_id:0x%x, buf:%p, addr:%p, dma_addr:%px\n",
                 env->rxbuf_idx, host_rxbuf->hostid, buf, buf->addr, (void *)buf->dma_addr);
    }

    if (aml_bus_type == PCIE_MODE) {
        record_push_rx_buf(buf->dma_addr, hostid, env->rxbuf_idx);
    }
#endif

    // Signal to the embedded CPU that at least one buffer is available
    //ipc_app2emb_trigger_set(env->pthis, IPC_IRQ_A2E_RXBUF_BACK);

    // Increment the array index
    if (aml_bus_type == PCIE_MODE) {
        env->rxbuf_idx = (env->rxbuf_idx + 1) % (IPC_RXBUF_CNT + IPC_RXBUF_CNT_EXT);
    } else {
        env->rxbuf_idx = (env->rxbuf_idx + 1) % IPC_RXBUF_CNT;
    }

    return 0;
}

#ifdef CONFIG_AML_FULLMAC
#ifdef DEBUG_CODE
struct debug_push_rxdesc_info debug_push_rxdesc[DEBUG_RX_BUF_CNT];
u16 debug_push_rxdesc_idx = 0;
static void record_push_rx_desc(u32 dma_addr,u16 rxdesc_idx)
{
    debug_push_rxdesc[debug_push_rxdesc_idx].addr = dma_addr;
    debug_push_rxdesc[debug_push_rxdesc_idx].idx = rxdesc_idx;
    debug_push_rxdesc[debug_push_rxdesc_idx].time = jiffies;
    debug_push_rxdesc_idx++;
    if (debug_push_rxdesc_idx == DEBUG_RX_BUF_CNT) {
        debug_push_rxdesc_idx = 0;
    }
}
#endif

/**
 ******************************************************************************
 */
int ipc_host_rxdesc_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;
    struct ipc_shared_rx_desc *host_rxdesc;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_rxdesc_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    if (env->rxdesc_idx < IPC_RXDESC_CNT) {
        host_rxdesc = (struct ipc_shared_rx_desc *)&shared_env->host_rxdesc[env->rxdesc_idx];
    } else if (g_host_rxdesc && (env->rxdesc_idx < (IPC_RXDESC_CNT + IPC_RXDESC_CNT_EXT))) {
        host_rxdesc = g_host_rxdesc + (env->rxdesc_idx - IPC_RXDESC_CNT);
    } else {
        AML_INFO("host push rxdesc idx is illegal");
        return -1;
    }

    host_rxdesc->dma_addr = buf->dma_addr;

    env->rxdesc[env->rxdesc_idx] = buf;
#ifdef DEBUG_CODE
    if (aml_bus_type == PCIE_MODE) {
        record_push_rx_desc(buf->dma_addr, env->rxdesc_idx);
    }
#endif
    // Signal to the embedded CPU that at least one descriptor is available
    //ipc_app2emb_trigger_set(env->pthis, IPC_IRQ_A2E_RXDESC_BACK);
    if (aml_bus_type == PCIE_MODE) {
        env->rxdesc_idx = (env->rxdesc_idx + 1) % (IPC_RXDESC_CNT + IPC_RXDESC_CNT_EXT);
    } else {
        env->rxdesc_idx = (env->rxdesc_idx + 1) % IPC_RXDESC_CNT;
    }

    return 0;
}
#endif /* CONFIG_AML_FULLMAC */

/**
 ******************************************************************************
 */
int ipc_host_radar_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_radar_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }
    // Copy the DMA address in the ipc shared memory
    shared_env->radarbuf_hostbuf[env->radar_idx] = buf->dma_addr;

    // Save Ipc buffer in host env
    env->radar[env->radar_idx] = buf;

    // Increment the array index
    env->radar_idx = (env->radar_idx + 1) % IPC_RADARBUF_CNT;

    return 0;
}

/**
 ******************************************************************************
 */
int ipc_host_unsuprxvec_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env_ptr = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_unsuprxvec_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    shared_env_ptr->unsuprxvecbuf_hostbuf[env->unsuprxvec_idx] = buf->dma_addr;

    env->unsuprxvec[env->unsuprxvec_idx] = buf;

    env->unsuprxvec_idx = (env->unsuprxvec_idx + 1) % IPC_UNSUPRXVECBUF_CNT;

    return 0;
}

struct debug_push_msginfo debug_push_msgbug[DEBUG_MSGE2A_BUF_CNT];
u8 debug_push_idx = 0;
static void record_push_msg_buf(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;
    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("record_push_msg_buf,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return;
        }
    }
    debug_push_msgbug[debug_push_idx].addr = buf->dma_addr;
    debug_push_msgbug[debug_push_idx].next_addr = shared_env->msg_e2a_hostbuf_addr[(env->msgbuf_idx + 1) % IPC_MSGE2A_BUF_CNT];
    debug_push_msgbug[debug_push_idx].idx = env->msgbuf_idx;
    debug_push_msgbug[debug_push_idx].time = jiffies;
    debug_push_idx++;
    if (debug_push_idx == DEBUG_MSGE2A_BUF_CNT) {
        debug_push_idx = 0;
    }
}

/**
 ******************************************************************************
 */
int ipc_host_msgbuf_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_msgbuf_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    shared_env->msg_e2a_hostbuf_addr[env->msgbuf_idx] = buf->dma_addr;
    env->msgbuf[env->msgbuf_idx] = buf;
    if (shared_env->msg_e2a_hostbuf_addr[env->msgbuf_idx] != buf->dma_addr) {
        AML_ERR("error:msg_e2a_hostbuf_addr=0x%x, dma_addr=%px, msgbuf_idx=%d\n",
                shared_env->msg_e2a_hostbuf_addr[env->msgbuf_idx], (void *)buf->dma_addr, env->msgbuf_idx);
    }
    record_push_msg_buf(env,buf);
    env->msgbuf_idx = (env->msgbuf_idx + 1) % IPC_MSGE2A_BUF_CNT;

    return 0;
}

/**
 ******************************************************************************
 */
int ipc_host_dbgbuf_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_dbgbuf_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    shared_env->dbg_hostbuf_addr[env->dbgbuf_idx] = buf->dma_addr;

    env->dbgbuf[env->dbgbuf_idx] = buf;

    env->dbgbuf_idx = (env->dbgbuf_idx + 1) % IPC_DBGBUF_CNT;

    return 0;
}

/**
 ******************************************************************************
 */
int ipc_host_txcfm_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_txcfm_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    shared_env->txcfm_hostbuf_addr[env->txcfm_idx] = buf->dma_addr;
    env->txcfm[env->txcfm_idx] = buf;

    env->txcfm_idx++;
    if (env->txcfm_idx == IPC_TXCFM_CNT)
        env->txcfm_idx = 0;

    return 0;
}

/**
 ******************************************************************************
 */
void ipc_host_dbginfo_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    struct ipc_shared_env_tag *shared_env = env->shared;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_dbginfo_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
        }
        return;
    }

    shared_env->la_dbginfo_addr = buf->dma_addr;
}

/**
 ******************************************************************************
 */
#define TXDESC_TRIGGER_THS 18
#define TXDESC_DYNAMIC_THS 6
struct aml_txdesc_trigger g_txdesc_trigger;
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, struct aml_ipc_buf *buf)
{
    if (aml_bus_type == PCIE_MODE) {
        uint32_t dma_idx = env->txdmadesc_idx;
        volatile struct dma_desc *dmadesc_pushed;

        dmadesc_pushed = &env->txdmadesc[dma_idx++];

        // Write DMA address to the descriptor
        dmadesc_pushed->src = buf->dma_addr;

        wmb();

        if (dma_idx == IPC_TXDMA_DESC_CNT)
            env->txdmadesc_idx = 0;
        else
            env->txdmadesc_idx = dma_idx;
    }

    // trigger interrupt to firmware
    //ipc_app2emb_trigger_setf(env->pthis, IPC_IRQ_A2E_TXDESC);
    if (g_txdesc_trigger.ths_enable > 0) {
        if (++g_txdesc_trigger.txdesc_cnt > g_txdesc_trigger.tx_pcie_ths) {
            // trigger interrupt to firmware
            ipc_app2emb_trigger_setf(env->pthis, IPC_IRQ_A2E_TXDESC);
            g_txdesc_trigger.txdesc_cnt = 0;
            g_txdesc_trigger.dynamic_cnt++;
            if (g_txdesc_trigger.dynamic_cnt == TXDESC_DYNAMIC_THS) {
                g_txdesc_trigger.tx_pcie_ths++;
                g_txdesc_trigger.dynamic_cnt = 0;
            }
            if (g_txdesc_trigger.tx_pcie_ths > TXDESC_TRIGGER_THS)
                g_txdesc_trigger.tx_pcie_ths = TXDESC_TRIGGER_THS;
        }
    } else {
        ipc_app2emb_trigger_setf(env->pthis, IPC_IRQ_A2E_TXDESC);
    }
}

/**
 * ipc_host_tx_host_ptr_to_id() - Save and convert host pointer to host id
 *
 * @env: pointer to the IPC Host environment
 * @host_ptr: host pointer to save in the ipc_hostid element ()
 * @return: uint32_t value associated to this host buffer.
 *
 * Move a free ipc_hostid from the tx_hostid_available list to the tx_hostid_pushed list.
 * The element is initialized with the host pointer and the associated 32bits value is
 * returned.
 * It is expected that list tx_hostid_available contains at least one element.
 */
uint32_t ipc_host_tx_host_ptr_to_id(struct ipc_host_env_tag *env, void *host_ptr)
{
    struct ipc_hostid *tx_hostid = list_first_entry_or_null(&env->tx_hostid_available,
                                                            struct ipc_hostid, list);

    if (!tx_hostid)
        return 0;

    list_del(&tx_hostid->list);
    list_add_tail(&tx_hostid->list, &env->tx_hostid_pushed);
    tx_hostid->hostptr = host_ptr;
    return tx_hostid->hostid;
}

/**
 * ipc_host_tx_host_id_to_ptr() - Retrieve host ptr from host id
 *
 * @env: pointer to the IPC Host environment
 * @hostid: hostid present in the confirmation
 * @return: pointer saved via ipc_host_tx_host_ptr_to_id()
 *
 * Allow to retrieve the host ptr (to the tx buffer) form the host id found in
 * the confirmation.
 * Move back the tx_hostid element from the tx_hostid_pushed list to the
 * tx_hostid_available list.
 */
void *ipc_host_tx_host_id_to_ptr(struct ipc_host_env_tag *env, uint32_t hostid)
{
    struct ipc_hostid *tx_hostid;

    if (unlikely(!hostid || (hostid > ARRAY_SIZE(env->tx_hostid))))
        return NULL;

    tx_hostid = &env->tx_hostid[hostid - 1];
    BUG_ON(aml_bus_type != PCIE_MODE);
    list_del(&tx_hostid->list);
    list_add_tail(&tx_hostid->list, &env->tx_hostid_available);
    return tx_hostid->hostptr;
}

void *ipc_host_tx_host_id_to_ptr_for_sdio_usb(struct ipc_host_env_tag *env, uint32_t hostid)
{
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
    struct ipc_hostid *tx_hostid = NULL;

    if (unlikely(!hostid || ((hostid & 0xffff) > ARRAY_SIZE(env->tx_hostid_sdio_usb))))
        return NULL;

    list_for_each_entry(tx_hostid, &env->tx_hostid_pushed, list) {
        if (tx_hostid->hostid == hostid) {
            list_del(&tx_hostid->list);
            tx_hostid->hostid = aml_compact_tx_host_id(++aml_hw->hostid_prefix, tx_hostid->hostid);
            list_add_tail(&tx_hostid->list, &env->tx_hostid_available);
            return tx_hostid->hostptr;
        }
    }
    return NULL;
}

static void aml_store_excep_info(struct aml_hw *aml_hw)
{
    struct exception_info excep_info = {0};
    char *fbuf;
    uint32_t sp_ctx[64] = {0};
    int res = 0;
    int size = 2048;
    int i = 0;

    fbuf = kzalloc(2048, GFP_KERNEL);
    if (!fbuf) {
        AML_INFO("fbuf alloc fail");
        return;
    }
    if (aml_bus_type == SDIO_MODE) {
        hi_sram_read(aml_hw, &excep_info, EXCEPTION_INFO_ADDR, sizeof(excep_info));
        hi_sram_read(aml_hw, sp_ctx, excep_info.sp, sizeof(sp_ctx));
    } else {
        AML_INFO("buf_type err, not support!");
        kfree(fbuf);
        return;
    }

    res = scnprintf(fbuf, size,
        "exception info:\n"
        "type                %08x\n"
        "mstatus_mps_bits    %08x\n"
        "mepc                %08x\n"
        "mtval               %08x\n"
        "mcause              %08x\n"
        "sp                  %08x\n"
        "fw_pc               %08x\n"
        "Stack trace:\n",
        excep_info.type, excep_info.mstatus_mps_bits, excep_info.mepc, excep_info.mtval,
        excep_info.mcause, excep_info.sp, AML_REG_READ(aml_hw->plat, AML_ADDR_MAC_PHY, AML_FW_PC_POINTER));

    for (i = 0; i < 64; i++) {
        res += scnprintf(&fbuf[res], size - res, "0x%08x\n", sp_ctx[i]);
    }

    AML_INFO("\n%s", fbuf);
    aml_send_err_info_to_diag(fbuf, strlen(fbuf));
    kfree(fbuf);
}

void aml_sdio_usb_extend_irq_handle(struct aml_hw *aml_hw)
{
    uint32_t status = AML_REG_READ(aml_hw->plat, 0, SDIO_USB_EXTEND_E2A_IRQ_STATUS);
    switch (status)
    {
        case DYNAMIC_BUF_HOST_TX_STOP:
            aml_hw->dynabuf_stop_tx = DYNAMIC_BUF_HOST_TX_STOP;
            aml_hw->send_tx_stop_to_fw = 1;
            AML_INFO("FW->host tx stop, aml_hw->dynabuf_stop_tx:%d\n", aml_hw->dynabuf_stop_tx);
            up(&aml_hw->aml_tx_sem);
            break;
        case DYNAMIC_BUF_HOST_TX_START:
            aml_hw->dynabuf_stop_tx = 0;
            AML_INFO("FW->host tx start, aml_hw->dynabuf_stop_tx:%d\n", aml_hw->dynabuf_stop_tx);
            up(&aml_hw->aml_tx_sem);
            break;
        case DYNAMIC_BUF_LA_SWITCH_FINISH:
            AML_INFO("la page had been released completely!\n");
            break;
        case DYNAMIC_BUF_TRACE_EXPEND_FINISH:
            AML_INFO("USB TRACE READY!\n");
            aml_hw->trace_malloc_success = 1;
            break;
        case DYNAMIC_BUF_TRACE_REDUCE_FINISH:
            AML_INFO("USB TRACE REDUCE!\n");
            aml_hw->trace_malloc_success = 0;
            break;
        case EXCEPTION_IRQ:
            if (aml_bus_type == SDIO_MODE) {
                AML_ERR("firmware exception!\n");
                aml_store_excep_info(aml_hw);
            }
            break;
        default:
            break;
    }
}

/**
 ******************************************************************************
 */
void ipc_host_irq(struct ipc_host_env_tag *env, uint32_t status)
{
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;

    // Acknowledge the pending interrupts
    if (aml_bus_type == PCIE_MODE)
        ipc_emb2app_ack_clear(env->pthis, status);

    AML_PROF_HI(ipc);
    // Optimized for only one IRQ at a time
    if (status & IPC_IRQ_E2A_RXDESC) {
        BUG_ON(aml_bus_type != PCIE_MODE);
        // handle the RX descriptor reception
#ifdef CONFIG_AML_USE_TASK
        aml_task_schedule(&aml_hw->pcie.task_rxdesc);
#else
        ipc_host_rxdesc_handler(env);
#endif
    }
    if (status & IPC_IRQ_E2A_MSG_ACK)
    {
        ipc_host_msgack_handler(env);
    }
    if (((status & IPC_IRQ_E2A_MSG) && (aml_bus_type == PCIE_MODE))
        || ((status & SDIO_IRQ_E2A_MSG) && (aml_bus_type != PCIE_MODE)))
    {
        ipc_host_msg_handler(env);
    }
    if (status & IPC_IRQ_E2A_TXCFM) {
        BUG_ON(aml_bus_type != PCIE_MODE);
        // handle the TX confirmation reception
        ipc_host_tx_cfm_handler_pcie(env, status);
#ifdef CONFIG_AML_POWER_SAVE_MODE
        aml_allow_fw_sleep(aml_hw->plat, PS_TX_START);
#endif
    }

    if (status & IPC_IRQ_E2A_RADAR)
    {
        // handle the radar event reception
        ipc_host_radar_handler(env);
    }

    if ((status & IPC_IRQ_E2A_UNSUP_RX_VEC) && (aml_bus_type == PCIE_MODE))
    {
        // handle the unsupported rx vector reception
        ipc_host_unsup_rx_vec_handler(env);
    }
    if (status & SDIO_IRQ_E2A_CHAN_SWITCH_IND_MSG)
    {
        ipc_host_chan_switch_ind_handler(env->pthis);
    }

    if (((status & IPC_IRQ_E2A_DBG) && (aml_bus_type == PCIE_MODE))
        || ((status & SDIO_IRQ_E2A_DBG) && (aml_bus_type != PCIE_MODE)))
    {
        ipc_host_dbg_handler(env);
    }

    if (((status & SDIO_IRQ_E2A_TRACE) && (aml_bus_type != PCIE_MODE)))
    {
        ipc_host_trace_handler((struct aml_hw *)env->pthis);
    }
    if ((status & SDIO_USB_EXTEND_E2A_IRQ) && (aml_bus_type != PCIE_MODE))
    {
        aml_sdio_usb_extend_irq_handle(aml_hw);
    }
    AML_PROF_LO(ipc);
}

/**
 ******************************************************************************
 */
int ipc_host_msg_push(struct ipc_host_env_tag *env, void *msg_buf, uint16_t len)
{
    uint8_t *src;
    int i; uint8_t *dst;
    struct aml_hw *aml_hw = (struct aml_hw *)env->pthis;
    struct aml_cmd *cmd = (struct aml_cmd *)msg_buf;
    struct lmac_msg *msg = cmd->a2e_msg;
    bool is_suspend_resume_msg;

    if (!msg)
        return -1;

    if (aml_bus_type == PCIE_MODE) {
        if (atomic_read(&g_wifi_pm.bus_suspend_cnt) || g_pci_shutdown) {
            AML_ERR("ipc_host_msg_push,bus_suspend_cnt = %x, g_pci_shutdown = %x \n", atomic_read(&g_wifi_pm.bus_suspend_cnt), g_pci_shutdown);
            return -1;
        }
    }

    //msg allow send when state=wow
    is_suspend_resume_msg = aml_check_suspend_resume_msg(aml_hw, msg);

    if (((g_pci_msg_suspend) || (!is_suspend_resume_msg)) && ((msg->param_len != 0) && (*(msg->param) != MM_SUB_SHUTDOWN))
#ifdef CONFIG_AML_RECOVERY
        && (!aml_recy_flags_chk(AML_RECY_STATE_ONGOING))
#endif
    ) {
        AML_INFO("driver in suspend, cmd not allow to send, id:%d,aml_hw->state:%d g_pci_msg_suspend:%d\n",
            msg->id, aml_hw->state, g_pci_msg_suspend);
        return -EBUSY;
    }

    REG_SW_SET_PROFILING(env->pthis, SW_PROF_IPC_MSGPUSH);

    ASSERT_ERR(!env->msga2e_hostid);
    ASSERT_ERR(round_up(len, 4) <= sizeof(env->shared->msg_a2e_buf.msg));

    // Copy the message into the IPC MSG buffer
    src = (uint8_t*)((struct aml_cmd *)msg_buf)->a2e_msg;
    dst = (uint8_t*)&(env->shared->msg_a2e_buf.msg);

    // Copy the message in the IPC queue
    if (aml_bus_type == USB_MODE) {
        aml_hw->plat->hif_ops->hi_write_sram((unsigned char *)src, (unsigned char *)&(env->shared->msg_a2e_buf.msg), len, USB_EP4);
    } else if (aml_bus_type == SDIO_MODE) {
        aml_hw->plat->hif_sdio_ops->hi_random_ram_write((unsigned char *)src, (unsigned char *)&(env->shared->msg_a2e_buf.msg), len);
    } else {
        for (i = 0; i < len; i ++ ) {
            *dst++ = *src++;
        }
    }

    env->msga2e_hostid = msg_buf;

    // Trigger the irq to send the message to EMB
    ipc_app2emb_trigger_set(env->pthis, IPC_IRQ_A2E_MSG);

    REG_SW_CLEAR_PROFILING(env->pthis, SW_PROF_IPC_MSGPUSH);

    return (0);
}

/**
 ******************************************************************************
 */
void ipc_host_enable_irq(struct ipc_host_env_tag *env, uint32_t value)
{
    // Enable the handled interrupts
    ipc_emb2app_unmask_set(env->pthis, value);
}

/**
 ******************************************************************************
 */
void ipc_host_disable_irq(struct ipc_host_env_tag *env, uint32_t value)
{
    // Enable the handled interrupts
    ipc_emb2app_unmask_clear(env->pthis, value);
}

/**
 ******************************************************************************
 */
uint32_t ipc_host_get_status(struct ipc_host_env_tag *env)
{
    volatile uint32_t status;

    status = ipc_emb2app_status_get(env->pthis);
    return status;
}

/**
 ******************************************************************************
 */
uint32_t ipc_host_get_rawstatus(struct ipc_host_env_tag *env)
{
    volatile uint32_t rawstatus;

    rawstatus = ipc_emb2app_rawstatus_get(env->pthis);

    return rawstatus;
}

#ifdef CONFIG_AML_USE_TASK
int aml_task_fn_irqhdlr(struct aml_task *t)
{
    struct aml_hw *aml_hw = container_of(t, struct aml_hw, pcie.task_irqhdlr);
    u32 status;

    while ((status = ipc_host_get_status(aml_hw->ipc_env))) {
         // Acknowledge the pending interrupts
         ipc_emb2app_ack_clear(aml_hw, status);
         ipc_host_irq(aml_hw->ipc_env, status);
     }

    aml_spin_lock(&aml_hw->tx_lock);
    aml_hwq_process_all(aml_hw);
    aml_spin_unlock(&aml_hw->tx_lock);


    if (!(kthread_should_stop() || t->quit))
        enable_irq(aml_platform_get_irq(aml_hw->plat));

    aml_hw->plat->ack_irq(aml_hw);      /* aml_pci_ack_irq */

    return 0;
}

int aml_task_fn_rxdesc(struct aml_task *t)
{
    struct aml_hw *aml_hw = container_of(t, struct aml_hw, pcie.task_rxdesc);

    ipc_host_rxdesc_handler(aml_hw->ipc_env);

    return 0;
}

#else

/**
 * aml_task - Bottom half for IRQ handler
 *
 * Read irq status and process accordingly
 */
void aml_pcie_task(unsigned long data)
{
    struct aml_hw *aml_hw = (struct aml_hw *)data;
    struct aml_plat *aml_plat = aml_hw->plat;
    u32 status;

    REG_SW_SET_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);

    /* Ack unconditionally in case ipc_host_get_status does not see the irq */
    aml_plat->ack_irq(aml_hw);

    while ((status = ipc_host_get_status(aml_hw->ipc_env))) {
        /* All kinds of IRQs will be handled in one shot (RX, MSG, DBG, ...)
         * this will ack IPC irqs not the cfpga irqs */

        // Acknowledge the pending interrupts
        ipc_emb2app_ack_clear(aml_hw, status);

        ipc_host_irq(aml_hw->ipc_env, status);
        aml_plat->ack_irq(aml_hw);
    }

    aml_spin_lock(&aml_hw->tx_lock);
    aml_hwq_process_all(aml_hw);
    aml_spin_unlock(&aml_hw->tx_lock);

    enable_irq(aml_platform_get_irq(aml_plat));
    REG_SW_CLEAR_PROFILING(aml_hw, SW_PROF_AML_IPC_IRQ_HDLR);
}
#endif
