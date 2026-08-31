/*
****************************************************************************************
*
* @file aml_rate.c
*
* @brief Implementation of the aml sdio_usb rx rate
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#define AML_MODULE                  RATE

#include <linux/types.h>
#include <linux/sort.h>
#include <linux/bitmap.h>

#include "aml_defs.h"
#include "aml_rate.h"
#include "aml_msg_tx.h"

struct aml_dyn_snr;
static void aml_dynamic_snr_rate_stats(struct aml_dyn_snr *dyn_snr, struct hw_vect *hwvect);

#define MAX_BITRATES_CCK_LEN    4
#define MAX_BITRATES_OFDM_LEN   8
#define MAX_RU_SIZE_HE_ER_LEN   2
#define MAX_RU_SIZE_HE_MU_LEN   6
#define MAX_HE_GI_LEN           3

static const int bitrates_cck[MAX_BITRATES_CCK_LEN] = { 10, 20, 55, 110 };
static const int bitrates_ofdm[MAX_BITRATES_OFDM_LEN] = { 6, 9, 12, 18, 24, 36, 48, 54};
static const int ru_size_he_er[MAX_RU_SIZE_HE_ER_LEN] = { 242, 106 };
static const int ru_size_he_mu[MAX_RU_SIZE_HE_MU_LEN] = { 26, 52, 106, 242, 484, 996 };
static const char he_gi[MAX_HE_GI_LEN][4] = {"0.8", "1.6", "3.2"};

void idx_to_rate_cfg(int idx, union aml_rate_ctrl_info *r_cfg, int *ru_size)
{
    union aml_mcs_index *r = (union aml_mcs_index*) r_cfg;

    r_cfg->value = 0;
    if (idx < N_CCK) {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->giAndPreTypeTx = (idx & 1) << 1;
        r_cfg->mcsIndexTx = idx / 2;
    } else if (idx < (N_CCK + N_OFDM)) {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->mcsIndexTx = idx - N_CCK + 4;
    } else if (idx < (N_CCK + N_OFDM + N_HT)) {
        idx -= (N_CCK + N_OFDM);
        r_cfg->formatModTx = FORMATMOD_HT_MF;
        r->ht.nss = idx / (8 * 2 * 2);
        r->ht.mcs = (idx % (8 * 2 * 2)) / (2 * 2);
        r_cfg->bwTx = ((idx % (8 * 2 * 2)) % (2 * 2)) / 2;
        r_cfg->giAndPreTypeTx = idx & 1;
    } else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT)) {
        idx -= (N_CCK + N_OFDM + N_HT);
        r_cfg->formatModTx = FORMATMOD_VHT;
        r->vht.nss = idx / (10 * 4 * 2);
        r->vht.mcs = (idx % (10 * 4 * 2)) / (4 * 2);
        r_cfg->bwTx = ((idx % (10 * 4 * 2)) % (4 * 2)) / 2;
        r_cfg->giAndPreTypeTx = idx & 1;
    } else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU)) {
        idx -= (N_CCK + N_OFDM + N_HT + N_VHT);
        r_cfg->formatModTx = FORMATMOD_HE_SU;
        r->vht.nss = idx / (12 * 4 * 3);
        r->vht.mcs = (idx % (12 * 4 * 3)) / (4 * 3);
        r_cfg->bwTx = ((idx % (12 * 4 * 3)) % (4 * 3)) / 3;
        r_cfg->giAndPreTypeTx = idx % 3;
    } else if (idx < (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU)) {
        idx -= (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU);
        r_cfg->formatModTx = FORMATMOD_HE_MU;
        r->vht.nss = idx / (12 * 6 * 3);
        r->vht.mcs = (idx % (12 * 6 * 3)) / (6 * 3);
        if (ru_size)
            *ru_size = ((idx % (12 * 6 * 3)) % (6 * 3)) / 3;
        r_cfg->giAndPreTypeTx = idx % 3;
        r_cfg->bwTx = 0;
    } else {
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

static inline int rate_idx_of_rx_vector(struct rx_vector_1 *rxvect)
{
    int rate_idx = -1;
    int bw = rxvect->ch_bw;
    int mcs;
    int sgi;
    int nss;

    switch (rxvect->format_mod) {
    case FORMATMOD_NON_HT:
    case FORMATMOD_NON_HT_DUP_OFDM:
        {
            int idx = legrates_lut[rxvect->leg_rate & 0xf].idx;

            if (idx < 4)
                rate_idx = idx * 2 + rxvect->pre_type;
            else
                rate_idx = N_CCK + idx - 4;
        }
        break;
    case FORMATMOD_HT_MF:
    case FORMATMOD_HT_GF:
        mcs = rxvect->ht.mcs & 0x7;
        nss = rxvect->ht.mcs >> 3;
        sgi = rxvect->ht.short_gi;
        rate_idx = N_CCK + N_OFDM + nss * 32 + mcs * 4 +  bw * 2 + sgi;
        break;
    case FORMATMOD_VHT:
        mcs = rxvect->vht.mcs;
        nss = rxvect->vht.nss;
        sgi = rxvect->vht.short_gi;
        rate_idx = N_CCK + N_OFDM + N_HT + nss * 80 + mcs * 8 + bw * 2 + sgi;
        break;
    case FORMATMOD_HE_SU:
        mcs = rxvect->he.mcs;
        nss = rxvect->he.nss;
        sgi = rxvect->he.gi_type;
        rate_idx = N_CCK + N_OFDM + N_HT + N_VHT + nss * 144 + mcs * 12 + bw * 3 + sgi;
        break;
    case FORMATMOD_HE_MU:
        mcs = rxvect->he.mcs;
        nss = rxvect->he.nss;
        sgi = rxvect->he.gi_type;
        rate_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU
            + nss * 216 + mcs * 18 + rxvect->he.ru_size * 3 + sgi;
        break;
    case FORMATMOD_HE_ER:
    case FORMATMOD_HE_TB:
        mcs = rxvect->he.mcs;
        sgi = rxvect->he.gi_type;
        rate_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU
            + rxvect->he.ru_size * 9 + mcs * 3 + sgi;
        break;
    default:
        AML_ERR("RX: Invalid index conversion => %d\n", rxvect->format_mod);
        WARN_ON(1);
        break;
    }
    return rate_idx;
}

int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
               int sgi, int pre, int dcm, int *r_idx)
{
    int res = 0;

    if (format < FORMATMOD_HT_MF) {
        if ((mcs >= 0) && (mcs < MAX_BITRATES_CCK_LEN)) {
            if (r_idx) {
                *r_idx = (mcs * 2) + pre;
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
            res += scnprintf(&buf[res], size - res, "L-CCK/%cP%11c%2u.%1uM   ",
                             pre > 0 ? 'L' : 'S', ' ',
                             bitrates_cck[mcs] / 10,
                             bitrates_cck[mcs] % 10);
        } else {
            mcs -= 4;
            if (r_idx) {
                *r_idx = N_CCK + mcs;
                res = scnprintf(buf, size - res, "%4d ", *r_idx);
            }
            if ((mcs >= 0) && (mcs < MAX_BITRATES_OFDM_LEN)) {
                res += scnprintf(&buf[res], size - res, "L-OFDM%13c%2u.0M   ",
                                 ' ', bitrates_ofdm[mcs]);
            } else {
                AML_M_ERR(RATE, "FORMATMOD_HT_MF mcs:%d\n", mcs);
            }
        }
    } else if (format < FORMATMOD_VHT) {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + nss * 32 + mcs * 4 + bw * 2 + sgi;
            res = scnprintf(buf, size - res, "%4d ", *r_idx);
        }
        mcs += nss * 8;
        res += scnprintf(&buf[res], size - res, "HT%d/%cGI%11cMCS%-2d   ",
                         20 * (1 << bw), sgi ? 'S' : 'L', ' ', mcs);
    } else if (format == FORMATMOD_VHT){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + nss * 80 + mcs * 8 + bw * 2 + sgi;
            res = scnprintf(buf, size - res, "%4d ", *r_idx);
        }
        res += scnprintf(&buf[res], size - res, "VHT%d/%cGI%*cMCS%d/%1d  ",
                         20 * (1 << bw), sgi ? 'S' : 'L', bw > 2 ? 9 : 10, ' ',
                         mcs, nss + 1);
    } else if (format == FORMATMOD_HE_SU){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + nss * 144 + mcs * 12 + bw * 3 + sgi;
            res = scnprintf(buf, size - res, "%4d ", *r_idx);
        }
        if ((sgi >= 0) && (sgi < MAX_HE_GI_LEN)) {
            res += scnprintf(&buf[res], size - res, "HE%d/GI%s%4s%*cMCS%d/%1d%*c",
                             20 * (1 << bw), he_gi[sgi], dcm ? "/DCM" : "",
                             bw > 2 ? 4 : 5, ' ', mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        } else {
            AML_M_ERR(RATE, "FORMATMOD_HE_SU sgi:%d\n", sgi);
        }
    } else if (format == FORMATMOD_HE_MU){
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + nss * 216 + mcs * 18 + bw * 3 + sgi;
            res = scnprintf(buf, size - res, "%4d ", *r_idx);
        }
        if ((sgi >= 0) && (sgi < MAX_HE_GI_LEN) && (bw >= 0) && (bw < MAX_RU_SIZE_HE_MU_LEN)) {
            res += scnprintf(&buf[res], size - res, "HEMU-%d/GI%s%*cMCS%d/%1d%*c",
                             ru_size_he_mu[bw], he_gi[sgi], bw > 1 ? 5 : 6, ' ',
                             mcs, nss + 1, mcs > 9 ? 1 : 2, ' ');
        } else {
            AML_M_ERR(RATE, "FORMATMOD_HE_MU sgi:%d, bw:%d\n", sgi, bw);
        }
    }
    else // HE ER
    {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU + N_HE_MU + bw * 9 + mcs * 3 + sgi;
            res = scnprintf(buf, size - res, "%4d ", *r_idx);
        }
        if ((sgi >= 0) && (sgi < MAX_HE_GI_LEN) && (bw >= 0) && (bw < MAX_RU_SIZE_HE_ER_LEN)) {
            res += scnprintf(&buf[res], size - res, "HEER-%d/GI%s%4s%1cMCS%d/%1d%2c",
                             ru_size_he_er[bw], he_gi[sgi], dcm ? "/DCM" : "",
                             ' ', mcs, nss + 1, ' ');
        } else {
            AML_M_ERR(RATE, "FORMATMOD_HE_ER sgi:%d, bw:%d\n", sgi, bw);
        }
    }

    return res;
}

int print_rate_from_cfg(char *buf, int size, u32 rate_config, int *r_idx, int ru_size)
{
    union aml_rate_ctrl_info *r_cfg = (union aml_rate_ctrl_info *)&rate_config;
    union aml_mcs_index *mcs_index = (union aml_mcs_index *)&rate_config;
    unsigned int ft, pre, gi, bw, nss, mcs, dcm;

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
            bw = ru_size;
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

    return print_rate(buf, size, ft, nss, mcs, bw, gi, pre, dcm, r_idx);
}

char *print_sta_rate_stats(struct aml_hw *aml_hw, struct aml_sta *sta)
{
    static const char hist[] = "##################################################";
    const int hist_len = sizeof(hist) - 1;

    struct aml_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    int bufsz = (rate_stats->rate_cnt * ( 50 + hist_len) + 200);
    char *buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    int i;
    int len = 0;
    unsigned int fmt, pre, bw, nss, mcs, gi, dcm = 0;
    struct rx_vector_1 *last_rx;
    u8 nrx;

    if (buf == NULL)
        return NULL;

    // Get number of RX paths
    nrx = (aml_hw->version_cfm.version_phy_1 & MDM_NRX_MASK) >> MDM_NRX_LSB;

    len += scnprintf(buf, bufsz,
                     "\nRX rate info for %02X:%02X:%02X:%02X:%02X:%02X:\n",
                     sta->mac_addr[0], sta->mac_addr[1], sta->mac_addr[2],
                     sta->mac_addr[3], sta->mac_addr[4], sta->mac_addr[5]);

    // Display Statistics
    for (i = 0; i < rate_stats->size; i++)
    {
        if (rate_stats->table && rate_stats->table[i]) {
            union aml_rate_ctrl_info rate_config;
            u64 permillage = div_u64((u64)rate_stats->table[i] * 1000, rate_stats->cpt);
            int p;
            int ru_size = 0;
            u32 rem;
            u64 div;

            rate_config.value = 0;
            idx_to_rate_cfg(i, &rate_config, &ru_size);
            len += print_rate_from_cfg(&buf[len], bufsz - len,
                                       rate_config.value, NULL, ru_size);
            p = div_u64((permillage * hist_len), 1000);
            div = div_u64_rem(permillage, 10, &rem);
            len += scnprintf(&buf[len], bufsz - len, ": %9d(%2lld.%1d%%)%.*s\n",
                             rate_stats->table[i],
                             div, rem, p, hist);
        }
    }

    // Display detailed info of the last received rate
    last_rx = &sta->stats.last_rx.rx_vect1;
    len += scnprintf(&buf[len], bufsz - len,"\nLast received rate\n"
                     "type               rate     LDPC STBC BEAMFM DCM DOPPLER SIG-B %s\n",
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

    len += print_rate(&buf[len], bufsz - len, fmt, nss, mcs, bw, gi, pre, dcm, NULL);

    /* flags for HT/VHT/HE */
    if (fmt >= FORMATMOD_HE_SU) {
        len += scnprintf(&buf[len], bufsz - len, "  %c    %c     %c    %c     %c    %c",
                         last_rx->he.fec ? 'L' : ' ',
                         last_rx->he.stbc ? 'S' : ' ',
                         last_rx->he.beamformed ? 'B' : ' ',
                         last_rx->he.dcm ? 'D' : ' ',
                         last_rx->he.doppler ? 'D' : ' ',
                         last_rx->he.sig_b_comp_mode ? 'S' : ' ');
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
        scnprintf(&buf[len], bufsz - len, "       %-4d       %d\n",
                         last_rx->rssi1, last_rx->rssi1);
    } else {
        scnprintf(&buf[len], bufsz - len, "      %d\n", last_rx->rssi1);
    }

    return buf;
}

