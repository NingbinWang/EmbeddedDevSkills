# 设备模型参考

## sysfs 属性

### 创建设备属性

```c
#include <linux/device.h>

/* 只读属性 */
static ssize_t mydrv_status_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
    struct mydrv_priv *priv = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%s\n", priv->is_running ? "running" : "stopped");
}
static DEVICE_ATTR_RO(status);  /* 生成 dev_attr_status */

/* 读写属性 */
static ssize_t mydrv_mode_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
    struct mydrv_priv *priv = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%d\n", priv->mode);
}

static ssize_t mydrv_mode_store(struct device *dev,
                                 struct device_attribute *attr,
                                 const char *buf, size_t count)
{
    struct mydrv_priv *priv = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    if (val < 0 || val > 3)
        return -EINVAL;

    mutex_lock(&priv->lock);
    priv->mode = val;
    mutex_unlock(&priv->lock);

    return count;
}
static DEVICE_ATTR_RW(mode);  /* 生成 dev_attr_mode */

/* 只写属性 */
static ssize_t mydrv_reset_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    struct mydrv_priv *priv = dev_get_drvdata(dev);
    mydrv_hw_reset(priv);
    return count;
}
static DEVICE_ATTR_WO(reset);
```

### 注册属性组

```c
static struct attribute *mydrv_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_mode.attr,
    &dev_attr_reset.attr,
    NULL,
};

static const struct attribute_group mydrv_attr_group = {
    .attrs = mydrv_attrs,
};

/* 在 probe 中创建 */
ret = sysfs_create_group(&pdev->dev.kobj, &mydrv_attr_group);

/* 在 remove 中删除 */
sysfs_remove_group(&pdev->dev.kobj, &mydrv_attr_group);
```

## kobject

### 独立 kobject（非设备关联）

```c
#include <linux/kobject.h>
#include <linux/sysfs.h>

static struct kobject *my_kobj;

static ssize_t my_show(struct kobject *kobj, struct kobj_attribute *attr,
                       char *buf)
{
    return sysfs_emit(buf, "hello\n");
}

static struct kobj_attribute my_attr = __ATTR_RO(my);

/* 创建 /sys/kernel/my_kobj */
my_kobj = kobject_create_and_add("my_kobj", kernel_kobj);
if (!my_kobj)
    return -ENOMEM;

ret = sysfs_create_file(my_kobj, &my_attr.attr);

/* 清理 */
sysfs_remove_file(my_kobj, &my_attr.attr);
kobject_put(my_kobj);
```

## proc 文件系统

```c
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* 使用 seq_file（推荐，处理大输出） */
static int mydrv_proc_show(struct seq_file *m, void *v)
{
    struct mydrv_priv *priv = m->private;

    seq_printf(m, "Status: %s\n", priv->is_running ? "running" : "stopped");
    seq_printf(m, "Mode: %d\n", priv->mode);
    seq_printf(m, "Errors: %lu\n", priv->error_count);

    return 0;
}

static int mydrv_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, mydrv_proc_show, PDE_DATA(inode));
}

static const struct proc_ops mydrv_proc_ops = {
    .proc_open    = mydrv_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* 创建 /proc/mydrv */
proc_create_data("mydrv", 0444, NULL, &mydrv_proc_ops, priv);

/* 删除 */
remove_proc_entry("mydrv", NULL);
```

## debugfs

```c
#include <linux/debugfs.h>

static struct dentry *debug_dir;

/* 在 probe 中创建 */
debug_dir = debugfs_create_dir("mydrv", NULL);

/* 创建调试文件 */
debugfs_create_u32("mode", 0644, debug_dir, &priv->mode);
debugfs_create_bool("running", 0644, debug_dir, &priv->is_running);
debugfs_create_x32("reg_dump", 0444, debug_dir, &priv->reg_shadow);

/* 自定义读写 */
static const struct file_operations debug_fops = {
    .open    = simple_open,
    .read    = debug_read,
    .write   = debug_write,
    .llseek  = default_llseek,
};
debugfs_create_file("control", 0644, debug_dir, priv, &debug_fops);

/* 清理（debugfs 在设备移除时自动清理，但显式删除更安全） */
debugfs_remove_recursive(debug_dir);
```

## 设备模型层次

```
/sys/
├── bus/
│   ├── platform/
│   │   ├── drivers/
│   │   │   └── my-driver/
│   │   │       ├── bind
│   │   │       ├── unbind
│   │   │       └── my_device -> ../../../../devices/.../my_device
│   │   └── devices/
│   │       └── my_device -> ../../devices/.../my_device
│   └── i2c/  (或 spi, usb, pci 等)
├── class/
│   └── my_class/
│       └── my_device -> ../../devices/.../my_device
├── devices/
│   └── platform/
│       └── my_device/
│           ├── driver -> ../../../../bus/platform/drivers/my-driver
│           ├── power/
│           ├── subsystem -> ../../../../bus/platform
│           └── my_attr  (sysfs 属性)
└── kernel/
    └── my_kobj/  (独立 kobject)
```

## 创建设备类和设备节点（自动创建 /dev/xxx）

```c
#include <linux/device.h>

static struct class *my_class;

/* 在模块初始化时 */
my_class = class_create("my_class");
if (IS_ERR(my_class))
    return PTR_ERR(my_class);

/* 在 probe 中创建设备节点 */
struct device *dev = device_create(my_class, &pdev->dev,
                                    MKDEV(major, minor), priv, "mydev%d", 0);
if (IS_ERR(dev))
    return PTR_ERR(dev);

/* 在 remove 中销毁 */
device_destroy(my_class, MKDEV(major, minor));

/* 在模块退出时 */
class_destroy(my_class);
```

## 常见陷阱

1. **sysfs 属性的 show 必须返回 sysfs_emit 的返回值** — 不是 snprintf
2. **sysfs 属性的 store 必须返回 count** — 表示成功消费了多少字节
3. **debugfs 可能在生产内核中被禁用** — 不要在关键路径依赖 debugfs
4. **proc 文件大输出必须用 seq_file** — 不要用 sprintf 直接写（可能溢出缓冲区）
5. **device_create 失败必须检查 IS_ERR** — 不是返回 NULL
