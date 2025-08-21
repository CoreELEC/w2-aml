#ifdef RF_D_TOP_REG
#else
#define RF_D_TOP_REG


#define RF_D_TOP_REG_BASE                         (0x0)

#define RG_TOP_A0                                 (RF_D_TOP_REG_BASE + 0x0)
// Bit 31  :0      rg_wf_rx_dig_rsv0              U     RW        default = 'hffff
typedef union RG_TOP_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_RX_DIG_RSV0 : 32;
  } b;
} RG_TOP_A0_FIELD_T;

#define RG_TOP_A1                                 (RF_D_TOP_REG_BASE + 0x4)
// Bit 31  :0      rg_wf_rx_dig_rsv1              U     RW        default = 'hffff
typedef union RG_TOP_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_RX_DIG_RSV1 : 32;
  } b;
} RG_TOP_A1_FIELD_T;

#define RG_TOP_A2                                 (RF_D_TOP_REG_BASE + 0x8)
// Bit 2   :0      rg_wf_mode_man                 U     RW        default = 'h0
// Bit 4           rg_wf_mode_man_mode            U     RW        default = 'h0
// Bit 7   :6      rg_wf_sleep_mode               U     RW        default = 'h0
// Bit 9   :8      rg_wf5g_sx_mode                U     RW        default = 'h1
// Bit 11  :10     rg_wf5g_tx_mode                U     RW        default = 'h2
// Bit 13  :12     rg_wf5g_rx_mode                U     RW        default = 'h3
// Bit 15  :14     rg_wf5g_dpd_mode               U     RW        default = 'h0
// Bit 17  :16     rg_wf2g_sx_mode                U     RW        default = 'h1
// Bit 19  :18     rg_wf2g_tx_mode                U     RW        default = 'h2
// Bit 21  :20     rg_wf2g_rx_mode                U     RW        default = 'h3
// Bit 24          rg_wf_rf_mode                  U     RW        default = 'h0
// Bit 26          rg_wf_agc_lock_man             U     RW        default = 'h0
// Bit 28          rg_wf_agc_lock_man_mode        U     RW        default = 'h0
// Bit 30  :29     rg_wf_trswitch_edge_mask       U     RW        default = 'h0
typedef union RG_TOP_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_MODE_MAN : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF_MODE_MAN_MODE : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF_SLEEP_MODE : 2;
    unsigned int RG_WF5G_SX_MODE : 2;
    unsigned int RG_WF5G_TX_MODE : 2;
    unsigned int RG_WF5G_RX_MODE : 2;
    unsigned int RG_WF5G_DPD_MODE : 2;
    unsigned int RG_WF2G_SX_MODE : 2;
    unsigned int RG_WF2G_TX_MODE : 2;
    unsigned int RG_WF2G_RX_MODE : 2;
    unsigned int rsvd_2 : 2;
    unsigned int RG_WF_RF_MODE : 1;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF_AGC_LOCK_MAN : 1;
    unsigned int rsvd_4 : 1;
    unsigned int RG_WF_AGC_LOCK_MAN_MODE : 1;
    unsigned int RG_WF_TRSWITCH_EDGE_MASK : 2;
    unsigned int rsvd_5 : 1;
  } b;
} RG_TOP_A2_FIELD_T;

#define RG_TOP_A3                                 (RF_D_TOP_REG_BASE + 0xc)
// Bit 0           rg_wf_trx_hf_ldo_fc_sel        U     RW        default = 'h0
// Bit 1           rg_wf_trx_hf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 2           rg_wf_trx_hf_ldo_fc_en_man     U     RW        default = 'h0
// Bit 7   :3      rg_wf_trx_hf_ldo_vsel          U     RW        default = 'h10
// Bit 8           rg_wf_trx_hf_ldo_rc_ctrl       U     RW        default = 'h1
// Bit 9           rg_wf_trx_lf_ldo_fc_sel        U     RW        default = 'h0
// Bit 10          rg_wf_trx_lf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 11          rg_wf_trx_lf_ldo_fc_en_man     U     RW        default = 'h0
// Bit 16  :12     rg_wf_trx_lf_ldo_vsel          U     RW        default = 'h10
// Bit 17          rg_wf_trx_lf_ldo_rc_ctrl       U     RW        default = 'h1
// Bit 20  :18     rg_wf_top_dc_test_sel          U     RW        default = 'h0
// Bit 23  :21     rg_wf_top_ac_test_sel          U     RW        default = 'h0
// Bit 24          rg_wf_top_dc_test_en           U     RW        default = 'h0
// Bit 25          rg_wf_top_ac_test_en           U     RW        default = 'h0
// Bit 26          rg_wf_top_tx_test_en           U     RW        default = 'h0
// Bit 27          rg_wf_top_rx_test_en           U     RW        default = 'h0
// Bit 28          rg_wf_top_adc_test_en          U     RW        default = 'h0
// Bit 29          rg_wf_top_rcal_test_en         U     RW        default = 'h0
// Bit 30          rg_wf_top_xosc_test_en         U     RW        default = 'h0
// Bit 31          rg_wf_top_bbpll_test_en        U     RW        default = 'h0
typedef union RG_TOP_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TRX_HF_LDO_FC_SEL : 1;
    unsigned int RG_WF_TRX_HF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_WF_TRX_HF_LDO_FC_EN_MAN : 1;
    unsigned int RG_WF_TRX_HF_LDO_VSEL : 5;
    unsigned int RG_WF_TRX_HF_LDO_RC_CTRL : 1;
    unsigned int RG_WF_TRX_LF_LDO_FC_SEL : 1;
    unsigned int RG_WF_TRX_LF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_WF_TRX_LF_LDO_FC_EN_MAN : 1;
    unsigned int RG_WF_TRX_LF_LDO_VSEL : 5;
    unsigned int RG_WF_TRX_LF_LDO_RC_CTRL : 1;
    unsigned int RG_WF_TOP_DC_TEST_SEL : 3;
    unsigned int RG_WF_TOP_AC_TEST_SEL : 3;
    unsigned int RG_WF_TOP_DC_TEST_EN : 1;
    unsigned int RG_WF_TOP_AC_TEST_EN : 1;
    unsigned int RG_WF_TOP_TX_TEST_EN : 1;
    unsigned int RG_WF_TOP_RX_TEST_EN : 1;
    unsigned int RG_WF_TOP_ADC_TEST_EN : 1;
    unsigned int RG_WF_TOP_RCAL_TEST_EN : 1;
    unsigned int RG_WF_TOP_XOSC_TEST_EN : 1;
    unsigned int RG_WF_TOP_BBPLL_TEST_EN : 1;
  } b;
} RG_TOP_A3_FIELD_T;

