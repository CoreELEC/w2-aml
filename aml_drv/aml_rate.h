/*
****************************************************************************************
*
* @file aml_rate.h
*
* @brief Implementation of the aml sdio_usb rx rate
*
* Copyright (C) Amlogic 2016-2024
*
****************************************************************************************
*/

#ifndef AML_RATE_H_
#define AML_RATE_H_

#define AML_HW_RATE_HE_SU_NUM   (N_CCK + N_OFDM + N_HT + N_VHT + N_HE_SU)
#define AML_HW_RATE_NUM         (AML_HW_RATE_HE_SU_NUM + N_HE_MU + N_HE_ER)

struct aml_hw;
struct aml_sta;
union aml_rate_ctrl_info;

int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
               int sgi, int pre, int dcm, int *r_idx);
int print_rate_from_cfg(char *buf, int size, u32 rate_config, int *r_idx, int ru_size);

void idx_to_rate_cfg(int idx, union aml_rate_ctrl_info *r_cfg, int *ru_size);
char *print_sta_rate_stats(struct aml_hw *aml_hw, struct aml_sta *sta);
char *print_sta_rc_stats( struct aml_hw *aml_hw, struct aml_sta *sta);

void aml_rx_statistic(struct aml_hw *aml_hw, struct hw_vect *hwvect);
void aml_rx_sta_stats(struct aml_hw *aml_hw, struct aml_sta *sta, struct hw_vect *hwvect);

int aml_sta_rate_table_init(struct aml_hw *aml_hw, struct aml_sta *sta);
void aml_sta_rate_table_deinit(struct aml_hw *aml_hw, struct aml_sta *sta);

int aml_dynamic_snr_config(struct aml_hw *aml_hw, int enable, int snr_cfg_or_mcs_ration);
int aml_dynamic_snr_init(struct aml_hw *aml_hw);
void aml_dynamic_snr_deinit(struct aml_hw *aml_hw);

#endif /* AML_RATE_H_ */
