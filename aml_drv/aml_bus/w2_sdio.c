/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 202X Original Author (retain original author information)
* Copyright (C) 202X Amlogic, Inc. All rights reserved.
*
* Description:
*/
#define AML_MODULE          SDIO
#define AML_FMT             AML_FMT_M

#include <linux/mutex.h>

#include "aml_interface.h"
#include "sdio_common.h"
#include "w2_sdio.h"
#include "chip_ana_reg.h"
#include "chip_pmu_reg.h"
#include "chip_intf_reg.h"
#include "wifi_intf_addr.h"
#include "wifi_top_addr.h"
#include "fi_w2_sdio.h"
#include "wifi_w2_shared_mem_cfg.h"
#include "aml_static_buf.h"
#include "aml_log.h"

struct aml_hif_sdio_ops g_hif_sdio_ops;
extern struct aml_bus_state_detect bus_state_detect;

extern unsigned char *g_func_kmalloc_buf;
static unsigned int tx_buffer_base_addr;
static unsigned int rx_buffer_base_addr;
extern unsigned char g_sdio_after_porbe;

extern struct aml_pm_type g_wifi_pm;
static DEFINE_MUTEX(wifi_bt_sdio_mutex);
static DEFINE_MUTEX(wifi_ipc_mutex);

extern unsigned char (*host_wake_req)(void);
extern int (*host_suspend_req)(struct device *device);

static int _aml_sdio_request_byte(unsigned char func_num,
    unsigned char write, unsigned int reg_addr, unsigned char *byte)
{
    int err_ret = 0;
    struct sdio_func * func = aml_priv_to_func(func_num);
    unsigned char *kmalloc_buf = NULL;
    unsigned char len = sizeof(unsigned char);

#if defined(DBG_PRINT_COST_TIME)
    struct timespec now, before;
    getnstimeofday(&before);
#endif /* End of DBG_PRINT_COST_TIME */

    if (!func) {
        AML_ERR("func is NULL!\n");
        return -1;
    }

    if (!byte) {
        AML_ERR("byte is NULL!\n");
        return -1;
    }

    BUG_ON(func->num != func_num);

    AML_BT_WIFI_MUTEX_ON();
    kmalloc_buf =  (unsigned char *)ZMALLOC(len, "sdio_write", GFP_DMA);
    if (kmalloc_buf == NULL)
    {
        ERROR_DEBUG_OUT("kmalloc buf fail\n");
        AML_BT_WIFI_MUTEX_OFF();
        return SDIOH_API_RC_FAIL;
    }
    memcpy(kmalloc_buf, byte, len);

    AML_PROF_HI(cmd52);
    /* Claim host controller */
    sdio_claim_host(func);

    if (write) {
        /* CMD52 Write */
        sdio_writeb(func, *kmalloc_buf, reg_addr, &err_ret);
    }
    else {
        /* CMD52 Read */
        *byte = sdio_readb(func, reg_addr, &err_ret);
    }

    /* Release host controller */
    sdio_release_host(func);
    AML_PROF_LO(cmd52);

#if defined(DBG_PRINT_COST_TIME)
    getnstimeofday(&now);

    AML_INFO("[sdio byte]: len=1 cost=%lds %luus\n",
        now.tv_sec-before.tv_sec, now.tv_nsec/1000 - before.tv_nsec/1000);
#endif /* End of DBG_PRINT_COST_TIME */

    FREE(kmalloc_buf, "sdio_write");
    AML_BT_WIFI_MUTEX_OFF();
    return (err_ret == 0) ? SDIOH_API_RC_SUCCESS : SDIOH_API_RC_FAIL;
}

