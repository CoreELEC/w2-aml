#ifndef DRI_TEST_CMD
#define DRI_TEST_CMD

#include "fi_sdio.h"

#define TSSI_5G_CAL_NUM 4

typedef struct W2_EFUSE_PARAM
{
    unsigned char customer_efuse_en;
    unsigned char FT_efuse_en;
    unsigned char xosc_ctune_vld;

    unsigned char absolute_power_wf0_2g_vld;
    unsigned char absolute_power_wf0_5g_vld;
    unsigned char absolute_power_wf1_2g_vld;
    unsigned char absolute_power_wf1_5g_vld;

    unsigned char offset_power_wf0_2g_vld;
    unsigned char offset_power_wf0_5g_vld;
    unsigned char offset_power_wf1_2g_vld;
    unsigned char offset_power_wf1_5g_vld;

    unsigned char pwr_delta_wf0_customer_vld;
    unsigned char pwr_delta_wf1_customer_vld;

    unsigned char xosc_ctune;

    unsigned char absolute_power_wf0_2g_l;
    unsigned char absolute_power_wf0_2g_m;
    unsigned char absolute_power_wf0_2g_h;
    unsigned char absolute_power_wf0_5200;
    unsigned char absolute_power_wf0_5300;
    unsigned char absolute_power_wf0_5530;
    unsigned char absolute_power_wf0_5660;
    unsigned char absolute_power_wf0_5780;

    unsigned char absolute_power_wf1_2g_l;
    unsigned char absolute_power_wf1_2g_m;
    unsigned char absolute_power_wf1_2g_h;
    unsigned char absolute_power_wf1_5200;
    unsigned char absolute_power_wf1_5300;
    unsigned char absolute_power_wf1_5530;
    unsigned char absolute_power_wf1_5660;
    unsigned char absolute_power_wf1_5780;

    unsigned char offset_power_wf0_2g_l;
    unsigned char offset_power_wf0_2g_m;
    unsigned char offset_power_wf0_2g_h;
    unsigned char offset_power_wf0_5200;
    unsigned char offset_power_wf0_5300;
    unsigned char offset_power_wf0_5530;
    unsigned char offset_power_wf0_5660;
    unsigned char offset_power_wf0_5780;

    unsigned char offset_power_wf1_2g_l;
    unsigned char offset_power_wf1_2g_m;
    unsigned char offset_power_wf1_2g_h;
    unsigned char offset_power_wf1_5200;
    unsigned char offset_power_wf1_5300;
    unsigned char offset_power_wf1_5530;
    unsigned char offset_power_wf1_5660;
    unsigned char offset_power_wf1_5780;

    unsigned char second_offset_power_wf0_2g_l;
    unsigned char second_offset_power_wf0_2g_m;
    unsigned char second_offset_power_wf0_2g_h;
    unsigned char second_offset_power_wf0_5200;
    unsigned char second_offset_power_wf0_5300;
    unsigned char second_offset_power_wf0_5530;
    unsigned char second_offset_power_wf0_5660;
    unsigned char second_offset_power_wf0_5780;

    unsigned char second_offset_power_wf1_2g_l;
    unsigned char second_offset_power_wf1_2g_m;
    unsigned char second_offset_power_wf1_2g_h;
    unsigned char second_offset_power_wf1_5200;
    unsigned char second_offset_power_wf1_5300;
    unsigned char second_offset_power_wf1_5530;
    unsigned char second_offset_power_wf1_5660;
    unsigned char second_offset_power_wf1_5780;

    unsigned char txt_shift_value_wf0_2g_l;
    unsigned char txt_shift_value_wf0_2g_m;
    unsigned char txt_shift_value_wf0_2g_h;
    unsigned char txt_shift_value_wf0_5200;
    unsigned char txt_shift_value_wf0_5300;
    unsigned char txt_shift_value_wf0_5530;
    unsigned char txt_shift_value_wf0_5660;
    unsigned char txt_shift_value_wf0_5780;

    unsigned char txt_shift_value_wf1_2g_l;
    unsigned char txt_shift_value_wf1_2g_m;
    unsigned char txt_shift_value_wf1_2g_h;
    unsigned char txt_shift_value_wf1_5200;
    unsigned char txt_shift_value_wf1_5300;
    unsigned char txt_shift_value_wf1_5530;
    unsigned char txt_shift_value_wf1_5660;
    unsigned char txt_shift_value_wf1_5780;

    unsigned char pmu_efuse_en;

    unsigned char rg_pmu_bucka_efuse_trim_i_osc_vld;
    unsigned char rg_pmu_bucka_rsv3_vld;
    unsigned char rg_pmu_buckd_efuse_trim_i_osc_vld;
    unsigned char rg_pmu_buckd_efuse_trim_offset_vref_vld;
    unsigned char rg_aoldo_vosel_vld;
    unsigned char rg_strup_rsva_vld;
    unsigned char rg_ao_hifldo_vosel_vld;
    unsigned char rg_aldo_vo_adj_vld;
    unsigned char rg_bg_tc_adj_vld;
    unsigned char rg_pmu_32k_adj_vld;
    unsigned char rg_pmu_bucka_efuse_trim_iref_vlreg_vld;
    unsigned char rg_pmu_bucka_efuse_trim_iref_vhreg_vld;
    unsigned char rg_pmu_buckd_ton_min_trim_vld;
    unsigned char rg_pmu_buckd_cs_gain_ramp_vld;
    unsigned char rg_pmu_buckd_vsel_vh_vld;
    unsigned char rg_pmu_buckd_capslp_ramp_vld;

    unsigned char rg_pmu_bucka_efuse_trim_i_osc;
    unsigned char rg_pmu_bucka_rsv3;
    unsigned char rg_pmu_buckd_efuse_trim_i_osc;
    unsigned char rg_pmu_buckd_efuse_trim_offset_vref;
    unsigned char rg_aoldo_vosel;
    unsigned char rg_strup_rsva;
    unsigned char rg_ao_hifldo_vosel;
    unsigned char rg_aldo_vo_adj;
    unsigned char rg_bg_tc_adj;
    unsigned char rg_pmu_bucka_efuse_trim_slp_csp;
    unsigned char rg_pmu_bucka_efuse_trim_iref_vlreg;
    unsigned char rg_pmu_bucka_efuse_trim_iref_vhreg;
    unsigned char rg_pmu_buckd_ton_min_trim;
    unsigned char rg_pmu_buckd_cs_gain_ramp;
    unsigned char rg_pmu_buckd_vsel_vh;
    unsigned char rg_pmu_buckd_capslp_ramp;
    unsigned char rg_pmu_32k_adj;
}W2_EFUSE_PARAM;

