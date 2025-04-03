/**
 ******************************************************************************
 *
 * @file aml_debugfs.h
 *
 * @brief Miscellaneous utility function definitions
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */


#ifndef _AML_DEBUGFS_H_
#define _AML_DEBUGFS_H_

#include <linux/workqueue.h>
#include <linux/if_ether.h>
#include "hal_desc.h"
#include "aml_fw_trace.h"

struct aml_hw;
struct aml_sta;
struct aml_vif;
struct aml_txq;

#define DEBUGFS_ADD_FILE(name, parent, mode) do {                  \
        struct dentry *__tmp;                                      \
        __tmp = debugfs_create_file(#name, mode, parent, aml_hw,  \
                                    &aml_dbgfs_##name##_ops);     \
        if (IS_ERR_OR_NULL(__tmp))                                 \
            goto err;                                              \
    } while (0)

#define DEBUGFS_ADD_BOOL(name, parent, ptr) do {                            \
        struct dentry *__tmp;                                               \
        __tmp = debugfs_create_bool(#name, S_IWUSR | S_IRUSR, parent, ptr); \
        if (IS_ERR_OR_NULL(__tmp))                                          \
            goto err;                                                       \
    } while (0)

#define DEBUGFS_ADD_X64(name, parent, ptr) do {                         \
        debugfs_create_x64(#name, S_IWUSR | S_IRUSR,parent, ptr);       \
    } while (0)

#define DEBUGFS_ADD_U64(name, parent, ptr, mode) do {           \
        debugfs_create_u64(#name, mode, parent, ptr);           \
    } while (0)

#define DEBUGFS_ADD_X32(name, parent, ptr) do {                         \
        debugfs_create_x32(#name, S_IWUSR | S_IRUSR, parent, ptr);      \
    } while (0)

#define DEBUGFS_ADD_U32(name, parent, ptr, mode) do {           \
        debugfs_create_u32(#name, mode, parent, ptr);           \
    } while (0)


/* file operation */
#define DEBUGFS_READ_FUNC(name)                                         \
    static ssize_t aml_dbgfs_##name##_read(struct file *file,          \
                                            char __user *user_buf,      \
                                            size_t count, loff_t *ppos);

#define DEBUGFS_WRITE_FUNC(name)                                         \
    static ssize_t aml_dbgfs_##name##_write(struct file *file,          \
                                             const char __user *user_buf,\
                                             size_t count, loff_t *ppos);

#define DEBUGFS_OPEN_FUNC(name)                              \
    static int aml_dbgfs_##name##_open(struct inode *inode, \
                                        struct file *file);

#define DEBUGFS_RELEASE_FUNC(name)                              \
    static int aml_dbgfs_##name##_release(struct inode *inode, \
                                           struct file *file);

#define DEBUGFS_READ_FILE_OPS(name)                             \
    DEBUGFS_READ_FUNC(name);                                    \
static const struct file_operations aml_dbgfs_##name##_ops = { \
    .read   = aml_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DEBUGFS_WRITE_FILE_OPS(name)                            \
    DEBUGFS_WRITE_FUNC(name);                                   \
static const struct file_operations aml_dbgfs_##name##_ops = { \
    .write  = aml_dbgfs_##name##_write,                        \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DEBUGFS_READ_WRITE_FILE_OPS(name)                       \
    DEBUGFS_READ_FUNC(name);                                    \
    DEBUGFS_WRITE_FUNC(name);                                   \
static const struct file_operations aml_dbgfs_##name##_ops = { \
    .write  = aml_dbgfs_##name##_write,                        \
    .read   = aml_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DEBUGFS_READ_WRITE_OPEN_RELEASE_FILE_OPS(name)              \
    DEBUGFS_READ_FUNC(name);                                        \
    DEBUGFS_WRITE_FUNC(name);                                       \
    DEBUGFS_OPEN_FUNC(name);                                        \
    DEBUGFS_RELEASE_FUNC(name);                                     \
static const struct file_operations aml_dbgfs_##name##_ops = {     \
    .write   = aml_dbgfs_##name##_write,                           \
    .read    = aml_dbgfs_##name##_read,                            \
    .open    = aml_dbgfs_##name##_open,                            \
    .release = aml_dbgfs_##name##_release,                         \
    .llseek  = generic_file_llseek,                                 \
};


#define TXQ_STA_PREF "tid|"
#define TXQ_STA_PREF_FMT "%3d|"

