#ifdef RF_D_BT_RX_REG
#else
#define RF_D_BT_RX_REG


#define RF_D_BT_RX_REG_BASE                       (0x80002300)

#define RG_BT_RX_A0                                  (0x0)
// Bit 31  :0      rg_wf_rx_dig_rsv0              U     RW        default = 'hffff
typedef union RG_BT_RX_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_RX_DIG_RSV0 : 32;
  } b;
} RG_BT_RX_A0_FIELD_T;

#define RG_BT_RX_A1                                  (0x4)
// Bit 31  :0      rg_wf_rx_dig_rsv1              U     RW        default = 'hffff
typedef union RG_BT_RX_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_RX_DIG_RSV1 : 32;
  } b;
} RG_BT_RX_A1_FIELD_T;

#define RG_BT_RX_A2                                  (0x8)
// Bit 0           rg_btrx_abb_bw_man_mode        U     RW        default = 'h0
// Bit 1           rg_btrx_abb_bw_man             U     RW        default = 'h0
// Bit 15  :8      rg_btrx_gain_man               U     RW        default = 'hff
// Bit 16          rg_btrx_gain_man_mode          U     RW        default = 'h0
// Bit 23  :20     rg_btrx_osc_freq               U     RW        default = 'h5
// Bit 31  :24     rg_btrx_reserved1              U     RW        default = 'h0
typedef union RG_BT_RX_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_ABB_BW_MAN_MODE : 1;
    unsigned int RG_BTRX_ABB_BW_MAN : 1;
    unsigned int rsvd_0 : 6;
    unsigned int RG_BTRX_GAIN_MAN : 8;
    unsigned int RG_BTRX_GAIN_MAN_MODE : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_BTRX_OSC_FREQ : 4;
    unsigned int RG_BTRX_RESERVED1 : 8;
  } b;
} RG_BT_RX_A2_FIELD_T;

#define RG_BT_RX_A3                                  (0xc)
// Bit 4   :0      rg_btrx_gm_ctl_g1              U     RW        default = 'h1
// Bit 9   :5      rg_btrx_gm_ctl_g2              U     RW        default = 'h2
// Bit 19  :15     rg_btrx_gm_ctl_g3              U     RW        default = 'h3
// Bit 24  :20     rg_btrx_gm_ctl_g4              U     RW        default = 'h10
// Bit 29  :25     rg_btrx_gm_ctl_g5              U     RW        default = 'h14
typedef union RG_BT_RX_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_GM_CTL_G1 : 5;
    unsigned int RG_BTRX_GM_CTL_G2 : 5;
    unsigned int rsvd_0 : 5;
    unsigned int RG_BTRX_GM_CTL_G3 : 5;
    unsigned int RG_BTRX_GM_CTL_G4 : 5;
    unsigned int RG_BTRX_GM_CTL_G5 : 5;
    unsigned int rsvd_1 : 2;
  } b;
} RG_BT_RX_A3_FIELD_T;

#define RG_BT_RX_A4                                  (0x10)
// Bit 4   :0      rg_btrx_gm_ctl_g6              U     RW        default = 'h18
// Bit 9   :5      rg_btrx_gm_ctl_g7              U     RW        default = 'h1e
// Bit 19  :15     rg_btrx_gm_ctl_man             U     RW        default = 'h1e
// Bit 20          rg_btrx_gm_ctl_man_mode        U     RW        default = 'h0
// Bit 31  :24     rg_bt_rx_reserved2             U     RW        default = 'h0
typedef union RG_BT_RX_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_GM_CTL_G6 : 5;
    unsigned int RG_BTRX_GM_CTL_G7 : 5;
    unsigned int rsvd_0 : 5;
    unsigned int RG_BTRX_GM_CTL_MAN : 5;
    unsigned int RG_BTRX_GM_CTL_MAN_MODE : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_BT_RX_RESERVED2 : 8;
  } b;
} RG_BT_RX_A4_FIELD_T;

#define RG_BT_RX_A5                                  (0x14)
// Bit 3   :0      rg_btrx_gm_ictrl_g123          U     RW        default = 'h9
// Bit 7   :4      rg_btrx_gm_ictrl_g45           U     RW        default = 'h1
// Bit 11  :8      rg_btrx_gm_ictrl_g67           U     RW        default = 'h1
// Bit 15  :12     rg_btrx_gm_ictrl_man           U     RW        default = 'h9
// Bit 16          rg_btrx_gm_ictrl_man_mode      U     RW        default = 'h0
// Bit 31  :20     rg_bt_rx_reserved3             U     RW        default = 'h0
typedef union RG_BT_RX_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_GM_ICTRL_G123 : 4;
    unsigned int RG_BTRX_GM_ICTRL_G45 : 4;
    unsigned int RG_BTRX_GM_ICTRL_G67 : 4;
    unsigned int RG_BTRX_GM_ICTRL_MAN : 4;
    unsigned int RG_BTRX_GM_ICTRL_MAN_MODE : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_BT_RX_RESERVED3 : 12;
  } b;
} RG_BT_RX_A5_FIELD_T;

#define RG_BT_RX_A6                                  (0x18)
// Bit 5   :0      rg_btrx_cbpf_gm_g1             U     RW        default = 'h5
// Bit 11  :6      rg_btrx_cbpf_gm_g2             U     RW        default = 'h7
// Bit 17  :12     rg_btrx_cbpf_gm_g3             U     RW        default = 'hf
// Bit 23  :18     rg_btrx_cbpf_gm_g4             U     RW        default = 'h2f
// Bit 29  :24     rg_btrx_cbpf_gm_man            U     RW        default = 'h2f
// Bit 30          rg_btrx_cbpf_gm_man_mode       U     RW        default = 'h0
typedef union RG_BT_RX_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_CBPF_GM_G1 : 6;
    unsigned int RG_BTRX_CBPF_GM_G2 : 6;
    unsigned int RG_BTRX_CBPF_GM_G3 : 6;
    unsigned int RG_BTRX_CBPF_GM_G4 : 6;
    unsigned int RG_BTRX_CBPF_GM_MAN : 6;
    unsigned int RG_BTRX_CBPF_GM_MAN_MODE : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_BT_RX_A6_FIELD_T;