#define RG_TOP_A4                                 (RF_D_TOP_REG_BASE + 0x10)
// Bit 15  :0      rg_wf_top_reserved0            U     RW        default = 'h0
// Bit 31  :16     rg_wf_top_reserved1            U     RW        default = 'hffff
typedef union RG_TOP_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TOP_RESERVED0 : 16;
    unsigned int RG_WF_TOP_RESERVED1 : 16;
  } b;
} RG_TOP_A4_FIELD_T;

#define RG_TOP_A5                                 (RF_D_TOP_REG_BASE + 0x14)
// Bit 0           rg_m0_wf_trx_hf_ldo_en         U     RW        default = 'h0
// Bit 1           rg_m0_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m0_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m0_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m0_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m0_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m0_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m0_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m0_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h0
// Bit 9           rg_m0_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h0
// Bit 10          rg_m0_wf_trx_lf_ldo_en         U     RW        default = 'h0
// Bit 11          rg_m0_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m0_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m0_wf_tx_hf_ldo_en          U     RW        default = 'h0
// Bit 14          rg_m0_wf_lo_hf_ldo_en          U     RW        default = 'h0
typedef union RG_TOP_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M0_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M0_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M0_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M0_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M0_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M0_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A5_FIELD_T;

#define RG_TOP_A6                                 (RF_D_TOP_REG_BASE + 0x18)
// Bit 0           rg_m1_wf_trx_hf_ldo_en         U     RW        default = 'h0
// Bit 1           rg_m1_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m1_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m1_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m1_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m1_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m1_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m1_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m1_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h0
// Bit 9           rg_m1_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h1
// Bit 10          rg_m1_wf_trx_lf_ldo_en         U     RW        default = 'h0
// Bit 11          rg_m1_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m1_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m1_wf_tx_hf_ldo_en          U     RW        default = 'h0
// Bit 14          rg_m1_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M1_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M1_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M1_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M1_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M1_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M1_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M1_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A6_FIELD_T;

#define RG_TOP_A7                                 (RF_D_TOP_REG_BASE + 0x1c)
// Bit 0           rg_m2_wf_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m2_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m2_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h1
// Bit 3           rg_m2_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m2_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m2_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m2_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m2_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m2_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h0
// Bit 9           rg_m2_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h1
// Bit 10          rg_m2_wf_trx_lf_ldo_en         U     RW        default = 'h1
// Bit 11          rg_m2_wf_trx_lf_ldo_tx_en      U     RW        default = 'h1
// Bit 12          rg_m2_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m2_wf_tx_hf_ldo_en          U     RW        default = 'h1
// Bit 14          rg_m2_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M2_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M2_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M2_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M2_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M2_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M2_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M2_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A7_FIELD_T;

#define RG_TOP_A8                                 (RF_D_TOP_REG_BASE + 0x20)
// Bit 0           rg_m3_wf_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m3_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m3_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m3_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h1
// Bit 4           rg_m3_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m3_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h1
// Bit 6           rg_m3_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m3_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h1
// Bit 8           rg_m3_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h0
// Bit 9           rg_m3_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h1
// Bit 10          rg_m3_wf_trx_lf_ldo_en         U     RW        default = 'h1
// Bit 11          rg_m3_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m3_wf_trx_lf_ldo_rx_en      U     RW        default = 'h1
// Bit 13          rg_m3_wf_tx_hf_ldo_en          U     RW        default = 'h1
// Bit 14          rg_m3_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M3_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M3_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M3_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M3_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M3_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M3_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M3_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A8_FIELD_T;

