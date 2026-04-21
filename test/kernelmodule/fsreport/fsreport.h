// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Hikvision Auto Technology Co., Ltd.
 *   Author: wsl <wangshulin@hikvision.com>
 */

 #ifndef _FS_REPORT_H_
 #define _FS_REPORT_H_
 
 #define FS_GENL_REPORT_GROUP_NAME	"report"
 #define FS_GENL_FAMILY_NAME	  	"filesystem"
 #define FS_GENL_VERSION       		0x01
 
 /* Attributes of blk_genl_family */
 enum {
     FS_GENL_ATTR_UNSPEC,
     FS_GENL_ATTR_NOTIFY,
     FS_GENL_ATTR_MSGRSV,
     __FS_GENL_ATTR_MAX,
 };
 #define FS_GENL_ATTR_MAX (__FS_GENL_ATTR_MAX - 1)
 
 /* Commands supported by the blk_genl_family */
 enum {
     FS_GENL_CMD_UNSPEC,
     FS_GENL_CMD_NOTIFY,
     FS_GENL_CMD_MSGRSV,
     __FS_GENL_CMD_MAX,
 };
 #define FS_GENL_CMD_MAX (__FS_GENL_CMD_MAX - 1)
 
 enum fs_report_type {
     FS_ERROR_TIMEOUT		              = 1, /* Timeout error */
     FS_ERROR_NO_SPACE		              = 2, /* Critical space allocation error */
     FS_ERROR_CLUS_BAD		              = 3, /* Critical space allocation error */
 };

int __init fsreport_init(void);
  
 #endif /* _BLK_NETLINK_H */
 