#define RG_BT_RX_A7                                  (0x1c)
// Bit 0           rg_m0_bt_trx_hf_ldo_en         U     RW        default = 'h0
// Bit 1           rg_m0_bt_trx_hf_ldo_rx1_en     U     RW        default = 'h0
// Bit 2           rg_m0_bt_trx_hf_ldo_rx2_en     U     RW        default = 'h0
// Bit 3           rg_m0_bt_trx_hf_ldo_rx3_en     U     RW        default = 'h0
// Bit 4           rg_m0_bt_trx_hf_ldo_tx_en      U     RW        default = 'h0
// Bit 5           rg_m0_bt_trx_hf_ldo_lo_en      U     RW        default = 'h0
// Bit 6           rg_m0_btrx_fe_topbias_en       U     RW        default = 'h0
// Bit 7           rg_m0_btrx_lna_bias_en         U     RW        default = 'h0
// Bit 8           rg_m0_btrx_gm_bias_en          U     RW        default = 'h0
// Bit 9           rg_m0_btrx_mxr_bias_en         U     RW        default = 'h0
// Bit 10          rg_m0_bt_trsw_en               U     RW        default = 'h0
// Bit 11          rg_m0_btrx_lna_en              U     RW        default = 'h0
// Bit 12          rg_m0_btrx_gm_en               U     RW        default = 'h0
// Bit 13          rg_m0_btrx_mxr_en              U     RW        default = 'h0
// Bit 14          rg_m0_btrx_mxr_lo_en           U     RW        default = 'h0
// Bit 15          rg_m0_btrx_abb_bias_en         U     RW        default = 'h0
// Bit 16          rg_m0_btrx_tia_en              U     RW        default = 'h0
// Bit 17          rg_m0_btrx_cbpf_en             U     RW        default = 'h0
// Bit 18          rg_m0_btrx_lpbk_en             U     RW        default = 'h0
// Bit 20          rg_m0_btrx_cbpf_cross_r        U     RW        default = 'h0
// Bit 21          rg_m0_btrx_pdet_en             U     RW        default = 'h0
// Bit 22          rg_m0_wfbt_trx_hf_ldo_en       U     RW        default = 'h0
// Bit 23          rg_m0_wfbt_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 24          rg_m0_wfbt_rx_lna_bias_en      U     RW        default = 'h0
// Bit 25          rg_m0_wfbt_rx_lna_en           U     RW        default = 'h0
// Bit 26          rg_m0_wfbt_rx_extlna_en        U     RW        default = 'h0
// Bit 27          rg_m0_wfbt_rx_cal_en           U     RW        default = 'h0
typedef union RG_BT_RX_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_BT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M0_BT_TRX_HF_LDO_RX1_EN : 1;
    unsigned int RG_M0_BT_TRX_HF_LDO_RX2_EN : 1;
    unsigned int RG_M0_BT_TRX_HF_LDO_RX3_EN : 1;
    unsigned int RG_M0_BT_TRX_HF_LDO_TX_EN : 1;
    unsigned int RG_M0_BT_TRX_HF_LDO_LO_EN : 1;
    unsigned int RG_M0_BTRX_FE_TOPBIAS_EN : 1;
    unsigned int RG_M0_BTRX_LNA_BIAS_EN : 1;
    unsigned int RG_M0_BTRX_GM_BIAS_EN : 1;
    unsigned int RG_M0_BTRX_MXR_BIAS_EN : 1;
    unsigned int RG_M0_BT_TRSW_EN : 1;
    unsigned int RG_M0_BTRX_LNA_EN : 1;
    unsigned int RG_M0_BTRX_GM_EN : 1;
    unsigned int RG_M0_BTRX_MXR_EN : 1;
    unsigned int RG_M0_BTRX_MXR_LO_EN : 1;
    unsigned int RG_M0_BTRX_ABB_BIAS_EN : 1;
    unsigned int RG_M0_BTRX_TIA_EN : 1;
    unsigned int RG_M0_BTRX_CBPF_EN : 1;
    unsigned int RG_M0_BTRX_LPBK_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_M0_BTRX_CBPF_CROSS_R : 1;
    unsigned int RG_M0_BTRX_PDET_EN : 1;
    unsigned int RG_M0_WFBT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M0_WFBT_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M0_WFBT_RX_LNA_BIAS_EN : 1;
    unsigned int RG_M0_WFBT_RX_LNA_EN : 1;
    unsigned int RG_M0_WFBT_RX_EXTLNA_EN : 1;
    unsigned int RG_M0_WFBT_RX_CAL_EN : 1;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BT_RX_A7_FIELD_T;

#define RG_BT_RX_A8                                  (0x20)
// Bit 0           rg_m1_bt_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m1_bt_trx_hf_ldo_rx1_en     U     RW        default = 'h0
// Bit 2           rg_m1_bt_trx_hf_ldo_rx2_en     U     RW        default = 'h0
// Bit 3           rg_m1_bt_trx_hf_ldo_rx3_en     U     RW        default = 'h0
// Bit 4           rg_m1_bt_trx_hf_ldo_tx_en      U     RW        default = 'h0
// Bit 5           rg_m1_bt_trx_hf_ldo_lo_en      U     RW        default = 'h1
// Bit 6           rg_m1_btrx_fe_topbias_en       U     RW        default = 'h0
// Bit 7           rg_m1_btrx_lna_bias_en         U     RW        default = 'h0
// Bit 8           rg_m1_btrx_gm_bias_en          U     RW        default = 'h0
// Bit 9           rg_m1_btrx_mxr_bias_en         U     RW        default = 'h0
// Bit 10          rg_m1_bt_trsw_en               U     RW        default = 'h0
// Bit 11          rg_m1_btrx_lna_en              U     RW        default = 'h0
// Bit 12          rg_m1_btrx_gm_en               U     RW        default = 'h0
// Bit 13          rg_m1_btrx_mxr_en              U     RW        default = 'h0
// Bit 14          rg_m1_btrx_mxr_lo_en           U     RW        default = 'h0
// Bit 15          rg_m1_btrx_abb_bias_en         U     RW        default = 'h0
// Bit 16          rg_m1_btrx_tia_en              U     RW        default = 'h0
// Bit 17          rg_m1_btrx_cbpf_en             U     RW        default = 'h0
// Bit 18          rg_m1_btrx_lpbk_en             U     RW        default = 'h0
// Bit 20          rg_m1_btrx_cbpf_cross_r        U     RW        default = 'h0
// Bit 21          rg_m1_btrx_pdet_en             U     RW        default = 'h0
// Bit 22          rg_m1_wfbt_trx_hf_ldo_en       U     RW        default = 'h0
// Bit 23          rg_m1_wfbt_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 24          rg_m1_wfbt_rx_lna_bias_en      U     RW        default = 'h0
// Bit 25          rg_m1_wfbt_rx_lna_en           U     RW        default = 'h0
// Bit 26          rg_m1_wfbt_rx_extlna_en        U     RW        default = 'h0
// Bit 27          rg_m1_wfbt_rx_cal_en           U     RW        default = 'h0
typedef union RG_BT_RX_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M1_BT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M1_BT_TRX_HF_LDO_RX1_EN : 1;
    unsigned int RG_M1_BT_TRX_HF_LDO_RX2_EN : 1;
    unsigned int RG_M1_BT_TRX_HF_LDO_RX3_EN : 1;
    unsigned int RG_M1_BT_TRX_HF_LDO_TX_EN : 1;
    unsigned int RG_M1_BT_TRX_HF_LDO_LO_EN : 1;
    unsigned int RG_M1_BTRX_FE_TOPBIAS_EN : 1;
    unsigned int RG_M1_BTRX_LNA_BIAS_EN : 1;
    unsigned int RG_M1_BTRX_GM_BIAS_EN : 1;
    unsigned int RG_M1_BTRX_MXR_BIAS_EN : 1;
    unsigned int RG_M1_BT_TRSW_EN : 1;
    unsigned int RG_M1_BTRX_LNA_EN : 1;
    unsigned int RG_M1_BTRX_GM_EN : 1;
    unsigned int RG_M1_BTRX_MXR_EN : 1;
    unsigned int RG_M1_BTRX_MXR_LO_EN : 1;
    unsigned int RG_M1_BTRX_ABB_BIAS_EN : 1;
    unsigned int RG_M1_BTRX_TIA_EN : 1;
    unsigned int RG_M1_BTRX_CBPF_EN : 1;
    unsigned int RG_M1_BTRX_LPBK_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_M1_BTRX_CBPF_CROSS_R : 1;
    unsigned int RG_M1_BTRX_PDET_EN : 1;
    unsigned int RG_M1_WFBT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M1_WFBT_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M1_WFBT_RX_LNA_BIAS_EN : 1;
    unsigned int RG_M1_WFBT_RX_LNA_EN : 1;
    unsigned int RG_M1_WFBT_RX_EXTLNA_EN : 1;
    unsigned int RG_M1_WFBT_RX_CAL_EN : 1;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BT_RX_A8_FIELD_T;

