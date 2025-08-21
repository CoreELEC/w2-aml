/**
 ****************************************************************************************
 *
 * @file aml_dini.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_DINI_H_
#define _AML_DINI_H_

#include <linux/pci.h>
#include "aml_platform.h"

int aml_dini_platform_init(struct pci_dev *pci_dev,
                            struct aml_plat **aml_plat);

#endif /* _AML_DINI_H_ */