typedef struct Cali_Param
{
    unsigned char Cmd;
    unsigned char version;
    unsigned short cali_config;
    unsigned char freq_offset;
    unsigned char htemp_freq_offset;
    unsigned char tssi_2g_offset;
    unsigned char tssi_5g_offset[TSSI_5G_CAL_NUM];
    unsigned char wf2g_spur_rmen;
    unsigned short spur_freq;
    unsigned char rf_num;
    unsigned char cw2mod[6];
    unsigned char wf2g_11b_tpwr[4]; //the number based on rates
    unsigned char wf2g_11g_tpwr[8];
    unsigned char wf2g_ht20_tpwr[8];
    unsigned char wf2g_ht40_tpwr[8];
    unsigned char wf2g_vht20_tpwr[9];
    unsigned char wf2g_vht40_tpwr[10];
    unsigned char wf2g_he20_tpwr[12];
    unsigned char wf2g_he40_tpwr[12];
    unsigned char wf5g_11a_tpwr_lm[8];
    unsigned char wf5g_ht20_tpwr_lm[8];
    unsigned char wf5g_ht40_tpwr_lm[8];
    unsigned char wf5g_vht20_tpwr_lm[9];
    unsigned char wf5g_vht40_tpwr_lm[10];
    unsigned char wf5g_vht80_tpwr_lm[10];
    unsigned char wf5g_he20_tpwr_lm[12];
    unsigned char wf5g_he40_tpwr_lm[12];
    unsigned char wf5g_he80_tpwr_lm[12];
    unsigned char wf5g_11a_tpwr_h[8];
    unsigned char wf5g_ht20_tpwr_h[8];
    unsigned char wf5g_ht40_tpwr_h[8];
    unsigned char wf5g_vht20_tpwr_h[9];
    unsigned char wf5g_vht40_tpwr_h[10];
    unsigned char wf5g_vht80_tpwr_h[10];
    unsigned char wf5g_he20_tpwr_h[12];
    unsigned char wf5g_he40_tpwr_h[12];
    unsigned char wf5g_he80_tpwr_h[12];
    unsigned char wf5g_lch_he_lowrssi_tpwr[6];
    unsigned char wf5g_mch_he_lowrssi_tpwr[6];
    unsigned char wf5g_hch_he20_lowrssi_tpwr[6];
    unsigned char wf5g_hch_he40_lowrssi_tpwr[6];
    unsigned char wf5g_hch_he80_lowrssi_tpwr[6];
    unsigned char wf5g_vht_lowrssi_tpwr[6];
    W2_EFUSE_PARAM w2_efuse_param;
} Cali_Param;


