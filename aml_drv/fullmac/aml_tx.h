/**
 ******************************************************************************
 *
 * @file aml_tx.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */
#ifndef _AML_TX_H_
#define _AML_TX_H_

#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/netdevice.h>
#include "lmac_types.h"
#include "ipc_shared.h"
#include "aml_txq.h"
#include "hal_desc.h"
#include "aml_utils.h"
#include "fi_sdio.h"

#define AML_HWQ_BK                     0
#define AML_HWQ_BE                     1
#define AML_HWQ_VI                     2
#define AML_HWQ_VO                     3
#define AML_HWQ_BCMC                   4
#define AML_HWQ_NB                     NX_TXQ_CNT
#define AML_HWQ_ALL_ACS (AML_HWQ_BK | AML_HWQ_BE | AML_HWQ_VI | AML_HWQ_VO)
#define AML_HWQ_ALL_ACS_BIT ( BIT(AML_HWQ_BK) | BIT(AML_HWQ_BE) |    \
                               BIT(AML_HWQ_VI) | BIT(AML_HWQ_VO) )

#define AML_TX_LIFETIME_MS             1000
#define AML_TX_MAX_RATES               NX_TX_MAX_RATES


#define TXU_CNTRL_RETRY        BIT(0)
#define TXU_CNTRL_MORE_DATA    BIT(2)
#define TXU_CNTRL_MGMT         BIT(3)
#define TXU_CNTRL_MGMT_NO_CCK  BIT(4)
#define TXU_CNTRL_AMSDU        BIT(6)
#define TXU_CNTRL_MGMT_ROBUST  BIT(7)
#define TXU_CNTRL_USE_4ADDR    BIT(8)
#define TXU_CNTRL_EOSP         BIT(9)
#define TXU_CNTRL_MESH_FWD     BIT(10)
#define TXU_CNTRL_SP_FRAME     BIT(10)
#define TXU_CNTRL_TDLS         BIT(11)
#define TXU_CNTRL_REUSE_SN     BIT(15)

#define DHCP_SP_V4      0x0043
#define DHCP_CP_V4      0x0044
#define DHCP_SP_V6      0x0223
#define DHCP_CP_V6      0x0222
#define ETH_ADDR_LEN  6
#define ETH_TYPE_LEN  2
#define ETH_HDR_LEN   (ETH_ADDR_LEN * 2 + ETH_TYPE_LEN)

#define MAC_SHORT_MAC_HDR_LEN   24
#define ACTION_CODE_VENDOR      0x09

#define ACTION_GAS_INIT_REQ        10
#define ACTION_GAS_INIT_RSP        11
#define ACTION_GAS_COMEBACK_REQ    12
#define ACTION_GAS_COMEBACK_RSP    13

#define ACTION_TYPE             0x0d
#define AUTH_TYPE               0x0b
#define PROBE_RSP_TYPE          0X05
#define ASSOC_RSP_TYPE          0X01

#define PUBLIC_ACTION           0x04
#define P2P_ACTION              0x7f
#define OUI_TYPE_P2P            0x09
#define OUI_TYPE_DPP            0x1a

#define CATEGORY_OFFSET         MAC_SHORT_MAC_HDR_LEN
#define ACTION_CODE_OFFSET      (CATEGORY_OFFSET + 1)
#define OUI_OFFSET              (CATEGORY_OFFSET + 2)
#define OUI_TYPE_OFFSET         (CATEGORY_OFFSET + 5)
#define OUI_SUBTYPE_OFFSET      (CATEGORY_OFFSET + 6)
#define DPP_PUBLIC_ACTION_SUBTYPE_OFFSET      (CATEGORY_OFFSET + 7)

#define AUTH_ALGO_OFFSET        MAC_SHORT_MAC_HDR_LEN

#define TXCFM_RESET_CNT   42
#define TXCFM_TRIGGER_TX_THR  20

extern const int aml_tid2hwq[IEEE80211_NUM_TIDS];

/**
 * struct aml_amsdu_txhdr - Structure added in skb headroom (instead of
 * aml_txhdr) for amsdu subframe buffer (except for the first subframe
 * that has a normal aml_txhdr)
 *
 * @list: List of other amsdu subframe (aml_sw_txhdr.amsdu.hdrs)
 * @ipc_data: IPC buffer for the A-MSDU subframe
 * @skb: skb
 * @pad: padding added before this subframe
 * (only use when amsdu must be dismantled)
 * @msdu_len Size, in bytes, of the MSDU (without padding nor amsdu header)
 */
