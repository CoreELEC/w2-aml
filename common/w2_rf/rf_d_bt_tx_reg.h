#ifdef RF_D_BT_TX_REG
#else
#define RF_D_BT_TX_REG


#define RF_D_BT_TX_REG_BASE                       (0x80002200)

#define RG_BT_TX_A0                                  ( + 0x0)
// Bit 0           rg_m0_bt_tx_en                 U     RW        default = 'h0
// Bit 1           rg_m0_bt_txm_mx_en             U     RW        default = 'h0
// Bit 4           rg_m0_bt_tx_tia_ibias_en       U     RW        default = 'h0
// Bit 5           rg_m0_bt_tx_pa_en              U     RW        default = 'h0
typedef union RG_BT_TX_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_BT_TX_EN : 1;
    unsigned int RG_M0_BT_TXM_MX_EN : 1;
    unsigned int rsvd_0 : 2;
    unsigned int RG_M0_BT_TX_TIA_IBIAS_EN : 1;
    unsigned int RG_M0_BT_TX_PA_EN : 1;
    unsigned int rsvd_1 : 26;
  } b;
} RG_BT_TX_A0_FIELD_T;

#define RG_BT_TX_A1                                  ( + 0x4)
// Bit 0           rg_m1_bt_tx_en                 U     RW        default = 'h0
// Bit 1           rg_m1_bt_txm_mx_en             U     RW        default = 'h0
// Bit 4           rg_m1_bt_tx_tia_ibias_en       U     RW        default = 'h0
// Bit 5           rg_m1_bt_tx_pa_en              U     RW        default = 'h0
typedef union RG_BT_TX_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M1_BT_TX_EN : 1;
    unsigned int RG_M1_BT_TXM_MX_EN : 1;
    unsigned int rsvd_0 : 2;
    unsigned int RG_M1_BT_TX_TIA_IBIAS_EN : 1;
    unsigned int RG_M1_BT_TX_PA_EN : 1;
    unsigned int rsvd_1 : 26;
  } b;
} RG_BT_TX_A1_FIELD_T;

#define RG_BT_TX_A2                                  ( + 0x8)
// Bit 0           rg_m2_bt_tx_en                 U     RW        default = 'h1
// Bit 1           rg_m2_bt_txm_mx_en             U     RW        default = 'h1
// Bit 4           rg_m2_bt_tx_tia_ibias_en       U     RW        default = 'h1
// Bit 5           rg_m2_bt_tx_pa_en              U     RW        default = 'h1
typedef union RG_BT_TX_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M2_BT_TX_EN : 1;
    unsigned int RG_M2_BT_TXM_MX_EN : 1;
    unsigned int rsvd_0 : 2;
    unsigned int RG_M2_BT_TX_TIA_IBIAS_EN : 1;
    unsigned int RG_M2_BT_TX_PA_EN : 1;
    unsigned int rsvd_1 : 26;
  } b;
} RG_BT_TX_A2_FIELD_T;

#define RG_BT_TX_A3                                  ( + 0xc)
// Bit 0           rg_m3_bt_tx_en                 U     RW        default = 'h0
// Bit 1           rg_m3_bt_txm_mx_en             U     RW        default = 'h0
// Bit 4           rg_m3_bt_tx_tia_ibias_en       U     RW        default = 'h0
// Bit 5           rg_m3_bt_tx_pa_en              U     RW        default = 'h0
typedef union RG_BT_TX_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M3_BT_TX_EN : 1;
    unsigned int RG_M3_BT_TXM_MX_EN : 1;
    unsigned int rsvd_0 : 2;
    unsigned int RG_M3_BT_TX_TIA_IBIAS_EN : 1;
    unsigned int RG_M3_BT_TX_PA_EN : 1;
    unsigned int rsvd_1 : 26;
  } b;
} RG_BT_TX_A3_FIELD_T;

