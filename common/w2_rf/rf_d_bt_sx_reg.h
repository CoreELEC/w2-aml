#ifdef RF_D_BT_SX_REG
#else
#define RF_D_BT_SX_REG


#define RF_D_BT_SX_REG_BASE                       (0x80002100)

#define RG_BT_SX_A0                                  (0x0)
// Bit 31  :0      rg_bt_sx_rsv0                  U     RW        default = 'hff00
typedef union RG_BT_SX_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_RSV0 : 32;
  } b;
} RG_BT_SX_A0_FIELD_T;

#define RG_BT_SX_A1                                  (0x4)
// Bit 0           rg_bt_sx_clk_edge_sel          U     RW        default = 'h0
// Bit 1           rg_bt_sx_fb_clk_edge_sel       U     RW        default = 'h0
// Bit 2           rg_bt_sx_tp_en                 U     RW        default = 'h0
// Bit 5   :3      rg_bt_sx_tp_sel                U     RW        default = 'h0
// Bit 6           rg_bt_vco_vctrl_tp_sel         U     RW        default = 'h0
// Bit 7           rg_bt_sx_cp_bypass_mode        U     RW        default = 'h0
// Bit 8           rg_bt_sx_cp_cali_ctrl          U     RW        default = 'h0
// Bit 9           rg_bt_sx_cp_rc_ctrl            U     RW        default = 'h1
// Bit 10          rg_bt_sx_lpf_bypass_mode       U     RW        default = 'h0
// Bit 11          rg_bt_sx_lpf_cali_ctrl         U     RW        default = 'h0
// Bit 12          rg_bt_sx_lpf_resample_mode     U     RW        default = 'h1
// Bit 17  :13     rg_bt_sx_cp_ldo_vref_adj       U     RW        default = 'h10
// Bit 20  :18     rg_bt_sx_lpf_bw                U     RW        default = 'h5
// Bit 22  :21     rg_bt_sx_pfd_delay             U     RW        default = 'h0
// Bit 26  :23     rg_bt_sx_cp_i                  U     RW        default = 'h3
// Bit 28  :27     rg_bt_sx_pfd_lock_t            U     RW        default = 'h0
// Bit 29          rg_bt_sx_pfd_ph_en             U     RW        default = 'h0
// Bit 30          rg_bt_sx_pfd_sel               U     RW        default = 'h1
typedef union RG_BT_SX_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_CLK_EDGE_SEL : 1;
    unsigned int RG_BT_SX_FB_CLK_EDGE_SEL : 1;
    unsigned int RG_BT_SX_TP_EN : 1;
    unsigned int RG_BT_SX_TP_SEL : 3;
    unsigned int RG_BT_VCO_VCTRL_TP_SEL : 1;
    unsigned int RG_BT_SX_CP_BYPASS_MODE : 1;
    unsigned int RG_BT_SX_CP_CALI_CTRL : 1;
    unsigned int RG_BT_SX_CP_RC_CTRL : 1;
    unsigned int RG_BT_SX_LPF_BYPASS_MODE : 1;
    unsigned int RG_BT_SX_LPF_CALI_CTRL : 1;
    unsigned int RG_BT_SX_LPF_RESAMPLE_MODE : 1;
    unsigned int RG_BT_SX_CP_LDO_VREF_ADJ : 5;
    unsigned int RG_BT_SX_LPF_BW : 3;
    unsigned int RG_BT_SX_PFD_DELAY : 2;
    unsigned int RG_BT_SX_CP_I : 4;
    unsigned int RG_BT_SX_PFD_LOCK_T : 2;
    unsigned int RG_BT_SX_PFD_PH_EN : 1;
    unsigned int RG_BT_SX_PFD_SEL : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_BT_SX_A1_FIELD_T;