struct aml_amsdu_txhdr {
    struct list_head list;
    struct aml_ipc_buf ipc_data;
    struct sk_buff *skb;
    u16 pad;
    u16 msdu_len;
};

/**
 * struct aml_amsdu - Structure to manage creation of an A-MSDU, updated
 * only In the first subframe of an A-MSDU
 *
 * @hdrs: List of subframe of aml_amsdu_txhdr
 * @len: Current size for this A-MDSU (doesn't take padding into account)
 * 0 means that no amsdu is in progress
 * @nb: Number of subframe in the amsdu
 * @pad: Padding to add before adding a new subframe
 */
struct aml_amsdu {
    struct list_head hdrs;
    u16 len;
    u8 nb;
    u8 pad;
};

/**
 * struct aml_sw_txhdr - Software part of tx header
 *
 * @aml_sta: sta to which this buffer is addressed
 * @aml_vif: vif that send the buffer
 * @txq: pointer to TXQ used to send the buffer
 * @hw_queue: Index of the HWQ used to push the buffer.
 *           May be different than txq->hwq->id on confirmation.
 * @frame_len: Size of the frame (doesn't not include mac header)
 *            (Only used to update stat, can't we use skb->len instead ?)
 * @amsdu: Description of amsdu whose first subframe is this buffer
 *        (amsdu.nb = 0 means this buffer is not part of amsdu)
 * @skb: skb received from transmission
 * @ipc_data: IPC buffer for the frame
 * @ipc_desc: IPC buffer for the TX descriptor
 * @jiffies: Jiffies when this buffer has been pushed to the driver
 * @desc: TX descriptor downloaded by firmware
 */
struct aml_sw_txhdr {
    struct aml_sta *aml_sta;
    struct aml_vif *aml_vif;
    struct aml_txq *txq;
    u8 hw_queue;
    u16 frame_len;
#ifdef CONFIG_AML_AMSDUS_TX
    struct aml_amsdu amsdu;
#endif
    struct sk_buff *skb;
    struct aml_ipc_buf ipc_data;
    struct aml_ipc_buf ipc_desc;
    unsigned long jiffies;
    struct txdesc_host desc;
    struct list_head list;
};

/**
 * struct aml_txhdr - Structure to control transmission of packet
 * (Added in skb headroom)
 *
 * @sw_hdr: Information from driver
 */
struct aml_txhdr {
    struct aml_sw_txhdr *sw_hdr;
};


struct aml_sdio_txhdr {
    struct aml_sw_txhdr *sw_hdr;
    unsigned int mpdu_buf_flag;// 4
    struct hw_tx_vector_bits tx_vector; //28
    struct txdesc_host desc; //72
};
struct aml_usb_txhdr {
    struct aml_sw_txhdr *sw_hdr; //8 byte
    struct txdesc_host desc;//72 byte
};

struct aml_eap_hdr {
    u8 version;
#define EAP_PACKET_TYPE     (0)
    u8 type;
    u16 len;
#define EAP_FAILURE_CODE    (4)
    u8 code;
    u8 id;
    u16 auth_proc_len;
    u8 auth_proc_type;
    u64 ex_id:24;
    u64 ex_type:32;
#define EAP_WSC_DONE        (5)
    u64 opcode:8;
};

struct tx_cfm_wait_rsp
{
    u64 cookie;
    struct sk_buff *skb;
    uint32_t len;
    struct wireless_dev *wdev;
};

typedef enum {
    SP_STATUS_TX_START = 0,
    SP_STATUS_RX,
    SP_STATUS_TX_SUC,
    SP_STATUS_TX_FAIL,
}AML_SP_STATUS_E;

enum {
    AML_SP_FRAME = BIT(0),
    AML_P2P_ACTION_FRAME = BIT(1),
    AML_DPP_ACTION_FRAME = BIT(2),
    AML_CSA_ACTION_FRAME = BIT(3),
    AML_GAS_ACTION_FRAME = BIT(4),
    AML_GAS_INIT_REQ_FRAME = BIT(5),
    AML_GAS_INIT_RSP_FRAME = BIT(6),
};

/**
 * AML_TX_HEADROOM - Headroom to use to store struct aml_txhdr
 */
#define AML_TX_HEADROOM sizeof(struct aml_txhdr)
#define AML_SDIO_TX_HEADROOM sizeof(struct aml_sdio_txhdr)
#define AML_USB_TX_HEADROOM sizeof(struct aml_usb_txhdr)
/**
 * AML_TX_AMSDU_HEADROOM - Maximum headroom need for an A-MSDU sub frame
 * Need to store struct aml_amsdu_txhdr, A-MSDU header (14)
 * optional padding (4) and LLC/SNAP header (8)
 */
