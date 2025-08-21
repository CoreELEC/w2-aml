#ifdef RF_D_TX_REG_0
#else
#define RF_D_TX_REG_0


#define RF_D_TX_REG_0_BASE                        (0x80000800)
#define RF_D_TX_REG_1_BASE                        (0x80001800)


#define RG_TX_A0                                  ( 0x0)
// Bit 2   :0      rg_wf5g_tx_pa_vgcg0            U     RW        default = 'h5
// Bit 6   :4      rg_wf5g_tx_da_cas              U     RW        default = 'h5
// Bit 13  :12     rg_wf5g_tx_tssi_attn_in        U     RW        default = 'h0
// Bit 19  :16     rg_wf5g_tx_tssi_tia_rsel       U     RW        default = 'h0
// Bit 22  :20     rg_wf5g_tx_mxr_lpf_ct          U     RW        default = 'h0
typedef union RG_TX_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_PA_VGCG0 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_TX_DA_CAS : 3;
    unsigned int rsvd_1 : 5;
    unsigned int RG_WF5G_TX_TSSI_ATTN_IN : 2;
    unsigned int rsvd_2 : 2;
    unsigned int RG_WF5G_TX_TSSI_TIA_RSEL : 4;
    unsigned int RG_WF5G_TX_MXR_LPF_CT : 3;
    unsigned int rsvd_3 : 9;
  } b;
} RG_TX_A0_FIELD_T;

#define RG_TX_A1                                  ( 0x4)
// Bit 2   :0      rg_wf5g_tx_adb_cbank           U     RW        default = 'h7
// Bit 6   :3      rg_wf5g_tx_adb_dc              U     RW        default = 'h4
// Bit 11          rg_wf5g_tx_adb_en              U     RW        default = 'h1
// Bit 12          rg_wf5g_tx_adb_ed_en           U     RW        default = 'h1
// Bit 18  :17     rg_wf5g_tx_adb_icgm            U     RW        default = 'h0
// Bit 21  :20     rg_wf5g_tx_adb_iptat           U     RW        default = 'h2
// Bit 25  :23     rg_wf5g_tx_ib_adj              U     RW        default = 'h4
typedef union RG_TX_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_ADB_CBANK : 3;
    unsigned int RG_WF5G_TX_ADB_DC : 4;
    unsigned int rsvd_0 : 4;
    unsigned int RG_WF5G_TX_ADB_EN : 1;
    unsigned int RG_WF5G_TX_ADB_ED_EN : 1;
    unsigned int rsvd_1 : 4;
    unsigned int RG_WF5G_TX_ADB_ICGM : 2;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WF5G_TX_ADB_IPTAT : 2;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF5G_TX_IB_ADJ : 3;
    unsigned int rsvd_4 : 6;
  } b;
} RG_TX_A1_FIELD_T;

#define RG_TX_A2                                  ( 0x8)
// Bit 0           rg_wf5g_tx_tssi_en             U     RW        default = 'h0
// Bit 3   :1      rg_wf5g_tx_tssi_attn           U     RW        default = 'h7
// Bit 6   :4      rg_wf5g_tx_tssi_lo_vb          U     RW        default = 'h3
// Bit 7           rg_wf5g_tx_iqcal_en            U     RW        default = 'h0
// Bit 8           rg_wf5g_tx_rxiqcal_en          U     RW        default = 'h0
// Bit 16  :9      rg_wf5g_tx_tp_sel              U     RW        default = 'h0
// Bit 17          rg_wf5g_tx_tp_en               U     RW        default = 'h0
// Bit 20  :18     rg_wf5g_tx_pa_gain_man         U     RW        default = 'h7
// Bit 24  :21     rg_wf5g_tx_da_gain_man         U     RW        default = 'h7
// Bit 25          rg_wf5g_tx_man_mode            U     RW        default = 'h0
typedef union RG_TX_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_TSSI_EN : 1;
    unsigned int RG_WF5G_TX_TSSI_ATTN : 3;
    unsigned int RG_WF5G_TX_TSSI_LO_VB : 3;
    unsigned int RG_WF5G_TX_IQCAL_EN : 1;
    unsigned int RG_WF5G_TX_RXIQCAL_EN : 1;
    unsigned int RG_WF5G_TX_TP_SEL : 8;
    unsigned int RG_WF5G_TX_TP_EN : 1;
    unsigned int RG_WF5G_TX_PA_GAIN_MAN : 3;
    unsigned int RG_WF5G_TX_DA_GAIN_MAN : 4;
    unsigned int RG_WF5G_TX_MAN_MODE : 1;
    unsigned int rsvd_0 : 6;
  } b;
} RG_TX_A2_FIELD_T;

