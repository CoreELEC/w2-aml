#ifdef CHIP_INTF_REG
#else
#define CHIP_INTF_REG


#define CHIP_INTF_REG_BASE                        (0xf00000)

#define RG_AON_A0                                 (CHIP_INTF_REG_BASE + 0x0)
// Bit 31  :0      rg_pin_mux0                    U     RW        default = 'h0
typedef union RG_AON_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux0 : 32;
  } b;
} RG_AON_A0_FIELD_T;

#define RG_AON_A1                                 (CHIP_INTF_REG_BASE + 0x4)
// Bit 31  :0      rg_pin_mux1                    U     RW        default = 'hfffffff0
typedef union RG_AON_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux1 : 32;
  } b;
} RG_AON_A1_FIELD_T;

#define RG_AON_A2                                 (CHIP_INTF_REG_BASE + 0x8)
// Bit 31  :0      rg_pin_mux2                    U     RW        default = 'hf
typedef union RG_AON_A2_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux2 : 32;
  } b;
} RG_AON_A2_FIELD_T;

#define RG_AON_A3                                 (CHIP_INTF_REG_BASE + 0xc)
// Bit 31  :0      rg_pin_mux3                    U     RW        default = 'h0
typedef union RG_AON_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux3 : 32;
  } b;
} RG_AON_A3_FIELD_T;

#define RG_AON_A4                                 (CHIP_INTF_REG_BASE + 0x10)
// Bit 31  :0      rg_pin_mux4                    U     RW        default = 'h0
typedef union RG_AON_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux4 : 32;
  } b;
} RG_AON_A4_FIELD_T;

#define RG_AON_A5                                 (CHIP_INTF_REG_BASE + 0x14)
// Bit 31  :0      rg_pin_mux5                    U     RW        default = 'h0
typedef union RG_AON_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux5 : 32;
  } b;
} RG_AON_A5_FIELD_T;

#define RG_AON_A6                                 (CHIP_INTF_REG_BASE + 0x18)
// Bit 31  :0      rg_pin_mux6                    U     RW        default = 'h0
typedef union RG_AON_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pin_mux6 : 32;
  } b;
} RG_AON_A6_FIELD_T;

#define RG_AON_A7                                 (CHIP_INTF_REG_BASE + 0x1c)
// Bit 31  :0      rg_pad_gpioa_pull_up           U     RW        default = 'hffffffff
typedef union RG_AON_A7_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pad_gpioA_pull_up : 32;
  } b;
} RG_AON_A7_FIELD_T;

#define RG_AON_A8                                 (CHIP_INTF_REG_BASE + 0x20)
// Bit 0           rg_test_n_gpio_en_n            U     RW        default = 'h0
typedef union RG_AON_A8_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_test_n_gpio_en_n : 1;
    unsigned int rsvd_0 : 31;
  } b;
} RG_AON_A8_FIELD_T;

#define RG_AON_A9                                 (CHIP_INTF_REG_BASE + 0x24)
// Bit 31  :0      rg_pad_gpioa_pull_up_en        U     RW        default = 'h158000
typedef union RG_AON_A9_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pad_gpioA_pull_up_en : 32;
  } b;
} RG_AON_A9_FIELD_T;

#define RG_AON_A10                                (CHIP_INTF_REG_BASE + 0x28)
// Bit 31  :0      rg_pad_gpioa_o                 U     RW        default = 'h7ffff
typedef union RG_AON_A10_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pad_gpioA_o : 32;
  } b;
} RG_AON_A10_FIELD_T;

#define RG_AON_A11                                (CHIP_INTF_REG_BASE + 0x2c)
// Bit 31  :0      rg_pad_gpioa_en_n              U     RW        default = 'h7ffff
typedef union RG_AON_A11_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pad_gpioA_en_n : 32;
  } b;
} RG_AON_A11_FIELD_T;

#define RG_AON_A12                                (CHIP_INTF_REG_BASE + 0x30)
// Bit 31  :0      ro_pad_gpioa_i                 U     RO        default = 'h0
typedef union RG_AON_A12_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_pad_gpioA_i : 32;
  } b;
} RG_AON_A12_FIELD_T;