#define AML_TX_AMSDU_HEADROOM (sizeof(struct aml_amsdu_txhdr) + 14 + 4 + 8)

/**
 * AML_TX_MAX_HEADROOM - Maximum size needed in skb headroom to prepare a buffer
 * for transmission
 */
#define AML_TX_MAX_HEADROOM max(AML_TX_HEADROOM, AML_TX_AMSDU_HEADROOM)
#define AML_SDIO_TX_MAX_HEADROOM max(AML_SDIO_TX_HEADROOM, AML_TX_AMSDU_HEADROOM)
#define AML_USB_TX_MAX_HEADROOM max(AML_USB_TX_HEADROOM, AML_TX_AMSDU_HEADROOM)
/**
 * AML_TX_DMA_MAP_LEN - Length, in bytes, to map for DMA transfer
 * To be called with skb BEFORE reserving headroom to store struct aml_txhdr.
 */
#define AML_TX_DMA_MAP_LEN(skb) (skb->len - sizeof(struct ethhdr))

#define WAPI_TYPE                 0x88B4

/**
 * SKB buffer format before it is pushed to MACSW
 *
 * For DATA frame
 *                    |--------------------|
 *                    | headroom           |
 *    skb->data ----> |--------------------|
 *                    | struct aml_txhdr  |
 *                    | * aml_sw_txhdr *  |
 *                    | * [L1 guard]       |
 *                    |--------------------|
 *                    | 802.3 Header       |
 *               +--> |--------------------| <---- desc.host.packet_addr[0]
 *     memory    :    | Data               |
 *     mapped    :    |                    |
 *     for DMA   :    |                    |
 *               :    |                    |
 *               +--> |--------------------|
 *                    | tailroom           |
 *                    |--------------------|
 *
 *
 * For MGMT frame (skb is created by the driver so buffer is always aligned
 *                 with no headroom/tailroom)
 *
 *    skb->data ----> |--------------------|
 *                    | struct aml_txhdr  |
 *                    | * aml_sw_txhdr *  |
 *                    | * [L1 guard]       |
 *                    |                    |
 *               +--> |--------------------| <---- desc.host.packet_addr[0]
 *     memory    :    | 802.11 HDR         |
 *     mapped    :    |--------------------|
 *     for DMA   :    | Data               |
 *               :    |                    |
 *               +--> |--------------------|
 *
 */

u16 aml_select_txq(struct aml_vif *aml_vif, struct sk_buff *skb);
netdev_tx_t aml_start_xmit(struct sk_buff *skb, struct net_device *dev);
int aml_start_mgmt_xmit(struct aml_vif *vif, struct aml_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         u64 *cookie);
int aml_txdatacfm(void *pthis, void *host_id);

struct aml_hw;
struct aml_sta;
void aml_set_traffic_status(struct aml_hw *aml_hw,
                             struct aml_sta *sta,
                             bool available,
                             u8 ps_id);
void aml_ps_bh_enable(struct aml_hw *aml_hw, struct aml_sta *sta,
                       bool enable);
void aml_ps_bh_traffic_req(struct aml_hw *aml_hw, struct aml_sta *sta,
                            u16 pkt_req, u8 ps_id);

void aml_switch_vif_sta_txq(struct aml_sta *sta, struct aml_vif *old_vif,
                             struct aml_vif *new_vif);

int aml_dbgfs_print_sta(char *buf, size_t size, struct aml_sta *sta,
                         struct aml_hw *aml_hw);
void aml_txq_credit_update(struct aml_hw *aml_hw, int sta_idx, u8 tid,
                            s8 update);
void aml_tx_push(struct aml_hw *aml_hw, struct aml_txhdr *txhdr, int flags);
int aml_update_tx_cfm(void *pthis);
int aml_sdio_tx_task(void *data);
bool aml_filter_sp_data_frame(struct sk_buff *skb,struct aml_vif *aml_vif,AML_SP_STATUS_E sp_status);
int aml_prep_dma_tx(struct aml_hw *aml_hw, struct aml_sw_txhdr *sw_txhdr, void *frame_start);
void aml_tx_cfm_wait_rsp(struct aml_hw *aml_hw, bool ack, u8* func, u32 line);
uint32_t aml_filter_sp_mgmt_frame(struct aml_vif *vif, u8 *buf, AML_SP_STATUS_E sp_status, u32 frame_len, u32* len_diff);

#endif /* _AML_TX_H_ */