#define RG_TX_A3                                  ( 0xc)
// Bit 0           rg_m0_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 1           rg_m0_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 2           rg_m0_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 3           rg_m0_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 4           rg_m1_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 5           rg_m1_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 6           rg_m1_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 7           rg_m1_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 8           rg_m2_wf5g_tx_pa_en            U     RW        default = 'h1
// Bit 9           rg_m2_wf5g_tx_mxr_en           U     RW        default = 'h1
// Bit 10          rg_m2_wf5g_tx_da_en            U     RW        default = 'h1
// Bit 11          rg_m2_wf5g_tx_ib_en            U     RW        default = 'h1
// Bit 12          rg_m3_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 13          rg_m3_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 14          rg_m3_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 15          rg_m3_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 31  :16     rg_wf5g_tx_rsv                 U     RW        default = 'h80ce
typedef union RG_TX_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_WF5G_TX_PA_EN : 1;
    unsigned int RG_M0_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M0_WF5G_TX_DA_EN : 1;
    unsigned int RG_M0_WF5G_TX_IB_EN : 1;
    unsigned int RG_M1_WF5G_TX_PA_EN : 1;
    unsigned int RG_M1_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M1_WF5G_TX_DA_EN : 1;
    unsigned int RG_M1_WF5G_TX_IB_EN : 1;
    unsigned int RG_M2_WF5G_TX_PA_EN : 1;
    unsigned int RG_M2_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M2_WF5G_TX_DA_EN : 1;
    unsigned int RG_M2_WF5G_TX_IB_EN : 1;
    unsigned int RG_M3_WF5G_TX_PA_EN : 1;
    unsigned int RG_M3_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M3_WF5G_TX_DA_EN : 1;
    unsigned int RG_M3_WF5G_TX_IB_EN : 1;
    unsigned int RG_WF5G_TX_RSV : 16;
  } b;
} RG_TX_A3_FIELD_T;

#define RG_TX_A4                                  ( 0x10)
// Bit 2   :0      rg_wf2g_tx_pa_vgcg0            U     RW        default = 'h7
// Bit 5   :3      rg_wf2g_tx_pa_vgcg1            U     RW        default = 'h7
// Bit 13  :11     rg_wf2g_tx_mxr_vgcg1           U     RW        default = 'h7
// Bit 16  :14     rg_wf2g_tx_mxr_lo_vb           U     RW        default = 'h7
// Bit 19  :17     rg_wf2g_tx_mxr_lobuf_ict       U     RW        default = 'h1
// Bit 23  :20     rg_wf2g_tx_var_u               U     RW        default = 'h8
// Bit 27  :24     rg_wf2g_tx_var_d               U     RW        default = 'h8
typedef union RG_TX_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_PA_VGCG0 : 3;
    unsigned int RG_WF2G_TX_PA_VGCG1 : 3;
    unsigned int rsvd_0 : 5;
    unsigned int RG_WF2G_TX_MXR_VGCG1 : 3;
    unsigned int RG_WF2G_TX_MXR_LO_VB : 3;
    unsigned int RG_WF2G_TX_MXR_LOBUF_ICT : 3;
    unsigned int RG_WF2G_TX_VAR_U : 4;
    unsigned int RG_WF2G_TX_VAR_D : 4;
    unsigned int rsvd_1 : 4;
  } b;
} RG_TX_A4_FIELD_T;

#define RG_TX_A5                                  ( 0x14)
// Bit 2   :0      rg_wf2g_tx_adb_cbank           U     RW        default = 'h3
// Bit 6   :3      rg_wf2g_tx_adb_dcib            U     RW        default = 'h1
// Bit 10  :7      rg_wf2g_tx_adb_pagmvg_csel     U     RW        default = 'h0
// Bit 11          rg_wf2g_tx_adb_en              U     RW        default = 'h1
// Bit 12          rg_wf2g_tx_adb_ed_en           U     RW        default = 'h1
// Bit 16  :13     rg_wf2g_tx_adb_ed_gain         U     RW        default = 'h3
// Bit 19  :17     rg_wf2g_tx_adb_icgm            U     RW        default = 'h2
// Bit 22  :20     rg_wf2g_tx_adb_iptat           U     RW        default = 'h2
// Bit 25  :23     rg_wf2g_tx_ib_adj              U     RW        default = 'h3
// Bit 26          rg_wf2g_tx_tssi_en             U     RW        default = 'h0
// Bit 29  :27     rg_wf2g_tx_tssi_attn           U     RW        default = 'h7
typedef union RG_TX_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 7;
    unsigned int RG_WF2G_TX_ADB_PAGMVG_CSEL : 4;
    unsigned int RG_WF2G_TX_ADB_EN : 1;
    unsigned int RG_WF2G_TX_ADB_ED_EN : 1;
    unsigned int rsvd_1 : 4;
    unsigned int RG_WF2G_TX_ADB_ICGM : 3;
    unsigned int RG_WF2G_TX_ADB_IPTAT : 3;
    unsigned int RG_WF2G_TX_IB_ADJ : 3;
    unsigned int RG_WF2G_TX_TSSI_EN : 1;
    unsigned int RG_WF2G_TX_TSSI_ATTN : 3;
    unsigned int rsvd_2 : 2;
  } b;
} RG_TX_A5_FIELD_T;