#define RG_AON_A13                                (CHIP_INTF_REG_BASE + 0x34)
// Bit 31  :0      ro_pad_gpioa_pull_up           U     RO        default = 'h0
typedef union RG_AON_A13_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_pad_gpioA_pull_up : 32;
  } b;
} RG_AON_A13_FIELD_T;

#define RG_AON_A14                                (CHIP_INTF_REG_BASE + 0x38)
// Bit 31  :0      rg_aon_wf_cfg                  U     RW        default = 'h24
typedef union RG_AON_A14_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_aon_wf_cfg : 32;
  } b;
} RG_AON_A14_FIELD_T;

#define RG_AON_A15                                (CHIP_INTF_REG_BASE + 0x3c)
// Bit 31  :0      rg_aon_bt_cfg                  U     RW        default = 'h80000000
typedef union RG_AON_A15_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_aon_bt_cfg : 32;
  } b;
} RG_AON_A15_FIELD_T;

#define RG_AON_A16                                (CHIP_INTF_REG_BASE + 0x40)
// Bit 31  :0      rg_wf_debug0                   U     RW        default = 'h0
typedef union RG_AON_A16_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_debug0 : 32;
  } b;
} RG_AON_A16_FIELD_T;

#define RG_AON_A17                                (CHIP_INTF_REG_BASE + 0x44)
// Bit 31  :0      rg_wf_debug1                   U     RW        default = 'h0
typedef union RG_AON_A17_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_debug1 : 32;
  } b;
} RG_AON_A17_FIELD_T;

#define RG_AON_A18                                (CHIP_INTF_REG_BASE + 0x48)
// Bit 31  :0      rg_mac_timer_cfg1              U     RW        default = 'h10a00
typedef union RG_AON_A18_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_mac_timer_cfg1 : 32;
  } b;
} RG_AON_A18_FIELD_T;

#define RG_AON_A19                                (CHIP_INTF_REG_BASE + 0x4c)
// Bit 31  :0      rg_mac_time_low                U     RW        default = 'h0
typedef union RG_AON_A19_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_mac_time_low : 32;
  } b;
} RG_AON_A19_FIELD_T;

#define RG_AON_A20                                (CHIP_INTF_REG_BASE + 0x50)
// Bit 31  :0      rg_mac_time_high               U     RW        default = 'h0
typedef union RG_AON_A20_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_mac_time_high : 32;
  } b;
} RG_AON_A20_FIELD_T;

#define RG_AON_A21                                (CHIP_INTF_REG_BASE + 0x54)
// Bit 31  :0      rg_tsf_clk_cfg                 U     RW        default = 'h9
typedef union RG_AON_A21_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_tsf_clk_cfg : 32;
  } b;
} RG_AON_A21_FIELD_T;

#define RG_AON_A22                                (CHIP_INTF_REG_BASE + 0x58)
// Bit 31  :0      ro_prod_tst_sts0               U     RO        default = 'h0
typedef union RG_AON_A22_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_prod_tst_sts0 : 32;
  } b;
} RG_AON_A22_FIELD_T;

#define RG_AON_A23                                (CHIP_INTF_REG_BASE + 0x5c)
// Bit 31  :0      ro_prod_tst_sts1               U     RO        default = 'h0
typedef union RG_AON_A23_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_prod_tst_sts1 : 32;
  } b;
} RG_AON_A23_FIELD_T;

#define RG_AON_A24                                (CHIP_INTF_REG_BASE + 0x60)
// Bit 31  :0      rg_wf_debug2                   U     RW        default = 'h0
typedef union RG_AON_A24_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_debug2 : 32;
  } b;
} RG_AON_A24_FIELD_T;

#define RG_AON_A25                                (CHIP_INTF_REG_BASE + 0x64)
// Bit 31  :0      rg_wf_debug3                   U     RW        default = 'h0
typedef union RG_AON_A25_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wf_debug3 : 32;
  } b;
} RG_AON_A25_FIELD_T;

#define RG_AON_A29                                (CHIP_INTF_REG_BASE + 0x74)
// Bit 31  :0      rg_ana_bpll_cfg                U     RW        default = 'h0
typedef union RG_AON_A29_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_ana_bpll_cfg : 32;
  } b;
} RG_AON_A29_FIELD_T;