#define LINE_MAX_SZ 150

struct st {
    char line[LINE_MAX_SZ + 1];
    unsigned int r_idx;
};

static int compare_idx(const void *st1, const void *st2)
{
    int index1 = ((struct st *)st1)->r_idx;
    int index2 = ((struct st *)st2)->r_idx;

    if (index1 > index2) return 1;
    if (index1 < index2) return -1;

    return 0;
}

char *print_sta_rc_stats( struct aml_hw *aml_hw, struct aml_sta *sta)
{
    char *buf;
    int bufsz, len = 0;
    int i = 0;
    int error = 0;
    struct me_rc_stats_cfm me_rc_stats_cfm = {0};
    unsigned int no_samples;
    struct st *st;

     /* Forward the information to the LMAC */
    if ((error = aml_send_me_rc_stats(aml_hw, sta->sta_idx, &me_rc_stats_cfm)))
        return NULL;

    no_samples = me_rc_stats_cfm.no_samples;
    if (no_samples == 0)
        return 0;

    bufsz = no_samples * LINE_MAX_SZ + 500;

    buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    if (buf == NULL)
        return NULL;

    st = kmalloc(sizeof(struct st) * no_samples, GFP_ATOMIC);
    if (st == NULL) {
        kfree(buf);
        return NULL;
    }

    for (i = 0; i < no_samples; i++) {
        unsigned int tp, eprob;
        len = print_rate_from_cfg(st[i].line, LINE_MAX_SZ,
                                  me_rc_stats_cfm.rate_stats[i].rate_config,
                                  (int *)&st[i].r_idx, 0);

        if (me_rc_stats_cfm.sw_retry_step != 0) {
            len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len,  "%c",
                    me_rc_stats_cfm.retry_step_idx[me_rc_stats_cfm.sw_retry_step] == i ? '*' : ' ');
        }
        else {
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
    for (i = 0; i < no_samples; i++) {
        len += scnprintf(&buf[len], bufsz - len, "%s\n", st[i].line);
    }
    kfree(st);

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
                                   NULL, ru_index < MAX_RU_SIZE_HE_MU_LEN ? ru_index : 0);

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
    /* coverity[assigned_value] - len is used */
    len += scnprintf(&buf[len], bufsz - len, "\n%6u %6u %3d.%1d %6u\n",
                     me_rc_stats_cfm.ampdu_len,
                     me_rc_stats_cfm.ampdu_packets,
                     me_rc_stats_cfm.avg_ampdu_len >> 16,
                     ((me_rc_stats_cfm.avg_ampdu_len * 10) >> 16) % 10,
                     me_rc_stats_cfm.sample_wait);

#if 0
    len += scnprintf(&buf[len], bufsz - len, "\n rate upper: ");
    len += print_rate_from_cfg(&buf[len], bufsz - len, me_rc_stats_cfm.upper_rate_cfg,
                               NULL, 0);
    len += scnprintf(&buf[len], bufsz - len, "\n rate lower: ");
    len += print_rate_from_cfg(&buf[len], bufsz - len, me_rc_stats_cfm.lower_rate_cfg,
                               NULL, 0);
#endif

    return buf;
}

