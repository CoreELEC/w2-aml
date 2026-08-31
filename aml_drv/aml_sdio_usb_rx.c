/*
****************************************************************************************
*
* @file aml_sdio_usb_rx.c
*
* @brief Implementation of the aml sdio_usb rx
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#define AML_MODULE                  RX

#include <linux/delay.h>
#include <linux/ktime.h>

#include "aml_defs.h"
#include "aml_rate.h"
#include "aml_prealloc.h"
#include "wifi_top_addr.h"

#define AML_RX_WRAP_FLAG            RX_WRAP_FLAG

/* FIXME: UNWRAP_SIZE is too long, but need cover aml_rhd_ext at least */
#define AML_RX_FRAG0_MINI_SIZE      (RX_DESC_SIZE + RX_PD_LEN + UNWRAP_SIZE)

#define AML_RX_BUF_FW_FLAGS         (RX_WRAP_TEMP_FLAG | FW_BUFFER_NARROW | FW_BUFFER_EXPAND)

#define AML_RX_BUF_HOST_FLAGS       (RX_ENLARGE_READ_RX_DATA_FINISH | HOST_RXBUF_ENLARGE_FINISH | \
                                     RX_REDUCE_READ_RX_DATA_FINISH | HOST_RXBUF_REDUCE_FINISH)

#ifdef CONFIG_AML_W2L_RX_MINISIZE

#define RX_DESC_SIZE                   ((u32)sizeof(struct rxdesc))             /* 52 bytes */
#define RX_HEADER_OFFSET               ((u32)offsetof(struct rxdesc, dma_hdrdesc.hd.frmlen))
#define RX_PD_LEN                      (0)

#elif defined(CONFIG_AML_W2_RX_MINISIZE)    /* FIXME: remove this section, it doesn't work */

#define RX_DESC_SIZE                   (80)
#define RX_HEADER_OFFSET               (28)
#define RX_PD_LEN                      (20)
#define RX_PAYLOAD_OFFSET              (RX_DESC_SIZE + RX_PD_LEN)

#define RX_HOSTID_OFFSET               (36)
#define RX_REORDER_LEN_OFFSET          (38)
#define RX_STATUS_OFFSET               (32)
#define RX_FRMLEN_OFFSET               (28)
#define NEXT_PKT_OFFSET                (56)

#else /* W2 layout */

#define RX_DESC_SIZE                   ((u32)sizeof(struct rxdesc))             /* 128 bytes */
#define RX_HEADER_OFFSET               ((u32)offsetof(struct rxdesc, dma_hdrdesc.hd.frmlen))
#define RX_PD_LEN                      ((u32)sizeof(struct rx_payloaddesc))     /* 36 bytes */

#endif

#define AML_FW_PATCHED  /* highlight the fields that firmware changed(patched) its usage */

#ifdef CONFIG_AML_W2L_RX_MINISIZE           /* W2L compressed RX descriptor */

struct rx_hd   /* = f/w: struct rxdesc_new */
{
    uint16_t frmlen;
    uint16_t ampdu_stat_info;
#ifdef AML_FW_PATCHED
    u8 payl_offset;
    u8 status;
    u16 hostid;             /* for SDIO/USB: = SN + 1 */
    struct {
        uint16_t hostid;
        u8 len;
        u8 tid:3;
        u8 pad:4;
        u8 valid:1;         /* SDIO_RX_REORDER_FlG */
    } reorder;
#else
    uint32_t tsflo;
    uint32_t tsfhi;
#endif

    struct rx_vector_1 rx_vec_1;
#ifdef AML_FW_PATCHED
    /* struct rx_info { */
        uint32_t new_read;
        /// Id of the buffer (0 or 1)
        uint8_t buf_id;
        uint8_t patched: 1; /* RXMINISIZE_FLAGS_TSFLO_IS_RX_STATUS */
        uint8_t reserve: 7;
        /// Total length of the received buffer include padding for 4-byte alignment
        uint16_t frmlen_padded;
    /* }; */
#else
    struct rx_vector_2 rx_vec_2;
#endif

    uint32_t statinfo;
    struct phy_channel_info phy_info;
    uint32_t flag;
};

struct rxdesc {
    struct {
        struct rx_hd hd;
    } dma_hdrdesc;
};

#elif defined(CONFIG_AML_W2_RX_MINISIZE)    /* W2 compressed RX descriptor */

#error "FIXME: remove CONFIG_AML_W2_RX_MINISIZE, actually it doesn't work due to hardware issues"

#else                                       /* W2 non-compressed RX descriptor */

struct rx_upload_cntrl_tag {
    u32 fw_internal_use[5];
};

/// Element in the pool of RX header descriptor.
struct rx_hd
{
    /// Unique pattern for receive DMA.
    uint32_t            upatternrx;
    /// Pointer to the location of the next Header Descriptor
    uint32_t            next;
    /// Pointer to the first payload buffer descriptor
    uint32_t            first_pbd_ptr;
    /// Pointer to the SW descriptor associated with this HW descriptor
    addr32_t            rxdesc;
#ifdef AML_FW_PATCHED
    struct {
        u16 hostid;
        u16 pad;

        u8 len;
        u8 tid;
        u16 reserved;
    } reorder;
    u8 payl_offset;
    u8 status;
    u16 hostid;
#else
    /// Pointer to the address in buffer where the hardware should start writing the data
    uint32_t            datastartptr;
    /// Pointer to the address in buffer where the hardware should stop writing data
    uint32_t            dataendptr;
    /// Header control information. Except for a single bit which is used for enabling the
    /// Interrupt for receive DMA rest of the fields are reserved
    uint32_t            headerctrlinfo;
#endif

    /// Total length of the received MPDU
    uint16_t            frmlen;
    /// AMPDU status information
    uint16_t            ampdu_stat_info;
    /// TSF Low
    uint32_t            tsflo;
    /// TSF High
    uint32_t            tsfhi;
    /// Rx Vector 1
    struct rx_vector_1  rx_vec_1;
    /// Rx Vector 2
    struct rx_vector_2  rx_vec_2;
    /// MPDU status information
    uint32_t            statinfo;
};

struct rx_dmadesc
{
    /// Rx header descriptor (this element MUST be the first of the structure)
    struct rx_hd hd;
    /// Structure containing the information about the PHY channel that was used for this RX
    struct phy_channel_info phy_info;

    /// Word containing some SW flags about the RX packet
    uint32_t flags;
    /// Spare room for LMAC FW to write a pattern when last DMA is sent
    uint32_t pattern;
    /// IPC DMA control structure for MAC Header transfer
    struct dma_desc dma_desc;
};

struct rxdesc
{
    /// Upload control element. Shall be the first element of the RX descriptor structure
    struct rx_upload_cntrl_tag upload_cntrl;
    /// HW descriptors
    struct rx_dmadesc dma_hdrdesc;
    /// Address of the expected HW descriptor following the present in the RX buffer one,
    /// and that should be used to set to the read pointer to free the buffer
    uint32_t new_read;
    /// Id of the buffer (0 or 1)
    uint8_t buf_id;
};

/// Element in the pool of rx payload buffer descriptors.
struct rx_pbd
{
    /// Unique pattern
    uint32_t            upattern;
    /// Points to the next payload buffer descriptor of the MPDU when the MPDU is split
    /// over several buffers
    uint32_t            next;
    /// Points to the address in the buffer where the data starts
    uint32_t            datastartptr;
    /// Points to the address in the buffer where the data ends
    uint32_t            dataendptr;
    /// buffer status info for receive DMA.
    uint16_t            bufstatinfo;
    /// complete length of the buffer in memory
    uint16_t            reserved;
};

struct rx_payloaddesc
{
    /// Mac header buffer (this element MUST be the first of the structure)
    struct rx_pbd pbd;
    /// IPC DMA control structures
#define NX_DMADESC_PER_RX_PDB_CNT   1
    struct dma_desc dma_desc[NX_DMADESC_PER_RX_PDB_CNT];
};
#endif

static inline uint32_t *aml_rx_desc_next_ptr(void *_rxdesc)
{
    struct rxdesc *rxdesc = (struct rxdesc *)_rxdesc;

#ifdef CONFIG_AML_W2L_RX_MINISIZE
    return &rxdesc->dma_hdrdesc.hd.new_read;
#elif defined(CONFIG_AML_W2_RX_MINISIZE)    /* W2 compressed RX descriptor */
#error "W2_RX_MINISIZE is unsupported!"
#else
    return &rxdesc->new_read;
#endif
}

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER

#define RX_DATA_MAX_CNT (512 + 128)

static inline void aml_sdio_usb_fw_reo_enqueue(struct aml_rx *rx, struct sk_buff *skb)
{
    int qlen = skb_queue_len(&rx->fw_reo.list);

    if (qlen > RX_DATA_MAX_CNT)
        AML_WARN("TODO: f/w reorder buffered too many frames (%d)!\n", qlen);

    skb_queue_tail(&rx->fw_reo.list, skb);
}