#define RG_TOP_A9                                 (RF_D_TOP_REG_BASE + 0x40)
// Bit 0           rg_wf_adda_ldo_lf_rc_ctrl      U     RW        default = 'h0
// Bit 1           rg_wf_adda_ldo_rc_modelsel     U     RW        default = 'h0
// Bit 6   :2      rg_wf_adda_ldo_vref_adj        U     RW        default = 'h10
// Bit 7           rg_wf_adda_ldo_load_en         U     RW        default = 'h0
// Bit 23  :8      rg_wf_adda_rsv                 U     RW        default = 'h0
// Bit 24          rg_wf_dac_bit9_inv             U     RW        default = 'h0
// Bit 30  :28     rg_wf_dac_clk_delay_i          U     RW        default = 'h1
typedef union RG_TOP_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_adda_ldo_lf_rc_ctrl : 1;
    unsigned int rg_wf_adda_ldo_rc_modelsel : 1;
    unsigned int rg_wf_adda_ldo_vref_adj : 5;
    unsigned int rg_wf_adda_ldo_load_en : 1;
    unsigned int rg_wf_adda_rsv : 16;
    unsigned int rg_wf_dac_bit9_inv : 1;
    unsigned int rsvd_0 : 3;
    unsigned int rg_wf_dac_clk_delay_i : 3;
    unsigned int rsvd_1 : 1;
  } b;
} RG_TOP_A9_FIELD_T;

#define RG_TOP_A11                                (RF_D_TOP_REG_BASE + 0x48)
// Bit 2   :0      rg_wf_dac_clk_delay_q          U     RW        default = 'h1
// Bit 6   :3      rg_wf_dac_dcoc_i               U     RW        default = 'h0
// Bit 10  :7      rg_wf_dac_dcoc_q               U     RW        default = 'h0
// Bit 11          rg_wf_dac_dem_en               U     RW        default = 'h0
// Bit 12          rg_wf_dac_driver_adj           U     RW        default = 'h1
// Bit 16  :13     rg_wf_dac_op_drive             U     RW        default = 'hf
// Bit 18  :17     rg_wf_dac_op_ibias             U     RW        default = 'h3
// Bit 22  :19     rg_wf_dac_ovcm                 U     RW        default = 'h0
// Bit 23          rg_wf_dac_fs_sel               U     RW        default = 'h1
// Bit 28  :24     rg_wf_dac_rc_adj_i             U     RW        default = 'h8
typedef union RG_TOP_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_dac_clk_delay_q : 3;
    unsigned int rg_wf_dac_dcoc_i : 4;
    unsigned int rg_wf_dac_dcoc_q : 4;
    unsigned int rg_wf_dac_dem_en : 1;
    unsigned int rg_wf_dac_driver_adj : 1;
    unsigned int rg_wf_dac_op_drive : 4;
    unsigned int rg_wf_dac_op_ibias : 2;
    unsigned int rg_wf_dac_ovcm : 4;
    unsigned int rg_wf_dac_fs_sel : 1;
    unsigned int rg_wf_dac_rc_adj_i : 5;
    unsigned int rsvd_0 : 3;
  } b;
} RG_TOP_A11_FIELD_T;

#define RG_TOP_A12                                (RF_D_TOP_REG_BASE + 0x4c)
// Bit 4   :0      rg_wf_dac_rc_adj_q             U     RW        default = 'h8
// Bit 12  :5      rg_wf_dac_rsv                  U     RW        default = 'h2b
// Bit 15  :13     rg_wf_dac_vimc_i               U     RW        default = 'h0
// Bit 18  :16     rg_wf_dac_vimc_q               U     RW        default = 'h0
// Bit 20  :19     rg_wf_dac_pole_sel             U     RW        default = 'h0
// Bit 21          rg_wf_dac_tia_mux_sel_i        U     RW        default = 'h0
// Bit 22          rg_wf_dac_tia_mux_sel_q        U     RW        default = 'h0
// Bit 24  :23     rg_wf_radc_bs_sel              U     RW        default = 'h1
// Bit 25          rg_wf_radc_c_adj               U     RW        default = 'h0
// Bit 28  :26     rg_wf_radc_clk_delay_adj       U     RW        default = 'h1
// Bit 29          rg_wf_radc_ref_adj             U     RW        default = 'h0
typedef union RG_TOP_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_dac_rc_adj_q : 5;
    unsigned int rg_wf_dac_rsv : 8;
    unsigned int rg_wf_dac_vimc_i : 3;
    unsigned int rg_wf_dac_vimc_q : 3;
    unsigned int rg_wf_dac_pole_sel : 2;
    unsigned int rg_wf_dac_tia_mux_sel_i : 1;
    unsigned int rg_wf_dac_tia_mux_sel_q : 1;
    unsigned int rg_wf_radc_bs_sel : 2;
    unsigned int rg_wf_radc_c_adj : 1;
    unsigned int rg_wf_radc_clk_delay_adj : 3;
    unsigned int rg_wf_radc_ref_adj : 1;
    unsigned int rsvd_0 : 2;
  } b;
} RG_TOP_A12_FIELD_T;

