#ifdef RF_D_SX_REG
#else
#define RF_D_SX_REG


#define RF_D_SX_REG_BASE                          (0x0)

#define RG_SX_A0                                  (RF_D_SX_REG_BASE + 0x0)
// Bit 0           rg_wf_sx_fcal_en               U     RW        default = 'h0
// Bit 2           rg_wf_sx_cali_start_man_mode     U     RW        default = 'h1
// Bit 4           rg_wf_sx_sdm_frac_en           U     RW        default = 'h1
// Bit 13  :8      rg_wf_sx_fcal_mode_sel         U     RW        default = 'h1b
// Bit 16          rg_wf_sx_rf_osc_double         U     RW        default = 'h0
// Bit 20          rg_wf_sx_dig_osc_double        U     RW        default = 'h0
// Bit 31          rg_wf_sx_soft_reset            U     RW        default = 'h0
typedef union RG_SX_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_FCAL_EN : 1;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF_SX_CALI_START_MAN_MODE : 1;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF_SX_SDM_FRAC_EN : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RG_WF_SX_FCAL_MODE_SEL : 6;
    unsigned int rsvd_3 : 2;
    unsigned int RG_WF_SX_RF_OSC_DOUBLE : 1;
    unsigned int rsvd_4 : 3;
    unsigned int RG_WF_SX_DIG_OSC_DOUBLE : 1;
    unsigned int rsvd_5 : 10;
    unsigned int RG_WF_SX_SOFT_RESET : 1;
  } b;
} RG_SX_A0_FIELD_T;

#define RG_SX_A1                                  (RF_D_SX_REG_BASE + 0x4)
// Bit 0           rg_wf_sx_fcal_sel_p            U     RW        default = 'h0
// Bit 10  :4      rg_wf_sx_vcoc_p                U     RW        default = 'h40
// Bit 12          rg_wf_sx_fcal_sel_t            U     RW        default = 'h0
// Bit 20  :16     rg_wf_sx_vcoc_t                U     RW        default = 'h10
// Bit 24          rg_wf_sx_ldo_fc_man            U     RW        default = 'h0
// Bit 28          rg_wf_sx_ldo_fc                U     RW        default = 'h0
typedef union RG_SX_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_FCAL_SEL_P : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_SX_VCOC_P : 7;
    unsigned int rsvd_1 : 1;
    unsigned int RG_WF_SX_FCAL_SEL_T : 1;
    unsigned int rsvd_2 : 3;
    unsigned int RG_WF_SX_VCOC_T : 5;
    unsigned int rsvd_3 : 3;
    unsigned int RG_WF_SX_LDO_FC_MAN : 1;
    unsigned int rsvd_4 : 3;
    unsigned int RG_WF_SX_LDO_FC : 1;
    unsigned int rsvd_5 : 3;
  } b;
} RG_SX_A1_FIELD_T;

#define RG_SX_A2                                  (RF_D_SX_REG_BASE + 0x8)
// Bit 6   :0      ro_da_wf_sx_vcoc_p             U     RO        default = 'h0
// Bit 20  :16     ro_da_wf_sx_vcoc_t             U     RO        default = 'h0
// Bit 23          ro_wf5g_sx_mmd_cali_ready      U     RO        default = 'h0
// Bit 24          ro_wf_sx_mxrtank_cali_ready     U     RO        default = 'h0
// Bit 25          ro_wf_sx_chn_map_ready         U     RO        default = 'h0
// Bit 26          ro_wf_sx_p_cali_ready          U     RO        default = 'h0
// Bit 27          ro_wf_sx_a_cali_ready          U     RO        default = 'h0
// Bit 28          ro_wf_sx_t_cali_ready          U     RO        default = 'h0
// Bit 29          ro_ad_wf_sx_ready              U     RO        default = 'h0
// Bit 31          ro_cali_end_flg                U     RO        default = 'h0
typedef union RG_SX_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_DA_WF_SX_VCOC_P : 7;
    unsigned int rsvd_0 : 9;
    unsigned int RO_DA_WF_SX_VCOC_T : 5;
    unsigned int rsvd_1 : 2;
    unsigned int RO_WF5G_SX_MMD_CALI_READY : 1;
    unsigned int RO_WF_SX_MXRTANK_CALI_READY : 1;
    unsigned int RO_WF_SX_CHN_MAP_READY : 1;
    unsigned int RO_WF_SX_P_CALI_READY : 1;
    unsigned int RO_WF_SX_A_CALI_READY : 1;
    unsigned int RO_WF_SX_T_CALI_READY : 1;
    unsigned int RO_AD_WF_SX_READY : 1;
    unsigned int rsvd_2 : 1;
    unsigned int RO_CALI_END_FLG : 1;
  } b;
} RG_SX_A2_FIELD_T;

#define RG_SX_A3                                  (RF_D_SX_REG_BASE + 0xc)
// Bit 1   :0      rg_wf_sx_ldo_fc_waittime       U     RW        default = 'h2
// Bit 5   :4      rg_wf_sx_set_code_waittime     U     RW        default = 'h1
// Bit 9   :8      rg_wf_sx_fcal_cw_waittime      U     RW        default = 'h1
// Bit 13  :12     rg_wf_sx_tr_wait_waittime      U     RW        default = 'h1
// Bit 17  :16     rg_wf_sx_acal_vco_waittime     U     RW        default = 'h1
// Bit 21  :20     rg_wf_sx_tr_switch_bnd         U     RW        default = 'h0
typedef union RG_SX_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LDO_FC_WAITTIME : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RG_WF_SX_SET_CODE_WAITTIME : 2;
    unsigned int rsvd_1 : 2;
    unsigned int RG_WF_SX_FCAL_CW_WAITTIME : 2;
    unsigned int rsvd_2 : 2;
    unsigned int RG_WF_SX_TR_WAIT_WAITTIME : 2;
    unsigned int rsvd_3 : 2;
    unsigned int RG_WF_SX_ACAL_VCO_WAITTIME : 2;
    unsigned int rsvd_4 : 2;
    unsigned int RG_WF_SX_TR_SWITCH_BND : 2;
    unsigned int rsvd_5 : 10;
  } b;
} RG_SX_A3_FIELD_T;

#define RG_SX_A4                                  (RF_D_SX_REG_BASE + 0x10)
// Bit 0           rg_wf_sx_vco_i_man             U     RW        default = 'h0
// Bit 7   :4      rg_wf_sx_vco_i                 U     RW        default = 'ha
// Bit 15  :8      rg_wf_sx_pd_vctrl_sel          U     RW        default = 'h3
typedef union RG_SX_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_VCO_I_MAN : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_SX_VCO_I : 4;
    unsigned int RG_WF_SX_PD_VCTRL_SEL : 8;
    unsigned int rsvd_1 : 16;
  } b;
} RG_SX_A4_FIELD_T;

#define RG_SX_A5                                  (RF_D_SX_REG_BASE + 0x14)
// Bit 3   :0      ro_wf_sx_vco_i                 U     RO        default = 'h0
// Bit 4           ro_da_wf_sx_vco_acfinish       U     RO        default = 'h0
// Bit 8           ro_ad_wf_sx_vco_alc_out        U     RO        default = 'h0
// Bit 23  :16     ro_wf_sx_pt_fsm                U     RO        default = 'h0
// Bit 30  :24     ro_wf_sx_top_fsm               U     RO        default = 'h0
typedef union RG_SX_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_SX_VCO_I : 4;
    unsigned int RO_DA_WF_SX_VCO_ACFINISH : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RO_AD_WF_SX_VCO_ALC_OUT : 1;
    unsigned int rsvd_1 : 7;
    unsigned int RO_WF_SX_PT_FSM : 8;
    unsigned int RO_WF_SX_TOP_FSM : 7;
    unsigned int rsvd_2 : 1;
  } b;
} RG_SX_A5_FIELD_T;

#define RG_SX_A6                                  (RF_D_SX_REG_BASE + 0x18)
// Bit 0           rg_sx_pdiv_sel                 U     RW        default = 'h1
// Bit 7   :4      rg_sx_osc_freq                 U     RW        default = 'h3
// Bit 15  :8      rg_2gb_sx_channel              U     RW        default = 'h8
// Bit 23  :16     rg_5gb_sx_channel              U     RW        default = 'h64
// Bit 27  :24     rg_wifi_freq_mode_man          U     RW        default = 'h0
// Bit 31          rg_wifi_freq_mode_man_mode     U     RW        default = 'h0
typedef union RG_SX_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_SX_PDIV_SEL : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_SX_OSC_FREQ : 4;
    unsigned int RG_2GB_SX_CHANNEL : 8;
    unsigned int RG_5GB_SX_CHANNEL : 8;
    unsigned int RG_WIFI_FREQ_MODE_MAN : 4;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WIFI_FREQ_MODE_MAN_MODE : 1;
  } b;
} RG_SX_A6_FIELD_T;

