/**
 ****************************************************************************************
 *
 * @file aml_cfg.h
 *
 * Copyright (C) Amlogic 2012-2023
 *
 ****************************************************************************************
 */

#ifndef _AML_CFG_H_
#define _AML_CFG_H_

#ifdef CONFIG_LINUXPC_VERSION
#define AML_CFG_DEFAULT_PATH   "/lib/firmware/wifi_conf.txt"
#else
#define AML_CFG_DEFAULT_PATH   "/data/vendor/wifi/wifi_conf.txt"
#endif

#define AML_CFG_LBUF_MAXLEN    (64)
#define AML_CFG_FBUF_MAXLEN    (AML_CFG_LBUF_MAXLEN * 16)

#define AML_EFUSE_CHIPID_LEN        12
#define AML_EFUSE_CHIPID_LOW        0x8
#define AML_EFUSE_CHIPID_HIGH       0x9
#define AML_EFUSE_MACADDR_LOW       0x1
#define AML_EFUSE_MACADDR_HIGH      0x2
#define AML_EFUSE_MACADDR_LOW1      0x11
#define AML_EFUSE_MACADDR_HIGH1     0x12
#define AML_EFUSE_MACADDR_VALID     0x7
#define AML_EFUSE_MACADDR_OFFSET    0x11

enum aml_cfg_flag {
    AML_CFG_ERROR,
    AML_CFG_EXIST,
    AML_CFG_CREATE,
};

struct aml_cfg {
    unsigned int chipid_l;
    unsigned int chipid_h;
    u8 vif0_mac[ETH_ALEN];
    u8 vif1_mac[ETH_ALEN];
    u8 vif2_mac[ETH_ALEN];
};

struct aml_cfg_phy {
    struct phy_trd_cfg_tag trd;
    struct phy_karst_cfg_tag karst;
    struct phy_cataxia_cfg_tag cataxia;
};

int aml_cfg_parse(struct aml_hw *aml_hw, struct aml_cfg *cfg);
int aml_cfg_parse_phy(struct aml_hw *aml_hw, const char *file,
        struct aml_cfg_phy *cfg, int path);
void aml_cfg_update_rps(void);
int aml_get_mac_addr_from_conftxt(unsigned int *efuse_data_l, unsigned int *efuse_data_h);

#endif /* _AML_CFG_H_ */
