/**
 ****************************************************************************************
 *
 * @file aml_cfg.c
 *
 * Copyright (C) Amlogic 2012-2023
 *
 ****************************************************************************************
 */
#include <linux/firmware.h>
#include <linux/if_ether.h>

#include "aml_defs.h"
#include "aml_cfg.h"
#include "aml_msg_tx.h"
#include "aml_utils.h"
#include "aml_compat.h"

/* for android wifi_get_mac() */
#if defined(CONFIG_AML_PLATFORM_ANDROID)
#include "linux/amlogic/wifi_dt.h"
extern u8 *wifi_get_mac(void);
#endif

#if defined(CONFIG_AML_PLATFORM_RANDOM_MAC)
extern u8 *wifi_get_mac_random(void);
#endif

static struct file *aml_cfg_open(const char *path, int flag, int mode)
{
    struct file *fp;
    fp = FILE_OPEN(path, flag, mode);
    if (IS_ERR(fp)) {
        return NULL;
    }
    return fp;
}

static void aml_cfg_close(struct file *fp)
{
    FILE_CLOSE(fp, NULL);
}

static int aml_cfg_read(struct file *fp, char *buf, int len)
{
    int rlen = 0, sum = 0;

    while (sum < len) {
        rlen = FILE_READ(fp, buf + sum, len - sum, &fp->f_pos);
        if (rlen > 0) {
            sum += rlen;
        } else if (0 != rlen) {
            return rlen;
        } else {
            break;
        }
    }
    return sum;
}

static int aml_cfg_write(struct file *fp, char *buf, int len)
{
    int wlen = 0, sum = 0;

    while (sum < len) {
        wlen = FILE_WRITE(fp, buf + sum, len - sum, &fp->f_pos);
        if (wlen > 0)
            sum += wlen;
        else if (0 != wlen)
            return wlen;
        else
            break;
    }
    return sum;
}

static int aml_cfg_retrieve(const char *path, u8 *buf, u32 len)
{
    struct file *fp;
    int ret = -1;

    if (path && buf) {
        fp = aml_cfg_open(path, O_RDONLY, 0);
        if (fp) {
            ret = aml_cfg_read(fp, buf, len);
            aml_cfg_close(fp);
        }
    } else {
        ret = -EINVAL;
    }
    return (ret >= 0 ? ret : 0);
}

static int aml_cfg_create(const char *path, struct file **fpp)
{
    struct file *fp;

    fp = aml_cfg_open(path, O_RDONLY, 0);
    if (!fp) {
        fp = aml_cfg_open(path, O_CREAT | O_RDWR, 0666);
        if (!fp) {
            return AML_CFG_ERROR;
        }
        *fpp = fp;
        return AML_CFG_CREATE;
    }
    *fpp = fp;
    return AML_CFG_EXIST;
}

static const u8 *aml_cfg_find_tag(const u8 *file_data, unsigned int file_size,
        const char *tag_name, unsigned int tag_len)
{
    unsigned int curr, line_start = 0, line_size;

    while (line_start < file_size) {
        for (curr = line_start; curr < file_size; curr++)
            if (file_data[curr] == '\n')
                break;

        line_size = curr - line_start;
        if ((line_size == (strlen(tag_name) + tag_len)) &&
            (!strncmp(&file_data[line_start], tag_name, strlen(tag_name)))) {
            return (&file_data[line_start + strlen(tag_name)]);
        }

        line_start = curr + 1;
    }

    return NULL;
}

static void aml_cfg_store_tag(struct file *fp, const char *tag_name, const char *tag_value)
{
    unsigned char lbuf[AML_CFG_LBUF_MAXLEN];
    unsigned char fbuf[AML_CFG_FBUF_MAXLEN];

    if (!fp || !tag_name || !tag_value)
        return;

    if (strlen(tag_name) + strlen(tag_value) > AML_CFG_LBUF_MAXLEN) {
        AML_INFO("store tag name exceed max line len(%d)", AML_CFG_LBUF_MAXLEN);
        return;
    }

    sprintf(lbuf, "%s=%s\n", tag_name, tag_value);
    AML_INFO("store %s=%s(len=%d) tag to file\n", tag_name, tag_value, strlen(lbuf));

    aml_cfg_read(fp, fbuf, AML_CFG_FBUF_MAXLEN);
    if (strlen(fbuf) + strlen(lbuf) > AML_CFG_FBUF_MAXLEN) {
        AML_INFO("store tag name exceed max file len(%d)", AML_CFG_FBUF_MAXLEN);
        goto out;
    }
    sprintf(fbuf, "%s", lbuf);
    aml_cfg_write(fp, fbuf, strlen(fbuf));

out:
    return;
}

