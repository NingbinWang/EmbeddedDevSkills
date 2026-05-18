# 块设备驱动参考

## 基本框架

```c
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>

#define DEVICE_NAME "mybdev"
#define SECTOR_SIZE 512
#define NR_SECTORS  (1024 * 1024)  /* 512MB */

struct mybdev_priv {
    struct gendisk *gd;
    struct blk_mq_tag_set tag_set;
    void *data;          /* 后端存储（内存模拟） */
    size_t size;
};
```

## blk-mq（现代多队列框架）

### 定义操作

```c
/* 处理单个请求 */
static blk_status_t mybdev_queue_rq(struct blk_mq_hw_ctx *hctx,
                                     const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct mybdev_priv *priv = rq->q->queuedata;
    struct bio_vec bvec;
    struct req_iterator iter;
    sector_t sector = blk_rq_pos(rq);
    size_t offset = sector * SECTOR_SIZE;
    size_t bytes;

    blk_mq_start_request(rq);

    rq_for_each_segment(bvec, rq, iter) {
        bytes = bvec.bv_len;
        void *buf = page_address(bvec.bv_page) + bvec.bv_offset;

        if (rq_data_dir(rq) == READ)
            memcpy(buf, priv->data + offset, bytes);
        else
            memcpy(priv->data + offset, buf, bytes);

        offset += bytes;
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops mybdev_mq_ops = {
    .queue_rq = mybdev_queue_rq,
};
```

### 设置标签集

```c
static const struct block_device_operations mybdev_fops = {
    .owner = THIS_MODULE,
};

static int mybdev_setup(struct mybdev_priv *priv)
{
    int ret;

    /* 分配标签集 */
    priv->tag_set.ops = &mybdev_mq_ops;
    priv->tag_set.nr_hw_queues = 1;
    priv->tag_set.queue_depth = 128;
    priv->tag_set.numa_node = NUMA_NO_NODE;
    priv->tag_set.cmd_size = 0;
    priv->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
    priv->tag_set.driver_data = priv;

    ret = blk_mq_alloc_tag_set(&priv->tag_set);
    if (ret)
        return ret;

    /* 分配 gendisk */
    priv->gd = blk_mq_alloc_disk(&priv->tag_set, priv);
    if (IS_ERR(priv->gd)) {
        ret = PTR_ERR(priv->gd);
        goto err_tag;
    }

    priv->gd->major = 0;  /* 动态分配 */
    priv->gd->first_minor = 0;
    priv->gd->minors = 1;
    priv->gd->fops = &mybdev_fops;
    priv->gd->private_data = priv;
    snprintf(priv->gd->disk_name, 32, DEVICE_NAME);
    set_capacity(priv->gd, NR_SECTORS);

    /* 添加磁盘 */
    ret = add_disk(priv->gd);
    if (ret)
        goto err_disk;

    return 0;

err_disk:
    put_disk(priv->gd);
err_tag:
    blk_mq_free_tag_set(&priv->tag_set);
    return ret;
}
```

## proc/sysfs 接口

```c
/* 通过 sysfs 暴露磁盘信息 */
/* gendisk 自动在 /sys/block/<name>/ 下创建属性 */

/* 自定义属性 */
static ssize_t mybdev_backend_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    struct gendisk *gd = dev_to_disk(dev);
    struct mybdev_priv *priv = gd->private_data;
    return sysfs_emit(buf, "%s\n", "memory");
}
static DEVICE_ATTR_RO(mybdev_backend);
```

## 分区支持

```c
/* 设置分区数（minors） */
priv->gd->minors = 16;  /* 支持最多 15 个分区 */

/* 不支持分区 */
priv->gd->minors = 1;
/* 或在 block_device_operations 中设置 */
```

## 请求队列属性

```c
struct request_queue *q = priv->gd->queue;

/* 设置最大扇区数 */
blk_queue_max_hw_sectors(q, 256);

/* 设置逻辑/物理块大小 */
blk_queue_logical_block_size(q, 512);
blk_queue_physical_block_size(q, 4096);

/* 设置最大段数 */
blk_queue_max_segments(q, 128);

/* 设置最大段大小 */
blk_queue_max_segment_size(q, 65536);

/* 设置旋转标志（SSD 设为 0） */
blk_queue_flag_set(QUEUE_FLAG_NONROT, q);
```

## BIO 直接处理（高级）

```c
/* 如果需要绕过请求队列，直接处理 BIO */
static void mybdev_submit_bio(struct bio *bio)
{
    struct mybdev_priv *priv = bio->bi_bdev->bd_disk->private_data;
    struct bio_vec bvec;
    struct bvec_iter iter;
    sector_t sector = bio->bi_iter.bi_sector;

    bio_for_each_segment(bvec, bio, iter) {
        void *buf = page_address(bvec.bv_page) + bvec.bv_offset;
        size_t offset = sector * SECTOR_SIZE;
        size_t len = bvec.bv_len;

        if (bio_data_dir(bio) == READ)
            memcpy(buf, priv->data + offset, len);
        else
            memcpy(priv->data + offset, buf, len);

        sector += len / SECTOR_SIZE;
    }

    bio_endio(bio);
}

static const struct block_device_operations mybdev_fops = {
    .owner     = THIS_MODULE,
    .submit_bio = mybdev_submit_bio,
};
```

## 清理

```c
static void mybdev_cleanup(struct mybdev_priv *priv)
{
    del_gendisk(priv->gd);
    put_disk(priv->gd);
    blk_mq_free_tag_set(&priv->tag_set);
    vfree(priv->data);
}
```

## 调试

```bash
# 查看块设备
lsblk
cat /proc/partitions

# 创建文件系统并挂载
sudo mkfs.ext4 /dev/mybdev
sudo mount /dev/mybdev /mnt

# 查看块设备属性
cat /sys/block/mybdev/queue/scheduler
cat /sys/block/mybdev/queue/nr_requests
```

## 常见陷阱

1. **blk-mq 是唯一选择** — 旧的 request_fn 接口已在内核 5.x+ 移除
2. **add_disk 失败必须回滚** — 先 put_disk 再 blk_mq_free_tag_set
3. **扇区号是 512 字节单位** — 即使物理块更大，sector 也按 512B 计
4. **gendisk 引用计数** — add_disk 后通过 del_gendisk 移除
5. **submit_bio 必须调用 bio_endio** — 否则上层永远等待