#define RG_SX_A7                                  (RF_D_SX_REG_BASE + 0x1c)
// Bit 0           rg_sx_rfch_man_en              U     RW        default = 'h0
// Bit 14  :4      rg_wf_sx_rfctrl_int            U     RW        default = 'h0
typedef union RG_SX_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_SX_RFCH_MAN_EN : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_SX_RFCTRL_INT : 11;
    unsigned int rsvd_1 : 17;
  } b;
} RG_SX_A7_FIELD_T;

#define RG_SX_A8                                  (RF_D_SX_REG_BASE + 0x20)
// Bit 24  :0      rg_wf_sx_rfctrl_frac           U     RW        default = 'h0
typedef union RG_SX_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_RFCTRL_FRAC : 25;
    unsigned int rsvd_0 : 7;
  } b;
} RG_SX_A8_FIELD_T;

#define RG_SX_A9                                  (RF_D_SX_REG_BASE + 0x24)
// Bit 15  :0      rg_sx_fcal_ntargt              U     RW        default = 'h0
// Bit 16          rg_sx_fcal_ntargt_man          U     RW        default = 'h0
typedef union RG_SX_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_SX_FCAL_NTARGT : 16;
    unsigned int RG_SX_FCAL_NTARGT_MAN : 1;
    unsigned int rsvd_0 : 15;
  } b;
} RG_SX_A9_FIELD_T;

#define RG_SX_A10                                 (RF_D_SX_REG_BASE + 0x28)
// Bit 15  :0      ro_wf_sx_fcal_cnt              U     RO        default = 'h0
// Bit 28  :16     ro_wf_sx_rf_freq_mhz           U     RO        default = 'h0
typedef union RG_SX_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_SX_FCAL_CNT : 16;
    unsigned int RO_WF_SX_RF_FREQ_MHZ : 13;
    unsigned int rsvd_0 : 3;
  } b;
} RG_SX_A10_FIELD_T;

#define RG_SX_A11                                 (RF_D_SX_REG_BASE + 0x2c)
// Bit 1   :0      rg_wf_sx_fcal_hvt              U     RW        default = 'h1
// Bit 5   :4      rg_wf_sx_fcal_lvt              U     RW        default = 'h1
typedef union RG_SX_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_FCAL_HVT : 2;
    unsigned int rsvd_0 : 2;
    unsigned int RG_WF_SX_FCAL_LVT : 2;
    unsigned int rsvd_1 : 26;
  } b;
} RG_SX_A11_FIELD_T;

#define RG_SX_A12                                 (RF_D_SX_REG_BASE + 0x30)
// Bit 10  :0      ro_sx_rfctrl_nint              U     RO        default = 'h0
typedef union RG_SX_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_SX_RFCTRL_NINT : 11;
    unsigned int rsvd_0 : 21;
  } b;
} RG_SX_A12_FIELD_T;

#define RG_SX_A13                                 (RF_D_SX_REG_BASE + 0x34)
// Bit 24  :0      ro_sx_rfctrl_nfrac             U     RO        default = 'h0
typedef union RG_SX_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_SX_RFCTRL_NFRAC : 25;
    unsigned int rsvd_0 : 7;
  } b;
} RG_SX_A13_FIELD_T;

#define RG_SX_A14                                 (RF_D_SX_REG_BASE + 0x38)
// Bit 15  :0      ro_sx_fcal_ntargt              U     RO        default = 'h0
typedef union RG_SX_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_SX_FCAL_NTARGT : 16;
    unsigned int rsvd_0 : 16;
  } b;
} RG_SX_A14_FIELD_T;

#define RG_SX_A15                                 (RF_D_SX_REG_BASE + 0x3c)
// Bit 6           rg_tr_switch_man               U     RW        default = 'h0
// Bit 7           rg_tr_switch_val               U     RW        default = 'h0
typedef union RG_SX_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 6;
    unsigned int RG_TR_SWITCH_MAN : 1;
    unsigned int RG_TR_SWITCH_VAL : 1;
    unsigned int rsvd_1 : 24;
  } b;
} RG_SX_A15_FIELD_T;

#define RG_SX_A16                                 (RF_D_SX_REG_BASE + 0x40)
// Bit 2   :0      rg_wf_sx_log_pdthreshold_man     U     RW        default = 'h7
// Bit 4           rg_wf_sx_log_pd_man_mode       U     RW        default = 'h0
// Bit 8           rg_wf_sx_log_pdtest_en         U     RW        default = 'h0
typedef union RG_SX_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG_PDTHRESHOLD_MAN : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF_SX_LOG_PD_MAN_MODE : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WF_SX_LOG_PDTEST_EN : 1;
    unsigned int rsvd_2 : 23;
  } b;
} RG_SX_A16_FIELD_T;

#define RG_SX_A17                                 (RF_D_SX_REG_BASE + 0x44)
// Bit 3   :0      rg_wf_sx_log5g_mxr_banksel_man0     U     RW        default = 'he
// Bit 7   :4      rg_wf_sx_log5g_mxr_banksel_man1     U     RW        default = 'hd
// Bit 11  :8      rg_wf_sx_log5g_mxr_banksel_man2     U     RW        default = 'hc
// Bit 15  :12     rg_wf_sx_log5g_mxr_banksel_man3     U     RW        default = 'hb
// Bit 19  :16     rg_wf_sx_log5g_mxr_banksel_man4     U     RW        default = 'ha
// Bit 23  :20     rg_wf_sx_log5g_mxr_banksel_man5     U     RW        default = 'h9
// Bit 27  :24     rg_wf_sx_log5g_mxr_banksel_man6     U     RW        default = 'h8
// Bit 31  :28     rg_wf_sx_log5g_mxr_banksel_man7     U     RW        default = 'h7
typedef union RG_SX_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN0 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN1 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN2 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN3 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN4 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN5 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN6 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN7 : 4;
  } b;
} RG_SX_A17_FIELD_T;

#define RG_SX_A18                                 (RF_D_SX_REG_BASE + 0x48)
// Bit 3   :0      rg_wf_sx_log5g_mxr_banksel_man8     U     RW        default = 'h6
// Bit 7   :4      rg_wf_sx_log5g_mxr_banksel_man9     U     RW        default = 'h4
// Bit 11  :8      rg_wf_sx_log2g_mxr_banksel_man     U     RW        default = 'h6
typedef union RG_SX_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN8 : 4;
    unsigned int RG_WF_SX_LOG5G_MXR_BANKSEL_MAN9 : 4;
    unsigned int RG_WF_SX_LOG2G_MXR_BANKSEL_MAN : 4;
    unsigned int rsvd_0 : 20;
  } b;
} RG_SX_A18_FIELD_T;

#define RG_SX_A20                                 (RF_D_SX_REG_BASE + 0x50)
// Bit 3   :0      ro_wf_sx_log5g_mxr_banksel_0     U     RO        default = 'h0
// Bit 7   :4      ro_wf_sx_log5g_mxr_banksel_1     U     RO        default = 'h0
// Bit 11  :8      ro_wf_sx_log5g_mxr_banksel_2     U     RO        default = 'h0
// Bit 15  :12     ro_wf_sx_log5g_mxr_banksel_3     U     RO        default = 'h0
// Bit 19  :16     ro_wf_sx_log5g_mxr_banksel_4     U     RO        default = 'h0
// Bit 23  :20     ro_wf_sx_log5g_mxr_banksel_5     U     RO        default = 'h0
// Bit 27  :24     ro_wf_sx_log5g_mxr_banksel_6     U     RO        default = 'h0
// Bit 31  :28     ro_wf_sx_log5g_mxr_banksel_7     U     RO        default = 'h0
typedef union RG_SX_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_0 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_1 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_2 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_3 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_4 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_5 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_6 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_7 : 4;
  } b;
} RG_SX_A20_FIELD_T;

#define RG_SX_A21                                 (RF_D_SX_REG_BASE + 0x54)
// Bit 3   :0      ro_wf_sx_log5g_mxr_banksel_8     U     RO        default = 'h0
// Bit 7   :4      ro_wf_sx_log5g_mxr_banksel_9     U     RO        default = 'h0
// Bit 11  :8      ro_wf_sx_log2g_mxr_banksel     U     RO        default = 'h0
typedef union RG_SX_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_8 : 4;
    unsigned int RO_WF_SX_LOG5G_MXR_BANKSEL_9 : 4;
    unsigned int RO_WF_SX_LOG2G_MXR_BANKSEL : 4;
    unsigned int rsvd_0 : 20;
  } b;
} RG_SX_A21_FIELD_T;