//cmd52, func 0, for self define domain
int aml_sdio_self_define_domain_func0_write8(int addr, unsigned char data)
{
    int ret = 0;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(SDIO_FUNC0, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    ret =  _aml_sdio_request_byte(SDIO_FUNC0, SDIO_WRITE, addr, &data);
    return ret;
}

//cmd52
unsigned char aml_sdio_self_define_domain_func0_read8(int addr)
{
    unsigned char sramdata = 0;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(SDIO_FUNC0, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    _aml_sdio_request_byte(SDIO_FUNC0, SDIO_READ, addr, &sramdata);
    return sramdata;
}

//cmd52, func 1, for self define domain
int aml_sdio_self_define_domain_write8(int addr, unsigned char data)
{
    int ret = 0;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(SDIO_FUNC1, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    ret =  _aml_sdio_request_byte(SDIO_FUNC1, SDIO_WRITE, addr, &data);
    return ret;
}

//cmd52
unsigned char aml_sdio_self_define_domain_read8(int addr)
{
    unsigned char sramdata = 0;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(SDIO_FUNC1, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    _aml_sdio_request_byte(SDIO_FUNC1, SDIO_READ, addr, &sramdata);
    return sramdata;
}

extern int _aml_sdio_request_buffer(unsigned char func_num, unsigned int fix_incr, unsigned char write, unsigned int addr, void * buf, unsigned int nbytes);
//cmd53
int aml_sdio_bottom_write(unsigned char func_num, unsigned int addr, void *buf, size_t len, int incr_addr)
{
    void *kmalloc_buf;
    int result;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(func_num, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    BUG_ON(func_num == SDIO_FUNC0);
    BUG_ON(!g_func_kmalloc_buf);

    if (host_wake_req != NULL) {
        if (host_wake_req() == 0) {
            ERROR_DEBUG_OUT("aml_sdio_bottom_write, host wake fail\n");
            return -1;
        }
    }

    AML_BT_WIFI_MUTEX_ON();
    kmalloc_buf = (unsigned char *)g_func_kmalloc_buf;
    memcpy(kmalloc_buf, buf, len);

    result = _aml_sdio_request_buffer(func_num, incr_addr, SDIO_WRITE, addr, kmalloc_buf, len);

    AML_BT_WIFI_MUTEX_OFF();
    if (result && !bus_state_detect.bus_err && (atomic_read(&g_wifi_pm.is_shut_down) == 0)) {
        if (bus_state_detect.is_drv_load_finished) {
            bus_state_detect.bus_err = 1;
            ERROR_DEBUG_OUT("sdio bus error(%d), will do reovery later\n", result);
        } else {
            bus_state_detect.bus_err = 2;
            ERROR_DEBUG_OUT("sdio bus error(%d), sdio reset by timer later\n", result);
         }
    }
    return result;
}

int aml_sdio_bottom_read(unsigned char func_num, int addr, void *buf, size_t len, int incr_addr)
{
    unsigned char *kmalloc_buf = NULL;
    int result;
    int align_len = 0;
    bool sdio_bus_block = false;

    sdio_bus_block = aml_sdio_block_bus_opt(func_num, addr);
    if (sdio_bus_block)
    {
       return 0;
    }

    func_num &= 0xf;
    BUG_ON(func_num == SDIO_FUNC0);
    BUG_ON(!g_func_kmalloc_buf);

    if (host_wake_req != NULL) {
        if (host_wake_req() == 0) {
            ERROR_DEBUG_OUT("aml_sdio_bottom_read, host wake fail\n");
            return -1;
        }
    }

    AML_BT_WIFI_MUTEX_ON();
    /* read block mode */
    if (func_num != SDIO_FUNC0) {
        if (incr_addr == SDIO_OPMODE_INCREMENT) {
            struct sdio_func * func = aml_priv_to_func(func_num);
            align_len = sdio_align_size(func, len);
            kmalloc_buf = (unsigned char *)g_func_kmalloc_buf;
        }
        else
            kmalloc_buf = (unsigned char *)g_func_kmalloc_buf;
    } else {
        kmalloc_buf = (unsigned char *)buf;
    }

    if (kmalloc_buf == NULL) {
        ERROR_DEBUG_OUT("kmalloc buf fail kmalloc_buf %p buf %p SDIO_FUNC %d\n", (void *)kmalloc_buf, buf, func_num);
        AML_BT_WIFI_MUTEX_OFF();
        return SDIOH_API_RC_FAIL;
    }

    result = _aml_sdio_request_buffer(func_num, incr_addr, SDIO_READ, addr, kmalloc_buf, len);

    if (kmalloc_buf != buf) {
        memcpy(buf, kmalloc_buf, len);
    }

    AML_BT_WIFI_MUTEX_OFF();
    if (result && !bus_state_detect.bus_err && (atomic_read(&g_wifi_pm.is_shut_down) == 0)) {
        if (bus_state_detect.is_drv_load_finished) {
            bus_state_detect.bus_err = 1;
            ERROR_DEBUG_OUT("sdio bus error(%d), will do reovery later\n", result);
        } else {
            bus_state_detect.bus_err = 2;
            ERROR_DEBUG_OUT("sdio bus error(%d), sdio reset by timer later\n", result);
         }
    }

    return result;
}

//func 1, cmd52, self define domain
int aml_sdio_self_define_domain_write32(unsigned long sram_addr, unsigned long sramdata)
{
    return aml_sdio_bottom_write(SDIO_FUNC1, sram_addr&SDIO_ADDR_MASK,
        (unsigned char *)&sramdata,  sizeof(unsigned long), SDIO_OPMODE_INCREMENT);
}

unsigned long  aml_sdio_self_define_domain_read32(unsigned long sram_addr)
{
    unsigned long sramdata = 0;

    aml_sdio_bottom_read(SDIO_FUNC1, sram_addr&SDIO_ADDR_MASK, &sramdata, 4, SDIO_OPMODE_INCREMENT);
    return sramdata;
}

//func2, for random ram
void aml_sdio_random_word_write(unsigned int addr, unsigned int data)
{
    unsigned int len = sizeof(unsigned int);

    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC2_BADDR_A, (unsigned long)(addr) & 0xfffe0000);
    aml_sdio_bottom_write(SDIO_FUNC2, (SYS_TYPE)addr & SDIO_ADDR_MASK,
        (unsigned char *)&data, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();
}

unsigned int aml_sdio_random_word_read(unsigned int addr)
{
    unsigned int regdata = 0;
    unsigned int len = sizeof(unsigned int);

    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC2_BADDR_A, (unsigned long)(addr) & 0xfffe0000);
    aml_sdio_bottom_read(SDIO_FUNC2, (SYS_TYPE)addr & SDIO_ADDR_MASK,
        (unsigned char*)&regdata, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();

    return regdata;
}

void aml_sdio_random_ram_write(unsigned char *buf, unsigned char *addr, size_t len)
{
    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC2_BADDR_A, (unsigned long)(addr) & 0xfffe0000);
    aml_sdio_bottom_write(SDIO_FUNC2, ((SYS_TYPE)addr & SDIO_ADDR_MASK),
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();
}

void aml_sdio_random_ram_read(unsigned char* buf, unsigned char* addr, size_t len)
{
    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC2_BADDR_A, (unsigned long)(addr) & 0xfffe0000);
    aml_sdio_bottom_read(SDIO_FUNC2, (SYS_TYPE)addr & SDIO_ADDR_MASK,
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();
}

//func3 for sram
void aml_sdio_sram_word_write(unsigned int addr, unsigned int data)
{
    unsigned int len = sizeof(unsigned int);
    aml_sdio_bottom_write(SDIO_FUNC3, (SYS_TYPE)addr % MAC_SRAM_BASE,
        (unsigned char *)&data, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

unsigned int aml_sdio_sram_word_read(unsigned int addr)
{
    unsigned int regdata = 0;
    unsigned int len = sizeof(unsigned int);

    aml_sdio_bottom_read(SDIO_FUNC3, (SYS_TYPE)addr % MAC_SRAM_BASE,
        (unsigned char*)&regdata, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    return regdata;
}

void aml_sdio_sram_write(unsigned char *buf, unsigned char *addr, size_t len)
{
    aml_sdio_bottom_write(SDIO_FUNC3, (SYS_TYPE)addr % MAC_SRAM_BASE,
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

void aml_sdio_sram_read(unsigned char* buf, unsigned char* addr, size_t len)
{
    aml_sdio_bottom_read(SDIO_FUNC3, (SYS_TYPE)addr % MAC_SRAM_BASE,
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

//sdio func4 for tx buffer write/read
void aml_sdio_func4_set_base_addr(unsigned int addr, size_t len)
{
    if ((addr % tx_buffer_base_addr) >= SRAM_MAX_LEN || addr < tx_buffer_base_addr
        || ((addr % tx_buffer_base_addr) < SRAM_MAX_LEN && ((addr + len) % tx_buffer_base_addr) >= SRAM_MAX_LEN)) {
        tx_buffer_base_addr = addr;
        aml_sdio_self_define_domain_write32(RG_SCFG_FUNC4_BADDR_A, tx_buffer_base_addr);
    }
}

void aml_sdio_tx_buffer_write(unsigned char *buf, unsigned char *addr, size_t len)
{
    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_func4_set_base_addr((unsigned long)addr, len);
    aml_sdio_bottom_write(SDIO_FUNC4, ((SYS_TYPE)addr % tx_buffer_base_addr),
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();
}

void aml_sdio_tx_buffer_read(unsigned char* buf, unsigned char* addr, size_t len)
{
    AML_WIFI_IPC_MUTEX_ON();

    aml_sdio_func4_set_base_addr((unsigned long)addr, len);
    aml_sdio_bottom_read(SDIO_FUNC4, ((SYS_TYPE)addr % tx_buffer_base_addr),
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));

    AML_WIFI_IPC_MUTEX_OFF();
}

//sdio func5 for rx desc
int aml_sdio_desc_read(void* buf, u32 addr, size_t len)
{
    return aml_sdio_bottom_read(SDIO_FUNC5, addr - RG_WIFI_IF_FW2HST_IRQ_CFG, buf, len,
            (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

//sdio func6 for rx buffer
static int aml_sdio_rx_buffer_read(void *buf, u32 addr, unsigned int len, unsigned int unused)
{
    int received = 0;
    int ret = -1;

    /* NB: the caller should check rx buffer boundary more strictly (with "real" rx buffer end) */
    BUG_ON(addr < RXBUF_START_ADDR);
    BUG_ON((addr + len) >= 0x60080000);
    while (len) {
        unsigned int read_len = len > SDIO_READ_MAX ? SDIO_READ_MAX : len;
        u32 addr_lo = (addr - RXBUF_START_ADDR) & SDIO_ADDR_MASK;
        u32 base = addr - addr_lo;

        if (base != rx_buffer_base_addr) {
            rx_buffer_base_addr = base;
            aml_sdio_self_define_domain_write32(RG_SCFG_FUNC6_BADDR_A, base);
        }
        ret = aml_sdio_bottom_read(SDIO_FUNC6, addr_lo, buf, read_len, SDIO_OPMODE_INCREMENT);
        if (ret)
            break;

        len -= read_len;
        buf += read_len;
        addr += read_len;
        received += read_len;
    }
    return received ? : ret;
}

//sdio func7 for bt
void aml_bt_sdio_read_sram(unsigned char *buf, unsigned char *addr, SYS_TYPE len)
{
    if (bus_state_detect.bus_err) {
        AML_ERR(" sdio bus is recovery ongoing, can not read/write\n");
        return;
    }
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        ERROR_DEBUG_OUT("bus suspend (%d) ongoing, do not read/write now!\n",
            atomic_read(&g_wifi_pm.bus_suspend_cnt));
        return;
    }
#endif

    aml_sdio_bottom_read(SDIO_FUNC7, ((SYS_TYPE)addr & SDIO_ADDR_MASK),
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

void aml_bt_sdio_write_sram(unsigned char *buf, unsigned char *addr, SYS_TYPE len)
{

    if (bus_state_detect.bus_err) {
        AML_ERR(" sdio bus is recovery ongoing, can not read/write\n");
        return;
    }
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        ERROR_DEBUG_OUT("bus suspend (%d) ongoing, do not read/write now!\n",
            atomic_read(&g_wifi_pm.bus_suspend_cnt));
        return;
    }
#endif

    aml_sdio_bottom_write(SDIO_FUNC7, ((SYS_TYPE)addr & SDIO_ADDR_MASK),
        buf, len, (len > 8 ? SDIO_OPMODE_INCREMENT : SDIO_OPMODE_FIXED));
}

unsigned int aml_bt_hi_read_word(unsigned int addr)
{
    unsigned int regdata = 0;
    unsigned int reg_tmp;
    /*
     * make sure function 5 section address-mapping feature is disabled,
     * when this feature is disabled,
     * all 128k space in one sdio-function use only
     * one address-mapping: 32-bit AHB Address = BaseAddr + cmdRegAddr
     */

    if (bus_state_detect.bus_err) {
        AML_ERR(" sdio bus is recovery ongoing, can not read/write\n");
        return regdata;
    }
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        ERROR_DEBUG_OUT("bus suspend (%d) ongoing, do not read/write now!\n",
            atomic_read(&g_wifi_pm.bus_suspend_cnt));
        return regdata;
    }
#endif

    reg_tmp = aml_sdio_self_define_domain_read32( RG_SDIO_IF_MISC_CTRL);

    if (!(reg_tmp & BIT(25))) {
        reg_tmp |= BIT(25);
        aml_sdio_self_define_domain_write32( RG_SDIO_IF_MISC_CTRL, reg_tmp);
    }

    /*config msb 15 bit address in BaseAddr Register*/
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC7_BADDR_A,addr & 0xfffe0000);
    /* coverity[overrun-buffer-val] - length is correct */
    aml_bt_sdio_read_sram((unsigned char*)(SYS_TYPE)&regdata,
        /*sdio cmd 52/53 can only take 17 bit address*/
        (unsigned char*)(SYS_TYPE)(addr & 0x1ffff), sizeof(unsigned int));

    return regdata;
}

void aml_bt_hi_write_word(unsigned int addr,unsigned int data)
{
    unsigned int reg_tmp;

    if (bus_state_detect.bus_err) {
        AML_ERR(" sdio bus is recovery ongoing, can not read/write\n");
        return;
    }
#ifdef CONFIG_PM
    if (atomic_read(&g_wifi_pm.bus_suspend_cnt)) {
        ERROR_DEBUG_OUT("bus suspend (%d) ongoing, do not read/write now!\n",
            atomic_read(&g_wifi_pm.bus_suspend_cnt));
        return;
    }
#endif
    /*
     * make sure function 5 section address-mapping feature is disabled,
     * when this feature is disabled,
     * all 128k space in one sdio-function use only
     * one address-mapping: 32-bit AHB Address = BaseAddr + cmdRegAddr
     */
    reg_tmp = aml_sdio_self_define_domain_read32( RG_SDIO_IF_MISC_CTRL);

    if (!(reg_tmp & BIT(25))) {
        reg_tmp |= BIT(25);
        aml_sdio_self_define_domain_write32( RG_SDIO_IF_MISC_CTRL, reg_tmp);
    }
    /*config msb 15 bit address in BaseAddr Register*/
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC7_BADDR_A, addr & 0xfffe0000);
    /* coverity[overrun-buffer-val] - length is correct */
    aml_bt_sdio_write_sram((unsigned char *)&data,
        /*sdio cmd 52/53 can only take 17 bit address*/
        (unsigned char*)(SYS_TYPE)(addr & 0x1ffff), sizeof(unsigned int));
}

static inline struct sdio_func *aml_hif_sdio_func(struct aml_hwif_sdio *h_sdio, int func_num)
{
    return h_sdio->sdio_func_if[func_num];
}

static int aml_sdio_cmd53_sg_blk_size(struct aml_hwif_sdio *h_sdio, int *max_req_size)
{
    struct sdio_func *func = aml_hif_sdio_func(h_sdio, SDIO_FUNC4);
    int blk_size = func->cur_blksize;

    if (max_req_size) {
        struct mmc_host *host = func->card->host;
        int max_blk = min(host->max_blk_count, (unsigned int)SDIO_MAX_BLK_CNT);

        *max_req_size = min(max_blk * blk_size, (int)host->max_req_size);
    }
    return blk_size;
}

static int aml_sdio_sg_blocks(struct aml_hwif_sdio *h_sdio,
                              struct scatterlist *sglist, unsigned int sg_len)
{
    int max_req_size;
    int blk_size = aml_sdio_cmd53_sg_blk_size(h_sdio, &max_req_size);
    struct scatterlist *sg;
    int i;
    int len = 0;

    if (blk_size < 0)
        return -EINVAL;

    for_each_sg(sglist, sg, sg_len, i) {
        /* SG limitations for most SDIO host controller */
        if (sg->length & 3) {
            AML_WARN("sg length %u should be 4-byte alignment\n", sg->length);
            return -EINVAL;
        }
        if (sg_len > 1 && (sg->length & (blk_size -1))) {
            AML_WARN("sg length %u should be multiple of block size %d\n", sg->length, blk_size);
            return -EINVAL;
        }
        /* FIXME: dma address of each SG should be 64-bit alignment */
        len += sg->length;
        if (len > max_req_size) {
            AML_WARN("total length %u > %d\n", len, max_req_size);
            return -EINVAL;
        }
    }
    return len / blk_size;
}

static int aml_sdio_cmd53_sg(struct aml_hwif_sdio *h_sdio, int write,
                             uint32_t addr, struct scatterlist *sglist, int sg_len)
{
    struct sdio_func *func = aml_hif_sdio_func(h_sdio, SDIO_FUNC4);
    int blocks = aml_sdio_sg_blocks(h_sdio, sglist, sg_len);
    struct mmc_data mmc_dat = {
        .flags = MMC_DATA_WRITE,
        .sg = sglist,
        .sg_len = sg_len,
        .blocks = blocks,
        .blksz = func->cur_blksize,
    };
    struct mmc_command mmc_cmd = {
        .opcode = SD_IO_RW_EXTENDED,
        .arg = (write ? BIT(31) : 0)
             | (SDIO_FUNC4 << 28)
             | BIT(27)
             | (addr << 9)
             | (blocks & 0x1ff),
        /* 0, fix address */
        .flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC,
    };
    struct mmc_request mmc_req = {
        .cmd = &mmc_cmd,
        .data = &mmc_dat,
    };

    int result;

    if (addr & ~SDIO_ADDR_MASK) {
        AML_ERR("SDIO address %x is out of range!\n", addr);
        return -EINVAL;
    }

    if (blocks < 0)
        return -EINVAL;

    if (aml_sdio_block_bus_opt(SDIO_FUNC4, addr))
       return -EIO;

    if (host_wake_req && host_wake_req() == 0) {
        AML_NOTICE("host wake fail while %s\n", write ? "writing" : "reading");
        return -EIO;
    }

#ifdef CONFIG_AML_RECOVERY
    if (bus_state_detect.bus_err)
        return -EIO;
#endif

    AML_PROF_CNT(cmd53_tx, mmc_dat.blksz * blocks);
    sdio_claim_host(func);
    mmc_set_data_timeout(&mmc_dat, func->card);
    mmc_wait_for_req(func->card->host, &mmc_req);
    sdio_release_host(func);
    AML_PROF_CNT(cmd53_tx, 0);

    AML_DBG("sg_len: %d (%d * %d = %d)\n", sg_len, mmc_dat.blksz, blocks, mmc_dat.blksz * blocks);

    result = mmc_cmd.error ? : mmc_dat.error;
    if (result) {
        AML_ERR("CMD53 error = %d data_error=%d\n", mmc_cmd.error, mmc_dat.error);
#ifdef CONFIG_AML_RECOVERY
        if (!bus_state_detect.bus_err)
           bus_state_detect.bus_err = 1;
#endif
    }

    return result ? : mmc_dat.blksz * blocks;
}

static int aml_sdio_send_frame(struct scatterlist *scat_list, int n_sg)
{
    struct aml_hwif_sdio *h_sdio = &g_hwif_sdio;

    return aml_sdio_cmd53_sg(h_sdio, 1, 0 /* fake address */, scat_list, n_sg);
}

extern int aml_sdio_suspend(unsigned int suspend_enable);

void aml_sdio_init_w2_ops(void)
{
    struct aml_hif_sdio_ops* ops = &g_hif_sdio_ops;
    //func0 operation func, read/write self define domain reg, no need to set base addr
    ops->hi_self_define_domain_func0_write8 = aml_sdio_self_define_domain_func0_write8;
    ops->hi_self_define_domain_func0_read8 = aml_sdio_self_define_domain_func0_read8;

    //func1 operation func, read/write self define domain reg, no need to set base addr
    ops->hi_self_define_domain_write8 = aml_sdio_self_define_domain_write8;
    ops->hi_self_define_domain_read8 = aml_sdio_self_define_domain_read8;
    ops->hi_self_define_domain_write32 = aml_sdio_self_define_domain_write32;
    ops->hi_self_define_domain_read32 = aml_sdio_self_define_domain_read32;

    //func2 operation func, need to set base addr firstly
    ops->hi_random_word_write = aml_sdio_random_word_write;
    ops->hi_random_word_read = aml_sdio_random_word_read;
    ops->hi_random_ram_write = aml_sdio_random_ram_write;
    ops->hi_random_ram_read = aml_sdio_random_ram_read;

    //func3 sram operation func
    ops->hi_sram_word_write = aml_sdio_sram_word_write;
    ops->hi_sram_word_read = aml_sdio_sram_word_read;
    ops->hi_sram_write = aml_sdio_sram_write;
    ops->hi_sram_read = aml_sdio_sram_read;

    //func4 tx buffer
    ops->hi_tx_buffer_write = aml_sdio_tx_buffer_write;
    ops->hi_tx_buffer_read = aml_sdio_tx_buffer_read;

    //func5 rx desc
    ops->hi_desc_read = aml_sdio_desc_read;

    //func6, rx buffer read func
    ops->hi_rx_buffer_read = aml_sdio_rx_buffer_read;

    //for scatter list
    ops->hi_send_frame = aml_sdio_send_frame;

    //sdio func7 for bt
    ops->bt_hi_write_sram = aml_bt_sdio_write_sram;
    ops->bt_hi_read_sram = aml_bt_sdio_read_sram;
    ops->bt_hi_write_word = aml_bt_hi_write_word;
    ops->bt_hi_read_word = aml_bt_hi_read_word;

    //for suspend & resume
    ops->hif_suspend = aml_sdio_suspend;
    g_sdio_after_porbe = 1;

    // check and wake firstly.
    host_wake_req = NULL;
    host_suspend_req = NULL;
}

void aml_sdio_init_base_addr(void)
{
    g_func_kmalloc_buf = (unsigned char *)aml_mem_prealloc(PREALLOC_BUF_BUS, PREALLOC_BUF_BUS_SIZE);
    if (!g_func_kmalloc_buf) {
         AML_ERR(">>>sdio kmalloc failed!");
    }

    //func3, config sram base addr
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC3_BADDR_A, MAC_SRAM_BASE);

    //func4, config tx buffer base addr
    tx_buffer_base_addr = (TXBUF_START_ADDR);
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC4_BADDR_A, tx_buffer_base_addr);

    //func5, rxdesc base addr
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC5_BADDR_A, RG_WIFI_IF_FW2HST_IRQ_CFG);

    //func6, rx buffer base addr
    rx_buffer_base_addr = (RXBUF_START_ADDR);
    aml_sdio_self_define_domain_write32(RG_SCFG_FUNC6_BADDR_A, rx_buffer_base_addr);
}



/*set_wifi_bt_sdio_driver_bit() is used to determine whether to unregister sdio power driver.
  *Only when g_sdio_wifi_bt_alive is 0, then call aml_sdio_exit().
*/


EXPORT_SYMBOL(g_hif_sdio_ops);

static void aml_sdio_calibration(void)
{
    int err;
    unsigned char i, j, k, l;
    unsigned char step;

    step = 4;
    aml_sdio_self_define_domain_write8(0x2c0, 0);
    for (i = 0; i < 32; i += step) {
        aml_sdio_self_define_domain_write8(0x2c2, i);

        for (j = 0; j < 32; j += step) {
            aml_sdio_self_define_domain_write8(0x2c3, j);

            for (k = 0; k < 32; k += step) {
                aml_sdio_self_define_domain_write8(0x2c4, k);

                for (l = 0; l < 32; l += step) {
                    aml_sdio_self_define_domain_write8(0x2c5, l);

                    //msleep(3000);
                    err = aml_sdio_self_define_domain_write32(RG_SCFG_FUNC2_BADDR_A, l);

                    if (err) {
                        //msleep(3000);
                        aml_sdio_self_define_domain_write8(SDIO_CCCR_ABORT, 0x1);
                        AML_ERR("error: i:%d, j:%d, k:%d, l:%d\n", i, j, k, l);

                    } else {
                        AML_INFO(" right, use this config: i:%d, j:%d, k:%d, l:%d\n", i, j, k, l);
                        return;
                    }
                }
            }
        }
    }
    AML_ERR("error: i:%d; j:%d, k:%d, l:%d\n", i, j, k, l);

    aml_sdio_self_define_domain_write8(0x2c2, 0);
    aml_sdio_self_define_domain_write8(0x2c3, 0);
    aml_sdio_self_define_domain_write8(0x2c4, 0);
    aml_sdio_self_define_domain_write8(0x2c5, 0);
}

void wifi_cpu_clk_switch(unsigned int clk_cfg)
{
    struct aml_hif_sdio_ops *hif_ops = &g_hif_sdio_ops;
    hif_ops->hi_random_word_write(RG_INTF_CPU_CLK, clk_cfg);

    AML_INFO("cpu_clk_reg=0x%08x\n",
    hif_ops->hi_random_word_read(RG_INTF_CPU_CLK));
}

#ifdef ICCM_CHECK
unsigned char buf_iccm_rd[ICCM_BUFFER_RD_LEN];
#endif

unsigned char aml_download_wifi_fw_img(char *firmware_filename)
{
    unsigned int offset_base = 0;
    size_t databyte = 0;
    int i = 0, err = 0;
    unsigned int offset = 0;
    //unsigned int rom_len = 0;
    unsigned int tmp_val = 0;
    unsigned int len = 0;
    char tmp_buf[9] = {0};
    unsigned char *src = NULL;
    unsigned char *kmalloc_buf = NULL;
    const struct firmware *fw = NULL;
    struct aml_hif_sdio_ops *hif_ops = &g_hif_sdio_ops;
    unsigned int to_sdio = ~(0);
    RG_DPLL_A5_FIELD_T rg_dpll_a5;
    struct sdio_func *func = aml_priv_to_func(SDIO_FUNC7);

    AML_FN_ENTRY();
    err =request_firmware(&fw, firmware_filename, &func->dev);
    if (err) {
        ERROR_DEBUG_OUT("request firmware fail!\n");
        return err;
    }

#ifdef ICCM_ROM
    offset = ICCM_ROM_LEN;
    len = ICCM_RAM_LEN;
#endif

    src = (unsigned char *)fw->data + (offset / 4) * BYTE_IN_LINE;
    kmalloc_buf = (unsigned char *)aml_mem_prealloc(PREALLOC_BUF_FW_DL, len);
    if (!kmalloc_buf) {
        ERROR_DEBUG_OUT("kmalloc buf fail\n");
        release_firmware(fw);
        return -ENOMEM;
    }

    for (i = 0; i < len /4; i++) {
        tmp_buf[8] = 0;
        strncpy(tmp_buf, (char *)src, 8);
        if ((err = kstrtouint(tmp_buf, 16, &tmp_val))) {
            release_firmware(fw);
            //FREE(kmalloc_buf, "sdio_write");
            return err;
        }
        *(unsigned int *)&kmalloc_buf[4 * i] = __swab32(tmp_val);
        src += BYTE_IN_LINE;
    }

    // close phy rest
    hif_ops->hi_random_word_write(RG_WIFI_RST_CTRL, to_sdio);

#ifdef EFUSE_ENABLE
    efuse_init();
    AML_INFO(" called efuse init\n");
#endif

    rg_dpll_a5.data = hif_ops->bt_hi_read_word(RG_DPLL_A5);
    AML_INFO(" img len 0x%x, start download fw\n", len);

    do {
        databyte = (len > SRAM_MAX_LEN) ? SRAM_MAX_LEN : len;

        hif_ops->hi_random_ram_write(kmalloc_buf + offset_base,
            (unsigned char *)(SYS_TYPE)(MAC_ICCM_AHB_BASE + offset_base + ICCM_ROM_LEN), databyte);

        offset_base += databyte;
        len -= databyte;
    } while(len > 0);

#ifdef ICCM_CHECK
    offset_base =0;
    len = ICCM_CHECK_LEN;

    //host iccm ram read
    do {
        databyte = (len > SRAM_MAX_LEN) ? SRAM_MAX_LEN : len;

        hif_ops->hi_random_ram_read(buf_iccm_rd + offset_base,
            (unsigned char*)(SYS_TYPE)(MAC_ICCM_AHB_BASE + offset_base + ICCM_ROM_LEN), databyte);

        offset_base += databyte;
        len -= databyte;
    } while(len > 0);

    if (memcmp(buf_iccm_rd, kmalloc_buf, ICCM_CHECK_LEN)) {
        ERROR_DEBUG_OUT("Host HAL: write ICCM ERROR!!!! \n");
        release_firmware(fw);
        //FREE(kmalloc_buf, "sdio_write");
        return false;

    } else {
        AML_INFO("Host HAL: write ICCM SUCCESS!!!! \n");
    }
#endif

    /* Starting download DCCM */
    src = (unsigned char *)fw->data + (ICCM_ALL_LEN / 4) * BYTE_IN_LINE;
    len = DCCM_ALL_LEN;
    offset_base = 0;

    for (i = 0; i < len /4; i++) {
        tmp_buf[8] = 0;
        strncpy(tmp_buf, (char *)src, 8);
        if ((err = kstrtouint(tmp_buf, 16, &tmp_val))) {
            release_firmware(fw);
            //FREE(kmalloc_buf, "sdio_write");
            return err;
        }
        *(unsigned int *)&kmalloc_buf[4 * i] = __swab32(tmp_val);
        src += BYTE_IN_LINE;
    }

    AML_INFO(" dccm img len 0x%x, start download dccm\n", len);
    do {
        databyte = (len > SRAM_MAX_LEN) ? SRAM_MAX_LEN : len;

        hif_ops->hi_random_ram_write(kmalloc_buf + offset_base,
            (unsigned char*)(SYS_TYPE)(MAC_DCCM_AHB_BASE + offset_base), databyte);

        offset_base += databyte;
        len -= databyte;
    } while(len > 0);


#if 1
    len = DCCM_CHECK_LEN;
    offset_base = 0;

    //host iccm ram read
    do {
        databyte = (len > SRAM_MAX_LEN) ? SRAM_MAX_LEN : len;

        hif_ops->hi_random_ram_read(buf_iccm_rd + offset_base,
            (unsigned char*)(SYS_TYPE)(MAC_DCCM_AHB_BASE + offset_base), databyte);

        offset_base += databyte;
        len -= databyte;
    } while(len > 0);

    if (memcmp(buf_iccm_rd, kmalloc_buf, DCCM_CHECK_LEN)) {
        ERROR_DEBUG_OUT("Host HAL: write DCCM ERROR!!!! \n");
        release_firmware(fw);
        //FREE(kmalloc_buf, "sdio_write");
        return false;

    } else {
        AML_INFO("Host HAL: write DCCM SUCCESS!!!! \n");
    }
#endif

    release_firmware(fw);
    //FREE(kmalloc_buf, "sdio_write");

    return true;
}

EXPORT_SYMBOL(aml_download_wifi_fw_img);

void aml_sdio_hw_init(void)
{
    uint32_t data;

    aml_sdio_calibration();

    /*
     * enable auto TX and related configuration
     */
    aml_sdio_self_define_domain_write8(RG_SCFG_FUNC1_AUTO_TX,
            aml_sdio_self_define_domain_read8(RG_SCFG_FUNC1_AUTO_TX) | BIT(4));

    /*
     * frame flag bypass for function4
     *  BIT(8)=1, w/o setting address
     *  BIT(9)=1, disable sdio updating page table ptr.
     *  BIT(10)=1, enable sdio update page read ptr(0xa070a8 low 8bit).
     *  BIT(16)=1, for func6 wrapper around by rtl
     *  BIT(25)=1, disable function 5 section address-mapping (for BT?)
     *             when this feature is disabled,
     *             all 128k space in one sdio-function use only
     *             one address-mapping: 32-bit AHB Address = BaseAddr + cmdRegAddr
     */
    /* coverity[overrun-buffer-val] - length is correct */
    aml_sdio_random_ram_read((unsigned char *)&data,
            (unsigned char *)(uintptr_t)RG_SDIO_IF_MISC_CTRL, sizeof(data));
    data |= BIT(8) | BIT(10) | BIT(16) | BIT(25);
    /* coverity[overrun-buffer-val] - length is correct */
    aml_sdio_random_ram_write((unsigned char *)&data,
            (unsigned char *)(uintptr_t)RG_SDIO_IF_MISC_CTRL, sizeof(data));

    data = BIT(0);
    /* coverity[overrun-buffer-val] - length is correct */
    aml_sdio_random_ram_write((unsigned char *)&data,
            (unsigned char *)(uintptr_t)RG_SDIO_IF_INTR2CPU_ENABLE, sizeof(data));
}
EXPORT_SYMBOL(aml_sdio_hw_init);
