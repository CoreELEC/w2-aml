/**
 ******************************************************************************
 *
 * @file aml_debugfs.c
 *
 * @brief Definition of debugfs entries
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */


#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/sort.h>
#include <linux/math64.h>
#include <linux/vmalloc.h>

#include "aml_msg_tx.h"
#include "aml_radar.h"
#include "aml_tx.h"

extern int aml_set_fwlog_cmd(struct net_device *dev, int mode);

#ifdef CONFIG_AML_SOFTMAC
static ssize_t aml_dbgfs_stats_read(struct file *file,
                                     char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char *buf;
    int per;
    int ret;
    int i, skipped;
    ssize_t read;
    int bufsz = (10 + NX_TX_PAYLOAD_MAX + NX_TXQ_CNT + IEEE80211_MAX_AMPDU_BUF) * 50;

    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    if (priv->stats->agg_done)
        per = DIV_ROUND_UP((priv->stats->agg_retries + priv->stats->agg_died) *
                           100, priv->stats->agg_done);
    else
        per = 0;

    ret = scnprintf(buf, min_t(size_t, bufsz - 1, count),
                    "agg_done         %10d\n"
                    "agg_retries      %10d\n"
                    "agg_retries_last %10d\n"
                    "agg_died         %10d\n"
                    "ampdu_all_ko     %10d\n"
                    "agg_PER (%%)      %10d\n"
                    "queues_stops     %10d\n\n",
                    priv->stats->agg_done,
                    priv->stats->agg_retries,
                    priv->stats->agg_retries_last,
                    priv->stats->agg_died,
                    priv->stats->ampdu_all_ko,
                    per,
                    priv->stats->queues_stops);

    ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                     "TXQs CFM balances ");
    for (i = 0; i < NX_TXQ_CNT; i++)
        ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                         "  [%1d]:%3d", i,
                         priv->stats->cfm_balance[i]);

#ifdef CONFIG_AML_SPLIT_TX_BUF
    ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                     "\n\nAMSDU[len]             done   failed(%%)\n");
    for (i = skipped = 0; i < NX_TX_PAYLOAD_MAX; i++) {
        if (priv->stats->amsdus[i].done) {
            per = DIV_ROUND_UP((priv->stats->amsdus[i].failed) *
                               100, priv->stats->amsdus[i].done);
        } else {
            per = 0;
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                             "   * * *         %10d  %10d\n", 0, 0);
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                         "   [%1d]           %10d  %10d\n", i + 1,
                         priv->stats->amsdus[i].done, per);
    }
    if (skipped)
        ret += scnprintf(&buf[ret], min_t(size_t, bufsz - 1, count - ret),
                         "   * * *         %10d  %10d\n", 0, 0);
#endif

    ret += scnprintf(&buf[ret], min_t(size_t, bufsz - ret - 1, count - ret),
                     "\nIn-AMPDU     TX failures(%%)   RX counts\n");
    for (i = skipped = 0; i < IEEE80211_MAX_AMPDU_BUF; i++) {
        int failed;

        if (priv->stats->in_ampdu[i].done) {
            failed = DIV_ROUND_UP(priv->stats->in_ampdu[i].failed *
                                  100, priv->stats->in_ampdu[i].done);
        } else {
            if (!priv->stats->rx_in_ampdu[i].cnt) {
                skipped = 1;
                continue;
            }
            failed = 0;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret],
                             min_t(size_t, bufsz - ret - 1, count - ret),
                             "   * * *         %10d  %10d\n", 0, 0);
            skipped = 0;
        }
        ret += scnprintf(&buf[ret],
                         min_t(size_t, bufsz - ret - 1, count - ret),
                         "   mpdu#%2d       %10d  %10d\n", i, failed,
                         priv->stats->rx_in_ampdu[i].cnt);

    }
    if (skipped)
        ret += scnprintf(&buf[ret],
                         min_t(size_t, bufsz - ret - 1, count - ret),
                         "   * * *         %10d  %10d\n", 0, 0);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    kfree(buf);

    return read;
}

#else

static ssize_t aml_dbgfs_stats_read(struct file *file,
                                     char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char *buf;
    int ret;
    int i, skipped;
#ifdef CONFIG_AML_SPLIT_TX_BUF
    int per;
#endif
    ssize_t read;
    int bufsz = (NX_TXQ_CNT) * 20 + (ARRAY_SIZE(priv->stats->amsdus_rx) + 1) * 40
        + (ARRAY_SIZE(priv->stats->ampdus_tx) * 30);

    if (*ppos)
        return 0;

    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    ret = scnprintf(buf, bufsz, "TXQs CFM balances ");
    for (i = 0; i < NX_TXQ_CNT; i++)
        ret += scnprintf(&buf[ret], bufsz - ret,
                         "  [%1d]:%3d", i,
                         priv->stats->cfm_balance[i]);

    ret += scnprintf(&buf[ret], bufsz - ret, "\n");

#ifdef CONFIG_AML_SPLIT_TX_BUF
    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMSDU[len]       done         failed   received\n");
    for (i = skipped = 0; i < NX_TX_PAYLOAD_MAX; i++) {
        if (priv->stats->amsdus[i].done) {
            per = DIV_ROUND_UP((priv->stats->amsdus[i].failed) *
                               100, priv->stats->amsdus[i].done);
        } else if (priv->stats->amsdus_rx[i]) {
            per = 0;
        } else {
            per = 0;
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret, "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]    %10d %8d(%3d%%) %10d\n",  i ? i + 1 : i,
                         priv->stats->amsdus[i].done,
                         priv->stats->amsdus[i].failed, per,
                         priv->stats->amsdus_rx[i]);
    }

    for (; i < ARRAY_SIZE(priv->stats->amsdus_rx); i++) {
        if (!priv->stats->amsdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret, "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]                              %10d\n",
                         i + 1, priv->stats->amsdus_rx[i]);
    }
#else
    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMSDU[len]   received\n");
    for (i = skipped = 0; i < ARRAY_SIZE(priv->stats->amsdus_rx); i++) {
        if (!priv->stats->amsdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret,
                             "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]    %10d\n",
                         i + 1, priv->stats->amsdus_rx[i]);
    }

#endif /* CONFIG_AML_SPLIT_TX_BUF */

    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMPDU[len]     done  received\n");
    for (i = skipped = 0; i < ARRAY_SIZE(priv->stats->ampdus_tx); i++) {
        if (!priv->stats->ampdus_tx[i] && !priv->stats->ampdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret,
                             "    ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]   %9d %9d\n", i ? i + 1 : i,
                         priv->stats->ampdus_tx[i], priv->stats->ampdus_rx[i]);
    }

    ret += scnprintf(&buf[ret], bufsz - ret,
                     "#mpdu missed        %9d\n",
                     priv->stats->ampdus_rx_miss);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    kfree(buf);

    return read;
}
#endif /* CONFIG_AML_SOFTMAC */