#define RG_AON_A30                                (CHIP_INTF_REG_BASE + 0x78)
// Bit 31  :0      rg_always_on_cfg4              U     RW        default = 'ha000d110
typedef union RG_AON_A30_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_always_on_cfg4 : 32;
  } b;
} RG_AON_A30_FIELD_T;

#define RG_AON_A31                                (CHIP_INTF_REG_BASE + 0x7c)
// Bit 31  :0      rg_always_on_cfg5              U     RW        default = 'ha000d110
typedef union RG_AON_A31_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_always_on_cfg5 : 32;
  } b;
} RG_AON_A31_FIELD_T;

#define RG_AON_A32                                (CHIP_INTF_REG_BASE + 0x80)
// Bit 31  :0      rg_mac_timer_cfg2              U     RW        default = 'h7d0001
typedef union RG_AON_A32_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_mac_timer_cfg2 : 32;
  } b;
} RG_AON_A32_FIELD_T;

#define RG_AON_A33                                (CHIP_INTF_REG_BASE + 0x84)
// Bit 31  :0      rg_mac_timer_cfg3              U     RW        default = 'h800100
typedef union RG_AON_A33_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_mac_timer_cfg3 : 32;
  } b;
} RG_AON_A33_FIELD_T;

#define RG_AON_A34                                (CHIP_INTF_REG_BASE + 0x88)
// Bit 31  :0      ro_always_on_sts1              U     RO        default = 'h0
typedef union RG_AON_A34_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_always_on_sts1 : 32;
  } b;
} RG_AON_A34_FIELD_T;

#define RG_AON_A35                                (CHIP_INTF_REG_BASE + 0x8c)
// Bit 31  :0      ro_always_on_sts2              U     RO        default = 'h0
typedef union RG_AON_A35_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_always_on_sts2 : 32;
  } b;
} RG_AON_A35_FIELD_T;

#define RG_AON_A36                                (CHIP_INTF_REG_BASE + 0x90)
// Bit 31  :0      ro_always_on_sts3              U     RO        default = 'h0
typedef union RG_AON_A36_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_always_on_sts3 : 32;
  } b;
} RG_AON_A36_FIELD_T;

#define RG_AON_A37                                (CHIP_INTF_REG_BASE + 0x94)
// Bit 31  :0      rg_intf_rsvd0                  U     RW        default = 'h1
typedef union RG_AON_A37_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_intf_rsvd0 : 32;
  } b;
} RG_AON_A37_FIELD_T;

#define RG_AON_A38                                (CHIP_INTF_REG_BASE + 0x98)
// Bit 31  :0      rg_pmu_power_dur_thr           U     RW        default = 'h0
typedef union RG_AON_A38_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pmu_power_dur_thr : 32;
  } b;
} RG_AON_A38_FIELD_T;

#define RG_AON_A39                                (CHIP_INTF_REG_BASE + 0x9c)
// Bit 31  :0      rg_gated_ctrl                  U     RW        default = 'h2
typedef union RG_AON_A39_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_gated_ctrl : 32;
  } b;
} RG_AON_A39_FIELD_T;

#define RG_AON_A40                                (CHIP_INTF_REG_BASE + 0xa0)
// Bit 7   :0      rg_dig_timebase                U     RW        default = 'h28
typedef union RG_AON_A40_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_dig_timebase : 8;
    unsigned int rsvd_0 : 24;
  } b;
} RG_AON_A40_FIELD_T;

#define RG_AON_A41                                (CHIP_INTF_REG_BASE + 0xa4)
// Bit 31  :0      ro_metal_rev                   U     RO        default = 'h0
typedef union RG_AON_A41_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_metal_rev : 32;
  } b;
} RG_AON_A41_FIELD_T;

#define RG_AON_A42                                (CHIP_INTF_REG_BASE + 0xa8)
// Bit 31  :0      rg_pmu_clk_cfg                 U     RW        default = 'h113
typedef union RG_AON_A42_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pmu_clk_cfg : 32;
  } b;
} RG_AON_A42_FIELD_T;

#define RG_AON_A44                                (CHIP_INTF_REG_BASE + 0xb0)
// Bit 31  :0      ro_efuse_rd0                   U     RO        default = 'h0
typedef union RG_AON_A44_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd0 : 32;
  } b;
} RG_AON_A44_FIELD_T;

