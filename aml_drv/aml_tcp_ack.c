/**
****************************************************************************************
*
* @file aml_tcp_ack.c
*
* Copyright (C) Amlogic, Inc. All rights reserved (2022-2023).
*
* @brief tcp ack.
*
****************************************************************************************
*/

#include <linux/if_ether.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/moduleparam.h>
#include <net/tcp.h>
#include "aml_tcp_ack.h"
#include "aml_tx.h"
#include "aml_compat.h"

unsigned short tcp_checksum(void *data, int len, unsigned int sip, unsigned int dip)
{
    int sum, oddbyte, phlen;
    unsigned short *ptr;

    struct {
        unsigned int sip;
        unsigned int dip;
        unsigned char zero;
        unsigned char prot;
        unsigned short len;
    } pseudo_hdr = { sip, dip, 0, 6 , htons(len & 0xffff) };

    sum = 0;

    phlen = sizeof(pseudo_hdr);
    ptr = (unsigned short *) &pseudo_hdr;

    while (phlen > 1) {
        sum += *ptr++;
        phlen -= 2;
    }

    ptr = (unsigned short *) data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len == 1) {
        oddbyte = 0;
        ((unsigned char *) &oddbyte)[0] = *(unsigned char *) ptr;
        ((unsigned char *) &oddbyte)[1] = 0;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    sum = ~sum & 0xffff;

    return sum;
}
static int aml_get_tcp_window_scaling(struct net_device *dev)
{
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;

    if (aml_bus_type != USB_MODE)
        return 0;
    if (atomic_read(&ack_mgr->enable) == 1)
        return 0;

    return ack_mgr->window_scaling;  //real window scaling factor
}

static void aml_send_tcp_ack(struct aml_tcp_ack_tx *tx_info)
{
    u8 tid = 0;
    struct aml_sw_txhdr *sw_txhdr = NULL;
    struct sk_buff *skb = NULL;
    struct txdesc_api *desc;
    struct ethhdr *eth;
    struct aml_txhdr *txhdr;
    struct aml_usb_txhdr *usb_txhdr;
    struct aml_sdio_txhdr *sdio_txhdr;
    struct aml_vif *aml_vif = tx_info->aml_vif;
    struct aml_txq *txq;
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    sw_txhdr = kmem_cache_alloc(aml_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL))
        goto free;
    skb = tx_info->skb;
    if (unlikely(skb == NULL))
        goto free;
    tid = skb->priority;
    txq = aml_txq_sta_get(tx_info->sta, tid, aml_hw);
    sw_txhdr->txq = txq;
    sw_txhdr->frame_len = AML_TX_DMA_MAP_LEN(skb);
    sw_txhdr->aml_sta  = tx_info->sta;
    sw_txhdr->aml_vif  = aml_vif;
    sw_txhdr->skb = skb;
    sw_txhdr->jiffies = jiffies;
#ifdef CONFIG_AML_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif
#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_IPC_ONGOING | AML_RECY_DROP_XMIT_PKT)) {
        goto free;
    }