static ssize_t aml_dbgfs_stats_write(struct file *file,
                                      const char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;

    /* Prevent from interrupt preemption as these statistics are updated under
     * interrupt */
    spin_lock_bh(&priv->tx_lock);

    memset(priv->stats, 0, sizeof(struct aml_stats));

    spin_unlock_bh(&priv->tx_lock);

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(stats);

int aml_dbgfs_txq(char *buf, size_t size, struct aml_txq *txq, int type, int tid, char *name)
{
    int res, idx = 0;
    int i, pushed = 0;

    if (type == STA_TXQ) {
        res = scnprintf(&buf[idx], size, TXQ_STA_PREF_FMT, tid);
        idx += res;
        size -= res;
    } else {
        res = scnprintf(&buf[idx], size, TXQ_VIF_PREF_FMT, name);
        idx += res;
        size -= res;
    }

    for (i = 0; i < CONFIG_USER_MAX; i++) {
        pushed += txq->pkt_pushed[i];
    }

    res = scnprintf(&buf[idx], size, TXQ_HDR_FMT, txq->idx,
                    (txq->status & AML_TXQ_IN_HWQ_LIST) ? "L" : " ",
                    (txq->status & AML_TXQ_STOP_FULL) ? "F" : " ",
                    (txq->status & AML_TXQ_STOP_STA_PS) ? "P" : " ",
                    (txq->status & AML_TXQ_STOP_VIF_PS) ? "V" : " ",
                    (txq->status & AML_TXQ_STOP_CHAN) ? "C" : " ",
                    (txq->status & AML_TXQ_STOP_CSA) ? "S" : " ",
                    (txq->status & AML_TXQ_STOP_MU_POS) ? "M" : " ",
                    (txq->status & AML_TXQ_NDEV_FLOW_CTRL) ? "N" : " ",
                    (txq->status & AML_TXQ_STOP_COEX_INACTIVE) ? "I" : " ",
                    txq->credits, skb_queue_len(&txq->sk_list),
                    txq->nb_retry, pushed);
    idx += res;
    size -= res;

#ifdef CONFIG_AML_AMSDUS_TX
    if (type == STA_TXQ) {
        res = scnprintf(&buf[idx], size, TXQ_HDR_SUFF_FMT,
#ifdef CONFIG_AML_FULLMAC
                        txq->amsdu_len
#else
                        txq->amsdu_ht_len_cap, txq->amsdu_vht_len_cap
#endif /* CONFIG_AML_FULLMAC */
                        );
        idx += res;
        size -= res;
    }
#endif

    res = scnprintf(&buf[idx], size, "\n");
    idx += res;
    size -= res;

    return idx;
}

int aml_dbgfs_txq_sta(char *buf, size_t size, struct aml_sta *aml_sta,
                              struct aml_hw *aml_hw)
{
    int tid, res, idx = 0;
    struct aml_txq *txq;
#ifdef CONFIG_AML_SOFTMAC
    struct ieee80211_sta *sta = aml_to_ieee80211_sta(aml_sta);
#endif /* CONFIG_AML_SOFTMAC */

    res = scnprintf(&buf[idx], size, "\n" STA_HDR,
                    aml_sta->sta_idx,
#ifdef CONFIG_AML_SOFTMAC
                    sta->addr
#else
                    aml_sta->mac_addr
#endif /* CONFIG_AML_SOFTMAC */
                    );
    idx += res;
    size -= res;

#ifdef CONFIG_AML_FULLMAC
    if (aml_sta->ps.active) {
        if (aml_sta->uapsd_tids &&
            (aml_sta->uapsd_tids == ((1 << NX_NB_TXQ_PER_STA) - 1)))
            res = scnprintf(&buf[idx], size, PS_HDR_UAPSD "\n",
                            aml_sta->ps.pkt_ready[UAPSD_ID],
                            aml_sta->ps.sp_cnt[UAPSD_ID]);
        else if (aml_sta->uapsd_tids)
            res = scnprintf(&buf[idx], size, PS_HDR "\n",
                            aml_sta->ps.pkt_ready[LEGACY_PS_ID],
                            aml_sta->ps.sp_cnt[LEGACY_PS_ID],
                            aml_sta->ps.pkt_ready[UAPSD_ID],
                            aml_sta->ps.sp_cnt[UAPSD_ID]);
        else
            res = scnprintf(&buf[idx], size, PS_HDR_LEGACY "\n",
                            aml_sta->ps.pkt_ready[LEGACY_PS_ID],
                            aml_sta->ps.sp_cnt[LEGACY_PS_ID]);
        idx += res;
        size -= res;
    } else {
        res = scnprintf(&buf[idx], size, "\n");
        idx += res;
        size -= res;
    }
#endif /* CONFIG_AML_FULLMAC */


    res = scnprintf(&buf[idx], size, TXQ_STA_PREF TXQ_HDR TXQ_HDR_SUFF "\n");
    idx += res;
    size -= res;


    foreach_sta_txq_safe(aml_sta, txq, tid, aml_hw) {
        res = aml_dbgfs_txq(&buf[idx], size, txq, STA_TXQ, tid, NULL);
        idx += res;
        size -= res;
    }

    return idx;
}

#ifdef CONFIG_AML_DEBUGFS
int aml_dbgfs_txq_vif(char *buf, size_t size, struct aml_vif *aml_vif,
                              struct aml_hw *aml_hw)
{
    int res, idx = 0;
    struct aml_txq *txq;
    struct aml_sta *aml_sta, *tmp;

#ifdef CONFIG_AML_FULLMAC
    res = scnprintf(&buf[idx], size, VIF_HDR, aml_vif->vif_index, aml_vif->ndev->name);
    idx += res;
    size -= res;
    if (!aml_vif->up || aml_vif->ndev == NULL)
        return idx;

#else
    int ac;
    char ac_name[2] = {'0', '\0'};

    res = scnprintf(&buf[idx], size, VIF_HDR, aml_vif->vif_index);
    idx += res;
    size -= res;
#endif /* CONFIG_AML_FULLMAC */

#ifdef CONFIG_AML_FULLMAC
    if (AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_AP ||
        AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_P2P_GO ||
        AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_MESH_POINT) {
        res = scnprintf(&buf[idx], size, TXQ_VIF_PREF TXQ_HDR "\n");
        idx += res;
        size -= res;
        txq = aml_txq_vif_get(aml_vif, NX_UNK_TXQ_TYPE);
        res = aml_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, "UNK");
        idx += res;
        size -= res;
        txq = aml_txq_vif_get(aml_vif, NX_BCMC_TXQ_TYPE);
        res = aml_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, "BCMC");
        idx += res;
        size -= res;
        aml_sta = aml_hw->sta_table + aml_vif->ap.bcmc_index;
        if (aml_sta->ps.active) {
            res = scnprintf(&buf[idx], size, PS_HDR_LEGACY "\n",
                            aml_sta->ps.sp_cnt[LEGACY_PS_ID],
                            aml_sta->ps.sp_cnt[LEGACY_PS_ID]);
            idx += res;
            size -= res;
        } else {
            res = scnprintf(&buf[idx], size, "\n");
            idx += res;
            size -= res;
        }

        list_for_each_entry_safe(aml_sta, tmp, &aml_vif->ap.sta_list, list) {
            res = aml_dbgfs_txq_sta(&buf[idx], size, aml_sta, aml_hw);
            idx += res;
            size -= res;
        }
    } else if (AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_STATION ||
               AML_VIF_TYPE(aml_vif) ==  NL80211_IFTYPE_P2P_CLIENT) {
        if (aml_vif->sta.ap) {
            res = aml_dbgfs_txq_sta(&buf[idx], size, aml_vif->sta.ap, aml_hw);
            idx += res;
            size -= res;
        }
    }

#else
    res = scnprintf(&buf[idx], size, TXQ_VIF_PREF TXQ_HDR "\n");
    idx += res;
    size -= res;

    foreach_vif_txq(aml_vif, txq, ac) {
        ac_name[0]++;
        res = aml_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, ac_name);
        idx += res;
        size -= res;
    }

    list_for_each_entry(aml_sta, &aml_vif->stations, list) {
        res = aml_dbgfs_txq_sta(&buf[idx], size, aml_sta, aml_hw);
        idx += res;
        size -= res;
    }
#endif /* CONFIG_AML_FULLMAC */
    return idx;
}
#endif

static ssize_t aml_dbgfs_txq_read(struct file *file ,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct aml_hw *aml_hw = file->private_data;
    struct aml_vif *vif;
    char *buf;
    int idx, res;
    ssize_t read;
    size_t bufsz = ((NX_VIRT_DEV_MAX * (VIF_HDR_MAX_LEN + 2 * VIF_SEP_LEN)) +
                    (NX_REMOTE_STA_MAX * STA_HDR_MAX_LEN) +
                    ((NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX + NX_NB_TXQ) *
                     TXQ_HDR_MAX_LEN) + CAPTION_LEN);

    /* everything is read in one go */
    if (*ppos)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    bufsz--;
    idx = 0;

    res = scnprintf(&buf[idx], bufsz, CAPTION);
    idx += res;
    bufsz -= res;

    //spin_lock_bh(&aml_hw->tx_lock);
    list_for_each_entry(vif, &aml_hw->vifs, list) {
        res = scnprintf(&buf[idx], bufsz, "\n"VIF_SEP);
        idx += res;
        bufsz -= res;
        res = aml_dbgfs_txq_vif(&buf[idx], bufsz, vif, aml_hw);
        idx += res;
        bufsz -= res;
        res = scnprintf(&buf[idx], bufsz, VIF_SEP);
        idx += res;
        bufsz -= res;
    }
    //spin_unlock_bh(&aml_hw->tx_lock);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, idx);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(txq);

static ssize_t aml_dbgfs_acsinfo_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    #ifdef CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = priv->hw->wiphy;
    #else //CONFIG_AML_SOFTMAC
    struct wiphy *wiphy = priv->wiphy;
    #endif //CONFIG_AML_SOFTMAC
    char *buf = NULL;
    int buf_size = (SCAN_CHANNEL_MAX + 1) * 43;
    int survey_cnt = 0;
    int len = 0, ret = 0;
    int band, chan_cnt;

    if ((buf = vmalloc(buf_size)) == NULL)
        return -1;

    mutex_lock(&priv->dbgdump.mutex);

    len += scnprintf(buf, min_t(size_t, buf_size - 1, count),
                     "FREQ    TIME(ms)    BUSY(ms)    NOISE(dBm)\n");

    for (band = NL80211_BAND_2GHZ; band <= NL80211_BAND_5GHZ; band++) {
        for (chan_cnt = 0; chan_cnt < wiphy->bands[band]->n_channels; chan_cnt++) {
            struct aml_survey_info *p_survey_info = &priv->survey[survey_cnt];
            struct ieee80211_channel *p_chan = &wiphy->bands[band]->channels[chan_cnt];

            if (p_survey_info->filled) {
                len += scnprintf(&buf[len], min_t(size_t, buf_size - len - 1, count),
                                 "%d    %03d         %03d         %d\n",
                                 p_chan->center_freq,
                                 p_survey_info->chan_time_ms,
                                 p_survey_info->chan_time_busy_ms,
                                 p_survey_info->noise_dbm);
            } else {
                len += scnprintf(&buf[len], min_t(size_t, buf_size -len -1, count),
                                 "%d    NOT AVAILABLE\n",
                                 p_chan->center_freq);
            }

            survey_cnt++;
        }
    }

    mutex_unlock(&priv->dbgdump.mutex);

    ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
    vfree(buf);
    return ret;
}

DEBUGFS_READ_FILE_OPS(acsinfo);

static ssize_t aml_dbgfs_fw_dbg_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count, loff_t *ppos)
{
    char help[]="usage: [MOD:<ALL|KE|DBG|IPC|DMA|MM|TX|RX|PHY>]* "
        "[DBG:<NONE|CRT|ERR|WRN|INF|VRB>]\n";

    return simple_read_from_buffer(user_buf, count, ppos, help, sizeof(help));
}


static ssize_t aml_dbgfs_fw_dbg_write(struct file *file,
                                            const char __user *user_buf,
                                            size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int idx = 0;
    u32 mod = 0;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';

#define AML_MOD_TOKEN(str, val)                                        \
    if (strncmp(&buf[idx], str, sizeof(str) - 1 ) == 0) {               \
        idx += sizeof(str) - 1;                                         \
        mod |= val;                                                     \
        continue;                                                       \
    }
#define AML_MOD_TOKEN_ALL(str, val)                                    \
    if (strncmp(&buf[idx], str, sizeof(str) - 1 ) == 0) {               \
        idx += sizeof(str) - 1;                                         \
        mod = val;                                                      \
        continue;                                                       \
    }

#define AML_DBG_TOKEN(str, val)                                \
    if (strncmp(&buf[idx], str, sizeof(str) - 1) == 0) {        \
        idx += sizeof(str) - 1;                                 \
        dbg = val;                                              \
        goto dbg_done;                                          \
    }

    while ((idx + 4) < len) {
        if (strncmp(&buf[idx], "MOD:", 4) == 0) {
            idx += 4;
            AML_MOD_TOKEN_ALL("ALL", 0xffffffff);
            AML_MOD_TOKEN("KE",  BIT(0));
            AML_MOD_TOKEN("DBG", BIT(1));
            AML_MOD_TOKEN("IPC", BIT(2));
            AML_MOD_TOKEN("DMA", BIT(3));
            AML_MOD_TOKEN("MM",  BIT(4));
            AML_MOD_TOKEN("TX",  BIT(5));
            AML_MOD_TOKEN("RX",  BIT(6));
            AML_MOD_TOKEN("PHY", BIT(7));
            idx++;
        } else if (strncmp(&buf[idx], "DBG:", 4) == 0) {
            u32 dbg = 0;
            idx += 4;
            AML_DBG_TOKEN("NONE", 0);
            AML_DBG_TOKEN("CRT",  1);
            AML_DBG_TOKEN("ERR",  2);
            AML_DBG_TOKEN("WRN",  3);
            AML_DBG_TOKEN("INF",  4);
            AML_DBG_TOKEN("VRB",  5);
            idx++;
            continue;
          dbg_done:
            aml_send_dbg_set_sev_filter_req(priv, dbg);
        } else {
            idx++;
        }
    }

    if (mod) {
        aml_send_dbg_set_mod_filter_req(priv, mod);
    }

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(fw_dbg);

static ssize_t aml_dbgfs_sys_stats_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[3*64];
    int len = 0;
    ssize_t read;
    int error = 0;
    struct dbg_get_sys_stat_cfm cfm;
    u64 sleep_int, sleep_frac, doze_int, doze_frac;
    u32 rem;

    AML_DBG(AML_FN_ENTRY_STR);

    /* Get the information from the FW */
    if ((error = aml_send_dbg_get_sys_stat_req(priv, &cfm)))
        return error;

    if (cfm.stats_time == 0)
        return 0;

    sleep_int = div_u64_rem(cfm.cpu_sleep_time * 100, cfm.stats_time, &rem);
    sleep_frac = div_u64(rem * 10,  cfm.stats_time);
    doze_int = div_u64_rem(cfm.doze_time * 100, cfm.stats_time, &rem);
    doze_frac = div_u64(rem * 10, cfm.stats_time);

    len += scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                     "\nSystem statistics:\n");
    len += scnprintf(&buf[len], min_t(size_t, sizeof(buf) - 1, count),
                     "  CPU sleep [%%]: %d.%d\n", sleep_int, sleep_frac);
    len += scnprintf(&buf[len], min_t(size_t, sizeof(buf) - 1, count),
                     "  Doze      [%%]: %d.%d\n", doze_int, doze_frac);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    return read;
}