#define RG_BT_SX_A2                                  (0x8)
// Bit 3   :0      rg_bt_sx_cp_offset             U     RW        default = 'h6
// Bit 8   :4      rg_bt_sx_vco_ldo_vref_adj      U     RW        default = 'h10
// Bit 9           rg_bt_sx_fcal_div              U     RW        default = 'h0
// Bit 10          rg_bt_sx_mmd_pw_en             U     RW        default = 'h1
// Bit 11          rg_bt_sx_sdm_frac_en           U     RW        default = 'h1
// Bit 13  :12     rg_bt_sx_fcal_hvt              U     RW        default = 'h1
// Bit 15  :14     rg_bt_sx_fcal_lvt              U     RW        default = 'h1
// Bit 16          rg_bt_sx_ldo_hf_rc_ctrl        U     RW        default = 'h1
// Bit 17          rg_bt_sx_ldo_hf_rc_modesel     U     RW        default = 'h0
// Bit 18          rg_bt_sx_ldo_lf_rc_ctrl        U     RW        default = 'h1
// Bit 19          rg_bt_sx_ldo_lf_rc_modesel     U     RW        default = 'h0
// Bit 20          rg_bt_sx_vco_lpf_testmode      U     RW        default = 'h0
// Bit 22  :21     rg_bt_sx_vco_var_dc            U     RW        default = 'h0
// Bit 24  :23     rg_bt_sx_vctrl_sel             U     RW        default = 'h0
// Bit 28  :25     rg_bt_sx_vco_i                 U     RW        default = 'h8
typedef union RG_BT_SX_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_CP_OFFSET : 4;
    unsigned int RG_BT_SX_VCO_LDO_VREF_ADJ : 5;
    unsigned int RG_BT_SX_FCAL_DIV : 1;
    unsigned int RG_BT_SX_MMD_PW_EN : 1;
    unsigned int RG_BT_SX_SDM_FRAC_EN : 1;
    unsigned int RG_BT_SX_FCAL_HVT : 2;
    unsigned int RG_BT_SX_FCAL_LVT : 2;
    unsigned int RG_BT_SX_LDO_HF_RC_CTRL : 1;
    unsigned int RG_BT_SX_LDO_HF_RC_MODESEL : 1;
    unsigned int RG_BT_SX_LDO_LF_RC_CTRL : 1;
    unsigned int RG_BT_SX_LDO_LF_RC_MODESEL : 1;
    unsigned int RG_BT_SX_VCO_LPF_TESTMODE : 1;
    unsigned int RG_BT_SX_VCO_VAR_DC : 2;
    unsigned int RG_BT_SX_VCTRL_SEL : 2;
    unsigned int RG_BT_SX_VCO_I : 4;
    unsigned int rsvd_0 : 3;
  } b;
} RG_BT_SX_A2_FIELD_T;

#define RG_BT_SX_A3                                  (0xc)
// Bit 0           rg_m0_bt_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 1           rg_m0_bt_sx_log2g_div_en       U     RW        default = 'h0
// Bit 2           rg_m0_bt_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 3           rg_m0_bt_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 4           rg_m0_bt_sx_log2g_txlo_en      U     RW        default = 'h0
// Bit 5           rg_m0_bt_sx_vco_en             U     RW        default = 'h0
// Bit 6           rg_m0_bt_sx_pfd_en             U     RW        default = 'h0
// Bit 7           rg_m0_bt_sx_cp_en              U     RW        default = 'h0
// Bit 8           rg_m0_bt_sx_vco_ldo_en         U     RW        default = 'h0
// Bit 9           rg_m0_bt_sx_cp_ldo_en          U     RW        default = 'h0
// Bit 10          rg_m0_bt_sx_lpf_comp_en        U     RW        default = 'h0
typedef union RG_BT_SX_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_BT_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M0_BT_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M0_BT_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M0_BT_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M0_BT_SX_LOG2G_TXLO_EN : 1;
    unsigned int RG_M0_BT_SX_VCO_EN : 1;
    unsigned int RG_M0_BT_SX_PFD_EN : 1;
    unsigned int RG_M0_BT_SX_CP_EN : 1;
    unsigned int RG_M0_BT_SX_VCO_LDO_EN : 1;
    unsigned int RG_M0_BT_SX_CP_LDO_EN : 1;
    unsigned int RG_M0_BT_SX_LPF_COMP_EN : 1;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A3_FIELD_T;