#endif

    /* Prepare IPC buffer for DMA transfer */
    eth = (struct ethhdr *)skb->data;
    if (unlikely(aml_prep_dma_tx(aml_hw, sw_txhdr, eth + 1)))
        goto free;

    /* Fill-in the API descriptor for the MACSW */
    desc = &sw_txhdr->desc.api;
    memcpy(&desc->host.eth_dest_addr, eth->h_dest, ETH_ALEN);
    memcpy(&desc->host.eth_src_addr, eth->h_source, ETH_ALEN);
    desc->host.ethertype = eth->h_proto;
    desc->host.staid = tx_info->sta->sta_idx;
    desc->host.tid = tid;
    if (unlikely(aml_vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN))
        desc->host.vif_idx = aml_vif->ap_vlan.master->vif_index;
    else
        desc->host.vif_idx = aml_vif->vif_index;
    desc->host.flags = 0;

    if (aml_vif->use_4addr && (tx_info->sta->sta_idx < NX_REMOTE_STA_MAX))
        desc->host.flags |= TXU_CNTRL_USE_4ADDR;

    if ((aml_vif->tdls_status == TDLS_LINK_ACTIVE) &&
       aml_vif->sta.tdls_sta &&
       (memcmp(desc->host.eth_dest_addr.array, aml_vif->sta.tdls_sta->mac_addr, ETH_ALEN) == 0)) {
        desc->host.flags |= TXU_CNTRL_TDLS;
        aml_vif->sta.tdls_sta->tdls.last_tid = desc->host.tid;
        aml_vif->sta.tdls_sta->tdls.last_sn = 0; //TODO: set this on confirm ?
    }

    if ((aml_vif->wdev.iftype == NL80211_IFTYPE_MESH_POINT) &&
       (aml_vif->is_resending)) {
        desc->host.flags |= TXU_CNTRL_MESH_FWD;
    }

    if (aml_bus_type == PCIE_MODE) {
        /* store Tx info in skb headroom */
        txhdr = (struct aml_txhdr *)skb_push(skb, AML_TX_HEADROOM);
        txhdr->sw_hdr = sw_txhdr;
    } else {
        if (aml_bus_type == USB_MODE) {
            /* store Tx info in skb headroom */
            skb_pull(skb, sizeof(struct ethhdr));
            usb_txhdr = (struct aml_usb_txhdr *)skb_push(skb, AML_USB_TX_HEADROOM);
            usb_txhdr->sw_hdr = sw_txhdr;
        } else {

            skb_pull(skb, sizeof(struct ethhdr));
            sdio_txhdr = (struct aml_sdio_txhdr *)skb_push(skb, AML_SDIO_TX_HEADROOM);
            sdio_txhdr->sw_hdr = sw_txhdr;
            sdio_txhdr->mpdu_buf_flag = 0;
            sdio_txhdr->mpdu_buf_flag = HW_FIRST_MPDUBUF_FLAG | HW_LAST_MPDUBUF_FLAG | HW_LAST_AGG_FLAG;
            sdio_txhdr->mpdu_buf_flag |= HW_MPDU_LEN_SET(sw_txhdr->frame_len + SDIO_FRAME_TAIL_LEN);

            memset(&sdio_txhdr->tx_vector, 0, sizeof(struct hw_tx_vector_bits) + sizeof(struct txdesc_host)/*8 byte alignment*/);
        }

    }
    if (aml_bus_type != PCIE_MODE) {
        AML_PRINT(AML_DBG_MODULES_TX, "ethertype:0x%04x, credits:%d, tid:%d, vif_idx:%d\n",
                  cpu_to_be16(desc->host.ethertype), txq->credits, desc->host.tid, desc->host.vif_idx);
    }

    /* queue the buffer */
    spin_lock_bh(&aml_hw->tx_lock);

    if (txq->idx == TXQ_INACTIVE) {
        printk("%s:%d Get txq idx is inactive after spin_lock_bh	\n", __func__, __LINE__);
        //"do not push and process it with kernel list lib it whill be re-pull out and used this freed buf"
        spin_unlock_bh(&aml_hw->tx_lock);
        goto free;
    }

    if (aml_txq_queue_skb(skb, txq, aml_hw, false, NULL)) {
        if (skb_queue_empty(&txq->sk_list))
            AML_INFO("txq queue skb list empty");
        aml_hwq_process(aml_hw, txq->hwq);
    }
    spin_unlock_bh(&aml_hw->tx_lock);

    return;

free:
    if (sw_txhdr)
        kmem_cache_free(aml_hw->sw_txhdr_cache, sw_txhdr);
    if (skb)
        dev_kfree_skb_any(skb);
    return;

}

