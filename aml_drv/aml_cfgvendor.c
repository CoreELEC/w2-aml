/**
 ******************************************************************************
 *
 * @file aml_cfgvendor.c
 *
 * @brief Linux cfg80211 Vendor Extension Code
 *        New vendor interface addition to nl80211/cfg80211 to allow vendors
 *        to implement proprietary features over the cfg80211 stack.
 *
 * Copyright (C) Amlogic 2012-2024
 *
 ******************************************************************************
 */

#include "aml_cfgvendor.h"
#include "aml_utils.h"
#include "aml_defs.h"
#include "aml_msg_tx.h"

#ifdef CONFIG_AML_APF

struct mutex apf_mutex;

#define APF_LOCK() do {\
    mutex_lock(&apf_mutex);\
} while (0)

#define APF_UNLOCK() do {\
    mutex_unlock(&apf_mutex);\
} while (0)

#ifdef APF_DEBUG
void dump_apf_program(const u8 *program, size_t length) {
    print_hex_dump(KERN_INFO, "APF Program Hex: ", DUMP_PREFIX_NONE, 16, 1, program, length, false);
}
#endif

/**
 * aml_cfgvendor_apf_get_capabilities - Retrieve APF (Android Packet Filter) capabilities
 * @wiphy: Pointer to the wireless hardware description structure
 * @wdev: Pointer to the wireless device structure
 * @data: Pointer to the input data buffer (may contain specific request parameters)
 * @len: Length of the input data buffer
 *
 * This function is responsible for retrieving the capabilities of the Android Packet Filter
 * (APF) for a given wireless device. The APF capabilities describe what filtering features
 * are supported by the device.
 *
 * Return:
 *   0 on success, or a negative error code on failure.
 */