#define RG_BT_SX_A4                                  (0x10)
// Bit 0           rg_m1_bt_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 1           rg_m1_bt_sx_log2g_div_en       U     RW        default = 'h1
// Bit 2           rg_m1_bt_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 3           rg_m1_bt_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 4           rg_m1_bt_sx_log2g_txlo_en      U     RW        default = 'h0
// Bit 5           rg_m1_bt_sx_vco_en             U     RW        default = 'h1
// Bit 6           rg_m1_bt_sx_pfd_en             U     RW        default = 'h1
// Bit 7           rg_m1_bt_sx_cp_en              U     RW        default = 'h1
// Bit 8           rg_m1_bt_sx_vco_ldo_en         U     RW        default = 'h1
// Bit 9           rg_m1_bt_sx_cp_ldo_en          U     RW        default = 'h1
// Bit 10          rg_m1_bt_sx_lpf_comp_en        U     RW        default = 'h1
typedef union RG_BT_SX_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M1_BT_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M1_BT_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M1_BT_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M1_BT_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M1_BT_SX_LOG2G_TXLO_EN : 1;
    unsigned int RG_M1_BT_SX_VCO_EN : 1;
    unsigned int RG_M1_BT_SX_PFD_EN : 1;
    unsigned int RG_M1_BT_SX_CP_EN : 1;
    unsigned int RG_M1_BT_SX_VCO_LDO_EN : 1;
    unsigned int RG_M1_BT_SX_CP_LDO_EN : 1;
    unsigned int RG_M1_BT_SX_LPF_COMP_EN : 1;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A4_FIELD_T;

#define RG_BT_SX_A5                                  (0x14)
// Bit 0           rg_m2_bt_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 1           rg_m2_bt_sx_log2g_div_en       U     RW        default = 'h1
// Bit 2           rg_m2_bt_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 3           rg_m2_bt_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 4           rg_m2_bt_sx_log2g_txlo_en      U     RW        default = 'h1
// Bit 5           rg_m2_bt_sx_vco_en             U     RW        default = 'h1
// Bit 6           rg_m2_bt_sx_pfd_en             U     RW        default = 'h1
// Bit 7           rg_m2_bt_sx_cp_en              U     RW        default = 'h1
// Bit 8           rg_m2_bt_sx_vco_ldo_en         U     RW        default = 'h1
// Bit 9           rg_m2_bt_sx_cp_ldo_en          U     RW        default = 'h1
// Bit 10          rg_m2_bt_sx_lpf_comp_en        U     RW        default = 'h1
typedef union RG_BT_SX_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M2_BT_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M2_BT_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M2_BT_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M2_BT_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M2_BT_SX_LOG2G_TXLO_EN : 1;
    unsigned int RG_M2_BT_SX_VCO_EN : 1;
    unsigned int RG_M2_BT_SX_PFD_EN : 1;
    unsigned int RG_M2_BT_SX_CP_EN : 1;
    unsigned int RG_M2_BT_SX_VCO_LDO_EN : 1;
    unsigned int RG_M2_BT_SX_CP_LDO_EN : 1;
    unsigned int RG_M2_BT_SX_LPF_COMP_EN : 1;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A5_FIELD_T;