static void aml_sdio_usb_fw_reo_dequeue(struct aml_rx *rx,
                                        struct sk_buff_head *frames,
                                        struct fw_reo_inst *inst)
{
    struct sk_buff_head *mpdus = &rx->fw_reo.list;
    u16 hostid = ieee80211_sn_add(inst->hostid, 0); /* truncated to 12-bit */
    int i;

    spin_lock_bh(&mpdus->lock);
    for (i = 0; i < inst->len && !skb_queue_empty(mpdus); ++i) {
        struct sk_buff *skb;

        skb_queue_walk(mpdus, skb) {
            struct aml_skb_rxcb *cb = AML_SKB_RXCB(skb);

            /* rhd_ext.sn is host id (sn + 1), and truncated to 12-bit */
            if (cb->tid == inst->tid && cb->rhd_ext.sn == hostid) {
                __skb_unlink(skb, mpdus);
                __skb_queue_tail(frames, skb);
                break;
            }
        }
        hostid = ieee80211_sn_inc(hostid);
    }
    spin_unlock_bh(&mpdus->lock);
}

static void aml_sdio_usb_fw_reo_inst_handle(struct aml_rx *rx, struct fw_reo_inst *embedded)
{
    int i;
    struct sk_buff_head frames;

    __skb_queue_head_init(&frames);

    /*
     * NB: handle the reorder instruction embedded in rxdesc, or received by e2a msg.
     * but the handling sequence can't be guaranteed same as it's generated in firmware.
     * it introduces the out-of-order issue.
     *
     * Therefore, host reorder is preferred for USB/SDIO,
     * firmware reorder is just for internal test.
     */

    /* handle the f/w reorder instruction embedded in rxdesc */
    if (embedded->len) {
        aml_sdio_usb_fw_reo_dequeue(rx, &frames, embedded);
    }

    /* handle the f/w reorder (timeout) instructions sent by e2a msg */
    for (i = 0; i < IEEE80211_NUM_UPS; i++) {
        struct fw_reo_inst *inst = &rx->fw_reo.instructions[i];

        if (inst->len) {
            aml_sdio_usb_fw_reo_dequeue(rx, &frames, inst);
            inst->len = 0;
        }
    }

    aml_reo_forward(rx, &frames);
}

int aml_sdio_usb_fw_reo_inst_save(struct aml_rx *rx, struct fw_reo_inst *reo_inst)
{
    u8 tid = reo_inst->tid;

    if (reo_inst->len == 0)
        return 0;

    if (WARN_ON(tid >= ARRAY_SIZE(rx->fw_reo.instructions))) {
        AML_INFO("tid %u >= 8!\n", tid);
        return -1;
    }
    /* host id = sn + 1 */
    if (WARN_ON(reo_inst->hostid == 0 || reo_inst->hostid > IEEE80211_SN_MODULO)) {
        AML_INFO("invalid host id %u!\n", reo_inst->hostid);
        return -1;
    }

    spin_lock_bh(&rx->fw_reo.list.lock);
    rx->fw_reo.instructions[tid] = *reo_inst;
    spin_unlock_bh(&rx->fw_reo.list.lock);
    return 0;
}

static inline void aml_sdio_usb_fw_reo_clean(struct aml_rx *rx)
{
    struct sk_buff *skb;

    while ((skb = skb_dequeue(&rx->fw_reo.list)))
        aml_mpdu_free(skb);
}

#endif

struct aml_frag {
    const u8 *data;
    int len;
};

static inline void aml_frags_copy(const struct aml_frag frags[2], int offset, void *dest, int len)
{
    int start = offset - frags[0].len;

    if (start >= 0) {
        memcpy(dest, frags[1].data + start, len);
    } else if ((start + len) <= 0) {
        memcpy(dest, frags[0].data + offset, len);
    } else {
        int frag0 = -start;

        memcpy(dest, frags[0].data + offset, frag0);
        memcpy(dest + frag0, frags[1].data, len - frag0);
    }
}

/* refer to ieee80211_amsdu_to_8023s() */
static void aml_amsdu_to_msdu(struct sk_buff_head *msdus,
                              const struct aml_frag amsdu_frags[2],
                              const unsigned int extra_headroom,
                              const u8 *check_da,
                              const u8 *check_sa)
{
    unsigned int hlen = ALIGN(extra_headroom, 4) + ALIGN(sizeof(struct ethhdr), 4);
    int amsdu_len = amsdu_frags[0].len + amsdu_frags[1].len;
    int offset = 0;

    while (offset < amsdu_len) {
        struct sk_buff *frame;
        struct ethhdr eth;
        int len;
        u8 padding;

        aml_frags_copy(amsdu_frags, offset, &eth, sizeof(eth));
        offset += sizeof(struct ethhdr);

        len = ntohs(eth.h_proto);
        if ((len < 0) || (len >= ETH_P_802_3_MIN)) {
            AML_RLMT_WARN("Invalid subframe length %d at offset %d", len, offset);
            goto purge;
        }

        padding = (4 - (sizeof(struct ethhdr) + len)) & 0x3;
        if (offset + len > amsdu_len) {
            AML_RLMT_WARN("offset %d subframe length %d exceeds amsdu length %d!",
                          offset, len, amsdu_len);
            goto purge;
        }

        if (ether_addr_equal(eth.h_dest, rfc1042_header)) {
            AML_RLMT_WARN("mitigate A-MSDU aggregation injection attacks at %d/%d!",
                          offset, amsdu_len);
            goto purge;
        }

        /* FIXME: should we really accept multicast DA? */
        if ((check_da && !is_multicast_ether_addr(eth.h_dest) &&
             !ether_addr_equal(check_da, eth.h_dest)) ||
            (check_sa && !ether_addr_equal(check_sa, eth.h_source))) {
            offset += len + padding;
            continue;
        }

        frame = dev_alloc_skb(hlen + len);
        if (!frame)
            goto purge;

        skb_reserve(frame, hlen);
        aml_frags_copy(amsdu_frags, offset, skb_put(frame, len), len);
        offset += len + padding;

        if (len >= ETH_ALEN + 2) {
            const u8 *payload = frame->data;
            u16 ethertype = (payload[6] << 8) | payload[7];

            if (likely((ether_addr_equal(payload, rfc1042_header) &&
                       ethertype != ETH_P_AARP && ethertype != ETH_P_IPX) ||
                ether_addr_equal(payload, bridge_tunnel_header))) {
                /* remove the room of rfc1042_header or bridge_tunnel_header */
                skb_pull(frame, ETH_ALEN + 2);
                eth.h_proto = htons(ethertype);
            }
        }

        skb_reset_network_header(frame);

        /* push back the ethernet header */
        memcpy(skb_push(frame, sizeof(eth)), &eth, sizeof(eth));

        __skb_queue_tail(msdus, frame);
    }

    return;

purge:
    __skb_queue_purge(msdus);
}