static int aml_hw_rate_num(struct aml_hw *aml_hw)
{
    int nb_rx_rate = N_CCK + N_OFDM;

    if (aml_hw->mod_params->ht_on)
        nb_rx_rate += N_HT;

    if (aml_hw->mod_params->vht_on)
        nb_rx_rate += N_VHT;

    if (aml_hw->mod_params->he_on)
        nb_rx_rate += N_HE_SU + N_HE_MU + N_HE_ER;

    return nb_rx_rate;
}

static int aml_rx_rate_stats_init(struct aml_hw *aml_hw, struct aml_rx_rate_stats *rate_stats)
{
    int nb_rx_rate = aml_hw_rate_num(aml_hw);

    rate_stats->size = 0;
    rate_stats->cpt = 0;
    rate_stats->rate_cnt = 0;
    rate_stats->table = kzalloc(nb_rx_rate * sizeof(rate_stats->table[0]),
                                in_atomic() ? GFP_ATOMIC : GFP_KERNEL);
    if (!rate_stats->table)
        return -1;
    rate_stats->size = nb_rx_rate;
    return 0;
}

static int aml_dynamic_snr_sta_active(struct aml_hw *aml_hw, struct aml_sta *sta);

int aml_sta_rate_table_init(struct aml_hw *aml_hw, struct aml_sta *sta)
{
    aml_dynamic_snr_sta_active(aml_hw, sta);
    return aml_rx_rate_stats_init(aml_hw, &sta->stats.rx_rate);
}