static void aml_tcp_sess_ageout(struct aml_tcp_sess_mgr *ack_mgr)
{
    int i;
    struct aml_tcp_sess_info *tcp_info;
    unsigned char drop_cnt;

    if (time_after(jiffies, ack_mgr->last_time + ack_mgr->timeout)) {
        spin_lock_bh(&ack_mgr->lock);
        ack_mgr->last_time = jiffies;
        spin_unlock_bh(&ack_mgr->lock);

        for (i = AML_TCP_SESS_NUM - 1; i >= 0; i--) {
            tcp_info = &ack_mgr->tcp_info[i];
            write_seqlock_bh(&tcp_info->seqlock);
            if (tcp_info->busy && time_after(jiffies, tcp_info->last_time + tcp_info->timeout)) {
                ack_mgr->free_index = i;
                ack_mgr->used_num--;
                tcp_info->busy = 0;
            }
            write_sequnlock_bh(&tcp_info->seqlock);
        }

        if (aml_bus_type == USB_MODE)
            drop_cnt = MAX_DROP_TCP_ACK_CNT_USB;
        else
            drop_cnt = MAX_DROP_TCP_ACK_CNT;

        /* need enable dynamic adjust drop number when do rx throughput test with less than 10 pair */
        if (atomic_read(&ack_mgr->dynamic_adjust)) {
            if (aml_bus_type == USB_MODE) {
                /*usb is not use dynamic drop ack num*/
                #if 0
                if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL2)
                    drop_cnt = 4;
                #endif
            } else {
                if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL1)
                    drop_cnt = 1;
                else if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL2)
                    drop_cnt = ack_mgr->used_num / 2;
            }
            atomic_set(&ack_mgr->max_drop_cnt, drop_cnt);
        }
    }
}

static int aml_tcp_sess_find(struct aml_tcp_sess_mgr *ack_mgr,
                             struct aml_pkt_info *pkt_info)
{
    int i, ret = -1;
    unsigned start;
    struct aml_tcp_sess_info *tcp_info;
    struct aml_pkt_info *pkt_info_temp;

    for (i = 0; ((ret < 0) && (i < AML_TCP_SESS_NUM)); i++) {
        tcp_info = &ack_mgr->tcp_info[i];
        do {
            start = read_seqbegin(&tcp_info->seqlock);
            ret = -1;

            pkt_info_temp = &tcp_info->pkt_info;
            if (tcp_info->busy &&
               pkt_info_temp->dest == pkt_info->dest &&
               pkt_info_temp->source == pkt_info->source &&
               pkt_info_temp->saddr == pkt_info->saddr &&
               pkt_info_temp->daddr == pkt_info->daddr) {
               ret = i;
            }
        } while (read_seqretry(&tcp_info->seqlock, start));
        if (ret != -1)
            break;
    }

    return ret;
}

static int aml_get_tcp_ack_info(struct net_device *ndev, struct tcphdr *tcphdr, int tcp_tot_len, unsigned short *win_scale)
{
    int type = 1;
    int len = tcphdr->doff * 4;
    unsigned char *ptr;
    int ws = 0;

    if (tcp_tot_len > len) {
        type = 0;
    } else {
        len -= sizeof(struct tcphdr);
        ptr = (unsigned char *)(tcphdr + 1);

        while ((len > 0) && type) {
            int opcode = *ptr++;
            int opsize;

            switch (opcode) {
            case TCPOPT_EOL:
                break;
            case TCPOPT_NOP:
                len--;
                continue;
            default:
                opsize = *ptr++;
                if (opsize < 2)
                    break;
                if (opsize > len)
                    break;

                switch (opcode) {
                case TCPOPT_TIMESTAMP:
                    break;
                case TCPOPT_WINDOW:
                    if (*ptr < 15)
                    {
                        ws = aml_get_tcp_window_scaling(ndev);
                        if (ws == 0xf) {
                            *ptr = *ptr - 1;
                        }
                        else if(ws != 0)
                           *ptr = ws - 1;
                        *win_scale = (1 << (*ptr));
                    }
                    break;
                default:
                    type = 2;
                }

                ptr += opsize - 2;
                len -= opsize;
            }
        }
    }

    return type;
}

