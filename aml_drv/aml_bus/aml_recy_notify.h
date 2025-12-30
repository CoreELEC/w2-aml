/*
* Copyright (c) 2025 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined int the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*    For recovery done notify other modules.
*/

#ifndef _AML_RECY_NOTIFY_H_
#define _AML_RECY_NOTIFY_H_

extern int aml_notify_recovery_event(int event);
extern int aml_register_recovery_event(struct notifier_block *nb);
extern int aml_unregister_recovery_event(struct notifier_block *nb);

#endif //_AML_RECY_NOTIFY_H_