static void aml_rx_rate_stats_deinit(struct aml_rx_rate_stats *rate_stats)
{
    if (rate_stats->table) {
        kfree(rate_stats->table);
        rate_stats->table = NULL;
        rate_stats->size = 0;
    }
}

void aml_sta_rate_table_deinit(struct aml_hw *aml_hw, struct aml_sta *sta)
{
    aml_rx_rate_stats_deinit(&sta->stats.rx_rate);
}

/**
 * aml_rx_statistic - save some statistics about received frames
 *
 * @aml_hw: main driver data.
 * @hwvect: Rx Hardware vector of the received frame.
 */
void aml_rx_statistic(struct aml_hw *aml_hw, struct hw_vect *hwvect)
{
    struct aml_stats *stats = aml_hw->stats;
    unsigned int mpdu = hwvect->mpdu_cnt;
    unsigned int ampdu = hwvect->ampdu_cnt;
    unsigned int mpdu_prev;

    if (ampdu >= AMPDUS_RX_MAP_NUM)
        return;

    mpdu_prev = stats->ampdus_rx_map[ampdu];

    /*
     * update ampdu rx stats
     */
    if (mpdu_prev > IEEE80211_MAX_AMPDU_BUF)
        mpdu_prev = mpdu;

    /* work-around, for MACHW that incorrectly return 63 for last MPDU of A-MPDU or S-MPDU */
    if (mpdu == 63) {
        if (ampdu == stats->ampdus_rx_last)
            mpdu = mpdu_prev + 1;
        else
            mpdu = 0;
    }

    if (mpdu_prev >= IEEE80211_MAX_AMPDU_BUF)
        return;

    if (ampdu != stats->ampdus_rx_last) {
        stats->ampdus_rx[mpdu_prev]++;
        stats->ampdus_rx_miss += mpdu;
    } else {
        if (mpdu <= mpdu_prev) {
            /* lost 4 (or a multiple of 4) complete A-MPDU/S-MPDU */
            stats->ampdus_rx_miss += mpdu;
        } else {
            stats->ampdus_rx_miss += mpdu - mpdu_prev - 1;
        }
    }

    stats->ampdus_rx_map[ampdu] = mpdu;
    stats->ampdus_rx_last = ampdu;

    if (aml_hw->dyn_snr)
        aml_dynamic_snr_rate_stats((struct aml_dyn_snr *)aml_hw->dyn_snr, hwvect);
}