#define RG_TOP_A13                                (RF_D_TOP_REG_BASE + 0x50)
// Bit 7   :0      rg_wf_radc_resv                U     RW        default = 'h3
// Bit 8           rg_wf_radc_skip_sel            U     RW        default = 'h0
// Bit 10  :9      rg_wf_wadc_bs_sel              U     RW        default = 'h2
// Bit 11          rg_wf_wadc_ns_cdm              U     RW        default = 'h1
// Bit 12          rg_wf_wadc_ns_en               U     RW        default = 'h1
// Bit 15  :13     rg_wf_wadc_ns_gain_sel         U     RW        default = 'h0
// Bit 23  :16     rg_wf_wadc_rsv                 U     RW        default = 'h8
// Bit 25  :24     rg_wf_wadc_vref_iadj           U     RW        default = 'h2
typedef union RG_TOP_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_radc_resv : 8;
    unsigned int rg_wf_radc_skip_sel : 1;
    unsigned int rg_wf_wadc_bs_sel : 2;
    unsigned int rg_wf_wadc_ns_cdm : 1;
    unsigned int rg_wf_wadc_ns_en : 1;
    unsigned int rg_wf_wadc_ns_gain_sel : 3;
    unsigned int rg_wf_wadc_rsv : 8;
    unsigned int rg_wf_wadc_vref_iadj : 2;
    unsigned int rsvd_0 : 6;
  } b;
} RG_TOP_A13_FIELD_T;

#define RG_TOP_A14                                (RF_D_TOP_REG_BASE + 0x54)
// Bit 0           rg_wf_wadc_clk_phase           U     RW        default = 'h0
// Bit 2   :1      rg_wf_wadc_delay_adj           U     RW        default = 'h0
// Bit 3           rg_wf_wadc_div2                U     RW        default = 'h0
// Bit 8   :4      rg_wf_wadc_divn                U     RW        default = 'h3
// Bit 9           rg_wf_wadc_clkout_phase        U     RW        default = 'h0
// Bit 10          rg_wf_wadc_skip_sel            U     RW        default = 'h0
// Bit 11          rg_wf_adda_ldo_fc              U     RW        default = 'h0
typedef union RG_TOP_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_wadc_clk_phase : 1;
    unsigned int rg_wf_wadc_delay_adj : 2;
    unsigned int rg_wf_wadc_div2 : 1;
    unsigned int rg_wf_wadc_divn : 5;
    unsigned int rg_wf_wadc_clkout_phase : 1;
    unsigned int rg_wf_wadc_skip_sel : 1;
    unsigned int rg_wf_adda_ldo_fc : 1;
    unsigned int rsvd_0 : 20;
  } b;
} RG_TOP_A14_FIELD_T;

#define RG_TOP_A15                                (RF_D_TOP_REG_BASE + 0x58)
// Bit 0           rg_wf_adda_man_mode            U     RW        default = 'h0
// Bit 6   :4      rg_wf_dac_ampctrl_man          U     RW        default = 'h4
// Bit 12          rg_wf_tx_tia_ibias_en_h        U     RW        default = 'h1
// Bit 18  :16     rg_wf_tx_ibias_pata            U     RW        default = 'h0
// Bit 22  :20     rg_wf_tx_ibias_zt              U     RW        default = 'h0
// Bit 23          rg_wf_wadc_fs_adj_man          U     RW        default = 'h0
// Bit 24          rg_wf_wadc_fs_adj_man_mode     U     RW        default = 'h0
typedef union RG_TOP_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_adda_man_mode : 1;
    unsigned int rsvd_0 : 3;
    unsigned int rg_wf_dac_ampctrl_man : 3;
    unsigned int rsvd_1 : 5;
    unsigned int rg_wf_tx_tia_ibias_en_h : 1;
    unsigned int rsvd_2 : 3;
    unsigned int rg_wf_tx_ibias_pata : 3;
    unsigned int rsvd_3 : 1;
    unsigned int rg_wf_tx_ibias_zt : 3;
    unsigned int rg_wf_wadc_FS_adj_man : 1;
    unsigned int rg_wf_wadc_FS_adj_man_mode : 1;
    unsigned int rsvd_4 : 7;
  } b;
} RG_TOP_A15_FIELD_T;

#define RG_TOP_A16                                (RF_D_TOP_REG_BASE + 0x5c)
// Bit 4   :0      ro_wf_wadc_ch1_os_corr_i       U     RO        default = 'h0
// Bit 9   :5      ro_wf_wadc_ch1_os_corr_q       U     RO        default = 'h0
// Bit 14  :10     ro_wf_wadc_ch2_os_corr_i       U     RO        default = 'h0
// Bit 19  :15     ro_wf_wadc_ch2_os_corr_q       U     RO        default = 'h0
typedef union RG_TOP_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_wf_wadc_ch1_os_corr_i : 5;
    unsigned int ro_wf_wadc_ch1_os_corr_q : 5;
    unsigned int ro_wf_wadc_ch2_os_corr_i : 5;
    unsigned int ro_wf_wadc_ch2_os_corr_q : 5;
    unsigned int rsvd_0 : 12;
  } b;
} RG_TOP_A16_FIELD_T;

