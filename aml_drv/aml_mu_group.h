/**
 ******************************************************************************
 *
 * @file aml_mu_group.h
 *
 * Copyright (C) Amlogic 2016-2021
 *
 ******************************************************************************
 */
#ifndef _AML_MU_GROUP_H_
#define _AML_MU_GROUP_H_

#include <linux/workqueue.h>
#include <linux/semaphore.h>

struct aml_hw;
struct aml_sta;

#ifdef CONFIG_AML_MUMIMO_TX

/**
 * struct aml_sta_group_info - Group Information for a STA
 *
 * @active: node for @mu->active_sta list
 * @update: node for @mu->update_sta list
 * @cnt: Number of groups the STA belongs to
 * @map: Bitfield of groups the sta belongs to
 * @traffic: Number of buffers sent since previous group selection
 * @group: Id of the group selected by previous group selection
 *         (cf @aml_mu_group_sta_select)
 */
struct aml_sta_group_info {
    struct list_head active;
    struct list_head update;
    u16 last_update;
    int cnt;
    u64 map;
    int traffic;
    u8  group;
};

/**
 * struct mu_group_info - Information about the users of a group
 *
 * @list: node for mu->active_groups
 * @group_id: Group identifier
 * @user_cnt: Number of the users in the group
 * @users: Pointer to the sta, ordered by user position
 */
struct aml_mu_group {
    struct list_head list;
    int group_id;
    int user_cnt;
    struct aml_sta *users[CONFIG_USER_MAX];
};

/**
 * struct aml_mu_info - Information about all MU group
 *
 * @active_groups: List of all possible groups. Ordered from the most recently
 *                 used one to the least one (and possibly never used)
 * @active_sta: List of MU beamformee sta that have been active (since previous
 *              group update). Ordered from the most recently active.
 * @update_sta: List of sta whose group information has changed and need to be
 *              updated at fw level
 * @groups: Table of all groups
 * @group_work: Work item used to schedule group update
 * @update_count: Counter used to identify the last group formation update.
 *                (cf aml_sta_group_info.last_update)
 * @lock: Lock taken during group update. If tx happens lock is taken, then tx
 *        will not used MU.
 * @next_group_assign: Next time the group selection should be run
 *                     (ref @aml_mu_group_sta_select)
 * @group_cnt: Number of group created
 */
struct aml_mu_info {
    struct list_head active_groups;
    struct list_head active_sta;
    struct list_head update_sta;
    struct aml_mu_group groups[NX_MU_GROUP_MAX];
    struct delayed_work group_work;
    u16 update_count;
    struct semaphore lock;
    unsigned long next_group_select;
    u8 group_cnt;
};

#define AML_SU_GROUP BIT_ULL(0)
#define AML_MU_GROUP_MASK 0x7ffffffffffffffeULL
#define AML_MU_GROUP_INTERVAL 200 /* in ms */
#define AML_MU_GROUP_SELECT_INTERVAL 100 /* in ms */
// minimum traffic in a AML_MU_GROUP_SELECT_INTERVAL to consider the sta
#define AML_MU_GROUP_MIN_TRAFFIC 50 /* in number of packet */


#define AML_GET_FIRST_GROUP_ID(map) (fls64(map) - 1)

#define group_sta_for_each(sta, id, map)                                \
    map = sta->group_info.map & AML_MU_GROUP_MASK;                     \
    for (id = (fls64(map) - 1) ; id > 0 ;                               \
         map &= ~(u64)BIT_ULL(id), id = (fls64(map) - 1))

#define group_for_each(id, map)                                         \
    for (id = (fls64(map) - 1) ; id > 0 ;                               \
         map &= ~(u64)BIT_ULL(id), id = (fls64(map) - 1))

#define AML_MUMIMO_INFO_POS_ID(info) (((info) >> 6) & 0x3)
#define AML_MUMIMO_INFO_GROUP_ID(info) ((info) & 0x3f)

static inline
struct aml_mu_group *aml_mu_group_from_id(struct aml_mu_info *mu, int id)
{
    if (id > NX_MU_GROUP_MAX)
        return NULL;

    return &mu->groups[id - 1];
}


void aml_mu_group_sta_init(struct aml_sta *sta,
                            const struct ieee80211_vht_cap *vht_cap);
void aml_mu_group_sta_del(struct aml_hw *aml_hw, struct aml_sta *sta);
u64 aml_mu_group_sta_get_map(struct aml_sta *sta);
int aml_mu_group_sta_get_pos(struct aml_hw *aml_hw, struct aml_sta *sta,
                              int group_id);

void aml_mu_group_init(struct aml_hw *aml_hw);

void aml_mu_set_active_sta(struct aml_hw *aml_hw, struct aml_sta *sta,
                            int traffic);
void aml_mu_set_active_group(struct aml_hw *aml_hw, int group_id);
void aml_mu_group_sta_select(struct aml_hw *aml_hw);


#else /* ! CONFIG_AML_MUMIMO_TX */

static inline
void aml_mu_group_sta_init(struct aml_sta *sta,
                            const struct ieee80211_vht_cap *vht_cap)
{}

static inline
void aml_mu_group_sta_del(struct aml_hw *aml_hw, struct aml_sta *sta)
{}

static inline
u64 aml_mu_group_sta_get_map(struct aml_sta *sta)
{
    return 0;
}

static inline
int aml_mu_group_sta_get_pos(struct aml_hw *aml_hw, struct aml_sta *sta,
                              int group_id)
{
    return 0;
}

static inline
void aml_mu_group_init(struct aml_hw *aml_hw)
{}

static inline
void aml_mu_set_active_sta(struct aml_hw *aml_hw, struct aml_sta *sta,
                            int traffic)
{}

static inline
void aml_mu_set_active_group(struct aml_hw *aml_hw, int group_id)
{}

static inline
void aml_mu_group_sta_select(struct aml_hw *aml_hw)
{}

#endif /* CONFIG_AML_MUMIMO_TX */

#endif /* _AML_MU_GROUP_H_ */