void aml_rx_sta_stats(struct aml_hw *aml_hw, struct aml_sta *sta, struct hw_vect *hwvect)
{
    struct aml_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    int rate_idx;

    /*
     * update sta statistic
     */
    sta->stats.last_rx = *hwvect;   /* save complete rx vector */
    sta->stats.rx_pkts ++;
    sta->stats.rx_bytes += hwvect->len;
    sta->stats.last_act = jiffies;

    /* FIXME: it seems that per sta rate_stats is useless except dump. */
    if (!rate_stats->size || !rate_stats->table)
        return;

    rate_idx = rate_idx_of_rx_vector(&hwvect->rx_vect1);
    if (rate_idx < 0 || rate_idx >= rate_stats->size) {
        AML_ERR("RX: Invalid index conversion => %d/%d\n", rate_idx, rate_stats->size);
        return;
    }

    if (!rate_stats->table[rate_idx])
        rate_stats->rate_cnt++;
    rate_stats->table[rate_idx]++;
    rate_stats->cpt++;
}

/*
 * Dynamic SNR functions
 */
#define SNR_CFG_REG     (0x60c00828 - AML_BASE_ADDR)
#define SNR_CFG_MASK    0x3U
#define SNR_CFG_SHIFT   29

#define SNR_TRIAL_MAX   3