static int aml_sdio_usb_rx_napi_poll(struct napi_struct *napi, int budget)
{
    struct aml_rx *rx = container_of(napi, struct aml_rx, napi);
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    struct aml_sta *sta = NULL;
    struct aml_vif *aml_vif = NULL;
    struct sk_buff *skb;
    bool sap = false;
    int done = 0;

    spin_lock(&rx->napi_preq.lock);
    AML_DBG("pending q: +%d msdus\n", skb_queue_len(&rx->napi_preq));
    skb_queue_splice_tail_init(&rx->napi_preq, &rx->napi_pending);
    AML_PROF_CNT(preq, 0);
    spin_unlock(&rx->napi_preq.lock);

    AML_PROF_CNT(pending, skb_queue_len(&rx->napi_pending));
    while (done < budget && (skb = __skb_dequeue(&rx->napi_pending))) {
        struct aml_skb_rxcb *rxcb = AML_SKB_RXCB(skb);
        bool forward = true;
        bool resend = false;

        AML_DBG("msdu(%4d): %32ph\n", skb->len, skb->data);

        if (!sta || sta->sta_idx != rxcb->sta_idx) {
            sta = aml_sta_get(aml_hw, rxcb->sta_idx);
            aml_vif = aml_rx_get_vif(aml_hw, sta ? sta->vlan_idx : rxcb->vif);
            if (!aml_vif) {
                AML_RLMT_ERR("Frame received but no active vif (%d)", rxcb->vif);
                dev_kfree_skb(skb);
                continue;
            }
            sap = (AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP ||
                   AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_AP_VLAN ||
                   AML_VIF_TYPE(aml_vif) == NL80211_IFTYPE_P2P_GO) &&
                           !(aml_vif->ap.flags & AML_AP_ISOLATE);
        }

        if (sta) {
            if (rxcb->is_4addr && !aml_vif->use_4addr)
                cfg80211_rx_unexpected_4addr_frame(aml_vif->ndev, sta->mac_addr, GFP_ATOMIC);
        }

        /* refer to cfg80211_classify8021d() */
        skb->priority = 256 + rxcb->tid;
        skb->dev = aml_vif->ndev;
        skb_reset_mac_header(skb);

        /* forward and/or resend? */
        if (sap) {
            const struct ethhdr *eth = (void *)eth_hdr(skb);

            if (unlikely(is_multicast_ether_addr(eth->h_dest))) {
                resend = true;
            } else if (unlikely(!ether_addr_equal(eth->h_dest, aml_vif->ndev->dev_addr))) {
                if (rxcb->dst_idx != AML_STA_ID_UNKNOWN) {
                    struct aml_sta *dst = aml_sta_get(aml_hw, rxcb->dst_idx);

                    /* destination is inside the BSS? */
                    resend = dst && dst->vlan_idx == aml_vif->vif_index;
                }
                forward = false;
            }

            AML_DBG("resend %d forward %d STA %d/%d: %pM %pM %16ph\n",
                    resend, forward, rxcb->dst_idx, rxcb->sta_idx,
                    eth->h_dest, eth->h_source, &eth->h_proto);
        }

        if (resend) {
            struct sk_buff *skb_tx = skb;

            if (forward) {
                /* FIXME: use skb_clone for better performance after refine TX data path */
                skb_tx = skb_copy_expand(skb, rx->skb_head_room, 0, GFP_ATOMIC);
                if (!skb_tx)
                    AML_RLMT_WARN("Failed to re-send due to no-memory!\n");
            }

            if (skb_tx) {
                int res;

                aml_vif->is_re_sending = true;
                res = dev_queue_xmit(skb_tx);
                aml_vif->is_re_sending = false;
                /* note: buffer is always consumed by dev_queue_xmit */
                if (res == NET_XMIT_DROP) {
                    aml_vif->net_stats.rx_dropped++;
                    aml_vif->net_stats.tx_dropped++;
                } else if (res != NET_XMIT_SUCCESS) {
                    aml_vif->net_stats.tx_errors++;
                    netdev_err(aml_vif->ndev,
                               "Failed to re-send buffer to driver (res=%d)",
                               res);
                }
            }

            if (!forward)
                continue;
        } else if (!forward) {
            AML_RLMT_NOTICE("RX drop (%4d) %32ph\n", skb->len, skb->data);
            aml_vif->net_stats.rx_dropped++;
            dev_kfree_skb(skb);
            continue;
        }

        /* forward */
        if (!rxcb->amsdu
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
                && rx->host_reorder /* firmware reorder has checked/dumped special frame */
#endif
            )
            aml_filter_sp_data_frame((u8 *)eth_hdr(skb), skb->len, aml_vif, SP_STATUS_RX);

        skb->protocol = eth_type_trans(skb, aml_vif->ndev);
        memset(skb->cb, 0, sizeof(skb->cb));

        /* Update statistics */
        aml_vif->net_stats.rx_packets++;
        aml_vif->net_stats.rx_bytes += skb->len;

        AML_PROF_CNT(gro_rx, skb->len);
        napi_gro_receive(napi, skb);
        AML_PROF_CNT(gro_rx, 0);
        ++done;
    }
    AML_PROF_CNT(pending, skb_queue_len(&rx->napi_pending));

    AML_DBG("napi poll: done: %d, budget:%d\n", done, budget);
    if (done < budget)
        napi_complete_done(napi, done);

    return done;
}

static void aml_sdio_usb_rx_napi_preq_append(struct aml_rx *rx, struct sk_buff_head *frames)
{
    struct sk_buff *skb;
    struct sk_buff_head msdus;

    if (skb_queue_empty(frames))
        return;

    __skb_queue_head_init(&msdus);

    while ((skb = __skb_dequeue(frames))) {
        struct aml_skb_rxcb *rxcb = AML_SKB_RXCB(skb);

        AML_DBG("%s(%4d): %32ph\n", rxcb->amsdu ? "amsdu's last": "msdu", skb->len, skb->data);
        if (rxcb->amsdu)
            skb_queue_splice_tail_init(&rxcb->amsdu->msdus, &msdus);
        __skb_queue_tail(&msdus, skb);
    }
    AML_DBG("preq: +%d msdus\n", skb_queue_len(&msdus));

    spin_lock_bh(&rx->napi_preq.lock);
    skb_queue_splice_tail(&msdus, &rx->napi_preq);
    AML_PROF_CNT(preq, skb_queue_len(&rx->napi_preq));
    spin_unlock_bh(&rx->napi_preq.lock);
    napi_schedule(&rx->napi);
}

void aml_reo_forward(struct aml_rx *rx, struct sk_buff_head *frames)
{
    aml_sdio_usb_rx_napi_preq_append(rx, frames);
}

void aml_reo_session_add_to_aging_list(struct aml_reo_session *reo)
{
    struct aml_rx *rx = reo->rx;

    BUG_ON(!rx);
    list_add_tail(&reo->aging_entry, &rx->reo_aging.list);
}

// Maximum time we can wait for a fragment (in us)
#define DEFRAG_MAX_WAIT               (100000)
// Maximum payload size (1.6k > MTU)
#define DEFRAG_MAX_PAYLOAD_SIZE       (1600)

struct aml_skb_rxcb_frag {
    unsigned long deadline;     /* in jiffies. */

    u16 sn :12;                 /* expected sequence number */
    u16 fn :4;                  /* expected fragment number */
};

static inline struct aml_skb_rxcb_frag *AML_SKB_RXCB_FRAG(struct sk_buff *skb)
{
    BUILD_BUG_ON(sizeof(struct aml_skb_rxcb_frag) > sizeof(skb->cb));
    return (struct aml_skb_rxcb_frag *)skb->cb;
}

static inline struct sk_buff *aml_rx_alloc_skb(struct aml_rx *rx, int len)
{
    struct sk_buff *skb = dev_alloc_skb(rx->skb_head_room + len);

    if (skb)
        skb_reserve(skb, rx->skb_head_room);
    return skb;
}

static inline int aml_frag_skb_append(struct sk_buff *skb, struct aml_frag frags[2])
{
    /* skip ethernet header if not first fragment */
    int offset = skb->len ? sizeof(struct ethhdr) : 0;
    int len = frags[0].len + frags[1].len - offset;

    if (skb->len + len > DEFRAG_MAX_PAYLOAD_SIZE)
        return -1;

    aml_frags_copy(frags, offset, skb_put(skb, len), len);
    return 0;
}

static inline struct sk_buff *aml_defrag_handle(struct aml_rx *rx,
                                                struct sk_buff **pfrag,
                                                struct aml_rhd_ext *rhd_ext,
                                                struct aml_frag frags[2])
{
    struct sk_buff *skb = *pfrag;
    struct aml_skb_rxcb_frag *rxcb = (skb && skb->len) ? AML_SKB_RXCB_FRAG(skb) : NULL;

    if (rxcb && (rxcb->sn != rhd_ext->sn || time_after(jiffies, rxcb->deadline))) {
        AML_RLMT_NOTICE("drop staled fragment %d.%d, new fragment: %d.%d\n",
                        rxcb->sn, rxcb->fn, rhd_ext->sn, rhd_ext->fn);
        /* don't free skb, reuse it for future fragment */
        rxcb = NULL;
        skb_trim(skb, 0);
    }

    if (rhd_ext->fn == 0) { /* the first fragment */
        if (!skb) {
            skb = aml_rx_alloc_skb(rx, DEFRAG_MAX_PAYLOAD_SIZE);
            if (!skb) {
                AML_RLMT_ERR("no skb for fragment %d.%d: %*ph\n",
                             rhd_ext->sn, rhd_ext->fn, frags[0].len, frags[0].data);
                return NULL;
            }
            *pfrag = skb;   /* save it */
        }
        rxcb = AML_SKB_RXCB_FRAG(skb);
        rxcb->sn = rhd_ext->sn;
        rxcb->fn = 0;
        rxcb->deadline = jiffies + usecs_to_jiffies(DEFRAG_MAX_WAIT);
    } else if (!rxcb || rxcb->fn != rhd_ext->fn) {
        if (rxcb)
            AML_RLMT_NOTICE("expected fragment %d.%d\n", rhd_ext->sn, rhd_ext->fn);
        AML_RLMT_NOTICE("drop unexpected fragment %d.%d: %*ph\n",
                        rhd_ext->sn, rhd_ext->fn, frags[0].len, frags[0].data);
        return NULL;
    }

    if (aml_frag_skb_append(skb, frags) < 0) {
        skb_trim(skb, 0);
        AML_RLMT_WARN("drop fragment %d.%d, total length %d + %d exceeds %d: %*ph\n",
                      rhd_ext->sn, rhd_ext->fn,
                      skb->len, frags[0].len + frags[1].len, DEFRAG_MAX_PAYLOAD_SIZE,
                      frags[0].len, frags[0].data);
        return NULL;
    }

    ++rxcb->fn;

    if (rhd_ext->morefrag)
        return NULL;    /* wait for the next fragment */

    /* done. already got every fragment. */
    *pfrag = NULL;  /* this skb will be returned */
    return skb;
}