#define RG_BT_RX_A9                                  (0x24)
// Bit 0           rg_m2_bt_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m2_bt_trx_hf_ldo_rx1_en     U     RW        default = 'h0
// Bit 2           rg_m2_bt_trx_hf_ldo_rx2_en     U     RW        default = 'h0
// Bit 3           rg_m2_bt_trx_hf_ldo_rx3_en     U     RW        default = 'h0
// Bit 4           rg_m2_bt_trx_hf_ldo_tx_en      U     RW        default = 'h1
// Bit 5           rg_m2_bt_trx_hf_ldo_lo_en      U     RW        default = 'h1
// Bit 6           rg_m2_btrx_fe_topbias_en       U     RW        default = 'h1
// Bit 7           rg_m2_btrx_lna_bias_en         U     RW        default = 'h0
// Bit 8           rg_m2_btrx_gm_bias_en          U     RW        default = 'h0
// Bit 9           rg_m2_btrx_mxr_bias_en         U     RW        default = 'h0
// Bit 10          rg_m2_bt_trsw_en               U     RW        default = 'h1
// Bit 11          rg_m2_btrx_lna_en              U     RW        default = 'h0
// Bit 12          rg_m2_btrx_gm_en               U     RW        default = 'h0
// Bit 13          rg_m2_btrx_mxr_en              U     RW        default = 'h0
// Bit 14          rg_m2_btrx_mxr_lo_en           U     RW        default = 'h0
// Bit 15          rg_m2_btrx_abb_bias_en         U     RW        default = 'h0
// Bit 16          rg_m2_btrx_tia_en              U     RW        default = 'h0
// Bit 17          rg_m2_btrx_cbpf_en             U     RW        default = 'h0
// Bit 18          rg_m2_btrx_lpbk_en             U     RW        default = 'h0
// Bit 20          rg_m2_btrx_cbpf_cross_r        U     RW        default = 'h0
// Bit 21          rg_m2_btrx_pdet_en             U     RW        default = 'h0
// Bit 22          rg_m2_wfbt_trx_hf_ldo_en       U     RW        default = 'h0
// Bit 23          rg_m2_wfbt_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 24          rg_m2_wfbt_rx_lna_bias_en      U     RW        default = 'h0
// Bit 25          rg_m2_wfbt_rx_lna_en           U     RW        default = 'h0
// Bit 26          rg_m2_wfbt_rx_extlna_en        U     RW        default = 'h0
// Bit 27          rg_m2_wfbt_rx_cal_en           U     RW        default = 'h0
typedef union RG_BT_RX_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M2_BT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M2_BT_TRX_HF_LDO_RX1_EN : 1;
    unsigned int RG_M2_BT_TRX_HF_LDO_RX2_EN : 1;
    unsigned int RG_M2_BT_TRX_HF_LDO_RX3_EN : 1;
    unsigned int RG_M2_BT_TRX_HF_LDO_TX_EN : 1;
    unsigned int RG_M2_BT_TRX_HF_LDO_LO_EN : 1;
    unsigned int RG_M2_BTRX_FE_TOPBIAS_EN : 1;
    unsigned int RG_M2_BTRX_LNA_BIAS_EN : 1;
    unsigned int RG_M2_BTRX_GM_BIAS_EN : 1;
    unsigned int RG_M2_BTRX_MXR_BIAS_EN : 1;
    unsigned int RG_M2_BT_TRSW_EN : 1;
    unsigned int RG_M2_BTRX_LNA_EN : 1;
    unsigned int RG_M2_BTRX_GM_EN : 1;
    unsigned int RG_M2_BTRX_MXR_EN : 1;
    unsigned int RG_M2_BTRX_MXR_LO_EN : 1;
    unsigned int RG_M2_BTRX_ABB_BIAS_EN : 1;
    unsigned int RG_M2_BTRX_TIA_EN : 1;
    unsigned int RG_M2_BTRX_CBPF_EN : 1;
    unsigned int RG_M2_BTRX_LPBK_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_M2_BTRX_CBPF_CROSS_R : 1;
    unsigned int RG_M2_BTRX_PDET_EN : 1;
    unsigned int RG_M2_WFBT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M2_WFBT_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M2_WFBT_RX_LNA_BIAS_EN : 1;
    unsigned int RG_M2_WFBT_RX_LNA_EN : 1;
    unsigned int RG_M2_WFBT_RX_EXTLNA_EN : 1;
    unsigned int RG_M2_WFBT_RX_CAL_EN : 1;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BT_RX_A9_FIELD_T;