static void aml_cfg_get_chipid(struct aml_hw *aml_hw, struct aml_cfg *cfg)
{
    cfg->chipid_l = aml_efuse_read(aml_hw, AML_EFUSE_CHIPID_LOW);
    cfg->chipid_h = aml_efuse_read(aml_hw, AML_EFUSE_CHIPID_HIGH);

    AML_INFO("get_chipid from efuse:%04x%08x",
            cfg->chipid_h & 0xffff, cfg->chipid_l);
}

static int aml_cfg_get_macaddr(struct aml_hw *aml_hw, u8 *mac_addr)
{
    unsigned int efuse_data_l = 0;
    unsigned int efuse_data_h = 0;
    unsigned int mac_valid = 0;

    if (!mac_addr)
        return -1;

    mac_valid = aml_efuse_read(aml_hw, AML_EFUSE_MACADDR_VALID);
    mac_valid = (mac_valid >> AML_EFUSE_MACADDR_OFFSET) & 0x1;
    if (mac_valid == 1) {
        efuse_data_l = aml_efuse_read(aml_hw, AML_EFUSE_MACADDR_LOW1);
        efuse_data_h = aml_efuse_read(aml_hw, AML_EFUSE_MACADDR_HIGH1);
    } else {
        efuse_data_l = aml_efuse_read(aml_hw, AML_EFUSE_MACADDR_LOW);
        efuse_data_h = aml_efuse_read(aml_hw, AML_EFUSE_MACADDR_HIGH);
    }

    if (efuse_data_l == 0 || efuse_data_h == 0) {
        AML_INFO("get mac address from efuse failed, use random mac");
        return -1;
    }

    mac_addr[0] = (efuse_data_h & 0xff00) >> 8;
    mac_addr[1] = efuse_data_h & 0x00ff;
    mac_addr[2] = (efuse_data_l & 0xff000000) >> 24;
    mac_addr[3] = (efuse_data_l & 0x00ff0000) >> 16;
    mac_addr[4] = (efuse_data_l & 0xff00) >> 8;
    mac_addr[5] = efuse_data_l & 0xff;

    return 0;
}

static void aml_cfg_store_chipid(struct file *fp, struct aml_cfg *cfg)
{
    char chipid_str[AML_EFUSE_CHIPID_LEN + 1];

    sprintf(chipid_str, "%04x%08x", cfg->chipid_h & 0xffff, cfg->chipid_l);
    aml_cfg_store_tag(fp, "CHIP_ID", chipid_str);
}

static void aml_cfg_store_macaddr(struct file *fp, struct aml_cfg *cfg)
{
    char mac_str[strlen("00:00:00:00:00:00") + 1];

    sprintf(mac_str, MACFMT, MACARG(cfg->vif0_mac));
    aml_cfg_store_tag(fp, "VIF0_MACADDR", mac_str);

    sprintf(mac_str, MACFMT, MACARG(cfg->vif1_mac));
    aml_cfg_store_tag(fp, "VIF1_MACADDR", mac_str);

    sprintf(mac_str, MACFMT, MACARG(cfg->vif2_mac));
    aml_cfg_store_tag(fp, "VIF2_MACADDR", mac_str);
}

static void aml_cfg_check_macaddr(u8 *mac_addr, bool use_aml_oui)
{
    if (use_aml_oui) {
        mac_addr[0] = 0x1c;
        mac_addr[1] = 0xa4;
        mac_addr[2] = 0x10;
    }
    /* unicast and globally unique */
    mac_addr[0] &= 0xfc;
}

