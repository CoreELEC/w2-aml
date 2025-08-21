#ifdef RF_D_BT_TOP_REG
#else
#define RF_D_BT_TOP_REG


#define RF_D_BT_TOP_REG_BASE                     (0x80002000)

#define RG_BT_TOP_A0                                 (0x0)
// Bit 15  :0      rg_bt_top_reserved0            U     RW        default = 'h0
// Bit 31  :16     rg_bt_top_reserved1            U     RW        default = 'hffff
typedef union RG_BT_TOP_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TOP_RESERVED0 : 16;
    unsigned int RG_BT_TOP_RESERVED1 : 16;
  } b;
} RG_BT_TOP_A0_FIELD_T;

#define RG_BT_TOP_A1                                 (0x4)
// Bit 1   :0      rg_bt_rx_mode                  U     RW        default = 'h3
// Bit 5   :4      rg_bt_tx_mode                  U     RW        default = 'h2
// Bit 8           rg_bt_mode_man_mode            U     RW        default = 'h0
// Bit 13  :12     rg_bt_mode_man                 U     RW        default = 'h0
// Bit 15  :14     rg_bt_trswitch_edge_mask       U     RW        default = 'h0
// Bit 18  :16     rg_bt_top_dc_test_sel          U     RW        default = 'h0
// Bit 21  :19     rg_bt_top_ac_test_sel          U     RW        default = 'h0
// Bit 22          rg_bt_ac_test_en               U     RW        default = 'h0
// Bit 23          rg_bt_dc_test_trx_en           U     RW        default = 'h0
// Bit 24          rg_bt_dc_test_abbsx_en         U     RW        default = 'h0
// Bit 25          rg_bt_top_tx_test_en           U     RW        default = 'h0
// Bit 26          rg_bt_top_adc_test_en          U     RW        default = 'h0
// Bit 27          rg_bt_top_rx_test_en           U     RW        default = 'h0
// Bit 28          rg_bt_top_wf0tx_test_en        U     RW        default = 'h0
// Bit 30          rg_bt_agc_lock_man_mode        U     RW        default = 'h0
// Bit 31          rg_bt_agc_lock_man             U     RW        default = 'h0
typedef union RG_BT_TOP_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_RX_MODE : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RG_BT_TX_MODE : 2;
    unsigned int rsvd_1 : 2;
    unsigned int RG_BT_MODE_MAN_MODE : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RG_BT_MODE_MAN : 2;
    unsigned int RG_BT_TRSWITCH_EDGE_MASK : 2;
    unsigned int RG_BT_TOP_DC_TEST_SEL : 3;
    unsigned int RG_BT_TOP_AC_TEST_SEL : 3;
    unsigned int RG_BT_AC_TEST_EN : 1;
    unsigned int RG_BT_DC_TEST_TRX_EN : 1;
    unsigned int RG_BT_DC_TEST_ABBSX_EN : 1;
    unsigned int RG_BT_TOP_TX_TEST_EN : 1;
    unsigned int RG_BT_TOP_ADC_TEST_EN : 1;
    unsigned int RG_BT_TOP_RX_TEST_EN : 1;
    unsigned int RG_BT_TOP_WF0TX_TEST_EN : 1;
    unsigned int rsvd_3 : 1;
    unsigned int RG_BT_AGC_LOCK_MAN_MODE : 1;
    unsigned int RG_BT_AGC_LOCK_MAN : 1;
  } b;
} RG_BT_TOP_A1_FIELD_T;