static int aml_check_tcp_ack_type(struct aml_vif *aml_vif, unsigned char *data,
                                  struct aml_pkt_info *msg,unsigned short *win_scale)
{
    int ret;
    int ip_hdr_len;
    int tcp_tot_len;
    unsigned char *temp;
    struct ethhdr *ethhdr;
    struct iphdr *iphdr;
    struct tcphdr *tcphdr;
    unsigned int *ipv4_addr = aml_vif->ipv4_addr;
    unsigned int *subnet_mask = aml_vif->subnet_mask;

    ethhdr = (struct ethhdr *)data;
    if (ethhdr->h_proto != htons(ETH_P_IP))
        return 0;
    iphdr = (struct iphdr *)(ethhdr + 1);
    if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
        return 0;
    ip_hdr_len = iphdr->ihl * 4;
    temp = (unsigned char *)(iphdr) + ip_hdr_len;
    tcphdr = (struct tcphdr *)temp;
    /*only tcp ack flag*/
    if (temp[13] != 0x10) {
        /*get win scale from SYNC ACK*/
        if (temp[13] == 0x12) {
            ret = aml_get_tcp_ack_info(aml_vif->ndev, tcphdr, tcp_tot_len,win_scale);
            tcphdr->check = 0;
            tcphdr->check = tcp_checksum(tcphdr, ntohs(iphdr->tot_len) - iphdr->ihl * 4, iphdr->saddr, iphdr->daddr);
        }
        return 0;
    }

    if (((*ipv4_addr) & (*subnet_mask)) != (iphdr->daddr & (*subnet_mask))) {
        return 0;
    }

    tcp_tot_len = ntohs(iphdr->tot_len) - ip_hdr_len;
    ret = aml_get_tcp_ack_info(aml_vif->ndev, tcphdr, tcp_tot_len,win_scale);

    if (ret > 0) {
        msg->saddr = iphdr->saddr;
        msg->daddr = iphdr->daddr;
        msg->source = tcphdr->source;
        msg->dest = tcphdr->dest;
        msg->seq = ntohl(tcphdr->ack_seq);
        msg->win = ntohs(tcphdr->window);
    }

    return ret;
}

static int aml_alloc_tcp_sess(struct aml_tcp_sess_mgr *ack_mgr)
{
    int i, ret = -1;
    struct aml_tcp_sess_info *tcp_info;
    unsigned start;

    spin_lock_bh(&ack_mgr->lock);
    if (ack_mgr->used_num == AML_TCP_SESS_NUM) {
        spin_unlock_bh(&ack_mgr->lock);
        return -1;
    }

    if (ack_mgr->free_index >= 0) {
        i = ack_mgr->free_index;
        ack_mgr->free_index = -1;
        ack_mgr->used_num++;
        spin_unlock_bh(&ack_mgr->lock);
        return i;
    }

    for (i = 0; ((ret < 0) && (i < AML_TCP_SESS_NUM)); i++) {
        tcp_info = &ack_mgr->tcp_info[i];
        do {
            start = read_seqbegin(&tcp_info->seqlock);
            ret = -1;
            if (!tcp_info->busy) {
                ack_mgr->free_index = -1;
                ack_mgr->used_num++;
                ret = i;
            }
        } while (read_seqretry(&tcp_info->seqlock, start));
        if (ret != -1)
            break;
    }
    spin_unlock_bh(&ack_mgr->lock);

    return ret;
}