DEBUGFS_READ_FILE_OPS(sys_stats);

#ifdef CONFIG_AML_MUMIMO_TX
static ssize_t aml_dbgfs_mu_group_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_hw *aml_hw = file->private_data;
    struct aml_mu_info *mu = &aml_hw->mu;
    struct aml_mu_group *group;
    size_t bufsz = NX_MU_GROUP_MAX * sizeof("xx = (xx - xx - xx - xx)\n") + 50;
    char *buf;
    int j, res, idx = 0;

    if (*ppos)
        return 0;

    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    res = scnprintf(&buf[idx], bufsz, "MU Group list (%d groups, %d users max)\n",
                    NX_MU_GROUP_MAX, CONFIG_USER_MAX);
    idx += res;
    bufsz -= res;

    list_for_each_entry(group, &mu->active_groups, list) {
        if (group->user_cnt) {
            res = scnprintf(&buf[idx], bufsz, "%2d = (", group->group_id);
            idx += res;
            bufsz -= res;
            for (j = 0; j < (CONFIG_USER_MAX - 1) ; j++) {
                if (group->users[j])
                    res = scnprintf(&buf[idx], bufsz, "%2d - ",
                                    group->users[j]->sta_idx);
                else
                    res = scnprintf(&buf[idx], bufsz, ".. - ");

                idx += res;
                bufsz -= res;
            }

            if (group->users[j])
                res = scnprintf(&buf[idx], bufsz, "%2d)\n",
                                group->users[j]->sta_idx);
            else
                res = scnprintf(&buf[idx], bufsz, "..)\n");

            idx += res;
            bufsz -= res;
        }
    }

    res = simple_read_from_buffer(user_buf, count, ppos, buf, idx);
    kfree(buf);

    return res;
}

DEBUGFS_READ_FILE_OPS(mu_group);
#endif

#ifdef CONFIG_AML_P2P_DEBUGFS
static ssize_t aml_dbgfs_oppps_write(struct file *file,
                                      const char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct aml_hw *rw_hw = file->private_data;
    struct aml_vif *rw_vif;
    char buf[32];
    size_t len = min_t(size_t, count, sizeof(buf) - 1);
    int ctw;

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';

    /* Read the written CT Window (provided in ms) value */
    if (sscanf(buf, "ctw=%d", &ctw) > 0) {
        /* Check if at least one VIF is configured as P2P GO */
        list_for_each_entry(rw_vif, &rw_hw->vifs, list) {
#ifdef CONFIG_AML_SOFTMAC
            if ((AML_VIF_TYPE(rw_vif) == NL80211_IFTYPE_AP) && rw_vif->vif->p2p) {
#else /* CONFIG_AML_FULLMAC */
            if (AML_VIF_TYPE(rw_vif) == NL80211_IFTYPE_P2P_GO) {
#endif /* CONFIG_AML_SOFTMAC */
                struct mm_set_p2p_oppps_cfm cfm;

                /* Forward request to the embedded and wait for confirmation */
                aml_send_p2p_oppps_req(rw_hw, rw_vif, (u8)ctw, &cfm);

                break;
            }
        }
    }

    return count;
}

DEBUGFS_WRITE_FILE_OPS(oppps);

static ssize_t aml_dbgfs_noa_write(struct file *file,
                                    const char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    struct aml_hw *rw_hw = file->private_data;
    struct aml_vif *rw_vif;
    char buf[64];
    size_t len = min_t(size_t, count, sizeof(buf) - 1);
    int noa_count, interval, duration, dyn_noa;

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';

    /* Read the written NOA information */
    if (sscanf(buf, "count=%d interval=%d duration=%d dyn=%d",
               &noa_count, &interval, &duration, &dyn_noa) > 0) {
        /* Check if at least one VIF is configured as P2P GO */
        list_for_each_entry(rw_vif, &rw_hw->vifs, list) {
#ifdef CONFIG_AML_SOFTMAC
            if ((AML_VIF_TYPE(rw_vif) == NL80211_IFTYPE_AP) && rw_vif->vif->p2p) {
#else /* CONFIG_AML_FULLMAC */
            if (AML_VIF_TYPE(rw_vif) == NL80211_IFTYPE_P2P_GO) {
#endif /* CONFIG_AML_SOFTMAC */
                struct mm_set_p2p_noa_cfm cfm;

                /* Forward request to the embedded and wait for confirmation */
                aml_send_p2p_noa_req(rw_hw, rw_vif, noa_count, interval,
                                      duration, (dyn_noa > 0),  &cfm);

                break;
            }
        }
    }

    return count;
}

DEBUGFS_WRITE_FILE_OPS(noa);
#endif /* CONFIG_AML_P2P_DEBUGFS */

struct aml_dbgfs_fw_trace {
    struct aml_fw_trace_local_buf lbuf;
    struct aml_fw_trace *trace;
    struct aml_hw *aml_hw;
};

static int aml_dbgfs_fw_trace_open(struct inode *inode, struct file *file)
{
    struct aml_dbgfs_fw_trace *ltrace = kmalloc(sizeof(*ltrace), GFP_KERNEL);
    struct aml_hw *priv = inode->i_private;

    if (!ltrace)
        return -ENOMEM;

    if (aml_fw_trace_alloc_local(&ltrace->lbuf, 5120)) {
        kfree(ltrace);
        return -ENOMEM;
    }

    ltrace->trace = &priv->debugfs.fw_trace;
    ltrace->aml_hw = priv;
    file->private_data = ltrace;
    return 0;
}

static int aml_dbgfs_fw_trace_release(struct inode *inode, struct file *file)
{
    struct aml_dbgfs_fw_trace *ltrace = file->private_data;

    if (ltrace) {
        aml_fw_trace_free_local(&ltrace->lbuf);
        kfree(ltrace);
    }

    return 0;
}

static ssize_t aml_dbgfs_fw_trace_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_dbgfs_fw_trace *ltrace = file->private_data;
    bool dont_wait = ((file->f_flags & O_NONBLOCK) ||
                      ltrace->aml_hw->debugfs.unregistering);

    if (aml_bus_type != PCIE_MODE) {
        struct net_device *dev = ltrace->aml_hw->vif_table[0]->ndev;
        printk("please disable the firewall(setenforce 0),sdio and usb trace_log are saved in /data/trace_log.txt\n");
        return aml_set_fwlog_cmd(dev, 1);
    }

    return aml_fw_trace_read(ltrace->trace, &ltrace->lbuf,
                              dont_wait, user_buf, count);
}

static ssize_t aml_dbgfs_fw_trace_write(struct file *file,
                                         const char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct aml_dbgfs_fw_trace *ltrace = file->private_data;
    int ret;

    ret = _aml_fw_trace_reset(ltrace->trace, true);
    if (ret)
        return ret;

    return count;
}

DEBUGFS_READ_WRITE_OPEN_RELEASE_FILE_OPS(fw_trace);

static ssize_t aml_dbgfs_fw_trace_level_read(struct file *file,
                                              char __user *user_buf,
                                              size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    return aml_fw_trace_level_read(&priv->debugfs.fw_trace, user_buf,
                                    count, ppos);
}

static ssize_t aml_dbgfs_fw_trace_level_write(struct file *file,
                                               const char __user *user_buf,
                                               size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    return aml_fw_trace_level_write(&priv->debugfs.fw_trace, user_buf, count);
}
DEBUGFS_READ_WRITE_FILE_OPS(fw_trace_level);


#ifdef CONFIG_AML_RADAR
static ssize_t aml_dbgfs_pulses_read(struct file *file,
                                      char __user *user_buf,
                                      size_t count, loff_t *ppos,
                                      int rd_idx)
{
    struct aml_hw *priv = file->private_data;
    char *buf;
    int len = 0;
    int bufsz;
    int i;
    int index;
    struct aml_radar_pulses *p = &priv->radar.pulses[rd_idx];
    ssize_t read;

    if (*ppos != 0)
        return 0;

    /* Prevent from interrupt preemption */
    spin_lock_bh(&priv->radar.lock);
    bufsz = p->count * 34 + 51;
    bufsz += aml_radar_dump_pattern_detector(NULL, 0, &priv->radar, rd_idx);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL) {
        spin_unlock_bh(&priv->radar.lock);
        return 0;
    }

    if (p->count) {
        len += scnprintf(&buf[len], bufsz - len,
                         " PRI     WIDTH     FOM     FREQ\n");
        index = p->index;
        for (i = 0; i < p->count; i++) {
            struct radar_pulse *pulse;

            if (index > 0)
                index--;
            else
                index = AML_RADAR_PULSE_MAX - 1;

            pulse = (struct radar_pulse *) &p->buffer[index];

            len += scnprintf(&buf[len], bufsz - len,
                             "%05dus  %03dus     %2d%%    %+3dMHz\n", pulse->rep,
                             2 * pulse->len, 6 * pulse->fom, 2*pulse->freq);
        }
    }

    len += aml_radar_dump_pattern_detector(&buf[len], bufsz - len,
                                            &priv->radar, rd_idx);

    spin_unlock_bh(&priv->radar.lock);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);

    return read;
}

static ssize_t aml_dbgfs_pulses_prim_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count, loff_t *ppos)
{
    return aml_dbgfs_pulses_read(file, user_buf, count, ppos, 0);
}