#define RG_BT_SX_A6                                  (0x18)
// Bit 0           rg_m3_bt_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 1           rg_m3_bt_sx_log2g_div_en       U     RW        default = 'h1
// Bit 2           rg_m3_bt_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 3           rg_m3_bt_sx_log2g_rxlo_en      U     RW        default = 'h1
// Bit 4           rg_m3_bt_sx_log2g_txlo_en      U     RW        default = 'h0
// Bit 5           rg_m3_bt_sx_vco_en             U     RW        default = 'h1
// Bit 6           rg_m3_bt_sx_pfd_en             U     RW        default = 'h1
// Bit 7           rg_m3_bt_sx_cp_en              U     RW        default = 'h1
// Bit 8           rg_m3_bt_sx_vco_ldo_en         U     RW        default = 'h1
// Bit 9           rg_m3_bt_sx_cp_ldo_en          U     RW        default = 'h1
// Bit 10          rg_m3_bt_sx_lpf_comp_en        U     RW        default = 'h1
typedef union RG_BT_SX_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M3_BT_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M3_BT_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M3_BT_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M3_BT_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M3_BT_SX_LOG2G_TXLO_EN : 1;
    unsigned int RG_M3_BT_SX_VCO_EN : 1;
    unsigned int RG_M3_BT_SX_PFD_EN : 1;
    unsigned int RG_M3_BT_SX_CP_EN : 1;
    unsigned int RG_M3_BT_SX_VCO_LDO_EN : 1;
    unsigned int RG_M3_BT_SX_CP_LDO_EN : 1;
    unsigned int RG_M3_BT_SX_LPF_COMP_EN : 1;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A6_FIELD_T;

#define RG_BT_SX_A7                                  (0x1c)
// Bit 3   :0      rg_bt_sx_log2g_mxr_vdd         U     RW        default = 'h7
// Bit 7   :4      rg_bt_sx_log2g_div_vdd         U     RW        default = 'h1
// Bit 11  :8      rg_bt_sx_log2g_iqdiv_vdd       U     RW        default = 'h1
// Bit 15  :12     rg_bt_sx_log2g_rxlo_vdd        U     RW        default = 'h3
// Bit 23  :20     rg_bt_sx_log2g_txlo_vdd        U     RW        default = 'h3
// Bit 27  :24     rg_bt_sx_log2g_mxr_banksel_man     U     RW        default = 'h0
// Bit 31  :28     rg_bt_sx_log2g_reserve         U     RW        default = 'h0
typedef union RG_BT_SX_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_LOG2G_MXR_VDD : 4;
    unsigned int RG_BT_SX_LOG2G_DIV_VDD : 4;
    unsigned int RG_BT_SX_LOG2G_IQDIV_VDD : 4;
    unsigned int RG_BT_SX_LOG2G_RXLO_VDD : 4;
    unsigned int rsvd_0 : 4;
    unsigned int RG_BT_SX_LOG2G_TXLO_VDD : 4;
    unsigned int RG_BT_SX_LOG2G_MXR_BANKSEL_MAN : 4;
    unsigned int RG_BT_SX_LOG2G_RESERVE : 4;
  } b;
} RG_BT_SX_A7_FIELD_T;

#define RG_BT_SX_A8                                  (0x20)
// Bit 0           rg_bt_sx_fcal_en               U     RW        default = 'h0
// Bit 2           rg_bt_sx_cali_start_man_mode     U     RW        default = 'h0
// Bit 7   :4      rg_bt_sx_fcal_mode_sel         U     RW        default = 'hf
// Bit 8           rg_bt_sx_soft_reset            U     RW        default = 'h0
// Bit 27  :12     rg_bt_sx_fcal_ntargt           U     RW        default = 'h0
// Bit 31          rg_bt_sx_fcal_ntargt_man       U     RW        default = 'h0
typedef union RG_BT_SX_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_FCAL_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BT_SX_CALI_START_MAN_MODE : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BT_SX_FCAL_MODE_SEL : 4;
    unsigned int RG_BT_SX_SOFT_RESET : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RG_BT_SX_FCAL_NTARGT : 16;
    unsigned int rsvd_3 : 3;
    unsigned int RG_BT_SX_FCAL_NTARGT_MAN : 1;
  } b;
} RG_BT_SX_A8_FIELD_T;