static int aml_cfg_to_file(struct aml_hw *aml_hw, struct aml_cfg *cfg, struct file *fp)
{
    u8 vif0_mac[ETH_ALEN] = { 0x1c, 0xa4, 0x10, 0x11, 0x22, 0x33 };
    u8 vif1_mac[ETH_ALEN] = { 0x1e, 0xa4, 0x10, 0x11, 0x22, 0x33 };
    u8 vif2_mac[ETH_ALEN] = { 0x1e, 0xa4, 0x10, 0x11, 0x22, 0x32 };
    u8 bcst_mac[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    int ret = -1;

    if (!aml_hw || !cfg)
        return -1;

    /* get chip id from efuse */
    aml_cfg_get_chipid(aml_hw, cfg);

    /* mac address rule:
     * - wifi_conf.txt only for efuse/local
     * - priority:
     *      efuse > local random
     * - use aml oui:
     *      efuse (no), local (yes)
     * */

    do {
#ifdef CONFIG_AML_PLATFORM_ANDROID
        /* get mac address from android (emmc) */
        memcpy(vif0_mac, wifi_get_mac(), ETH_ALEN);
        if (!fp && memcmp(vif0_mac, bcst_mac, ETH_ALEN) != 0) {
            aml_cfg_check_macaddr(vif0_mac, 0);
            AML_INFO("get mac address from emmc is:%pM", vif0_mac);
            break;
        }
#endif
        /* get mac address from efuse */
        ret = aml_cfg_get_macaddr(aml_hw, vif0_mac);
        if (ret == 0) {
            aml_cfg_check_macaddr(vif0_mac, 0);
            AML_INFO("get mac address from efuse is:%pM", vif0_mac);
            break;
        }
#ifdef CONFIG_AML_PLATFORM_RANDOM_MAC
        /* get mac address from android (random) */
        memcpy(vif0_mac, wifi_get_mac_random(), ETH_ALEN);
        if (!fp && memcmp(vif0_mac, bcst_mac, ETH_ALEN) != 0) {
            aml_cfg_check_macaddr(vif0_mac, 0);
            AML_INFO("get mac address from platform random is:%pM", vif0_mac);
            break;
        }
#endif
        else {
            get_random_bytes(vif0_mac, ETH_ALEN);
            aml_cfg_check_macaddr(vif0_mac, 1);
            AML_INFO("get mac address from local is:%pM", vif0_mac);
        }
    } while(0);

    /* locally administered for vif1_mac */
    memcpy(vif1_mac, vif0_mac, ETH_ALEN);
    vif1_mac[0] |= BIT(1);
    memcpy(vif2_mac, vif1_mac, ETH_ALEN);
    vif2_mac[5] ^= BIT(0);

    /* update mac address to cfg->vifx_mac */
    memcpy(cfg->vif0_mac, vif0_mac, ETH_ALEN);
    memcpy(cfg->vif1_mac, vif1_mac, ETH_ALEN);
    memcpy(cfg->vif2_mac, vif2_mac, ETH_ALEN);
    AML_INFO("vif0 mac address:%pM, vif1 mac address: %pM, vif2 mac address: %pM\n",
            cfg->vif0_mac, cfg->vif1_mac, cfg->vif2_mac);

    if (fp) {
        aml_cfg_store_chipid(fp, cfg);
        aml_cfg_store_macaddr(fp, cfg);
    }

    return 0;
}

static int aml_cfg_from_file(struct aml_cfg *cfg, struct file *fp)
{
    u8 fbuf[AML_CFG_FBUF_MAXLEN] = {0};
    const u8 *tag_ptr = NULL;
    u8 bcst_mac[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    u8 chipid_h[5];
    int ret = -1;

    if (!cfg || !fp)
        return -1;

    ret = aml_cfg_read(fp, fbuf, AML_CFG_FBUF_MAXLEN);
    if (ret >= AML_CFG_FBUF_MAXLEN) {
        AML_INFO("read file data error");
        return -1;
    }

    /* get chip id from file */
    tag_ptr = aml_cfg_find_tag(fbuf, strlen(fbuf),
            "CHIP_ID=", AML_EFUSE_CHIPID_LEN);
    if (tag_ptr) {
        cfg->chipid_l = simple_strtoul(tag_ptr + 4, NULL, 16);
        memcpy(chipid_h, tag_ptr, 4);
        chipid_h[4] = '\0';
        cfg->chipid_h = simple_strtoul(chipid_h, NULL, 16);
        AML_INFO("get chipid:%04x%08x", cfg->chipid_h & 0xffff, cfg->chipid_l);
    }

    /* mac address rule:
     * if wifi_conf.txt exist
     * - get mac from android(emmc) every time
     * - get mac from wifi_conf.txt (avoid efuse read fail)
     * */
#ifdef CONFIG_AML_PLATFORM_ANDROID
    /* get mac address from amlogic android(emmc) */
    memcpy(cfg->vif0_mac, wifi_get_mac(), ETH_ALEN);
    if (memcmp(cfg->vif0_mac, bcst_mac, ETH_ALEN) != 0)
    {
        aml_cfg_check_macaddr(cfg->vif0_mac, 0);
        /* locally administered for vif1_mac and vif2_mac */
        memcpy(cfg->vif1_mac, cfg->vif0_mac, ETH_ALEN);
        cfg->vif1_mac[0] |= BIT(1);
        memcpy(cfg->vif2_mac, cfg->vif1_mac, ETH_ALEN);
        cfg->vif2_mac[5] ^= BIT(0);
        AML_INFO("vif0 mac address:%pM, vif1 mac address: %pM, vif2 mac address: %pM\n",
                cfg->vif0_mac, cfg->vif1_mac, cfg->vif2_mac);
    }
    else
#endif
    {
        /* get vif0 mac address from file */
        tag_ptr = aml_cfg_find_tag(fbuf, strlen(fbuf),
                "VIF0_MACADDR=", strlen("00:00:00:00:00:00"));
        if (tag_ptr) {
            sscanf((const char *)tag_ptr,
                    "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    cfg->vif0_mac + 0, cfg->vif0_mac + 1,
                    cfg->vif0_mac + 2, cfg->vif0_mac + 3,
                    cfg->vif0_mac + 4, cfg->vif0_mac + 5);
            AML_INFO("get vif0 mac:"MACFMT, MACARG(cfg->vif0_mac));
        }

        /* get vif1 mac address from file */
        tag_ptr = aml_cfg_find_tag(fbuf, strlen(fbuf),
                "VIF1_MACADDR=", strlen("00:00:00:00:00:00"));
        if (tag_ptr) {
            sscanf((const char *)tag_ptr,
                    "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    cfg->vif1_mac + 0, cfg->vif1_mac + 1,
                    cfg->vif1_mac + 2, cfg->vif1_mac + 3,
                    cfg->vif1_mac + 4, cfg->vif1_mac + 5);
            AML_INFO("get vif1 mac:"MACFMT, MACARG(cfg->vif1_mac));
        }

        /* get vif2 mac address from file */
        tag_ptr = aml_cfg_find_tag(fbuf, strlen(fbuf),
                "VIF2_MACADDR=", strlen("00:00:00:00:00:00"));
        if (tag_ptr) {
            sscanf((const char *)tag_ptr,
                    "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                    cfg->vif2_mac + 0, cfg->vif2_mac + 1,
                    cfg->vif2_mac + 2, cfg->vif2_mac + 3,
                    cfg->vif2_mac + 4, cfg->vif2_mac + 5);
            AML_INFO("get vif2 mac:"MACFMT, MACARG(cfg->vif2_mac));
        }
    }

    return 0;
}

int aml_cfg_parse(struct aml_hw *aml_hw, struct aml_cfg *cfg)
{
    const char *path = AML_CFG_DEFAULT_PATH;
    struct file *fp = NULL;
    int status = -1;

    status = aml_cfg_create(path, &fp);
    switch (status) {
        case AML_CFG_EXIST:
            aml_cfg_from_file(cfg, fp);
            break;
        case AML_CFG_CREATE:
            aml_cfg_to_file(aml_hw, cfg, fp);
            fp->f_pos = 0;
            aml_cfg_from_file(cfg, fp);
            break;
        case AML_CFG_ERROR:
            /* when fp == NULL, just update */
            aml_cfg_to_file(aml_hw, cfg, NULL);
            break;
        default:
            AML_INFO("cfg file status error");
            break;
    }
    if (fp)
        aml_cfg_close(fp);

    return 0;
}

int aml_get_mac_addr_from_conftxt(unsigned int *efuse_data_l, unsigned int *efuse_data_h)
{
    int ret = -1;
    const u8 *tag_ptr;
    u8 fbuf[AML_CFG_FBUF_MAXLEN] = {0};
    const char *path = AML_CFG_DEFAULT_PATH;

    ret = aml_cfg_retrieve(path, fbuf, AML_CFG_FBUF_MAXLEN);
    if (ret >= AML_CFG_FBUF_MAXLEN) {
        AML_INFO("retrieve file data error\n");
        return -1;
    }

    tag_ptr = aml_cfg_find_tag(fbuf, strlen(fbuf),
            "VIF0_MACADDR=", strlen("00:00:00:00:00:00"));

    if (tag_ptr != NULL)
    {
        *efuse_data_l = (simple_strtoul(tag_ptr + 6, NULL, 16) << 24)
                        | (simple_strtoul(tag_ptr + 9, NULL, 16) << 16)
                        | (simple_strtoul(tag_ptr + 12, NULL, 16) << 8)
                        | (simple_strtoul(tag_ptr + 15, NULL, 16) << 0);
        *efuse_data_h = (simple_strtoul(tag_ptr, NULL, 16) << 8)
                        | (simple_strtoul(tag_ptr + 3, NULL, 16));
    }

    return 0;
}

int aml_cfg_parse_phy(struct aml_hw *aml_hw, const char *filename,
        struct aml_cfg_phy *cfg, int path)
{
    const struct firmware *cfg_fw = NULL;
    int ret;
    const u8 *tag_ptr;

    AML_DBG(AML_FN_ENTRY_STR);

    if ((ret = request_firmware(&cfg_fw, filename, aml_hw->dev))) {
        printk(KERN_CRIT "%s: Failed to get %s (%d)\n", __func__, filename, ret);
        return ret;
    }

    /* Get Trident path mapping */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "TRD_PATH_MAPPING=", strlen("00"));
    if (tag_ptr != NULL) {
        u8 val;
        if (sscanf((const char *)tag_ptr, "%hhx", &val) == 1)
            cfg->trd.path_mapping = val;
        else
            cfg->trd.path_mapping = path;
    } else
        cfg->trd.path_mapping = path;

    AML_DBG("Trident path mapping is: %d\n", cfg->trd.path_mapping);

    /* Get DC offset compensation */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "TX_DC_OFF_COMP=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->trd.tx_dc_off_comp) != 1)
            cfg->trd.tx_dc_off_comp = 0;
    } else
        cfg->trd.tx_dc_off_comp = 0;

    AML_DBG("TX DC offset compensation is: %08X\n", cfg->trd.tx_dc_off_comp);

    /* Get Karst TX IQ compensation value for path0 on 2.4GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_TX_IQ_COMP_2_4G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.tx_iq_comp_2_4G[0]) != 1)
            cfg->karst.tx_iq_comp_2_4G[0] = 0x01000000;
    } else
        cfg->karst.tx_iq_comp_2_4G[0] = 0x01000000;

    AML_DBG("Karst TX IQ compensation for path 0 on 2.4GHz is: %08X\n", cfg->karst.tx_iq_comp_2_4G[0]);

    /* Get Karst TX IQ compensation value for path1 on 2.4GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_TX_IQ_COMP_2_4G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.tx_iq_comp_2_4G[1]) != 1)
            cfg->karst.tx_iq_comp_2_4G[1] = 0x01000000;
    } else
        cfg->karst.tx_iq_comp_2_4G[1] = 0x01000000;

    AML_DBG("Karst TX IQ compensation for path 1 on 2.4GHz is: %08X\n", cfg->karst.tx_iq_comp_2_4G[1]);

    /* Get Karst RX IQ compensation value for path0 on 2.4GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_RX_IQ_COMP_2_4G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.rx_iq_comp_2_4G[0]) != 1)
            cfg->karst.rx_iq_comp_2_4G[0] = 0x01000000;
    } else
        cfg->karst.rx_iq_comp_2_4G[0] = 0x01000000;

    AML_DBG("Karst RX IQ compensation for path 0 on 2.4GHz is: %08X\n", cfg->karst.rx_iq_comp_2_4G[0]);

    /* Get Karst RX IQ compensation value for path1 on 2.4GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_RX_IQ_COMP_2_4G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.rx_iq_comp_2_4G[1]) != 1)
            cfg->karst.rx_iq_comp_2_4G[1] = 0x01000000;
    } else
        cfg->karst.rx_iq_comp_2_4G[1] = 0x01000000;

    AML_DBG("Karst RX IQ compensation for path 1 on 2.4GHz is: %08X\n", cfg->karst.rx_iq_comp_2_4G[1]);

    /* Get Karst TX IQ compensation value for path0 on 5GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_TX_IQ_COMP_5G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.tx_iq_comp_5G[0]) != 1)
            cfg->karst.tx_iq_comp_5G[0] = 0x01000000;
    } else
        cfg->karst.tx_iq_comp_5G[0] = 0x01000000;

    AML_DBG("Karst TX IQ compensation for path 0 on 5GHz is: %08X\n", cfg->karst.tx_iq_comp_5G[0]);

    /* Get Karst TX IQ compensation value for path1 on 5GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_TX_IQ_COMP_5G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.tx_iq_comp_5G[1]) != 1)
            cfg->karst.tx_iq_comp_5G[1] = 0x01000000;
    } else
        cfg->karst.tx_iq_comp_5G[1] = 0x01000000;

    AML_DBG("Karst TX IQ compensation for path 1 on 5GHz is: %08X\n", cfg->karst.tx_iq_comp_5G[1]);

    /* Get Karst RX IQ compensation value for path0 on 5GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_RX_IQ_COMP_5G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.rx_iq_comp_5G[0]) != 1)
            cfg->karst.rx_iq_comp_5G[0] = 0x01000000;
    } else
        cfg->karst.rx_iq_comp_5G[0] = 0x01000000;

    AML_DBG("Karst RX IQ compensation for path 0 on 5GHz is: %08X\n", cfg->karst.rx_iq_comp_5G[0]);

    /* Get Karst RX IQ compensation value for path1 on 5GHz */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_RX_IQ_COMP_5G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf((const char *)tag_ptr, "%08x", &cfg->karst.rx_iq_comp_5G[1]) != 1)
            cfg->karst.rx_iq_comp_5G[1] = 0x01000000;
    } else
        cfg->karst.rx_iq_comp_5G[1] = 0x01000000;

    AML_DBG("Karst RX IQ compensation for path 1 on 5GHz is: %08X\n", cfg->karst.rx_iq_comp_5G[1]);

    /* Get Karst default path */
    tag_ptr = aml_cfg_find_tag(cfg_fw->data, cfg_fw->size,
                            "KARST_DEFAULT_PATH=", strlen("00"));
    if (tag_ptr != NULL) {
        u8 val;
        if (sscanf((const char *)tag_ptr, "%hhx", &val) == 1)
            cfg->karst.path_used = val;
        else
            cfg->karst.path_used = path;
    } else
        cfg->karst.path_used = path;

    AML_DBG("Karst default path is: %d\n", cfg->karst.path_used);

    /* Release the configuration file */
    release_firmware(cfg_fw);

    return 0;
}