//================cmd=========================================

#define CMD_GET (0x80)
#define BCNInterval_Cmd 0x1
#define Bcn_Frm_Addr_Cmd  (CMD_GET|0x2)
#define Power_Save_Cmd 0x4
#define Send_NullData_Cmd 0x5
#define Keep_Alive_Cmd 0x6
#define Beacon_Miss_Cmd 0x7
#define CAM_Mode_Cmd 0x8
#define ARP_Agent_Cmd 0x9
#define Add_Pattern_Cmd 0xa
#define WoW_Enable_Cmd 0xb
#define AC_BE_Param_Cmd 0xc
#define AC_BK_Param_Cmd 0xd
#define AC_VI_Param_Cmd 0xe
#define AC_VO_Param_Cmd 0xf
#define CWmin_Cwmax_Cmd 0x10
#define LongRetryLimit_Cmd 0x11
#define ShortRetryLimit_Cmd 0x12
#define NDP_Anncmnt_Cmd 0x13
#define BmfmInfo_Cmd 0x15
#define UniCast_Key_Set_Cmd 0x16
#define Multicast_Key_Set_Cmd 0x17
#define Reset_Key_Cmd 0x18
#define Rekey_Data_Set_Cmd 0x19
#define MaxTxLifetime_Cmd 0x20
#define dhcp_start_cmd 0x21
#define vsdb_set_cmd 0x22
#define Beacon_Enable_Cmd 0x24
#define ADDBA_OK_CMD 0x2a
#define RegStaID_CMD 0x30
#define UnRegStaID_CMD 0x31
#define UnRegAllStaID_CMD 0x32
#define Del_BA_CMD 0x34
/*set_page_len_cmd*/
#define SET_PAGE_LEN_CMD 0x33
#define EXTERN_CHANNEL_STATUS_CMD (CMD_GET|0x35)
#define RD_SUPPORT_CMD 0x3a
#define MAC_SCAN_CMD 0x42
#define PHY_DEBUG (CMD_GET|0x44)
#define P2P_OPPPS_CWEND_IRQ_ENABLE_CMD 0x45
#define P2P_NoA_ENABLE_CMD 0x46
#define GET_TSF_CMD (CMD_GET|0x47)
#define GET_PN_CMD (CMD_GET|0x48)
#define GET_SDIO_PTR_CMD (CMD_GET|0x49)
#define Mac_Rtc_Cmd 0x4a

#define COEXIST_CMD  0x4B

#define CHANNEL_SWITCH_CMD  0x25
#define DPD_MEMORY_CMD  0x26
#define FWLOG_MODE_CMD  0x27
#define CALI_PARAM_CMD  0x28
#define PHY_INTERFACE_CMD  0x29

/*coexist cmd1 comand*/
#define COEXIST_EN_CMD  BIT(0)
#define COEXIST_MAX_MISS_BCN_CNT  BIT(1)
#define COEXIST_REQ_TIMEOUT  BIT(2)
#define COEXIST_NOT_GRANT_WEIGHT  BIT(3)
#define COEXIST_FW_RETRY_WEIGHT  BIT(4)
#define COEXIST_MAX_NOT_GRANT_CNT  BIT(5)
#define COEXIST_SCAN_PRIORITY_RANGE  BIT(6)
#define COEXIST_BE_BK_NOQOS_PRI_RANGE  BIT(7)
#define COEXIST_INFOR_BT_WIFI_WORK_FREQ  BIT(8)