#define RG_BT_SX_A9                                  (0x24)
// Bit 0           rg_bt_sx_fcal_sel_p            U     RW        default = 'h0
// Bit 10  :4      rg_bt_sx_vcoc_p                U     RW        default = 'h40
// Bit 12          rg_bt_sx_fcal_sel_t            U     RW        default = 'h0
// Bit 20  :16     rg_bt_sx_vcoc_t                U     RW        default = 'h10
// Bit 24          rg_bt_sx_ldo_fc_man            U     RW        default = 'h0
// Bit 28          rg_bt_sx_ldo_fc                U     RW        default = 'h0
typedef union RG_BT_SX_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_FCAL_SEL_P : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_BT_SX_VCOC_P : 7;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BT_SX_FCAL_SEL_T : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RG_BT_SX_VCOC_T : 5;
    unsigned int rsvd_3 : 3;
    unsigned int RG_BT_SX_LDO_FC_MAN : 1;
    unsigned int rsvd_4 : 3;
    unsigned int RG_BT_SX_LDO_FC : 1;
    unsigned int rsvd_5 : 3;
  } b;
} RG_BT_SX_A9_FIELD_T;

#define RG_BT_SX_A10                                 (0x28)
// Bit 6   :0      ro_da_bt_sx_vcoc_p             U     RO        default = 'h0
// Bit 12  :8      ro_da_bt_sx_vcoc_t             U     RO        default = 'h0
// Bit 31  :16     ro_bt_sx_fcal_ntargt           U     RO        default = 'h0
typedef union RG_BT_SX_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_DA_BT_SX_VCOC_P : 7;
    unsigned int rsvd_0 : 1;
    unsigned int RO_DA_BT_SX_VCOC_T : 5;
    unsigned int rsvd_1 : 3;
    unsigned int RO_BT_SX_FCAL_NTARGT : 16;
  } b;
} RG_BT_SX_A10_FIELD_T;

#define RG_BT_SX_A11                                 (0x2c)
// Bit 10  :0      rg_bt_sx_rfctrl_int            U     RW        default = 'h0
typedef union RG_BT_SX_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_RFCTRL_INT : 11;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A11_FIELD_T;

#define RG_BT_SX_A12                                 (0x30)
// Bit 24  :0      rg_bt_sx_rfctrl_frac           U     RW        default = 'h0
typedef union RG_BT_SX_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_RFCTRL_FRAC : 25;
    unsigned int rsvd_0 : 7;
  } b;
} RG_BT_SX_A12_FIELD_T;

#define RG_BT_SX_A13                                 (0x34)
// Bit 10  :0      ro_bt_sx_rfctrl_nint           U     RO        default = 'h0
typedef union RG_BT_SX_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BT_SX_RFCTRL_NINT : 11;
    unsigned int rsvd_0 : 21;
  } b;
} RG_BT_SX_A13_FIELD_T;

#define RG_BT_SX_A14                                 (0x38)
// Bit 24  :0      ro_bt_sx_rfctrl_nfrac          U     RO        default = 'h0
typedef union RG_BT_SX_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BT_SX_RFCTRL_NFRAC : 25;
    unsigned int rsvd_0 : 7;
  } b;
} RG_BT_SX_A14_FIELD_T;

#define RG_BT_SX_A15                                 (0x3c)
// Bit 15  :0      ro_bt_sx_fcal_cnt              U     RO        default = 'h0
typedef union RG_BT_SX_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BT_SX_FCAL_CNT : 16;
    unsigned int rsvd_0 : 16;
  } b;
} RG_BT_SX_A15_FIELD_T;

