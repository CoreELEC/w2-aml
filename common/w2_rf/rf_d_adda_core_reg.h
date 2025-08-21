#ifdef RF_D_ADDA_CORE_REG
#else
#define RF_D_ADDA_CORE_REG


#define RF_D_ADDA0_CORE_REG_BASE                   (0xa0e000)
#define RF_D_ADDA1_CORE_REG_BASE                   (0xa0f000)


#define RG_CORE_A0                                ( 0x0)
// Bit 31  :0      rg_core_cfg0                   U     RW        default = 'h0
typedef union RG_CORE_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_core_cfg0 : 32;
  } b;
} RG_CORE_A0_FIELD_T;

#define RG_CORE_A1                                ( 0x4)
// Bit 31  :0      rg_core_cfg1                   U     RW        default = 'h0
typedef union RG_CORE_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_core_cfg1 : 32;
  } b;
} RG_CORE_A1_FIELD_T;

#define RG_CORE_A2                                ( 0x8)
// Bit 0           rg_core_enb_man                U     RW        default = 'h0
// Bit 4           rg_core_enb_man_en             U     RW        default = 'h0
// Bit 8           rg_xmit_enb_man                U     RW        default = 'h0
// Bit 12          rg_xmit_enb_man_en             U     RW        default = 'h0
// Bit 16          rg_recv_enb_man                U     RW        default = 'h0
// Bit 20          rg_recv_enb_man_en             U     RW        default = 'h0
// Bit 24          rg_esti_enb_man                U     RW        default = 'h0
// Bit 28          rg_esti_enb_man_en             U     RW        default = 'h0
typedef union RG_CORE_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_core_enb_man : 1;
    unsigned int rsvd_0 : 3;
    unsigned int rg_core_enb_man_en : 1;
    unsigned int rsvd_1 : 3;
    unsigned int rg_xmit_enb_man : 1;
    unsigned int rsvd_2 : 3;
    unsigned int rg_xmit_enb_man_en : 1;
    unsigned int rsvd_3 : 3;
    unsigned int rg_recv_enb_man : 1;
    unsigned int rsvd_4 : 3;
    unsigned int rg_recv_enb_man_en : 1;
    unsigned int rsvd_5 : 3;
    unsigned int rg_esti_enb_man : 1;
    unsigned int rsvd_6 : 3;
    unsigned int rg_esti_enb_man_en : 1;
    unsigned int rsvd_7 : 3;
  } b;
} RG_CORE_A2_FIELD_T;

#define RG_CORE_A3                                ( 0xc)
// Bit 12  :0      rg_freq_mhz_man                U     RW        default = 'h0
// Bit 15          rg_freq_mhz_man_en             U     RW        default = 'h0
// Bit 24          rg_2g_5g_man                   U     RW        default = 'h0
// Bit 31          rg_2g_5g_man_en                U     RW        default = 'h0
typedef union RG_CORE_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_freq_mhz_man : 13;
    unsigned int rsvd_0 : 2;
    unsigned int rg_freq_mhz_man_en : 1;
    unsigned int rsvd_1 : 8;
    unsigned int rg_2g_5g_man : 1;
    unsigned int rsvd_2 : 6;
    unsigned int rg_2g_5g_man_en : 1;
  } b;
} RG_CORE_A3_FIELD_T;

#define RG_CORE_A4                                ( 0x10)
// Bit 7   :0      rg_tx_pwrdbm_man               U     RW        default = 'h0
// Bit 8           rg_tx_pwrdbm_man_en            U     RW        default = 'h0
// Bit 12          rg_tx_pwrdbmv_man              U     RW        default = 'h0
// Bit 13          rg_tx_pwrdbmv_man_en           U     RW        default = 'h0
// Bit 17  :16     rg_tx_bw_man                   U     RW        default = 'h0
// Bit 20          rg_tx_bw_man_en                U     RW        default = 'h0
// Bit 25  :24     rg_dac_fs_man                  U     RW        default = 'h0
// Bit 28          rg_dac_fs_man_en               U     RW        default = 'h0
typedef union RG_CORE_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_tx_pwrdbm_man : 8;
    unsigned int rg_tx_pwrdbm_man_en : 1;
    unsigned int rsvd_0 : 3;
    unsigned int rg_tx_pwrdbmv_man : 1;
    unsigned int rg_tx_pwrdbmv_man_en : 1;
    unsigned int rsvd_1 : 2;
    unsigned int rg_tx_bw_man : 2;
    unsigned int rsvd_2 : 2;
    unsigned int rg_tx_bw_man_en : 1;
    unsigned int rsvd_3 : 3;
    unsigned int rg_dac_fs_man : 2;
    unsigned int rsvd_4 : 2;
    unsigned int rg_dac_fs_man_en : 1;
    unsigned int rsvd_5 : 3;
  } b;
} RG_CORE_A4_FIELD_T;