#define RG_TX_A5_REVA                              ( 0x14)
// Bit 2   :0      rg_wf2g_tx_adb_cbank           U     RW        default = 'h3
// Bit 6   :3      rg_wf2g_tx_adb_dcib            U     RW        default = 'h1
// Bit 10  :7      rg_wf2g_tx_adb_pagmvg_csel     U     RW        default = 'h0
// Bit 11          rg_wf2g_tx_adb_en              U     RW        default = 'h1
// Bit 12          rg_wf2g_tx_adb_ed_en           U     RW        default = 'h1
// Bit 16  :13     rg_wf2g_tx_adb_ed_gain         U     RW        default = 'h3
// Bit 19  :17     rg_wf2g_tx_adb_icgm            U     RW        default = 'h2
// Bit 22  :20     rg_wf2g_tx_adb_iptat           U     RW        default = 'h2
// Bit 25  :23     rg_wf2g_tx_ib_adj              U     RW        default = 'h3
// Bit 26          rg_wf2g_tx_tssi_en             U     RW        default = 'h0
// Bit 29  :27     rg_wf2g_tx_tssi_attn           U     RW        default = 'h7
typedef union RG_TX_A5_FIELD_REVA
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_ADB_CBANK : 3;
    unsigned int RG_WF2G_TX_ADB_DCIB : 4;
    unsigned int RG_WF2G_TX_ADB_PAGMVG_CSEL : 4;
    unsigned int RG_WF2G_TX_ADB_EN : 1;
    unsigned int RG_WF2G_TX_ADB_ED_EN : 1;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN : 4;
    unsigned int RG_WF2G_TX_ADB_ICGM : 3;
    unsigned int RG_WF2G_TX_ADB_IPTAT : 3;
    unsigned int RG_WF2G_TX_IB_ADJ : 3;
    unsigned int RG_WF2G_TX_TSSI_EN : 1;
    unsigned int RG_WF2G_TX_TSSI_ATTN : 3;
    unsigned int rsvd_0 : 2;
  } b;
} RG_TX_A5_FIELD_T_REVA;

#define RG_TX_A6                                  ( 0x18)
// Bit 2   :0      rg_wf2g_tx_tssi_attn_pain      U     RW        default = 'h2
// Bit 7   :3      rg_wf2g_tx_tssi_tia_rsel       U     RW        default = 'h8
// Bit 10  :8      rg_wf2g_tx_tssi_lo_vb          U     RW        default = 'h3
// Bit 11          rg_wf2g_tx_iqcal_en            U     RW        default = 'h0
// Bit 12          rg_wf2g_tx_rxiqcal_en          U     RW        default = 'h0
// Bit 20  :13     rg_wf2g_tx_tp_sel              U     RW        default = 'h0
// Bit 21          rg_wf2g_tx_tp_en               U     RW        default = 'h0
// Bit 24  :22     rg_wf2g_tx_pa_gain_man         U     RW        default = 'h7
// Bit 27  :25     rg_wf2g_tx_mxr_gain_man        U     RW        default = 'h7
// Bit 31          rg_wf2g_tx_man_mode            U     RW        default = 'h0
typedef union RG_TX_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_TSSI_ATTN_PAIN : 3;
    unsigned int RG_WF2G_TX_TSSI_TIA_RSEL : 5;
    unsigned int RG_WF2G_TX_TSSI_LO_VB : 3;
    unsigned int RG_WF2G_TX_IQCAL_EN : 1;
    unsigned int RG_WF2G_TX_RXIQCAL_EN : 1;
    unsigned int RG_WF2G_TX_TP_SEL : 8;
    unsigned int RG_WF2G_TX_TP_EN : 1;
    unsigned int RG_WF2G_TX_PA_GAIN_MAN : 3;
    unsigned int RG_WF2G_TX_MXR_GAIN_MAN : 3;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF2G_TX_MAN_MODE : 1;
  } b;
} RG_TX_A6_FIELD_T;

#define RG_TX_A7                                  ( 0x1c)
// Bit 0           rg_m0_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 1           rg_m0_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 2           rg_m0_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 3           rg_m0_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 4           rg_m1_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 5           rg_m1_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 6           rg_m1_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 7           rg_m1_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 8           rg_m2_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 9           rg_m2_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 10          rg_m2_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 11          rg_m2_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 12          rg_m3_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 13          rg_m3_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 14          rg_m3_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 15          rg_m3_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 31  :16     rg_wf2g_tx_rsv                 U     RW        default = 'h80ce
typedef union RG_TX_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_WF2G_TX_PA_EN : 1;
    unsigned int RG_M0_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M0_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M0_WF2G_TX_IB_EN : 1;
    unsigned int RG_M1_WF2G_TX_PA_EN : 1;
    unsigned int RG_M1_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M1_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M1_WF2G_TX_IB_EN : 1;
    unsigned int RG_M2_WF2G_TX_PA_EN : 1;
    unsigned int RG_M2_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M2_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M2_WF2G_TX_IB_EN : 1;
    unsigned int RG_M3_WF2G_TX_PA_EN : 1;
    unsigned int RG_M3_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M3_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M3_WF2G_TX_IB_EN : 1;
    unsigned int RG_WF2G_TX_RSV : 16;
  } b;
} RG_TX_A7_FIELD_T;

