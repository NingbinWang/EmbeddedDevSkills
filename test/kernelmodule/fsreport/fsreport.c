// SPDX-License-Identifier: GPL-2.0

#include <linux/device.h>
#include <linux/math64.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/string.h>
#include "fsreport.h"
#include "fsreport_notify.h" 

 #define MAX_DATALEN 256
 typedef struct {
	  int event;
	  int fclus;//出错的簇号
	  int post;
	  char devid[MAX_DATALEN];//那个分区
 }FSREPORT_DATA_T;


 struct fsreport_dev {
	 spinlock_t report_lock;
	 FSREPORT_DATA_T info;
	 pid_t pid;
 };


 struct fsreport_dev *pfsreport_dev=NULL;




 
 static const struct genl_multicast_group fsreport_genl_mcgrps[] = {
     { .name = FS_GENL_REPORT_GROUP_NAME, },
 };
 
 static struct genl_family fsreport_genl_family __ro_after_init = {
     .module = THIS_MODULE,
     .name = FS_GENL_FAMILY_NAME,
     .version = FS_GENL_VERSION,
     .maxattr = FS_GENL_ATTR_MAX,
     .mcgrps = fsreport_genl_mcgrps,
     .n_mcgrps = ARRAY_SIZE(fsreport_genl_mcgrps),
 };
 

 int fsreport_genl_send_msg(FSREPORT_DATA_T *data)
 {
     struct sk_buff *skb;
     struct nlattr *attr;
    FSREPORT_DATA_T *_msg;
     void *msg_header;
     int size;
     int result;
     static u32 event_seqnum;
     if (!data)
         return -EINVAL;
     size = nla_total_size(sizeof(FSREPORT_DATA_T)) + nla_total_size(0);
     skb = genlmsg_new(size, GFP_ATOMIC);
     if (!skb)
         return -ENOMEM;
 
     msg_header = genlmsg_put(skb, 0, event_seqnum++, 
                 &fsreport_genl_family, 0, FS_GENL_CMD_NOTIFY);
     if (!msg_header) {
         nlmsg_free(skb);
         return -ENOMEM;
     }
     attr = nla_reserve(skb, FS_GENL_ATTR_NOTIFY, sizeof(FSREPORT_DATA_T));
     if (!attr) {
         nlmsg_free(skb);
         return -EINVAL;
     }
 
     _msg = nla_data(attr);
     if (!_msg) {
         nlmsg_free(skb);
         return -EINVAL;
     }
 
     memcpy(_msg, data, sizeof(FSREPORT_DATA_T));
 
     genlmsg_end(skb, msg_header);
 
     /* send multicast genetlink message */
     result = genlmsg_multicast(&fsreport_genl_family, skb, 0,
                     0, GFP_ATOMIC);
     return result;
 }

 /**@fn 	 fsreport_event_notifier
  * @brief	 通知事件函数
  * @param	 [in]this:	  struct fsreport_notifier_block *
  * @param	 [in]event:   具体通知的事件类型
  * @param	 [in]ptr:	  私有数据成员
  * @param	 [out]N/A
  * @return  HKBAL_NOTIFY_OK
  */
 static int fsreport_event_notifier(struct fsreport_notifier_block *this
		 , unsigned long event, void *ptr)
 {
	 int ret = 0;
	 unsigned long flags;
 
	 switch(event)
	 {
		 case FSREPORT_NF_CLUSTER_ERROR:
		 {
		 	FSREPORT_NF_DATA_T data = {0};
			memcpy((void *)&data,ptr,sizeof(FSREPORT_NF_DATA_T));
			spin_lock_irqsave(&pfsreport_dev->report_lock, flags);
		    pfsreport_dev->info.event = FS_ERROR_CLUS_BAD;
			pfsreport_dev->info.fclus =data.fclus;
			pfsreport_dev->info.post = data.post;
			strcpy(pfsreport_dev->info.devid,data.devid);
			spin_unlock_irqrestore(&pfsreport_dev->report_lock, flags);
			fsreport_genl_send_msg(&(pfsreport_dev->info));
			printk("fsreport_event_notifier send ok\n");
		 	break;
		 }
	 } 
	 return fsreport_notifier_from_errno(ret);
 }


 static struct fsreport_notifier_block fsreport_event_nb =
 {
	 .notifier_call = fsreport_event_notifier,
 };




 /* Called by blk_dev_init() */
 int __init fsreport_init(void)
 {
    int ret = 0;
	struct fsreport_dev *dev;
	dev = kzalloc(sizeof(struct fsreport_dev),
					GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	spin_lock_init(&dev->report_lock);
	pfsreport_dev = dev;
	ret = fsreport_notifier_register(&fsreport_event_nb);
    if (ret)
    {
        printk("[%s][%d]register filesystem notifier failed.\n",__func__,__LINE__);
	    return ret;
    }	
    ret = genl_register_family(&fsreport_genl_family);
    printk("fsreport_init ok\n");
	return ret ;
 }