#define RG_SX_A23                                 (RF_D_SX_REG_BASE + 0x5c)
// Bit 0           rg_m0_wf2g_sx_vco_en           U     RW        default = 'h0
// Bit 1           rg_m0_wf2g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 2           rg_m0_wf2g_sx_pfd_en           U     RW        default = 'h0
// Bit 3           rg_m0_wf2g_sx_cp_en            U     RW        default = 'h0
// Bit 4           rg_m0_wf2g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 5           rg_m0_wf2g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 6           rg_m0_wf2g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 7           rg_m0_wf5g_sx_vco_s1_en        U     RW        default = 'h0
// Bit 8           rg_m0_wf5g_sx_vco_s2_en        U     RW        default = 'h0
// Bit 9           rg_m0_wf5g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 10          rg_m0_wf5g_sx_pfd_en           U     RW        default = 'h0
// Bit 11          rg_m0_wf5g_sx_cp_en            U     RW        default = 'h0
// Bit 12          rg_m0_wf5g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 13          rg_m0_wf5g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 14          rg_m0_wf5g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 15          rg_m0_wf_sx_log5g_mxr_en       U     RW        default = 'h0
// Bit 16          rg_m0_wf_sx_log5g_inv_en       U     RW        default = 'h0
// Bit 17          rg_m0_wf_sx_log5g_div_en       U     RW        default = 'h0
// Bit 18          rg_m0_wf_sx_log5g_iqdiv_en     U     RW        default = 'h0
// Bit 19          rg_m0_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m0_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m0_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m0_wf_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 23          rg_m0_wf_sx_log2g_div_en       U     RW        default = 'h0
// Bit 24          rg_m0_wf_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 25          rg_m0_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m0_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m0_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M0_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M0_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M0_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M0_WF2G_SX_CP_EN : 1;
    unsigned int RG_M0_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M0_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M0_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M0_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M0_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M0_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M0_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M0_WF5G_SX_CP_EN : 1;
    unsigned int RG_M0_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M0_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M0_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M0_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M0_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A23_FIELD_T;

#define RG_SX_A24                                 (RF_D_SX_REG_BASE + 0x60)
// Bit 0           rg_m1_wf2g_sx_vco_en           U     RW        default = 'h0
// Bit 1           rg_m1_wf2g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 2           rg_m1_wf2g_sx_pfd_en           U     RW        default = 'h0
// Bit 3           rg_m1_wf2g_sx_cp_en            U     RW        default = 'h0
// Bit 4           rg_m1_wf2g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 5           rg_m1_wf2g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 6           rg_m1_wf2g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 7           rg_m1_wf5g_sx_vco_s1_en        U     RW        default = 'h1
// Bit 8           rg_m1_wf5g_sx_vco_s2_en        U     RW        default = 'h1
// Bit 9           rg_m1_wf5g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 10          rg_m1_wf5g_sx_pfd_en           U     RW        default = 'h1
// Bit 11          rg_m1_wf5g_sx_cp_en            U     RW        default = 'h1
// Bit 12          rg_m1_wf5g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 13          rg_m1_wf5g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 14          rg_m1_wf5g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 15          rg_m1_wf_sx_log5g_mxr_en       U     RW        default = 'h1
// Bit 16          rg_m1_wf_sx_log5g_inv_en       U     RW        default = 'h1
// Bit 17          rg_m1_wf_sx_log5g_div_en       U     RW        default = 'h1
// Bit 18          rg_m1_wf_sx_log5g_iqdiv_en     U     RW        default = 'h1
// Bit 19          rg_m1_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m1_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m1_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m1_wf_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 23          rg_m1_wf_sx_log2g_div_en       U     RW        default = 'h0
// Bit 24          rg_m1_wf_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 25          rg_m1_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m1_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m1_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M1_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M1_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M1_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M1_WF2G_SX_CP_EN : 1;
    unsigned int RG_M1_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M1_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M1_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M1_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M1_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M1_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M1_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M1_WF5G_SX_CP_EN : 1;
    unsigned int RG_M1_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M1_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M1_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M1_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M1_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A24_FIELD_T;

#define RG_SX_A25                                 (RF_D_SX_REG_BASE + 0x64)
// Bit 0           rg_m2_wf2g_sx_vco_en           U     RW        default = 'h0
// Bit 1           rg_m2_wf2g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 2           rg_m2_wf2g_sx_pfd_en           U     RW        default = 'h0
// Bit 3           rg_m2_wf2g_sx_cp_en            U     RW        default = 'h0
// Bit 4           rg_m2_wf2g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 5           rg_m2_wf2g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 6           rg_m2_wf2g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 7           rg_m2_wf5g_sx_vco_s1_en        U     RW        default = 'h1
// Bit 8           rg_m2_wf5g_sx_vco_s2_en        U     RW        default = 'h1
// Bit 9           rg_m2_wf5g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 10          rg_m2_wf5g_sx_pfd_en           U     RW        default = 'h1
// Bit 11          rg_m2_wf5g_sx_cp_en            U     RW        default = 'h1
// Bit 12          rg_m2_wf5g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 13          rg_m2_wf5g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 14          rg_m2_wf5g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 15          rg_m2_wf_sx_log5g_mxr_en       U     RW        default = 'h1
// Bit 16          rg_m2_wf_sx_log5g_inv_en       U     RW        default = 'h1
// Bit 17          rg_m2_wf_sx_log5g_div_en       U     RW        default = 'h1
// Bit 18          rg_m2_wf_sx_log5g_iqdiv_en     U     RW        default = 'h1
// Bit 19          rg_m2_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m2_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m2_wf_sx_log5g_txlo_en      U     RW        default = 'h1
// Bit 22          rg_m2_wf_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 23          rg_m2_wf_sx_log2g_div_en       U     RW        default = 'h0
// Bit 24          rg_m2_wf_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 25          rg_m2_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m2_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m2_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A25_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M2_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M2_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M2_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M2_WF2G_SX_CP_EN : 1;
    unsigned int RG_M2_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M2_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M2_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M2_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M2_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M2_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M2_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M2_WF5G_SX_CP_EN : 1;
    unsigned int RG_M2_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M2_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M2_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M2_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M2_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A25_FIELD_T;

#define RG_SX_A26                                 (RF_D_SX_REG_BASE + 0x68)
// Bit 0           rg_m3_wf2g_sx_vco_en           U     RW        default = 'h0
// Bit 1           rg_m3_wf2g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 2           rg_m3_wf2g_sx_pfd_en           U     RW        default = 'h0
// Bit 3           rg_m3_wf2g_sx_cp_en            U     RW        default = 'h0
// Bit 4           rg_m3_wf2g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 5           rg_m3_wf2g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 6           rg_m3_wf2g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 7           rg_m3_wf5g_sx_vco_s1_en        U     RW        default = 'h1
// Bit 8           rg_m3_wf5g_sx_vco_s2_en        U     RW        default = 'h1
// Bit 9           rg_m3_wf5g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 10          rg_m3_wf5g_sx_pfd_en           U     RW        default = 'h1
// Bit 11          rg_m3_wf5g_sx_cp_en            U     RW        default = 'h1
// Bit 12          rg_m3_wf5g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 13          rg_m3_wf5g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 14          rg_m3_wf5g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 15          rg_m3_wf_sx_log5g_mxr_en       U     RW        default = 'h1
// Bit 16          rg_m3_wf_sx_log5g_inv_en       U     RW        default = 'h1
// Bit 17          rg_m3_wf_sx_log5g_div_en       U     RW        default = 'h1
// Bit 18          rg_m3_wf_sx_log5g_iqdiv_en     U     RW        default = 'h1
// Bit 19          rg_m3_wf_sx_log5g_rxlo_en      U     RW        default = 'h1
// Bit 20          rg_m3_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m3_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m3_wf_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 23          rg_m3_wf_sx_log2g_div_en       U     RW        default = 'h0
// Bit 24          rg_m3_wf_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 25          rg_m3_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m3_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m3_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A26_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M3_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M3_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M3_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M3_WF2G_SX_CP_EN : 1;
    unsigned int RG_M3_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M3_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M3_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M3_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M3_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M3_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M3_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M3_WF5G_SX_CP_EN : 1;
    unsigned int RG_M3_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M3_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M3_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M3_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M3_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A26_FIELD_T;

#define RG_SX_A27                                 (RF_D_SX_REG_BASE + 0x6c)
// Bit 31  :0      rg_wf_sx_dig_rsv0              U     RW        default = 'hffff
typedef union RG_SX_A27_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_DIG_RSV0 : 32;
  } b;
} RG_SX_A27_FIELD_T;

