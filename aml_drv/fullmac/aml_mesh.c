/**
 ****************************************************************************************
 *
 * @file aml_mesh.c
 *
 * Copyright (C) Amlogic 2016-2021
 *
 ****************************************************************************************
 */

/**
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "aml_mesh.h"

/**
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

struct aml_mesh_proxy *aml_get_mesh_proxy_info(struct aml_vif *p_aml_vif, u8 *p_sta_addr, bool local)
{
    struct aml_mesh_proxy *p_mesh_proxy = NULL;
    struct aml_mesh_proxy *p_cur_proxy;

    /* Look for proxied devices with provided address */
    list_for_each_entry(p_cur_proxy, &p_aml_vif->ap.proxy_list, list) {
        if (p_cur_proxy->local != local) {
            continue;
        }

        if (!memcmp(&p_cur_proxy->ext_sta_addr, p_sta_addr, ETH_ALEN)) {
            p_mesh_proxy = p_cur_proxy;
            break;
        }
    }

    /* Return the found information */
    return p_mesh_proxy;
}