#define RG_TX_A8                                  ( 0x20)
// Bit 2   :0      rg_wf5g_tx_pa_gain_g1          U     RW        default = 'h7
// Bit 6   :4      rg_wf5g_tx_pa_gain_g2          U     RW        default = 'h7
// Bit 10  :8      rg_wf5g_tx_pa_gain_g3          U     RW        default = 'h3
// Bit 14  :12     rg_wf5g_tx_pa_gain_g4          U     RW        default = 'h1
// Bit 19  :16     rg_wf5g_tx_da_gain_g1          U     RW        default = 'h7
// Bit 23  :20     rg_wf5g_tx_da_gain_g2          U     RW        default = 'h7
// Bit 27  :24     rg_wf5g_tx_da_gain_g3          U     RW        default = 'h3
// Bit 31  :28     rg_wf5g_tx_da_gain_g4          U     RW        default = 'h1
typedef union RG_TX_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_PA_GAIN_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_TX_PA_GAIN_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF5G_TX_PA_GAIN_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WF5G_TX_PA_GAIN_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF5G_TX_DA_GAIN_G1 : 4;
    unsigned int RG_WF5G_TX_DA_GAIN_G2 : 4;
    unsigned int RG_WF5G_TX_DA_GAIN_G3 : 4;
    unsigned int RG_WF5G_TX_DA_GAIN_G4 : 4;
  } b;
} RG_TX_A8_FIELD_T;

#define RG_TX_A9                                  ( 0x24)
// Bit 2   :0      rg_wf2g_tx_pa_gain_g1          U     RW        default = 'h7
// Bit 6   :4      rg_wf2g_tx_pa_gain_g2          U     RW        default = 'h7
// Bit 10  :8      rg_wf2g_tx_pa_gain_g3          U     RW        default = 'h3
// Bit 14  :12     rg_wf2g_tx_pa_gain_g4          U     RW        default = 'h1
// Bit 18  :16     rg_wf2g_tx_mxr_gain_g1         U     RW        default = 'h7
// Bit 22  :20     rg_wf2g_tx_mxr_gain_g2         U     RW        default = 'h7
// Bit 26  :24     rg_wf2g_tx_mxr_gain_g3         U     RW        default = 'h3
// Bit 30  :28     rg_wf2g_tx_mxr_gain_g4         U     RW        default = 'h1
typedef union RG_TX_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_PA_GAIN_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF2G_TX_PA_GAIN_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF2G_TX_PA_GAIN_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WF2G_TX_PA_GAIN_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF2G_TX_MXR_GAIN_G1 : 3;
    unsigned int rsvd_4 : 1;
    unsigned int RG_WF2G_TX_MXR_GAIN_G2 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_WF2G_TX_MXR_GAIN_G3 : 3;
    unsigned int rsvd_6 : 1;
    unsigned int RG_WF2G_TX_MXR_GAIN_G4 : 3;
    unsigned int rsvd_7 : 1;
  } b;
} RG_TX_A9_FIELD_T;

#define RG_TX_A10                                 ( 0x28)
// Bit 0           rg_m4_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 1           rg_m4_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 2           rg_m4_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 3           rg_m4_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 4           rg_m5_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 5           rg_m5_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 6           rg_m5_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 7           rg_m5_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 8           rg_m6_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 9           rg_m6_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 10          rg_m6_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 11          rg_m6_wf5g_tx_ib_en            U     RW        default = 'h0
// Bit 12          rg_m7_wf5g_tx_pa_en            U     RW        default = 'h0
// Bit 13          rg_m7_wf5g_tx_mxr_en           U     RW        default = 'h0
// Bit 14          rg_m7_wf5g_tx_da_en            U     RW        default = 'h0
// Bit 15          rg_m7_wf5g_tx_ib_en            U     RW        default = 'h0
typedef union RG_TX_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M4_WF5G_TX_PA_EN : 1;
    unsigned int RG_M4_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M4_WF5G_TX_DA_EN : 1;
    unsigned int RG_M4_WF5G_TX_IB_EN : 1;
    unsigned int RG_M5_WF5G_TX_PA_EN : 1;
    unsigned int RG_M5_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M5_WF5G_TX_DA_EN : 1;
    unsigned int RG_M5_WF5G_TX_IB_EN : 1;
    unsigned int RG_M6_WF5G_TX_PA_EN : 1;
    unsigned int RG_M6_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M6_WF5G_TX_DA_EN : 1;
    unsigned int RG_M6_WF5G_TX_IB_EN : 1;
    unsigned int RG_M7_WF5G_TX_PA_EN : 1;
    unsigned int RG_M7_WF5G_TX_MXR_EN : 1;
    unsigned int RG_M7_WF5G_TX_DA_EN : 1;
    unsigned int RG_M7_WF5G_TX_IB_EN : 1;
    unsigned int rsvd_0 : 16;
  } b;
} RG_TX_A10_FIELD_T;