#define RG_SX_A32                                 (RF_D_SX_REG_BASE + 0x80)
// Bit 2   :0      rg_wf_sx_lpf_bw                U     RW        default = 'h7
// Bit 5   :4      rg_wf_sx_pfd_delay             U     RW        default = 'h0
// Bit 12  :8      rg_wf_sx_cp_offset             U     RW        default = 'h5
// Bit 17  :16     rg_wf_sx_stable_cnt            U     RW        default = 'h0
// Bit 20          rg_wf_sx_stable_en_man_mode     U     RW        default = 'h0
// Bit 24          rg_wf_sx_stable_en_man         U     RW        default = 'h0
typedef union RG_SX_A32_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LPF_BW : 3;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF_SX_PFD_DELAY : 2;
    unsigned int rsvd_1 : 2;
    unsigned int RG_WF_SX_CP_OFFSET : 5;
    unsigned int rsvd_2 : 3;
    unsigned int RG_WF_SX_STABLE_CNT : 2;
    unsigned int rsvd_3 : 2;
    unsigned int RG_WF_SX_STABLE_EN_MAN_MODE : 1;
    unsigned int rsvd_4 : 3;
    unsigned int RG_WF_SX_STABLE_EN_MAN : 1;
    unsigned int rsvd_5 : 7;
  } b;
} RG_SX_A32_FIELD_T;

#define RG_SX_A34                                 (RF_D_SX_REG_BASE + 0x88)
// Bit 8   :4      rg_wf_sx_vco_ldo_vref_adj      U     RW        default = 'h17
// Bit 20  :16     rg_wf_sx_cp_ldo_vref_adj       U     RW        default = 'h10
typedef union RG_SX_A34_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 4;
    unsigned int RG_WF_SX_VCO_LDO_VREF_ADJ : 5;
    unsigned int rsvd_1 : 7;
    unsigned int RG_WF_SX_CP_LDO_VREF_ADJ : 5;
    unsigned int rsvd_2 : 11;
  } b;
} RG_SX_A34_FIELD_T;

#define RG_SX_A35                                 (RF_D_SX_REG_BASE + 0x8c)
// Bit 2   :0      rg_wf_sx_cp_i                  U     RW        default = 'h5
// Bit 4   :3      rg_wf_sx_pfd_lock_t            U     RW        default = 'h0
// Bit 5           rg_wf_sx_pfd_ph_en             U     RW        default = 'h1
// Bit 6           rg_wf_sx_pfd_sel               U     RW        default = 'h0
// Bit 7           rg_wf_sx_mmd_pw_en             U     RW        default = 'h0
// Bit 8           rg_wf_sx_cp_bypass_mode        U     RW        default = 'h0
// Bit 9           rg_wf_sx_lpf_bypass_mode       U     RW        default = 'h0
// Bit 10          rg_wf_sx_lpf_resample_mode     U     RW        default = 'h1
// Bit 11          rg_wf_sx_cp_cali_ctrl          U     RW        default = 'h0
// Bit 12          rg_wf_sx_lpf_cali_ctrl         U     RW        default = 'h0
// Bit 13          rg_wf_sx_cp_rc_ctrl            U     RW        default = 'h0
// Bit 14          rg_wf_sx_ldo_hf_rc_ctrl        U     RW        default = 'h1
// Bit 15          rg_wf_sx_ldo_hf_rc_modesel     U     RW        default = 'h0
// Bit 16          rg_wf_sx_ldo_lf_rc_ctrl        U     RW        default = 'h1
// Bit 17          rg_wf_sx_ldo_lf_rc_modesel     U     RW        default = 'h0
// Bit 18          rg_wf_sx_pll_rst_man           U     RW        default = 'h0
// Bit 19          rg_wf_sx_pll_rst_man_val       U     RW        default = 'h0
// Bit 21  :20     rg_wf_sx_vctrl_sel             U     RW        default = 'h0
// Bit 23  :22     rg_wf_sx_vco_var_dc            U     RW        default = 'h3
// Bit 24          rg_wf_sx_fcal_div              U     RW        default = 'h1
// Bit 27  :25     rg_wf_sx_vco_2nd_capn          U     RW        default = 'h0
// Bit 30  :28     rg_wf_sx_vco_2nd_capp          U     RW        default = 'h0
// Bit 31          rg_sx_vco_lpf_testmode         U     RW        default = 'h0
typedef union RG_SX_A35_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_CP_I : 3;
    unsigned int RG_WF_SX_PFD_LOCK_T : 2;
    unsigned int RG_WF_SX_PFD_PH_EN : 1;
    unsigned int RG_WF_SX_PFD_SEL : 1;
    unsigned int RG_WF_SX_MMD_PW_EN : 1;
    unsigned int RG_WF_SX_CP_BYPASS_MODE : 1;
    unsigned int RG_WF_SX_LPF_BYPASS_MODE : 1;
    unsigned int RG_WF_SX_LPF_RESAMPLE_MODE : 1;
    unsigned int RG_WF_SX_CP_CALI_CTRL : 1;
    unsigned int RG_WF_SX_LPF_CALI_CTRL : 1;
    unsigned int RG_WF_SX_CP_RC_CTRL : 1;
    unsigned int RG_WF_SX_LDO_HF_RC_CTRL : 1;
    unsigned int RG_WF_SX_LDO_HF_RC_MODESEL : 1;
    unsigned int RG_WF_SX_LDO_LF_RC_CTRL : 1;
    unsigned int RG_WF_SX_LDO_LF_RC_MODESEL : 1;
    unsigned int RG_WF_SX_PLL_RST_MAN : 1;
    unsigned int RG_WF_SX_PLL_RST_MAN_VAL : 1;
    unsigned int RG_WF_SX_VCTRL_SEL : 2;
    unsigned int RG_WF_SX_VCO_VAR_DC : 2;
    unsigned int RG_WF_SX_FCAL_DIV : 1;
    unsigned int RG_WF_SX_VCO_2ND_CAPN : 3;
    unsigned int RG_WF_SX_VCO_2ND_CAPP : 3;
    unsigned int RG_SX_VCO_LPF_TESTMODE : 1;
  } b;
} RG_SX_A35_FIELD_T;

#define RG_SX_A36                                 (RF_D_SX_REG_BASE + 0x90)
// Bit 0           rg_wf_sx_log5g_pd_en_man       U     RW        default = 'h0
// Bit 4           rg_wf_sx_log2g_pd_en_man       U     RW        default = 'h0
// Bit 11  :8      rg_wf_sx_log5g_mxr_ictrl       U     RW        default = 'ha
// Bit 15  :12     rg_wf_sx_log5g_div_vdd         U     RW        default = 'h1
// Bit 31          rg_wf_sx_log5g_reserve1        U     RW        default = 'h0
typedef union RG_SX_A36_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG5G_PD_EN_MAN : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_SX_LOG2G_PD_EN_MAN : 1;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WF_SX_LOG5G_MXR_ICTRL : 4;
    unsigned int RG_WF_SX_LOG5G_DIV_VDD : 4;
    unsigned int rsvd_2 : 15;
    unsigned int RG_WF_SX_LOG5G_RESERVE1 : 1;
  } b;
} RG_SX_A36_FIELD_T;

#define RG_SX_A37                                 (RF_D_SX_REG_BASE + 0x94)
// Bit 3   :0      rg_wf_sx_log5g_rxlo_vdd        U     RW        default = 'h7
// Bit 7   :4      rg_wf_sx_log5g_dpdlo_vdd       U     RW        default = 'h7
// Bit 11  :8      rg_wf_sx_log5g_txlo_vdd        U     RW        default = 'hf
// Bit 15  :12     rg_wf_sx_log5g_iqdiv_rctrl     U     RW        default = 'h0
// Bit 20  :16     rg_wf_sx_log5g_iqdiv_ictrl     U     RW        default = 'h6
// Bit 27  :24     rg_wf_sx_log5g_buffer_vdd      U     RW        default = 'h6
typedef union RG_SX_A37_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG5G_RXLO_VDD : 4;
    unsigned int RG_WF_SX_LOG5G_DPDLO_VDD : 4;
    unsigned int RG_WF_SX_LOG5G_TXLO_VDD : 4;
    unsigned int RG_WF_SX_LOG5G_IQDIV_RCTRL : 4;
    unsigned int RG_WF_SX_LOG5G_IQDIV_ICTRL : 5;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF_SX_LOG5G_BUFFER_VDD : 4;
    unsigned int rsvd_1 : 4;
  } b;
} RG_SX_A37_FIELD_T;

#define RG_SX_A38                                 (RF_D_SX_REG_BASE + 0x98)
// Bit 3   :0      rg_wf_sx_log2g_mxr_vdd         U     RW        default = 'hf
// Bit 7   :4      rg_wf_sx_log2g_div_vdd         U     RW        default = 'h1
// Bit 11  :8      rg_wf_sx_log2g_iqdiv_vdd       U     RW        default = 'h1
// Bit 15  :12     rg_wf_sx_log2g_rxlo_vdd        U     RW        default = 'h4
// Bit 19  :16     rg_wf_sx_log2g_dpdlo_vdd       U     RW        default = 'h4
// Bit 23  :20     rg_wf_sx_log2g_txlo_vdd        U     RW        default = 'h4
// Bit 27  :24     rg_wf_sx_log2g_buffer_vdd      U     RW        default = 'h4
typedef union RG_SX_A38_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_LOG2G_MXR_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_DIV_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_IQDIV_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_RXLO_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_DPDLO_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_TXLO_VDD : 4;
    unsigned int RG_WF_SX_LOG2G_BUFFER_VDD : 4;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A38_FIELD_T;

