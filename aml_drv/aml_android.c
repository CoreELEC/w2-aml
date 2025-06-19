/**
 ****************************************************************************************
 *
 * @file aml_android.c
 *
 * Copyright (C) Amlogic, Inc. All rights reserved (2022).
 *
 * @brief Android WPA driver API implementation.
 *
 ****************************************************************************************
 */
#include <linux/version.h>
#include <linux/compat.h>

#include "aml_utils.h"
#include "aml_android.h"

static struct aml_android_cmd aml_android_cmd_tbl[] = {
    AML_ANDROID_CMD(CMDID_RSSI, "RSSI"),
    AML_ANDROID_CMD(CMDID_COUNTRY, "COUNTRY"),
    AML_ANDROID_CMD(CMDID_P2P_SET_NOA, "P2P_SET_NOA"),
    AML_ANDROID_CMD(CMDID_P2P_GET_NOA, "P2P_GET_NOA"),
    AML_ANDROID_CMD(CMDID_P2P_SET_PS, "P2P_SET_PS"),
    AML_ANDROID_CMD(CMDID_SET_AP_WPS_P2P_IE, "SET_AP_WPS_P2P_IE"),
    AML_ANDROID_CMD(CMDID_BTCOEXSCAN, "BTCOEXSCAN"),
    AML_ANDROID_CMD(CMDID_SETSUSPENDMODE, "SETSUSPENDMODE"),
};

static int aml_android_cmdstr2id(char *cmdstr)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(aml_android_cmd_tbl); i++) {
        const char *str = aml_android_cmd_tbl[i].str;
        if (!strncasecmp(cmdstr, str, strlen(str)))
            break;
    }

    if (i >= ARRAY_SIZE(aml_android_cmd_tbl))
        return -1;

    return aml_android_cmd_tbl[i].id;
}

static int aml_android_get_rssi(struct aml_vif *vif, char *cmdstr, int len)
{
    int bytes = 0;
    s8 rssi = 0;

    if (AML_VIF_TYPE(vif) != NL80211_IFTYPE_STATION)
        return 0;

    if (vif->sta.ap) {
        struct aml_plat *aml_plat = vif->aml_hw->plat;

        // Generic info
        rssi = (AML_REG_READ(aml_plat, AML_ADDR_MAC_PHY, REG_OF_SYNC_RSSI) & 0xffff) - 256;
        bytes = snprintf(&cmdstr[bytes], len, "%s rssi %d\n",
                ssid_sprintf(vif->sta.assoc_ssid, vif->sta.assoc_ssid_len), rssi);
    }
    return bytes;
}

static int aml_android_set_suspend_mode(struct aml_vif *vif, char *cmdstr, int len)
{
    int skip = strlen("SETSUSPENDMODE") + 1;
    struct aml_hw *aml_hw = vif->aml_hw;
    int val;
    int ret;

    ret = sscanf(cmdstr + skip, "%d", &val);
    if (ret != 1) {
        AML_INFO("param error %s\n", cmdstr);
        return -1;
    }

    aml_hw->google_cast = val;
    AML_INFO("set google_cast %d\n", aml_hw->google_cast);

    return 0;
}

int aml_android_priv_ioctl(struct aml_vif *vif, void __user *data)
{
    struct aml_android_priv_cmd cmd;
    int id = 0, ret = 0;
    char *resp = NULL;
    int resp_len = 0;

    if (!data)
        return -EINVAL;

#ifdef CONFIG_COMPAT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0))
    if (in_compat_syscall())
#else
    if (is_compat_task())
#endif
    {
        struct aml_android_compat_priv_cmd compat_cmd;
        if (copy_from_user(&compat_cmd, data,
                    sizeof(struct aml_android_compat_priv_cmd)))
            return -EFAULT;

        cmd.buf = compat_ptr(compat_cmd.buf);
        cmd.used_len = compat_cmd.used_len;
        cmd.total_len = compat_cmd.total_len;
    } else
#endif
    {
        if (copy_from_user(&cmd, data, sizeof(struct aml_android_priv_cmd)))
            return -EFAULT;
    }

    if (cmd.total_len > AML_ANDROID_CMD_MAX_LEN || cmd.total_len < 0)
        return -EFAULT;

    resp = kmalloc(cmd.total_len, GFP_KERNEL);
    if (!resp) {
        ret = -ENOMEM;
        goto exit;
    }

    if (copy_from_user(resp, cmd.buf, cmd.total_len)) {
        ret = -EFAULT;
        goto exit;
    }

    id = aml_android_cmdstr2id(resp);
    if (id == -1) {
        ret = -EFAULT;
        goto exit;
    }

    switch (id) {
        case CMDID_RSSI:
            resp_len = aml_android_get_rssi(vif, resp, cmd.total_len);
            break;
        case CMDID_COUNTRY:
        /* FIXME: android will call p2p priv ioctl, if invokes failed,
         * it will cause the dynamic p2p interface create failed. */
        case CMDID_P2P_SET_NOA:
        case CMDID_P2P_GET_NOA:
        case CMDID_P2P_SET_PS:
        case CMDID_SET_AP_WPS_P2P_IE:
            ret = 0; /* do nothing in these cases */
            break;
        case CMDID_SETSUSPENDMODE:
            aml_android_set_suspend_mode(vif, resp, cmd.total_len);
            break;
        default:
            AML_INFO("not support id=%u\n", id);
            break;
    }

    if (resp_len >= 0) {
        if (resp_len == 0 && cmd.total_len > 0)
            resp[0] = '\0';
        if (resp_len >= cmd.total_len) {
            resp_len = cmd.total_len;
        } else {
            resp_len++;
        }
        cmd.used_len = resp_len;
        if (copy_to_user(cmd.buf, resp, resp_len))
        {
            AML_INFO("copy data to user buffer failed\n");
            ret = -EFAULT;
        }
    }

exit:
    if (resp)
        kfree(resp);
    return ret;
}