struct aml_dyn_snr_stats {
    u64 bytes;
    u32 pkts;
    int pkts_per_hw_rate[AML_HW_RATE_HE_SU_NUM];
};

struct aml_dyn_snr {
    struct aml_hw *aml_hw;

    spinlock_t lock;
    struct work_struct work;
    struct workqueue_struct *workqueue;
    unsigned long last_time;

    enum nl80211_band band;

    struct {
        bool enable;
        u16 permillage;  /* permillage */
    } cfg;

    bool need_trial;
    u8 cur_snr_cfg;
    u8 trial_cnt;
    u8 snr_cfg[SNR_TRIAL_MAX];
    unsigned int rx_tp[SNR_TRIAL_MAX];

    u16 rate_num;
    DECLARE_BITMAP(hi_rate_bmp, AML_HW_RATE_HE_SU_NUM);

    struct aml_dyn_snr_stats stats;
    struct aml_dyn_snr_stats cache; /* cache of recent stats */
};

static void aml_dynamic_snr_set(struct aml_hw *aml_hw, u8 snr_cfg)
{
    u32 value;

#ifdef CONFIG_AML_RECOVERY
    if (aml_recy != NULL && aml_recy_flags_chk(AML_RECY_STATE_ONGOING))
        return;
#endif

    value = AML_REG_READ(aml_hw->plat, AML_ADDR_SYSTEM, SNR_CFG_REG);

    AML_DBG("SNR config: %d ==> %d (%x)\n",
            (value >> SNR_CFG_SHIFT) & SNR_CFG_MASK, snr_cfg, value);

    value &= ~(SNR_CFG_MASK << SNR_CFG_SHIFT);
    value |= ((u32)(snr_cfg & SNR_CFG_MASK)) << SNR_CFG_SHIFT;
    AML_REG_WRITE(value, aml_hw->plat, AML_ADDR_SYSTEM, SNR_CFG_REG);
}

static void aml_hi_rate_bmp_flush(unsigned long *bmp, enum nl80211_band band)
{
    int i;

    for (i = 0; i < AML_HW_RATE_HE_SU_NUM; i++) {
        union aml_rate_ctrl_info rate_config;
        union aml_mcs_index *r = (union aml_mcs_index *)&rate_config;

        idx_to_rate_cfg(i, &rate_config, NULL);

        switch (rate_config.formatModTx) {
        case FORMATMOD_NON_HT:
        case FORMATMOD_NON_HT_DUP_OFDM:
            clear_bit(i, bmp);
            break;
        case FORMATMOD_HT_MF:
            if ((r->ht.nss >= 1) && (r->ht.mcs >= 6))
                set_bit(i, bmp);
            else
                clear_bit(i, bmp);
            break;
        case FORMATMOD_VHT:
            if ((r->vht.nss >= 1) && (r->vht.mcs > 8))
                set_bit(i, bmp);
            else
                clear_bit(i, bmp);
            break;
        case FORMATMOD_HE_SU:
        case FORMATMOD_HE_MU:
            if ((r->he.nss >= 1) && (r->he.mcs > 10))
                set_bit(i, bmp);
            else if (band == NL80211_BAND_2GHZ && (r->he.nss >= 1) && (r->he.mcs > 8))
                /* W2L 2.4G doesn't support MCS10/11, for convenient, relax W2 too */
                set_bit(i, bmp);
            else
                clear_bit(i, bmp);
            break;
        default:
            clear_bit(i, bmp);
            break;
        }
    }
}