#define RG_BT_TX_A4                                  ( + 0x10)
// Bit 3   :0      rg_bt_tx_ibias_mix_vblo        U     RW        default = 'h5
// Bit 8   :4      rg_bt_tx_ibias_mix_vcm_man     U     RW        default = 'h5
// Bit 13  :9      rg_bt_tx_ibias_mix_vcm_g1      U     RW        default = 'h5
// Bit 18  :14     rg_bt_tx_ibias_mix_vcm_g2      U     RW        default = 'h5
// Bit 23  :19     rg_bt_tx_ibias_mix_vcm_g3      U     RW        default = 'h5
// Bit 28  :24     rg_bt_tx_ibias_mix_vcm_g4      U     RW        default = 'h5
// Bit 30  :29     rg_bt_tx_pa_vbcas              U     RW        default = 'h2
typedef union RG_BT_TX_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_IBIAS_MIX_VBLO : 4;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_MAN : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G1 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G2 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G3 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G4 : 5;
    unsigned int RG_BT_TX_PA_VBCAS : 2;
    unsigned int rsvd_0 : 1;
  } b;
} RG_BT_TX_A4_FIELD_T;

#define RG_BT_TX_A5                                  ( + 0x14)
// Bit 0           rg_bt_txm_man_mode             U     RW        default = 'h0
// Bit 3   :1      rg_bt_txm_gain_slide_man       U     RW        default = 'h7
// Bit 6   :4      rg_bt_txm_gain_slide_g1        U     RW        default = 'h1
// Bit 9   :7      rg_bt_txm_gain_slide_g2        U     RW        default = 'h3
// Bit 12  :10     rg_bt_txm_gain_slide_g3        U     RW        default = 'h7
// Bit 15  :13     rg_bt_txm_gain_slide_g4        U     RW        default = 'h7
// Bit 18  :16     rg_bt_txm_gain_slide_g5        U     RW        default = 'h1
// Bit 21  :19     rg_bt_txm_gain_slide_g6        U     RW        default = 'h3
// Bit 24  :22     rg_bt_txm_gain_slide_g7        U     RW        default = 'h7
// Bit 27  :25     rg_bt_txm_gain_slide_g8        U     RW        default = 'h7
typedef union RG_BT_TX_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TXM_MAN_MODE : 1;
    unsigned int RG_BT_TXM_GAIN_SLIDE_MAN : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G1 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G2 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G3 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G4 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G5 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G6 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G7 : 3;
    unsigned int RG_BT_TXM_GAIN_SLIDE_G8 : 3;
    unsigned int rsvd_0 : 4;
  } b;
} RG_BT_TX_A5_FIELD_T;

#define RG_BT_TX_A6                                  ( + 0x18)
// Bit 3   :1      rg_bt_tx_tp_sel                U     RW        default = 'h3
// Bit 13  :10     rg_bt_txm_lodrv                U     RW        default = 'h8
// Bit 16  :14     rg_bt_tx_pa_ctune              U     RW        default = 'h5
// Bit 19  :17     rg_bt_tx_iqdect_ctgm           U     RW        default = 'h3
// Bit 22  :20     rg_bt_tx_iqdect_ctlo           U     RW        default = 'h0
// Bit 26  :24     rg_bt_txm_ctune_man            U     RW        default = 'h3
// Bit 30  :28     ro_bt_txm_ctune                U     RO        default = 'h3
// Bit 31          rg_bt_txm_ctune_cal_man_mode     U     RW        default = 'h0
typedef union RG_BT_TX_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 1;
    unsigned int RG_BT_TX_TP_SEL : 3;
    unsigned int rsvd_1 : 6;
    unsigned int RG_BT_TXM_LODRV : 4;
    unsigned int RG_BT_TX_PA_CTUNE : 3;
    unsigned int RG_BT_TX_IQDECT_CTGM : 3;
    unsigned int RG_BT_TX_IQDECT_CTLO : 3;
    unsigned int rsvd_2 : 1;
    unsigned int RG_BT_TXM_CTUNE_MAN : 3;
    unsigned int rsvd_3 : 1;
    unsigned int RO_BT_TXM_CTUNE : 3;
    unsigned int RG_BT_TXM_CTUNE_CAL_MAN_MODE : 1;
  } b;
} RG_BT_TX_A6_FIELD_T;

