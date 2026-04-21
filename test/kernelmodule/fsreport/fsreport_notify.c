// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/fsreport/fsreport_notify.c
 *
 *  Written 2025 by AlexKing
 *
 */
#include <linux/notifier.h>
#include <linux/slab.h>
#include "fsreport_notify.h"
#include <linux/module.h>

static DEFINE_MUTEX(fsreport_nf_mutex);



static struct fsreport_notifier_block *fsreport_nf_head = NULL;

/**@fn      fsreport_notify_register
 * @brief   向通知链表中注册通知
 * @param   [in]nl:通知链表
 * @param   [in]n:通知函数
 * @param   [out]N/A
 * @return  Errno on Linux
 */
static int fsreport_notify_register(struct fsreport_notifier_block **nl, struct fsreport_notifier_block *n)
{
    while ((*nl) != NULL)
    {
        if (n->priority > (*nl)->priority)
            break;
        nl = &((*nl)->next);
    }
    n->next = *nl;
    *nl = n;
    return 0;
}

/**@fn      fsreport_notify_unregister
 * @brief   向通知链表中注销通知
 * @param   [in]nl:通知链表
 * @param   [in]n:通知函数
 * @param   [out]N/A
 * @return  Errno on Linux
 */
static int fsreport_notify_unregister(struct fsreport_notifier_block **nl, struct fsreport_notifier_block *n)
{
    while ((*nl) != NULL)
    {
        if ((*nl) == n)
        {
            *nl = n->next;
            return 0;
        }
        nl = &((*nl)->next);
    }
    return -ENOENT;
}

/**@fn      fsreport_call
 * @param   [in]nl:通知链表
 * @param   [in]val:通知事件
 * @param   [in]v:通知事件的参数
 * @param   [in]nr_to_call:调用处理的次数
 * @param   [out]nr_calls:返回以调用成功的次数
 * @return  Errno on Linux
 */
static int fsreport_call(struct fsreport_notifier_block **nl,
                   unsigned long val, void *v,
                   int nr_to_call, int *nr_calls)
{
    int ret = FSREPORT_NOTIFY_DONE;
    struct fsreport_notifier_block *nb, *next_nb;

    nb = *nl;

    while (nb && nr_to_call) {
        next_nb = nb->next;

        ret = nb->notifier_call(nb, val, v);

        if (nr_calls)
            (*nr_calls)++;

        if ((ret & FSREPORT_NOTIFY_STOP_MASK) == FSREPORT_NOTIFY_STOP_MASK)
        {
            printk(KERN_ERR"[NFB][ERROR][%pF][v:0x%lx] return ret[%d] errno[%d]!!!!!!!!\n"
                , nb->notifier_call, val, ret, fsreport_notifier_to_errno(ret));
        }
        if ((ret & FSREPORT_NOTIFY_STOP_MASK) == FSREPORT_NOTIFY_STOP_MASK)
            break;
        nb = next_nb;
        nr_to_call--;
    }
    return ret;
}


/**@fn      fsreport_notifier_register
 * @param   [in]hnb: 通知链处理函数
 * @param   [out]N/A
 * @return  errno on Linux
 */
int fsreport_notifier_register(struct fsreport_notifier_block *hnb)
{
    int ret = 0;

    mutex_lock(&fsreport_nf_mutex);
    ret = fsreport_notify_register(&fsreport_nf_head, hnb);
    mutex_unlock(&fsreport_nf_mutex);
    return ret;
}
EXPORT_SYMBOL(fsreport_notifier_register);

/**@fn      fsreport_notifier_unregister
 * @brief   在hikbaseal中注销通知链
 * @param   [in]hnb: 通知链处理函数
 * @param   [out]N/A
 * @return  errno on Linux
 */
int fsreport_notifier_unregister(struct fsreport_notifier_block *hnb)
{
    int ret = 0;

    if (hnb == NULL)
    {
        return 0;
    }
    mutex_lock(&fsreport_nf_mutex);
    ret = fsreport_notify_unregister(&fsreport_nf_head, hnb);
    mutex_unlock(&fsreport_nf_mutex);
    return ret;
}
EXPORT_SYMBOL(fsreport_notifier_unregister);



/**@fn      fsreport_notifiers_call
 * @brief  用于向注册到通知链上的产生通知信号
 * @param   [in]val:    指定的通知事件值
 * @param   [in]dev:    指定的通知事件参数
 * @param   [out]N/A
 * @return  errno on Linux
 */
int fsreport_notifiers_call(unsigned long val, void *data)
{
    return fsreport_call(&fsreport_nf_head, val, data, -1, NULL);
}
EXPORT_SYMBOL(fsreport_notifiers_call);

/**@fn      fsreport_notifiers_callv2
 * @brief   用于向注册到通知链上的产生通知信号；参数可扩展
 * @param   [in]val:    指定的通知事件值
 * @param   [in]args:   紧跟后面参数个数；后面紧跟的参数类型为void*类型
 * @param   [out]N/A
 * @return  errno on Linux
 */
int fsreport_notifiers_callv2(unsigned long val, unsigned int args, ...)
{
    struct fsreport_notifiter_args arg = {0};
    va_list vargs;
    int i   = 0;

    if  (args > FSREPORT_NF_ARGS_NR)
        return -EINVAL;

    arg.num     = args;
    va_start(vargs, args);
    for (i = 0; i < args; i++)
        arg.arg[i] = va_arg(vargs, void*);
    va_end(vargs);

    return fsreport_call(&fsreport_nf_head, val, &arg, -1, NULL);
}
EXPORT_SYMBOL(fsreport_notifiers_callv2);

















