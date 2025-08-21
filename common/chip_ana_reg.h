#ifdef CHIP_ANA_REG
#else
#define CHIP_ANA_REG


#define CHIP_ANA_REG_BASE                         (0xf01000)

#define RG_DPLL_A0                                (CHIP_ANA_REG_BASE + 0x0)
// Bit 8   :0      rg_bbpll_div_n                 U     RW        default = 'h60
// Bit 31  :16     rg_bbpll_reve                  U     RW        default = 'h0
typedef union RG_DPLL_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_div_n : 9;
    unsigned int rsvd_0 : 7;
    unsigned int rg_bbpll_reve : 16;
  } b;
} RG_DPLL_A0_FIELD_T;

#define RG_DPLL_A1                                (CHIP_ANA_REG_BASE + 0x4)
// Bit 0           rg_bbpll_en                    U     RW        default = 'h0
// Bit 1           rg_bbpll_rst                   U     RW        default = 'h1
// Bit 8           rg_bbpll_fr_en                 U     RW        default = 'h0
// Bit 10  :9      rg_bbpll_fr_adj                U     RW        default = 'h2
// Bit 11          rg_bbpll_cp_en                 U     RW        default = 'h1
typedef union RG_DPLL_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_en : 1;
    unsigned int rg_bbpll_rst : 1;
    unsigned int rsvd_0 : 6;
    unsigned int rg_bbpll_fr_en : 1;
    unsigned int rg_bbpll_fr_adj : 2;
    unsigned int rg_bbpll_cp_en : 1;
    unsigned int rsvd_1 : 20;
  } b;
} RG_DPLL_A1_FIELD_T;

#define RG_DPLL_A2                                (CHIP_ANA_REG_BASE + 0x8)
// Bit 5   :0      rg_bbpll_ibn_adj               U     RW        default = 'h10
// Bit 13  :8      rg_bbpll_ibp_adj               U     RW        default = 'h10
// Bit 19  :16     rg_bbpll_r2_cnt                U     RW        default = 'h2
// Bit 24          rg_bbpll_vref_adj              U     RW        default = 'h0
// Bit 26  :25     rg_bbpll_dt_sel                U     RW        default = 'h0
typedef union RG_DPLL_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_ibn_adj : 6;
    unsigned int rsvd_0 : 2;
    unsigned int rg_bbpll_ibp_adj : 6;
    unsigned int rsvd_1 : 2;
    unsigned int rg_bbpll_r2_cnt : 4;
    unsigned int rsvd_2 : 4;
    unsigned int rg_bbpll_vref_adj : 1;
    unsigned int rg_bbpll_dt_sel : 2;
    unsigned int rsvd_3 : 5;
  } b;
} RG_DPLL_A2_FIELD_T;

#define RG_DPLL_A3                                (CHIP_ANA_REG_BASE + 0xc)
// Bit 1   :0      rg_bbpll_lk_w_sel              U     RW        default = 'h0
// Bit 2           rg_bbpll_lk_clk_gate           U     RW        default = 'h0
// Bit 3           rg_bbpll_lk_lock_long          U     RW        default = 'h0
// Bit 4           rg_bbpll_lk_lock_f             U     RW        default = 'h0
// Bit 5           rg_bbpll_lk_rst                U     RW        default = 'h1
// Bit 26  :8      rg_bbpll_sdm_fra               U     RW        default = 'h0
typedef union RG_DPLL_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_lk_w_sel : 2;
    unsigned int rg_bbpll_lk_clk_gate : 1;
    unsigned int rg_bbpll_lk_lock_long : 1;
    unsigned int rg_bbpll_lk_lock_f : 1;
    unsigned int rg_bbpll_lk_rst : 1;
    unsigned int rsvd_0 : 2;
    unsigned int rg_bbpll_sdm_fra : 19;
    unsigned int rsvd_1 : 5;
  } b;
} RG_DPLL_A3_FIELD_T;

#define RG_DPLL_A4                                (CHIP_ANA_REG_BASE + 0x10)
// Bit 0           rg_bbpll_wadc_clk_en           U     RW        default = 'h1
// Bit 1           rg_bbpll_wdac_clk_en           U     RW        default = 'h1
// Bit 2           rg_bbpll_wdac_clk_sel          U     RW        default = 'h0
// Bit 3           rg_bbpll_btadc_clk_en          U     RW        default = 'h1
// Bit 4           rg_bbpll_test_en               U     RW        default = 'h0
typedef union RG_DPLL_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_wadc_clk_en : 1;
    unsigned int rg_bbpll_wdac_clk_en : 1;
    unsigned int rg_bbpll_wdac_clk_sel : 1;
    unsigned int rg_bbpll_btadc_clk_en : 1;
    unsigned int rg_bbpll_test_en : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_DPLL_A4_FIELD_T;

#define RG_DPLL_A5                                (CHIP_ANA_REG_BASE + 0x14)
// Bit 0           rg_bbpll_ssc_en                U     RW        default = 'h0
// Bit 3   :1      rg_bbpll_ssc_fref              U     RW        default = 'h0
// Bit 5   :4      rg_bbpll_ssc_os                U     RW        default = 'h0
// Bit 6           rg_bbpll_ssc_load_en           U     RW        default = 'h1
// Bit 7           rg_bbpll_ssc_load              U     RW        default = 'h1
// Bit 8           rg_bbpll_ssc_shift_en          U     RW        default = 'h0
// Bit 12  :9      rg_bbpll_ssc_str_m             U     RW        default = 'h0
// Bit 14  :13     rg_bbpll_ssc_mode              U     RW        default = 'h0
// Bit 16  :15     rg_bbpll_ssc_shift_v           U     RW        default = 'h0
// Bit 20  :17     rg_bbpll_ssc_dep               U     RW        default = 'h0
// Bit 21          rg_bbpll_sdm_en                U     RW        default = 'h0
typedef union RG_DPLL_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_ssc_en : 1;
    unsigned int rg_bbpll_ssc_fref : 3;
    unsigned int rg_bbpll_ssc_os : 2;
    unsigned int rg_bbpll_ssc_load_en : 1;
    unsigned int rg_bbpll_ssc_load : 1;
    unsigned int rg_bbpll_ssc_shift_en : 1;
    unsigned int rg_bbpll_ssc_str_m : 4;
    unsigned int rg_bbpll_ssc_mode : 2;
    unsigned int rg_bbpll_ssc_shift_v : 2;
    unsigned int rg_bbpll_ssc_dep : 4;
    unsigned int rg_bbpll_sdm_en : 1;
    unsigned int rsvd_0 : 10;
  } b;
} RG_DPLL_A5_FIELD_T;

