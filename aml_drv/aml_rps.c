/**
 ******************************************************************************
 *
 * @file aml_rps.c
 *
 * @brief Entry point of the AML driver
 *
 * Copyright (C) Amlogic 2012-2023
 *
 ******************************************************************************
 */

#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <net/addrconf.h>
#include <net/cfg80211.h>
#include <net/ip.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 0, 0)
#include <net/netdev_rx_queue.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
#include <net/hotdata.h>
#include <net/rps.h>
#endif
#include "aml_rps.h"
#include "aml_utils.h"
#include "aml_interface.h"
#include "aml_defs.h"

#define AML_RPS_FLOW_ENTRIES    4096

#define BIT_FIELD_MASK(a, b)    (((unsigned) - 1 >> (31 - (b))) & ~((1U << (a)) - 1))

static int aml_rps_map_set(struct netdev_rx_queue *queue, uint32_t cpu_mask)
{
    struct rps_map *old_map, *map;
    cpumask_var_t mask;
    int err, cpu, i;
    static DEFINE_MUTEX(rps_map_mutex);
    char buf[32];

    if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
        AML_INFO("alloc_cpumask_var fail.\n");
        return -ENOMEM;
    }

    sprintf(buf, "%x", cpu_mask);
    err = bitmap_parse(buf, strlen(buf), cpumask_bits(mask), nr_cpumask_bits);
    if (err) {
        free_cpumask_var(mask);
        AML_INFO("bitmap_parse fail");
        return err;
    }

    map = kzalloc(max_t(unsigned int,
                RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES), GFP_KERNEL);
    if (!map) {
        free_cpumask_var(mask);
        AML_INFO("map malloc fail.\n");
        return -ENOMEM;
    }

    i = 0;
    for_each_cpu_and(cpu, mask, cpu_online_mask)
        map->cpus[i++] = cpu;
    if (i) {
        map->len = i;
    } else {
        kfree(map);
        map = NULL;
    }

    mutex_lock(&rps_map_mutex);
    old_map = rcu_dereference_protected(queue->rps_map, mutex_is_locked(&rps_map_mutex));
    /* coverity[overrun-local] , because of [] */
    rcu_assign_pointer(queue->rps_map, map);
    if (map)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
        static_branch_inc(&rps_needed);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        static_key_slow_inc(&rps_needed);
#endif
    if (old_map)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
        static_branch_dec(&rps_needed);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        static_key_slow_dec(&rps_needed);
#endif
    mutex_unlock(&rps_map_mutex);

    if (old_map)
        kfree_rcu(old_map, rcu);
    free_cpumask_var(mask);

    return 0;
}

extern unsigned int aml_bus_type;

int aml_rps_cpus_enable(struct net_device *net)
{
    int rx_idx = 0;
    uint32_t cpu_mask = BIT_FIELD_MASK(0, num_online_cpus() - 1);

    if (aml_bus_type == PCIE_MODE)
        cpu_mask = BIT_FIELD_MASK(0, num_online_cpus() - 1);
    else if (aml_bus_type == SDIO_MODE)
        cpu_mask = 1 << (num_online_cpus() - 1); //bind cpu for s905l3a
    if (net && net->_rx) {
        for (rx_idx = 0; rx_idx < net->num_rx_queues; rx_idx++)
            aml_rps_map_set(net->_rx + rx_idx, cpu_mask);
    }
    return 0;
}

int aml_rps_cpus_disable(struct net_device *net)
{
    int rx_idx = 0;

    if (net && net->_rx) {
        for (rx_idx = 0; rx_idx < net->num_rx_queues; rx_idx++)
            aml_rps_map_set(net->_rx + rx_idx, 0);
    }
    return 0;
}

static int aml_xps_map_set(struct net_device *net, uint32_t cpu_mask)
{
    cpumask_var_t mask;
    int tx_idx = 0;
    int err = 0;
    char buf[32];

    AML_INFO("set xps_cpus as [%0x]\n", cpu_mask);

    if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
        AML_INFO("alloc_cpumask_var fail.\n");
        return -ENOMEM;
    }

    sprintf(buf, "%0x", cpu_mask);
    err = bitmap_parse(buf, strlen(buf), cpumask_bits(mask), nr_cpumask_bits);
    if (err) {
        free_cpumask_var(mask);
        AML_INFO("bitmap_parse fail.\n");
        return err;
    }

    for (tx_idx = 0; (tx_idx < net->num_tx_queues) && (err == 0); tx_idx++)
        err = netif_set_xps_queue(net, mask, tx_idx);
    free_cpumask_var(mask);

    if (err != 0)
        AML_INFO("XPS mapping cpu fail\n");
    return err;
}

int aml_xps_cpus_enable(struct net_device *net)
{
    if (!net)
        return -1;

    aml_xps_map_set(net, BIT_FIELD_MASK(0, num_online_cpus() - 1));

    return 0;
}

static void aml_rps_dev_flow_table_release(struct rcu_head *rcu)
{
    struct rps_dev_flow_table *table = container_of(rcu, struct rps_dev_flow_table, rcu);

    vfree(table);
}