#define RG_TOP_A17                                (RF_D_TOP_REG_BASE + 0x60)
// Bit 0           rg_m0_wf_adda_ldo_en           U     RW        default = 'h0
// Bit 1           rg_m0_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m0_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m0_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m0_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m0_wf_adda_ldo_en : 1;
    unsigned int rg_m0_wf_dac_enable : 1;
    unsigned int rg_m0_wf_radc_en : 1;
    unsigned int rg_m0_wf_wadc_enable_i : 1;
    unsigned int rg_m0_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A17_FIELD_T;

#define RG_TOP_A18                                (RF_D_TOP_REG_BASE + 0x64)
// Bit 0           rg_m1_wf_adda_ldo_en           U     RW        default = 'h0
// Bit 1           rg_m1_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m1_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m1_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m1_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m1_wf_adda_ldo_en : 1;
    unsigned int rg_m1_wf_dac_enable : 1;
    unsigned int rg_m1_wf_radc_en : 1;
    unsigned int rg_m1_wf_wadc_enable_i : 1;
    unsigned int rg_m1_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A18_FIELD_T;

#define RG_TOP_A19                                (RF_D_TOP_REG_BASE + 0x68)
// Bit 0           rg_m2_wf_adda_ldo_en           U     RW        default = 'h1
// Bit 1           rg_m2_wf_dac_enable            U     RW        default = 'h1
// Bit 2           rg_m2_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m2_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m2_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m2_wf_adda_ldo_en : 1;
    unsigned int rg_m2_wf_dac_enable : 1;
    unsigned int rg_m2_wf_radc_en : 1;
    unsigned int rg_m2_wf_wadc_enable_i : 1;
    unsigned int rg_m2_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A19_FIELD_T;

#define RG_TOP_A20                                (RF_D_TOP_REG_BASE + 0x6c)
// Bit 0           rg_m3_wf_adda_ldo_en           U     RW        default = 'h1
// Bit 1           rg_m3_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m3_wf_radc_en               U     RW        default = 'h1
// Bit 3           rg_m3_wf_wadc_enable_i         U     RW        default = 'h1
// Bit 4           rg_m3_wf_wadc_enable_q         U     RW        default = 'h1
typedef union RG_TOP_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m3_wf_adda_ldo_en : 1;
    unsigned int rg_m3_wf_dac_enable : 1;
    unsigned int rg_m3_wf_radc_en : 1;
    unsigned int rg_m3_wf_wadc_enable_i : 1;
    unsigned int rg_m3_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A20_FIELD_T;

#define RG_TOP_A21                                (RF_D_TOP_REG_BASE + 0x70)
// Bit 0           rg_m4_wf_adda_ldo_en           U     RW        default = 'h0
// Bit 1           rg_m4_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m4_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m4_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m4_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m4_wf_adda_ldo_en : 1;
    unsigned int rg_m4_wf_dac_enable : 1;
    unsigned int rg_m4_wf_radc_en : 1;
    unsigned int rg_m4_wf_wadc_enable_i : 1;
    unsigned int rg_m4_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A21_FIELD_T;

#define RG_TOP_A22                                (RF_D_TOP_REG_BASE + 0x74)
// Bit 0           rg_m5_wf_adda_ldo_en           U     RW        default = 'h0
// Bit 1           rg_m5_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m5_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m5_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m5_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m5_wf_adda_ldo_en : 1;
    unsigned int rg_m5_wf_dac_enable : 1;
    unsigned int rg_m5_wf_radc_en : 1;
    unsigned int rg_m5_wf_wadc_enable_i : 1;
    unsigned int rg_m5_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A22_FIELD_T;

#define RG_TOP_A23                                (RF_D_TOP_REG_BASE + 0x78)
// Bit 0           rg_m6_wf_adda_ldo_en           U     RW        default = 'h1
// Bit 1           rg_m6_wf_dac_enable            U     RW        default = 'h1
// Bit 2           rg_m6_wf_radc_en               U     RW        default = 'h0
// Bit 3           rg_m6_wf_wadc_enable_i         U     RW        default = 'h0
// Bit 4           rg_m6_wf_wadc_enable_q         U     RW        default = 'h0
typedef union RG_TOP_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m6_wf_adda_ldo_en : 1;
    unsigned int rg_m6_wf_dac_enable : 1;
    unsigned int rg_m6_wf_radc_en : 1;
    unsigned int rg_m6_wf_wadc_enable_i : 1;
    unsigned int rg_m6_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A23_FIELD_T;

#define RG_TOP_A24                                (RF_D_TOP_REG_BASE + 0x7c)
// Bit 0           rg_m7_wf_adda_ldo_en           U     RW        default = 'h1
// Bit 1           rg_m7_wf_dac_enable            U     RW        default = 'h0
// Bit 2           rg_m7_wf_radc_en               U     RW        default = 'h1
// Bit 3           rg_m7_wf_wadc_enable_i         U     RW        default = 'h1
// Bit 4           rg_m7_wf_wadc_enable_q         U     RW        default = 'h1
typedef union RG_TOP_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_m7_wf_adda_ldo_en : 1;
    unsigned int rg_m7_wf_dac_enable : 1;
    unsigned int rg_m7_wf_radc_en : 1;
    unsigned int rg_m7_wf_wadc_enable_i : 1;
    unsigned int rg_m7_wf_wadc_enable_q : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_TOP_A24_FIELD_T;