#define RG_DPLL_A6                                (CHIP_ANA_REG_BASE + 0x1c)
// Bit 28          ro_bbpll_fb_clk_done           U     RO        default = 'h0
// Bit 29          ro_bbpll_ref_clk_done          U     RO        default = 'h0
// Bit 30          ro_bbpll_vco_clk_done          U     RO        default = 'h0
// Bit 31          ro_bbpll_done                  U     RO        default = 'h0
typedef union RG_DPLL_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 28;
    unsigned int ro_bbpll_fb_clk_done : 1;
    unsigned int ro_bbpll_ref_clk_done : 1;
    unsigned int ro_bbpll_vco_clk_done : 1;
    unsigned int ro_bbpll_done : 1;
  } b;
} RG_DPLL_A6_FIELD_T;

#define RG_XOSC_A7                                (CHIP_ANA_REG_BASE + 0x20)
// Bit 0           rg_xo_bg_en_man                U     RW        default = 'h1
// Bit 2           rg_xo_bg_en_man_sel            U     RW        default = 'h0
// Bit 3           rg_xo_ldo_bypass_en            U     RW        default = 'h0
// Bit 4           rg_xo_ldo_en_man               U     RW        default = 'h1
// Bit 5           rg_xo_ldo_en_man_sel           U     RW        default = 'h0
// Bit 7   :6      rg_xo_ldo_fbr_ctrl             U     RW        default = 'h2
// Bit 13  :8      rg_xo_ldo_vref_cal             U     RW        default = 'h10
// Bit 14          rg_xo_en_man                   U     RW        default = 'h1
// Bit 15          rg_xo_en_man_sel               U     RW        default = 'h0
// Bit 16          rg_xo_sel                      U     RW        default = 'h0
// Bit 21  :17     rg_xo_core_bias_rcal_ctrl      U     RW        default = 'h10
// Bit 22          rg_xo_buf_wf2g_en_man_sel      U     RW        default = 'h0
// Bit 23          rg_xo_buf_wf2g_en_man          U     RW        default = 'h1
typedef union RG_XOSC_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_XO_BG_EN_MAN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_XO_BG_EN_MAN_SEL : 1;
    unsigned int RG_XO_LDO_BYPASS_EN : 1;
    unsigned int RG_XO_LDO_EN_MAN : 1;
    unsigned int RG_XO_LDO_EN_MAN_SEL : 1;
    unsigned int RG_XO_LDO_FBR_CTRL : 2;
    unsigned int RG_XO_LDO_VREF_CAL : 6;
    unsigned int RG_XO_EN_MAN : 1;
    unsigned int RG_XO_EN_MAN_SEL : 1;
    unsigned int RG_XO_SEL : 1;
    unsigned int RG_XO_CORE_BIAS_RCAL_CTRL : 5;
    unsigned int RG_XO_BUF_WF2G_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_WF2G_EN_MAN : 1;
    unsigned int rsvd_1 : 8;
  } b;
} RG_XOSC_A7_FIELD_T;

#define RG_XOSC_A8                                (CHIP_ANA_REG_BASE + 0x24)
// Bit 3   :0      rg_xo_core_cfixed_ctrl         U     RW        default = 'h8
// Bit 11  :4      rg_xo_core_ctune_ctrl          U     RW        default = 'h32
// Bit 15  :12     rg_xo_core_gmos_vb_sel         U     RW        default = 'h3
// Bit 20  :17     rg_xo_pd_vref_sel              U     RW        default = 'h8
// Bit 21          rg_xo_dcc_ctrl_man_sel         U     RW        default = 'h0
// Bit 31  :22     rg_xo_dcc_ctrl                 U     RW        default = 'h200
typedef union RG_XOSC_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_XO_CORE_CFIXED_CTRL : 4;
    unsigned int RG_XO_CORE_CTUNE_CTRL : 8;
    unsigned int RG_XO_CORE_GMOS_VB_SEL : 4;
    unsigned int rsvd_0 : 1;
    unsigned int RG_XO_PD_VREF_SEL : 4;
    unsigned int RG_XO_DCC_CTRL_MAN_SEL : 1;
    unsigned int RG_XO_DCC_CTRL : 10;
  } b;
} RG_XOSC_A8_FIELD_T;

#define RG_XOSC_A9                                (CHIP_ANA_REG_BASE + 0x28)
// Bit 3   :0      rg_xo_buf1_bias_ctrl           U     RW        default = 'h8
// Bit 7   :4      rg_xo_buf2_bias_ctrl           U     RW        default = 'h1
// Bit 11  :8      rg_xo_buf3_bias_ctrl           U     RW        default = 'hf
// Bit 12          rg_xo_aml_cal_error_ctrl       U     RW        default = 'h0
// Bit 23  :16     rg_xo_core_ctune_offset_ctrl     U     RW        default = 'h0
// Bit 24          rg_xo_dcc_en_man_sel           U     RW        default = 'h0
// Bit 26          rg_xo_dcc_en_man               U     RW        default = 'h1
typedef union RG_XOSC_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_XO_BUF1_BIAS_CTRL : 4;
    unsigned int RG_XO_BUF2_BIAS_CTRL : 4;
    unsigned int RG_XO_BUF3_BIAS_CTRL : 4;
    unsigned int RG_XO_AML_CAL_ERROR_CTRL : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_XO_CORE_CTUNE_OFFSET_CTRL : 8;
    unsigned int RG_XO_DCC_EN_MAN_SEL : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_XO_DCC_EN_MAN : 1;
    unsigned int rsvd_2 : 5;
  } b;
} RG_XOSC_A9_FIELD_T;

#define RG_XOSC_A10                               (CHIP_ANA_REG_BASE + 0x2c)
// Bit 15  :0      rg_xo_reserved                 U     RW        default = 'h0
// Bit 23  :16     ro_xo_core_bias_ctrl           U     RO        default = 'h0
// Bit 24          ro_xosc_cali_ready             U     RO        default = 'h0
// Bit 31          ro_xo_aml_cal_done             U     RO        default = 'h0
typedef union RG_XOSC_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_XO_RESERVED : 16;
    unsigned int RO_XO_CORE_BIAS_CTRL : 8;
    unsigned int RO_XOSC_CALI_READY : 1;
    unsigned int rsvd_0 : 6;
    unsigned int RO_XO_AML_CAL_DONE : 1;
  } b;
} RG_XOSC_A10_FIELD_T;

