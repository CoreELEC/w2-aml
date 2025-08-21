/**
 ******************************************************************************
 *
 * @file aml_pci.h
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */

#ifndef _AML_W2_PCI_H_
#define _AML_W2_PCI_H_

int aml_pci_register_drv(void);
void aml_pci_unregister_drv(void);
int aml_pci_insmod(void);
void aml_pci_rmmod(void);

#endif /* _AML_PCI_H_ */