DEBUGFS_READ_FILE_OPS(pulses_prim);

static ssize_t aml_dbgfs_pulses_sec_read(struct file *file,
                                          char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
    return aml_dbgfs_pulses_read(file, user_buf, count, ppos, 1);
}

DEBUGFS_READ_FILE_OPS(pulses_sec);

static ssize_t aml_dbgfs_detected_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char *buf;
    int bufsz,len = 0;
    ssize_t read;

    if (*ppos != 0)
        return 0;

    bufsz = 5; // RIU:\n
    bufsz += aml_radar_dump_radar_detected(NULL, 0, &priv->radar,
                                            AML_RADAR_RIU);

    if (priv->phy.cnt > 1) {
        bufsz += 5; // FCU:\n
        bufsz += aml_radar_dump_radar_detected(NULL, 0, &priv->radar,
                                                AML_RADAR_FCU);
    }

    buf = kmalloc(bufsz, GFP_KERNEL);
    if (buf == NULL) {
        return 0;
    }

    len = scnprintf(&buf[len], bufsz, "RIU:\n");
    len += aml_radar_dump_radar_detected(&buf[len], bufsz - len, &priv->radar,
                                            AML_RADAR_RIU);

    if (priv->phy.cnt > 1) {
        len += scnprintf(&buf[len], bufsz - len, "FCU:\n");
        len += aml_radar_dump_radar_detected(&buf[len], bufsz - len,
                                              &priv->radar, AML_RADAR_FCU);
    }

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);

    return read;
}

DEBUGFS_READ_FILE_OPS(detected);

static ssize_t aml_dbgfs_enable_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "RIU=%d FCU=%d\n", priv->radar.dpd[AML_RADAR_RIU]->enabled,
                    priv->radar.dpd[AML_RADAR_FCU]->enabled);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_enable_write(struct file *file,
                                     const char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if (sscanf(buf, "RIU=%d", &val) > 0)
        aml_radar_detection_enable(&priv->radar, val, AML_RADAR_RIU);

    if (sscanf(buf, "FCU=%d", &val) > 0)
        aml_radar_detection_enable(&priv->radar, val, AML_RADAR_FCU);

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(enable);

static ssize_t aml_dbgfs_band_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "BAND=%d\n", priv->phy.sec_chan.band);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_band_write(struct file *file,
                                     const char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if ((sscanf(buf, "%d", &val) > 0) && (val >= 0) && (val <= NL80211_BAND_5GHZ))
        priv->phy.sec_chan.band = val;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(band);

static ssize_t aml_dbgfs_type_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "TYPE=%d\n", priv->phy.sec_chan.type);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_type_write(struct file *file,
                                     const char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if ((sscanf(buf, "%d", &val) > 0) && (val >= PHY_CHNL_BW_20) &&
        (val <= PHY_CHNL_BW_80P80))
        priv->phy.sec_chan.type = val;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(type);

static ssize_t aml_dbgfs_prim20_read(struct file *file,
                                      char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "PRIM20=%dMHz\n", priv->phy.sec_chan.prim20_freq);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_prim20_write(struct file *file,
                                       const char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if (sscanf(buf, "%d", &val) > 0)
        priv->phy.sec_chan.prim20_freq = val;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(prim20);

static ssize_t aml_dbgfs_center1_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "CENTER1=%dMHz\n", priv->phy.sec_chan.center1_freq);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_center1_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if (sscanf(buf, "%d", &val) > 0)
        priv->phy.sec_chan.center1_freq = val;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(center1);

static ssize_t aml_dbgfs_center2_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "CENTER2=%dMHz\n", priv->phy.sec_chan.center2_freq);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_center2_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if (sscanf(buf, "%d", &val) > 0)
        priv->phy.sec_chan.center2_freq = val;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(center2);


static ssize_t aml_dbgfs_set_read(struct file *file,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    return 0;
}

static ssize_t aml_dbgfs_set_write(struct file *file,
                                    const char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    struct aml_hw *priv = file->private_data;

    aml_send_set_channel(priv, 1, NULL);
    aml_radar_detection_enable(&priv->radar, AML_RADAR_DETECT_ENABLE,
                                AML_RADAR_FCU);

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(set);
#endif /* CONFIG_AML_RADAR */

#ifdef CONFIG_AML_FULLMAC

static const int ru_size_he_er[] =
{
    242,
    106
};

static const int ru_size_he_mu[] =
{
    26,
    52,
    106,
    242,
    484,
    996
};

#ifdef CONFIG_AML_DEBUGFS
int compare_idx(const void *st1, const void *st2)
{
    int index1 = ((struct st *)st1)->r_idx;
    int index2 = ((struct st *)st2)->r_idx;

    if (index1 > index2) return 1;
    if (index1 < index2) return -1;

    return 0;
}

int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
                      int sgi, int pre, int dcm, int *r_idx, bool bprint)
{
    int res = 0;
    int bitrates_cck[4] = { 10, 20, 55, 110 };
    int bitrates_ofdm[8] = { 6, 9, 12, 18, 24, 36, 48, 54};
    char he_gi[3][4] = {"0.8", "1.6", "3.2"};

    if (format < FORMATMOD_HT_MF) {
        if (mcs < 4) {
            if (r_idx) {
                *r_idx = (mcs * 2) + pre;
                if (bprint) {
                    printk("%4d ", *r_idx);
                } else {
                    res = scnprintf(buf, size - res, "%4d ", *r_idx);
                }
            }
            if (bprint) {
                printk("L-CCK/%cP%11c%2u.%1uM   ", pre > 0 ? 'L' : 'S', ' ', bitrates_cck[mcs] / 10, bitrates_cck[mcs] % 10);
            } else {
                res += scnprintf(&buf[res], size - res, "L-CCK/%cP%11c%2u.%1uM   ",
                                 pre > 0 ? 'L' : 'S', ' ',
                                 bitrates_cck[mcs] / 10,
                                 bitrates_cck[mcs] % 10);
            }
        } else {
            mcs -= 4;
            if (r_idx) {
                *r_idx = N_CCK + mcs;
                if (bprint) {
                    printk("%4d ", *r_idx);
                } else {
                    res = scnprintf(buf, size - res, "%4d ", *r_idx);
                }
            }
            if (bprint) {
                printk("L-OFDM%13c%2u.0M   ", ' ', bitrates_ofdm[mcs]);
            } else {
                res += scnprintf(&buf[res], size - res, "L-OFDM%13c%2u.0M   ",
                                 ' ', bitrates_ofdm[mcs]);
            }
        }
    } else if (format < FORMATMOD_VHT) {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + nss * 32 + mcs * 4 + bw * 2 + sgi;
            if (bprint) {
                printk("%4d ", *r_idx);
            } else {
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
        }
        mcs += nss * 8;
        if (bprint) {
            printk("HT%d/%cGI%11cMCS%-2d   ", 20 * (1 << bw), sgi ? 'S' : 'L', ' ', mcs);
        } else {
            res += scnprintf(&buf[res], size - res, "HT%d/%cGI%11cMCS%-2d   ",
                             20 * (1 << bw), sgi ? 'S' : 'L', ' ', mcs);
        }
    } else if (format == FORMATMOD_VHT){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + nss * 80 + mcs * 8 + bw * 2 + sgi;
            if (bprint) {
                printk("%4d ", *r_idx);
            } else {
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
        }
        if (bprint) {
            printk("VHT%d/%cGI%*cMCS%d/%1d  ", 20 * (1 << bw), sgi ? 'S' : 'L', bw > 2 ? 9 : 10, ' ',
                     mcs, nss + 1);
        } else {
            res += scnprintf(&buf[res], size - res, "VHT%d/%cGI%*cMCS%d/%1d  ",
                             20 * (1 << bw), sgi ? 'S' : 'L', bw > 2 ? 9 : 10, ' ',
                             mcs, nss + 1);
        }
    } else if (format == FORMATMOD_HE_SU){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + nss * 144 + mcs * 12 + bw * 3 + sgi;
            if (bprint) {
                printk("%4d ", *r_idx);
            } else {
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
        }
        if (bprint) {
            printk("HE%d/GI%s%4s%*cMCS%d/%1d%*c",
                    20 * (1 << bw), he_gi[sgi], dcm ? "/DCM" : "",
                    bw > 2 ? 4 : 5, ' ', mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        } else {
            res += scnprintf(&buf[res], size - res, "HE%d/GI%s%4s%*cMCS%d/%1d%*c",
                             20 * (1 << bw), he_gi[sgi], dcm ? "/DCM" : "",
                             bw > 2 ? 4 : 5, ' ', mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        }
    } else if (format == FORMATMOD_HE_MU){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + nss * 216 + mcs * 18 + bw * 3 + sgi;
            if (bprint) {
                printk("%4d ", *r_idx);
            } else {
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
        }
        if (bprint) {
            printk("HEMU-%d/GI%s%*cMCS%d/%1d%*c",
                     ru_size_he_mu[bw], he_gi[sgi], bw > 1 ? 5 : 6, ' ',
                     mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        } else {
            res += scnprintf(&buf[res], size - res, "HEMU-%d/GI%s%*cMCS%d/%1d%*c",
                             ru_size_he_mu[bw], he_gi[sgi], bw > 1 ? 5 : 6, ' ',
                             mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        }

    }
    else // HE ER
    {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU + bw * 9 + mcs * 3 + sgi;
            if (bprint) {
                printk("%4d ", *r_idx);
            } else {
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
        }
        if (bprint) {
            printk("HEER-%d/GI%s%4s%1cMCS%d/%1d%2c",
                     ru_size_he_er[bw], he_gi[sgi], dcm ? "/DCM" : "",
                     ' ', mcs, nss + 1, ' ');
        } else {
            res += scnprintf(&buf[res], size - res, "HEER-%d/GI%s%4s%1cMCS%d/%1d%2c",
                             ru_size_he_er[bw], he_gi[sgi], dcm ? "/DCM" : "",
                             ' ', mcs, nss + 1, ' ');
        }
    }

    return res;
}

int print_rate_from_cfg(char *buf, int size, u32 rate_config, int *r_idx, int ru_size, bool bprint)
{
    union aml_rate_ctrl_info *r_cfg = (union aml_rate_ctrl_info *)&rate_config;
    union aml_mcs_index *mcs_index = (union aml_mcs_index *)&rate_config;
    unsigned int ft, pre, gi, bw, nss, mcs, dcm, len;

    ft = r_cfg->formatModTx;
    pre = r_cfg->giAndPreTypeTx >> 1;
    gi = r_cfg->giAndPreTypeTx;
    bw = r_cfg->bwTx;
    dcm = 0;
    if (ft >= FORMATMOD_HE_SU) {
        mcs = mcs_index->he.mcs;
        nss = mcs_index->he.nss;
        dcm = r_cfg->dcmTx;
        if (ft == FORMATMOD_HE_MU)
            bw = ru_size > 3 ? ru_size - 3 : ru_size;
    } else if (ft == FORMATMOD_VHT) {
        mcs = mcs_index->vht.mcs;
        nss = mcs_index->vht.nss;
    } else if (ft >= FORMATMOD_HT_MF) {
        mcs = mcs_index->ht.mcs;
        nss = mcs_index->ht.nss;
    } else {
        mcs = mcs_index->legacy;
        nss = 0;
    }

    len = print_rate(buf, size, ft, nss, mcs, bw, gi, pre, dcm, r_idx, bprint);
    return len;
}

void idx_to_rate_cfg(int idx, union aml_rate_ctrl_info *r_cfg, int *ru_size)
{
    r_cfg->value = 0;
    if (idx < N_CCK)
    {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->giAndPreTypeTx = (idx & 1) << 1;
        r_cfg->mcsIndexTx = idx / 2;
    }
    else if (idx < (N_CCK + N_OFDM))
    {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->mcsIndexTx =  idx - N_CCK + 4;
    }
    else if (idx < (N_CCK + N_OFDM + N_HT))
    {
        union aml_mcs_index *r = (union aml_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM);
        r_cfg->formatModTx = FORMATMOD_HT_MF;
        r->ht.nss = idx / (8*2*2);
        r->ht.mcs = (idx % (8*2*2)) / (2*2);
        r_cfg->bwTx = ((idx % (8*2*2)) % (2*2)) / 2;
        r_cfg->giAndPreTypeTx = idx & 1;
    }
    else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT))
    {
        union aml_mcs_index *r = (union aml_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM + N_HT);
        r_cfg->formatModTx = FORMATMOD_VHT;
        r->vht.nss = idx / (10*4*2);
        r->vht.mcs = (idx % (10*4*2)) / (4*2);
        r_cfg->bwTx = ((idx % (10*4*2)) % (4*2)) / 2;
        r_cfg->giAndPreTypeTx = idx & 1;
    }
    else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU))
    {
        union aml_mcs_index *r = (union aml_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM + N_HT + N_VHT);
        r_cfg->formatModTx = FORMATMOD_HE_SU;
        r->vht.nss = idx / (12*4*3);
        r->vht.mcs = (idx % (12*4*3)) / (4*3);
        r_cfg->bwTx = ((idx % (12*4*3)) % (4*3)) / 3;
        r_cfg->giAndPreTypeTx = idx % 3;
    }
    else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU))
    {
        union aml_mcs_index *r = (union aml_mcs_index *)r_cfg;

        BUG_ON(ru_size == NULL);

        idx -= (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU);
        r_cfg->formatModTx = FORMATMOD_HE_MU;
        r->vht.nss = idx / (12*6*3);
        r->vht.mcs = (idx % (12*6*3)) / (6*3);
        *ru_size = ((idx % (12*6*3)) % (6*3)) / 3;
        r_cfg->giAndPreTypeTx = idx % 3;
        r_cfg->bwTx = 0;
    }
    else
    {
        union aml_mcs_index *r = (union aml_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU);
        r_cfg->formatModTx = FORMATMOD_HE_ER;
        r_cfg->bwTx = idx / 9;
        if (ru_size)
            *ru_size = idx / 9;
        r_cfg->giAndPreTypeTx = idx % 3;
        r->vht.mcs = (idx % 9) / 3;
        r->vht.nss = 0;
    }
}
#endif