#define RG_BT_SX_A16                                 (0x40)
// Bit 0           rg_bt_sx_pll_rst_man           U     RW        default = 'h0
// Bit 1           rg_bt_sx_pll_rst_man_mode      U     RW        default = 'h0
// Bit 4   :2      rg_bt_sx_log2g_pdthreshold_man     U     RW        default = 'h7
// Bit 5           rg_bt_sx_log2g_pd_man_mode     U     RW        default = 'h0
// Bit 6           rg_bt_sx_log2g_pd_en_man       U     RW        default = 'h0
// Bit 7           rg_bt_sx_log2g_pdtest_en       U     RW        default = 'h0
// Bit 15  :12     ro_bt_sx_log2g_mxr_banksel     U     RO        default = 'h0
typedef union RG_BT_SX_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_PLL_RST_MAN : 1;
    unsigned int RG_BT_SX_PLL_RST_MAN_MODE : 1;
    unsigned int RG_BT_SX_LOG2G_PDTHRESHOLD_MAN : 3;
    unsigned int RG_BT_SX_LOG2G_PD_MAN_MODE : 1;
    unsigned int RG_BT_SX_LOG2G_PD_EN_MAN : 1;
    unsigned int RG_BT_SX_LOG2G_PDTEST_EN : 1;
    unsigned int rsvd_0 : 4;
    unsigned int RO_BT_SX_LOG2G_MXR_BANKSEL : 4;
    unsigned int rsvd_1 : 16;
  } b;
} RG_BT_SX_A16_FIELD_T;

#define RG_BT_SX_A17                                 (0x44)
// Bit 1   :0      rg_bt_sx_ldo_fc_waittime       U     RW        default = 'h2
// Bit 5   :4      rg_bt_sx_set_code_waittime     U     RW        default = 'h0
// Bit 9   :8      rg_bt_sx_fcal_cw_waittime      U     RW        default = 'h0
// Bit 13  :12     rg_bt_sx_tr_wait_waittime      U     RW        default = 'h1
// Bit 21  :20     rg_bt_sx_tr_switch_bnd         U     RW        default = 'h3
typedef union RG_BT_SX_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_LDO_FC_WAITTIME : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RG_BT_SX_SET_CODE_WAITTIME : 2;
    unsigned int rsvd_1 : 2;
    unsigned int RG_BT_SX_FCAL_CW_WAITTIME : 2;
    unsigned int rsvd_2 : 2;
    unsigned int RG_BT_SX_TR_WAIT_WAITTIME : 2;
    unsigned int rsvd_3 : 6;
    unsigned int RG_BT_SX_TR_SWITCH_BND : 2;
    unsigned int rsvd_4 : 10;
  } b;
} RG_BT_SX_A17_FIELD_T;

#define RG_BT_SX_A18                                 (0x48)
// Bit 0           rg_bt_sx_btrx_side             U     RW        default = 'h0
// Bit 1           rg_bt_sx_pdiv_sel              U     RW        default = 'h0
// Bit 14  :2      rg_bt_sx_fif_ost               U     RW        default = 'h7d0
// Bit 22  :15     rg_bt_sx_bt_channel            U     RW        default = 'h27
// Bit 27  :24     rg_bt_sx_osc_freq              U     RW        default = 'h5
// Bit 28          rg_bt_sx_rfch_man_en           U     RW        default = 'h0
typedef union RG_BT_SX_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_BTRX_SIDE : 1;
    unsigned int RG_BT_SX_PDIV_SEL : 1;
    unsigned int RG_BT_SX_FIF_OST : 13;
    unsigned int RG_BT_SX_BT_CHANNEL : 8;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BT_SX_OSC_FREQ : 4;
    unsigned int RG_BT_SX_RFCH_MAN_EN : 1;
    unsigned int rsvd_1 : 3;
  } b;
} RG_BT_SX_A18_FIELD_T;