#define RG_BT_TX_A7                                  ( + 0x1c)
// Bit 18          rg_bt_tx_iq_cal_en             U     RW        default = 'h0
// Bit 20          rg_bt_tx_tp_en                 U     RW        default = 'h0
// Bit 21          rg_bt_txm_lc_cal_en            U     RW        default = 'h0
typedef union RG_BT_TX_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 18;
    unsigned int RG_BT_TX_IQ_CAL_EN : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BT_TX_TP_EN : 1;
    unsigned int RG_BT_TXM_LC_CAL_EN : 1;
    unsigned int rsvd_2 : 10;
  } b;
} RG_BT_TX_A7_FIELD_T;

#define RG_BT_TX_A8                                  ( + 0x20)
// Bit 15  :0      rg_bt_tx_rev                   U     RW        default = 'h0
// Bit 23  :16     rg_bt_txm_mx_en_delay_bnd      U     RW        default = 'h14
// Bit 24          rg_bt_txm_mx_en_delay_bypass     U     RW        default = 'h1
typedef union RG_BT_TX_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_REV : 16;
    unsigned int RG_BT_TXM_MX_EN_DELAY_BND : 8;
    unsigned int RG_BT_TXM_MX_EN_DELAY_BYPASS : 1;
    unsigned int rsvd_0 : 7;
  } b;
} RG_BT_TX_A8_FIELD_T;

#define RG_BT_TX_A9                                  ( + 0x24)
// Bit 7   :0      rg_bt_tx_gain_man              U     RW        default = 'h0
// Bit 8           rg_bt_tx_gain_man_mode         U     RW        default = 'h0
// Bit 12          rg_bt_tx_sw_en_d               U     RW        default = 'h1
// Bit 16          rg_bt_tx_sw_en_s               U     RW        default = 'h0
typedef union RG_BT_TX_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_GAIN_MAN : 8;
    unsigned int RG_BT_TX_GAIN_MAN_MODE : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_BT_TX_SW_EN_D : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_BT_TX_SW_EN_S : 1;
    unsigned int rsvd_2 : 15;
  } b;
} RG_BT_TX_A9_FIELD_T;

#define RG_BT_TX_A10                                 ( + 0x28)
// Bit 0           rg_bt_tx_pa_man_mode           U     RW        default = 'h0
// Bit 4   :1      rg_bt_tx_pa_gain_slide_man     U     RW        default = 'hf
// Bit 8   :5      rg_bt_tx_pa_gain_slide_g1      U     RW        default = 'h1
// Bit 12  :9      rg_bt_tx_pa_gain_slide_g2      U     RW        default = 'h3
// Bit 16  :13     rg_bt_tx_pa_gain_slide_g3      U     RW        default = 'h7
// Bit 20  :17     rg_bt_tx_pa_gain_slide_g4      U     RW        default = 'hf
typedef union RG_BT_TX_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_PA_MAN_MODE : 1;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_MAN : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G1 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G2 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G3 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G4 : 4;
    unsigned int rsvd_0 : 11;
  } b;
} RG_BT_TX_A10_FIELD_T;

#define RG_BT_TX_A11                                 ( + 0x2c)
// Bit 3   :0      rg_bt_tx_pa_vcm_capl           U     RW        default = 'hc
// Bit 6   :4      rg_bt_tx_pa_vcm_capr           U     RW        default = 'h1
// Bit 9   :7      rg_bt_tx_pa_vcm_res            U     RW        default = 'h7
// Bit 12  :10     rg_bt_txm_lc_atten             U     RW        default = 'h3
typedef union RG_BT_TX_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_PA_VCM_CAPL : 4;
    unsigned int RG_BT_TX_PA_VCM_CAPR : 3;
    unsigned int RG_BT_TX_PA_VCM_RES : 3;
    unsigned int RG_BT_TXM_LC_ATTEN : 3;
    unsigned int rsvd_0 : 19;
  } b;
} RG_BT_TX_A11_FIELD_T;