#define RG_TOP_A27                                (RF_D_TOP_REG_BASE + 0x88)
// Bit 0           rg_m4_wf_trx_hf_ldo_en         U     RW        default = 'h0
// Bit 1           rg_m4_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m4_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m4_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m4_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m4_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m4_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m4_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m4_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h0
// Bit 9           rg_m4_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h0
// Bit 10          rg_m4_wf_trx_lf_ldo_en         U     RW        default = 'h0
// Bit 11          rg_m4_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m4_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m4_wf_tx_hf_ldo_en          U     RW        default = 'h0
// Bit 14          rg_m4_wf_lo_hf_ldo_en          U     RW        default = 'h0
typedef union RG_TOP_A27_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M4_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M4_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M4_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M4_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M4_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M4_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M4_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A27_FIELD_T;

#define RG_TOP_A28                                (RF_D_TOP_REG_BASE + 0x8c)
// Bit 0           rg_m5_wf_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m5_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m5_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m5_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m5_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m5_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m5_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m5_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m5_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h1
// Bit 9           rg_m5_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h0
// Bit 10          rg_m5_wf_trx_lf_ldo_en         U     RW        default = 'h0
// Bit 11          rg_m5_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m5_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m5_wf_tx_hf_ldo_en          U     RW        default = 'h0
// Bit 14          rg_m5_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A28_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M5_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M5_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M5_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M5_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M5_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M5_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M5_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A28_FIELD_T;

#define RG_TOP_A29                                (RF_D_TOP_REG_BASE + 0x90)
// Bit 0           rg_m6_wf_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m6_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h1
// Bit 2           rg_m6_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m6_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 4           rg_m6_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h0
// Bit 5           rg_m6_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m6_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h0
// Bit 7           rg_m6_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h0
// Bit 8           rg_m6_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h1
// Bit 9           rg_m6_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h0
// Bit 10          rg_m6_wf_trx_lf_ldo_en         U     RW        default = 'h1
// Bit 11          rg_m6_wf_trx_lf_ldo_tx_en      U     RW        default = 'h1
// Bit 12          rg_m6_wf_trx_lf_ldo_rx_en      U     RW        default = 'h0
// Bit 13          rg_m6_wf_tx_hf_ldo_en          U     RW        default = 'h1
// Bit 14          rg_m6_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A29_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M6_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M6_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M6_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M6_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M6_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M6_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M6_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A29_FIELD_T;

#define RG_TOP_A30                                (RF_D_TOP_REG_BASE + 0x94)
// Bit 0           rg_m7_wf_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m7_wf_trx_hf_ldo_wf2gtx_en     U     RW        default = 'h0
// Bit 2           rg_m7_wf_trx_hf_ldo_wf5gtx_en     U     RW        default = 'h0
// Bit 3           rg_m7_wf_trx_hf_ldo_rxb_en     U     RW        default = 'h1
// Bit 4           rg_m7_wf_trx_hf_ldo_wf2glna_en     U     RW        default = 'h1
// Bit 5           rg_m7_wf_trx_hf_ldo_wf5glna_en     U     RW        default = 'h0
// Bit 6           rg_m7_wf_trx_hf_ldo_wf2grx_en     U     RW        default = 'h1
// Bit 7           rg_m7_wf_trx_hf_ldo_wf5grx_en     U     RW        default = 'h1
// Bit 8           rg_m7_wf_trx_hf_ldo_wf2glo_en     U     RW        default = 'h1
// Bit 9           rg_m7_wf_trx_hf_ldo_wf5glo_en     U     RW        default = 'h0
// Bit 10          rg_m7_wf_trx_lf_ldo_en         U     RW        default = 'h1
// Bit 11          rg_m7_wf_trx_lf_ldo_tx_en      U     RW        default = 'h0
// Bit 12          rg_m7_wf_trx_lf_ldo_rx_en      U     RW        default = 'h1
// Bit 13          rg_m7_wf_tx_hf_ldo_en          U     RW        default = 'h1
// Bit 14          rg_m7_wf_lo_hf_ldo_en          U     RW        default = 'h1
typedef union RG_TOP_A30_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M7_WF_TRX_HF_LDO_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF2GTX_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF5GTX_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF2GLNA_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF5GLNA_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF2GRX_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF5GRX_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF2GLO_EN : 1;
    unsigned int RG_M7_WF_TRX_HF_LDO_WF5GLO_EN : 1;
    unsigned int RG_M7_WF_TRX_LF_LDO_EN : 1;
    unsigned int RG_M7_WF_TRX_LF_LDO_TX_EN : 1;
    unsigned int RG_M7_WF_TRX_LF_LDO_RX_EN : 1;
    unsigned int RG_M7_WF_TX_HF_LDO_EN : 1;
    unsigned int RG_M7_WF_LO_HF_LDO_EN : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_TOP_A30_FIELD_T;