static struct aml_sta* aml_dbgfs_get_sta(struct aml_hw *aml_hw,
                                           char* mac_addr)
{
    u8 mac[6];

    if (sscanf(mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6)
        return NULL;
    return aml_get_sta(aml_hw, mac);
}

static ssize_t aml_dbgfs_twt_request_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count,
                                           loff_t *ppos)
{
    char buf[750];
    ssize_t read;
    struct aml_hw *priv = file->private_data;
    struct aml_sta *sta = NULL;
    int len;

    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;
    if (sta->twt_ind.sta_idx != AML_INVALID_STA)
    {
        struct twt_conf_tag *conf = &sta->twt_ind.conf;
        if (sta->twt_ind.resp_type == MAC_TWT_SETUP_ACCEPT)
            len = scnprintf(buf, sizeof(buf) - 1, "Accepted configuration");
        else if (sta->twt_ind.resp_type == MAC_TWT_SETUP_ALTERNATE)
            len = scnprintf(buf, sizeof(buf) - 1, "Alternate configuration proposed by AP");
        else if (sta->twt_ind.resp_type == MAC_TWT_SETUP_DICTATE)
            len = scnprintf(buf, sizeof(buf) - 1, "AP dictates the following configuration");
        else if (sta->twt_ind.resp_type == MAC_TWT_SETUP_REJECT)
            len = scnprintf(buf, sizeof(buf) - 1, "AP rejects the following configuration");
        else
        {
            len = scnprintf(buf, sizeof(buf) - 1, "Invalid response from the peer");
            goto end;
        }
        len += scnprintf(&buf[len], sizeof(buf) - 1 - len,":\n"
                         "flow_type = %d\n"
                         "wake interval mantissa = %d\n"
                         "wake interval exponent = %d\n"
                         "wake interval = %d us\n"
                         "nominal minimum wake duration = %d us\n",
                         conf->flow_type, conf->wake_int_mantissa,
                         conf->wake_int_exp,
                         conf->wake_int_mantissa << conf->wake_int_exp,
                         conf->wake_dur_unit ?
                         conf->min_twt_wake_dur * 1024:
                         conf->min_twt_wake_dur * 256);
    }
    else
    {
        len = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                        "setup_command = <0: request, 1: suggest, 2: demand>,"
                        "flow_type = <0: announced, 1: unannounced>,"
                        "wake_interval_mantissa = <0 if setup request and no constraints>,"
                        "wake_interval_exp = <0 if setup request and no constraints>,"
                        "nominal_min_wake_dur = <0 if setup request and no constraints>,"
                        "wake_dur_unit = <0: 256us, 1: tu>");
    }
  end:
    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);
    return read;
}

