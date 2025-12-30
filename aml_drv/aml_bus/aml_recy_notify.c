/*
* Copyright (c) 2025 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined int the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*    For recovery done notify other modules.
*/

#include <linux/module.h>
#include <linux/notifier.h>

static BLOCKING_NOTIFIER_HEAD(aml_recovery_notifier_list);

int aml_notify_recovery_event(int event)
{
    return blocking_notifier_call_chain(&aml_recovery_notifier_list, event, NULL);
}
EXPORT_SYMBOL_GPL(aml_notify_recovery_event);

int aml_register_recovery_event(struct notifier_block *nb)
{
    return blocking_notifier_chain_register(&aml_recovery_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(aml_register_recovery_event);

int aml_unregister_recovery_event(struct notifier_block *nb)
{
    return blocking_notifier_chain_unregister(&aml_recovery_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(aml_unregister_recovery_event);