static void aml_dynamic_snr_rate_stats(struct aml_dyn_snr *dyn_snr, struct hw_vect *hwvect)
{
    int rate_idx = rate_idx_of_rx_vector(&hwvect->rx_vect1);

    if (rate_idx < 0)
        return;

    spin_lock(&dyn_snr->lock);
    dyn_snr->stats.bytes += hwvect->len;
    dyn_snr->stats.pkts++;
    /* coverity[OVERRUN] , protect by check rate_idx < dyn_snr->rate_num*/
    if (rate_idx < dyn_snr->rate_num)
        dyn_snr->stats.pkts_per_hw_rate[rate_idx]++;
    spin_unlock(&dyn_snr->lock);

    if (rate_idx >= dyn_snr->rate_num)
        AML_RLMT_WARN("hw rate index %d is out of range %d!\n", rate_idx, dyn_snr->rate_num);
}

static inline void aml_dynamic_snr_probe(struct aml_dyn_snr *dyn_snr, struct aml_hw *aml_hw)
{
    int i;
    int hi_rate_pkts = 0;
    u32 high_rate_permillage = 0;
    unsigned int mbps;
    u64 bytes = 0;

    /* save recent stats and clear it */
    spin_lock_bh(&dyn_snr->lock);
    dyn_snr->cache = dyn_snr->stats;
    dyn_snr->stats = (struct aml_dyn_snr_stats ) { };

    /* calculate throughput and save it */
    bytes = dyn_snr->cache.bytes;
    mbps = div_u64((dyn_snr->cache.bytes >> 17) * HZ, (jiffies - dyn_snr->last_time));
    spin_unlock_bh(&dyn_snr->lock);

    dyn_snr->rx_tp[dyn_snr->trial_cnt] = mbps;

    AML_DBG("snr_cfg[%d] = %d: %d Mbps = %lld bytes / %ld jiffies\n",
            dyn_snr->trial_cnt, dyn_snr->cur_snr_cfg,
            mbps, bytes, jiffies - dyn_snr->last_time);
    dyn_snr->last_time = jiffies;

    /* do nothing if rx throughput is pretty low */
    if (mbps < 30) {
        dyn_snr->need_trial = false;
        dyn_snr->trial_cnt = 0;
        return;
    }

    /* calculate permillage of high rate */
    for (i = 0; i < dyn_snr->rate_num; i++) {
        if (dyn_snr->cache.pkts_per_hw_rate[i] && test_bit(i, dyn_snr->hi_rate_bmp))
            hi_rate_pkts += dyn_snr->cache.pkts_per_hw_rate[i];
    }
    if (dyn_snr->cache.pkts)
        high_rate_permillage = hi_rate_pkts * 1000 / dyn_snr->cache.pkts;
    AML_DBG("packets %d/%d = %d permillage\n",
            hi_rate_pkts, dyn_snr->cache.pkts, high_rate_permillage);

    if (high_rate_permillage >= dyn_snr->cfg.permillage) {
       // keep current configuration
        dyn_snr->need_trial = false;
        dyn_snr->trial_cnt = 0;
        dyn_snr->rx_tp[0] = mbps;
        dyn_snr->snr_cfg[0] = dyn_snr->cur_snr_cfg;
    } else {
        u8 trial_cnt = dyn_snr->trial_cnt;
        u32 snr_cfg = dyn_snr->snr_cfg[0];

        switch (trial_cnt) {
        case 0:
            dyn_snr->need_trial = true;    /* fast / short duration */

            /* try 2 more times: last_snr_cfg + 1, last_snr_cfg - 1 */
            snr_cfg = (snr_cfg + 1) & SNR_CFG_MASK;
            trial_cnt = 1;
            break;
        case 1:
            snr_cfg = (snr_cfg - 1) & SNR_CFG_MASK;
            trial_cnt = 2;
            break;
        case 2: {
            unsigned int best_tp = 0;

            // find the best and save it into rank 0
            for (i = 0; i < SNR_TRIAL_MAX; i++) {
                if (dyn_snr->rx_tp[i] > best_tp) {
                    best_tp = dyn_snr->rx_tp[i];
                    snr_cfg = dyn_snr->snr_cfg[i];
                }
            }
            dyn_snr->need_trial = false;
            trial_cnt = 0;
            break;
            }
        default:
            AML_ERR("wrong trial_cnt %d\n", trial_cnt);
            BUG_ON(trial_cnt >= SNR_TRIAL_MAX);
            break;
        }
        dyn_snr->trial_cnt = trial_cnt;
        dyn_snr->snr_cfg[trial_cnt] = snr_cfg;

        if (dyn_snr->cur_snr_cfg != snr_cfg) {
            aml_dynamic_snr_set(aml_hw, snr_cfg);
            dyn_snr->cur_snr_cfg = snr_cfg;
        }
    }
}