#define RG_SX_A42                                 (RF_D_SX_REG_BASE + 0xa8)
// Bit 31  :0      rg_wf_sx_rsv0                  U     RW        default = 'h41
typedef union RG_SX_A42_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_RSV0 : 32;
  } b;
} RG_SX_A42_FIELD_T;

#define RG_SX_A43                                 (RF_D_SX_REG_BASE + 0xac)
// Bit 31  :0      rg_wf_sx_rsv1                  U     RW        default = 'hffff
typedef union RG_SX_A43_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_RSV1 : 32;
  } b;
} RG_SX_A43_FIELD_T;

#define RG_SX_A45                                 (RF_D_SX_REG_BASE + 0xb4)
// Bit 0           rg_wf_sx_open_drain_tp_en      U     RW        default = 'h0
// Bit 1           rg_wf_sx_vco_vctrl_tp_sel      U     RW        default = 'h0
// Bit 2           rg_wf_sx_tp_en                 U     RW        default = 'h0
// Bit 5   :3      rg_wf_sx_tp_sel                U     RW        default = 'h4
// Bit 6           rg_wf_sx_fb_clk_edge_sel       U     RW        default = 'h0
// Bit 7           rg_wf_sx_clk_edge_sel          U     RW        default = 'h0
// Bit 8           rg_wf2g_sx_oscbuf_en           U     RW        default = 'h1
// Bit 15  :12     rg_wf2g_sx_oscbuf_rsv          U     RW        default = 'h0
typedef union RG_SX_A45_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF_SX_OPEN_DRAIN_TP_EN : 1;
    unsigned int RG_WF_SX_VCO_VCTRL_TP_SEL : 1;
    unsigned int RG_WF_SX_TP_EN : 1;
    unsigned int RG_WF_SX_TP_SEL : 3;
    unsigned int RG_WF_SX_FB_CLK_EDGE_SEL : 1;
    unsigned int RG_WF_SX_CLK_EDGE_SEL : 1;
    unsigned int RG_WF2G_SX_OSCBUF_EN : 1;
    unsigned int rsvd_0 : 3;
    unsigned int RG_WF2G_SX_OSCBUF_RSV : 4;
    unsigned int rsvd_1 : 16;
  } b;
} RG_SX_A45_FIELD_T;

#define RG_SX_A46                                 (RF_D_SX_REG_BASE + 0xb8)
// Bit 0           rg_m4_wf2g_sx_vco_en           U     RW        default = 'h0
// Bit 1           rg_m4_wf2g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 2           rg_m4_wf2g_sx_pfd_en           U     RW        default = 'h0
// Bit 3           rg_m4_wf2g_sx_cp_en            U     RW        default = 'h0
// Bit 4           rg_m4_wf2g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 5           rg_m4_wf2g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 6           rg_m4_wf2g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 7           rg_m4_wf5g_sx_vco_s1_en        U     RW        default = 'h0
// Bit 8           rg_m4_wf5g_sx_vco_s2_en        U     RW        default = 'h0
// Bit 9           rg_m4_wf5g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 10          rg_m4_wf5g_sx_pfd_en           U     RW        default = 'h0
// Bit 11          rg_m4_wf5g_sx_cp_en            U     RW        default = 'h0
// Bit 12          rg_m4_wf5g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 13          rg_m4_wf5g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 14          rg_m4_wf5g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 15          rg_m4_wf_sx_log5g_mxr_en       U     RW        default = 'h0
// Bit 16          rg_m4_wf_sx_log5g_inv_en       U     RW        default = 'h0
// Bit 17          rg_m4_wf_sx_log5g_div_en       U     RW        default = 'h0
// Bit 18          rg_m4_wf_sx_log5g_iqdiv_en     U     RW        default = 'h0
// Bit 19          rg_m4_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m4_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m4_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m4_wf_sx_log2g_mxr_en       U     RW        default = 'h0
// Bit 23          rg_m4_wf_sx_log2g_div_en       U     RW        default = 'h0
// Bit 24          rg_m4_wf_sx_log2g_iqdiv_en     U     RW        default = 'h0
// Bit 25          rg_m4_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m4_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m4_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A46_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M4_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M4_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M4_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M4_WF2G_SX_CP_EN : 1;
    unsigned int RG_M4_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M4_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M4_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M4_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M4_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M4_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M4_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M4_WF5G_SX_CP_EN : 1;
    unsigned int RG_M4_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M4_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M4_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M4_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M4_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A46_FIELD_T;

#define RG_SX_A47                                 (RF_D_SX_REG_BASE + 0xbc)
// Bit 0           rg_m5_wf2g_sx_vco_en           U     RW        default = 'h1
// Bit 1           rg_m5_wf2g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 2           rg_m5_wf2g_sx_pfd_en           U     RW        default = 'h1
// Bit 3           rg_m5_wf2g_sx_cp_en            U     RW        default = 'h1
// Bit 4           rg_m5_wf2g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 5           rg_m5_wf2g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 6           rg_m5_wf2g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 7           rg_m5_wf5g_sx_vco_s1_en        U     RW        default = 'h0
// Bit 8           rg_m5_wf5g_sx_vco_s2_en        U     RW        default = 'h0
// Bit 9           rg_m5_wf5g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 10          rg_m5_wf5g_sx_pfd_en           U     RW        default = 'h0
// Bit 11          rg_m5_wf5g_sx_cp_en            U     RW        default = 'h0
// Bit 12          rg_m5_wf5g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 13          rg_m5_wf5g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 14          rg_m5_wf5g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 15          rg_m5_wf_sx_log5g_mxr_en       U     RW        default = 'h0
// Bit 16          rg_m5_wf_sx_log5g_inv_en       U     RW        default = 'h0
// Bit 17          rg_m5_wf_sx_log5g_div_en       U     RW        default = 'h0
// Bit 18          rg_m5_wf_sx_log5g_iqdiv_en     U     RW        default = 'h0
// Bit 19          rg_m5_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m5_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m5_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m5_wf_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 23          rg_m5_wf_sx_log2g_div_en       U     RW        default = 'h1
// Bit 24          rg_m5_wf_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 25          rg_m5_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m5_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m5_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A47_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M5_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M5_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M5_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M5_WF2G_SX_CP_EN : 1;
    unsigned int RG_M5_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M5_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M5_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M5_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M5_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M5_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M5_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M5_WF5G_SX_CP_EN : 1;
    unsigned int RG_M5_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M5_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M5_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M5_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M5_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A47_FIELD_T;

#define RG_SX_A48                                 (RF_D_SX_REG_BASE + 0xc0)
// Bit 0           rg_m6_wf2g_sx_vco_en           U     RW        default = 'h1
// Bit 1           rg_m6_wf2g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 2           rg_m6_wf2g_sx_pfd_en           U     RW        default = 'h1
// Bit 3           rg_m6_wf2g_sx_cp_en            U     RW        default = 'h1
// Bit 4           rg_m6_wf2g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 5           rg_m6_wf2g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 6           rg_m6_wf2g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 7           rg_m6_wf5g_sx_vco_s1_en        U     RW        default = 'h0
// Bit 8           rg_m6_wf5g_sx_vco_s2_en        U     RW        default = 'h0
// Bit 9           rg_m6_wf5g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 10          rg_m6_wf5g_sx_pfd_en           U     RW        default = 'h0
// Bit 11          rg_m6_wf5g_sx_cp_en            U     RW        default = 'h0
// Bit 12          rg_m6_wf5g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 13          rg_m6_wf5g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 14          rg_m6_wf5g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 15          rg_m6_wf_sx_log5g_mxr_en       U     RW        default = 'h0
// Bit 16          rg_m6_wf_sx_log5g_inv_en       U     RW        default = 'h0
// Bit 17          rg_m6_wf_sx_log5g_div_en       U     RW        default = 'h0
// Bit 18          rg_m6_wf_sx_log5g_iqdiv_en     U     RW        default = 'h0
// Bit 19          rg_m6_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m6_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m6_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m6_wf_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 23          rg_m6_wf_sx_log2g_div_en       U     RW        default = 'h1
// Bit 24          rg_m6_wf_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 25          rg_m6_wf_sx_log2g_rxlo_en      U     RW        default = 'h0
// Bit 26          rg_m6_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m6_wf_sx_log2g_txlo_en      U     RW        default = 'h1
typedef union RG_SX_A48_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M6_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M6_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M6_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M6_WF2G_SX_CP_EN : 1;
    unsigned int RG_M6_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M6_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M6_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M6_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M6_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M6_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M6_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M6_WF5G_SX_CP_EN : 1;
    unsigned int RG_M6_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M6_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M6_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M6_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M6_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A48_FIELD_T;