#ifdef CONFIG_AML_FULLMAC
#define TXQ_VIF_PREF "type|"
#define TXQ_VIF_PREF_FMT "%4s|"
#else
#define TXQ_VIF_PREF "AC|"
#define TXQ_VIF_PREF_FMT "%2s|"
#endif /* CONFIG_AML_FULLMAC */

#define TXQ_HDR "idx|  status|credit|ready|retry|pushed"
#define TXQ_HDR_FMT "%3d|%s%s%s%s%s%s%s%s%s|%6d|%5d|%5d|%6d"

#ifdef CONFIG_AML_AMSDUS_TX
#ifdef CONFIG_AML_FULLMAC
#define TXQ_HDR_SUFF "|amsdu"
#define TXQ_HDR_SUFF_FMT "|%5d"
#else
#define TXQ_HDR_SUFF "|amsdu-ht|amdsu-vht"
#define TXQ_HDR_SUFF_FMT "|%8d|%9d"
#endif /* CONFIG_AML_FULLMAC */
#else
#define TXQ_HDR_SUFF ""
#define TXQ_HDR_SUF_FMT ""
#endif /* CONFIG_AML_AMSDUS_TX */

#define TXQ_HDR_MAX_LEN (sizeof(TXQ_STA_PREF) + sizeof(TXQ_HDR) + sizeof(TXQ_HDR_SUFF) + 1)

#ifdef CONFIG_AML_FULLMAC
#define PS_HDR  "Legacy PS: ready=%d, sp=%d / UAPSD: ready=%d, sp=%d"
#define PS_HDR_LEGACY "Legacy PS: ready=%d, sp=%d"
#define PS_HDR_UAPSD  "UAPSD: ready=%d, sp=%d"
#define PS_HDR_MAX_LEN  sizeof("Legacy PS: ready=xxx, sp=xxx / UAPSD: ready=xxx, sp=xxx\n")
#else
#define PS_HDR ""
#define PS_HDR_MAX_LEN 0
#endif /* CONFIG_AML_FULLMAC */

#define STA_HDR "** STA %d (%pM)\n"
#define STA_HDR_MAX_LEN sizeof("- STA xx (xx:xx:xx:xx:xx:xx)\n") + PS_HDR_MAX_LEN

#ifdef CONFIG_AML_FULLMAC
#define VIF_HDR "* VIF [%d] %s\n"
#define VIF_HDR_MAX_LEN sizeof(VIF_HDR) + IFNAMSIZ
#else
#define VIF_HDR "* VIF [%d]\n"
#define VIF_HDR_MAX_LEN sizeof(VIF_HDR)
#endif


#ifdef CONFIG_AML_AMSDUS_TX

#ifdef CONFIG_AML_FULLMAC
#define VIF_SEP "---------------------------------------\n"
#else
#define VIF_SEP "----------------------------------------------------\n"
#endif /* CONFIG_AML_FULLMAC */

#else /* ! CONFIG_AML_AMSDUS_TX */
#define VIF_SEP "---------------------------------\n"
#endif /* CONFIG_AML_AMSDUS_TX*/

#define VIF_SEP_LEN sizeof(VIF_SEP)

#define CAPTION "status: L=in hwq list, F=stop full, P=stop sta PS, V=stop vif PS,\
 C=stop channel, S=stop CSA, M=stop MU, N=Ndev queue stopped, I=CO Inactive"
#define CAPTION_LEN sizeof(CAPTION)

#define STA_TXQ 0
#define VIF_TXQ 1

#define LINE_MAX_SZ 150

struct st {
    char line[LINE_MAX_SZ + 1];
    unsigned int r_idx;
};


#ifdef CONFIG_AML_DEBUGFS

struct aml_debugfs {
    unsigned long long rateidx;
    struct dentry *dir;
    struct dentry *dir_stas;
    bool trace_prst;

