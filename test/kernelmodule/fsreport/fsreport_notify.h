// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/fsreport/fsreport_notify.c
 *
 *  Written 2025 by AlexKing
 *
 */
#ifndef _FS_REPORT_NOTIFY_H_
#define _FS_REPORT_NOTIFY_H_

//FSREPORT_NF_DATA_T
#define MAX_NF_DATALEN 256
typedef struct {
	int fclus;//出错的簇号
	int post;
	char devid[MAX_NF_DATALEN];//数据
}FSREPORT_NF_DATA_T;


/****************************************************************/
/**
 *  notifier API
 **/
/****************************************************************/

#define FSREPORT_NF_CLUSTER_ERROR   0x0001      //无效的簇链


/* 定义参数管理结构体 */
#define FSREPORT_NF_ARGS_NR         (10)
struct fsreport_notifiter_args {
    unsigned int num;                       /* 当前参数个数 */
    void *arg[FSREPORT_NF_ARGS_NR];            /* 当前参数的地址 */
};

struct fsreport_notifier_block {
    int (*notifier_call)(struct fsreport_notifier_block *nb, unsigned long action, void *data);
    void *nb;   /* 用于指向操作系统层的管理结构体 */
    struct fsreport_notifier_block *next;
    int priority;
};
typedef int (*fsreport_notifier_fn_t)(struct fsreport_notifier_block *nb, unsigned long action, void *data);

#define FSREPORT_NOTIFY_DONE       0x0000      /* Don't care */
#define FSREPORT_NOTIFY_OK         0x0001      /* Suits me */
#define FSREPORT_NOTIFY_STOP_MASK  0x8000      /* Don't call further */

static inline int fsreport_notifier_from_errno(int err)
{
    if (err)
        return FSREPORT_NOTIFY_STOP_MASK | (FSREPORT_NOTIFY_OK - err);

    return FSREPORT_NOTIFY_OK;
}

static inline int fsreport_notifier_to_errno(int ret)
{
    ret &= ~FSREPORT_NOTIFY_STOP_MASK;
    return ret > FSREPORT_NOTIFY_OK ? FSREPORT_NOTIFY_OK - ret : 0;
}

/**@fn      bal_notifier_register
 * @brief   在hikbaseal中注册通知链，用于HIK-BSP与KERNEL之间进行通信
 * @param   [in]hnb: 通知链处理函数
 * @param   [out]N/A
 * @return  errno on Linux
 */
extern int fsreport_notifier_register(struct fsreport_notifier_block *hnb);

/**@fn      bal_notifier_unregister
 * @brief   在hikbaseal中注销通知链
 * @param   [in]hnb: 通知链处理函数
 * @param   [out]N/A
 * @return  errno on Linux
 */
extern int fsreport_notifier_unregister(struct fsreport_notifier_block *hnb);

/**@fn      bal_notifiers_call
 * @brief   在hikbase中通知机制，用于向注册到通知链上的产生通知信号
 * @param   [in]val:    指定的通知事件值
 * @param   [in]dev:    指定的通知事件参数
 * @param   [out]N/A
 * @return  errno on Linux
 */
extern int fsreport_notifiers_call(unsigned long val, void *dev);

/**@fn      bal_notifiers_callv2
 * @brief   Author/Date: liweijie/2016-8-17
 * @brief   在hikbase中通知机制，用于向注册到通知链上的产生通知信号；参数可扩展
 * @param   [in]val:    指定的通知事件值
 * @param   [in]args:   紧跟后面参数个数；后面紧跟的参数类型为void*类型
 * @param   [out]N/A
 * @return  errno on Linux
 */
extern int fsreport_notifiers_callv2(unsigned long val, unsigned int args, ...);

#endif