#define RG_TX_A11                                 ( 0x2c)
// Bit 0           rg_m4_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 1           rg_m4_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 2           rg_m4_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 3           rg_m4_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 4           rg_m5_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 5           rg_m5_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 6           rg_m5_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 7           rg_m5_wf2g_tx_ib_en            U     RW        default = 'h0
// Bit 8           rg_m6_wf2g_tx_pa_en            U     RW        default = 'h1
// Bit 9           rg_m6_wf2g_tx_mxr_en           U     RW        default = 'h1
// Bit 10          rg_m6_wf2g_tx_var_en           U     RW        default = 'h1
// Bit 11          rg_m6_wf2g_tx_ib_en            U     RW        default = 'h1
// Bit 12          rg_m7_wf2g_tx_pa_en            U     RW        default = 'h0
// Bit 13          rg_m7_wf2g_tx_mxr_en           U     RW        default = 'h0
// Bit 14          rg_m7_wf2g_tx_var_en           U     RW        default = 'h0
// Bit 15          rg_m7_wf2g_tx_ib_en            U     RW        default = 'h0
typedef union RG_TX_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M4_WF2G_TX_PA_EN : 1;
    unsigned int RG_M4_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M4_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M4_WF2G_TX_IB_EN : 1;
    unsigned int RG_M5_WF2G_TX_PA_EN : 1;
    unsigned int RG_M5_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M5_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M5_WF2G_TX_IB_EN : 1;
    unsigned int RG_M6_WF2G_TX_PA_EN : 1;
    unsigned int RG_M6_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M6_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M6_WF2G_TX_IB_EN : 1;
    unsigned int RG_M7_WF2G_TX_PA_EN : 1;
    unsigned int RG_M7_WF2G_TX_MXR_EN : 1;
    unsigned int RG_M7_WF2G_TX_VAR_EN : 1;
    unsigned int RG_M7_WF2G_TX_IB_EN : 1;
    unsigned int rsvd_0 : 16;
  } b;
} RG_TX_A11_FIELD_T;

#define RG_TX_A12                                 ( 0x30)
// Bit 8   :0      rg_wf_tx_gain_man              U     RW        default = 'h0
// Bit 12          rg_wf_tx_gain_man_mode         U     RW        default = 'h0
// Bit 23  :16     rg_wf_tx_pa_en_delay_bnd       U     RW        default = 'h14
// Bit 24          rg_wf_tx_pa_en_delay_bypass     U     RW        default = 'h0
typedef union RG_TX_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_TX_GAIN_MAN : 9;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_TX_GAIN_MAN_MODE : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WF_TX_PA_EN_DELAY_BND : 8;
    unsigned int RG_WF_TX_PA_EN_DELAY_BYPASS : 1;
    unsigned int rsvd_2 : 7;
  } b;
} RG_TX_A12_FIELD_T;

#define RG_TX_A13                                 ( 0x34)
// Bit 4   :0      rg_wf5g_tx_mxr_cbank_f1        U     RW        default = 'h7
// Bit 9   :5      rg_wf5g_tx_mxr_cbank_f2        U     RW        default = 'h7
// Bit 14  :10     rg_wf5g_tx_mxr_cbank_f3        U     RW        default = 'h7
// Bit 19  :15     rg_wf5g_tx_mxr_cbank_f4        U     RW        default = 'h3
// Bit 24  :20     rg_wf5g_tx_mxr_cbank_f5        U     RW        default = 'h7
// Bit 29  :25     rg_wf5g_tx_mxr_cbank_man       U     RW        default = 'h4
// Bit 30          rg_wf5g_tx_mxr_cbank_man_mode     U     RW        default = 'h0
typedef union RG_TX_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_MXR_CBANK_F1 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_F2 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_F3 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_F4 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_F5 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_MAN : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_MAN_MODE : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_TX_A13_FIELD_T;

#define RG_TX_A14                                 ( 0x38)
// Bit 4   :0      rg_wf5g_tx_da_ct_f1            U     RW        default = 'h7
// Bit 9   :5      rg_wf5g_tx_da_ct_f2            U     RW        default = 'h7
// Bit 14  :10     rg_wf5g_tx_da_ct_f3            U     RW        default = 'h7
// Bit 19  :15     rg_wf5g_tx_da_ct_f4            U     RW        default = 'h3
// Bit 24  :20     rg_wf5g_tx_da_ct_f5            U     RW        default = 'h3
// Bit 29  :25     rg_wf5g_tx_da_ct_man           U     RW        default = 'h4
// Bit 30          rg_wf5g_tx_da_ct_man_mode      U     RW        default = 'h0
typedef union RG_TX_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_DA_CT_F1 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F2 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F3 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F4 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F5 : 5;
    unsigned int RG_WF5G_TX_DA_CT_MAN : 5;
    unsigned int RG_WF5G_TX_DA_CT_MAN_MODE : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_TX_A14_FIELD_T;