#define RG_CORE_A5                                ( 0x14)
// Bit 8   :0      rg_rx_gain_man                 U     RW        default = 'h0
// Bit 9           rg_rx_gain_man_en              U     RW        default = 'h0
// Bit 17  :16     rg_rx_bw_man                   U     RW        default = 'h0
// Bit 20          rg_rx_bw_man_en                U     RW        default = 'h0
// Bit 25  :24     rg_adc_fs_man                  U     RW        default = 'h0
// Bit 28          rg_adc_fs_man_en               U     RW        default = 'h0
typedef union RG_CORE_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_rx_gain_man : 9;
    unsigned int rg_rx_gain_man_en : 1;
    unsigned int rsvd_0 : 6;
    unsigned int rg_rx_bw_man : 2;
    unsigned int rsvd_1 : 2;
    unsigned int rg_rx_bw_man_en : 1;
    unsigned int rsvd_2 : 3;
    unsigned int rg_adc_fs_man : 2;
    unsigned int rsvd_3 : 2;
    unsigned int rg_adc_fs_man_en : 1;
    unsigned int rsvd_4 : 3;
  } b;
} RG_CORE_A5_FIELD_T;

#define RG_CORE_A6                                ( 0x18)
// Bit 1   :0      rg_rf_mode_man                 U     RW        default = 'h0
// Bit 4           rg_rf_mode_man_en              U     RW        default = 'h0
// Bit 11  :8      rg_cali_mode_man               U     RW        default = 'h0
// Bit 16          rg_cali_mode_man_en            U     RW        default = 'h0
// Bit 20          rg_tx_rx_man_en                U     RW        default = 'h0
// Bit 24          rg_tx_rx_man                   U     RW        default = 'h0
typedef union RG_CORE_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_rf_mode_man : 2;
    unsigned int rsvd_0 : 2;
    unsigned int rg_rf_mode_man_en : 1;
    unsigned int rsvd_1 : 3;
    unsigned int rg_cali_mode_man : 4;
    unsigned int rsvd_2 : 4;
    unsigned int rg_cali_mode_man_en : 1;
    unsigned int rsvd_3 : 3;
    unsigned int rg_tx_rx_man_en : 1;
    unsigned int rsvd_4 : 3;
    unsigned int rg_tx_rx_man : 1;
    unsigned int rsvd_5 : 7;
  } b;
} RG_CORE_A6_FIELD_T;

#define RG_CORE_A7                                ( 0x1c)
// Bit 7   :0      rg_wifi_adda_soft_rst_ctrl     U     RW        default = 'h0
typedef union RG_CORE_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wifi_adda_soft_rst_ctrl : 8;
    unsigned int rsvd_0 : 24;
  } b;
} RG_CORE_A7_FIELD_T;

#define RG_CORE_A8                                ( 0x20)
// Bit 7   :0      rg_wf_ana2g_channel_frombb     U     RW        default = 'h0
// Bit 15  :8      rg_wf_ana5g_channel_frombb     U     RW        default = 'h0
// Bit 18  :16     rg_wf_ana_mode_frombb          U     RW        default = 'h0
// Bit 24          rg_wf_ana_cali_start_frombb     U     RW        default = 'h0
// Bit 28          rg_wf_ana2g_channel_frombb_mode     U     RW        default = 'h0
// Bit 29          rg_wf_ana5g_channel_frombb_mode     U     RW        default = 'h0
// Bit 30          rg_wf_ana_mode_frombb_mode     U     RW        default = 'h0
// Bit 31          rg_wf_ana_cali_start_frombb_mode     U     RW        default = 'h0
typedef union RG_CORE_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_ana2g_channel_frombb : 8;
    unsigned int rg_wf_ana5g_channel_frombb : 8;
    unsigned int rg_wf_ana_mode_frombb : 3;
    unsigned int rsvd_0 : 5;
    unsigned int rg_wf_ana_cali_start_frombb : 1;
    unsigned int rsvd_1 : 3;
    unsigned int rg_wf_ana2g_channel_frombb_mode : 1;
    unsigned int rg_wf_ana5g_channel_frombb_mode : 1;
    unsigned int rg_wf_ana_mode_frombb_mode : 1;
    unsigned int rg_wf_ana_cali_start_frombb_mode : 1;
  } b;
} RG_CORE_A8_FIELD_T;

#define RG_CORE_A9                                ( 0x24)
// Bit 4   :0      rg_testbus_sel                 U     RW        default = 'h0
typedef union RG_CORE_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_testbus_sel : 5;
    unsigned int rsvd_0 : 27;
  } b;
} RG_CORE_A9_FIELD_T;

#define RG_CORE_A10                               ( 0x28)
// Bit 1   :0      rg_bt_rf_ttg_mode              U     RW        default = 'h0
typedef union RG_CORE_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bt_rf_ttg_mode : 2;
    unsigned int rsvd_0 : 30;
  } b;
} RG_CORE_A10_FIELD_T;

#define RG_CORE_A11                               ( 0x2c)
// Bit 3   :0      rg_tx_pwrlvl_man               U     RW        default = 'h0
// Bit 8           rg_tx_pwrlvl_man_en            U     RW        default = 'h0
typedef union RG_CORE_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_tx_pwrlvl_man : 4;
    unsigned int rsvd_0 : 4;
    unsigned int rg_tx_pwrlvl_man_en : 1;
    unsigned int rsvd_1 : 23;
  } b;
} RG_CORE_A11_FIELD_T;

#endif