//Reset_Key_Cmd
#define ALL_KEY_RST 0xffff
#define MULTI_KEY_RST 0xeeee

//Reset_Key_Cmd
#define ALL_UNIKEY_RST 0xeeee
#define ALL_MULTIKEY_RST 0xdddd

#pragma pack(1)

//Set Page_Len Cmd
typedef struct PagelenCmd
{
    unsigned char Cmd;
    unsigned char reserve[3];
    unsigned int page_len;
} PagelenCmd;


//
///BCNInterval_Cmd
//
typedef struct BCNIntervalCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int interval;
} BCNIntervalCmd;
//
///Bcn_Frm_Addr_Cmd
//
typedef struct BcnFrmAddrCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int address;
} BcnFrmAddrCmd;
//
///Beacon_Enable_Cmd
//
typedef struct BeaconEnableCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned char enable;
} BeaconEnableCmd;

typedef struct PowerSaveCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned char psmode;
} PowerSaveCmd;
typedef struct CAMModeCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned char cammode;
} CAMModeCmd;

typedef struct DHCPStartCmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int ip;
} DHCPStartCmd;

//Del_BA_CMD
typedef struct
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char STA_ID;
    unsigned char TID;
} Station;

//EXTERN_CHANNEL_STATUS_CMD
typedef struct Extern_Channel_Status
{
    unsigned char Cmd;
    unsigned char reserve[3];
    unsigned int Status;
}  Extern_Channel_Status;

typedef struct CommonCmdUC
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned char param[8];
} CommonCmdUC;

typedef struct CommonCmdUS
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned short param[4];
} CommonCmdUS;

typedef struct CommonCmdUL
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned long param[2];
} CommonCmdUL;

typedef struct CommonCmdULL
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int rsrv2;
    unsigned long long param0;
} CommonCmdULL;

typedef struct NoACmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int duration;
    unsigned int interval;
    unsigned int starttime;
    unsigned char count;
    unsigned char flag;
} NoACmd;

typedef struct Bcn_Frm_Addr
{
    unsigned int cmd;
    unsigned int address;
} Bcn_Frm_Addr;

typedef struct RegStaIdParam
{
    unsigned char Cmd;
    unsigned char vid;
    unsigned short StaId; //statin id
    unsigned char StaMac[6]; //station mac address
    unsigned char encrypt;
} RegStaIdParam;

typedef struct UnRegStaID
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid; //statin id
    unsigned int StaId; //statin id
} UnRegStaID;

///UnRegAllStaID_CMD
typedef struct UnRegAllStaID
{
    unsigned char Cmd;
    unsigned char reserve[3];
    unsigned char vid; //statin id
} UnRegAllStaID;


//AC_BE_Param_Cmd
typedef struct SetEDCACmd
{
    unsigned char Cmd;
    unsigned char reserve[2];
    unsigned char vid;
    unsigned int data;
} SetEDCACmd;

///PHY_DEBUG
typedef struct phydebugcmd
{
    unsigned char Cmd;
    unsigned char len;
    unsigned short serve;
    unsigned int   offset;
    unsigned char buf[48];
} phydebugcmd;

#define WIFINET_ADDR_LEN    6
/*send (qos)null data command */
typedef struct NullDataCmd
{
    unsigned char Cmd;
    unsigned char vid;
    /*tid id useless when qos is 0 */
    unsigned char qos:1,
                /*1 is go to power save state, 0 is awake or keep alive*/
                tid:4,
                pwr_save:1,
                eosp:1,
                /* pwr save packet flag */
                pwr_flag:1;
    unsigned char dest_addr[WIFINET_ADDR_LEN];
    unsigned char bw;
    unsigned char rate;
    unsigned short sn;
    unsigned short staid;
    /*Indicates the length of the PSDU in octets*/
    unsigned short tv_l_length;
} NullDataCmd;