static ssize_t aml_dbgfs_twt_request_write(struct file *file,
                                            const char __user *user_buf,
                                            size_t count,
                                            loff_t *ppos)
{
    char *accepted_params[] = {"setup_command",
                               "flow_type",
                               "wake_interval_mantissa",
                               "wake_interval_exp",
                               "nominal_min_wake_dur",
                               "wake_dur_unit",
                               0
                               };
    struct twt_conf_tag twt_conf;
    struct twt_setup_cfm twt_setup_cfm;
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;
    char buf[1024], param[30];
    char *line;
    int error = 1, i, val, setup_command = -1;
    bool_l found;
    size_t len = sizeof(buf) - 1;

    AML_DBG(AML_FN_ENTRY_STR);
    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;

    /* Get the content of the file */
    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';
    memset(&twt_conf, 0, sizeof(twt_conf));

    line = buf;
    /* Get the content of the file */
    while (line != NULL)
    {
        if (sscanf(line, "%s = %d", param, &val) == 2)
        {
            i = 0;
            found = false;
            // Check if parameter is valid
            while(accepted_params[i])
            {
                if (strcmp(accepted_params[i], param) == 0)
                {
                    found = true;
                    break;
                }
                i++;
            }

            if (!found)
            {
                dev_err(priv->dev, "%s: parameter %s is not valid\n", __func__, param);
                return -EINVAL;
            }

            if (!strcmp(param, "setup_command"))
            {
                setup_command = val;
            }
            else if (!strcmp(param, "flow_type"))
            {
                twt_conf.flow_type = val;
            }
            else if (!strcmp(param, "wake_interval_mantissa"))
            {
                twt_conf.wake_int_mantissa = val;
            }
            else if (!strcmp(param, "wake_interval_exp"))
            {
                twt_conf.wake_int_exp = val;
            }
            else if (!strcmp(param, "nominal_min_wake_dur"))
            {
                twt_conf.min_twt_wake_dur = val;
            }
            else if (!strcmp(param, "wake_dur_unit"))
            {
                twt_conf.wake_dur_unit = val;
            }
        }
        else
        {
            dev_err(priv->dev, "%s: Impossible to read TWT configuration option\n", __func__);
            return -EFAULT;
        }
        line = strchr(line, ',');
        if(line == NULL)
            break;
        line++;
    }

    if (setup_command == -1)
    {
        dev_err(priv->dev, "%s: TWT missing setup command\n", __func__);
        return -EFAULT;
    }

    // Forward the request to the LMAC
    if ((error = aml_send_twt_request(priv, setup_command, sta->vif_idx,
                                       &twt_conf, &twt_setup_cfm)) != 0)
        return error;

    // Check the status
    if (twt_setup_cfm.status != CO_OK)
        return -EIO;

    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(twt_request);

static ssize_t aml_dbgfs_twt_teardown_read(struct file *file,
                                            char __user *user_buf,
                                            size_t count,
                                            loff_t *ppos)
{
    char buf[512];
    int ret;
    ssize_t read;


    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "TWT teardown format:\n\n"
                    "flow_id = <ID>\n");
    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t aml_dbgfs_twt_teardown_write(struct file *file,
                                             const char __user *user_buf,
                                             size_t count,
                                             loff_t *ppos)
{
    struct twt_teardown_req twt_teardown;
    struct twt_teardown_cfm twt_teardown_cfm;
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;
    char buf[256];
    char *line;
    int error = 1;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    AML_DBG(AML_FN_ENTRY_STR);
    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;

    /* Get the content of the file */
    if (copy_from_user(buf, user_buf, len))
        return -EINVAL;

    buf[len] = '\0';
    memset(&twt_teardown, 0, sizeof(twt_teardown));

    /* Get the content of the file */
    line = buf;

    if (sscanf(line, "flow_id = %d", (int *) &twt_teardown.id) != 1)
    {
        dev_err(priv->dev, "%s: Invalid TWT configuration\n", __func__);
        return -EINVAL;
    }

    twt_teardown.neg_type = 0;
    twt_teardown.all_twt = 0;
    twt_teardown.vif_idx = sta->vif_idx;

    // Forward the request to the LMAC
    if ((error = _aml_send_twt_teardown(priv, &twt_teardown, &twt_teardown_cfm)) != 0)
        return error;

    // Check the status
    if (twt_teardown_cfm.status != CO_OK)
        return -EIO;

    return count;
}
DEBUGFS_READ_WRITE_FILE_OPS(twt_teardown);

static ssize_t aml_dbgfs_rc_stats_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;
    char *buf;
    int bufsz, len = 0;
    ssize_t read;
    int i = 0;
    int error = 0;
    struct me_rc_stats_cfm me_rc_stats_cfm;
    unsigned int no_samples;
    struct st *st;

    AML_DBG(AML_FN_ENTRY_STR);

    /* everything should fit in one call */
    if (*ppos)
        return 0;

    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;

    /* Forward the information to the LMAC */
    if ((error = aml_send_me_rc_stats(priv, sta->sta_idx, &me_rc_stats_cfm)))
        return error;

    no_samples = me_rc_stats_cfm.no_samples;
    if (no_samples == 0)
        return 0;

    bufsz = no_samples * LINE_MAX_SZ + 500;

    buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    st = kmalloc(sizeof(struct st) * no_samples, GFP_ATOMIC);
    if (st == NULL)
    {
        kfree(buf);
        return 0;
    }

    for (i = 0; i < no_samples; i++)
    {
        unsigned int tp, eprob;
        len = print_rate_from_cfg(st[i].line, LINE_MAX_SZ,
                                  me_rc_stats_cfm.rate_stats[i].rate_config,
                                  (int *)&st[i].r_idx, 0, 0);

        if (me_rc_stats_cfm.sw_retry_step != 0)
        {
            len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len,  "%c",
                    me_rc_stats_cfm.retry_step_idx[me_rc_stats_cfm.sw_retry_step] == i ? '*' : ' ');
        }
        else
        {
            len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, " ");
        }
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c",
                me_rc_stats_cfm.retry_step_idx[0] == i ? 'T' : ' ');
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c",
                me_rc_stats_cfm.retry_step_idx[1] == i ? 't' : ' ');
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c ",
                me_rc_stats_cfm.retry_step_idx[2] == i ? 'P' : ' ');

        tp = me_rc_stats_cfm.tp[i] / 10;
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, " %4u.%1u",
                         tp / 10, tp % 10);

        eprob = ((me_rc_stats_cfm.rate_stats[i].probability * 1000) >> 16) + 1;
        scnprintf(&st[i].line[len],LINE_MAX_SZ - len,
                         "  %4u.%1u %5u(%6u)  %6u",
                         eprob / 10, eprob % 10,
                         me_rc_stats_cfm.rate_stats[i].success,
                         me_rc_stats_cfm.rate_stats[i].attempts,
                         me_rc_stats_cfm.rate_stats[i].sample_skipped);
    }
    len = scnprintf(buf, bufsz ,
                     "\nTX rate info for %02X:%02X:%02X:%02X:%02X:%02X:\n",
                     sta->mac_addr[0], sta->mac_addr[1], sta->mac_addr[2],
                     sta->mac_addr[3], sta->mac_addr[4], sta->mac_addr[5]);

    len += scnprintf(&buf[len], bufsz - len,
            "   # type               rate             tpt   eprob    ok(   tot)   skipped\n");

    // add sorted statistics to the buffer
    sort(st, no_samples, sizeof(st[0]), compare_idx, NULL);
    for (i = 0; i < no_samples; i++)
    {
        len += scnprintf(&buf[len], bufsz - len, "%s\n", st[i].line);
    }

    // display HE TB statistics if any
    if (me_rc_stats_cfm.rate_stats[RC_HE_STATS_IDX].rate_config != 0) {
        unsigned int tp, eprob;
        struct rc_rate_stats *rate_stats = &me_rc_stats_cfm.rate_stats[RC_HE_STATS_IDX];
        int ru_index = rate_stats->ru_and_length & 0x07;
        int ul_length = rate_stats->ru_and_length >> 3;

        len += scnprintf(&buf[len], bufsz - len,
                         "\nHE TB rate info:\n");

        len += scnprintf(&buf[len], bufsz - len,
                "     type               rate             tpt   eprob    ok(   tot)   ul_length\n     ");
        len += print_rate_from_cfg(&buf[len], bufsz - len, rate_stats->rate_config,
                                   NULL, ru_index, 0);

        tp = me_rc_stats_cfm.tp[RC_HE_STATS_IDX] / 10;
        len += scnprintf(&buf[len], bufsz - len, "      %4u.%1u",
                         tp / 10, tp % 10);

        eprob = ((rate_stats->probability * 1000) >> 16) + 1;
        len += scnprintf(&buf[len],bufsz - len,
                         "  %4u.%1u %5u(%6u)  %6u\n",
                         eprob / 10, eprob % 10,
                         rate_stats->success,
                         rate_stats->attempts,
                         ul_length);
    }

    len += scnprintf(&buf[len], bufsz - len, "\n MPDUs AMPDUs AvLen trialP");
    len += scnprintf(&buf[len], bufsz - len, "\n%6u %6u %3d.%1d %6u\n",
                     me_rc_stats_cfm.ampdu_len,
                     me_rc_stats_cfm.ampdu_packets,
                     me_rc_stats_cfm.avg_ampdu_len >> 16,
                     ((me_rc_stats_cfm.avg_ampdu_len * 10) >> 16) % 10,
                     me_rc_stats_cfm.sample_wait);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);
    kfree(st);

    return read;
}

DEBUGFS_READ_FILE_OPS(rc_stats);

static ssize_t aml_dbgfs_rc_fixed_rate_idx_write(struct file *file,
                                                  const char __user *user_buf,
                                                  size_t count, loff_t *ppos)
{
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;
    char buf[10];
    int fixed_rate_idx = -1;
    union aml_rate_ctrl_info rate_config;
    int error = 0;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    AML_DBG(AML_FN_ENTRY_STR);

    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);

    if (sta == NULL)
        return -EINVAL;

    /* Get the content of the file */
    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';
    if (sscanf(buf, "%i\n", &fixed_rate_idx) != 1) {
        fixed_rate_idx = -1;
    }

    /* Convert rate index into rate configuration */
    if ((fixed_rate_idx < 0) || (fixed_rate_idx >= (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU + N_HE_ER)))
    {
        // disable fixed rate
        rate_config.value = (u32)-1;
    }
    else
    {
        idx_to_rate_cfg(fixed_rate_idx, &rate_config, NULL);
    }

    // Forward the request to the LMAC
    if ((error = aml_send_me_rc_set_rate(priv, sta->sta_idx,
                                          (u16)rate_config.value)) != 0)
    {
        return error;
    }

    priv->debugfs.rc_config[sta->sta_idx] = (int)rate_config.value;
    return len;
}

DEBUGFS_WRITE_FILE_OPS(rc_fixed_rate_idx);

static ssize_t aml_dbgfs_last_rx_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    ssize_t read = 0;
#ifdef CONFIG_AML_DEBUGFS
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;
    struct aml_rx_rate_stats *rate_stats;
    char *buf;
    int bufsz, i, len = 0;
    unsigned int fmt, pre, bw, nss, mcs, gi, dcm = 0;
    struct rx_vector_1 *last_rx;
    char hist[] = "##################################################";
    int hist_len = sizeof(hist) - 1;
    u8 nrx;

    AML_DBG(AML_FN_ENTRY_STR);

    /* everything should fit in one call */
    if (*ppos)
        return 0;

    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;

    rate_stats = &sta->stats.rx_rate;
    bufsz = (rate_stats->rate_cnt * ( 50 + hist_len) + 200);
    buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    // Get number of RX paths
    nrx = (priv->version_cfm.version_phy_1 & MDM_NRX_MASK) >> MDM_NRX_LSB;

    len += scnprintf(buf, bufsz,
                     "\nRX rate info for %02X:%02X:%02X:%02X:%02X:%02X:\n",
                     sta->mac_addr[0], sta->mac_addr[1], sta->mac_addr[2],
                     sta->mac_addr[3], sta->mac_addr[4], sta->mac_addr[5]);

    // Display Statistics
    for (i = 0; i < rate_stats->size; i++)
    {
        if (rate_stats->table && rate_stats->table[i]) {
            union aml_rate_ctrl_info rate_config;
            u64 percent = div_u64(rate_stats->table[i] * 1000, rate_stats->cpt);
            u64 p;
            int ru_size;
            u32 rem;

            idx_to_rate_cfg(i, &rate_config, &ru_size);
            len += print_rate_from_cfg(&buf[len], bufsz - len,
                                       rate_config.value, NULL, ru_size, 0);
            p = div_u64((percent * hist_len), 1000);
            len += scnprintf(&buf[len], bufsz - len, ": %9d(%2d.%1d%%)%.*s\n",
                             rate_stats->table[i],
                             div_u64_rem(percent, 10, &rem), rem, p, hist);
        }
    }

    // Display detailed info of the last received rate
    last_rx = &sta->stats.last_rx.rx_vect1;
    len += scnprintf(&buf[len], bufsz - len,"\nLast received rate\n"
                     "type               rate     LDPC STBC BEAMFM DCM DOPPLER %s\n",
                     (nrx > 1) ? "rssi1(dBm) rssi2(dBm)" : "rssi(dBm)");

    fmt = last_rx->format_mod;
    bw = last_rx->ch_bw;
    pre = last_rx->pre_type;
    if (fmt >= FORMATMOD_HE_SU) {
        mcs = last_rx->he.mcs;
        nss = last_rx->he.nss;
        gi = last_rx->he.gi_type;
        if ((fmt == FORMATMOD_HE_MU) || (fmt == FORMATMOD_HE_ER))
            bw = last_rx->he.ru_size;
        dcm = last_rx->he.dcm;
    } else if (fmt == FORMATMOD_VHT) {
        mcs = last_rx->vht.mcs;
        nss = last_rx->vht.nss;
        gi = last_rx->vht.short_gi;
    } else if (fmt >= FORMATMOD_HT_MF) {
        mcs = last_rx->ht.mcs % 8;
        nss = last_rx->ht.mcs / 8;;
        gi = last_rx->ht.short_gi;
    } else {
        BUG_ON((mcs = legrates_lut[last_rx->leg_rate].idx) == -1);
        nss = 0;
        gi = 0;
    }

    len += print_rate(&buf[len], bufsz - len, fmt, nss, mcs, bw, gi, pre, dcm, NULL, 0);

    /* flags for HT/VHT/HE */
    if (fmt >= FORMATMOD_HE_SU) {
        len += scnprintf(&buf[len], bufsz - len, "  %c    %c     %c    %c     %c",
                         last_rx->he.fec ? 'L' : ' ',
                         last_rx->he.stbc ? 'S' : ' ',
                         last_rx->he.beamformed ? 'B' : ' ',
                         last_rx->he.dcm ? 'D' : ' ',
                         last_rx->he.doppler ? 'D' : ' ');
    } else if (fmt == FORMATMOD_VHT) {
        len += scnprintf(&buf[len], bufsz - len, "  %c    %c     %c           ",
                         last_rx->vht.fec ? 'L' : ' ',
                         last_rx->vht.stbc ? 'S' : ' ',
                         last_rx->vht.beamformed ? 'B' : ' ');
    } else if (fmt >= FORMATMOD_HT_MF) {
        len += scnprintf(&buf[len], bufsz - len, "  %c    %c                  ",
                         last_rx->ht.fec ? 'L' : ' ',
                         last_rx->ht.stbc ? 'S' : ' ');
    } else {
        len += scnprintf(&buf[len], bufsz - len, "                         ");
    }
    if (nrx > 1) {
        len += scnprintf(&buf[len], bufsz - len, "       %-4d       %d\n",
                         last_rx->rssi1, last_rx->rssi1);
    } else {
        len += scnprintf(&buf[len], bufsz - len, "      %d\n", last_rx->rssi1);
    }

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);
#endif
    return read;
}