#define AML_CFG_RPS_CPUS(is_sta)  do { \
    char path[128]; \
    struct file *fp; \
    int i;\
    for (i = 0; i < 4; i++) { \
        sprintf(path, "/sys/class/net/%s/queues/rx-%d/rps_cpus", \
                (is_sta == 1) ? "wlan0": "ap0", i); \
        fp = aml_cfg_open(path, O_RDWR, 0666); \
        if (!fp) return; \
        aml_cfg_write(fp, "f", 1); \
        aml_cfg_close(fp); \
    } \
} while (0);

#define AML_CFG_RPS_FLOW(is_sta)  do { \
    char path[128]; \
    struct file *fp; \
    int i;\
    for (i = 0; i < 4; i++) { \
        sprintf(path, "/sys/class/net/%s/queues/rx-%d/rps_flow_cnt", \
                (is_sta == 1) ? "wlan0": "ap0", i); \
        fp = aml_cfg_open(path, O_RDWR, 0666); \
        if (!fp) return; \
        aml_cfg_write(fp, "4096", strlen("4096")); \
        aml_cfg_close(fp); \
    } \
} while (0);

#define AML_CFG_RPS_SOCK()  do { \
    char path[128]; \
    struct file *fp; \
    sprintf(path, "/proc/sys/net/core/rps_sock_flow_entries"); \
    fp = aml_cfg_open(path, O_RDWR, 0666); \
    if (!fp) return; \
    aml_cfg_write(fp, "16384", strlen("16384")); \
    aml_cfg_close(fp); \
} while (0);

void aml_cfg_update_rps(void)
{
    /* wlan0 interface */
    AML_CFG_RPS_CPUS(1);
    AML_CFG_RPS_FLOW(1);

    /* ap0 interface */
    AML_CFG_RPS_CPUS(0);
    AML_CFG_RPS_FLOW(0);

    AML_CFG_RPS_SOCK();
}
