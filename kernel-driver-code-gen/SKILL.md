---
name: kernel-driver-code-gen
description: |
  生成Linux内核驱动代码与Kbuild/Makefile。覆盖：字符设备、平台驱动、块设备、网络设备、
  设备树绑定、中断处理、DMA、内核内存管理、设备模型(sysfs/kobject)。
  TRIGGER when: 设计文档已完成，需要生成内核驱动模块代码、编译配置、加载测试。
  关键词：内核驱动、字符设备、块设备、网络设备、platform_driver、设备树、DTS、中断、DMA、
  Kbuild、Makefile、insmod、modprobe、module_init、file_operations、ioctl、probe、remove、
  blk-mq、gendisk、net_device、sk_buff、NAPI。
---

# Linux 内核驱动代码生成

根据设计文档或用户描述，生成符合 Linux 内核编码规范的驱动代码和 Kbuild/Makefile。

## 核心原则

1. **内核规范** — 遵循 kernel coding style（Documentation/process/coding-style.rst）
2. **错误处理** — 所有内核 API 调用必须检查返回值，资源获取失败时逆序释放
3. **可移植性** — 优先使用标准内核 API，避免架构特定代码
4. **模块化** — 清晰的 probe/remove 生命周期，支持热插拔

## 前置条件

调用此技能时，需提供以下信息之一：

| 输入 | 说明 |
|------|------|
| 设计文档路径 | 读取文档后分析需求，生成代码 |
| 功能描述 | 直接描述要实现的驱动功能 |
| 硬件规格 | 寄存器地址、中断号、DMA 通道等 |

如果缺少输入，提示用户补充。至少需要知道：
- 驱动类型（字符设备 / 平台驱动 / I2C / SPI / USB 等）
- 硬件接口（寄存器映射 / 中断 / DMA）
- 用户空间接口（read/write / ioctl / sysfs）

## 工作流程

```
读取设计文档 → 确定驱动类型 → 加载对应参考文档
    → 选择模板 → 生成代码 → 自检清单 → 输出
```

### 阶段 1：需求分析

从输入中提取：

| 提取项 | 用途 |
|--------|------|
| 驱动类型 | 选择模板（见阶段2） |
| 硬件接口 | 寄存器布局、中断、DMA |
| 用户空间接口 | file_operations / sysfs / ioctl |
| 并发需求 | 锁策略（spinlock / mutex / rwlock） |
| 设备树绑定 | compatible 字符串、属性定义 |

### 阶段 2：确定驱动类型与参考文档

| 驱动类型 | 参考文档 | 模板 |
|----------|----------|------|
| 字符设备 | [references/01_char_device.md](references/01_char_device.md) | [templates/char_device.c](templates/char_device.c) |
| 平台驱动 | [references/02_platform_driver.md](references/02_platform_driver.md) | [templates/platform_driver.c](templates/platform_driver.c) |
| 块设备 | [references/08_block_device.md](references/08_block_device.md) | [templates/block_device.c](templates/block_device.c) |
| 网络设备 | [references/09_net_device.md](references/09_net_device.md) | [templates/net_device.c](templates/net_device.c) |
| 中断处理 | [references/03_interrupt_handling.md](references/03_interrupt_handling.md) | — |
| DMA/内存 | [references/04_dma_and_memory.md](references/04_dma_and_memory.md) | — |
| Kbuild | [references/05_kbuild_makefile.md](references/05_kbuild_makefile.md) | [templates/Makefile](templates/Makefile) |
| 内核 API | [references/06_kernel_apis.md](references/06_kernel_apis.md) | — |
| 设备模型 | [references/07_device_model.md](references/07_device_model.md) | — |

**MANDATORY**: 读取对应参考文件后再生成代码。

### 阶段 3：生成代码

#### 文件结构

典型内核模块文件布局：

```
mydriver/
├── Kconfig          # 配置选项（可选，用于集成到内核树）
├── Makefile         # 模块编译规则
├── mydriver.c       # 驱动主文件
├── mydriver.h       # 私有头文件（寄存器定义、数据结构）
└── mydriver_dt.h    # 设备树相关定义（可选）
```

#### 内核编码风格要点

```c
/*
 * 缩进：Tab = 8 空格
 * 行宽：80 列（警告），100 列（硬限制）
 * 命名：小写 + 下划线，避免驼峰
 * 注释：/* */ 风格（不用 //，虽然现在也接受）
 * 函数：尽量短小，每个函数做一件事
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

/* 前向声明放在文件顶部 */
static int mydriver_probe(struct platform_device *pdev);
static int mydriver_remove(struct platform_device *pdev);

/* 数据结构定义 */
struct mydriver_priv {
    struct device *dev;
    void __iomem *regs;
    int irq;
    /* ... */
};
```