static void aml_dynamic_snr_work(struct work_struct *work)
{
    struct aml_dyn_snr *dyn_snr = container_of(work, struct aml_dyn_snr, work);
    struct aml_hw *aml_hw;

    while ((aml_hw = dyn_snr->aml_hw)) {
        if (!dyn_snr->cfg.enable || dyn_snr->need_trial) {
            msleep(100);
        } else {
            msleep(3000);
        }
        if (dyn_snr->cfg.enable && aml_hw->vif_started)
            aml_dynamic_snr_probe(dyn_snr, aml_hw);
    }
}

int aml_dynamic_snr_config(struct aml_hw *aml_hw, int enable, int snr_cfg_or_mcs_ration)
{
    struct aml_dyn_snr *dyn_snr = aml_hw->dyn_snr;

    if (!dyn_snr)
        return 0;

    if ((dyn_snr->cfg.enable = enable)) {
        dyn_snr->cfg.permillage = snr_cfg_or_mcs_ration * 10;
        dyn_snr->need_trial = true;
        dyn_snr->cur_snr_cfg = 0;
    } else {
        dyn_snr->cur_snr_cfg = snr_cfg_or_mcs_ration & SNR_CFG_MASK;
    }
    aml_dynamic_snr_set(aml_hw, dyn_snr->cur_snr_cfg);

    return 0;
}

static int aml_dynamic_snr_sta_active(struct aml_hw *aml_hw, struct aml_sta *sta)
{
    struct aml_dyn_snr *dyn_snr = aml_hw->dyn_snr;

    if (!dyn_snr || dyn_snr->band == sta->band)
        return 0;

    dyn_snr->band = sta->band;
    aml_hi_rate_bmp_flush(dyn_snr->hi_rate_bmp, dyn_snr->band);
    return 0;
}

int aml_dynamic_snr_init(struct aml_hw *aml_hw)
{
    struct aml_dyn_snr *dyn_snr = aml_hw->dyn_snr;

    BUG_ON(dyn_snr);

    dyn_snr = kzalloc(sizeof(struct aml_dyn_snr), GFP_KERNEL);
    if (!dyn_snr) {
        AML_ERR("alloc %d bytes memory failed!\n", (int)sizeof(struct aml_dyn_snr));
        return -ENOMEM;
    }

    aml_hw->dyn_snr = dyn_snr;

    dyn_snr->aml_hw = aml_hw;
    dyn_snr->last_time = jiffies;
    /* coverity[USELESS_CALL], spinlock_check(_lock); */
    spin_lock_init(&dyn_snr->lock);
    INIT_WORK(&dyn_snr->work, aml_dynamic_snr_work);
    dyn_snr->workqueue = alloc_workqueue("%s.dyn_snr_cfg",
            WQ_UNBOUND | WQ_HIGHPRI | WQ_MEM_RECLAIM, 1,
            wiphy_name(aml_hw->wiphy));
    if (!dyn_snr->workqueue) {
        AML_ERR("alloc_workqueue \"%s.dyn_snr_cfg\" failed\n", wiphy_name(aml_hw->wiphy));
        return -1;
    }

    dyn_snr->band = NL80211_BAND_2GHZ;
    dyn_snr->rate_num = aml_hw_rate_num(aml_hw);
    if (dyn_snr->rate_num > AML_HW_RATE_HE_SU_NUM)
        dyn_snr->rate_num = AML_HW_RATE_HE_SU_NUM;
    AML_DBG("rate num: %d\n", dyn_snr->rate_num);
    aml_hi_rate_bmp_flush(dyn_snr->hi_rate_bmp, dyn_snr->band);

    aml_dynamic_snr_config(aml_hw, 1, 60);

    queue_work(dyn_snr->workqueue, &dyn_snr->work);

    return 0;
}

void aml_dynamic_snr_deinit(struct aml_hw *aml_hw)
{
    if (aml_hw->dyn_snr) {
        struct aml_dyn_snr *dyn_snr = aml_hw->dyn_snr;

        dyn_snr->aml_hw = NULL;
        cancel_work_sync(&dyn_snr->work);
        destroy_workqueue(dyn_snr->workqueue);

        kfree(aml_hw->dyn_snr);
        aml_hw->dyn_snr = NULL;
    }
}