static int aml_rps_dev_flow_table_set(struct netdev_rx_queue *queue, unsigned long count)
{
    unsigned long mask;
    struct rps_dev_flow_table *table, *old_table;
    static DEFINE_SPINLOCK(rps_dev_flow_lock);

    if (!capable(CAP_NET_ADMIN))
    {
        AML_INFO("check capable fail.\n");
        return -EPERM;
    }

    if (count) {
        mask = count - 1;
        /* mask = roundup_pow_of_two(count) - 1;
         * without overflows...
         */
        while ((mask | (mask >> 1)) != mask)
            mask |= (mask >> 1);
        /* On 64 bit arches, must check mask fits in table->mask (u32),
         * and on 32bit arches, must check
         * RPS_DEV_FLOW_TABLE_SIZE(mask + 1) doesn't overflow.
         */
#if BITS_PER_LONG > 32
        if (mask > (unsigned long)(u32)mask)
        {
            AML_INFO("check mask fail %lx\n", mask);
            return -EINVAL;
        }
#else
        if (mask > (ULONG_MAX - RPS_DEV_FLOW_TABLE_SIZE(1)) / sizeof(struct rps_dev_flow)) {
            AML_INFO("check mask fail %lx, max %lx\n", mask,
                    (ULONG_MAX - RPS_DEV_FLOW_TABLE_SIZE(1)) / sizeof(struct rps_dev_flow));
            /* Enforce a limit to prevent overflow */
            return -EINVAL;
        }
#endif
        table = vmalloc(RPS_DEV_FLOW_TABLE_SIZE(mask + 1));
        if (!table)
        {
            AML_INFO("alloc table fail\n");
            return -ENOMEM;
        }
        table->mask = mask;
        for (count = 0; count <= mask; count++)
            table->flows[count].cpu = RPS_NO_CPU;
    } else {
        table = NULL;
    }

    spin_lock(&rps_dev_flow_lock);
    old_table = rcu_dereference_protected(queue->rps_flow_table,
            lockdep_is_held(&rps_dev_flow_lock));
    /* coverity[overrun-local] , because of [] */
    rcu_assign_pointer(queue->rps_flow_table, table);
    spin_unlock(&rps_dev_flow_lock);

    if (old_table)
        call_rcu(&old_table->rcu, aml_rps_dev_flow_table_release);

    return 0;
}

int aml_rps_dev_flow_table_enable(struct net_device *net)
{
    int rx_idx = 0;

    if (net && net->_rx) {
        for (rx_idx = 0; rx_idx < net->num_rx_queues; rx_idx++)
            aml_rps_dev_flow_table_set(net->_rx + rx_idx, AML_RPS_FLOW_ENTRIES);
    }
    return 0;
}

//#define RPS_SOCK_FLOW_TABLE_SIZE(_num) (offsetof(struct rps_sock_flow_table, ents[_num]))
static int aml_rps_sock_flow_sysctl_set(unsigned int size)
{
    unsigned int orig_size;
    int ret = 0, i;
    struct rps_sock_flow_table *orig_sock_table = NULL, *sock_table = NULL;
    static DEFINE_MUTEX(sock_flow_mutex);

    if (ret < 0 || size == 0)
        return -1;

    mutex_lock(&sock_flow_mutex);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
    orig_sock_table = rcu_dereference_protected(net_hotdata.rps_sock_flow_table,
        lockdep_is_held(&sock_flow_mutex));
#else
    orig_sock_table = rcu_dereference_protected(rps_sock_flow_table,
        lockdep_is_held(&sock_flow_mutex));
#endif
    orig_size = orig_sock_table ? orig_sock_table->mask + 1 : 0;

    if (size > (1 << 29)) {
        /* Enforce limit to prevent overflow */
        mutex_unlock(&sock_flow_mutex);
        AML_INFO("size is large %d\n", size);
        return -EINVAL;
    }
    /* coverity[CONSTANT_EXPRESSION_RESULT] */
    size = (unsigned int)roundup_pow_of_two(size);
    if (size != orig_size && size > 0) {
        sock_table = vmalloc(sizeof(u32) * (size + 1)/*RPS_SOCK_FLOW_TABLE_SIZE(size)*/);
        if (!sock_table) {
            mutex_unlock(&sock_flow_mutex);
            AML_INFO("alloc table fail\n");
            return -ENOMEM;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
        /* coverity[CONSTANT_EXPRESSION_RESULT] */
        net_hotdata.rps_cpu_mask = (unsigned int)roundup_pow_of_two(nr_cpu_ids) - 1;
#else
        /* coverity[CONSTANT_EXPRESSION_RESULT] */
        rps_cpu_mask = (unsigned int)roundup_pow_of_two(nr_cpu_ids) - 1;
#endif
        sock_table->mask = size - 1;
    } else {
        mutex_unlock(&sock_flow_mutex);
        AML_INFO("Done, new size is same as orig_size, %d\n", orig_size);
        return 0;
    }

    for (i = 0; i < size; i++) {
        /* coverity[OVERRUN] , because of ents[0] */
        sock_table->ents[i] = RPS_NO_CPU;
    }
    /* coverity[OVERRUN] , because of ents[0] */
    rcu_assign_pointer(orig_sock_table, sock_table);
    if (sock_table) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
        static_branch_inc(&rps_needed);
        static_branch_inc(&rfs_needed);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        static_key_slow_inc(&rps_needed);
#endif
    }
    if (orig_sock_table) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
        static_branch_dec(&rps_needed);
        static_branch_dec(&rfs_needed);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        static_key_slow_dec(&rps_needed);
#endif
        synchronize_rcu();
        vfree(orig_sock_table);
    }
    mutex_unlock(&sock_flow_mutex);
    return 0;
}

int aml_rps_sock_flow_sysctl_enable(void)
{
    int cpu_nums = num_online_cpus();

    if (cpu_nums <= 0)
        return 0;

    aml_rps_sock_flow_sysctl_set(AML_RPS_FLOW_ENTRIES * cpu_nums);

    return 0;
}
