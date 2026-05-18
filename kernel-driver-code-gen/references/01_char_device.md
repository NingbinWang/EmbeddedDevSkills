# 字符设备驱动参考

## 基本框架

### 动态分配设备号

```c
#include <linux/cdev.h>
#include <linux/fs.h>

#define DEVICE_NAME "mydev"
#define MINOR_COUNT 1

static dev_t devno;
static struct cdev my_cdev;

static int __init mydev_init(void)
{
    int ret;

    /* 动态分配设备号 */
    ret = alloc_chrdev_region(&devno, 0, MINOR_COUNT, DEVICE_NAME);
    if (ret) {
        pr_err("mydev: failed to allocate device number\n");
        return ret;
    }

    pr_info("mydev: major=%d, minor=%d\n", MAJOR(devno), MINOR(devno));

    /* 初始化 cdev */
    cdev_init(&my_cdev, &mydev_fops);
    my_cdev.owner = THIS_MODULE;

    /* 注册 cdev */
    ret = cdev_add(&my_cdev, devno, MINOR_COUNT);
    if (ret) {
        pr_err("mydev: failed to add cdev\n");
        goto err_cdev;
    }

    return 0;

err_cdev:
    unregister_chrdev_region(devno, MINOR_COUNT);
    return ret;
}

static void __exit mydev_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(devno, MINOR_COUNT);
}
```

### 静态分配设备号

```c
/* 已知主设备号时使用 */
#define MY_MAJOR 240

ret = register_chrdev_region(MKDEV(MY_MAJOR, 0), MINOR_COUNT, DEVICE_NAME);
```

## file_operations 实现

### open / release

```c
static int mydev_open(struct inode *inode, struct file *filp)
{
    struct mydev_priv *priv;

    /* 从 inode 获取 cdev，再获取私有数据 */
    priv = container_of(inode->i_cdev, struct mydev_priv, cdev);
    filp->private_data = priv;

    /* 检查是否已打开（排他打开等） */
    if (priv->is_open)
        return -EBUSY;
    priv->is_open = true;

    return 0;
}

static int mydev_release(struct inode *inode, struct file *filp)
{
    struct mydev_priv *priv = filp->private_data;
    priv->is_open = false;
    return 0;
}
```

### read / write

```c
static ssize_t mydev_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    struct mydev_priv *priv = filp->private_data;
    ssize_t ret;

    /* 参数检查 */
    if (*f_pos >= priv->data_size)
        return 0;  /* EOF */

    if (*f_pos + count > priv->data_size)
        count = priv->data_size - *f_pos;

    /* 拷贝到用户空间 */
    if (copy_to_user(buf, priv->data + *f_pos, count))
        return -EFAULT;

    *f_pos += count;
    ret = count;

    return ret;
}

static ssize_t mydev_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct mydev_priv *priv = filp->private_data;

    if (count > BUFFER_SIZE)
        return -EINVAL;

    if (copy_from_user(priv->data, buf, count))
        return -EFAULT;

    priv->data_size = count;
    *f_pos += count;

    return count;
}
```

### unlocked_ioctl

```c
#include <linux/uaccess.h>

/* ioctl 命令定义 */
#define MYDEV_MAGIC     'M'
#define MYDEV_RESET     _IO(MYDEV_MAGIC, 0)
#define MYDEV_GET_SIZE  _IOR(MYDEV_MAGIC, 1, int)
#define MYDEV_SET_SIZE  _IOW(MYDEV_MAGIC, 2, int)
#define MYDEV_RW_DATA   _IOWR(MYDEV_MAGIC, 3, struct mydev_data)

static long mydev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct mydev_priv *priv = filp->private_data;
    int size;

    /* 验证 magic number */
    if (_IOC_TYPE(cmd) != MYDEV_MAGIC)
        return -ENOTTY;

    switch (cmd) {
    case MYDEV_RESET:
        priv->data_size = 0;
        break;

    case MYDEV_GET_SIZE:
        size = priv->data_size;
        if (copy_to_user((int __user *)arg, &size, sizeof(size)))
            return -EFAULT;
        break;

    case MYDEV_SET_SIZE:
        if (copy_from_user(&size, (int __user *)arg, sizeof(size)))
            return -EFAULT;
        if (size < 0 || size > BUFFER_SIZE)
            return -EINVAL;
        priv->data_size = size;
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}
```

### poll（等待事件）

```c
#include <linux/poll.h>

static __poll_t mydev_poll(struct file *filp, poll_table *wait)
{
    struct mydev_priv *priv = filp->private_data;
    __poll_t mask = 0;

    /* 注册等待队列 */
    poll_wait(filp, &priv->wq, wait);

    /* 检查条件 */
    if (priv->data_ready)
        mask |= POLLIN | POLLRDNORM;  /* 可读 */
    if (priv->writable)
        mask |= POLLOUT | POLLWRNORM; /* 可写 */

    return mask;
}
```

### mmap

```c
static int mydev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct mydev_priv *priv = filp->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > priv->dma_size)
        return -EINVAL;

    /* 将 DMA 缓冲区映射到用户空间 */
    if (dma_mmap_coherent(priv->dev, vma, priv->dma_buf,
                          priv->dma_addr, size))
        return -ENXIO;

    return 0;
}
```

## 使用 devm 自动资源管理

```c
/* devm_ 版本的资源在设备移除时自动释放 */
static int mydev_probe(struct platform_device *pdev)
{
    struct mydev_priv *priv;

    /* 自动释放的内存分配 */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* 自动释放的字符设备注册 */
    ret = devm_cdev_add(&pdev->dev, &priv->cdev, devno, 1);

    /* 自动释放的中断注册 */
    ret = devm_request_irq(&pdev->dev, irq, mydev_isr, 0, "mydev", priv);

    /* 自动释放的 I/O 映射 */
    priv->regs = devm_ioremap_resource(&pdev->dev, res);

    /* probe 失败时所有 devm_ 资源自动释放 */
    return 0;
}
```

## 常见陷阱

1. **copy_to_user / copy_from_user 必须检查返回值** — 返回非零表示有不可访问的用户地址
2. **ioctl 命令定义** — 使用 _IO/_IOR/_IOW/_IOWR 宏，不要手动拼数字
3. **poll_wait 不阻塞** — 只是注册等待队列，实际阻塞由 VFS 层处理
4. **mmap 的 vm_ops** — 需要设置 vm_ops.fault 处理缺页（如果是非连续内存）
5. **cdev_del 必须在 unregister_chrdev_region 之前** — 否则可能有竞态