#define RG_BT_RX_A10                                 (0x28)
// Bit 0           rg_m3_bt_trx_hf_ldo_en         U     RW        default = 'h1
// Bit 1           rg_m3_bt_trx_hf_ldo_rx1_en     U     RW        default = 'h1
// Bit 2           rg_m3_bt_trx_hf_ldo_rx2_en     U     RW        default = 'h1
// Bit 3           rg_m3_bt_trx_hf_ldo_rx3_en     U     RW        default = 'h1
// Bit 4           rg_m3_bt_trx_hf_ldo_tx_en      U     RW        default = 'h0
// Bit 5           rg_m3_bt_trx_hf_ldo_lo_en      U     RW        default = 'h1
// Bit 6           rg_m3_btrx_fe_topbias_en       U     RW        default = 'h1
// Bit 7           rg_m3_btrx_lna_bias_en         U     RW        default = 'h1
// Bit 8           rg_m3_btrx_gm_bias_en          U     RW        default = 'h1
// Bit 9           rg_m3_btrx_mxr_bias_en         U     RW        default = 'h1
// Bit 10          rg_m3_bt_trsw_en               U     RW        default = 'h0
// Bit 11          rg_m3_btrx_lna_en              U     RW        default = 'h1
// Bit 12          rg_m3_btrx_gm_en               U     RW        default = 'h1
// Bit 13          rg_m3_btrx_mxr_en              U     RW        default = 'h1
// Bit 14          rg_m3_btrx_mxr_lo_en           U     RW        default = 'h1
// Bit 15          rg_m3_btrx_abb_bias_en         U     RW        default = 'h1
// Bit 16          rg_m3_btrx_tia_en              U     RW        default = 'h1
// Bit 17          rg_m3_btrx_cbpf_en             U     RW        default = 'h1
// Bit 18          rg_m3_btrx_lpbk_en             U     RW        default = 'h0
// Bit 20          rg_m3_btrx_cbpf_cross_r        U     RW        default = 'h1
// Bit 21          rg_m3_btrx_pdet_en             U     RW        default = 'h1
// Bit 22          rg_m3_wfbt_trx_hf_ldo_en       U     RW        default = 'h0
// Bit 23          rg_m3_wfbt_trx_hf_ldo_rxb_en     U     RW        default = 'h0
// Bit 24          rg_m3_wfbt_rx_lna_bias_en      U     RW        default = 'h0
// Bit 25          rg_m3_wfbt_rx_lna_en           U     RW        default = 'h0
// Bit 26          rg_m3_wfbt_rx_extlna_en        U     RW        default = 'h0
// Bit 27          rg_m3_wfbt_rx_cal_en           U     RW        default = 'h0
typedef union RG_BT_RX_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M3_BT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M3_BT_TRX_HF_LDO_RX1_EN : 1;
    unsigned int RG_M3_BT_TRX_HF_LDO_RX2_EN : 1;
    unsigned int RG_M3_BT_TRX_HF_LDO_RX3_EN : 1;
    unsigned int RG_M3_BT_TRX_HF_LDO_TX_EN : 1;
    unsigned int RG_M3_BT_TRX_HF_LDO_LO_EN : 1;
    unsigned int RG_M3_BTRX_FE_TOPBIAS_EN : 1;
    unsigned int RG_M3_BTRX_LNA_BIAS_EN : 1;
    unsigned int RG_M3_BTRX_GM_BIAS_EN : 1;
    unsigned int RG_M3_BTRX_MXR_BIAS_EN : 1;
    unsigned int RG_M3_BT_TRSW_EN : 1;
    unsigned int RG_M3_BTRX_LNA_EN : 1;
    unsigned int RG_M3_BTRX_GM_EN : 1;
    unsigned int RG_M3_BTRX_MXR_EN : 1;
    unsigned int RG_M3_BTRX_MXR_LO_EN : 1;
    unsigned int RG_M3_BTRX_ABB_BIAS_EN : 1;
    unsigned int RG_M3_BTRX_TIA_EN : 1;
    unsigned int RG_M3_BTRX_CBPF_EN : 1;
    unsigned int RG_M3_BTRX_LPBK_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_M3_BTRX_CBPF_CROSS_R : 1;
    unsigned int RG_M3_BTRX_PDET_EN : 1;
    unsigned int RG_M3_WFBT_TRX_HF_LDO_EN : 1;
    unsigned int RG_M3_WFBT_TRX_HF_LDO_RXB_EN : 1;
    unsigned int RG_M3_WFBT_RX_LNA_BIAS_EN : 1;
    unsigned int RG_M3_WFBT_RX_LNA_EN : 1;
    unsigned int RG_M3_WFBT_RX_EXTLNA_EN : 1;
    unsigned int RG_M3_WFBT_RX_CAL_EN : 1;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BT_RX_A10_FIELD_T;

#define RG_BT_RX_A11                                 (0x2c)
// Bit 0           rg_bt_trx_hf_ldo_fc_sel        U     RW        default = 'h1
// Bit 5   :1      rg_bt_trx_hf_ldo_vsel          U     RW        default = 'h10
// Bit 6           rg_bt_trx_hf_ldo_rc_ctrl       U     RW        default = 'h1
// Bit 15  :7      rg_bt_trx_hf_ldo_reserved      U     RW        default = 'h0
typedef union RG_BT_RX_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TRX_HF_LDO_FC_SEL : 1;
    unsigned int RG_BT_TRX_HF_LDO_VSEL : 5;
    unsigned int RG_BT_TRX_HF_LDO_RC_CTRL : 1;
    unsigned int RG_BT_TRX_HF_LDO_RESERVED : 9;
    unsigned int rsvd_0 : 16;
  } b;
} RG_BT_RX_A11_FIELD_T;

#define RG_BT_RX_A12                                 (0x30)
// Bit 7   :0      rg_btrx_fe_tp_sel              U     RW        default = 'h0
// Bit 8           rg_btrx_fe_tp_en               U     RW        default = 'h0
// Bit 23  :16     rg_btrx_abb_tp_sel             U     RW        default = 'h0
// Bit 24          rg_btrx_abb_tp_en              U     RW        default = 'h0
typedef union RG_BT_RX_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_FE_TP_SEL : 8;
    unsigned int RG_BTRX_FE_TP_EN : 1;
    unsigned int rsvd_0 : 7;
    unsigned int RG_BTRX_ABB_TP_SEL : 8;
    unsigned int RG_BTRX_ABB_TP_EN : 1;
    unsigned int rsvd_1 : 7;
  } b;
} RG_BT_RX_A12_FIELD_T;