#define RG_TX_A15                                 ( 0x3c)
// Bit 0           rg_wf2g_tx_pa_vgcg1_sel        U     RW        default = 'h0
// Bit 3   :1      rg_wf2g_tx_pa_ccg1_csel        U     RW        default = 'h4
// Bit 5   :4      rg_wf2g_tx_tssi_tia_ictrl_stg1     U     RW        default = 'h1
// Bit 8   :6      rg_wf2g_tx_tssi_tia_ictrl_stg2     U     RW        default = 'h3
// Bit 11  :9      rg_wf2g_tx_pa_xfmr_pri_csel     U     RW        default = 'h4
// Bit 14  :12     rg_wf2g_tx_pa_xfmr_ctap_csel     U     RW        default = 'h4
// Bit 17  :16     rg_wf2g_tx_mxr_topifsw         U     RW        default = 'h0
typedef union RG_TX_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_PA_VGCG1_SEL : 1;
    unsigned int RG_WF2G_TX_PA_CCG1_CSEL : 3;
    unsigned int RG_WF2G_TX_TSSI_TIA_ICTRL_STG1 : 2;
    unsigned int RG_WF2G_TX_TSSI_TIA_ICTRL_STG2 : 3;
    unsigned int RG_WF2G_TX_PA_XFMR_PRI_CSEL : 3;
    unsigned int RG_WF2G_TX_PA_XFMR_CTAP_CSEL : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF2G_TX_MXR_TOPIFSW : 2;
    unsigned int rsvd_1 : 14;
  } b;
} RG_TX_A15_FIELD_T;

#define RG_TX_A16                                 ( 0x40)
// Bit 4   :0      rg_wf5g_tx_mxr_cbank_f6        U     RW        default = 'h7
// Bit 9   :5      rg_wf5g_tx_mxr_cbank_f7        U     RW        default = 'h7
// Bit 14  :10     rg_wf5g_tx_da_ct_f6            U     RW        default = 'h3
// Bit 19  :15     rg_wf5g_tx_da_ct_f7            U     RW        default = 'h3
// Bit 24          rg_wf5g_tx_adb_ed_gain_man_mode     U     RW        default = 'h0
typedef union RG_TX_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_MXR_CBANK_F6 : 5;
    unsigned int RG_WF5G_TX_MXR_CBANK_F7 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F6 : 5;
    unsigned int RG_WF5G_TX_DA_CT_F7 : 5;
    unsigned int rsvd_0 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_MAN_MODE : 1;
    unsigned int rsvd_1 : 7;
  } b;
} RG_TX_A16_FIELD_T;

#define RG_TX_A17                                 ( 0x44)
// Bit 3   :0      rg_wf5g_tx_adb_ed_gain_f1      U     RW        default = 'hc
// Bit 7   :4      rg_wf5g_tx_adb_ed_gain_f2      U     RW        default = 'hc
// Bit 11  :8      rg_wf5g_tx_adb_ed_gain_f3      U     RW        default = 'hc
// Bit 15  :12     rg_wf5g_tx_adb_ed_gain_f4      U     RW        default = 'hc
// Bit 19  :16     rg_wf5g_tx_adb_ed_gain_f5      U     RW        default = 'hc
// Bit 23  :20     rg_wf5g_tx_adb_ed_gain_f6      U     RW        default = 'hc
// Bit 27  :24     rg_wf5g_tx_adb_ed_gain_f7      U     RW        default = 'hc
// Bit 31  :28     rg_wf5g_tx_adb_ed_gain_man     U     RW        default = 'hc
typedef union RG_TX_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F1 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F2 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F3 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F4 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F5 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F6 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_F7 : 4;
    unsigned int RG_WF5G_TX_ADB_ED_GAIN_MAN : 4;
  } b;
} RG_TX_A17_FIELD_T;

#define RG_TX_A18                                 ( 0x48)
// Bit 2   :0      rg_wf5g_tx_pa_gain_g5          U     RW        default = 'h1
// Bit 6   :4      rg_wf5g_tx_pa_gain_g6          U     RW        default = 'h1
// Bit 11  :8      rg_wf5g_tx_da_gain_g5          U     RW        default = 'h1
// Bit 15  :12     rg_wf5g_tx_da_gain_g6          U     RW        default = 'h1
typedef union RG_TX_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_PA_GAIN_G5 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_TX_PA_GAIN_G6 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF5G_TX_DA_GAIN_G5 : 4;
    unsigned int RG_WF5G_TX_DA_GAIN_G6 : 4;
    unsigned int rsvd_2 : 16;
  } b;
} RG_TX_A18_FIELD_T;

#define RG_TX_A19                                 ( 0x4c)
// Bit 2   :0      rg_wf5g_tx_da_fine_gain_g1     U     RW        default = 'h3
// Bit 6   :4      rg_wf5g_tx_da_fine_gain_g2     U     RW        default = 'h3
// Bit 10  :8      rg_wf5g_tx_da_fine_gain_g3     U     RW        default = 'h3
// Bit 14  :12     rg_wf5g_tx_da_fine_gain_g4     U     RW        default = 'h3
// Bit 18  :16     rg_wf5g_tx_da_fine_gain_g5     U     RW        default = 'h3
// Bit 22  :20     rg_wf5g_tx_da_fine_gain_g6     U     RW        default = 'h3
// Bit 26  :24     rg_wf5g_tx_da_fine_gain_man     U     RW        default = 'h3
// Bit 28          rg_wf5g_tx_da_ibias_tune_man_mode     U     RW        default = 'h0
typedef union RG_TX_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G1 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G2 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G3 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G4 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G5 : 3;
    unsigned int rsvd_4 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_G6 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_WF5G_TX_DA_FINE_GAIN_MAN : 3;
    unsigned int rsvd_6 : 1;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_MAN_MODE : 1;
    unsigned int rsvd_7 : 3;
  } b;
} RG_TX_A19_FIELD_T;