#define RG_XOSC_A11                               (CHIP_ANA_REG_BASE + 0x30)
// Bit 0           rg_xo_ldo_fc_en_man_sel        U     RW        default = 'h0
// Bit 1           rg_xo_ldo_fc_en_man            U     RW        default = 'h0
// Bit 2           rg_xo_pd_en_man_sel            U     RW        default = 'h0
// Bit 3           rg_xo_pd_en_man                U     RW        default = 'h1
// Bit 4           rg_xo_buf_core_en_man_sel      U     RW        default = 'h0
// Bit 5           rg_xo_buf_core_en_man          U     RW        default = 'h1
// Bit 6           rg_xo_buf_wf5g_en_man_sel      U     RW        default = 'h0
// Bit 7           rg_xo_buf_wf5g_en_man          U     RW        default = 'h1
// Bit 8           rg_xo_buf_dig_en_man_sel       U     RW        default = 'h0
// Bit 9           rg_xo_buf_dig_en_man           U     RW        default = 'h1
// Bit 10          rg_xo_buf_bb_en_man_sel        U     RW        default = 'h0
// Bit 11          rg_xo_buf_bb_en_man            U     RW        default = 'h1
// Bit 12          rg_xo_buf_bt_en_man_sel        U     RW        default = 'h0
// Bit 13          rg_xo_buf_bt_en_man            U     RW        default = 'h1
// Bit 15          rg_xo_core_bias_ctrl_man_sel     U     RW        default = 'h0
// Bit 23  :16     rg_xo_core_bias_ctrl_man       U     RW        default = 'hff
// Bit 31  :24     rg_xosc_wait_bnd               U     RW        default = 'h3f
typedef union RG_XOSC_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_XO_LDO_FC_EN_MAN_SEL : 1;
    unsigned int RG_XO_LDO_FC_EN_MAN : 1;
    unsigned int RG_XO_PD_EN_MAN_SEL : 1;
    unsigned int RG_XO_PD_EN_MAN : 1;
    unsigned int RG_XO_BUF_CORE_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_CORE_EN_MAN : 1;
    unsigned int RG_XO_BUF_WF5G_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_WF5G_EN_MAN : 1;
    unsigned int RG_XO_BUF_DIG_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_DIG_EN_MAN : 1;
    unsigned int RG_XO_BUF_BB_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_BB_EN_MAN : 1;
    unsigned int RG_XO_BUF_BT_EN_MAN_SEL : 1;
    unsigned int RG_XO_BUF_BT_EN_MAN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_XO_CORE_BIAS_CTRL_MAN_SEL : 1;
    unsigned int RG_XO_CORE_BIAS_CTRL_MAN : 8;
    unsigned int RG_XOSC_WAIT_BND : 8;
  } b;
} RG_XOSC_A11_FIELD_T;

#define RG_BG_A12                                 (CHIP_ANA_REG_BASE + 0x34)
// Bit 0           rg_wf_bg_en                    U     RW        default = 'h1
// Bit 1           rg_wf_bg_rcal_en               U     RW        default = 'h0
// Bit 14  :8      rg_wf_bg_ptat_cal_i_man        U     RW        default = 'h40
// Bit 21  :15     rg_wf_bg_zt_cal_i_man          U     RW        default = 'h40
// Bit 22          rg_wf_bg_iout_en               U     RW        default = 'h1
// Bit 23          rg_wf_bg_iout_sel              U     RW        default = 'h1
// Bit 24          rg_wf_bg_zt_cal_i_man_sel      U     RW        default = 'h1
// Bit 25          rg_wf_bg_ptat_cal_i_man_sel     U     RW        default = 'h1
// Bit 27  :26     rg_wf_bg_rcal_waittime         U     RW        default = 'h1
typedef union RG_BG_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_BG_EN : 1;
    unsigned int RG_WF_BG_RCAL_EN : 1;
    unsigned int rsvd_0 : 6;
    unsigned int RG_WF_BG_PTAT_CAL_I_MAN : 7;
    unsigned int RG_WF_BG_ZT_CAL_I_MAN : 7;
    unsigned int RG_WF_BG_IOUT_EN : 1;
    unsigned int RG_WF_BG_IOUT_SEL : 1;
    unsigned int RG_WF_BG_ZT_CAL_I_MAN_SEL : 1;
    unsigned int RG_WF_BG_PTAT_CAL_I_MAN_SEL : 1;
    unsigned int RG_WF_BG_RCAL_WAITTIME : 2;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BG_A12_FIELD_T;

#define RG_BG_A13                                 (CHIP_ANA_REG_BASE + 0x38)
// Bit 6   :0      ro_wf_bg_zt_cal_i              U     RO        default = 'h0
// Bit 14  :8      ro_wf_bg_ptat_cal_i            U     RO        default = 'h0
// Bit 29          ro_wf_bg_rcal_finish           U     RO        default = 'h0
// Bit 30          ro_wf_bg_cmp_out               U     RO        default = 'h0
typedef union RG_BG_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_BG_ZT_CAL_I : 7;
    unsigned int rsvd_0 : 1;
    unsigned int RO_WF_BG_PTAT_CAL_I : 7;
    unsigned int rsvd_1 : 14;
    unsigned int RO_WF_BG_RCAL_FINISH : 1;
    unsigned int RO_WF_BG_CMP_OUT : 1;
    unsigned int rsvd_2 : 1;
  } b;
} RG_BG_A13_FIELD_T;

#define RG_BG_A14                                 (CHIP_ANA_REG_BASE + 0x3c)
// Bit 0           rg_bt_bg_en                    U     RW        default = 'h1
// Bit 1           rg_bt_bg_rcal_en               U     RW        default = 'h0
// Bit 14  :8      rg_bt_bg_ptat_cal_i_man        U     RW        default = 'h40
// Bit 21  :15     rg_bt_bg_zt_cal_i_man          U     RW        default = 'h40
// Bit 22          rg_bt_bg_iout_en               U     RW        default = 'h1
// Bit 23          rg_bt_bg_iout_sel              U     RW        default = 'h1
// Bit 24          rg_bt_bg_zt_cal_i_man_sel      U     RW        default = 'h1
// Bit 25          rg_bt_bg_ptat_cal_i_man_sel     U     RW        default = 'h1
// Bit 27  :26     rg_bt_bg_rcal_waittime         U     RW        default = 'h1
typedef union RG_BG_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_BG_EN : 1;
    unsigned int RG_BT_BG_RCAL_EN : 1;
    unsigned int rsvd_0 : 6;
    unsigned int RG_BT_BG_PTAT_CAL_I_MAN : 7;
    unsigned int RG_BT_BG_ZT_CAL_I_MAN : 7;
    unsigned int RG_BT_BG_IOUT_EN : 1;
    unsigned int RG_BT_BG_IOUT_SEL : 1;
    unsigned int RG_BT_BG_ZT_CAL_I_MAN_SEL : 1;
    unsigned int RG_BT_BG_PTAT_CAL_I_MAN_SEL : 1;
    unsigned int RG_BT_BG_RCAL_WAITTIME : 2;
    unsigned int rsvd_1 : 4;
  } b;
} RG_BG_A14_FIELD_T;

