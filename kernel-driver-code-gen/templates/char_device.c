/*
 * 字符设备驱动模板
 * 编译: make
 * 加载: sudo insmod char_device.ko
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 1024

struct mydev_priv {
    struct cdev cdev;
    struct device *device;
    struct class *class;
    dev_t devno;
    char buffer[BUFFER_SIZE];
    size_t data_size;
    bool is_open;
    struct mutex lock;
};

static struct mydev_priv *g_priv;

static int mydev_open(struct inode *inode, struct file *filp) {
    struct mydev_priv *priv = container_of(inode->i_cdev, struct mydev_priv, cdev);
    if (priv->is_open) return -EBUSY;
    priv->is_open = true; filp->private_data = priv; return 0;
}

static int mydev_release(struct inode *inode, struct file *filp) {
    struct mydev_priv *priv = filp->private_data;
    priv->is_open = false; return 0;
}

static ssize_t mydev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct mydev_priv *priv = filp->private_data; ssize_t ret;
    mutex_lock(&priv->lock);
    if (*f_pos >= priv->data_size) { ret = 0; goto out; }
    if (*f_pos + count > priv->data_size) count = priv->data_size - *f_pos;
    if (copy_to_user(buf, priv->buffer + *f_pos, count)) { ret = -EFAULT; goto out; }
    *f_pos += count; ret = count;
out: mutex_unlock(&priv->lock); return ret;
}

static ssize_t mydev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct mydev_priv *priv = filp->private_data; ssize_t ret;
    mutex_lock(&priv->lock);
    if (count > BUFFER_SIZE) { ret = -EINVAL; goto out; }
    if (copy_from_user(priv->buffer, buf, count)) { ret = -EFAULT; goto out; }
    priv->data_size = count; *f_pos += count; ret = count;
out: mutex_unlock(&priv->lock); return ret;
}

static const struct file_operations mydev_fops = {
    .owner = THIS_MODULE, .open = mydev_open, .release = mydev_release,
    .read = mydev_read, .write = mydev_write,
};

static int __init mydev_init(void) {
    struct mydev_priv *priv; int ret;
    priv = kzalloc(sizeof(*priv), GFP_KERNEL); if (!priv) return -ENOMEM;
    g_priv = priv; mutex_init(&priv->lock);
    ret = alloc_chrdev_region(&priv->devno, 0, 1, DEVICE_NAME); if (ret) goto err_free;
    cdev_init(&priv->cdev, &mydev_fops); priv->cdev.owner = THIS_MODULE;
    ret = cdev_add(&priv->cdev, priv->devno, 1); if (ret) goto err_unreg;
    priv->class = class_create(DEVICE_NAME);
    if (IS_ERR(priv->class)) { ret = PTR_ERR(priv->class); goto err_cdev; }
    priv->device = device_create(priv->class, NULL, priv->devno, NULL, DEVICE_NAME);
    if (IS_ERR(priv->device)) { ret = PTR_ERR(priv->device); goto err_class; }
    pr_info(DEVICE_NAME ": loaded (major=%d)\n", MAJOR(priv->devno)); return 0;
err_class: class_destroy(priv->class);
err_cdev: cdev_del(&priv->cdev);
err_unreg: unregister_chrdev_region(priv->devno, 1);
err_free: kfree(priv); return ret;
}

static void __exit mydev_exit(void) {
    struct mydev_priv *priv = g_priv;
    device_destroy(priv->class, priv->devno); class_destroy(priv->class);
    cdev_del(&priv->cdev); unregister_chrdev_region(priv->devno, 1); kfree(priv);
}

module_init(mydev_init); module_exit(mydev_exit);
MODULE_LICENSE("GPL"); MODULE_DESCRIPTION("Simple character device driver template");