typedef struct KeepAliveCmd
{
    unsigned char Cmd;
    unsigned char enable;
    /* in second, need transform to ms in firmware */
    unsigned int period;
    struct NullDataCmd null_data_param;
} KeepAliveCmd;

#define IPV6_LEN 16
#define IPV4_LEN 4
#define IPV6_ADDR_BUF_LEN  (IPV6_LEN * 3 + 1)

#define WOW_PATTERN_SIZE 52
#define WOW_MAX_PATTERNS 4
#define WOW_PATTERN_MASK_SIZE 8
/* timeout for supend wifi, in jiffies */
#define WOW_TIMEOUT (200)

/* arp agent command */
typedef struct ArpAgentCmd
{
    unsigned char Cmd;
    unsigned char enable;
    unsigned char vid;
    unsigned char reserve;
    unsigned int ip_addr;
    unsigned char ip6_addr[IPV6_ADDR_BUF_LEN];
} ArpAgentCmd;

/* beacon miss monitor */
typedef struct BeaconMissCmd
{
    unsigned char Cmd;
    unsigned char vid;
    /*1 enable , 0 disable */
    unsigned char enable;
    unsigned char reserved;
    unsigned int period;
}BeaconMissCmd;

typedef struct vsdb_cmd
{
    unsigned char Cmd;
    unsigned char vid;
    /*1 enable , 0 disable */
    unsigned char enable;
    unsigned char reserved[5];
} vsdb_cmd;

/* suspend command */
typedef struct SuspendCmd
{
    unsigned char Cmd;
    unsigned char enable;
    unsigned char vid;
    unsigned char wow;
    unsigned char deepsleep;
    unsigned char reserved[3];
    unsigned int filters;
    unsigned char PN[16];
} SuspendCmd;

/* add patterns command for suspend*/
typedef struct AddPatternCmd
{
    unsigned char Cmd;
    unsigned char vid;
    /*offset from cfg layer, indicates where pattern
     *start to match with packets
     */
    unsigned char pattern_offset;
    unsigned char pattern_len;
    unsigned char pattern_id;
    unsigned char mask[WOW_PATTERN_MASK_SIZE];
    unsigned char pattern[WOW_PATTERN_SIZE];
} AddPatternCmd;

typedef struct NDPAnncmntCmd
{
    unsigned char Cmd;
    unsigned char vid;

    unsigned char rate;
    unsigned char bw;

    unsigned char dest_addr[WIFINET_ADDR_LEN];
    unsigned char reserved[2];

    unsigned short sn;
    unsigned short stainfo_num;

    unsigned short stainfo[WIFI_MAX_STA];
} NDPAnncmntCmd;

///TxBA_TYPE
#define  BA_DELAY  0
#define  BA_IMMIDIATE  1

///AuthRole
#define BA_INITIATOR  0
#define BA_RESPONSER  1

typedef struct Add_BA_Struct
{
    unsigned char   Cmd;
    unsigned char   reserve[2];
    unsigned char   vid;
    unsigned char   TID;
    unsigned char   BA_TYPE;
    unsigned char   AuthRole;
    unsigned char   BA_Size;
    unsigned short   STA_ID;
    unsigned short   SeqNumStart;
} Add_BA_Struct;

typedef struct Delete_TxBA_Struct
{
    unsigned char   Cmd;
    unsigned char   reserve[2];
    unsigned char   vid;
    unsigned char   STA_ID;
    unsigned char   TID;
    unsigned char   AuthRole;
    unsigned char   reserve1;
} Delete_TxBA_Struct;

typedef struct MacRtcCmd
{
    unsigned char Cmd;
    unsigned char reserve[3];
    unsigned int enable;
} MacRtcCmd;

typedef struct Bmfm_Info_Cmd
{
    unsigned char Cmd;
    unsigned char vid;
    unsigned char reserved;

    unsigned char feedback_type;
    unsigned char group_id[8];
    unsigned char user_position[16];
}Bmfm_Info_Cmd;