#define RG_TOP_A31                                (RF_D_TOP_REG_BASE + 0x98)
// Bit 31  :0      rg_wf_ad_inv                   U     RW        default = 'h0
typedef union RG_TOP_A31_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_AD_INV : 32;
  } b;
} RG_TOP_A31_FIELD_T;

#define RG_TOP_A32                                (RF_D_TOP_REG_BASE + 0x9c)
// Bit 23  :0      ro_wf_top_ad_monitor           U     RO        default = 'h0
// Bit 24          ro_wf5g_sx_dcc2_cnt_finish     U     RO        default = 'h0
typedef union RG_TOP_A32_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_TOP_AD_MONITOR : 24;
    unsigned int RO_WF5G_SX_DCC2_CNT_FINISH : 1;
    unsigned int rsvd_0 : 7;
  } b;
} RG_TOP_A32_FIELD_T;

#define RG_TOP_A33                                (RF_D_TOP_REG_BASE + 0xa0)
// Bit 1   :0      rg_wf_trx_ldo_fc_cnt           U     RW        default = 'h0
typedef union RG_TOP_A33_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TRX_LDO_FC_CNT : 2;
    unsigned int rsvd_0 : 30;
  } b;
} RG_TOP_A33_FIELD_T;

#define RG_TOP_A34                                (RF_D_TOP_REG_BASE + 0xa4)
// Bit 1   :0      ro_hw_wf_trx_mode              U     RO        default = 'h0
// Bit 12  :4      ro_hw_wf_rx_gain               U     RO        default = 'h0
// Bit 15  :14     ro_hw_wf_bw                    U     RO        default = 'h0
// Bit 24  :16     ro_hw_wf_tx_gain               U     RO        default = 'h0
// Bit 28          ro_hw_wf_agc_lock              U     RO        default = 'h0
typedef union RG_TOP_A34_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_HW_WF_TRX_MODE : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RO_HW_WF_RX_GAIN : 9;
    unsigned int rsvd_1 : 1;
    unsigned int RO_HW_WF_BW : 2;
    unsigned int RO_HW_WF_TX_GAIN : 9;
    unsigned int rsvd_2 : 3;
    unsigned int RO_HW_WF_AGC_LOCK : 1;
    unsigned int rsvd_3 : 3;
  } b;
} RG_TOP_A34_FIELD_T;

#define RG_TOP_A35                                (RF_D_TOP_REG_BASE + 0xa8)
// Bit 0           rg_wf_ttgdrv_en                U     RW        default = 'h0
// Bit 1           rg_wf_ttgbuf1_en               U     RW        default = 'h0
// Bit 2           rg_wf_ttgbuf1_rep_en           U     RW        default = 'h0
// Bit 3           rg_wf_ttgbuf2_en               U     RW        default = 'h0
// Bit 4           rg_wf_ttgbuf2_rep_en           U     RW        default = 'h0
// Bit 5           rg_wf_ttgbuf2_wf2grxcal_en     U     RW        default = 'h0
// Bit 6           rg_wf_ttgbuf2_wf2gdpdcal_en     U     RW        default = 'h0
// Bit 7           rg_wf_ttgbuf3_en               U     RW        default = 'h0
// Bit 8           rg_wf_ttgbuf3_rep_en           U     RW        default = 'h0
// Bit 9           rg_wf_ttgbuf3_wf5grxcal_en     U     RW        default = 'h0
// Bit 10          rg_wf_ttgbuf3_wf5gdpdcal_en     U     RW        default = 'h0
// Bit 23  :16     rg_wf_ttgbuf_reserved1         U     RW        default = 'h0
typedef union RG_TOP_A35_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TTGDRV_EN : 1;
    unsigned int RG_WF_TTGBUF1_EN : 1;
    unsigned int RG_WF_TTGBUF1_REP_EN : 1;
    unsigned int RG_WF_TTGBUF2_EN : 1;
    unsigned int RG_WF_TTGBUF2_REP_EN : 1;
    unsigned int RG_WF_TTGBUF2_WF2GRXCAL_EN : 1;
    unsigned int RG_WF_TTGBUF2_WF2GDPDCAL_EN : 1;
    unsigned int RG_WF_TTGBUF3_EN : 1;
    unsigned int RG_WF_TTGBUF3_REP_EN : 1;
    unsigned int RG_WF_TTGBUF3_WF5GRXCAL_EN : 1;
    unsigned int RG_WF_TTGBUF3_WF5GDPDCAL_EN : 1;
    unsigned int rsvd_0 : 5;
    unsigned int RG_WF_TTGBUF_RESERVED1 : 8;
    unsigned int rsvd_1 : 8;
  } b;
} RG_TOP_A35_FIELD_T;