static struct sk_buff *aml_sdio_usb_rx_desc_to_mpdu(struct aml_rx *rx,
                                                    struct rxdesc *rxdesc,
                                                    int frag0)
{
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    struct rx_hd *rhd = &rxdesc->dma_hdrdesc.hd;
    struct hw_rxhdr *rhd_hw = (struct hw_rxhdr *)&rhd->frmlen;
    int frmlen = rhd->frmlen;
    u8 *payload = (u8*)rxdesc + RX_DESC_SIZE + (frmlen ? RX_PD_LEN : 0);
    struct aml_frag frags[2] = { { .data = payload + rhd->payl_offset, .len = frmlen }, { 0 } };
    struct sk_buff_head msdus;
    struct sk_buff *mpdu = NULL;
    struct aml_sta *sta = NULL;
    struct aml_skb_rxcb *rxcb;
    int count = 0;

    /* discard it after informing upper layer */
    if (rhd->status & RX_STAT_SPURIOUS) {
        struct aml_vif *aml_vif = aml_rx_get_vif(aml_hw, rhd_hw->flags_vif_idx);

        if (aml_vif) {
            struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)payload;

            cfg80211_rx_spurious_frame(aml_vif->ndev, hdr->addr2, GFP_ATOMIC);
        }
        return NULL;
    }

    if (rhd_hw->flags_sta_idx != AML_STA_ID_UNKNOWN)
        sta = aml_sta_get(aml_hw, rhd_hw->flags_sta_idx);

    aml_rx_vector_convert(aml_hw->machw_type, &rhd_hw->hwvect.rx_vect1, &rhd_hw->hwvect.rx_vect2);
    aml_rx_statistic(aml_hw, &rhd_hw->hwvect);
    if (sta)
        aml_rx_sta_stats(aml_hw, sta, &rhd_hw->hwvect);

    if (frag0) {
        int len0 = (u8 *)rxdesc + frag0 - frags[0].data;

        if (len0 <= 0) {
            BUG_ON(1);
        } else {
            frags[0].len = len0;
            frags[1].data = rx->buf;
            frags[1].len = frmlen - len0;
        }
    }

    if (rhd->status == RX_STAT_DEFRAG) {
        if (!sta) {
            AML_RLMT_WARN("no STA (%d) for fragment (%d/%d): %*ph\n", rhd_hw->flags_sta_idx,
                          frags[0].len, frmlen, frags[0].len, frags[0].data);
        } else {
            struct aml_rhd_ext *rhd_ext = (struct aml_rhd_ext *)payload;
            int tid = rhd_ext->qos ? rhd_hw->flags_user_prio : IEEE80211_NUM_UPS;

            mpdu = aml_defrag_handle(rx, &sta->frags[tid], rhd_ext, frags);
        }
    } else if (rhd_hw->flags_is_amsdu) {
        __skb_queue_head_init(&msdus);
        aml_amsdu_to_msdu(&msdus, frags, rx->skb_head_room, NULL, NULL);
        mpdu = __skb_dequeue_tail(&msdus);
        count = skb_queue_len(&msdus);
    } else {
        mpdu = aml_rx_alloc_skb(rx, frmlen);
        if (mpdu)
            aml_frags_copy(frags, 0, skb_put(mpdu, frmlen), frmlen);
        else
            AML_RLMT_ERR("no skb for mpdu %d: %*ph\n", frmlen, frags[0].len, frags[0].data);
    }

    if (!mpdu)
        return NULL;

    if (!rhd_hw->flags_is_80211_mpdu) {
        if (count >= ARRAY_SIZE(aml_hw->stats->amsdus_rx))
            aml_hw->stats->amsdus_rx[ARRAY_SIZE(aml_hw->stats->amsdus_rx) - 1]++;
        else
            aml_hw->stats->amsdus_rx[count]++;
    }

    rxcb = AML_SKB_RXCB(mpdu);

    rxcb->amsdu = NULL;
    rxcb->sta_idx = rhd_hw->flags_sta_idx;
    rxcb->dst_idx = rhd_hw->flags_dst_idx;
    rxcb->vif = rhd_hw->flags_vif_idx;
    rxcb->tid = rhd_hw->flags_user_prio;
    rxcb->is_mpdu = rhd_hw->flags_is_80211_mpdu;
    rxcb->is_4addr = rhd_hw->flags_is_4addr;
    if (rhd->payl_offset < sizeof(struct aml_rhd_ext)) {
        rxcb->rhd_ext.qos = 0;
    } else {
        struct aml_rhd_ext *src = (struct aml_rhd_ext *)payload;

        rxcb->rhd_ext = *src;
        if (src->pn_present)
            rxcb->rhd_ext.pn[0] = src->pn[0];
    }

    if (count) {
        struct sk_buff *skb;
        /* use the head room of the last msdu, it's a little bit dirty, but efficient */
        struct aml_rx_amsdu *amsdu = (struct aml_rx_amsdu *)mpdu->head;

        __skb_queue_head_init(&amsdu->msdus);
        skb_queue_splice_tail(&msdus, &amsdu->msdus);
        amsdu->last = mpdu;

        rxcb->amsdu = amsdu;

        /* clone it to each msdu */
        skb_queue_walk(&amsdu->msdus, skb)
            *AML_SKB_RXCB(skb) = *rxcb;
    }

    return mpdu;
}

static inline int aml_sdio_usb_rx_desc_handle(struct aml_rx *rx,
                                              struct sk_buff_head *frames,
                                              struct rxdesc *rxdesc,
                                              int frag0)
{
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    struct rx_hd *rhd = &rxdesc->dma_hdrdesc.hd;
    struct hw_rxhdr *hw_rxhdr = (struct hw_rxhdr *)&rhd->frmlen;
    struct sk_buff *skb;
    struct aml_skb_rxcb *rxcb;

#define AML_RX_STATUS_UNSUPPORTED   (RX_STAT_ETH_LEN_UPDATE | RX_STAT_COPY | \
                                     RX_STAT_MONITOR | RX_STAT_DELETE)

    if (rhd->status != RX_STAT_HOST_REO &&
        rhd->status != RX_STAT_DEFRAG &&
        (rhd->status & AML_RX_STATUS_UNSUPPORTED)) {
        AML_RLMT_WARN("invalid rx status %x [%d]: %32ph\n",
                      rhd->status, rhd->frmlen, (u8*)rxdesc + RX_DESC_SIZE + RX_PD_LEN);
        return 0;
    }

#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
    /* auto-learning, if firmware is configured to reorder in host */
    if (rhd->status == RX_STAT_HOST_REO)
        aml_sdio_usb_host_reo_detected(rx);
#else
    if (rhd->status == RX_STAT_ALLOC) {
        AML_ERR("SDIO/USB firmware reorder is enabled, but driver doesn't support!");
        return 0;
    }
#endif

    skb = aml_sdio_usb_rx_desc_to_mpdu(rx, rxdesc, frag0);
    if (!skb)
        return 0;

    rxcb = AML_SKB_RXCB(skb);
    if (rxcb->is_mpdu) {  /* management / control frame */
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;

        /* actually BAR is a control frame */
        if (ieee80211_is_back_req(mgmt->frame_control))
            aml_reo_bar_process(rx, aml_sta_get(aml_hw, rxcb->sta_idx), skb);
        else if (ieee80211_is_beacon(mgmt->frame_control) ||
                 ieee80211_is_probe_resp(mgmt->frame_control))
            aml_scan_rx(aml_hw, hw_rxhdr, skb);   /* FIXME: skb_clone() while aml_scan_rx() */
        /* coverity[TAINTED_SCALAR] */
        aml_rx_mgmt_any(aml_hw, skb, hw_rxhdr);
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
    } else if (!rx->host_reorder) {
        if (!rxcb->amsdu &&
                (aml_filter_sp_data_frame((void *)(rhd + 1), skb->len - sizeof(*rhd),
                                          NULL, SP_STATUS_RX) & AML_PKT_SP_RX)) {
            /* WAR: bypass special frames for firmware reorder */
            __skb_queue_tail(frames, skb);
        } else if (rhd->status == RX_STAT_ALLOC) {
            /* for f/w reorder, tid and sn is embedded in rhd */
            rxcb->tid = rhd->reorder.tid;
            rxcb->rhd_ext.sn = rhd->reorder.hostid; /* actually it's sn + 1 */

            aml_sdio_usb_fw_reo_enqueue(rx, skb);
        } else {
            BUG_ON(!(rhd->status & RX_STAT_FORWARD));
            __skb_queue_tail(frames, skb);
        }

        /* handle reorder instruction from firmware */
        aml_sdio_usb_fw_reo_inst_handle(rx,
                &(struct fw_reo_inst){
                    .hostid = rhd->reorder.hostid,
                    .tid = rhd->reorder.tid,
                    .len = rhd->reorder.len,
                });
#endif
    } else {
        struct aml_reo_session *reo = NULL;

        /*
         * if rx status != RX_STAT_HOST_REO, the frame is not under a RX BA,
         * or it's received before the BA(reo) session is created,
         * do not try to get the reo session or handle the frame under it.
         */
        if (rxcb->rhd_ext.qos && rhd->status == RX_STAT_HOST_REO)
            reo = aml_reo_session_get(aml_sta_get(aml_hw, rxcb->sta_idx), rxcb->tid);
        if (reo) {
            aml_reo_enqueue(reo, skb, frames);
            aml_reo_session_put(reo);
        } else {
            if (rhd->status == RX_STAT_HOST_REO) {
                net_info_ratelimited("got a frame under BA, but no reo session! "
                         "forward it by default. sta %u tid %u qos %d sn %u\n",
                         rxcb->sta_idx, rxcb->tid, rxcb->rhd_ext.qos, rxcb->rhd_ext.sn);
            }
            __skb_queue_tail(frames, skb);
        }
    }
    return 1;
}