struct Coexist_Cmd
{
    unsigned char Cmd;
    unsigned char reserve[3];
    unsigned int coexist_id_bitmap;
    unsigned int req_timeout_value;
    unsigned int scan_priority_range;
    unsigned int be_bk_no_qos_priority_range;
    unsigned char coexist_enable;
    unsigned char max_miss_bcn_cnt;
    unsigned char not_grant_weight;
    unsigned char fw_retry_weight;
    unsigned char max_not_grant_cnt;
    unsigned char enable_infor_bt_wifi_work_freq;
    unsigned char reserve1[38];
};


struct New_Cmd
{
    unsigned char cmd_buffer[APP_CMD_PERFIFO_LEN];
};

/* channel switch flag */
enum {
    CHANNEL_RESTORE_FLAG = BIT(0),
    CHANNEL_CONNECT_FLAG = BIT(1),
    CHANNEL_NO_EVENT_FLAG = BIT(2),
};

typedef struct Channel_Switch
{
    unsigned char Cmd;
    unsigned char vid;
    unsigned char flag;
    unsigned char bw;
    unsigned int channel;
    /* center freq value */
    unsigned int pri_chan;
}Channel_Switch;

typedef struct DPD_Memory_Download
{
    unsigned char Cmd;
    unsigned char vid;
    unsigned char reserved[2];
} DPD_Memory_Download;

typedef struct Fwlog_Mode_Control
{
    unsigned char Cmd;
    unsigned char mode; //0x00:fw  0x01:host
    unsigned char reserved[2];
} Fwlog_Mode_Control;

// event
typedef struct channel_switch_event
{
    struct fw_event_basic_info basic_info;
    unsigned int channel;
    unsigned int bw;
    unsigned int done;
} channel_switch_event;

typedef struct tx_error_event
{
    struct fw_event_basic_info basic_info;
    unsigned int error_type;
    unsigned int frame_type;
} tx_error_event;

typedef struct Phy_Interface_Param
{
    unsigned char Cmd;
    unsigned char vid;
    unsigned char interface_enable;
    unsigned char reserve;
} Phy_Interface_Param;

#pragma pack()

typedef union FI_CMDFIFO_PARAM
{
    struct RegStaIdParam StaIdParam;
    struct MultiCastKeyCmd MultiKeyCmd ;
    struct UniCastKeyCmd UniKeyCmd;
    struct RekeyDataCmd RekeyCmd;
    struct Extern_Channel_Status exchanstatus;
    struct UnRegStaID UnRegStaID;
    struct SetEDCACmd SetEDCACmd;
    struct PowerSaveCmd PowerSaveCmd;
    struct CAMModeCmd CAMModeCmd;
#ifdef DHCP_OFFLOAD
    struct DHCPStartCmd DHCPStartCmd;
#endif
#ifdef VSDB
    struct vsdb_cmd vsdb_enable_cmd;
#endif
    struct BCNIntervalCmd BCNIntervalCmd;
    struct BcnFrmAddrCmd BcnFrmAddrCmd;
    struct BeaconEnableCmd BeaconEnableCmd;
    struct UnRegAllStaID UnRegAllStaID;
    struct phydebugcmd Phydbgcmd;
    struct CommonCmdUC CommonCmdUC;
    struct CommonCmdUS CommonCmdUS;
    struct CommonCmdUL CommonCmdUL;
    struct CommonCmdULL CommonCmdULL;
    struct NoACmd NoACmd;
    struct MacRtcCmd  MacRtcCmd;
    struct NullDataCmd null_data_cmd;
    struct NDPAnncmntCmd ndp_anncmnt_data;
    struct KeepAliveCmd keep_alive_cmd;
    struct BeaconMissCmd beacon_miss_cmd;
    struct ArpAgentCmd arp_agent_cmd;
    struct SuspendCmd suspend_cmd;
    struct AddPatternCmd add_pattern_cmd;
    struct Bmfm_Info_Cmd bmfm_info_cmd;
    struct Coexist_Cmd coexist_cmd;
    struct New_Cmd new_cmd;

    struct Channel_Switch channel_switch_cmd;
    struct DPD_Memory_Download dpd_download_cmd;
    struct Fwlog_Mode_Control fwlog_mode_cmd;
    struct Cali_Param cali_param_cmd;
    struct Phy_Interface_Param phy_interface_cmd;
    struct PagelenCmd page_len_cmd;
} FI_CMDFIFO_PARAM;

#endif