#### 模块初始化/退出框架

```c
static int __init mydriver_init(void)
{
    return platform_driver_register(&mydriver_driver);
}
module_init(mydriver_init);

static void __exit mydriver_exit(void)
{
    platform_driver_unregister(&mydriver_driver);
}
module_exit(mydriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author Name");
MODULE_DESCRIPTION("Driver description");
MODULE_VERSION("1.0");
```

#### 错误处理模式（goto 清理）

```c
static int mydriver_probe(struct platform_device *pdev)
{
    struct mydriver_priv *priv;
    struct resource *res;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    platform_set_drvdata(pdev, priv);

    /* 获取资源 */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "no memory resource\n");
        return -ENODEV;
    }

    priv->regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(priv->regs))
        return PTR_ERR(priv->regs);

    /* 获取中断 */
    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
        return priv->irq;

    ret = devm_request_irq(&pdev->dev, priv->irq, mydriver_isr,
                           IRQF_SHARED, "mydriver", priv);
    if (ret) {
        dev_err(&pdev->dev, "failed to request irq: %d\n", ret);
        return ret;
    }

    /* 注册字符设备 / 创建 sysfs 等 */
    ret = mydriver_register_cdev(priv);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "probe successful\n");
    return 0;
}
```

#### file_operations 模板

```c
static const struct file_operations mydriver_fops = {
    .owner          = THIS_MODULE,
    .open           = mydriver_open,
    .release        = mydriver_close,
    .read           = mydriver_read,
    .write          = mydriver_write,
    .unlocked_ioctl = mydriver_ioctl,
    .poll           = mydriver_poll,
    .mmap           = mydriver_mmap,
};
```

### 阶段 4：生成 Makefile

```makefile
# 外部模块编译（out-of-tree）
obj-m := mydriver.o

# 多文件模块
# mydriver-objs := main.o hw.o utils.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 安装模块
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a
```

### 阶段 5：自检清单

生成代码后逐项检查：

#### 头文件与模块
- [ ] 包含必要的头文件（linux/module.h, linux/kernel.h 等）
- [ ] MODULE_LICENSE("GPL")（如需使用 GPL-only 符号）
- [ ] module_init / module_exit 定义

#### 资源管理
- [ ] 使用 devm_ 系列 API（自动释放）
- [ ] probe 失败时资源正确逆序释放
- [ ] remove 中释放所有手动申请的资源
- [ ] ioremap 配对 iounmap（devm 版自动处理）

#### 并发安全
- [ ] 共享数据有适当的锁保护
- [ ] 中断上下文中使用 spinlock（不能用 mutex）
- [ ] 持有 spinlock 时禁用中断（spin_lock_irqsave）

#### 设备树
- [ ] compatible 字符串与 DTS 绑定一致
- [ ] 使用 of_match_table 匹配设备树节点
- [ ] 使用 platform_get_resource / platform_get_irq 获取硬件资源

#### 内存
- [ ] 中断上下文用 GFP_ATOMIC
- [ ] 进程上下文用 GFP_KERNEL
- [ ] 所有 kmalloc 都检查返回值
- [ ] DMA 缓冲区使用 dma_alloc_coherent 或 dma_map_single

## 反模式清单

- **NEVER** 在中断上下文中调用可能睡眠的函数（mutex, kmalloc(GFP_KERNEL), msleep）
- **NEVER** 忘记检查 platform_get_resource / platform_get_irq 返回值
- **NEVER** 在 probe 中硬编码中断号或寄存器地址（从设备树获取）
- **NEVER** 使用 deprecated API（如 request_region, check_mem_region）
- **NEVER** 忘记 MODULE_LICENSE
- **NEVER** 在 remove 中遗漏资源释放
- **NEVER** 使用 memcpy_fromio / memcpy_toio 之外的方式访问 I/O 内存
- **NEVER** 在持有 spinlock 时调用 schedule / msleep / mutex_lock
- **NEVER** 使用 printk 不带 KERN_* 级别前缀
- **NEVER** 忽略 devm_ioremap_resource 的 IS_ERR 检查

## 输出格式

输出包含：

1. **功能说明** — 一段话描述驱动功能
2. **文件列表** — 所有生成的文件
3. **编译命令** — make 命令
4. **加载测试** — insmod / modprobe 命令
5. **设备树片段** — DTS 节点示例（如适用）
6. **完整源码** — 可直接编译的文件
7. **注意事项** — 已知限制、依赖条件