static void aml_tcp_ack_timeout(struct timer_list *t)
{
    struct aml_tcp_sess_info *tcp_info = NULL;
    struct sk_buff *skb = NULL;

    tcp_info = (struct aml_tcp_sess_info *)from_timer(tcp_info, t, timer);
    write_seqlock_bh(&tcp_info->seqlock);
    skb = tcp_info->skb;
    if (tcp_info->busy && skb && !tcp_info->in_txq_skb) {
        struct aml_tcp_ack_tx tx_info;
        tx_info.aml_vif = tcp_info->aml_vif;
        tx_info.skb = tcp_info->skb;
        tx_info.sta = tcp_info->sta;
        tcp_info->skb = NULL;
        tcp_info->drop_cnt = 0;
        write_sequnlock_bh(&tcp_info->seqlock);
        aml_send_tcp_ack(&tx_info);
        return;
    }
    write_sequnlock_bh(&tcp_info->seqlock);
}

void aml_tcp_delay_ack_init(struct aml_hw *aml_hw)
{
    int i;
    struct aml_tcp_sess_info *tcp_info;
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;

    memset(ack_mgr, 0, sizeof(struct aml_tcp_sess_mgr));
    ack_mgr->aml_hw = aml_hw;
    spin_lock_init(&ack_mgr->lock);
    atomic_set(&ack_mgr->max_timeout, MAX_TCP_ACK_TIMEOUT);
    if (aml_bus_type == USB_MODE) {
        atomic_set(&ack_mgr->max_drop_cnt, MAX_DROP_TCP_ACK_CNT_USB);
        atomic_set(&ack_mgr->dynamic_adjust, 0);
    } else {
        atomic_set(&ack_mgr->max_drop_cnt, MAX_DROP_TCP_ACK_CNT);
        atomic_set(&ack_mgr->dynamic_adjust, 1);
    }
    ack_mgr->last_time = jiffies;
    ack_mgr->total_drop_cnt = 0;
    ack_mgr->timeout = msecs_to_jiffies(TCK_SESS_TIMEOUT_TIME);
    for (i = 0; i < AML_TCP_SESS_NUM; i++) {
        tcp_info = &ack_mgr->tcp_info[i];
        tcp_info->index = i;
        seqlock_init(&tcp_info->seqlock);
        tcp_info->last_time = jiffies;
        tcp_info->timeout = msecs_to_jiffies(TCK_SESS_TIMEOUT_TIME);
        timer_setup(&tcp_info->timer, aml_tcp_ack_timeout, 0);
    }
    atomic_set(&ack_mgr->enable, 1);

    ack_mgr->ack_winsize = MIN_WIN;
    ack_mgr->win_scale = 1;
    ack_mgr->window_scaling = 4;
    ack_mgr->rssi_l_thr = -66;
    ack_mgr->rssi_h_thr = -63;

}

void aml_tcp_delay_ack_deinit(struct aml_hw *aml_hw)
{
    int i;
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;
    struct sk_buff *drop_skb = NULL;

    atomic_set(&ack_mgr->enable, 0);
    for (i = 0; i < AML_TCP_SESS_NUM; i++) {
        drop_skb = NULL;
        write_seqlock_bh(&ack_mgr->tcp_info[i].seqlock);
        del_timer(&ack_mgr->tcp_info[i].timer);
        drop_skb = ack_mgr->tcp_info[i].skb;
        ack_mgr->tcp_info[i].skb = NULL;
        write_sequnlock_bh(&ack_mgr->tcp_info[i].seqlock);
        if (drop_skb)
            dev_kfree_skb_any(drop_skb);
    }
}

void aml_check_tcpack_skb(struct aml_hw *aml_hw, struct sk_buff *skb, u32 len)
{
    struct aml_tcp_sess_info *tcp_info;
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;
    int i = 0;

    if (!atomic_read(&ack_mgr->enable))
        return;

    if (len > MAX_TCP_ACK_LEN)
        return;

    for (i = 0; i < AML_TCP_SESS_NUM; i++) {
        tcp_info = &ack_mgr->tcp_info[i];
        write_seqlock_bh(&tcp_info->seqlock);
        if (tcp_info->busy && (tcp_info->in_txq_skb == skb)) {
            tcp_info->in_txq_skb = NULL;
        }
        write_sequnlock_bh(&tcp_info->seqlock);
    }
}