#define RG_BT_RX_A13                                 (0x34)
// Bit 2   :0      rg_btrx_mxr_vcmsel             U     RW        default = 'h1
// Bit 6   :3      rg_btrx_mxr_vbsel              U     RW        default = 'h8
// Bit 10  :7      rg_btrx_mxr_lodrv_ictrl        U     RW        default = 'h3
// Bit 13  :11     rg_btrx_gm_vcmsel              U     RW        default = 'h4
// Bit 16  :14     rg_btrx_match                  U     RW        default = 'h0
typedef union RG_BT_RX_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_MXR_VCMSEL : 3;
    unsigned int RG_BTRX_MXR_VBSEL : 4;
    unsigned int RG_BTRX_MXR_LODRV_ICTRL : 4;
    unsigned int RG_BTRX_GM_VCMSEL : 3;
    unsigned int RG_BTRX_MATCH : 3;
    unsigned int rsvd_0 : 15;
  } b;
} RG_BT_RX_A13_FIELD_T;

#define RG_BT_RX_A14                                 (0x38)
// Bit 1   :0      rg_btrx_if_sel                 U     RW        default = 'h3
// Bit 4   :2      rg_btrx_vcm_ctl                U     RW        default = 'h5
// Bit 7   :5      rg_btrx_abb_ictl_tia           U     RW        default = 'h3
// Bit 10  :8      rg_btrx_abb_ictl_op            U     RW        default = 'h3
// Bit 13  :11     rg_btrx_abb_ictl_dri           U     RW        default = 'h3
// Bit 31  :14     rg_bt_rx_reserved4             U     RW        default = 'h0
typedef union RG_BT_RX_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_IF_SEL : 2;
    unsigned int RG_BTRX_VCM_CTL : 3;
    unsigned int RG_BTRX_ABB_ICTL_TIA : 3;
    unsigned int RG_BTRX_ABB_ICTL_OP : 3;
    unsigned int RG_BTRX_ABB_ICTL_DRI : 3;
    unsigned int RG_BT_RX_RESERVED4 : 18;
  } b;
} RG_BT_RX_A14_FIELD_T;

#define RG_BT_RX_A15                                 (0x3c)
// Bit 3   :0      rg_btrx_lna_band               U     RW        default = 'h1
// Bit 7   :4      rg_wfbt_lna_band               U     RW        default = 'h0
// Bit 11  :8      rg_btrx_lna_band_man           U     RW        default = 'h1
// Bit 16          rg_btrx_lna_band_man_mode      U     RW        default = 'h0
typedef union RG_BT_RX_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_BAND : 4;
    unsigned int RG_WFBT_LNA_BAND : 4;
    unsigned int RG_BTRX_LNA_BAND_MAN : 4;
    unsigned int rsvd_0 : 4;
    unsigned int RG_BTRX_LNA_BAND_MAN_MODE : 1;
    unsigned int rsvd_1 : 15;
  } b;
} RG_BT_RX_A15_FIELD_T;

#define RG_BT_RX_A16                                 (0x40)
// Bit 2   :0      rg_btrx_lna_band_offset_g1     U     RW        default = 'h0
// Bit 6   :4      rg_btrx_lna_band_offset_g2     U     RW        default = 'h0
// Bit 10  :8      rg_btrx_lna_band_offset_g3     U     RW        default = 'h0
// Bit 14  :12     rg_btrx_lna_band_offset_g4     U     RW        default = 'h0
// Bit 18  :16     rg_btrx_lna_band_offset_g5     U     RW        default = 'h0
// Bit 26  :24     rg_btrx_lna_band_offset_g6     U     RW        default = 'h0
// Bit 30  :28     rg_btrx_lna_band_offset_g7     U     RW        default = 'h0
typedef union RG_BT_RX_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G5 : 3;
    unsigned int rsvd_4 : 5;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G6 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_BTRX_LNA_BAND_OFFSET_G7 : 3;
    unsigned int rsvd_6 : 1;
  } b;
} RG_BT_RX_A16_FIELD_T;

#define RG_BT_RX_A17                                 (0x44)
// Bit 2   :0      rg_btrx_lna_ictrl_g1           U     RW        default = 'h2
// Bit 6   :4      rg_btrx_lna_ictrl_g2           U     RW        default = 'h2
// Bit 10  :8      rg_btrx_lna_ictrl_g3           U     RW        default = 'h2
// Bit 14  :12     rg_btrx_lna_ictrl_g4           U     RW        default = 'h2
// Bit 18  :16     rg_btrx_lna_ictrl_g5           U     RW        default = 'h5
// Bit 22  :20     rg_btrx_lna_ictrl_g6           U     RW        default = 'h5
// Bit 26  :24     rg_btrx_lna_ictrl_g7           U     RW        default = 'h5
// Bit 30  :28     rg_btrx_lna_ictrl_man          U     RW        default = 'h5
// Bit 31          rg_btrx_lna_ictrl_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_ICTRL_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G5 : 3;
    unsigned int rsvd_4 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G6 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_G7 : 3;
    unsigned int rsvd_6 : 1;
    unsigned int RG_BTRX_LNA_ICTRL_MAN : 3;
    unsigned int RG_BTRX_LNA_ICTRL_MAN_MODE : 1;
  } b;
} RG_BT_RX_A17_FIELD_T;

