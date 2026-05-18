/*
 * 块设备驱动模板（内存模拟）
 * 编译: make
 * 加载: sudo insmod block_device.ko
 * 测试: sudo mkfs.ext4 /dev/mybdev && sudo mount /dev/mybdev /mnt
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>

#define DEVICE_NAME  "mybdev"
#define SECTOR_SIZE  512
#define NR_SECTORS   (1024 * 256)  /* 128MB */

struct mybdev_priv {
    struct gendisk *gd;
    struct blk_mq_tag_set tag_set;
    void *data;
};

static int mybdev_major;

static blk_status_t mybdev_queue_rq(struct blk_mq_hw_ctx *hctx,
                                     const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct mybdev_priv *priv = rq->q->queuedata;
    struct bio_vec bvec;
    struct req_iterator iter;
    sector_t sector = blk_rq_pos(rq);

    blk_mq_start_request(rq);

    rq_for_each_segment(bvec, rq, iter) {
        void *buf = page_address(bvec.bv_page) + bvec.bv_offset;
        size_t offset = sector * SECTOR_SIZE;
        size_t len = bvec.bv_len;

        if (rq_data_dir(rq) == READ)
            memcpy(buf, priv->data + offset, len);
        else
            memcpy(priv->data + offset, buf, len);

        sector += len / SECTOR_SIZE;
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops mybdev_mq_ops = {
    .queue_rq = mybdev_queue_rq,
};

static const struct block_device_operations mybdev_fops = {
    .owner = THIS_MODULE,
};

static struct mybdev_priv *g_priv;

static int __init mybdev_init(void)
{
    struct mybdev_priv *priv;
    int ret;

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) return -ENOMEM;
    g_priv = priv;

    priv->data = vmalloc(NR_SECTORS * SECTOR_SIZE);
    if (!priv->data) { kfree(priv); return -ENOMEM; }

    priv->tag_set.ops = &mybdev_mq_ops;
    priv->tag_set.nr_hw_queues = 1;
    priv->tag_set.queue_depth = 128;
    priv->tag_set.numa_node = NUMA_NO_NODE;
    priv->tag_set.cmd_size = 0;
    priv->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
    priv->tag_set.driver_data = priv;

    ret = blk_mq_alloc_tag_set(&priv->tag_set);
    if (ret) goto err_vfree;

    priv->gd = blk_mq_alloc_disk(&priv->tag_set, priv);
    if (IS_ERR(priv->gd)) { ret = PTR_ERR(priv->gd); goto err_tag; }

    priv->gd->major = mybdev_major;
    priv->gd->first_minor = 0;
    priv->gd->minors = 1;
    priv->gd->fops = &mybdev_fops;
    priv->gd->private_data = priv;
    snprintf(priv->gd->disk_name, 32, DEVICE_NAME);
    set_capacity(priv->gd, NR_SECTORS);

    ret = add_disk(priv->gd);
    if (ret) goto err_disk;

    pr_info(DEVICE_NAME ": loaded, %luMB\n", (unsigned long)(NR_SECTORS * SECTOR_SIZE / 1048576));
    return 0;

err_disk: put_disk(priv->gd);
err_tag:  blk_mq_free_tag_set(&priv->tag_set);
err_vfree: vfree(priv->data); kfree(priv);
    return ret;
}

static void __exit mybdev_exit(void)
{
    struct mybdev_priv *priv = g_priv;
    del_gendisk(priv->gd);
    put_disk(priv->gd);
    blk_mq_free_tag_set(&priv->tag_set);
    vfree(priv->data);
    kfree(priv);
}

module_init(mybdev_init);
module_exit(mybdev_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple block device driver template (memory-backed)");