int aml_replace_tcp_ack(struct sk_buff *skb,
                        struct aml_tcp_sess_mgr *ack_mgr,
                        struct aml_tcp_sess_info *tcp_info,
                        struct aml_pkt_info *pkt_info,
                        int type)
{
    struct aml_pkt_info *old_pkt;
    int ret = 0;
    struct sk_buff *drop_skb = NULL;

    write_seqlock_bh(&tcp_info->seqlock);

    tcp_info->last_time = jiffies;
    old_pkt = &tcp_info->pkt_info;
    if (type == 2) {
        if (AML_U32_BEFORE(old_pkt->seq, pkt_info->seq)) {
            old_pkt->seq = pkt_info->seq;

            if (tcp_info->skb) {
                drop_skb = tcp_info->skb;
                tcp_info->skb = NULL;
                del_timer(&tcp_info->timer);
            }

            tcp_info->in_txq_skb = NULL;
            tcp_info->drop_cnt = atomic_read(&ack_mgr->max_drop_cnt);
        } else {
            AML_INFO("before abnormal ack: %d, %d\n", old_pkt->seq, pkt_info->seq);
            drop_skb = skb;
            ret = 1;
        }
    } else if (AML_U32_BEFORE(old_pkt->seq, pkt_info->seq)) {
        if (tcp_info->skb) {
            drop_skb = tcp_info->skb;
            tcp_info->skb = NULL;
        }
        tcp_info->drop_cnt++;
        old_pkt->seq = pkt_info->seq;
        if (!tcp_info->in_txq_skb &&
           (tcp_info->drop_cnt >= atomic_read(&ack_mgr->max_drop_cnt))) {
            tcp_info->drop_cnt = 0;
            del_timer(&tcp_info->timer);
        } else {
            ret = 1;
            tcp_info->skb = skb;
            if (!timer_pending(&tcp_info->timer))
                mod_timer(&tcp_info->timer, (jiffies + msecs_to_jiffies(atomic_read(&ack_mgr->max_timeout))));
        }
    } else {
        AML_INFO("before ack: %d, %d\n", old_pkt->seq, pkt_info->seq);
        drop_skb = skb;
        ret = 1;
    }

    write_sequnlock_bh(&tcp_info->seqlock);
    if (drop_skb) {
        spin_lock_bh(&ack_mgr->lock);
        ack_mgr->total_drop_cnt++;
        spin_unlock_bh(&ack_mgr->lock);
        dev_kfree_skb_any(drop_skb);
    }

    return ret;
}

/* return zero if no change */
int aml_set_tcp_ack_accord_to_rssi(struct aml_sta *sta, struct aml_hw *aml_hw, s32_l rssi)
{
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;
    bool enable = false;

    if (aml_bus_type != USB_MODE)   /* only effect on USB device */
        return 0;

    if (aml_work_on_5g_band(aml_hw))
        enable = true;  /* force to enable it if work on 5GHz band */
    else if (rssi > ack_mgr->rssi_h_thr)
        enable = true;  /* enable it if rssi is good enough */
    else if (rssi >= ack_mgr->rssi_l_thr)
        return 0;       /* no change if rssi in range[rssi_l_thr, rssi_h_thr] */
    else
        enable = false; /* rssi is worse, disable it */

    if (enable == !!atomic_read(&ack_mgr->enable))
        return 0;

    atomic_set(&ack_mgr->enable, enable);
    /* printk("%s, set enable %d\n", __func__, enable); */

    return 1;
}