static inline bool aml_sdio_usb_rx_pos_in_range(struct aml_rx *rx, addr32_t pos, unsigned int len)
{
    if (pos >= RXBUF_START_ADDR && (pos + len) <= rx->fw.end)
        return true;
    AML_ERR("f/w pos [%x, %x] is out of range [%x, %x]\n",
            pos, pos + len, RXBUF_START_ADDR, rx->fw.end);
    return false;
}

static inline struct rxdesc *aml_sdio_usb_rx_desc_next(struct aml_rx *rx,
                                                       struct rxdesc *desc,
                                                       int *frag0)
{
    int pos = *aml_rx_desc_next_ptr(desc);
    int wrap = pos & AML_RX_WRAP_FLAG;

    pos &= ~AML_RX_WRAP_FLAG;
    /* if pos is not f/w position, reach the last desc */
    AML_DBG("head,tail,pos: %7d, %7d, 0x%x (%d)\n", rx->head, rx->tail, pos, pos);
    if (pos >= 0 && pos < rx->buf_sz) {
        if (!wrap) {
            *frag0 = 0;
        } else {
            *frag0 = rx->frag0;
            rx->frag0 = 0;
        }
        return (struct rxdesc *)(rx->buf + pos);
    }

    if (pos < RXBUF_START_ADDR || pos >= rx->layouts[AML_RX_BUF_EXPAND].rx_end)
        AML_ERR("head, tail, end, pos: %7d, %7d, %7d, %7d, invalid pos 0x%x!\n",
                rx->head, rx->tail, rx->buf_sz, pos, pos);
    return NULL;
}

static inline int aml_sdio_usb_rx_buf_put(struct aml_rx *rx, struct sk_buff_head *frames)
{
    int done = 0;
    int frag0 = 0;
    struct rxdesc *rxdesc = (struct rxdesc *)(rx->buf + rx->tail);

    AML_PROF_HI(rx_buf_put);

    while ((rxdesc = aml_sdio_usb_rx_desc_next(rx, rxdesc, &frag0))) {
        struct rx_hd *rhd = &rxdesc->dma_hdrdesc.hd;
        u8 *p = (u8 *)rxdesc;

        AML_DBG("head,tail,pos: %7d, %7d, %7ld\n", rx->head, rx->tail, (unsigned long)(p - rx->buf));

        /* fragment end or packet data end */
        if (frag0)
            p += frag0;
        else
            p += RX_DESC_SIZE + (rhd->frmlen ? RX_PD_LEN : 0) + rhd->payl_offset + rhd->frmlen;
        if (p >= rx->buf + rx->buf_sz) {
            AML_ERR("buf_sz %d end %ld frag0 %d len %d rx->head/tail %d/%d\n",
                    rx->buf_sz, (unsigned long)(p - rx->buf), frag0, rhd->frmlen, rx->head, rx->tail);
            BUG_ON(1);
        }

        /* if status is cleared by firmware, then ignore it. refer to rxl_mpdu_free() */
        if (rhd->status && rhd->frmlen)
            done += aml_sdio_usb_rx_desc_handle(rx, frames, rxdesc, frag0);

        rx->tail = (u8 *)rxdesc - rx->buf;
        AML_PROF_CNT(tail, rx->tail);
    }

    /* new buffer is available, wake up the irq task */
    if (test_bit(AML_RX_STATE_NO_BUF, &rx->state)) {
        clear_bit(AML_RX_STATE_NO_BUF, &rx->state);
        //AML_PROF_CNT(no_buf, 0);

        rx->irq_pending = IPC_IRQ_E2A_RXDESC;
#ifdef CONFIG_AML_SDIO_IRQ_VIA_GPIO
        up(&aml_rx2hw(rx)->aml_irq_sem);
#else
        /* FIXME: trigger an interrupt */
#endif
    }

    AML_PROF_LO(rx_buf_put);
    return done;
}

static inline int aml_sdio_usb_rx_task(struct aml_hw *aml_hw)
{
    struct aml_rx *rx = &aml_hw->rx;

    aml_sched_rt_set(SCHED_RR, AML_TASK_PRI);
    while (!aml_hw->aml_rx_task_quit) {
        struct sk_buff_head frames;

        if (down_interruptible(&aml_hw->aml_rx_sem)) {
            /* interrupted, exit */
            AML_INFO("wait aml_rx_sem fail!\n");
            break;
        }

        __skb_queue_head_init(&frames);

        aml_sdio_usb_rx_buf_put(rx, &frames);
        aml_reo_aging(&rx->reo_aging, &frames);

        aml_sdio_usb_rx_napi_preq_append(rx, &frames);
    }

    if (aml_hw->aml_rx_completion_init) {
        aml_hw->aml_rx_completion_init = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 16, 20)
        complete_and_exit(&aml_hw->aml_rx_completion, 0);
#else
        complete(&aml_hw->aml_rx_completion);
#endif
    }

    return 0;
}

int aml_rx_task(void *data)
{
    return aml_sdio_usb_rx_task((struct aml_hw *)data);
}

/*
 * NB: any bus transfer should be done in aml_irq_task context.
 * actually it should be renamed as aml_bus_trans.
 * in future, TX operation also should be moved into this task.
 */
#undef AML_MODULE
#define AML_MODULE  RX_IRQ

static inline void __aml_sdio_usb_rx_confirm(struct aml_rx *rx, addr32_t confirm)
{
    uint32_t cmd[2] = { 1, confirm };

    AML_PROF_HI(rx_cfm);
    hi_sram_write(aml_rx2hw(rx), CMD_DOWN_FIFO_FDH_ADDR, cmd, sizeof(cmd));
    AML_PROF_LO(rx_cfm);
}

int aml_sdio_usb_fw_rx_head_ind(struct aml_rx *rx, addr32_t fw_rx_head)
{
    addr32_t head;

    if (fw_rx_head == 0)    /* device is initializing */
        return 0;

    /* remove extra flags */
    head = SHARED_MEM_BASE_ADDR | (fw_rx_head & ~AML_RX_BUF_FW_FLAGS);
    if (!aml_sdio_usb_rx_pos_in_range(rx, head, 0)) {
        /* FIXME: USB firmware should update its rx head after RX buffer scaled */
        AML_ERR("fw_rx_head %x head %x\n", fw_rx_head, head);
        return -1;
    }

    if (fw_rx_head & RX_WRAP_TEMP_FLAG)
        head |= AML_RX_WRAP_FLAG;
    rx->fw.head = head;

    rx->fw.state = fw_rx_head & (FW_BUFFER_NARROW | FW_BUFFER_EXPAND);

    return 0;
}

/* FIXME: move this function to aml_tx_sdio_usb.c */
static inline void aml_sdio_usb_tx_page_apply(struct aml_hw *aml_hw, u32 tx_page)
{
    struct tx_task_param *tx_param = &aml_hw->g_tx_param;

    spin_lock_bh(&aml_hw->tx_buf_lock);
    tx_param->tx_page_tot_num = tx_page;
    tx_param->tx_page_free_num = tx_page;
    spin_unlock_bh(&aml_hw->tx_buf_lock);
}

static void aml_shared_mem_layout_appy(struct aml_rx *rx, enum aml_rx_buf_layout layout)
{
    rx->fw.end = rx->layouts[layout].rx_end;
    aml_sdio_usb_tx_page_apply(aml_rx2hw(rx), rx->layouts[layout].tx_page);
}