#define RG_BG_A15                                 (CHIP_ANA_REG_BASE + 0x40)
// Bit 6   :0      ro_bt_bg_zt_cal_i              U     RO        default = 'h0
// Bit 14  :8      ro_bt_bg_ptat_cal_i            U     RO        default = 'h0
// Bit 29          ro_bt_bg_rcal_finish           U     RO        default = 'h0
// Bit 30          ro_bt_bg_cmp_out               U     RO        default = 'h0
typedef union RG_BG_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_BT_BG_ZT_CAL_I : 7;
    unsigned int rsvd_0 : 1;
    unsigned int RO_BT_BG_PTAT_CAL_I : 7;
    unsigned int rsvd_1 : 14;
    unsigned int RO_BT_BG_RCAL_FINISH : 1;
    unsigned int RO_BT_BG_CMP_OUT : 1;
    unsigned int rsvd_2 : 1;
  } b;
} RG_BG_A15_FIELD_T;

#define RG_BG_A16                                 (CHIP_ANA_REG_BASE + 0x44)
// Bit 0           rg_wf_sleep_enb                U     RW        default = 'h1
// Bit 1           rg_wf_dvdd_ldo_en              U     RW        default = 'h1
// Bit 8   :4      rg_wf_dvdd_vsel                U     RW        default = 'h10
// Bit 11  :9      rg_wf0_top_mux_en              U     RW        default = 'h0
// Bit 14  :12     rg_wf1_top_mux_en              U     RW        default = 'h0
typedef union RG_BG_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SLEEP_ENB : 1;
    unsigned int RG_WF_DVDD_LDO_EN : 1;
    unsigned int rsvd_0 : 2;
    unsigned int RG_WF_DVDD_VSEL : 5;
    unsigned int RG_WF0_TOP_MUX_EN : 3;
    unsigned int RG_WF1_TOP_MUX_EN : 3;
    unsigned int rsvd_1 : 17;
  } b;
} RG_BG_A16_FIELD_T;

#define RG_BG_A17                                 (CHIP_ANA_REG_BASE + 0x48)
// Bit 16          rg_bt_sleep_enb                U     RW        default = 'h1
// Bit 17          rg_bt_dvdd_ldo_en              U     RW        default = 'h1
// Bit 24  :20     rg_bt_dvdd_vsel                U     RW        default = 'h10
// Bit 26  :25     rg_bt_top_mux_en               U     RW        default = 'h0
typedef union RG_BG_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 16;
    unsigned int RG_BT_SLEEP_ENB : 1;
    unsigned int RG_BT_DVDD_LDO_EN : 1;
    unsigned int rsvd_1 : 2;
    unsigned int RG_BT_DVDD_VSEL : 5;
    unsigned int RG_BT_TOP_MUX_EN : 2;
    unsigned int rsvd_2 : 5;
  } b;
} RG_BG_A17_FIELD_T;

#define RG_PMIC_A18                               (CHIP_ANA_REG_BASE + 0x4c)
// Bit 0           rg_pmu_bucka_bypass            U     RW        default = 'h0
// Bit 1           rg_pmu_bucka_fpwm              U     RW        default = 'h0
// Bit 2           rg_pmu_bucka_ndis_en           U     RW        default = 'h1
// Bit 3           rg_pmu_bucka_dis_lpmode        U     RW        default = 'h0
// Bit 7   :4      rg_pmu_bucka_zx_offset         U     RW        default = 'h5
// Bit 8           rg_pmu_bucka_zx_pdn            U     RW        default = 'h0
// Bit 9           rg_pmu_bucka_force_2m_internal     U     RW        default = 'h1
// Bit 10          rg_pmu_bucka_1m_en             U     RW        default = 'h0
// Bit 11          rg_pmu_bucka_intclk_dis        U     RW        default = 'h0
// Bit 15  :12     rg_pmu_bucka_vsel_vh           U     RW        default = 'h7
// Bit 19  :16     rg_pmu_bucka_vsel_vl           U     RW        default = 'h6
// Bit 22  :20     rg_pmu_bucka_ndri_on_sr        U     RW        default = 'h4
// Bit 25  :23     rg_pmu_bucka_ndri_off_sr       U     RW        default = 'h4
// Bit 28  :26     rg_pmu_bucka_pdri_on_sr        U     RW        default = 'h4
// Bit 31  :29     rg_pmu_bucka_pdri_off_sr       U     RW        default = 'h4
typedef union RG_PMIC_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_BYPASS : 1;
    unsigned int RG_PMU_BUCKA_FPWM : 1;
    unsigned int RG_PMU_BUCKA_NDIS_EN : 1;
    unsigned int RG_PMU_BUCKA_DIS_LPMODE : 1;
    unsigned int RG_PMU_BUCKA_ZX_OFFSET : 4;
    unsigned int RG_PMU_BUCKA_ZX_PDN : 1;
    unsigned int RG_PMU_BUCKA_FORCE_2M_INTERNAL : 1;
    unsigned int RG_PMU_BUCKA_1M_EN : 1;
    unsigned int RG_PMU_BUCKA_INTCLK_DIS : 1;
    unsigned int RG_PMU_BUCKA_VSEL_VH : 4;
    unsigned int RG_PMU_BUCKA_VSEL_VL : 4;
    unsigned int RG_PMU_BUCKA_NDRI_ON_SR : 3;
    unsigned int RG_PMU_BUCKA_NDRI_OFF_SR : 3;
    unsigned int RG_PMU_BUCKA_PDRI_ON_SR : 3;
    unsigned int RG_PMU_BUCKA_PDRI_OFF_SR : 3;
  } b;
} RG_PMIC_A18_FIELD_T;

#define RG_PMIC_A19                               (CHIP_ANA_REG_BASE + 0x50)
// Bit 0           rg_pmu_bucka_trimtest_ea       U     RW        default = 'h0
// Bit 1           rg_pmu_bucka_trimtest_vref     U     RW        default = 'h0
// Bit 2           rg_pmu_bucka_trimtest_csp      U     RW        default = 'h0
// Bit 4   :3      rg_pmu_bucka_voutfb_cap        U     RW        default = 'h1
// Bit 7   :5      rg_pmu_bucka_ea_ccomp          U     RW        default = 'h2
// Bit 11  :8      rg_pmu_bucka_ea_rcomp          U     RW        default = 'h4
// Bit 17  :12     rg_pmu_bucka_vref_sel          U     RW        default = 'h8
// Bit 19  :18     rg_pmu_bucka_capslp_ramp       U     RW        default = 'h2
// Bit 23  :20     rg_pmu_bucka_cs_gain_ramp      U     RW        default = 'h8
// Bit 27  :24     rg_pmu_bucka_offset_ramp       U     RW        default = 'h8
// Bit 31  :28     rg_pmu_bucka_slp_ramp          U     RW        default = 'h8
typedef union RG_PMIC_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_TRIMTEST_EA : 1;
    unsigned int RG_PMU_BUCKA_TRIMTEST_VREF : 1;
    unsigned int RG_PMU_BUCKA_TRIMTEST_CSP : 1;
    unsigned int RG_PMU_BUCKA_VOUTFB_CAP : 2;
    unsigned int RG_PMU_BUCKA_EA_CCOMP : 3;
    unsigned int RG_PMU_BUCKA_EA_RCOMP : 4;
    unsigned int RG_PMU_BUCKA_VREF_SEL : 6;
    unsigned int RG_PMU_BUCKA_CAPSLP_RAMP : 2;
    unsigned int RG_PMU_BUCKA_CS_GAIN_RAMP : 4;
    unsigned int RG_PMU_BUCKA_OFFSET_RAMP : 4;
    unsigned int RG_PMU_BUCKA_SLP_RAMP : 4;
  } b;
} RG_PMIC_A19_FIELD_T;