#define RG_SX_A49                                 (RF_D_SX_REG_BASE + 0xc4)
// Bit 0           rg_m7_wf2g_sx_vco_en           U     RW        default = 'h1
// Bit 1           rg_m7_wf2g_sx_vco_acal_en      U     RW        default = 'h1
// Bit 2           rg_m7_wf2g_sx_pfd_en           U     RW        default = 'h1
// Bit 3           rg_m7_wf2g_sx_cp_en            U     RW        default = 'h1
// Bit 4           rg_m7_wf2g_sx_vco_ldo_en       U     RW        default = 'h1
// Bit 5           rg_m7_wf2g_sx_cp_ldo_en        U     RW        default = 'h1
// Bit 6           rg_m7_wf2g_sx_lpf_comp_en      U     RW        default = 'h1
// Bit 7           rg_m7_wf5g_sx_vco_s1_en        U     RW        default = 'h0
// Bit 8           rg_m7_wf5g_sx_vco_s2_en        U     RW        default = 'h0
// Bit 9           rg_m7_wf5g_sx_vco_acal_en      U     RW        default = 'h0
// Bit 10          rg_m7_wf5g_sx_pfd_en           U     RW        default = 'h0
// Bit 11          rg_m7_wf5g_sx_cp_en            U     RW        default = 'h0
// Bit 12          rg_m7_wf5g_sx_vco_ldo_en       U     RW        default = 'h0
// Bit 13          rg_m7_wf5g_sx_cp_ldo_en        U     RW        default = 'h0
// Bit 14          rg_m7_wf5g_sx_lpf_comp_en      U     RW        default = 'h0
// Bit 15          rg_m7_wf_sx_log5g_mxr_en       U     RW        default = 'h0
// Bit 16          rg_m7_wf_sx_log5g_inv_en       U     RW        default = 'h0
// Bit 17          rg_m7_wf_sx_log5g_div_en       U     RW        default = 'h0
// Bit 18          rg_m7_wf_sx_log5g_iqdiv_en     U     RW        default = 'h0
// Bit 19          rg_m7_wf_sx_log5g_rxlo_en      U     RW        default = 'h0
// Bit 20          rg_m7_wf_sx_log5g_dpdlo_en     U     RW        default = 'h0
// Bit 21          rg_m7_wf_sx_log5g_txlo_en      U     RW        default = 'h0
// Bit 22          rg_m7_wf_sx_log2g_mxr_en       U     RW        default = 'h1
// Bit 23          rg_m7_wf_sx_log2g_div_en       U     RW        default = 'h1
// Bit 24          rg_m7_wf_sx_log2g_iqdiv_en     U     RW        default = 'h1
// Bit 25          rg_m7_wf_sx_log2g_rxlo_en      U     RW        default = 'h1
// Bit 26          rg_m7_wf_sx_log2g_dpdlo_en     U     RW        default = 'h0
// Bit 27          rg_m7_wf_sx_log2g_txlo_en      U     RW        default = 'h0
typedef union RG_SX_A49_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_M7_WF2G_SX_VCO_EN : 1;
    unsigned int RG_M7_WF2G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M7_WF2G_SX_PFD_EN : 1;
    unsigned int RG_M7_WF2G_SX_CP_EN : 1;
    unsigned int RG_M7_WF2G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M7_WF2G_SX_CP_LDO_EN : 1;
    unsigned int RG_M7_WF2G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M7_WF5G_SX_VCO_S1_EN : 1;
    unsigned int RG_M7_WF5G_SX_VCO_S2_EN : 1;
    unsigned int RG_M7_WF5G_SX_VCO_ACAL_EN : 1;
    unsigned int RG_M7_WF5G_SX_PFD_EN : 1;
    unsigned int RG_M7_WF5G_SX_CP_EN : 1;
    unsigned int RG_M7_WF5G_SX_VCO_LDO_EN : 1;
    unsigned int RG_M7_WF5G_SX_CP_LDO_EN : 1;
    unsigned int RG_M7_WF5G_SX_LPF_COMP_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_MXR_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_INV_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_DIV_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_IQDIV_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_RXLO_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_DPDLO_EN : 1;
    unsigned int RG_M7_WF_SX_LOG5G_TXLO_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_MXR_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_DIV_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_IQDIV_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_RXLO_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_DPDLO_EN : 1;
    unsigned int RG_M7_WF_SX_LOG2G_TXLO_EN : 1;
    unsigned int rsvd_0 : 4;
  } b;
} RG_SX_A49_FIELD_T;

#define RG_SX_A50                                 (RF_D_SX_REG_BASE + 0xc8)
// Bit 6   :0      ro_wf5g_sx_fx2_delay_ctrl_for_sw_cal     U     RO        default = 'h0
// Bit 8           ro_wf5g_sx_fx2_delay_ctrl_for_sw_cal_ready     U     RO        default = 'h0
// Bit 16          ro_wf5g_sx_fx2_duty_cal_flag     U     RO        default = 'h0
// Bit 17          rg_wf5g_sx_fx2_cali_en         U     RW        default = 'h0
// Bit 20          rg_wf5g_sx_cp_rc_sel           U     RW        default = 'h0
// Bit 21          rg_wf5g_sx_cp_offset_ctrl      U     RW        default = 'h0
// Bit 22          rg_wf5g_sx_lpf_comp_sel_h      U     RW        default = 'h0
// Bit 23          rg_wf5g_sx_lpf_comp_sel_l      U     RW        default = 'h0
// Bit 25  :24     rg_wf5g_sx_sdm_frac_sel        U     RW        default = 'h0
typedef union RG_SX_A50_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_FX2_DELAY_CTRL_FOR_SW_CAL : 7;
    unsigned int rsvd_0 : 1;
    unsigned int RO_WF5G_SX_FX2_DELAY_CTRL_FOR_SW_CAL_READY : 1;
    unsigned int rsvd_1 : 7;
    unsigned int RO_WF5G_SX_FX2_DUTY_CAL_FLAG : 1;
    unsigned int RG_WF5G_SX_FX2_CALI_EN : 1;
    unsigned int rsvd_2 : 2;
    unsigned int RG_WF5G_SX_CP_RC_SEL : 1;
    unsigned int RG_WF5G_SX_CP_OFFSET_CTRL : 1;
    unsigned int RG_WF5G_SX_LPF_COMP_SEL_H : 1;
    unsigned int RG_WF5G_SX_LPF_COMP_SEL_L : 1;
    unsigned int RG_WF5G_SX_SDM_FRAC_SEL : 2;
    unsigned int rsvd_3 : 6;
  } b;
} RG_SX_A50_FIELD_T;

#define RG_SX_A52                                 (RF_D_SX_REG_BASE + 0xd0)
// Bit 3   :0      rg_wf5g_sx_mmd_wait_time       U     RW        default = 'h0
// Bit 13  :4      rg_wf5g_sx_mmd_cali_time       U     RW        default = 'h80
// Bit 31          rg_wf5g_sx_mmd_en              U     RW        default = 'h1
typedef union RG_SX_A52_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_MMD_WAIT_TIME : 4;
    unsigned int RG_WF5G_SX_MMD_CALI_TIME : 10;
    unsigned int rsvd_0 : 17;
    unsigned int RG_WF5G_SX_MMD_EN : 1;
  } b;
} RG_SX_A52_FIELD_T;

#define RG_SX_A53                                 (RF_D_SX_REG_BASE + 0xd4)
// Bit 5   :0      rg_wf5g_sx_mmd_vcdl_1p_man     U     RW        default = 'hf
// Bit 11  :6      rg_wf5g_sx_mmd_vcdl_8p_man     U     RW        default = 'h20
// Bit 15  :12     rg_wf5g_sx_mmd_vcdl_s1_man     U     RW        default = 'h8
// Bit 31          rg_wf5g_sx_mmd_vcdl_man_mode     U     RW        default = 'h0
typedef union RG_SX_A53_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_MMD_VCDL_1P_MAN : 6;
    unsigned int RG_WF5G_SX_MMD_VCDL_8P_MAN : 6;
    unsigned int RG_WF5G_SX_MMD_VCDL_S1_MAN : 4;
    unsigned int rsvd_0 : 15;
    unsigned int RG_WF5G_SX_MMD_VCDL_MAN_MODE : 1;
  } b;
} RG_SX_A53_FIELD_T;