static ssize_t aml_dbgfs_last_rx_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct aml_sta *sta = NULL;
    struct aml_hw *priv = file->private_data;

    /* Get the station index from MAC address */
    sta = aml_dbgfs_get_sta(priv, (char *)file->f_path.dentry->d_parent->d_parent->d_iname);
    if (sta == NULL)
        return -EINVAL;

    /* Prevent from interrupt preemption as these statistics are updated under
     * interrupt */
#ifdef CONFIG_AML_DEBUGFS
    spin_lock_bh(&priv->tx_lock);
    memset(sta->stats.rx_rate.table, 0,
           sta->stats.rx_rate.size * sizeof(sta->stats.rx_rate.table[0]));
    sta->stats.rx_rate.cpt = 0;
    sta->stats.rx_rate.rate_cnt = 0;
    spin_unlock_bh(&priv->tx_lock);
#endif
    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(last_rx);

#endif /* CONFIG_AML_FULLMAC */

/*
 * trace helper
 */
#ifdef CONFIG_AML_DEBUGFS
void aml_fw_trace_dump(struct aml_hw *aml_hw)
{
    /* may be called before aml_dbgfs_register */
    if (aml_hw->plat->enabled && !aml_hw->debugfs.fw_trace.buf.data) {
        if (aml_fw_trace_buf_init(&aml_hw->debugfs.fw_trace.buf,
                               aml_ipc_fw_trace_desc_get(aml_hw))) {
            return;
        }

    }

    if (!aml_hw->debugfs.fw_trace.buf.data)
        return;

    _aml_fw_trace_dump(aml_hw, &aml_hw->debugfs.fw_trace.buf);
}

void aml_fw_trace_reset(struct aml_hw *aml_hw)
{
    _aml_fw_trace_reset(&aml_hw->debugfs.fw_trace, true);
}

void aml_dbgfs_trigger_fw_dump(struct aml_hw *aml_hw, char *reason)
{
    aml_send_dbg_trigger_req(aml_hw, reason);
}

#ifdef CONFIG_AML_FULLMAC
extern struct aml_rx_rate_stats gst_rx_rate;
extern struct aml_dyn_snr_cfg g_dyn_snr;

extern struct aml_vif *aml_rx_get_vif(struct aml_hw *aml_hw, int vif_idx);
static void _aml_dbgfs_register_sta(struct aml_debugfs *aml_debugfs, struct aml_sta *sta)
{
    struct aml_hw *aml_hw = container_of(aml_debugfs, struct aml_hw, debugfs);
    struct dentry *dir_sta;
    char sta_name[18];
    struct dentry *dir_rc;
    struct dentry *file;
    struct aml_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    int nb_rx_rate = N_CCK + N_OFDM;
    struct aml_rc_config_save *rc_cfg, *next;
    struct aml_vif * vif;

    if (sta->sta_idx >= NX_REMOTE_STA_MAX) {
        scnprintf(sta_name, sizeof(sta_name), "bc_mc");
    } else {
        scnprintf(sta_name, sizeof(sta_name), "%pM", sta->mac_addr);
    }

    if (!(dir_sta = debugfs_create_dir(sta_name, aml_debugfs->dir_stas)))
        goto error;
    aml_debugfs->dir_sta[sta->sta_idx] = dir_sta;

    if (!(dir_rc = debugfs_create_dir("rc", aml_debugfs->dir_sta[sta->sta_idx])))
        goto error_after_dir;

    aml_debugfs->dir_rc_sta[sta->sta_idx] = dir_rc;

    file = debugfs_create_file("stats", S_IRUSR, dir_rc, aml_hw,
                               &aml_dbgfs_rc_stats_ops);
    if (IS_ERR_OR_NULL(file))
        goto error_after_dir;

    file = debugfs_create_file("fixed_rate_idx", S_IWUSR , dir_rc, aml_hw,
                               &aml_dbgfs_rc_fixed_rate_idx_ops);
    if (IS_ERR_OR_NULL(file))
        goto error_after_dir;

    file = debugfs_create_file("rx_rate", S_IRUSR | S_IWUSR, dir_rc, aml_hw,
                               &aml_dbgfs_last_rx_ops);
    if (IS_ERR_OR_NULL(file))
        goto error_after_dir;

    if (aml_hw->mod_params->ht_on)
        nb_rx_rate += N_HT;

    if (aml_hw->mod_params->vht_on)
        nb_rx_rate += N_VHT;

    if (aml_hw->mod_params->he_on)
        nb_rx_rate += N_HE_SU + N_HE_MU + N_HE_ER;

    rate_stats->table = kzalloc(nb_rx_rate * sizeof(rate_stats->table[0]),
                                GFP_KERNEL);
    if (!rate_stats->table)
        goto error_after_dir;
    vif = aml_rx_get_vif(aml_hw, sta->vif_idx);
    if (gst_rx_rate.table == NULL && vif && (vif->is_sta_mode)) {
        gst_rx_rate.table = kzalloc(nb_rx_rate * sizeof(rate_stats->table[0]),
                                    GFP_KERNEL);
        if (!gst_rx_rate.table)
            goto error_after_dir;
        gst_rx_rate.size = nb_rx_rate;
        gst_rx_rate.cpt = 0;
        gst_rx_rate.rate_cnt = 0;

        memset(&g_dyn_snr, 0, sizeof(struct aml_dyn_snr_cfg));
        g_dyn_snr.enable = 1;
        g_dyn_snr.snr_mcs_ration = 60;
    }

    rate_stats->size = nb_rx_rate;
    rate_stats->cpt = 0;
    rate_stats->rate_cnt = 0;

    /* By default enable rate contoller */
    aml_debugfs->rc_config[sta->sta_idx] = -1;

    /* Unless we already fix the rate for this station */
    list_for_each_entry_safe(rc_cfg, next, &aml_debugfs->rc_config_save, list) {
        if (jiffies_to_msecs(jiffies - rc_cfg->timestamp) > RC_CONFIG_DUR) {
            list_del(&rc_cfg->list);
            kfree(rc_cfg);
        } else if (!memcmp(rc_cfg->mac_addr, sta->mac_addr, ETH_ALEN)) {
            aml_debugfs->rc_config[sta->sta_idx] = rc_cfg->rate;
            list_del(&rc_cfg->list);
            kfree(rc_cfg);
            break;
        }
    }

    if ((aml_debugfs->rc_config[sta->sta_idx] >= 0) &&
        aml_send_me_rc_set_rate(aml_hw, sta->sta_idx,
                                 (u16)aml_debugfs->rc_config[sta->sta_idx]))
        aml_debugfs->rc_config[sta->sta_idx] = -1;


    if ((aml_hw->vif_table[sta->vif_idx] != NULL) &&
        AML_VIF_TYPE(aml_hw->vif_table[sta->vif_idx]) == NL80211_IFTYPE_STATION)
    {
        /* register the sta */
        struct dentry *dir_twt;
        struct dentry *file;

        if (!(dir_twt = debugfs_create_dir("twt", aml_debugfs->dir_sta[sta->sta_idx])))
            goto error_after_dir;

        aml_debugfs->dir_twt_sta[sta->sta_idx] = dir_twt;

        file = debugfs_create_file("request", S_IRUSR | S_IWUSR, dir_twt, aml_hw,
                                   &aml_dbgfs_twt_request_ops);
        if (IS_ERR_OR_NULL(file))
            goto error_after_dir;

        file = debugfs_create_file("teardown", S_IRUSR | S_IWUSR, dir_twt, aml_hw,
                                   &aml_dbgfs_twt_teardown_ops);
        if (IS_ERR_OR_NULL(file))
            goto error_after_dir;

        sta->twt_ind.sta_idx = AML_INVALID_STA;
    }
    return;

error_after_dir:
    debugfs_remove_recursive(aml_debugfs->dir_sta[sta->sta_idx]);
    aml_debugfs->dir_sta[sta->sta_idx] = NULL;
    aml_debugfs->dir_rc_sta[sta->sta_idx] = NULL;
    aml_debugfs->dir_twt_sta[sta->sta_idx] = NULL;

    if (sta->stats.rx_rate.table) {
        kfree(sta->stats.rx_rate.table);
        sta->stats.rx_rate.table = NULL;
    }
    vif = aml_rx_get_vif(aml_hw, sta->vif_idx);
    if (gst_rx_rate.table && vif && vif->is_sta_mode) {
        kfree(gst_rx_rate.table);
        gst_rx_rate.table = NULL;
    }

error:
    dev_err(aml_hw->dev,
        "Error while registering debug entry for sta %d\n", sta->sta_idx);
}

