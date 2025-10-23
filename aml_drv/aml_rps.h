/**
 ******************************************************************************
 *
 * @file aml_rps.h
 *
 * Copyright (C) Amlogic 2012-2023
 *
 ******************************************************************************
 */

#ifndef _AML_RPS_H_
#define _AML_RPS_H_

int aml_rps_cpus_enable(struct net_device *net);
int aml_rps_cpus_disable(struct net_device *net);
int aml_xps_cpus_enable(struct net_device *net);
int aml_rps_dev_flow_table_enable(struct net_device *net);
int aml_rps_sock_flow_sysctl_enable(void);

#endif /* _AML_RPS_H_ */