#define RG_AON_A45                                (CHIP_INTF_REG_BASE + 0xb4)
// Bit 31  :0      ro_efuse_rd1                   U     RO        default = 'h0
typedef union RG_AON_A45_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd1 : 32;
  } b;
} RG_AON_A45_FIELD_T;

#define RG_AON_A46                                (CHIP_INTF_REG_BASE + 0xb8)
// Bit 31  :0      ro_efuse_rd2                   U     RO        default = 'h0
typedef union RG_AON_A46_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd2 : 32;
  } b;
} RG_AON_A46_FIELD_T;

#define RG_AON_A47                                (CHIP_INTF_REG_BASE + 0xbc)
// Bit 31  :0      ro_efuse_rd3                   U     RO        default = 'h0
typedef union RG_AON_A47_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd3 : 32;
  } b;
} RG_AON_A47_FIELD_T;

#define RG_AON_A48                                (CHIP_INTF_REG_BASE + 0xc0)
// Bit 31  :0      ro_efuse_rd4                   U     RO        default = 'h0
typedef union RG_AON_A48_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd4 : 32;
  } b;
} RG_AON_A48_FIELD_T;

#define RG_AON_A49                                (CHIP_INTF_REG_BASE + 0xc4)
// Bit 31  :0      ro_efuse_rd5                   U     RO        default = 'h0
typedef union RG_AON_A49_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd5 : 32;
  } b;
} RG_AON_A49_FIELD_T;

#define RG_AON_A50                                (CHIP_INTF_REG_BASE + 0xc8)
// Bit 31  :0      ro_efuse_rd6                   U     RO        default = 'h0
typedef union RG_AON_A50_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd6 : 32;
  } b;
} RG_AON_A50_FIELD_T;

#define RG_AON_A51                                (CHIP_INTF_REG_BASE + 0xcc)
// Bit 31  :0      ro_efuse_rd7                   U     RO        default = 'h0
typedef union RG_AON_A51_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_efuse_rd7 : 32;
  } b;
} RG_AON_A51_FIELD_T;

#define RG_AON_A52                                (CHIP_INTF_REG_BASE + 0xd0)
// Bit 31  :0      rg_for_debug0                  U     RW        default = 'h0
typedef union RG_AON_A52_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug0 : 32;
  } b;
} RG_AON_A52_FIELD_T;

#define RG_AON_A53                                (CHIP_INTF_REG_BASE + 0xd4)
// Bit 31  :0      rg_for_debug1                  U     RW        default = 'h0
typedef union RG_AON_A53_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug1 : 32;
  } b;
} RG_AON_A53_FIELD_T;

#define RG_AON_A54                                (CHIP_INTF_REG_BASE + 0xd8)
// Bit 31  :0      rg_for_debug2                  U     RW        default = 'h0
typedef union RG_AON_A54_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug2 : 32;
  } b;
} RG_AON_A54_FIELD_T;

#define RG_AON_A55                                (CHIP_INTF_REG_BASE + 0xdc)
// Bit 31  :0      rg_for_debug3                  U     RW        default = 'hffff
typedef union RG_AON_A55_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug3 : 32;
  } b;
} RG_AON_A55_FIELD_T;

#define RG_AON_A56                                (CHIP_INTF_REG_BASE + 0xe0)
// Bit 31  :0      rg_for_debug4                  U     RW        default = 'hffff
typedef union RG_AON_A56_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug4 : 32;
  } b;
} RG_AON_A56_FIELD_T;

#define RG_AON_A57                                (CHIP_INTF_REG_BASE + 0xe4)
// Bit 31  :0      rg_for_debug5                  U     RW        default = 'h0
typedef union RG_AON_A57_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug5 : 32;
  } b;
} RG_AON_A57_FIELD_T;

#define RG_AON_A58                                (CHIP_INTF_REG_BASE + 0xe8)
// Bit 31  :0      rg_for_debug6                  U     RW        default = 'h0
typedef union RG_AON_A58_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug6 : 32;
  } b;
} RG_AON_A58_FIELD_T;