#define RG_BT_RX_A18                                 (0x48)
// Bit 0           rg_btrx_lna_hg_en_man          U     RW        default = 'h0
// Bit 1           rg_btrx_lna_mg_en_man          U     RW        default = 'h0
// Bit 2           rg_btrx_lna_lg_en_man          U     RW        default = 'h0
// Bit 3           rg_btrx_lna_en_man_mode        U     RW        default = 'h0
// Bit 6   :4      rg_btrx_lna_lg_g1              U     RW        default = 'h1
// Bit 9   :7      rg_btrx_lna_lg_g2              U     RW        default = 'h2
// Bit 12  :10     rg_btrx_lna_lg_g3              U     RW        default = 'h4
// Bit 15  :13     rg_btrx_lna_lg_man             U     RW        default = 'h4
// Bit 17  :16     rg_btrx_lna_mg_g4              U     RW        default = 'h1
// Bit 19  :18     rg_btrx_lna_mg_g5              U     RW        default = 'h2
// Bit 21  :20     rg_btrx_lna_mg_man             U     RW        default = 'h2
// Bit 24  :22     rg_btrx_lna_hg_g6              U     RW        default = 'h1
// Bit 27  :25     rg_btrx_lna_hg_g7              U     RW        default = 'h3
// Bit 30  :28     rg_btrx_lna_hg_man             U     RW        default = 'h3
// Bit 31          rg_btrx_lna_slice_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_HG_EN_MAN : 1;
    unsigned int RG_BTRX_LNA_MG_EN_MAN : 1;
    unsigned int RG_BTRX_LNA_LG_EN_MAN : 1;
    unsigned int RG_BTRX_LNA_EN_MAN_MODE : 1;
    unsigned int RG_BTRX_LNA_LG_G1 : 3;
    unsigned int RG_BTRX_LNA_LG_G2 : 3;
    unsigned int RG_BTRX_LNA_LG_G3 : 3;
    unsigned int RG_BTRX_LNA_LG_MAN : 3;
    unsigned int RG_BTRX_LNA_MG_G4 : 2;
    unsigned int RG_BTRX_LNA_MG_G5 : 2;
    unsigned int RG_BTRX_LNA_MG_MAN : 2;
    unsigned int RG_BTRX_LNA_HG_G6 : 3;
    unsigned int RG_BTRX_LNA_HG_G7 : 3;
    unsigned int RG_BTRX_LNA_HG_MAN : 3;
    unsigned int RG_BTRX_LNA_SLICE_MAN_MODE : 1;
  } b;
} RG_BT_RX_A18_FIELD_T;

#define RG_BT_RX_A19                                 (0x4c)
// Bit 3   :0      rg_btrx_lna_rfb_g4             U     RW        default = 'h0
// Bit 7   :4      rg_btrx_lna_rfb_g5             U     RW        default = 'h0
// Bit 11  :8      rg_btrx_lna_rfb_g6             U     RW        default = 'h8
// Bit 15  :12     rg_btrx_lna_rfb_g7             U     RW        default = 'h0
// Bit 19  :16     rg_btrx_lna_rfb_man            U     RW        default = 'h4
// Bit 20          rg_btrx_lna_rfb_man_mode       U     RW        default = 'h0
typedef union RG_BT_RX_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_RFB_G4 : 4;
    unsigned int RG_BTRX_LNA_RFB_G5 : 4;
    unsigned int RG_BTRX_LNA_RFB_G6 : 4;
    unsigned int RG_BTRX_LNA_RFB_G7 : 4;
    unsigned int RG_BTRX_LNA_RFB_MAN : 4;
    unsigned int RG_BTRX_LNA_RFB_MAN_MODE : 1;
    unsigned int rsvd_0 : 11;
  } b;
} RG_BT_RX_A19_FIELD_T;

#define RG_BT_RX_A20                                 (0x50)
// Bit 2   :0      rg_btrx_lna_ro_g1              U     RW        default = 'h3
// Bit 5   :3      rg_btrx_lna_ro_g2              U     RW        default = 'h3
// Bit 8   :6      rg_btrx_lna_ro_g3              U     RW        default = 'h3
// Bit 11  :9      rg_btrx_lna_ro_g4              U     RW        default = 'h3
// Bit 14  :12     rg_btrx_lna_ro_g5              U     RW        default = 'h3
// Bit 17  :15     rg_btrx_lna_ro_g6              U     RW        default = 'h0
// Bit 26  :24     rg_btrx_lna_ro_man             U     RW        default = 'h3
// Bit 31          rg_btrx_lna_ro_man_mode        U     RW        default = 'h0
typedef union RG_BT_RX_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_RO_G1 : 3;
    unsigned int RG_BTRX_LNA_RO_G2 : 3;
    unsigned int RG_BTRX_LNA_RO_G3 : 3;
    unsigned int RG_BTRX_LNA_RO_G4 : 3;
    unsigned int RG_BTRX_LNA_RO_G5 : 3;
    unsigned int RG_BTRX_LNA_RO_G6 : 3;
    unsigned int rsvd_0 : 6;
    unsigned int RG_BTRX_LNA_RO_MAN : 3;
    unsigned int rsvd_1 : 4;
    unsigned int RG_BTRX_LNA_RO_MAN_MODE : 1;
  } b;
} RG_BT_RX_A20_FIELD_T;

#define RG_BT_RX_A21                                 (0x54)
// Bit 2   :0      rg_btrx_lna_att_g1             U     RW        default = 'h1
// Bit 5   :3      rg_btrx_lna_att_g2             U     RW        default = 'h2
// Bit 8   :6      rg_btrx_lna_att_g3             U     RW        default = 'h4
// Bit 11  :9      rg_btrx_lna_att_man            U     RW        default = 'h4
// Bit 12          rg_btrx_lna_att_man_mode       U     RW        default = 'h0
typedef union RG_BT_RX_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_LNA_ATT_G1 : 3;
    unsigned int RG_BTRX_LNA_ATT_G2 : 3;
    unsigned int RG_BTRX_LNA_ATT_G3 : 3;
    unsigned int RG_BTRX_LNA_ATT_MAN : 3;
    unsigned int RG_BTRX_LNA_ATT_MAN_MODE : 1;
    unsigned int rsvd_0 : 19;
  } b;
} RG_BT_RX_A21_FIELD_T;

#define RG_BT_RX_A22                                 (0x58)
// Bit 2   :0      rg_btrx_pdet_threshold         U     RW        default = 'h3
// Bit 4   :3      rg_btrx_pdet_gain              U     RW        default = 'h2
// Bit 7   :5      rg_btrx_pdet_ictrl             U     RW        default = 'h6
// Bit 14  :8      rg_btrx_pdet_reserved          U     RW        default = 'h0
typedef union RG_BT_RX_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_PDET_THRESHOLD : 3;
    unsigned int RG_BTRX_PDET_GAIN : 2;
    unsigned int RG_BTRX_PDET_ICTRL : 3;
    unsigned int RG_BTRX_PDET_RESERVED : 7;
    unsigned int rsvd_0 : 17;
  } b;
} RG_BT_RX_A22_FIELD_T;

#define RG_BT_RX_A23                                 (0x5c)
// Bit 3   :0      rg_btrx_csel_offset            U     RW        default = 'h0
// Bit 25  :20     rg_btrx_csel_man               U     RW        default = 'h1a
// Bit 31          rg_btrx_csel_man_mode          U     RW        default = 'h0
typedef union RG_BT_RX_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_CSEL_OFFSET : 4;
    unsigned int rsvd_0 : 16;
    unsigned int RG_BTRX_CSEL_MAN : 6;
    unsigned int rsvd_1 : 5;
    unsigned int RG_BTRX_CSEL_MAN_MODE : 1;
  } b;
} RG_BT_RX_A23_FIELD_T;