#define RG_SX_A54                                 (RF_D_SX_REG_BASE + 0xd8)
// Bit 5   :0      rg_wf5g_sx_mmd_vcdl2_1p        U     RW        default = 'hf
// Bit 12  :8      rg_wf5g_sx_mmd_vcdl2_8p        U     RW        default = 'hf
// Bit 20  :16     rg_wf5g_sx_mmd_vcdl2_32p       U     RW        default = 'hf
// Bit 26  :21     rg_wf5g_sx_mmd_dly1_man        U     RW        default = 'h20
typedef union RG_SX_A54_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_MMD_VCDL2_1P : 6;
    unsigned int rsvd_0 : 2;
    unsigned int RG_WF5G_SX_MMD_VCDL2_8P : 5;
    unsigned int rsvd_1 : 3;
    unsigned int RG_WF5G_SX_MMD_VCDL2_32P : 5;
    unsigned int RG_WF5G_SX_MMD_DLY1_MAN : 6;
    unsigned int rsvd_2 : 5;
  } b;
} RG_SX_A54_FIELD_T;

#define RG_SX_A55                                 (RF_D_SX_REG_BASE + 0xdc)
// Bit 5   :0      rg_wf5g_sx_mmd_dly2_man        U     RW        default = 'h20
// Bit 11  :6      rg_wf5g_sx_mmd_dly3_man        U     RW        default = 'h20
// Bit 17  :12     rg_wf5g_sx_mmd_dly4_man        U     RW        default = 'h20
// Bit 21  :18     rg_wf5g_sx_mmd_dly1_4p_man     U     RW        default = 'h8
// Bit 25  :22     rg_wf5g_sx_mmd_dly2_4p_man     U     RW        default = 'h8
// Bit 29  :26     rg_wf5g_sx_mmd_dly3_4p_man     U     RW        default = 'h8
// Bit 31          rg_wf5g_sx_mmd_dly_man_mode     U     RW        default = 'h0
typedef union RG_SX_A55_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_MMD_DLY2_MAN : 6;
    unsigned int RG_WF5G_SX_MMD_DLY3_MAN : 6;
    unsigned int RG_WF5G_SX_MMD_DLY4_MAN : 6;
    unsigned int RG_WF5G_SX_MMD_DLY1_4P_MAN : 4;
    unsigned int RG_WF5G_SX_MMD_DLY2_4P_MAN : 4;
    unsigned int RG_WF5G_SX_MMD_DLY3_4P_MAN : 4;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_SX_MMD_DLY_MAN_MODE : 1;
  } b;
} RG_SX_A55_FIELD_T;

#define RG_SX_A59                                 (RF_D_SX_REG_BASE + 0xec)
// Bit 3   :0      rg_wf5g_sx_cp_i                U     RW        default = 'hb
// Bit 5           rg_wf5g_sx_mmd_mux_en          U     RW        default = 'h1
// Bit 6           rg_wf5g_sx_zps_man             U     RW        default = 'h0
// Bit 7           rg_wf5g_sx_zps_man_in          U     RW        default = 'h0
// Bit 8           rg_wf5g_sx_rst_pfd_man         U     RW        default = 'h0
// Bit 9           rg_wf5g_sx_rst_pfd             U     RW        default = 'h0
// Bit 10          rg_wf5g_sx_rst_mmd_man         U     RW        default = 'h0
// Bit 11          rg_wf5g_sx_rst_mmd             U     RW        default = 'h0
// Bit 12          rg_wf5g_sx_mmd_hfvddrc_bypass     U     RW        default = 'h0
// Bit 13          rg_wf5g_sx_mmd_hfvddrc_ctrl     U     RW        default = 'h0
// Bit 14          rg_wf5g_sx_mmd_lfvddrc_bypass     U     RW        default = 'h0
// Bit 15          rg_wf5g_sx_mmd_lfvddrc_ctrl     U     RW        default = 'h0
// Bit 16          rg_wf5g_sx_vcodiv16_en         U     RW        default = 'h0
// Bit 17          rg_wf5g_sx_vcodiv16_rst        U     RW        default = 'h0
// Bit 18          rg_wf5g_sx_dcc2_en             U     RW        default = 'h0
// Bit 19          rg_wf5g_sx_lf_c1               U     RW        default = 'h1
// Bit 21  :20     rg_wf5g_sx_lf_rz               U     RW        default = 'h1
// Bit 22          rg_wf5g_sx_select_mpd          U     RW        default = 'h0
// Bit 23          rg_wf5g_sx_refdcc_dl34_pwr_man_in     U     RW        default = 'h0
// Bit 24          rg_wf5g_sx_refdcc_dl34_pwr_man     U     RW        default = 'h0
// Bit 28  :25     rg_wf5g_sx_vco_buf_vdd         U     RW        default = 'h8
// Bit 29          rg_wf5g_sx_mmd_pwr_en          U     RW        default = 'h1
// Bit 30          rg_wf5g_sx_mux_vddrc_bypass     U     RW        default = 'h0
typedef union RG_SX_A59_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_CP_I : 4;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_SX_MMD_MUX_EN : 1;
    unsigned int RG_WF5G_SX_ZPS_MAN : 1;
    unsigned int RG_WF5G_SX_ZPS_MAN_IN : 1;
    unsigned int RG_WF5G_SX_RST_PFD_MAN : 1;
    unsigned int RG_WF5G_SX_RST_PFD : 1;
    unsigned int RG_WF5G_SX_RST_MMD_MAN : 1;
    unsigned int RG_WF5G_SX_RST_MMD : 1;
    unsigned int RG_WF5G_SX_MMD_HFVDDRC_BYPASS : 1;
    unsigned int RG_WF5G_SX_MMD_HFVDDRC_CTRL : 1;
    unsigned int RG_WF5G_SX_MMD_LFVDDRC_BYPASS : 1;
    unsigned int RG_WF5G_SX_MMD_LFVDDRC_CTRL : 1;
    unsigned int RG_WF5G_SX_VCODIV16_EN : 1;
    unsigned int RG_WF5G_SX_VCODIV16_RST : 1;
    unsigned int RG_WF5G_SX_DCC2_EN : 1;
    unsigned int RG_WF5G_SX_LF_C1 : 1;
    unsigned int RG_WF5G_SX_LF_RZ : 2;
    unsigned int RG_WF5G_SX_SELECT_MPD : 1;
    unsigned int RG_WF5G_SX_REFDCC_DL34_PWR_MAN_IN : 1;
    unsigned int RG_WF5G_SX_REFDCC_DL34_PWR_MAN : 1;
    unsigned int RG_WF5G_SX_VCO_BUF_VDD : 4;
    unsigned int RG_WF5G_SX_MMD_PWR_EN : 1;
    unsigned int RG_WF5G_SX_MUX_VDDRC_BYPASS : 1;
    unsigned int rsvd_1 : 1;
  } b;
} RG_SX_A59_FIELD_T;

#define RG_SX_A60                                 (RF_D_SX_REG_BASE + 0xf0)
// Bit 14  :0      rg_wf5g_sx_dbl_delay1          U     RW        default = 'h1e00
// Bit 29  :15     rg_wf5g_sx_dbl_delay2          U     RW        default = 'h1e00
// Bit 30          rg_wf5g_sx_dbl_delay_man       U     RW        default = 'h0
typedef union RG_SX_A60_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_DBL_DELAY1 : 15;
    unsigned int RG_WF5G_SX_DBL_DELAY2 : 15;
    unsigned int RG_WF5G_SX_DBL_DELAY_MAN : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_SX_A60_FIELD_T;

#define RG_SX_A61                                 (RF_D_SX_REG_BASE + 0xf4)
// Bit 14  :0      rg_wf5g_sx_dbl_delay3          U     RW        default = 'h1e00
// Bit 29  :15     rg_wf5g_sx_dbl_delay4          U     RW        default = 'h1e00
typedef union RG_SX_A61_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_DBL_DELAY3 : 15;
    unsigned int RG_WF5G_SX_DBL_DELAY4 : 15;
    unsigned int rsvd_0 : 2;
  } b;
} RG_SX_A61_FIELD_T;