#define RG_AON_A59                                (CHIP_INTF_REG_BASE + 0xec)
// Bit 31  :0      rg_for_debug7                  U     RW        default = 'h0
typedef union RG_AON_A59_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug7 : 32;
  } b;
} RG_AON_A59_FIELD_T;

#define RG_AON_A60                                (CHIP_INTF_REG_BASE + 0xf0)
// Bit 31  :0      rg_for_debug8                  U     RW        default = 'h0
typedef union RG_AON_A60_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug8 : 32;
  } b;
} RG_AON_A60_FIELD_T;

#define RG_AON_A61                                (CHIP_INTF_REG_BASE + 0xf4)
// Bit 31  :0      rg_for_debug9                  U     RW        default = 'hffff
typedef union RG_AON_A61_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_for_debug9 : 32;
  } b;
} RG_AON_A61_FIELD_T;

#define RG_AON_A62                                (CHIP_INTF_REG_BASE + 0xf8)
// Bit 31  :0      ro_ana_debug0                  U     RO        default = 'h0
typedef union RG_AON_A62_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_ana_debug0 : 32;
  } b;
} RG_AON_A62_FIELD_T;

#define RG_AON_A63                                (CHIP_INTF_REG_BASE + 0xfc)
// Bit 31  :0      ro_ana_debug1                  U     RO        default = 'h0
typedef union RG_AON_A63_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_ana_debug1 : 32;
  } b;
} RG_AON_A63_FIELD_T;

#define RG_AON_A64                                (CHIP_INTF_REG_BASE + 0x100)
// Bit 31  :0      ro_ana_debug2                  U     RO        default = 'h0
typedef union RG_AON_A64_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_ana_debug2 : 32;
  } b;
} RG_AON_A64_FIELD_T;

#define RG_AON_A65                                (CHIP_INTF_REG_BASE + 0x104)
// Bit 31  :0      ro_ana_debug3                  U     RO        default = 'h0
typedef union RG_AON_A65_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_ana_debug3 : 32;
  } b;
} RG_AON_A65_FIELD_T;

#define RG_AON_A66                                (CHIP_INTF_REG_BASE + 0x108)
// Bit 7   :0      rg_wifi_soft_rst_cfg           U     RW        default = 'h0
typedef union RG_AON_A66_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_wifi_soft_rst_cfg : 8;
    unsigned int rsvd_0 : 24;
  } b;
} RG_AON_A66_FIELD_T;

#define RG_AON_A67                                (CHIP_INTF_REG_BASE + 0x10c)
// Bit 0           rg_usb_clock_enable            U     RW        default = 'h1
// Bit 1           rg_usb_phy_clock_enable        U     RW        default = 'h1
// Bit 4           rg_usb_linestate_from_ana      U     RW        default = 'h0
typedef union RG_AON_A67_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_usb_clock_enable : 1;
    unsigned int rg_usb_phy_clock_enable : 1;
    unsigned int rsvd_0 : 2;
    unsigned int rg_usb_linestate_from_ana : 1;
    unsigned int rsvd_1 : 27;
  } b;
} RG_AON_A67_FIELD_T;

#define RG_AON_A68                                (CHIP_INTF_REG_BASE + 0x110)
// Bit 13  :0      rg_pcie_soft_rst_cfg           U     RW        default = 'h0
// Bit 18  :16     rg_pcie_clock_enable           U     RW        default = 'h0
// Bit 26  :24     rg_pcie_clock_gate             U     RW        default = 'h0
// Bit 31          rg_m31phy_pcie_rxodt_pmen_l0     U     RW        default = 'h0
typedef union RG_AON_A68_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_pcie_soft_rst_cfg : 14;
    unsigned int rsvd_0 : 2;
    unsigned int rg_pcie_clock_enable : 3;
    unsigned int rsvd_1 : 5;
    unsigned int rg_pcie_clock_gate : 3;
    unsigned int rsvd_2 : 4;
    unsigned int rg_m31phy_pcie_RXODT_PMEN_L0 : 1;
  } b;
} RG_AON_A68_FIELD_T;