#define RG_TOP_A36                                (RF_D_TOP_REG_BASE + 0xac)
// Bit 4   :0      rg_wf_ttgdrv_vddr_sel          U     RW        default = 'h8
// Bit 7   :5      rg_wf_ttgbuf1_fbr_sel          U     RW        default = 'h6
// Bit 12  :8      rg_wf_ttgbuf1_vddr_sel         U     RW        default = 'h8
// Bit 15  :13     rg_wf_ttgbuf2_fbr_sel          U     RW        default = 'h6
// Bit 20  :16     rg_wf_ttgbuf2_vddr_sel         U     RW        default = 'h8
// Bit 23  :21     rg_wf_ttgbuf3_fbr_sel          U     RW        default = 'h6
// Bit 28  :24     rg_wf_ttgbuf3_vddr_sel         U     RW        default = 'h8
// Bit 31  :29     rg_wf_ttgbuf_reserved2         U     RW        default = 'h6
typedef union RG_TOP_A36_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TTGDRV_VDDR_SEL : 5;
    unsigned int RG_WF_TTGBUF1_FBR_SEL : 3;
    unsigned int RG_WF_TTGBUF1_VDDR_SEL : 5;
    unsigned int RG_WF_TTGBUF2_FBR_SEL : 3;
    unsigned int RG_WF_TTGBUF2_VDDR_SEL : 5;
    unsigned int RG_WF_TTGBUF3_FBR_SEL : 3;
    unsigned int RG_WF_TTGBUF3_VDDR_SEL : 5;
    unsigned int RG_WF_TTGBUF_RESERVED2 : 3;
  } b;
} RG_TOP_A36_FIELD_T;

#define RG_TOP_A37                                (RF_D_TOP_REG_BASE + 0xb0)
// Bit 0           rg_wf_tx_hf_ldo_fc_sel         U     RW        default = 'h0
// Bit 1           rg_wf_tx_hf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 2           rg_wf_tx_hf_ldo_fc_en_man      U     RW        default = 'h0
// Bit 7   :3      rg_wf_tx_hf_ldo_vsel           U     RW        default = 'h10
// Bit 8           rg_wf_tx_hf_ldo_rc_ctrl        U     RW        default = 'h1
// Bit 9           rg_wf_lo_hf_ldo_fc_sel         U     RW        default = 'h0
// Bit 10          rg_wf_lo_hf_ldo_fc_en_man_mode     U     RW        default = 'h0
// Bit 11          rg_wf_lo_hf_ldo_fc_en_man      U     RW        default = 'h0
// Bit 16  :12     rg_wf_lo_hf_ldo_vsel           U     RW        default = 'h10
// Bit 17          rg_wf_lo_hf_ldo_rc_ctrl        U     RW        default = 'h1
typedef union RG_TOP_A37_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TX_HF_LDO_FC_SEL : 1;
    unsigned int RG_WF_TX_HF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_WF_TX_HF_LDO_FC_EN_MAN : 1;
    unsigned int RG_WF_TX_HF_LDO_VSEL : 5;
    unsigned int RG_WF_TX_HF_LDO_RC_CTRL : 1;
    unsigned int RG_WF_LO_HF_LDO_FC_SEL : 1;
    unsigned int RG_WF_LO_HF_LDO_FC_EN_MAN_MODE : 1;
    unsigned int RG_WF_LO_HF_LDO_FC_EN_MAN : 1;
    unsigned int RG_WF_LO_HF_LDO_VSEL : 5;
    unsigned int RG_WF_LO_HF_LDO_RC_CTRL : 1;
    unsigned int rsvd_0 : 14;
  } b;
} RG_TOP_A37_FIELD_T;

#define RG_TOP_A38                                (RF_D_TOP_REG_BASE + 0xb4)
// Bit 3   :0      ro_code_m1                     U     RO        default = 'h0
// Bit 15  :8      ro_code_m2                     U     RO        default = 'h0
// Bit 23  :16     ro_code_m3                     U     RO        default = 'h0
// Bit 31  :24     ro_code_m4                     U     RO        default = 'h0
typedef union RG_TOP_A38_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_code_m1 : 4;
    unsigned int rsvd_0 : 4;
    unsigned int ro_code_m2 : 8;
    unsigned int ro_code_m3 : 8;
    unsigned int ro_code_m4 : 8;
  } b;
} RG_TOP_A38_FIELD_T;

#define RG_TOP_A39                                (RF_D_TOP_REG_BASE + 0xb8)
// Bit 7   :0      ro_code_m5                     U     RO        default = 'h0
// Bit 15  :8      ro_code_m6                     U     RO        default = 'h0
// Bit 23  :16     ro_code_m7                     U     RO        default = 'h0
// Bit 31  :24     ro_code_m8                     U     RO        default = 'h0
typedef union RG_TOP_A39_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_code_m5 : 8;
    unsigned int ro_code_m6 : 8;
    unsigned int ro_code_m7 : 8;
    unsigned int ro_code_m8 : 8;
  } b;
} RG_TOP_A39_FIELD_T;

#define RG_TOP_A40                                (RF_D_TOP_REG_BASE + 0xbc)
// Bit 7   :0      ro_code_ap                     U     RO        default = 'h0
typedef union RG_TOP_A40_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_code_ap : 8;
    unsigned int rsvd_0 : 24;
  } b;
} RG_TOP_A40_FIELD_T;

#endif

