#ifndef _FI_W2_SDIO_H
#define _FI_W2_SDIO_H

#define FW_VERSION_W1   0x1
#define FW_VERSION_W1U  0x2

#define PHY_REG_AGC_BASE     0x00a08000
#define PHY_AGC_BUSY_FSM          (PHY_REG_AGC_BASE+0x34)
#define DF_AGC_REG_A12 (PHY_REG_AGC_BASE + 0x30)
#define DF_AGC_REG_A27 (PHY_REG_AGC_BASE + 0x6c)
#define REG_STF_AC_Q_THR (PHY_REG_AGC_BASE + 0x5c)
#define REG_ED_THR_DB (PHY_REG_AGC_BASE + 0x74)

#define HOST_TO_FIRMWARE_FLAGS (MAC_SRAM_BASE+0x00002c)
#define PAGE_LEN_USB 1680
#define PHY_PRIMARY_CHANNEL  (0x00a0b000 + 0x22c)
#define CH_BW_LINK_SRAM_ADDR  (MAC_SRAM_BASE + 0xf4)

#define FW_IRQ_VERSION          FW_VERSION_W1U

#define W1_FW_TXDESC_X_LEN  ((size_t) sizeof(struct Fw_TxPriv)+sizeof(struct HW_TxOption)+sizeof(struct hw_tx_vector_bits))

#define RG_SCFG_SRAM_FUNC (0x8040)
#define RG_SCFG_REG_FUNC (0x8044)
#define RG_SCFG_EEPROM_FUNC (0x8048)
#define RG_SCFG_SRAM_FUNC6 (0x8160)

#define USB_FW_SLEEP BIT(0)


#endif