#define RG_PMIC_A20                               (CHIP_ANA_REG_BASE + 0x54)
// Bit 3   :0      rg_pmu_bucka_oc_level_csp      U     RW        default = 'h8
// Bit 4           rg_pmu_bucka_csoffset_ramp     U     RW        default = 'h0
// Bit 7   :5      rg_pmu_bucka_rsv1              U     RW        default = 'h7
// Bit 15  :8      rg_pmu_bucka_rsv2              U     RW        default = 'h0
// Bit 23  :16     rg_pmu_bucka_rsv3              U     RW        default = 'h88
// Bit 31  :24     rg_pmu_bucka_rsv4              U     RW        default = 'hff
typedef union RG_PMIC_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_OC_LEVEL_CSP : 4;
    unsigned int RG_PMU_BUCKA_CSOFFSET_RAMP : 1;
    unsigned int RG_PMU_BUCKA_RSV1 : 3;
    unsigned int RG_PMU_BUCKA_RSV2 : 8;
    unsigned int RG_PMU_BUCKA_RSV3 : 8;
    unsigned int RG_PMU_BUCKA_RSV4 : 8;
  } b;
} RG_PMIC_A20_FIELD_T;

#define RG_PMIC_A21                               (CHIP_ANA_REG_BASE + 0x58)
// Bit 0           rg_pmu_bucka_lg_blank_dis      U     RW        default = 'h0
// Bit 2   :1      rg_pmu_bucka_mode_testmux      U     RW        default = 'h0
// Bit 4   :3      rg_pmu_bucka_testmux_ib        U     RW        default = 'h0
// Bit 7   :5      rg_pmu_bucka_testmux_ana       U     RW        default = 'h0
// Bit 11  :8      rg_pmu_bucka_testmux_dig       U     RW        default = 'h0
// Bit 12          rg_pmu_testmux_ana_bypass      U     RW        default = 'h0
// Bit 14  :13     rg_pmu_testmux_mode_sel        U     RW        default = 'h0
// Bit 17  :15     rg_pmu_testmux_ib_sel          U     RW        default = 'h0
// Bit 20  :18     rg_pmu_testmux_ana_sel         U     RW        default = 'h0
// Bit 23  :21     rg_pmu_testmux_dig_sel         U     RW        default = 'h0
// Bit 25  :24     rg_pmu_bucka_con_trim          U     RW        default = 'h0
// Bit 29  :26     rg_pmu_bucka_toff_min_trim     U     RW        default = 'h8
typedef union RG_PMIC_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_LG_BLANK_DIS : 1;
    unsigned int RG_PMU_BUCKA_MODE_TESTMUX : 2;
    unsigned int RG_PMU_BUCKA_TESTMUX_IB : 2;
    unsigned int RG_PMU_BUCKA_TESTMUX_ANA : 3;
    unsigned int RG_PMU_BUCKA_TESTMUX_DIG : 4;
    unsigned int RG_PMU_TESTMUX_ANA_BYPASS : 1;
    unsigned int RG_PMU_TESTMUX_MODE_SEL : 2;
    unsigned int RG_PMU_TESTMUX_IB_SEL : 3;
    unsigned int RG_PMU_TESTMUX_ANA_SEL : 3;
    unsigned int RG_PMU_TESTMUX_DIG_SEL : 3;
    unsigned int RG_PMU_BUCKA_CON_TRIM : 2;
    unsigned int RG_PMU_BUCKA_TOFF_MIN_TRIM : 4;
    unsigned int rsvd_0 : 2;
  } b;
} RG_PMIC_A21_FIELD_T;

#define RG_PMIC_A22                               (CHIP_ANA_REG_BASE + 0x5c)
// Bit 3   :0      rg_pmu_bucka_ton_min_trim      U     RW        default = 'h8
// Bit 8   :4      rg_pmu_bucka_efuse_trim_offset_vref     U     RW        default = 'h10
// Bit 12  :9      rg_pmu_bucka_efuse_trim_iref_vhreg     U     RW        default = 'h8
// Bit 16  :13     rg_pmu_bucka_efuse_trim_iref_vlreg     U     RW        default = 'h8
// Bit 19  :17     rg_pmu_bucka_efuse_trim_vref_burst     U     RW        default = 'h4
// Bit 24  :20     rg_pmu_bucka_efuse_trim_ea     U     RW        default = 'h10
// Bit 29  :25     rg_pmu_bucka_efuse_trim_offset_csp     U     RW        default = 'h10
typedef union RG_PMIC_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_TON_MIN_TRIM : 4;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_OFFSET_VREF : 5;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_IREF_VHREG : 4;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_IREF_VLREG : 4;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_VREF_BURST : 3;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_EA : 5;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_OFFSET_CSP : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_PMIC_A22_FIELD_T;

#define RG_PMIC_A23                               (CHIP_ANA_REG_BASE + 0x60)
// Bit 3   :0      rg_pmu_bucka_efuse_trim_slp_csp     U     RW        default = 'h8
// Bit 6   :4      rg_pmu_bucka_efuse_trim_cap_osc     U     RW        default = 'h4
// Bit 10  :7      rg_pmu_bucka_efuse_trim_i_osc     U     RW        default = 'h8
// Bit 11          ro_bucka_oc_status             U     RO        default = 'h0
// Bit 12          ro_bucka_status_dac            U     RO        default = 'h0
// Bit 13          ro_bucka_status_offset_csp     U     RO        default = 'h0
// Bit 14          ro_bucka_status_trim_ea        U     RO        default = 'h0
typedef union RG_PMIC_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_SLP_CSP : 4;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_CAP_OSC : 3;
    unsigned int RG_PMU_BUCKA_EFUSE_TRIM_I_OSC : 4;
    unsigned int RO_BUCKA_OC_STATUS : 1;
    unsigned int RO_BUCKA_STATUS_DAC : 1;
    unsigned int RO_BUCKA_STATUS_OFFSET_CSP : 1;
    unsigned int RO_BUCKA_STATUS_TRIM_EA : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_PMIC_A23_FIELD_T;