#define RG_SX_A62                                 (RF_D_SX_REG_BASE + 0xf8)
// Bit 10  :0      rg_wf5g_sx_refdcc_delay        U     RW        default = 'h100
// Bit 12          rg_wf5g_sx_refdcc_delay_man     U     RW        default = 'h0
// Bit 24          rg_wf5g_sx_dbl_cal_en          U     RW        default = 'h0
// Bit 25          rg_wf5g_sx_dbl_cal_en_man      U     RW        default = 'h0
// Bit 26          rg_wf5g_sx_refdcc_polarity     U     RW        default = 'h0
// Bit 27          rg_wf5g_sx_refdcc_polarity_man     U     RW        default = 'h0
// Bit 28          rg_wf5g_sx_refdcc_step         U     RW        default = 'h0
// Bit 29          rg_wf5g_sx_refdcc_step_man     U     RW        default = 'h0
typedef union RG_SX_A62_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_REFDCC_DELAY : 11;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WF5G_SX_REFDCC_DELAY_MAN : 1;
    unsigned int rsvd_1 : 11;
    unsigned int RG_WF5G_SX_DBL_CAL_EN : 1;
    unsigned int RG_WF5G_SX_DBL_CAL_EN_MAN : 1;
    unsigned int RG_WF5G_SX_REFDCC_POLARITY : 1;
    unsigned int RG_WF5G_SX_REFDCC_POLARITY_MAN : 1;
    unsigned int RG_WF5G_SX_REFDCC_STEP : 1;
    unsigned int RG_WF5G_SX_REFDCC_STEP_MAN : 1;
    unsigned int rsvd_2 : 2;
  } b;
} RG_SX_A62_FIELD_T;

#define RG_SX_A63                                 (RF_D_SX_REG_BASE + 0xfc)
// Bit 14  :0      ro_wf5g_sx_dbl_delay1          U     RO        default = 'h0
// Bit 29  :15     ro_wf5g_sx_dbl_delay2          U     RO        default = 'h0
// Bit 30          ro_wf5g_sx_dbl_half_out        U     RO        default = 'h0
typedef union RG_SX_A63_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_DBL_DELAY1 : 15;
    unsigned int RO_WF5G_SX_DBL_DELAY2 : 15;
    unsigned int RO_WF5G_SX_DBL_HALF_OUT : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_SX_A63_FIELD_T;

#define RG_SX_A64                                 (RF_D_SX_REG_BASE + 0x100)
// Bit 14  :0      ro_wf5g_sx_dbl_delay3          U     RO        default = 'h0
// Bit 29  :15     ro_wf5g_sx_dbl_delay4          U     RO        default = 'h0
// Bit 30          ro_wf5g_sx_dbl_full_out        U     RO        default = 'h0
typedef union RG_SX_A64_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_DBL_DELAY3 : 15;
    unsigned int RO_WF5G_SX_DBL_DELAY4 : 15;
    unsigned int RO_WF5G_SX_DBL_FULL_OUT : 1;
    unsigned int rsvd_0 : 1;
  } b;
} RG_SX_A64_FIELD_T;

#define RG_SX_A65                                 (RF_D_SX_REG_BASE + 0x104)
// Bit 0           ro_wf5g_sx_refdcc_delay        U     RO        default = 'h0
// Bit 1           ro_wf5g_sx_dbl_cal_en          U     RO        default = 'h0
// Bit 2           ro_wf5g_sx_refdcc_polarity     U     RO        default = 'h0
// Bit 3           ro_wf5g_sx_refdcc_step         U     RO        default = 'h0
// Bit 4           ro_wf5g_sx_refscan_step        U     RO        default = 'h0
// Bit 24  :5      ro_wf5g_sx_dcc2_high_cnt       U     RO        default = 'h0
typedef union RG_SX_A65_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_REFDCC_DELAY : 1;
    unsigned int RO_WF5G_SX_DBL_CAL_EN : 1;
    unsigned int RO_WF5G_SX_REFDCC_POLARITY : 1;
    unsigned int RO_WF5G_SX_REFDCC_STEP : 1;
    unsigned int RO_WF5G_SX_REFSCAN_STEP : 1;
    unsigned int RO_WF5G_SX_DCC2_HIGH_CNT : 20;
    unsigned int rsvd_0 : 7;
  } b;
} RG_SX_A65_FIELD_T;

#define RG_SX_A66                                 (RF_D_SX_REG_BASE + 0x108)
// Bit 19  :0      ro_wf5g_sx_dcc2_low_cnt        U     RO        default = 'h0
// Bit 24  :20     ro_wf5g_sx_vco_2nd_capn        U     RO        default = 'h0
// Bit 29  :25     ro_wf5g_sx_vco_2nd_capp        U     RO        default = 'h0
typedef union RG_SX_A66_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_DCC2_LOW_CNT : 20;
    unsigned int RO_WF5G_SX_VCO_2ND_CAPN : 5;
    unsigned int RO_WF5G_SX_VCO_2ND_CAPP : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_SX_A66_FIELD_T;

#define RG_SX_A67                                 (RF_D_SX_REG_BASE + 0x10c)
// Bit 4   :0      rg_wf5g_sx_vco_2nd_capn_man     U     RW        default = 'h0
// Bit 5           rg_wf5g_sx_vco_2nd_capn_man_mode     U     RW        default = 'h0
// Bit 10  :6      rg_wf5g_sx_vco_2nd_capp_man     U     RW        default = 'h0
// Bit 11          rg_wf5g_sx_vco_2nd_capp_man_mode     U     RW        default = 'h0
// Bit 13  :12     rg_wf5g_sx_kvco_sel            U     RW        default = 'h0
// Bit 18  :14     rg_wf5g_sx_cp_ioffset          U     RW        default = 'hf
// Bit 22  :20     rg_wifi_freq_2nd_mode_man      U     RW        default = 'h0
// Bit 23          rg_wifi_freq_2nd_mode_man_mode     U     RW        default = 'h0
typedef union RG_SX_A67_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_MAN : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_MAN_MODE : 1;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_MAN : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_MAN_MODE : 1;
    unsigned int RG_WF5G_SX_KVCO_SEL : 2;
    unsigned int RG_WF5G_SX_CP_IOFFSET : 5;
    unsigned int rsvd_0 : 1;
    unsigned int RG_WIFI_FREQ_2ND_MODE_MAN : 3;
    unsigned int RG_WIFI_FREQ_2ND_MODE_MAN_MODE : 1;
    unsigned int rsvd_1 : 8;
  } b;
} RG_SX_A67_FIELD_T;

#define RG_SX_A68                                 (RF_D_SX_REG_BASE + 0x110)
// Bit 4   :0      rg_wf5g_sx_vco_2nd_capn_f1     U     RW        default = 'h9
// Bit 9   :5      rg_wf5g_sx_vco_2nd_capp_f1     U     RW        default = 'h9
// Bit 14  :10     rg_wf5g_sx_vco_2nd_capn_f2     U     RW        default = 'ha
// Bit 19  :15     rg_wf5g_sx_vco_2nd_capp_f2     U     RW        default = 'ha
// Bit 24  :20     rg_wf5g_sx_vco_2nd_capn_f3     U     RW        default = 'hb
// Bit 29  :25     rg_wf5g_sx_vco_2nd_capp_f3     U     RW        default = 'hb
typedef union RG_SX_A68_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F1 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F1 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F2 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F2 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F3 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F3 : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_SX_A68_FIELD_T;

#define RG_SX_A69                                 (RF_D_SX_REG_BASE + 0x114)
// Bit 4   :0      rg_wf5g_sx_vco_2nd_capn_f4     U     RW        default = 'hc
// Bit 9   :5      rg_wf5g_sx_vco_2nd_capp_f4     U     RW        default = 'hc
// Bit 14  :10     rg_wf5g_sx_vco_2nd_capn_f5     U     RW        default = 'hd
// Bit 19  :15     rg_wf5g_sx_vco_2nd_capp_f5     U     RW        default = 'hd
// Bit 24  :20     rg_wf5g_sx_vco_2nd_capn_f6     U     RW        default = 'he
// Bit 29  :25     rg_wf5g_sx_vco_2nd_capp_f6     U     RW        default = 'he
typedef union RG_SX_A69_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F4 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F4 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F5 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F5 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F6 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F6 : 5;
    unsigned int rsvd_0 : 2;
  } b;
} RG_SX_A69_FIELD_T;

#define RG_SX_A70                                 (RF_D_SX_REG_BASE + 0x118)
// Bit 4   :0      rg_wf5g_sx_vco_2nd_capn_f7     U     RW        default = 'hf
// Bit 9   :5      rg_wf5g_sx_vco_2nd_capp_f7     U     RW        default = 'hf
// Bit 14  :10     rg_wf5g_sx_vco_2nd_capn_f8     U     RW        default = 'h10
// Bit 19  :15     rg_wf5g_sx_vco_2nd_capp_f8     U     RW        default = 'h10
typedef union RG_SX_A70_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F7 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F7 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPN_F8 : 5;
    unsigned int RG_WF5G_SX_VCO_2ND_CAPP_F8 : 5;
    unsigned int rsvd_0 : 12;
  } b;
} RG_SX_A70_FIELD_T;

#define RG_SX_A71                                 (RF_D_SX_REG_BASE + 0x11c)
// Bit 0           ro_wf5g_sx_refdcc_finish       U     RO        default = 'h0
typedef union RG_SX_A71_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int RO_WF5G_SX_REFDCC_FINISH : 1;
    unsigned int rsvd_0 : 31;
  } b;
} RG_SX_A71_FIELD_T;

#endif