#define RG_BT_TOP_A2                                 (0x8)
// Bit 1   :0      rg_bt_adc_bs_sel               U     RW        default = 'h1
// Bit 2           rg_bt_adc_c_adj                U     RW        default = 'h0
// Bit 5   :3      rg_bt_adc_clk_delay_adj        U     RW        default = 'h2
// Bit 6           rg_bt_adc_ref_adj              U     RW        default = 'h1
// Bit 14  :7      rg_bt_adc_resv                 U     RW        default = 'h7
// Bit 15          rg_bt_adc_skip_sel             U     RW        default = 'h0
// Bit 16          rg_bt_adda_ldo_lf_rc_ctrl      U     RW        default = 'h0
// Bit 17          rg_bt_adda_ldo_rc_modelsel     U     RW        default = 'h0
// Bit 22  :18     rg_bt_adda_ldo_vref_adj        U     RW        default = 'h10
// Bit 23          rg_bt_adda_load_en             U     RW        default = 'h0
// Bit 28  :24     rg_bt_dac_rc_adj               U     RW        default = 'h1f
typedef union RG_BT_TOP_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bt_adc_bs_sel : 2;
    unsigned int rg_bt_adc_c_adj : 1;
    unsigned int rg_bt_adc_clk_delay_adj : 3;
    unsigned int rg_bt_adc_ref_adj : 1;
    unsigned int rg_bt_adc_resv : 8;
    unsigned int rg_bt_adc_skip_sel : 1;
    unsigned int rg_bt_adda_ldo_lf_rc_ctrl : 1;
    unsigned int rg_bt_adda_ldo_rc_modelsel : 1;
    unsigned int rg_bt_adda_ldo_vref_adj : 5;
    unsigned int rg_bt_adda_load_en : 1;
    unsigned int rg_bt_dac_rc_adj : 5;
    unsigned int rsvd_0 : 3;
  } b;
} RG_BT_TOP_A2_FIELD_T;

#define RG_BT_TOP_A3                                 (0xc)
// Bit 15  :0      rg_bt_adda_rsv                 U     RW        default = 'h0
// Bit 16          rg_bt_dac_bias_tunc            U     RW        default = 'h0
// Bit 17          rg_bt_dac_clk_en               U     RW        default = 'h1
// Bit 18          rg_bt_dac_clk_phase_i          U     RW        default = 'h1
// Bit 19          rg_bt_dac_clk_phase_q          U     RW        default = 'h1
// Bit 20          rg_bt_dac_data_phase_mode      U     RW        default = 'h0
// Bit 21          rg_bt_dac_dcoc_en              U     RW        default = 'h0
// Bit 25  :22     rg_bt_dac_dcoc_i               U     RW        default = 'h0
// Bit 29  :26     rg_bt_dac_dcoc_q               U     RW        default = 'h0
typedef union RG_BT_TOP_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bt_adda_rsv : 16;
    unsigned int rg_bt_dac_bias_tunc : 1;
    unsigned int rg_bt_dac_clk_en : 1;
    unsigned int rg_bt_dac_clk_phase_i : 1;
    unsigned int rg_bt_dac_clk_phase_q : 1;
    unsigned int rg_bt_dac_data_phase_mode : 1;
    unsigned int rg_bt_dac_dcoc_en : 1;
    unsigned int rg_bt_dac_dcoc_i : 4;
    unsigned int rg_bt_dac_dcoc_q : 4;
    unsigned int rsvd_0 : 2;
  } b;
} RG_BT_TOP_A3_FIELD_T;

#define RG_BT_TOP_A4                                 (0x10)
// Bit 3   :0      rg_bt_dac_driver_adj           U     RW        default = 'h6
// Bit 5   :4      rg_bt_dac_gain                 U     RW        default = 'h2
// Bit 7   :6      rg_bt_dac_ibias_tunc           U     RW        default = 'h0
// Bit 8           rg_bt_dac_op_en                U     RW        default = 'h1
// Bit 17  :14     rg_bt_dac_vcm_adj              U     RW        default = 'h2
// Bit 18          rg_bt_adda_ldo_fc              U     RW        default = 'h0
// Bit 19          rg_bt_dac_tia_mux_sel_i        U     RW        default = 'h0
// Bit 20          rg_bt_dac_tia_mux_sel_q        U     RW        default = 'h0
// Bit 21          rg_bt_adda_man_mode            U     RW        default = 'h0
// Bit 23  :22     rg_da_bt_dac_ampctrl           U     RW        default = 'h0
typedef union RG_BT_TOP_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bt_dac_driver_adj : 4;
    unsigned int rg_bt_dac_gain : 2;
    unsigned int rg_bt_dac_ibias_tunc : 2;
    unsigned int rg_bt_dac_op_en : 1;
    unsigned int rsvd_0 : 5;
    unsigned int rg_bt_dac_vcm_adj : 4;
    unsigned int rg_bt_adda_ldo_fc : 1;
    unsigned int rg_bt_dac_tia_mux_sel_i : 1;
    unsigned int rg_bt_dac_tia_mux_sel_q : 1;
    unsigned int rg_bt_adda_man_mode : 1;
    unsigned int rg_da_bt_dac_ampctrl : 2;
    unsigned int rsvd_1 : 8;
  } b;
} RG_BT_TOP_A4_FIELD_T;