#define RG_PMIC_A24                               (CHIP_ANA_REG_BASE + 0x64)
// Bit 3   :0      rg_rough_bg_adj                U     RW        default = 'h8
// Bit 7   :4      rg_bg650_adj                   U     RW        default = 'h8
// Bit 11  :8      rg_bg_tc_adj                   U     RW        default = 'h8
// Bit 12          rg_force_pmu_32k_off           U     RW        default = 'h0
// Bit 16  :13     rg_pmu_32k_adj                 U     RW        default = 'h8
// Bit 17          rg_force_aldo_off              U     RW        default = 'h0
// Bit 18          rg_force_bucka_off             U     RW        default = 'h0
// Bit 22  :20     rg_prereg_dmmy_ld              U     RW        default = 'h4
// Bit 26  :23     rg_strup_rsva                  U     RW        default = 'h8
// Bit 30          rg_force_buckd_off             U     RW        default = 'h0
typedef union RG_PMIC_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_ROUGH_BG_ADJ : 4;
    unsigned int RG_BG650_ADJ : 4;
    unsigned int RG_BG_TC_ADJ : 4;
    unsigned int RG_FORCE_PMU_32K_OFF : 1;
    unsigned int RG_PMU_32K_ADJ : 4;
    unsigned int RG_FORCE_ALDO_OFF : 1;
    unsigned int RG_FORCE_BUCKA_OFF : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_PREREG_DMMY_LD : 3;
    unsigned int RG_STRUP_RSVA : 4;
    unsigned int rsvd_1 : 3;
    unsigned int RG_FORCE_BUCKD_OFF : 1;
    unsigned int rsvd_2 : 1;
  } b;
} RG_PMIC_A24_FIELD_T;

#define RG_PMIC_A25                               (CHIP_ANA_REG_BASE + 0x68)
// Bit 3   :0      rg_strup_rsvc                  U     RW        default = 'h8
// Bit 7   :4      rg_strup_rsvd                  U     RW        default = 'h0
// Bit 11  :8      rg_strup_rsve                  U     RW        default = 'hf
// Bit 15  :12     ro_strup_status_rsva           U     RO        default = 'h0
// Bit 19  :16     ro_strup_status_rsvb           U     RO        default = 'h8
// Bit 23  :20     rg_strup_rsvb                  U     RW        default = 'h8
typedef union RG_PMIC_A25_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_STRUP_RSVC : 4;
    unsigned int RG_STRUP_RSVD : 4;
    unsigned int RG_STRUP_RSVE : 4;
    unsigned int RO_STRUP_STATUS_RSVA : 4;
    unsigned int RO_STRUP_STATUS_RSVB : 4;
    unsigned int RG_STRUP_RSVB : 4;
    unsigned int rsvd_0 : 8;
  } b;
} RG_PMIC_A25_FIELD_T;

#define RG_PMIC_A26                               (CHIP_ANA_REG_BASE + 0x6c)
// Bit 0           rg_aoldo_oc_en                 U     RW        default = 'h1
// Bit 1           rg_aoldo_dischr_en             U     RW        default = 'h1
// Bit 9   :6      rg_aoldo_ocsel                 U     RW        default = 'h8
// Bit 13  :10     rg_aoldo_comp_adj              U     RW        default = 'h8
// Bit 16  :14     rg_aoldo_dummy_ld              U     RW        default = 'h4
// Bit 20  :17     rg_aoldo_rsva                  U     RW        default = 'h8
// Bit 24  :21     rg_aoldo_rsvb                  U     RW        default = 'h8
// Bit 28  :25     rg_aoldo_rsvc                  U     RW        default = 'h8
typedef union RG_PMIC_A26_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_AOLDO_OC_EN : 1;
    unsigned int RG_AOLDO_DISCHR_EN : 1;
    unsigned int rsvd_0 : 4;
    unsigned int RG_AOLDO_OCSEL : 4;
    unsigned int RG_AOLDO_COMP_ADJ : 4;
    unsigned int RG_AOLDO_DUMMY_LD : 3;
    unsigned int RG_AOLDO_RSVA : 4;
    unsigned int RG_AOLDO_RSVB : 4;
    unsigned int RG_AOLDO_RSVC : 4;
    unsigned int rsvd_1 : 3;
  } b;
} RG_PMIC_A26_FIELD_T;

#define RG_PMIC_A27                               (CHIP_ANA_REG_BASE + 0x70)
// Bit 6   :0      rg_aoldo_vosel                 U     RW        default = 'h20
// Bit 8           ro_aoldo_oc_status             U     RO        default = 'h0
typedef union RG_PMIC_A27_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_AOLDO_VOSEL : 7;
    unsigned int rsvd_0 : 1;
    unsigned int RO_AOLDO_OC_STATUS : 1;
    unsigned int rsvd_1 : 23;
  } b;
} RG_PMIC_A27_FIELD_T;

#define RG_PMIC_A30                               (CHIP_ANA_REG_BASE + 0x7c)
// Bit 7   :0      rg_aldo_vo_adj                 U     RW        default = 'h80
// Bit 11  :8      rg_aldo_comp_adj               U     RW        default = 'h8
// Bit 15  :12     rg_aldo_bias_adj               U     RW        default = 'h8
// Bit 19  :16     rg_aldo_oc_adj                 U     RW        default = 'h8
// Bit 20          rg_aldo_fast_dischr_en         U     RW        default = 'h1
// Bit 21          rg_aldo_fc_en                  U     RW        default = 'h1
// Bit 23  :22     rg_aldo_fc_adj                 U     RW        default = 'h2
// Bit 31  :24     rg_aldo_rsva                   U     RW        default = 'h80
typedef union RG_PMIC_A30_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_ALDO_VO_ADJ : 8;
    unsigned int RG_ALDO_COMP_ADJ : 4;
    unsigned int RG_ALDO_BIAS_ADJ : 4;
    unsigned int RG_ALDO_OC_ADJ : 4;
    unsigned int RG_ALDO_FAST_DISCHR_EN : 1;
    unsigned int RG_ALDO_FC_EN : 1;
    unsigned int RG_ALDO_FC_ADJ : 2;
    unsigned int RG_ALDO_RSVA : 8;
  } b;
} RG_PMIC_A30_FIELD_T;

#define RG_PMIC_A31                               (CHIP_ANA_REG_BASE + 0x80)
// Bit 7   :0      rg_aldo_rsvb                   U     RW        default = 'h80
// Bit 15  :8      rg_aldo_rsvc                   U     RW        default = 'h80
// Bit 23  :16     rg_aldo_rsvd                   U     RW        default = 'h80
// Bit 24          ro_aldo_oc_status              U     RO        default = 'h0
// Bit 25          ro_aldo_status_rsva            U     RO        default = 'h0
// Bit 26          ro_aldo_status_rsvb            U     RO        default = 'h0
typedef union RG_PMIC_A31_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_ALDO_RSVB : 8;
    unsigned int RG_ALDO_RSVC : 8;
    unsigned int RG_ALDO_RSVD : 8;
    unsigned int RO_ALDO_OC_STATUS : 1;
    unsigned int RO_ALDO_STATUS_RSVA : 1;
    unsigned int RO_ALDO_STATUS_RSVB : 1;
    unsigned int rsvd_0 : 5;
  } b;
} RG_PMIC_A31_FIELD_T;