#define RG_TX_A20                                 ( 0x50)
// Bit 3   :0      rg_wf5g_tx_da_ibias_tune_f1     U     RW        default = 'h3
// Bit 7   :4      rg_wf5g_tx_da_ibias_tune_f2     U     RW        default = 'h3
// Bit 11  :8      rg_wf5g_tx_da_ibias_tune_f3     U     RW        default = 'h3
// Bit 15  :12     rg_wf5g_tx_da_ibias_tune_f4     U     RW        default = 'h3
// Bit 19  :16     rg_wf5g_tx_da_ibias_tune_f5     U     RW        default = 'h3
// Bit 23  :20     rg_wf5g_tx_da_ibias_tune_f6     U     RW        default = 'h3
// Bit 27  :24     rg_wf5g_tx_da_ibias_tune_f7     U     RW        default = 'h3
// Bit 31  :28     rg_wf5g_tx_da_ibias_tune_man     U     RW        default = 'h3
typedef union RG_TX_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F1 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F2 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F3 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F4 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F5 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F6 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_F7 : 4;
    unsigned int RG_WF5G_TX_DA_IBIAS_TUNE_MAN : 4;
  } b;
} RG_TX_A20_FIELD_T;

#define RG_TX_A21                                 ( 0x54)
// Bit 0           rg_wf5g_tx_man_mode_xfmer      U     RW        default = 'h0
// Bit 4           rg_wf5g_tx_man_mode_sec_cap     U     RW        default = 'h0
// Bit 8           rg_wf5g_tx_pa_man_mode         U     RW        default = 'h0
// Bit 16          rg_wf5g_tx_sw_sec_cap          U     RW        default = 'h0
// Bit 20          rg_wf5g_tx_sw_xfmer            U     RW        default = 'h0
// Bit 24          rg_wf5g_tx_pabd_sw             U     RW        default = 'h0
// Bit 27  :25     rg_wf5g_tx_mx_cas              U     RW        default = 'h4
// Bit 28          rg_wf5g_tx_mx_mode_ctrol       U     RW        default = 'h0
typedef union RG_TX_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_MAN_MODE_XFMER : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF5G_TX_MAN_MODE_SEC_CAP : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WF5G_TX_PA_MAN_MODE : 1;
    unsigned int rsvd_2 : 7;
    unsigned int RG_WF5G_TX_SW_SEC_CAP : 1;
    unsigned int rsvd_3 : 3;
    unsigned int RG_WF5G_TX_SW_XFMER : 1;
    unsigned int rsvd_4 : 3;
    unsigned int RG_WF5G_TX_PABD_SW : 1;
    unsigned int RG_WF5G_TX_MX_CAS : 3;
    unsigned int RG_WF5G_TX_MX_Mode_Ctrol : 1;
    unsigned int rsvd_5 : 3;
  } b;
} RG_TX_A21_FIELD_T;

#define RG_TX_A22                                 ( 0x58)
// Bit 2   :0      rg_wf2g_tx_pa_gain_g5          U     RW        default = 'h7
// Bit 6   :4      rg_wf2g_tx_mxr_gain_g5         U     RW        default = 'h7
// Bit 10  :8      rg_wf2g_tx_mxr_gaincal_g1      U     RW        default = 'h3
// Bit 14  :12     rg_wf2g_tx_mxr_gaincal_g2      U     RW        default = 'h3
// Bit 18  :16     rg_wf2g_tx_mxr_gaincal_g3      U     RW        default = 'h3
// Bit 22  :20     rg_wf2g_tx_mxr_gaincal_g4      U     RW        default = 'h3
// Bit 26  :24     rg_wf2g_tx_mxr_gaincal_g5      U     RW        default = 'h3
// Bit 30  :28     rg_wf2g_tx_mxr_gaincal_man     U     RW        default = 'h3
typedef union RG_TX_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_PA_GAIN_G5 : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF2G_TX_MXR_GAIN_G5 : 3;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_G1 : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_G2 : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_G3 : 3;
    unsigned int rsvd_4 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_G4 : 3;
    unsigned int rsvd_5 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_G5 : 3;
    unsigned int rsvd_6 : 1;
    unsigned int RG_WF2G_TX_MXR_GAINCAL_MAN : 3;
    unsigned int rsvd_7 : 1;
  } b;
} RG_TX_A22_FIELD_T;