int aml_filter_tx_tcp_ack(struct net_device *dev,
                          struct sk_buff *skb,
                          struct aml_sta *sta)
{
    int ret = 0;
    int index, type;
    unsigned short win_scale = 0;
    unsigned int win = 0;
    struct aml_vif *aml_vif = netdev_priv(dev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct aml_pkt_info pkt_info;
    struct aml_pkt_info *pkt_info_ptr;
    struct aml_tcp_sess_info *tcp_info;
    struct aml_tcp_sess_mgr *ack_mgr = &aml_hw->ack_mgr;

    if (skb->len > MAX_TCP_ACK_LEN)
        return 0;

    aml_tcp_sess_ageout(ack_mgr);
    /*type 0:not tcp ack,1:tcp ack,2:tcp ack with other info*/
    type = aml_check_tcp_ack_type(aml_vif, skb->data, &pkt_info,&win_scale);
    if (win_scale && (ack_mgr->win_scale != win_scale))
        ack_mgr->win_scale = win_scale;
    if (!type)
        return 0;

    if (!atomic_read(&ack_mgr->enable))
         return 0;
    index = aml_tcp_sess_find(ack_mgr, &pkt_info);
    if (index >= 0) {
        tcp_info = ack_mgr->tcp_info + index;

        if ((ack_mgr->win_scale != 0) && (tcp_info->win_scale != ack_mgr->win_scale)) {
            write_seqlock_bh(&tcp_info->seqlock);
            tcp_info->win_scale = ack_mgr->win_scale;
            write_sequnlock_bh(&tcp_info->seqlock);
        }

        if (type > 0) {
            win = tcp_info->win_scale * pkt_info.win;
            if (win < (ack_mgr->ack_winsize * SIZE_KB))
                type = 2;
            ret = aml_replace_tcp_ack(skb, ack_mgr, tcp_info, &pkt_info, type);
        }
        return ret;
    }

    index = aml_alloc_tcp_sess(ack_mgr);
    if (index >= 0) {
        u8 drop_cnt;

        if (aml_bus_type == USB_MODE)
            drop_cnt = MAX_DROP_TCP_ACK_CNT_USB;
        else
            drop_cnt = MAX_DROP_TCP_ACK_CNT;

        tcp_info = ack_mgr->tcp_info + index;
        /* need enable dynamic adjust drop number when do rx throughput test with less than 10 pair */
        if (atomic_read(&ack_mgr->dynamic_adjust)) {
            if (aml_bus_type == USB_MODE) {
              /*usb is not use dynamic drop ack num*/
              #if 0
                if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL2)
                    drop_cnt = 4;
              #endif
            } else {
                if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL1)
                    drop_cnt = 1;
                else if (ack_mgr->used_num < MAX_TCP_SESS_LEVEL2)
                    drop_cnt = ack_mgr->used_num / 2;
            }
            atomic_set(&ack_mgr->max_drop_cnt, drop_cnt);
        }
        write_seqlock_bh(&ack_mgr->tcp_info[index].seqlock);
        tcp_info->busy = 1;
        tcp_info->last_time = jiffies;
        tcp_info->drop_cnt = drop_cnt;
        tcp_info->win_scale = ack_mgr->win_scale;
        tcp_info->sta = sta;
        tcp_info->aml_vif = aml_vif;
        if (!timer_pending(&tcp_info->timer))
            mod_timer(&tcp_info->timer, (jiffies + msecs_to_jiffies(atomic_read(&ack_mgr->max_timeout))));
        pkt_info_ptr = &tcp_info->pkt_info;
        pkt_info_ptr->dest = pkt_info.dest;
        pkt_info_ptr->source = pkt_info.source;
        pkt_info_ptr->saddr = pkt_info.saddr;
        pkt_info_ptr->daddr = pkt_info.daddr;
        pkt_info_ptr->seq = pkt_info.seq;
        write_sequnlock_bh(&ack_mgr->tcp_info[index].seqlock);
    }
    return ret;
}