#define RG_WFRF_A33                               (CHIP_ANA_REG_BASE + 0x88)
// Bit 0           rg_wf_rfdig_clk_fromrf         U     RW        default = 'h0
// Bit 7   :4      rg_wf_rfdig_clkbuf_rc_ctrl     U     RW        default = 'h8
typedef union RG_WFRF_A33_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_RFDIG_CLK_FROMRF : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_RFDIG_CLKBUF_RC_CTRL : 4;
    unsigned int rsvd_1 : 24;
  } b;
} RG_WFRF_A33_FIELD_T;

#define RG_BTRF_A34                               (CHIP_ANA_REG_BASE + 0x8c)
// Bit 0           rg_bt_rfdig_clk_fromrf         U     RW        default = 'h0
// Bit 7   :4      rg_bt_rfdig_clkbuf_rc_ctrl     U     RW        default = 'h8
typedef union RG_BTRF_A34_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_BT_RFDIG_CLK_FROMRF : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_BT_RFDIG_CLKBUF_RC_CTRL : 4;
    unsigned int rsvd_1 : 24;
  } b;
} RG_BTRF_A34_FIELD_T;

#define RG_PMIC_A35                               (CHIP_ANA_REG_BASE + 0x90)
// Bit 0           rg_pmu_buckd_bypass            U     RW        default = 'h0
// Bit 1           rg_pmu_buckd_fpwm              U     RW        default = 'h0
// Bit 2           rg_pmu_buckd_ndis_en           U     RW        default = 'h1
// Bit 3           rg_pmu_buckd_dis_lpmode        U     RW        default = 'h0
// Bit 7   :4      rg_pmu_buckd_zx_offset         U     RW        default = 'h5
// Bit 8           rg_pmu_buckd_zx_pdn            U     RW        default = 'h0
// Bit 9           rg_pmu_buckd_force_2m_internal     U     RW        default = 'h1
// Bit 10          rg_pmu_buckd_1m_en             U     RW        default = 'h0
// Bit 11          rg_pmu_buckd_intclk_dis        U     RW        default = 'h0
// Bit 15  :12     rg_pmu_buckd_vsel_vh           U     RW        default = 'h6
// Bit 19  :16     rg_pmu_buckd_vsel_vl           U     RW        default = 'h6
// Bit 22  :20     rg_pmu_buckd_ndri_on_sr        U     RW        default = 'h4
// Bit 25  :23     rg_pmu_buckd_ndri_off_sr       U     RW        default = 'h4
// Bit 28  :26     rg_pmu_buckd_pdri_on_sr        U     RW        default = 'h4
// Bit 31  :29     rg_pmu_buckd_pdri_off_sr       U     RW        default = 'h4
typedef union RG_PMIC_A35_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_BYPASS : 1;
    unsigned int RG_PMU_BUCKD_FPWM : 1;
    unsigned int RG_PMU_BUCKD_NDIS_EN : 1;
    unsigned int RG_PMU_BUCKD_DIS_LPMODE : 1;
    unsigned int RG_PMU_BUCKD_ZX_OFFSET : 4;
    unsigned int RG_PMU_BUCKD_ZX_PDN : 1;
    unsigned int RG_PMU_BUCKD_FORCE_2M_INTERNAL : 1;
    unsigned int RG_PMU_BUCKD_1M_EN : 1;
    unsigned int RG_PMU_BUCKD_INTCLK_DIS : 1;
    unsigned int RG_PMU_BUCKD_VSEL_VH : 4;
    unsigned int RG_PMU_BUCKD_VSEL_VL : 4;
    unsigned int RG_PMU_BUCKD_NDRI_ON_SR : 3;
    unsigned int RG_PMU_BUCKD_NDRI_OFF_SR : 3;
    unsigned int RG_PMU_BUCKD_PDRI_ON_SR : 3;
    unsigned int RG_PMU_BUCKD_PDRI_OFF_SR : 3;
  } b;
} RG_PMIC_A35_FIELD_T;

#define RG_PMIC_A36                               (CHIP_ANA_REG_BASE + 0x94)
// Bit 0           rg_pmu_buckd_trimtest_ea       U     RW        default = 'h0
// Bit 1           rg_pmu_buckd_trimtest_vref     U     RW        default = 'h0
// Bit 2           rg_pmu_buckd_trimtest_csp      U     RW        default = 'h0
// Bit 4   :3      rg_pmu_buckd_voutfb_cap        U     RW        default = 'h1
// Bit 7   :5      rg_pmu_buckd_ea_ccomp          U     RW        default = 'h2
// Bit 11  :8      rg_pmu_buckd_ea_rcomp          U     RW        default = 'h4
// Bit 17  :12     rg_pmu_buckd_vref_sel          U     RW        default = 'h8
// Bit 19  :18     rg_pmu_buckd_capslp_ramp       U     RW        default = 'h2
// Bit 23  :20     rg_pmu_buckd_cs_gain_ramp      U     RW        default = 'h7
// Bit 27  :24     rg_pmu_buckd_offset_ramp       U     RW        default = 'h8
// Bit 31  :28     rg_pmu_buckd_slp_ramp          U     RW        default = 'h8
typedef union RG_PMIC_A36_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_TRIMTEST_EA : 1;
    unsigned int RG_PMU_BUCKD_TRIMTEST_VREF : 1;
    unsigned int RG_PMU_BUCKD_TRIMTEST_CSP : 1;
    unsigned int RG_PMU_BUCKD_VOUTFB_CAP : 2;
    unsigned int RG_PMU_BUCKD_EA_CCOMP : 3;
    unsigned int RG_PMU_BUCKD_EA_RCOMP : 4;
    unsigned int RG_PMU_BUCKD_VREF_SEL : 6;
    unsigned int RG_PMU_BUCKD_CAPSLP_RAMP : 2;
    unsigned int RG_PMU_BUCKD_CS_GAIN_RAMP : 4;
    unsigned int RG_PMU_BUCKD_OFFSET_RAMP : 4;
    unsigned int RG_PMU_BUCKD_SLP_RAMP : 4;
  } b;
} RG_PMIC_A36_FIELD_T;

#define RG_PMIC_A37                               (CHIP_ANA_REG_BASE + 0x98)
// Bit 3   :0      rg_pmu_buckd_oc_level_csp      U     RW        default = 'h8
// Bit 4           rg_pmu_buckd_csoffset_ramp     U     RW        default = 'h0
// Bit 7   :5      rg_pmu_buckd_rsv1              U     RW        default = 'h7
// Bit 15  :8      rg_pmu_buckd_rsv2              U     RW        default = 'h0
// Bit 23  :16     rg_pmu_buckd_rsv3              U     RW        default = 'h88
// Bit 31  :24     rg_pmu_buckd_rsv4              U     RW        default = 'hff
typedef union RG_PMIC_A37_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_OC_LEVEL_CSP : 4;
    unsigned int RG_PMU_BUCKD_CSOFFSET_RAMP : 1;
    unsigned int RG_PMU_BUCKD_RSV1 : 3;
    unsigned int RG_PMU_BUCKD_RSV2 : 8;
    unsigned int RG_PMU_BUCKD_RSV3 : 8;
    unsigned int RG_PMU_BUCKD_RSV4 : 8;
  } b;
} RG_PMIC_A37_FIELD_T;