#define RG_BT_TOP_A5                                 (0x14)
// Bit 0           rg_m0_da_bt_adda_ldo_en        U     RW        default = 'h0
// Bit 1           rg_m0_da_bt_adc_en             U     RW        default = 'h0
// Bit 2           rg_m0_da_bt_dac_en             U     RW        default = 'h0
// Bit 3           rg_m1_da_bt_adda_ldo_en        U     RW        default = 'h0
// Bit 4           rg_m1_da_bt_adc_en             U     RW        default = 'h0
// Bit 5           rg_m1_da_bt_dac_en             U     RW        default = 'h0
// Bit 6           rg_m2_da_bt_adda_ldo_en        U     RW        default = 'h1
// Bit 7           rg_m2_da_bt_adc_en             U     RW        default = 'h0
// Bit 8           rg_m2_da_bt_dac_en             U     RW        default = 'h1
// Bit 9           rg_m3_da_bt_adda_ldo_en        U     RW        default = 'h1
// Bit 10          rg_m3_da_bt_adc_en             U     RW        default = 'h1
// Bit 11          rg_m3_da_bt_dac_en             U     RW        default = 'h0
typedef union RG_BT_TOP_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m0_da_bt_adda_ldo_en : 1;
    unsigned int rg_m0_da_bt_adc_en : 1;
    unsigned int rg_m0_da_bt_dac_en : 1;
    unsigned int rg_m1_da_bt_adda_ldo_en : 1;
    unsigned int rg_m1_da_bt_adc_en : 1;
    unsigned int rg_m1_da_bt_dac_en : 1;
    unsigned int rg_m2_da_bt_adda_ldo_en : 1;
    unsigned int rg_m2_da_bt_adc_en : 1;
    unsigned int rg_m2_da_bt_dac_en : 1;
    unsigned int rg_m3_da_bt_adda_ldo_en : 1;
    unsigned int rg_m3_da_bt_adc_en : 1;
    unsigned int rg_m3_da_bt_dac_en : 1;
    unsigned int rsvd_0 : 20;
  } b;
} RG_BT_TOP_A5_FIELD_T;

#define RG_BT_TOP_A6                                 (0x18)
// Bit 28  :24     rg_bt_ad_inv                   U     RW        default = 'h0
typedef union RG_BT_TOP_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 24;
    unsigned int RG_BT_AD_INV : 5;
    unsigned int rsvd_1 : 3;
  } b;
} RG_BT_TOP_A6_FIELD_T;

#define RG_BT_TOP_A7                                 (0x1c)
// Bit 0           rg_bt_trx_hf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 1           rg_bt_trx_hf_ldo_fc_en_man     U     RW        default = 'h0
// Bit 2           rg_wfbt_trx_hf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 3           rg_wfbt_trx_hf_ldo_fc_en_man     U     RW        default = 'h0
// Bit 5   :4      rg_bt_trx_ldo_fc_cnt           U     RW        default = 'h0
typedef union RG_BT_TOP_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TRX_HF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_BT_TRX_HF_LDO_FC_EN_MAN : 1;
    unsigned int RG_WFBT_TRX_HF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_WFBT_TRX_HF_LDO_FC_EN_MAN : 1;
    unsigned int RG_BT_TRX_LDO_FC_CNT : 2;
    unsigned int rsvd_0 : 26;
  } b;
} RG_BT_TOP_A7_FIELD_T;

#define RG_BT_TOP_A8                                 (0x20)
// Bit 1   :0      ro_hw_bt_trx_mode              U     RO        default = 'h0
// Bit 4           ro_hw_bt_rx_bw                 U     RO        default = 'h0
// Bit 15  :8      ro_hw_bt_rx_gain               U     RO        default = 'h0
// Bit 16          ro_hw_bt_agc_lock              U     RO        default = 'h0
// Bit 20          ro_bt_sx_chmap_en              U     RO        default = 'h0
// Bit 31  :24     ro_bt_sx_chmap                 U     RO        default = 'h0
typedef union RG_BT_TOP_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_HW_BT_TRX_MODE : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RO_HW_BT_RX_BW : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RO_HW_BT_RX_GAIN : 8;
    unsigned int RO_HW_BT_AGC_LOCK : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RO_BT_SX_CHMAP_EN : 1;
    unsigned int rsvd_3 : 3;
    unsigned int RO_BT_SX_CHMAP : 8;
  } b;
} RG_BT_TOP_A8_FIELD_T;

#endif