static void aml_sdio_usb_rx_confirm(struct aml_rx *rx, uint32_t received)
{
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    u32 flags = rx->fw.confirm.flags;
    u32 state = rx->fw.state | flags;
    addr32_t confirm;

    switch (state) {
    case FW_BUFFER_NARROW: {
        addr32_t end = rx->layouts[AML_RX_BUF_NARROW].rx_end;

        /* RX is stopped and RX buffer is drained. it's safe to reset device RX pointer */
        if ((rx->fw.head & ~AML_RX_WRAP_FLAG) >= end) {
            rx->fw.head = rx->fw.tail = RXBUF_START_ADDR;
            /* FIXME: let firmware reset it as RXBUF_START_ADDR, just like USB */
            if (aml_bus_type == SDIO_MODE)
                AML_REG_WRITE(RXBUF_START_ADDR & SDIO_ADDR_MASK, aml_hw->plat, 0, RG_WIFI_IF_FW2HST_IRQ_CFG);
        }
        aml_shared_mem_layout_appy(rx, AML_RX_BUF_NARROW);
        flags = RX_REDUCE_READ_RX_DATA_FINISH;
        break;
    }
    case FW_BUFFER_NARROW | RX_REDUCE_READ_RX_DATA_FINISH:
        /* nothing to do, wait for firmware to clear FW_BUFFER_NARROW */
        break;
    case RX_REDUCE_READ_RX_DATA_FINISH:
        flags = HOST_RXBUF_REDUCE_FINISH;
        AML_WARN("RX buffer reduced!\n");
        break;
    case FW_BUFFER_EXPAND:
        aml_shared_mem_layout_appy(rx, AML_RX_BUF_EXPAND);
        flags = RX_ENLARGE_READ_RX_DATA_FINISH;
        break;
    case FW_BUFFER_EXPAND | RX_ENLARGE_READ_RX_DATA_FINISH:
        /* nothing to do, wait for firmware to clear FW_BUFFER_EXPAND */
        break;
    case RX_ENLARGE_READ_RX_DATA_FINISH:
        flags = HOST_RXBUF_ENLARGE_FINISH;
        AML_WARN("RX buffer enlarged!\n");
        break;
    case 0:
        flags = 0;
        break;
    default:
        AML_ERR("invalid firmware state or confirm flags! (%x/%x)\n",
                rx->fw.state, rx->fw.confirm.flags);
        break;
    }
    if (flags == HOST_RXBUF_REDUCE_FINISH || flags == HOST_RXBUF_ENLARGE_FINISH)
        rx->fw.confirm.flags = 0; /*host switch has been completed, clear the flag.*/
    else
        rx->fw.confirm.flags = flags;   /* save new flags*/

    confirm = flags | rx->fw.tail;
    if (rx->fw.confirm.last == confirm)
        return;

    if ((rx->fw.confirm.last ^ confirm) & (AML_RX_BUF_FW_FLAGS | AML_RX_BUF_HOST_FLAGS))
        AML_NOTICE("RX confirm %x last %x\n", confirm, rx->fw.confirm.last);
    rx->fw.confirm.last = confirm;
    if (aml_bus_type == USB_MODE) {
        if (state || flags || received >= ((rx->fw.end - RXBUF_START_ADDR) / 4)
            || auc_cmd_rxrd_set(confirm) != 0)
            __aml_sdio_usb_rx_confirm(rx, confirm);
    } else if (aml_bus_type == SDIO_MODE) {
        if (aml_hw->state == WIFI_SUSPEND_STATE_NONE)
            __aml_sdio_usb_rx_confirm(rx, confirm);
    }
}

static inline void aml_stats_rx_trans_update(struct aml_stats *stats, unsigned int len)
{
    int i;

    ++stats->rx_trans_total;
    for (i = 0; i < AML_RX_TRANS_RANK_NUM - 1; i++) {
        if (len >= (AML_RX_TRANS_RANK_SIZE_0 >> i)) {
            ++stats->rx_trans[i];
            return;
        }
    }
    ++stats->rx_trans[i];
}

static inline int aml_sdio_usb_rx_buf_get(int head, int tail, int buf_sz, int len, int frag0)
{
    int free;

    head += frag0;        /* include last mpdu's 1st fragment that's already read */
    free = tail - head;

    /* reserved additional memory space for aml_sdio_cmd53() */
    if (len > SDIO_BLKSIZE)
        len = ALIGN(len, SDIO_BLKSIZE);

    if (free > 0) {
        /* head < tail */
        if (free > len)
            return head;
    } else {
        /* tail <= head */
        if (head + len < buf_sz)
            return head;

        if (frag0 < AML_RX_FRAG0_MINI_SIZE) {
            /* later 1st fragment of the last mpdu will be moved to the beginning. */
            len += frag0;
        }
        if (len < tail)
            return 0;
    }
    return -1;
}

static void aml_sdio_usb_rx_error(struct aml_rx *rx)
{
    if (test_bit(AML_RX_STATE_RESET, &rx->state))
        return;

    AML_NOTICE("bus error detected!\n");
    set_bit(AML_RX_STATE_RESET, &rx->state);
    clear_bit(AML_RX_STATE_START, &rx->state);
}

static int aml_sdio_usb_rx_desc_fetch(struct aml_rx *rx, addr32_t fw_pos, int len)
{
    int last = rx->last;
    int frag0 = rx->fw.frag0;
    int pos = aml_sdio_usb_rx_buf_get(rx->head, rx->tail, rx->buf_sz, len, frag0);
    int received;
    struct rxdesc *rxdesc;
    addr32_t *pnext = NULL;     /* the next pointer of previous rx desc */
    int pos0;
    bool reach_end = rx->fw.end == fw_pos + len;

    if (pos < 0) {
        AML_ERR("no RX buffer! host [%d, %d) fw [%x, %x) %x + %d frag0 %d\n",
                rx->tail, rx->head, rx->fw.tail, rx->fw.head, fw_pos, len, frag0);
        return 0;               /* host rx buffer is not enough */
    }

    AML_DBG("===\n");
    AML_DBG("fw [%x, %x) fw_pos %x | last %d pos %d len %d, frag0 %d\n",
            rx->fw.tail, rx->fw.head, fw_pos, last, pos, len, frag0);

    BUG_ON(!aml_sdio_usb_rx_pos_in_range(rx, fw_pos, len));

    if (pos == 0) {
        if (frag0 < AML_RX_FRAG0_MINI_SIZE) {
            if (frag0) {
                AML_NOTICE("move 1st fragment of the mpdu from %u (%d bytes) to the beginning\n",
                           rx->head, frag0);
                memmove(rx->buf, rx->buf + rx->head, frag0);
                pos = frag0;            /* skip this part that's already read */
            }
            rx->frag0 = 0;  /* as usual the mpdu is only 1 fragment in host buffer */
        } else if (rx->frag0 == 0) {
            /* the MPDU is stored into 2 fragments in host */
            rx->frag0 = frag0;
        } else {
            /*
             * the MPDU is stored into 2 fragments in both f/w and host.
             *  the length of 1st fragment in f/w is rx.fw.frag0.
             *
             * host reads the mpdu in 3 times, this is 3rd read:
             *  1. length: rx->frag0 (1st fragment at the end of host buffer)
             *  2. length: rx.fw.frag0 - rx->frag0 (at the beginning of host buffer, reach f/w end)
             *  3. now read the remainder
             */
            BUG_ON(frag0 < rx->frag0);
            pos = frag0 - rx->frag0;    /* skip the part that's already read */
        }
    }

    //AML_PROF_CNT(read, len);
    received = hi_rx_buffer_read(aml_rx2hw(rx), rx->buf + pos, fw_pos, len);
    //AML_PROF_CNT(read, 0);
    if (received <= 0) {
        AML_ERR("RX read failed(%d)! host [%d, %d) fw [%x, %x) %x + %d frag0 %d\n",
                received, rx->tail, rx->head, rx->fw.tail, rx->fw.head, fw_pos, len, frag0);
        /* bus error, wait for recovery done */
        aml_sdio_usb_rx_error(rx);
        return 0;
    }

    len = received;
    if (frag0) {
        /* align them to rx->head */
        len += frag0;
        pos -= frag0;
        fw_pos -= frag0;
    }

    /*
     * go through new RX data to update its next pointer
     */
    pos0 = pos < 0 ? rx->head : pos;
    rxdesc = (struct rxdesc *)(rx->buf + pos0);
    while (len > 0) {
        struct rx_hd *rhd = &rxdesc->dma_hdrdesc.hd;
        int pkt_len = rhd->payl_offset + rhd->frmlen;   /* only RX desc + payload */
        int tot_len;                                    /* RX desc + payload + trailer */
        addr32_t next_pos;
        addr32_t *next_ptr;

        if (len < RX_DESC_SIZE) {
            /* next pointer is unavailable yet */
            if (reach_end)
                /* discard it if it's the padding at the end of f/w RX buffer */
                len = 0;
            break;
        }

        /* W2: if no payload, then no payload descriptor is attached */
        pkt_len = RX_DESC_SIZE + (pkt_len ? RX_PD_LEN : 0) + ALIGN(pkt_len, sizeof(u32));

        next_ptr = aml_rx_desc_next_ptr(rxdesc);
        next_pos = *next_ptr;
        tot_len = next_pos - fw_pos;
        if (tot_len < 0 && next_pos == RXBUF_START_ADDR)
            tot_len = rx->fw.end - fw_pos;

        AML_DBG("fw|pos %x|%d len %d, pkt_len %d, frag0 %d tot_len %d next %x to end %x(%d)\n",
                fw_pos, pos, len, pkt_len, frag0,
                tot_len, next_pos, rx->fw.end - fw_pos, rx->fw.end - fw_pos);
        if (!aml_sdio_usb_rx_pos_in_range(rx, next_pos, 0)) {
            AML_ERR("fw|pos %x|%d len %d, pkt_len %d, frag0 %d tot_len %d next %x to end %x(%d) %d\n",
                    fw_pos, pos, len, pkt_len, frag0,
                    tot_len, next_pos, rx->fw.end - fw_pos, rx->fw.end - fw_pos, reach_end);
            BUG_ON(1);
        }

        /* wrap in device */
        if (tot_len < 0) {
            AML_NOTICE("fw_pos %x len %d/%d%s\n",
                       fw_pos, len, tot_len, reach_end ? ", reach end": "");
            if (!reach_end) {
                AML_DBG("wait for the rest before f/w RX buffer end(fw_pos: %x len %d/%d)\n",
                        fw_pos, len, tot_len);
                break;
            }

            if (RX_PD_LEN && len < RX_DESC_SIZE + RX_PD_LEN) {
                len = RX_DESC_SIZE;
#ifndef CONFIG_AML_W2L_RX_MINISIZE
            } else if (((struct hw_rxhdr *)&rhd->frmlen)->flags_is_80211_mpdu) {
                /* management or control frame (no msdu/amsdu) */
                len = RX_DESC_SIZE + RX_PD_LEN;
#endif
            } else if (len < RX_DESC_SIZE + RX_PD_LEN + UNWRAP_SIZE) {
                len = RX_DESC_SIZE + RX_PD_LEN;
            } else {
                rx->fw.skip = RX_PD_LEN;
                if (len > pkt_len + sizeof(u64) + sizeof(u32)) {    /* pkt_len + icv + fcs */
                    AML_ERR("new frag0/skip: %d/%d, pkt len %d, fw pos %x next %x\n",
                            len, rx->fw.skip, pkt_len, fw_pos, next_pos);
                    BUG_ON(1);
                }
            }
            AML_INFO("new frag0/skip: %d/%d, pkt len %d\n", len, rx->fw.skip, pkt_len);
            break;  /* later continue after got the rest */
        }

        if (tot_len < pkt_len) {
            AML_ERR("fw pos %x next %x frag0 %d skip %d last %d "
                    "wrong len/tot_len/pkt_len: %d/%d/%d!\n",
                    fw_pos, next_pos, frag0, rx->fw.skip, last, len, tot_len, pkt_len);
            BUG_ON(1);
            len = 0;    /* drop the remainder */
            break;
        }

        if (len < tot_len) {
            AML_NOTICE("rx data is not completed!\n");
            break;
        }

        /* replace f/w position with s/w position */
        if (pnext)
            *pnext = pos;

        pnext = next_ptr;
        last = pos < 0 ? pos0 : pos;

        /* move to next */
        len -= tot_len;
        pos += tot_len;
        fw_pos += tot_len;
        if (pos < 0 || pos >= rx->buf_sz || !aml_sdio_usb_rx_pos_in_range(rx, fw_pos, 0)) {
            AML_ERR("pos %d fw_pos %x tot_len %d len %d\n", pos, fw_pos, tot_len, len);
            BUG_ON(1);
        }

        rxdesc = (struct rxdesc *)(rx->buf + pos);
    }

    if (rx->last != last) {
        /* update the next position of previous "last", then consumer can take new data */
        if (pos0 > pos)
            pos0 |= AML_RX_WRAP_FLAG;
        *aml_rx_desc_next_ptr(rx->buf + rx->last) = pos0;
        rx->last = last;
    }

    rx->fw.frag0 = len;
    if (pos >= 0)
        rx->head = pos;
    AML_PROF_CNT(head, rx->head);

    return received;
}