#define RG_BT_SX_A19                                 (0x4c)
// Bit 0           rg_bt_sx_tr_switch_man_mode     U     RW        default = 'h0
// Bit 1           rg_bt_sx_tr_switch_man         U     RW        default = 'h0
// Bit 2           rg_bt_sx_stable_en_man_mode     U     RW        default = 'h0
// Bit 3           rg_bt_sx_stable_en_man         U     RW        default = 'h0
// Bit 5   :4      rg_bt_sx_stable_cnt            U     RW        default = 'h0
// Bit 8           rg_bt_sx_chmap_tx_man_mode     U     RW        default = 'h0
// Bit 9           rg_bt_sx_chmap_tx_man          U     RW        default = 'h1
typedef union RG_BT_SX_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_SX_TR_SWITCH_MAN_MODE : 1;
    unsigned int RG_BT_SX_TR_SWITCH_MAN : 1;
    unsigned int RG_BT_SX_STABLE_EN_MAN_MODE : 1;
    unsigned int RG_BT_SX_STABLE_EN_MAN : 1;
    unsigned int RG_BT_SX_STABLE_CNT : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RG_BT_SX_CHMAP_TX_MAN_MODE : 1;
    unsigned int RG_BT_SX_CHMAP_TX_MAN : 1;
    unsigned int rsvd_1 : 22;
  } b;
} RG_BT_SX_A19_FIELD_T;

#define RG_BT_SX_A20                                 (0x50)
// Bit 3   :0      rg_bt_ttg_vdd                  U     RW        default = 'h8
// Bit 4           rg_bt_ttg_post_div2_en_man     U     RW        default = 'h0
// Bit 5           rg_bt_sx_log2g_div4_en_man     U     RW        default = 'h1
// Bit 6           rg_bt_lo_ind_sw_en_man         U     RW        default = 'h0
// Bit 8           rg_bt_ttg_post_div2_en_m0      U     RW        default = 'h0
// Bit 9           rg_bt_sx_log2g_div4_en_m0      U     RW        default = 'h1
// Bit 10          rg_bt_lo_ind_sw_en_m0          U     RW        default = 'h0
// Bit 12          rg_bt_ttg_post_div2_en_m1      U     RW        default = 'h1
// Bit 13          rg_bt_sx_log2g_div4_en_m1      U     RW        default = 'h1
// Bit 14          rg_bt_lo_ind_sw_en_m1          U     RW        default = 'h0
// Bit 16          rg_bt_ttg_post_div2_en_m2      U     RW        default = 'h0
// Bit 17          rg_bt_sx_log2g_div4_en_m2      U     RW        default = 'h1
// Bit 18          rg_bt_lo_ind_sw_en_m2          U     RW        default = 'h1
// Bit 20          rg_bt_ttg_post_div2_en_m3      U     RW        default = 'h0
// Bit 21          rg_bt_sx_log2g_div4_en_m3      U     RW        default = 'h0
// Bit 22          rg_bt_lo_ind_sw_en_m3          U     RW        default = 'h1
// Bit 24          rg_bt_ttg_post_div2_en_man_mode     U     RW        default = 'h0
// Bit 25          rg_bt_sx_log2g_div4_en_man_mode     U     RW        default = 'h0
// Bit 26          rg_bt_lo_ind_sw_en_man_mode     U     RW        default = 'h0
// Bit 27          rg_bt_ttg_mode_man_mode        U     RW        default = 'h0
// Bit 29  :28     rg_bt_ttg_mode_man             U     RW        default = 'h0
// Bit 31  :30     ro_bt_ttg_mode                 U     RO        default = 'h0
typedef union RG_BT_SX_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_TTG_VDD : 4;
    unsigned int RG_BT_TTG_POST_DIV2_EN_MAN : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_MAN : 1;
    unsigned int RG_BT_LO_IND_SW_EN_MAN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_BT_TTG_POST_DIV2_EN_M0 : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_M0 : 1;
    unsigned int RG_BT_LO_IND_SW_EN_M0 : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_BT_TTG_POST_DIV2_EN_M1 : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_M1 : 1;
    unsigned int RG_BT_LO_IND_SW_EN_M1 : 1;
    unsigned int rsvd_2 : 1;
    unsigned int RG_BT_TTG_POST_DIV2_EN_M2 : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_M2 : 1;
    unsigned int RG_BT_LO_IND_SW_EN_M2 : 1;
    unsigned int rsvd_3 : 1;
    unsigned int RG_BT_TTG_POST_DIV2_EN_M3 : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_M3 : 1;
    unsigned int RG_BT_LO_IND_SW_EN_M3 : 1;
    unsigned int rsvd_4 : 1;
    unsigned int RG_BT_TTG_POST_DIV2_EN_MAN_MODE : 1;
    unsigned int RG_BT_SX_LOG2G_DIV4_EN_MAN_MODE : 1;
    unsigned int RG_BT_LO_IND_SW_EN_MAN_MODE : 1;
    unsigned int RG_BT_TTG_MODE_MAN_MODE : 1;
    unsigned int RG_BT_TTG_MODE_MAN : 2;
    unsigned int RO_BT_TTG_MODE : 2;
  } b;
} RG_BT_SX_A20_FIELD_T;