#define RG_BT_RX_A24                                 (0x60)
// Bit 1   :0      rg_btrx_rck_t1                 U     RW        default = 'h0
// Bit 3   :2      rg_btrx_rck_t2                 U     RW        default = 'h0
// Bit 5   :4      rg_btrx_rck_t3                 U     RW        default = 'h0
// Bit 7   :6      rg_btrx_rck_t4                 U     RW        default = 'h2
// Bit 9   :8      rg_btrx_rck_t5                 U     RW        default = 'h0
// Bit 11  :10     rg_btrx_rck_t6                 U     RW        default = 'h0
// Bit 22  :20     rg_btrx_rck_refsel_man         U     RW        default = 'h1
// Bit 24          rg_btrx_rck_refsel_man_mode     U     RW        default = 'h0
// Bit 31          rg_btrx_rck_cali_en            U     RW        default = 'h0
typedef union RG_BT_RX_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_RCK_T1 : 2;
    unsigned int RG_BTRX_RCK_T2 : 2;
    unsigned int RG_BTRX_RCK_T3 : 2;
    unsigned int RG_BTRX_RCK_T4 : 2;
    unsigned int RG_BTRX_RCK_T5 : 2;
    unsigned int RG_BTRX_RCK_T6 : 2;
    unsigned int rsvd_0 : 8;
    unsigned int RG_BTRX_RCK_REFSEL_MAN : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BTRX_RCK_REFSEL_MAN_MODE : 1;
    unsigned int rsvd_2 : 6;
    unsigned int RG_BTRX_RCK_CALI_EN : 1;
  } b;
} RG_BT_RX_A24_FIELD_T;

#define RG_BT_RX_A25                                 (0x64)
// Bit 5   :0      ro_btrx_rck                    U     RO        default = 'h0
// Bit 8           ro_btrx_rck_vld                U     RO        default = 'h0
typedef union RG_BT_RX_A25_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BTRX_RCK : 6;
    unsigned int rsvd_0 : 2;
    unsigned int RO_BTRX_RCK_VLD : 1;
    unsigned int rsvd_1 : 23;
  } b;
} RG_BT_RX_A25_FIELD_T;

#define RG_BT_RX_A26                                 (0x68)
// Bit 2   :0      rg_wfbt_rx_lna_ictrl_g1        U     RW        default = 'h1
// Bit 6   :4      rg_wfbt_rx_lna_ictrl_g2        U     RW        default = 'h1
// Bit 10  :8      rg_wfbt_rx_lna_ictrl_g3        U     RW        default = 'h4
// Bit 14  :12     rg_wfbt_rx_lna_ictrl_g4        U     RW        default = 'h4
// Bit 18  :16     rg_wfbt_rx_lna_ictrl_g5        U     RW        default = 'h3
// Bit 22  :20     rg_wfbt_rx_lna_ictrl_g6        U     RW        default = 'h4
// Bit 26  :24     rg_wfbt_rx_lna_ictrl_g7        U     RW        default = 'h4
// Bit 30  :28     rg_wfbt_rx_lna_ictrl_man       U     RW        default = 'h0
// Bit 31          rg_wfbt_rx_lna_ictrl_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A26_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_ICTRL_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G5 : 3;
    unsigned int rsvd_4 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G6 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_G7 : 3;
    unsigned int rsvd_6 : 1;
    unsigned int RG_WFBT_RX_LNA_ICTRL_MAN : 3;
    unsigned int RG_WFBT_RX_LNA_ICTRL_MAN_MODE : 1;
  } b;
} RG_BT_RX_A26_FIELD_T;

#define RG_BT_RX_A27                                 (0x6c)
// Bit 7   :0      rg_wfbt_rx_lna_slice_g1        U     RW        default = 'had
// Bit 15  :8      rg_wfbt_rx_lna_slice_g2        U     RW        default = 'h26
// Bit 23  :16     rg_wfbt_rx_lna_slice_g3        U     RW        default = 'hb7
// Bit 31  :24     rg_wfbt_rx_lna_slice_g4        U     RW        default = 'hc
typedef union RG_BT_RX_A27_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_SLICE_G1 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_G2 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_G3 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_G4 : 8;
  } b;
} RG_BT_RX_A27_FIELD_T;

#define RG_BT_RX_A28                                 (0x70)
// Bit 7   :0      rg_wfbt_rx_lna_slice_g5        U     RW        default = 'h1b
// Bit 15  :8      rg_wfbt_rx_lna_slice_g6        U     RW        default = 'h7d
// Bit 23  :16     rg_wfbt_rx_lna_slice_g7        U     RW        default = 'hfb
// Bit 31  :24     rg_wfbt_rx_lna_slice_man       U     RW        default = 'h0
typedef union RG_BT_RX_A28_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_SLICE_G5 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_G6 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_G7 : 8;
    unsigned int RG_WFBT_RX_LNA_SLICE_MAN : 8;
  } b;
} RG_BT_RX_A28_FIELD_T;

#define RG_BT_RX_A29                                 (0x74)
// Bit 4   :0      rg_wfbt_rx_lna_rfb_g1          U     RW        default = 'h0
// Bit 9   :5      rg_wfbt_rx_lna_rfb_g2          U     RW        default = 'h0
// Bit 14  :10     rg_wfbt_rx_lna_rfb_g3          U     RW        default = 'h0
// Bit 19  :15     rg_wfbt_rx_lna_rfb_g4          U     RW        default = 'h2
// Bit 24  :20     rg_wfbt_rx_lna_rfb_g5          U     RW        default = 'h4
// Bit 29  :25     rg_wfbt_rx_lna_rfb_g6          U     RW        default = 'h8
typedef union RG_BT_RX_A29_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_RFB_G1 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_G2 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_G3 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_G4 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_G5 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_G6 : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_BT_RX_A29_FIELD_T;