static inline u32 aml_sdio_usb_fw_tail_restart(struct aml_rx *rx, addr32_t rx_tail)
{
    BUG_ON(rx->fw.tail == rx->fw.head);
    if (rx_tail == rx->fw.end || rx_tail - rx->fw.frag0 + RX_DESC_SIZE > rx->fw.end) {
        if (rx_tail != rx->fw.end) {
            AML_NOTICE("fw[%x, %x) frag0 %d, drop %d padding at device RX buffer end.\n",
                       rx->fw.tail, rx->fw.head, rx->fw.frag0,
                       rx->fw.end - rx_tail + rx->fw.frag0);
            rx->fw.frag0 = 0;
        }
        /* restart from RXBUF_START_ADDR */
        rx_tail = RXBUF_START_ADDR;
        rx->fw.tail = RXBUF_START_ADDR | (rx->fw.head & AML_RX_WRAP_FLAG);
    }
    return rx_tail;
}

int aml_sdio_usb_rxdataind(struct aml_rx *rx)
{
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    addr32_t rx_head = rx->fw.head & ~AML_RX_WRAP_FLAG;
    addr32_t rx_tail = rx->fw.tail & ~AML_RX_WRAP_FLAG;
    int received = 0;

    rx->irq_pending = 0;

    if (bus_state_detect.bus_err
#ifdef CONFIG_AML_RECOVERY
        || aml_recy_flags_chk(AML_RECY_FW_ONGOING)  /* recovering, do nothing */
#endif
        ) {
        aml_sdio_usb_rx_error(rx);
        return -1;
    }

    if (!test_bit(AML_RX_STATE_START, &rx->state))
        return -1;

    if (test_bit(AML_RX_STATE_RESET, &rx->state)) {
#define NXMAC_RX_BUF_1_RD_PTR_ADDR   0x60B081D0
        addr32_t new_tail = hi_reg_read(aml_hw, NXMAC_RX_BUF_1_RD_PTR_ADDR);

        if (!aml_sdio_usb_rx_pos_in_range(rx, new_tail & ~AML_RX_WRAP_FLAG, 0)) {
            AML_WARN("device rx tail %x is invalid!\n", new_tail);
            return -1;
        }
        AML_NOTICE("fw[%x, %x) rx buffer is reset, new tail is %x. drop frag %d, %d\n",
                   rx->fw.tail, rx->fw.head, new_tail, rx->fw.frag0, rx->frag0);
        clear_bit(AML_RX_STATE_RESET, &rx->state);
        rx->frag0 = 0;
        rx->fw.frag0 = 0;
        rx->fw.skip = 0;
        rx->fw.tail = new_tail;
        rx_tail = new_tail & ~AML_RX_WRAP_FLAG;
    }

    if (rx->fw.tail == rx->fw.head) {
        /* no new RX data */
        if (rx->fw.state || rx->fw.confirm.flags) {
            /* but shared memory layout is changing */
            aml_sdio_usb_rx_confirm(rx, 0);
        } else {
            AML_DBG("nothing to do (rx.fw.head %x)\n", rx->fw.head);
        }

        /* FIXME: add reorder aging timer instead of waking up rx task here */
        up(&aml_hw->aml_rx_sem);
        return 0;
    }

    set_bit(AML_RX_STATE_READING, &rx->state);
    rx_tail = aml_sdio_usb_fw_tail_restart(rx, rx_tail);
    while (rx->fw.tail != rx->fw.head) {
        /* max length can be read */
        int len = rx_head - rx_tail;

        if (len <= 0)
            len = rx->fw.end - rx_tail;
        AML_DBG("fw[%x, %x) len %4d - %d end %x\n", rx_tail, rx_head, len, rx->fw.skip, rx->fw.end);

#if 0
        /*
         * 1. each SDIO transfer can't exceed 128k
         * 2. for SDIO, reading more data gets better bus performance.
         */
        if (rx_head > rx_tail && len < SDIO_READ_MAX &&
            !test_bit(AML_RX_STATE_DEFERRED, &rx->state)) {
            /*
             * try to wait for more rx data.
             * later this function will be visited with new f/w rx head
             */
            unsigned long usec = 0;

            if (!received) {
                /*
                 * SDIO clock is 200MHz, x 4 data lines.
                 * its net throughput is about 750Mbps, or 94 bytes/microsecond.
                 */
                usec = (SDIO_READ_MAX - len) / 94;
                if (usec > 100)
                    usleep_range(usec - 50, usec + 50);
            }
            AML_DBG("RX length %d < 128k, delay %d us\n", len, usec);
            set_bit(AML_RX_STATE_DEFERRED, &rx->state);
            break;
        }
        clear_bit(AML_RX_STATE_DEFERRED, &rx->state);
#endif

        /* skip the payload buffer descriptor at the beginning of f/w rx buffer */
        if (rx->fw.skip && rx_tail == RXBUF_START_ADDR) {
            if (len <= rx->fw.skip) {
                AML_DBG("fw[%x, %x) len %d < skip %d\n", rx_tail, rx_head, len, rx->fw.skip);
                BUG_ON(1);
            }
            len -= rx->fw.skip;
            rx_tail += rx->fw.skip;
            rx->fw.tail += rx->fw.skip;
            rx->fw.skip = 0;
        }

        if (len == 0) {
            AML_ERR("fw[%x, %x) len %d", rx_tail, rx_head, len);
            break;
        }

        len = aml_sdio_usb_rx_desc_fetch(rx, rx_tail, len < SDIO_READ_MAX ? len : SDIO_READ_MAX);
        if (len <= 0) {
            /* FIXME: ignore RX interrupt */
            break;
        }

        received += len;
        rx_tail += len;
        rx->fw.tail += len;
        if (rx_tail > rx->fw.end) {
            AML_ERR("fw[%x, %x) rx_tail %x > %x\n", rx->fw.tail, rx->fw.head, rx_tail, rx->fw.end);
            BUG_ON(1);
        }

        if (rx->fw.tail != rx->fw.head)
            rx_tail = aml_sdio_usb_fw_tail_restart(rx, rx_tail);

        up(&aml_hw->aml_rx_sem);
    }

    if (received) {
        aml_stats_rx_trans_update(aml_hw->stats, received);

        /* FIXME: return RX buffer to device earlier */
        aml_sdio_usb_rx_confirm(rx, received);
    }
    clear_bit(AML_RX_STATE_READING, &rx->state);

    return received;
}