static void _aml_dbgfs_unregister_sta(struct aml_debugfs *aml_debugfs, struct aml_sta *sta)
{
    struct aml_hw *aml_hw = container_of(aml_debugfs, struct aml_hw, debugfs);
    struct aml_vif * vif;

    debugfs_remove_recursive(aml_debugfs->dir_sta[sta->sta_idx]);
    /* unregister the sta */
    if (sta->stats.rx_rate.table) {
        kfree(sta->stats.rx_rate.table);
        sta->stats.rx_rate.table = NULL;
    }
    vif = aml_rx_get_vif(aml_hw, sta->vif_idx);
    if (gst_rx_rate.table && vif && vif->is_sta_mode) {
        kfree(gst_rx_rate.table);
        gst_rx_rate.table = NULL;
    }

    sta->stats.rx_rate.size = 0;
    sta->stats.rx_rate.cpt  = 0;
    sta->stats.rx_rate.rate_cnt = 0;

    /* If fix rate was set for this station, save the configuration in case
       we reconnect to this station within RC_CONFIG_DUR msec */
    if (aml_debugfs->rc_config[sta->sta_idx] >= 0) {
        struct aml_rc_config_save *rc_cfg;
        rc_cfg = kmalloc(sizeof(*rc_cfg), GFP_KERNEL);
        if (rc_cfg) {
            rc_cfg->rate = aml_debugfs->rc_config[sta->sta_idx];
            rc_cfg->timestamp = jiffies;
            memcpy(rc_cfg->mac_addr, sta->mac_addr, ETH_ALEN);
            list_add_tail(&rc_cfg->list, &aml_debugfs->rc_config_save);
        }
        /* coverity[leaked_storage] - rc_cfg have added to list */
    }

    aml_debugfs->dir_sta[sta->sta_idx] = NULL;
    aml_debugfs->dir_rc_sta[sta->sta_idx] = NULL;
    aml_debugfs->dir_twt_sta[sta->sta_idx] = NULL;
    sta->twt_ind.sta_idx = AML_INVALID_STA;
}

static void aml_sta_work(struct work_struct *ws)
{
    struct aml_debugfs *aml_debugfs = container_of(ws, struct aml_debugfs, sta_work);
    struct aml_hw *aml_hw = container_of(aml_debugfs, struct aml_hw, debugfs);
    struct aml_sta *sta;
    uint8_t sta_idx;

    sta_idx = aml_debugfs->sta_idx;
    if (sta_idx >= (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
        WARN(1, "Invalid sta index %d", sta_idx);
        return;
    }

    aml_debugfs->sta_idx = AML_INVALID_STA;
    sta = aml_hw->sta_table + sta_idx;
    if (!sta) {
        WARN(1, "Invalid sta %d", sta_idx);
        return;
    }

    if (aml_debugfs->dir_sta[sta_idx] == NULL)
        _aml_dbgfs_register_sta(aml_debugfs, sta);
    else
        _aml_dbgfs_unregister_sta(aml_debugfs, sta);

    return;
}

void _aml_dbgfs_sta_write(struct aml_debugfs *aml_debugfs, uint8_t sta_idx)
{
    if (aml_debugfs->unregistering)
        return;

    aml_debugfs->sta_idx = sta_idx;
    schedule_work(&aml_debugfs->sta_work);
}

void aml_dbgfs_unregister_sta(struct aml_hw *aml_hw,
                               struct aml_sta *sta)
{
    _aml_dbgfs_sta_write(&aml_hw->debugfs, sta->sta_idx);
}

void aml_dbgfs_register_sta(struct aml_hw *aml_hw,
                             struct aml_sta *sta)
{
    _aml_dbgfs_sta_write(&aml_hw->debugfs, sta->sta_idx);
}
#endif /* CONFIG_AML_FULLMAC */

#ifdef CONFIG_AML_TCP_KEEPALIVE_OFFLOAD_DEBUGFS
static ssize_t aml_dbgfs_tko_config_write(struct file *file,
                    const char __user *user_buf,
                    size_t count, loff_t *ppos)
{
    struct aml_hw *rw_hw = file->private_data;
    struct aml_vif *rw_vif;
    char buf[64];
    size_t len = min_t(size_t, count, sizeof(buf) - 1);
    int interval, retry_interval, retry_count;

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';

    if (sscanf(buf, "interval=%d retry_interval=%d retry_count=%d",
                &interval, &retry_interval, &retry_count) > 0) {
        list_for_each_entry(rw_vif, &rw_hw->vifs, list) {
            if (AML_VIF_TYPE(rw_vif) == NL80211_IFTYPE_STATION) {

                aml_tko_config_req(rw_hw, rw_vif, interval,
                        retry_interval, retry_count);
                break;
            }
        }
    }
    return count;
}
DEBUGFS_WRITE_FILE_OPS(tko_config);
#endif

void aml_dbgfs_fw_trace_create(struct aml_hw *aml_hw)
{
    static int is_init = 0;

    if (is_init) {
        return;
    }
    if (!aml_fw_trace_init(&aml_hw->debugfs.fw_trace,
                            aml_ipc_fw_trace_desc_get(aml_hw))) {
        DEBUGFS_ADD_FILE(fw_trace, aml_hw->debugfs.dir_diags, S_IWUSR | S_IRUSR);
        if (aml_hw->debugfs.fw_trace.buf.nb_compo)
            DEBUGFS_ADD_FILE(fw_trace_level, aml_hw->debugfs.dir_diags, S_IWUSR | S_IRUSR);
        is_init = 1;
    } else {
        aml_hw->debugfs.fw_trace.buf.data = NULL;
    }
err:
    printk("create trace dbgfs fail");
}

int aml_dbgfs_register(struct aml_hw *aml_hw, const char *name)
{
#ifdef CONFIG_AML_SOFTMAC
    struct dentry *phyd = aml_hw->hw->wiphy->debugfsdir;
#else
    struct dentry *phyd = aml_hw->wiphy->debugfsdir;
#endif /* CONFIG_AML_SOFTMAC */
    struct aml_debugfs *aml_debugfs = &aml_hw->debugfs;
    struct dentry *dir_drv, *dir_diags, *dir_stas;

    if (!(dir_drv = debugfs_create_dir(name, phyd)))
        return -ENOMEM;

    aml_debugfs->dir = dir_drv;

    if (!(dir_stas = debugfs_create_dir("stations", dir_drv)))
        return -ENOMEM;

    aml_debugfs->dir_stas = dir_stas;
    aml_debugfs->unregistering = false;

    if (!(dir_diags = debugfs_create_dir("diags", dir_drv)))
        goto err;

#ifdef CONFIG_AML_FULLMAC
    INIT_WORK(&aml_debugfs->sta_work, aml_sta_work);
    INIT_LIST_HEAD(&aml_debugfs->rc_config_save);
    aml_debugfs->sta_idx = AML_INVALID_STA;
#endif

    DEBUGFS_ADD_U32(tcp_pacing_shift, dir_drv, &aml_hw->tcp_pacing_shift,
                    S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(stats, dir_drv, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(sys_stats, dir_drv,  S_IRUSR);
#ifdef CONFIG_AML_SOFTMAC
    DEBUGFS_ADD_X64(rateidx, dir_drv, &aml_hw->debugfs.rateidx);
#endif
    DEBUGFS_ADD_FILE(txq, dir_drv, S_IRUSR);
    DEBUGFS_ADD_FILE(acsinfo, dir_drv, S_IRUSR);
#ifdef CONFIG_AML_MUMIMO_TX
    DEBUGFS_ADD_FILE(mu_group, dir_drv, S_IRUSR);
#endif

#ifdef CONFIG_AML_P2P_DEBUGFS
    {
        /* Create a p2p directory */
        struct dentry *dir_p2p;
        if (!(dir_p2p = debugfs_create_dir("p2p", dir_drv)))
            goto err;

        /* Add file allowing to control Opportunistic PS */
        DEBUGFS_ADD_FILE(oppps, dir_p2p, S_IRUSR);
        /* Add file allowing to control Notice of Absence */
        DEBUGFS_ADD_FILE(noa, dir_p2p, S_IRUSR);
    }
#endif /* CONFIG_AML_P2P_DEBUGFS */

    if (aml_dbgfs_register_fw_dump(aml_hw, dir_drv, dir_diags))
        goto err;
    DEBUGFS_ADD_FILE(fw_dbg, dir_diags, S_IWUSR | S_IRUSR);

    aml_debugfs->dir_diags = dir_diags;
    if (!aml_fw_trace_init(&aml_hw->debugfs.fw_trace,
                            aml_ipc_fw_trace_desc_get(aml_hw))) {
        DEBUGFS_ADD_FILE(fw_trace, dir_diags, S_IWUSR | S_IRUSR);
        if (aml_hw->debugfs.fw_trace.buf.nb_compo)
            DEBUGFS_ADD_FILE(fw_trace_level, dir_diags, S_IWUSR | S_IRUSR);
    } else {
        aml_debugfs->fw_trace.buf.data = NULL;
    }

#ifdef CONFIG_AML_RADAR
    {
        struct dentry *dir_radar, *dir_sec;
        if (!(dir_radar = debugfs_create_dir("radar", dir_drv)))
            goto err;

        DEBUGFS_ADD_FILE(pulses_prim, dir_radar, S_IRUSR);
        DEBUGFS_ADD_FILE(detected,    dir_radar, S_IRUSR);
        DEBUGFS_ADD_FILE(enable,      dir_radar, S_IRUSR);

        if (aml_hw->phy.cnt == 2) {
            DEBUGFS_ADD_FILE(pulses_sec, dir_radar, S_IRUSR);

            if (!(dir_sec = debugfs_create_dir("sec", dir_radar)))
                goto err;

            DEBUGFS_ADD_FILE(band,    dir_sec, S_IWUSR | S_IRUSR);
            DEBUGFS_ADD_FILE(type,    dir_sec, S_IWUSR | S_IRUSR);
            DEBUGFS_ADD_FILE(prim20,  dir_sec, S_IWUSR | S_IRUSR);
            DEBUGFS_ADD_FILE(center1, dir_sec, S_IWUSR | S_IRUSR);
            DEBUGFS_ADD_FILE(center2, dir_sec, S_IWUSR | S_IRUSR);
            DEBUGFS_ADD_FILE(set,     dir_sec, S_IWUSR | S_IRUSR);
        }
    }
#endif /* CONFIG_AML_RADAR */
    return 0;

err:
    aml_dbgfs_unregister(aml_hw);
    return -ENOMEM;
}

void aml_dbgfs_unregister(struct aml_hw *aml_hw)
{
    struct aml_debugfs *aml_debugfs = &aml_hw->debugfs;

#ifdef CONFIG_AML_FULLMAC
    struct aml_rc_config_save *cfg, *next;
    list_for_each_entry_safe(cfg, next, &aml_debugfs->rc_config_save, list) {
        list_del(&cfg->list);
        kfree(cfg);
    }
#endif /* CONFIG_AML_FULLMAC */

    if (!aml_hw->debugfs.dir)
        return;

    spin_lock_bh(&aml_debugfs->umh_lock);
    aml_debugfs->unregistering = true;
    spin_unlock_bh(&aml_debugfs->umh_lock);
    aml_wait_um_helper(aml_hw);
    aml_fw_trace_deinit(&aml_hw->debugfs.fw_trace);
#ifdef CONFIG_AML_FULLMAC
    flush_work(&aml_debugfs->sta_work);
#endif
    debugfs_remove_recursive(aml_hw->debugfs.dir);
    aml_hw->debugfs.dir = NULL;
}
#endif
