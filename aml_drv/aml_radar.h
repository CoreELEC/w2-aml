/**
 ******************************************************************************
 *
 * @file aml_radar.h
 *
 * @brief Functions to handle radar detection
 *
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ******************************************************************************
 */
#ifndef _AML_RADAR_H_
#define _AML_RADAR_H_

#include <linux/nl80211.h>

struct aml_vif;
struct aml_hw;

enum aml_radar_chain {
    AML_RADAR_RIU = 0,
    AML_RADAR_FCU,
    AML_RADAR_LAST
};

enum aml_radar_detector {
    AML_RADAR_DETECT_DISABLE = 0, /* Ignore radar pulses */
    AML_RADAR_DETECT_ENABLE  = 1, /* Process pattern detection but do not
                                      report radar to upper layer (for test) */
    AML_RADAR_DETECT_REPORT  = 2  /* Process pattern detection and report
                                      radar to upper layer. */
};

#ifdef CONFIG_AML_RADAR
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#define AML_RADAR_PULSE_MAX  32

/**
 * struct aml_radar_pulses - List of pulses reported by HW
 * @index: write index
 * @count: number of valid pulses
 * @buffer: buffer of pulses
 */
struct aml_radar_pulses {
    /* Last radar pulses received */
    int index;
    int count;
    u32 buffer[AML_RADAR_PULSE_MAX];
};

/**
 * struct dfs_pattern_detector - DFS pattern detector
 * @region: active DFS region, NL80211_DFS_UNSET until set
 * @num_radar_types: number of different radar types
 * @last_pulse_ts: time stamp of last valid pulse in usecs
 * @prev_jiffies:
 * @radar_detector_specs: array of radar detection specs
 * @channel_detectors: list connecting channel_detector elements
 */
struct dfs_pattern_detector {
    u8 enabled;
    enum nl80211_dfs_regions region;
    u8 num_radar_types;
    u64 last_pulse_ts;
    u32 prev_jiffies;
    const struct radar_detector_specs *radar_spec;
    struct list_head detectors[];
};

#define NX_NB_RADAR_DETECTED 4

/**
 * struct aml_radar_detected - List of radar detected
 */
struct aml_radar_detected {
    u16 index;
    u16 count;
    s64 time[NX_NB_RADAR_DETECTED];
    s16 freq[NX_NB_RADAR_DETECTED];
};


struct aml_radar {
    struct aml_radar_pulses pulses[AML_RADAR_LAST];
    struct dfs_pattern_detector *dpd[AML_RADAR_LAST];
    struct aml_radar_detected detected[AML_RADAR_LAST];
    struct work_struct detection_work;  /* Work used to process radar pulses */
    spinlock_t lock;                    /* lock for pulses processing */

    /* In softmac cac is handled by mac80211 */
#ifdef CONFIG_AML_FULLMAC
    struct delayed_work cac_work;       /* Work used to handle CAC */
    struct aml_vif *cac_vif;           /* vif on which we started CAC */
#endif
};

bool aml_radar_detection_init(struct aml_radar *radar);
void aml_radar_detection_deinit(struct aml_radar *radar);
bool aml_radar_set_domain(struct aml_radar *radar,
                           enum nl80211_dfs_regions region);
void aml_radar_detection_enable(struct aml_radar *radar, u8 enable, u8 chain);
bool aml_radar_detection_is_enable(struct aml_radar *radar, u8 chain);
void aml_radar_start_cac(struct aml_radar *radar, u32 cac_time_ms,
                          struct aml_vif *vif);
void aml_radar_cancel_cac(struct aml_radar *radar);
void aml_radar_detection_enable_on_cur_channel(struct aml_hw *aml_hw);
int  aml_radar_dump_pattern_detector(char *buf, size_t len,
                                      struct aml_radar *radar, u8 chain);
int  aml_radar_dump_radar_detected(char *buf, size_t len,
                                    struct aml_radar *radar, u8 chain);

#else

struct aml_radar {
};

static inline bool aml_radar_detection_init(struct aml_radar *radar)
{return true;}

static inline void aml_radar_detection_deinit(struct aml_radar *radar)
{}

static inline bool aml_radar_set_domain(struct aml_radar *radar,
                                         enum nl80211_dfs_regions region)
{return true;}

static inline void aml_radar_detection_enable(struct aml_radar *radar,
                                               u8 enable, u8 chain)
{}

static inline bool aml_radar_detection_is_enable(struct aml_radar *radar,
                                                 u8 chain)
{return false;}

static inline void aml_radar_start_cac(struct aml_radar *radar,
                                        u32 cac_time_ms, struct aml_vif *vif)
{}

static inline void aml_radar_cancel_cac(struct aml_radar *radar)
{}

static inline void aml_radar_detection_enable_on_cur_channel(struct aml_hw *aml_hw)
{}

static inline int aml_radar_dump_pattern_detector(char *buf, size_t len,
                                                   struct aml_radar *radar,
                                                   u8 chain)
{return 0;}

static inline int aml_radar_dump_radar_detected(char *buf, size_t len,
                                                 struct aml_radar *radar,
                                                 u8 chain)
{return 0;}

#endif /* CONFIG_AML_RADAR */

#endif // _AML_RADAR_H_