#define RG_BT_RX_A30                                 (0x78)
// Bit 4   :0      rg_wfbt_rx_lna_rfb_g7          U     RW        default = 'h0
// Bit 9   :5      rg_wfbt_rx_lna_rfb_man         U     RW        default = 'h0
// Bit 10          rg_wfbt_rx_lna_rfb_man_mode     U     RW        default = 'h0
// Bit 31          rg_wfbt_rx_lna_slice_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A30_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_RFB_G7 : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_MAN : 5;
    unsigned int RG_WFBT_RX_LNA_RFB_MAN_MODE : 1;
    unsigned int rsvd_0 : 20;
    unsigned int RG_WFBT_RX_LNA_SLICE_MAN_MODE : 1;
  } b;
} RG_BT_RX_A30_FIELD_T;

#define RG_BT_RX_A31                                 (0x7c)
// Bit 2   :0      rg_wfbt_rx_lna_rt_g1           U     RW        default = 'h5
// Bit 5   :3      rg_wfbt_rx_lna_rt_g2           U     RW        default = 'h5
// Bit 8   :6      rg_wfbt_rx_lna_rt_g3           U     RW        default = 'h5
// Bit 11  :9      rg_wfbt_rx_lna_rt_g4           U     RW        default = 'h5
// Bit 14  :12     rg_wfbt_rx_lna_rt_g5           U     RW        default = 'h5
// Bit 17  :15     rg_wfbt_rx_lna_rt_g6           U     RW        default = 'h0
// Bit 26  :24     rg_wfbt_rx_lna_rt_man          U     RW        default = 'h0
// Bit 31          rg_wfbt_rx_lna_rt_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A31_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_RT_G1 : 3;
    unsigned int RG_WFBT_RX_LNA_RT_G2 : 3;
    unsigned int RG_WFBT_RX_LNA_RT_G3 : 3;
    unsigned int RG_WFBT_RX_LNA_RT_G4 : 3;
    unsigned int RG_WFBT_RX_LNA_RT_G5 : 3;
    unsigned int RG_WFBT_RX_LNA_RT_G6 : 3;
    unsigned int rsvd_0 : 6;
    unsigned int RG_WFBT_RX_LNA_RT_MAN : 3;
    unsigned int rsvd_1 : 4;
    unsigned int RG_WFBT_RX_LNA_RT_MAN_MODE : 1;
  } b;
} RG_BT_RX_A31_FIELD_T;

#define RG_BT_RX_A32                                 (0x80)
// Bit 2   :0      rg_wfbt_rx_lna_att_g1          U     RW        default = 'h7
// Bit 5   :3      rg_wfbt_rx_lna_att_g2          U     RW        default = 'h5
// Bit 8   :6      rg_wfbt_rx_lna_att_g3          U     RW        default = 'h7
// Bit 11  :9      rg_wfbt_rx_lna_att_man         U     RW        default = 'h0
// Bit 12          rg_wfbt_rx_lna_att_man_mode     U     RW        default = 'h0
typedef union RG_BT_RX_A32_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_ATT_G1 : 3;
    unsigned int RG_WFBT_RX_LNA_ATT_G2 : 3;
    unsigned int RG_WFBT_RX_LNA_ATT_G3 : 3;
    unsigned int RG_WFBT_RX_LNA_ATT_MAN : 3;
    unsigned int RG_WFBT_RX_LNA_ATT_MAN_MODE : 1;
    unsigned int rsvd_0 : 19;
  } b;
} RG_BT_RX_A32_FIELD_T;

#define RG_BT_RX_A33                                 (0x84)
// Bit 7   :0      rg_wfbt_rx_lna_priority        U     RW        default = 'h53
// Bit 23  :8      rg_wfbt_rx_lna_reserved        U     RW        default = 'h0
typedef union RG_BT_RX_A33_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WFBT_RX_LNA_PRIORITY : 8;
    unsigned int RG_WFBT_RX_LNA_RESERVED : 16;
    unsigned int rsvd_0 : 8;
  } b;
} RG_BT_RX_A33_FIELD_T;

#define RG_BT_RX_A34                                 (0x88)
// Bit 0           rg_btrx_cbpf_testout_en        U     RW        default = 'h0
// Bit 1           rg_btrx_adc_test_en            U     RW        default = 'h0
// Bit 2           rg_btrx_dac_test_en            U     RW        default = 'h0
// Bit 3           rg_btrx_lna_steer_en           U     RW        default = 'h0
// Bit 7   :4      rg_btrx_fe_reserved            U     RW        default = 'h1
// Bit 23  :16     rg_bt_rx_lna_en_delay_bnd      U     RW        default = 'h14
// Bit 24          rg_bt_rx_lna_en_delay_bypass     U     RW        default = 'h1
typedef union RG_BT_RX_A34_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_CBPF_TESTOUT_EN : 1;
    unsigned int RG_BTRX_ADC_TEST_EN : 1;
    unsigned int RG_BTRX_DAC_TEST_EN : 1;
    unsigned int RG_BTRX_LNA_STEER_EN : 1;
    unsigned int RG_BTRX_FE_RESERVED : 4;
    unsigned int rsvd_0 : 8;
    unsigned int RG_BT_RX_LNA_EN_DELAY_BND : 8;
    unsigned int RG_BT_RX_LNA_EN_DELAY_BYPASS : 1;
    unsigned int rsvd_1 : 7;
  } b;
} RG_BT_RX_A34_FIELD_T;

#define RG_BT_RX_A35                                 (0x8c)
// Bit 1   :0      rg_bt_agc_lock_option          U     RW        default = 'h2
typedef union RG_BT_RX_A35_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_AGC_LOCK_OPTION : 2;
    unsigned int rsvd_0 : 30;
  } b;
} RG_BT_RX_A35_FIELD_T;

#define RG_BT_RX_A36                                 (0x90)
// Bit 2   :0      rg_btrx_cal_buf_ictrl          U     RW        default = 'h3
// Bit 6   :4      rg_btrx_lna_cal_att            U     RW        default = 'h3
// Bit 10  :8      rg_wfbt_rx_cal_buf_ictrl       U     RW        default = 'h3
// Bit 14  :12     rg_wfbt_rx_lna_cal_att         U     RW        default = 'h3
// Bit 23  :16     rg_btrx_cal_reserved           U     RW        default = 'h0
// Bit 24          ro_bt_rx_pdet_out              U     RO        default = 'h0
typedef union RG_BT_RX_A36_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BTRX_CAL_BUF_ICTRL : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BTRX_LNA_CAL_ATT : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WFBT_RX_CAL_BUF_ICTRL : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WFBT_RX_LNA_CAL_ATT : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_BTRX_CAL_RESERVED : 8;
    unsigned int RO_BT_RX_PDET_OUT : 1;
    unsigned int rsvd_4 : 7;
  } b;
} RG_BT_RX_A36_FIELD_T;

#endif