#define RG_BT_TX_A12                                 ( + 0x30)
// Bit 3   :0      rg_bt_tx_pa_gain_slide_g5      U     RW        default = 'h1
// Bit 7   :4      rg_bt_tx_pa_gain_slide_g6      U     RW        default = 'h3
// Bit 11  :8      rg_bt_tx_pa_gain_slide_g7      U     RW        default = 'h7
// Bit 15  :12     rg_bt_tx_pa_gain_slide_g8      U     RW        default = 'hf
// Bit 20  :16     rg_bt_tx_ibias_pa_vcm_man      U     RW        default = 'h5
// Bit 25  :21     rg_bt_tx_ibias_pa_vcm_g1       U     RW        default = 'h5
// Bit 30  :26     rg_bt_tx_ibias_pa_vcm_g2       U     RW        default = 'h5
typedef union RG_BT_TX_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G5 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G6 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G7 : 4;
    unsigned int RG_BT_TX_PA_GAIN_SLIDE_G8 : 4;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_MAN : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G1 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G2 : 5;
    unsigned int rsvd_0 : 1;
  } b;
} RG_BT_TX_A12_FIELD_T;

#define RG_BT_TX_A13                                 ( + 0x34)
// Bit 4   :0      rg_bt_tx_ibias_pa_vcm_g3       U     RW        default = 'h5
// Bit 9   :5      rg_bt_tx_ibias_pa_vcm_g4       U     RW        default = 'h5
// Bit 14  :10     rg_bt_tx_ibias_pa_vcm_g5       U     RW        default = 'h5
// Bit 19  :15     rg_bt_tx_ibias_pa_vcm_g6       U     RW        default = 'h5
// Bit 24  :20     rg_bt_tx_ibias_pa_vcm_g7       U     RW        default = 'h5
// Bit 29  :25     rg_bt_tx_ibias_pa_vcm_g8       U     RW        default = 'h5
typedef union RG_BT_TX_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G3 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G4 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G5 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G6 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G7 : 5;
    unsigned int RG_BT_TX_IBIAS_PA_VCM_G8 : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_BT_TX_A13_FIELD_T;

#define RG_BT_TX_A14                                 ( + 0x38)
// Bit 4   :0      rg_bt_tx_ibias_mix_vcm_g5      U     RW        default = 'h5
// Bit 9   :5      rg_bt_tx_ibias_mix_vcm_g6      U     RW        default = 'h5
// Bit 14  :10     rg_bt_tx_ibias_mix_vcm_g7      U     RW        default = 'h5
// Bit 19  :15     rg_bt_tx_ibias_mix_vcm_g8      U     RW        default = 'h5
typedef union RG_BT_TX_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G5 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G6 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G7 : 5;
    unsigned int RG_BT_TX_IBIAS_MIX_VCM_G8 : 5;
    unsigned int rsvd_0 : 12;
  } b;
} RG_BT_TX_A14_FIELD_T;

#define RG_BT_TX_A15                                 ( + 0x3c)
// Bit 0           rg_bt_tx_pa_en_delay_bypass     U     RW        default = 'h1
// Bit 16  :1      rg_bt_tx_pa_en_delay_bnd       U     RW        default = 'h14
typedef union RG_BT_TX_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TX_PA_EN_DELAY_BYPASS : 1;
    unsigned int RG_BT_TX_PA_EN_DELAY_BND : 16;
    unsigned int rsvd_0 : 15;
  } b;
} RG_BT_TX_A15_FIELD_T;

#endif