#define RG_PMIC_A38                               (CHIP_ANA_REG_BASE + 0x9c)
// Bit 0           rg_pmu_buckd_lg_blank_dis      U     RW        default = 'h0
// Bit 2   :1      rg_pmu_buckd_mode_testmux      U     RW        default = 'h0
// Bit 4   :3      rg_pmu_buckd_testmux_ib        U     RW        default = 'h0
// Bit 7   :5      rg_pmu_buckd_testmux_ana       U     RW        default = 'h0
// Bit 11  :8      rg_pmu_buckd_testmux_dig       U     RW        default = 'h0
// Bit 25  :24     rg_pmu_buckd_con_trim          U     RW        default = 'h0
// Bit 29  :26     rg_pmu_buckd_toff_min_trim     U     RW        default = 'h8
typedef union RG_PMIC_A38_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_LG_BLANK_DIS : 1;
    unsigned int RG_PMU_BUCKD_MODE_TESTMUX : 2;
    unsigned int RG_PMU_BUCKD_TESTMUX_IB : 2;
    unsigned int RG_PMU_BUCKD_TESTMUX_ANA : 3;
    unsigned int RG_PMU_BUCKD_TESTMUX_DIG : 4;
    unsigned int rsvd_0 : 12;
    unsigned int RG_PMU_BUCKD_CON_TRIM : 2;
    unsigned int RG_PMU_BUCKD_TOFF_MIN_TRIM : 4;
    unsigned int rsvd_1 : 2;
  } b;
} RG_PMIC_A38_FIELD_T;

#define RG_PMIC_A39                               (CHIP_ANA_REG_BASE + 0xa0)
// Bit 3   :0      rg_pmu_buckd_ton_min_trim      U     RW        default = 'h8
// Bit 8   :4      rg_pmu_buckd_efuse_trim_offset_vref     U     RW        default = 'h10
// Bit 12  :9      rg_pmu_buckd_efuse_trim_iref_vhreg     U     RW        default = 'h8
// Bit 16  :13     rg_pmu_buckd_efuse_trim_iref_vlreg     U     RW        default = 'h8
// Bit 19  :17     rg_pmu_buckd_efuse_trim_vref_burst     U     RW        default = 'h4
// Bit 24  :20     rg_pmu_buckd_efuse_trim_ea     U     RW        default = 'h10
// Bit 29  :25     rg_pmu_buckd_efuse_trim_offset_csp     U     RW        default = 'h10
typedef union RG_PMIC_A39_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_TON_MIN_TRIM : 4;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_OFFSET_VREF : 5;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_IREF_VHREG : 4;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_IREF_VLREG : 4;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_VREF_BURST : 3;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_EA : 5;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_OFFSET_CSP : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_PMIC_A39_FIELD_T;

#define RG_PMIC_A40                               (CHIP_ANA_REG_BASE + 0xa4)
// Bit 3   :0      rg_pmu_buckd_efuse_trim_slp_csp     U     RW        default = 'h8
// Bit 6   :4      rg_pmu_buckd_efuse_trim_cap_osc     U     RW        default = 'h4
// Bit 10  :7      rg_pmu_buckd_efuse_trim_i_osc     U     RW        default = 'h8
// Bit 11          ro_buckd_oc_status             U     RO        default = 'h0
// Bit 12          ro_buckd_status_dac            U     RO        default = 'h0
// Bit 13          ro_buckd_status_offset_csp     U     RO        default = 'h0
// Bit 14          ro_buckd_status_trim_ea        U     RO        default = 'h0
typedef union RG_PMIC_A40_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_SLP_CSP : 4;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_CAP_OSC : 3;
    unsigned int RG_PMU_BUCKD_EFUSE_TRIM_I_OSC : 4;
    unsigned int RO_BUCKD_OC_STATUS : 1;
    unsigned int RO_BUCKD_STATUS_DAC : 1;
    unsigned int RO_BUCKD_STATUS_OFFSET_CSP : 1;
    unsigned int RO_BUCKD_STATUS_TRIM_EA : 1;
    unsigned int rsvd_0 : 17;
  } b;
} RG_PMIC_A40_FIELD_T;

#define RG_PMIC_A41                               (CHIP_ANA_REG_BASE + 0xa8)
// Bit 1           rg_ao_hifldo_oc_en             U     RW        default = 'h1
// Bit 2           rg_ao_hifldo_dischr_en         U     RW        default = 'h1
// Bit 9   :3      rg_ao_hifldo_vosel             U     RW        default = 'h20
// Bit 13  :10     rg_ao_hifldo_ocsel             U     RW        default = 'h8
// Bit 14          ro_ao_hifldo_oc_status         U     RO        default = 'h0
// Bit 15          rg_force_ao_hifldo_off         U     RW        default = 'h0
// Bit 19  :16     rg_ao_hifldo_comp_adj          U     RW        default = 'h8
// Bit 22  :20     rg_ao_hifldo_dummy_ld          U     RW        default = 'h4
// Bit 26  :23     rg_ao_hifldo_rsva              U     RW        default = 'h8
// Bit 30  :27     rg_ao_hifldo_rsvb              U     RW        default = 'h8
typedef union RG_PMIC_A41_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 1;
    unsigned int RG_AO_HIFLDO_OC_EN : 1;
    unsigned int RG_AO_HIFLDO_DISCHR_EN : 1;
    unsigned int RG_AO_HIFLDO_VOSEL : 7;
    unsigned int RG_AO_HIFLDO_OCSEL : 4;
    unsigned int RO_AO_HIFLDO_OC_STATUS : 1;
    unsigned int RG_FORCE_AO_HIFLDO_OFF : 1;
    unsigned int RG_AO_HIFLDO_COMP_ADJ : 4;
    unsigned int RG_AO_HIFLDO_DUMMY_LD : 3;
    unsigned int RG_AO_HIFLDO_RSVA : 4;
    unsigned int RG_AO_HIFLDO_RSVB : 4;
    unsigned int rsvd_1 : 1;
  } b;
} RG_PMIC_A41_FIELD_T;

#define RG_PMIC_A42                               (CHIP_ANA_REG_BASE + 0xac)
// Bit 3   :0      rg_ao_hifldo_rsvc              U     RW        default = 'h8
typedef union RG_PMIC_A42_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_AO_HIFLDO_RSVC : 4;
    unsigned int rsvd_0 : 28;
  } b;
} RG_PMIC_A42_FIELD_T;

#endif