int aml_cfgvendor_apf_get_capabilities(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void *data, int len)
{
    struct net_device *ndev = wdev->netdev;
    struct aml_vif *aml_vif = netdev_priv(ndev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct sk_buff *skb = NULL;
    int ret, ver, max_len, mem_needed;

    APF_LOCK();

    // Get the APF capabilities of the WiFi firmware.
    ret = aml_apf_get_capabilities(aml_hw);
    if (ret) {
        AML_INFO("get apf capabilities fail\n");
        APF_UNLOCK();
        return ret;
    }

    // Get APF version
    ver = aml_hw->apf_params.apf_cap.version;
    // Get APF memory size limit
    max_len = aml_hw->apf_params.apf_cap.max_len;

    // Set APF memory address
    aml_hw->apf_params.apf_cap.apf_mem_addr |= DCCM_RAM_ADDR;

    AML_INFO("apf_version: %d max_len:%d apf_mem_addr 0x%x\n",
        ver, max_len, aml_hw->apf_params.apf_cap.apf_mem_addr);

    // Calculate the memory needed
    mem_needed = VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

    // Allocate socket buffer
    skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
    if (unlikely(!skb)) {
        AML_INFO("can't allocate %d bytes\n", mem_needed);
        APF_UNLOCK();
        return -ENOMEM;
    }

    // Put APF version into the buffer
    ret = nla_put_u32(skb, APF_ATTRIBUTE_VERSION, ver);
    if (ret < 0) {
        AML_INFO("failed to put APF_ATTRIBUTE_VERSION, ret:%d\n", ret);
        goto exit;
    }

    // Put APF max length into the buffer
    ret = nla_put_u32(skb, APF_ATTRIBUTE_MAX_LEN, max_len);
    if (ret < 0) {
        AML_INFO("failed to put APF_ATTRIBUTE_MAX_LEN, ret:%d\n", ret);
        goto exit;
    }

    // Send vendor command reply
    ret = cfg80211_vendor_cmd_reply(skb);
    if (unlikely(ret)) {
        AML_INFO("vendor command reply failed, ret=%d\n", ret);
    }

    APF_UNLOCK();

    return ret;

    exit:
    // Free skb memory
    kfree_skb(skb);
    APF_UNLOCK();
    return ret;
}


/**
 * aml_cfgvendor_apf_set_filter - Set the Android Packet Filter (APF) for the wireless device
 * @wiphy: Pointer to the wireless hardware description structure
 * @wdev: Pointer to the wireless device structure
 * @data: Pointer to the input data buffer containing APF program and length
 * @len: Length of the input data buffer
 *
 * This function sets a new Android Packet Filter (APF) program on the given wireless device.
 * The APF program is provided in the input data buffer along with its length.
 *
 * Return:
 *   0 on success, or a negative error code on failure.
 */
int aml_cfgvendor_apf_set_filter(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void  *data, int len)
{
    struct net_device *ndev = wdev->netdev;
    const struct nlattr *iter;
    u8 *program = NULL;
    u32 program_len = 0;
    int ret, tmp, type;
    struct aml_vif *aml_vif = netdev_priv(ndev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct apf_pgm_status apf_pgm_status = {0};

    APF_LOCK();

    // Check if the input data length is valid
    if (len <= 0) {
        AML_INFO("Invalid len: %d\n", len);
        ret = -EINVAL;
        goto exit;
    }

    // Iterate over netlink attributes
    nla_for_each_attr(iter, data, len, tmp) {
        type = nla_type(iter);
        switch (type) {
            case APF_ATTRIBUTE_PROGRAM_LEN:
                // Check if the iter value is valid and program_len is not already initialized.
                if (nla_len(iter) == sizeof(uint32_t) && !program_len) {
                    // Get the APF program length
                    program_len = nla_get_u32(iter);
                } else {
                    ret = -EINVAL;
                    goto exit;
                }

                // Check if the program length is within expected limits
                if (program_len > aml_hw->apf_params.apf_cap.max_len) {
                    AML_INFO("program len:%d is more than expected len :%d \n",
                        program_len, aml_hw->apf_params.apf_cap.max_len);
                    ret = -EINVAL;
                    goto exit;
                }

                // Check if the program length is zero
                if (unlikely(!program_len)) {
                    AML_INFO("zero program length\n");
                    ret = -EINVAL;
                    goto exit;
                }
                break;
            case APF_ATTRIBUTE_PROGRAM:
                // Check if the program is already allocated
                if (unlikely(program)) {
                    AML_INFO("program already allocated\n");
                    ret = -EINVAL;
                    goto exit;
                }
                // Check if the program length is not set
                if (unlikely(!program_len)) {
                    AML_INFO("program len is not set\n");
                    ret = -EINVAL;
                    goto exit;
                }
                // Check if the program length matches the expected length
                if (nla_len(iter) != program_len) {
                    AML_INFO("program_len is not same\n");
                    ret = -EINVAL;
                    goto exit;
                }
                // Allocate memory for the program
                program= kmalloc(program_len, GFP_KERNEL);
                if (!program) {
                    AML_INFO("can't allocate %d bytes\n", program_len);
                    ret = -ENOMEM;
                    goto exit;
                }
                // Copy the program data
                memcpy(program, (uint8_t*)nla_data(iter), program_len);
                break;
            default:
                AML_INFO("no such attribute %d\n", type);
                ret = -EINVAL;
                goto exit;
            }
    }

    // Delete the existing filter, if any
    if (aml_hw->apf_params.apf_set)
    {
        aml_apf_delete_filter(aml_hw, &apf_pgm_status);
        if (apf_pgm_status.apf_status == APF_PROGRAM_DELETED)
        {
            AML_INFO("apf_program delete success\n");
        }
        aml_hw->apf_params.apf_set = false;
    }

    // Add the new APF filter
    aml_apf_add_filter(aml_hw, program, program_len);
    aml_hw->apf_params.apf_set = true;
    aml_hw->apf_params.program_len = program_len;

#ifdef APF_DEBUG
    dump_apf_program(program, program_len);
#endif

    exit:
    if (program) {
        // Free the program memory
        kfree(program);
    }
    APF_UNLOCK();

    return ret;
}


/**
 * aml_cfgvendor_apf_read_filter_data - Read Android Packet Filter (APF) filter data from hardware
 * @wiphy: Pointer to the wireless hardware description structure
 * @wdev: Pointer to the wireless device structure
 * @data: Pointer to input data buffer (not used in this function)
 * @len: Length of the input data buffer (not used in this function)
 *
 * This function retrieves APF filter data from the hardware, formats it into a Netlink attribute,
 * and sends it back as a vendor command reply. It allocates memory for the APF filter data and
 * constructs the Netlink attributes accordingly.
 *
 * Return:
 *   0 on success, or a negative error code on failure.
 */
int aml_cfgvendor_apf_read_filter_data(struct wiphy *wiphy,
    struct wireless_dev *wdev, const void *data, int len)
{
    struct net_device *ndev = wdev->netdev;
    struct aml_vif *aml_vif = netdev_priv(ndev);
    struct aml_hw *aml_hw = aml_vif->aml_hw;
    struct sk_buff *skb = NULL;
    u8 *buf = NULL;
    int ret, buf_len, program_len, mem_needed;

    APF_LOCK();

    AML_FN_ENTRY();

    // Update APF max length if not already initialized
    if (aml_hw->apf_params.apf_cap.max_len == 0) {
        ret = aml_apf_get_capabilities(aml_hw);
        if (ret) {
            AML_INFO("APF get maximum length failed ret=%d\n", ret);
            APF_UNLOCK();
            return -1;
        }
    }

    aml_hw->apf_params.apf_cap.apf_mem_addr |= DCCM_RAM_ADDR;

    if (aml_hw->apf_params.program_len == 0)
    {
        AML_INFO("Error:program_len is 0\n");
        APF_UNLOCK();
        return -1;
    }

    // Retrieve APF maximum length
    program_len = aml_hw->apf_params.apf_cap.max_len;

    buf = kmalloc(program_len, GFP_KERNEL);
    if (unlikely(!buf)) {
        AML_INFO("can't allocate %d bytes\n", program_len);
        ret = -ENOMEM;
        goto fail;
    }

    // Get APF filter data
    aml_apf_read_filter_data(aml_hw, buf, program_len);

    // Calculate the memory needed
    mem_needed = VENDOR_REPLY_OVERHEAD + ATTRIBUTE_U32_LEN + program_len;

    // Allocate socket buffer for vendor command reply
    skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
    if (unlikely(!skb)) {
        AML_INFO("can't allocate %d bytes\n", mem_needed);
        ret = -ENOMEM;
        goto fail;
    }

    // Add APF program length attribute to the reply
    ret = nla_put_u32(skb, APF_ATTRIBUTE_PROGRAM_LEN, program_len);
    if (ret < 0) {
        AML_INFO("Failed to put APF_ATTRIBUTE_MAX_LEN, ret=%d\n", ret);
        goto fail;
    }

    // Add APF program attribute to the reply
    ret = nla_put(skb, APF_ATTRIBUTE_PROGRAM, program_len, buf);
    if (ret < 0) {
        AML_INFO("Failed to put APF_ATTRIBUTE_MAX_LEN, ret=%d\n", ret);
        goto fail;
    }

    // Send vendor command reply
    ret = cfg80211_vendor_cmd_reply(skb);
    if (unlikely(ret)) {
        AML_INFO("vendor command reply failed, ret=%d\n", ret);
    }

#ifdef APF_DEBUG
    dump_apf_program(buf, program_len);
#endif

    // Clean up allocated resources
    if (buf) {
        kfree(buf);
    }

    AML_FN_EXIT();
    APF_UNLOCK();
    return ret;

fail:
    if (buf) {
        // Clean up allocated resources in case of failure
        kfree(buf);
    }

    if (skb) {
        // Free socket buffer memory
        kfree_skb(skb);
    }

    APF_UNLOCK();

    return ret;
}
#endif /* CONFIG_AML_APF */