void aml_shared_mem_layout_update(struct aml_rx *rx)
{
    struct aml_hw *aml_hw = aml_rx2hw(rx);
    struct aml_sharedmem_layout narrow;
    struct aml_sharedmem_layout expand;

    if (aml_bus_type == SDIO_MODE) {
        narrow.tx_page = SDIO_TX_PAGE_NUM_LARGE;
        expand.tx_page = SDIO_TX_PAGE_NUM_SMALL;

        narrow.rx_end = RXBUF_END_ADDR_SMALL;
        expand.rx_end = RXBUF_END_ADDR_LARGE;
        /* adjust layout by la_enable */
        if (aml_hw->la_enable) {
            narrow.tx_page -= SDIO_LA_PAGE_NUM;
            expand.rx_end = RXBUF_END_ADDR_LA_LARGE;
        }
    } else {
        BUG_ON(aml_bus_type != USB_MODE);
        /* tx_page is different if CONFIG_AML_USB_LARGE_PAGE is defined or not */
        narrow.tx_page = USB_TX_PAGE_NUM_LARGE;
        expand.tx_page = USB_TX_PAGE_NUM_SMALL;

        narrow.rx_end = USB_RXBUF_END_ADDR_SMALL;
        expand.rx_end = USB_RXBUF_END_ADDR_LARGE;
        /* adjust layout by la_enable/usb_trace_enable */
#ifdef CONFIG_AML_USB_LARGE_PAGE
        if (aml_hw->la_enable) {
            narrow.tx_page -= USB_LA_PAGE_NUM;
            expand.rx_end = USB_RXBUF_END_ADDR_LA_LARGE;
        } else if (aml_hw->trace_enable) {
            narrow.tx_page -= USB_TRACE_PAGE_NUM;
            expand.rx_end = USB_RXBUF_END_ADDR_TRACE_LARGE;
        }
#else
        if (aml_hw->la_enable || aml_hw->trace_enable) {
            AML_ERR("FIXME: memory layout if not defined CONFIG_AML_USB_LARGE_PAGE is unknown!\n");
            BUG_ON(1);
        }
#endif
    }
    rx->layouts[AML_RX_BUF_NARROW] = narrow;
    rx->layouts[AML_RX_BUF_EXPAND] = expand;
}

static inline int aml_sdio_usb_rx_msdu_has_pending(struct aml_rx *rx)
{
    /* FIXME: skb_queue_len_lockless */
    return skb_queue_len(&rx->napi_pending) || skb_queue_len(&rx->napi_preq);
}

static inline int aml_sdio_usb_rx_desc_has_pending(struct aml_rx *rx)
{
    int pos = *aml_rx_desc_next_ptr(rx->buf + rx->tail) & ~AML_RX_WRAP_FLAG;

    return (pos >= 0 && pos < rx->buf_sz);
}

static inline int aml_sdio_usb_rx_reading(struct aml_rx *rx)
{
    return test_bit(AML_RX_STATE_READING, &rx->state);
}

static int aml_sdio_usb_rx_drain_check(struct aml_rx *rx,
                                       const char *name, int (*has_pending)(struct aml_rx *rx))
{
    ktime_t start = ktime_get_boottime();

    if (has_pending(rx)) {
        ktime_t now = start;
        ktime_t show = ktime_add_ns(now, NSEC_PER_MSEC);

        do {
            now = ktime_get_boottime();
            if (ktime_after(now, show)) {
                show = ktime_add_ns(now, NSEC_PER_MSEC);
                AML_NOTICE("rx %s is pending\n", name);
            }
            usleep_range(100, 200);
        } while (has_pending(rx));
        AML_ERR("all rx %s is done in %u us.\n", name, (int)ktime_us_delta(now, start));
        return 1;
    }
    return 0;
}

static inline void aml_sdio_usb_rx_napi_enable(struct aml_rx *rx)
{
    if (!test_and_set_bit(AML_RX_STATE_NAPI_EN, &rx->state))
        napi_enable(&rx->napi);
}

static inline void aml_sdio_usb_rx_napi_disable(struct aml_rx *rx)
{
    if (test_and_clear_bit(AML_RX_STATE_NAPI_EN, &rx->state)) {
        napi_synchronize(&rx->napi);
        napi_disable(&rx->napi);
    }
}

int aml_sdio_usb_rx_stop(struct aml_rx *rx)
{
    clear_bit(AML_RX_STATE_START, &rx->state);  /* stop fetch rx desc */

    /* must check them in correct order */
    aml_sdio_usb_rx_drain_check(rx, "reading", aml_sdio_usb_rx_reading);
    aml_sdio_usb_rx_drain_check(rx, "desc", aml_sdio_usb_rx_desc_has_pending);

    if (aml_sdio_usb_host_reo_enabled(rx))
        aml_reo_reset_all(rx);

    aml_sdio_usb_rx_napi_disable(rx);
    aml_sdio_usb_rx_drain_check(rx, "msdu", aml_sdio_usb_rx_msdu_has_pending);

    return 0;
}

void aml_sdio_usb_rx_restart(struct aml_rx *rx)
{
    aml_sdio_usb_rx_napi_enable(rx);

    set_bit(AML_RX_STATE_RESET, &rx->state);
    if (test_and_set_bit(AML_RX_STATE_START, &rx->state))
        return;

    aml_shared_mem_layout_appy(rx, AML_RX_BUF_EXPAND);
}

int aml_sdio_usb_rx_init(struct aml_rx *rx)
{
    size_t buf_sz = PREALLOC_BUF_TYPE_RXBUF_SIZE;
    int head_room = (aml_bus_type == USB_MODE) ? AML_USB_TX_MAX_HEADROOM : AML_SDIO_TX_MAX_HEADROOM;

    if (head_room < sizeof(struct aml_rx_amsdu))
        head_room = sizeof(struct aml_rx_amsdu);
    rx->skb_head_room = ALIGN(head_room, 4);

    aml_shared_mem_layout_update(rx);

#ifdef CONFIG_AML_PREALLOC_BUF_STATIC
    rx->buf = aml_prealloc_get_ex(PREALLOC_BUF_TYPE_RXBUF, buf_sz, &buf_sz);
#else
    rx->buf = kmalloc(buf_sz, GFP_KERNEL);
#endif
    if (!rx->buf)
        return -ENOMEM;

    rx->buf_sz = buf_sz;

    rx->fw.head = RXBUF_START_ADDR;
    rx->fw.tail = RXBUF_START_ADDR;

    aml_shared_mem_layout_appy(rx, AML_RX_BUF_EXPAND);

    /* dummy last */
    rx->last = rx->tail = buf_sz - RX_DESC_SIZE ;
    *aml_rx_desc_next_ptr(rx->buf + rx->last) = RXBUF_START_ADDR;

    INIT_LIST_HEAD(&rx->reo_aging.list);
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
    skb_queue_head_init(&rx->fw_reo.list);
#endif

    /* initialize NAPI */
    skb_queue_head_init(&rx->napi_preq);
    skb_queue_head_init(&rx->napi_pending);
    init_dummy_netdev(&rx->napi_dev);
    netif_napi_add_weight(&rx->napi_dev, &rx->napi,
                          aml_sdio_usb_rx_napi_poll, NAPI_POLL_WEIGHT);

    return 0;
}

void aml_sdio_usb_rx_deinit(struct aml_rx *rx)
{
#ifdef CONFIG_AML_SDIO_USB_FW_REORDER
    aml_sdio_usb_fw_reo_clean(rx);
#endif

    __skb_queue_purge(&rx->napi_preq);
    __skb_queue_purge(&rx->napi_pending);

    aml_sdio_usb_rx_napi_disable(rx);

    netif_napi_del(&rx->napi);

#ifndef CONFIG_AML_PREALLOC_BUF_STATIC
    if (rx->buf)
        kfree(rx->buf);
#endif
    rx->buf = NULL;
}

void aml_rx_sta_deinit(struct aml_rx *rx, struct aml_sta *sta)
{
    int tid;

    aml_reo_sta_deinit(rx, sta);

    for (tid = 0; tid < ARRAY_SIZE(sta->frags); tid++) {
        struct sk_buff *skb = sta->frags[tid];

        sta->frags[tid] = NULL;
        if (skb )
            dev_kfree_skb(skb);
    }
}