#define RG_AON_A69                                (CHIP_INTF_REG_BASE + 0x114)
// Bit 6   :0      reg_sdio_ema_cfg               U     RW        default = 'h77
// Bit 14  :8      reg_usb_ema_cfg                U     RW        default = 'h77
// Bit 22  :16     reg_pcie_ema_cfg               U     RW        default = 'h77
// Bit 29  :24     reg_shareram_ema_cfg           U     RW        default = 'h1a
typedef union RG_AON_A69_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int reg_sdio_ema_cfg : 7;
    unsigned int rsvd_0 : 1;
    unsigned int reg_usb_ema_cfg : 7;
    unsigned int rsvd_1 : 1;
    unsigned int reg_pcie_ema_cfg : 7;
    unsigned int rsvd_2 : 1;
    unsigned int reg_shareram_ema_cfg : 6;
    unsigned int rsvd_3 : 2;
  } b;
} RG_AON_A69_FIELD_T;

#define RG_AON_A70                                (CHIP_INTF_REG_BASE + 0x118)
// Bit 0           rg_clk32k_mes_en               U     RW        default = 'h0
// Bit 11  :4      rg_clk32k_mes_bnd              U     RW        default = 'h1
// Bit 31          ro_clk32k_mes_end_flag         U     RO        default = 'h0
typedef union RG_AON_A70_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_clk32k_mes_en : 1;
    unsigned int rsvd_0 : 3;
    unsigned int rg_clk32k_mes_bnd : 8;
    unsigned int rsvd_1 : 19;
    unsigned int ro_clk32k_mes_end_flag : 1;
  } b;
} RG_AON_A70_FIELD_T;

#define RG_AON_A71                                (CHIP_INTF_REG_BASE + 0x11c)
// Bit 31  :0      ro_clk32k_mes_cnt_pos          U     RO        default = 'h0
typedef union RG_AON_A71_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_clk32k_mes_cnt_pos : 32;
  } b;
} RG_AON_A71_FIELD_T;

#define RG_AON_A72                                (CHIP_INTF_REG_BASE + 0x120)
// Bit 31  :0      ro_clk32k_mes_cnt_neg          U     RO        default = 'h0
typedef union RG_AON_A72_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_clk32k_mes_cnt_neg : 32;
  } b;
} RG_AON_A72_FIELD_T;

#define RG_AON_A73                                (CHIP_INTF_REG_BASE + 0x124)
// Bit 10  :0      rg_cpu_osc_clk_div             U     RW        default = 'h4e1
typedef union RG_AON_A73_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_cpu_osc_clk_div : 11;
    unsigned int rsvd_0 : 21;
  } b;
} RG_AON_A73_FIELD_T;

#define RG_AON_A74                                (CHIP_INTF_REG_BASE + 0x128)
// Bit 0           rg_cpu_osc_clk_en              U     RW        default = 'h0
typedef union RG_AON_A74_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_cpu_osc_clk_en : 1;
    unsigned int rsvd_0 : 31;
  } b;
} RG_AON_A74_FIELD_T;

#define RG_AON_A75                                (CHIP_INTF_REG_BASE + 0x12c)
// Bit 0           rg_sclk_32k_s                  U     RW        default = 'h0
// Bit 13  :8      reg_iccm_ema_cfg               U     RW        default = 'h1a
typedef union RG_AON_A75_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_sclk_32k_s : 1;
    unsigned int rsvd_0 : 7;
    unsigned int reg_iccm_ema_cfg : 6;
    unsigned int rsvd_1 : 18;
  } b;
} RG_AON_A75_FIELD_T;

#define RG_AON_A76                                (CHIP_INTF_REG_BASE + 0x130)
// Bit 1   :0      ro_usb_linestate               U     RO        default = 'h0
// Bit 4           ro_pcie_pl_rxelecidle          U     RO        default = 'h0
// Bit 8           ro_pcie_pl_l2_exit_n           U     RO        default = 'h0
typedef union RG_AON_A76_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int ro_usb_linestate : 2;
    unsigned int rsvd_0 : 2;
    unsigned int ro_pcie_pl_rxelecidle : 1;
    unsigned int rsvd_1 : 3;
    unsigned int ro_pcie_pl_l2_exit_n : 1;
    unsigned int rsvd_2 : 23;
  } b;
} RG_AON_A76_FIELD_T;

#endif