    char helper_cmd[64];
    struct work_struct helper_work;
    bool helper_scheduled;
    spinlock_t umh_lock;
    bool unregistering;

#ifndef CONFIG_AML_FHOST
    struct aml_fw_trace fw_trace;
#endif /* CONFIG_AML_FHOST */

#ifdef CONFIG_AML_FULLMAC
    struct work_struct sta_work;
    struct dentry *dir_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    uint8_t sta_idx;
    struct dentry *dir_rc;
    struct dentry *dir_rc_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    int rc_config[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    struct list_head rc_config_save;
    struct dentry *dir_twt;
    struct dentry *dir_twt_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
#endif
};

#ifdef CONFIG_AML_FULLMAC

// Max duration in msecs to save rate config for a sta after disconnection
#define RC_CONFIG_DUR 600000

struct aml_rc_config_save {
    struct list_head list;
    unsigned long timestamp;
    int rate;
    u8 mac_addr[ETH_ALEN];
};
#endif

int aml_dbgfs_register(struct aml_hw *aml_hw, const char *name);
void aml_dbgfs_unregister(struct aml_hw *aml_hw);
int aml_um_helper(struct aml_debugfs *aml_debugfs, const char *cmd);
int aml_trigger_um_helper(struct aml_debugfs *aml_debugfs);
void aml_wait_um_helper(struct aml_hw *aml_hw);
#ifdef CONFIG_AML_FULLMAC
void aml_dbgfs_register_sta(struct aml_hw *aml_hw, struct aml_sta *sta);
void aml_dbgfs_unregister_sta(struct aml_hw *aml_hw, struct aml_sta *sta);
#endif

int aml_dbgfs_register_fw_dump(struct aml_hw *aml_hw,
                                struct dentry *dir_drv,
                                struct dentry *dir_diags);
void aml_dbgfs_trigger_fw_dump(struct aml_hw *aml_hw, char *reason);

void aml_fw_trace_dump(struct aml_hw *aml_hw);
void aml_fw_trace_reset(struct aml_hw *aml_hw);
void idx_to_rate_cfg(int idx, union aml_rate_ctrl_info *r_cfg, int *ru_size);
int aml_dbgfs_txq(char *buf, size_t size, struct aml_txq *txq, int type, int tid, char *name);
int aml_dbgfs_txq_sta(char *buf, size_t size, struct aml_sta *aml_sta, struct aml_hw *aml_hw);
int aml_dbgfs_txq_vif(char *buf, size_t size, struct aml_vif *aml_vif, struct aml_hw *aml_hw);
int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
                      int sgi, int pre, int dcm, int *r_idx, bool bprint);
int print_rate_from_cfg(char *buf, int size, u32 rate_config, int *r_idx, int ru_size, bool bprint);
int compare_idx(const void *st1, const void *st2);

#else

struct aml_debugfs {
    unsigned long long rateidx;
    struct dentry *dir;
    struct dentry *dir_stas;
    bool trace_prst;

    char helper_cmd[64];
    struct work_struct helper_work;
    bool helper_scheduled;
    spinlock_t umh_lock;
    bool unregistering;

#ifndef CONFIG_AML_FHOST
    struct aml_fw_trace fw_trace;
#endif /* CONFIG_AML_FHOST */

#ifdef CONFIG_AML_FULLMAC
    struct work_struct sta_work;
    struct dentry *dir_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    uint8_t sta_idx;
    struct dentry *dir_rc;
    struct dentry *dir_rc_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    int rc_config[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    struct list_head rc_config_save;
    struct dentry *dir_twt;
    struct dentry *dir_twt_sta[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
#endif

};

static inline int aml_dbgfs_register(struct aml_hw *aml_hw, const char *name) {return 0;}
static inline void aml_dbgfs_unregister(struct aml_hw *aml_hw) {}
static inline int aml_um_helper(struct aml_debugfs *aml_debugfs, const char *cmd) {return 0;}
static inline int aml_trigger_um_helper(struct aml_debugfs *aml_debugfs) {return 0;}
static inline void aml_wait_um_helper(struct aml_hw *aml_hw) {}
#ifdef CONFIG_AML_FULLMAC
static inline void aml_dbgfs_register_sta(struct aml_hw *aml_hw, struct aml_sta *sta) {}
static inline void aml_dbgfs_unregister_sta(struct aml_hw *aml_hw, struct aml_sta *sta) {}
#endif

static inline void aml_fw_trace_dump(struct aml_hw *aml_hw) {}
static inline void aml_fw_trace_reset(struct aml_hw *aml_hw) {}

static inline void idx_to_rate_cfg(int idx, union aml_rate_ctrl_info *r_cfg, int *ru_size) {}
static inline int print_rate_from_cfg(char *buf, int size, u32 rate_config,
            int *r_idx, int ru_size, bool bprint) {return 0;}
static inline int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
            int sgi, int pre, int dcm, int *r_idx, bool bprint) {return 0;}
static inline void aml_dbgfs_trigger_fw_dump(struct aml_hw *aml_hw, char *reason) {}
static inline int compare_idx(const void *st1, const void *st2) {return 0;}
static inline int aml_dbgfs_txq_vif(char *buf, size_t size,
            struct aml_vif *aml_vif, struct aml_hw *aml_hw) {return 0;}

#endif /* CONFIG_AML_DEBUGFS */


#endif /* _AML_DEBUGFS_H_ */