#define RG_TX_A23                                 ( 0x5c)
// Bit 4   :0      rg_wf2g_tx_mxr_cbank_g1        U     RW        default = 'h4
// Bit 9   :5      rg_wf2g_tx_mxr_cbank_g2        U     RW        default = 'h4
// Bit 14  :10     rg_wf2g_tx_mxr_cbank_g3        U     RW        default = 'h4
// Bit 19  :15     rg_wf2g_tx_mxr_cbank_g4        U     RW        default = 'h4
// Bit 24  :20     rg_wf2g_tx_mxr_cbank_g5        U     RW        default = 'h5
// Bit 29  :25     rg_wf2g_tx_mxr_cbank_man       U     RW        default = 'h4
typedef union RG_TX_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_MXR_CBANK_G1 : 5;
    unsigned int RG_WF2G_TX_MXR_CBANK_G2 : 5;
    unsigned int RG_WF2G_TX_MXR_CBANK_G3 : 5;
    unsigned int RG_WF2G_TX_MXR_CBANK_G4 : 5;
    unsigned int RG_WF2G_TX_MXR_CBANK_G5 : 5;
    unsigned int RG_WF2G_TX_MXR_CBANK_MAN : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_TX_A23_FIELD_T;

#define RG_TX_A24                                 ( 0x60)
// Bit 2   :0      rg_wf5g_tx_mxr_lobuf_ict       U     RW        default = 'h7
// Bit 3           rg_wf5g_tx_pacap_sw            U     RW        default = 'h0
typedef union RG_TX_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_TX_MXR_LOBUF_ICT : 3;
    unsigned int RG_WF5G_TX_PACAP_SW : 1;
    unsigned int rsvd_0 : 28;
  } b;
} RG_TX_A24_FIELD_T;

#define RG_TX_A25                                 ( 0x64)
// Bit 3   :0      rg_wf2g_tx_adb_dcib_g1         U     RW        default = 'h1
// Bit 7   :4      rg_wf2g_tx_adb_dcib_g2         U     RW        default = 'h1
// Bit 11  :8      rg_wf2g_tx_adb_dcib_g3         U     RW        default = 'h1
// Bit 15  :12     rg_wf2g_tx_adb_dcib_g4         U     RW        default = 'h1
// Bit 19  :16     rg_wf2g_tx_adb_dcib_g5         U     RW        default = 'h1
// Bit 23  :20     rg_wf2g_tx_adb_dcib_man        U     RW        default = 'h1
typedef union RG_TX_A25_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_ADB_DCIB_G1 : 4;
    unsigned int RG_WF2G_TX_ADB_DCIB_G2 : 4;
    unsigned int RG_WF2G_TX_ADB_DCIB_G3 : 4;
    unsigned int RG_WF2G_TX_ADB_DCIB_G4 : 4;
    unsigned int RG_WF2G_TX_ADB_DCIB_G5 : 4;
    unsigned int RG_WF2G_TX_ADB_DCIB_MAN : 4;
    unsigned int rsvd_0 : 8;
  } b;
} RG_TX_A25_FIELD_T;

#define RG_TX_A26                                 ( 0x68)
// Bit 3   :0      rg_wf2g_tx_adb_ed_gain_g1      U     RW        default = 'h3
// Bit 7   :4      rg_wf2g_tx_adb_ed_gain_g2      U     RW        default = 'h3
// Bit 11  :8      rg_wf2g_tx_adb_ed_gain_g3      U     RW        default = 'h3
// Bit 15  :12     rg_wf2g_tx_adb_ed_gain_g4      U     RW        default = 'h3
// Bit 19  :16     rg_wf2g_tx_adb_ed_gain_g5      U     RW        default = 'h3
// Bit 23  :20     rg_wf2g_tx_adb_ed_gain_man     U     RW        default = 'h3
typedef union RG_TX_A26_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_G1 : 4;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_G2 : 4;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_G3 : 4;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_G4 : 4;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_G5 : 4;
    unsigned int RG_WF2G_TX_ADB_ED_GAIN_MAN : 4;
    unsigned int rsvd_0 : 8;
  } b;
} RG_TX_A26_FIELD_T;

#define RG_TX_A27                                 ( 0x6c)
// Bit 2   :0      rg_wf2g_tx_adb_cbank_g1        U     RW        default = 'h3
// Bit 5   :3      rg_wf2g_tx_adb_cbank_g2        U     RW        default = 'h3
// Bit 8   :6      rg_wf2g_tx_adb_cbank_g3        U     RW        default = 'h3
// Bit 11  :9      rg_wf2g_tx_adb_cbank_g4        U     RW        default = 'h3
// Bit 14  :12     rg_wf2g_tx_adb_cbank_g5        U     RW        default = 'h3
// Bit 17  :15     rg_wf2g_tx_adb_cbank_man       U     RW        default = 'h3
typedef union RG_TX_A27_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF2G_TX_ADB_CBANK_G1 : 3;
    unsigned int RG_WF2G_TX_ADB_CBANK_G2 : 3;
    unsigned int RG_WF2G_TX_ADB_CBANK_G3 : 3;
    unsigned int RG_WF2G_TX_ADB_CBANK_G4 : 3;
    unsigned int RG_WF2G_TX_ADB_CBANK_G5 : 3;
    unsigned int RG_WF2G_TX_ADB_CBANK_MAN : 3;
    unsigned int rsvd_0 : 14;
  } b;
} RG_TX_A27_FIELD_T;

#endif