#define RG_BT_SX_A21                                 (0x54)
// Bit 12  :0      ro_bt_sx_rf_freq_mhz           U     RO        default = 'h0
// Bit 16          ro_bt_sx_chn_map_ready         U     RO        default = 'h0
// Bit 17          ro_bt_sx_p_cali_ready          U     RO        default = 'h0
// Bit 18          ro_bt_sx_t_cali_ready          U     RO        default = 'h0
// Bit 19          ro_bt_sx_mxrtank_cali_ready     U     RO        default = 'h0
// Bit 20          ro_bt_sx_cali_end_flg          U     RO        default = 'h0
// Bit 21          ro_ad_bt_sx_ready              U     RO        default = 'h0
// Bit 22          ro_ad_bt_sx_fcal_inc_up        U     RO        default = 'h0
// Bit 23          ro_ad_bt_sx_fcal_inc_dn        U     RO        default = 'h0
// Bit 29  :24     ro_bt_sx_top_fsm               U     RO        default = 'h0
typedef union RG_BT_SX_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BT_SX_RF_FREQ_MHZ : 13;
    unsigned int rsvd_0 : 3;
    unsigned int RO_BT_SX_CHN_MAP_READY : 1;
    unsigned int RO_BT_SX_P_CALI_READY : 1;
    unsigned int RO_BT_SX_T_CALI_READY : 1;
    unsigned int RO_BT_SX_MXRTANK_CALI_READY : 1;
    unsigned int RO_BT_SX_CALI_END_FLG : 1;
    unsigned int RO_AD_BT_SX_READY : 1;
    unsigned int RO_AD_BT_SX_FCAL_INC_UP : 1;
    unsigned int RO_AD_BT_SX_FCAL_INC_DN : 1;
    unsigned int RO_BT_SX_TOP_FSM : 6;
    unsigned int rsvd_1 : 2;
  } b;
} RG_BT_SX_A21_FIELD_T;

#define RG_BT_SX_A22                                 (0x58)
// Bit 3   :0      rg_bt_lo_acbuf_vdd             U     RW        default = 'h8
// Bit 5   :4      rg_bt_sx_log2g_vbias_p_ct      U     RW        default = 'h1
// Bit 7   :6      rg_bt_sx_log2g_vbias_n_ct      U     RW        default = 'h1
// Bit 21  :8      ro_bt_ttg_rf_freq_mhz          U     RO        default = 'h0
// Bit 22          ro_bt_ttg_ready                U     RO        default = 'h0
// Bit 23          rg_bt_sx_ttg_cali_start_man_mode     U     RW        default = 'h0
// Bit 24          rg_bt_sx_ttg_mux               U     RW        default = 'h1
typedef union RG_BT_SX_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_LO_ACBUF_VDD : 4;
    unsigned int RG_BT_SX_LOG2G_VBIAS_P_CT : 2;
    unsigned int RG_BT_SX_LOG2G_VBIAS_N_CT : 2;
    unsigned int RO_BT_TTG_RF_FREQ_MHZ : 14;
    unsigned int RO_BT_TTG_READY : 1;
    unsigned int RG_BT_SX_TTG_CALI_START_MAN_MODE : 1;
    unsigned int RG_BT_SX_TTG_MUX : 1;
    unsigned int rsvd_0 : 7;
  } b;
} RG_BT_SX_A22_FIELD_T;

#endif

