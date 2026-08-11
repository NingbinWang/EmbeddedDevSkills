---
name: kernel-driver-code-gen
description: |
  根据设计文档或硬件规格，生成符合 Linux 内核编码规范的驱动代码与 Kbuild/Makefile。
  覆盖字符设备、平台驱动、块设备、网络设备、中断处理、DMA、设备模型等。
triggers:
  - "生成内核驱动"
  - "写一个驱动"
  - "创建字符设备驱动"
  - "平台驱动"
  - "platform driver"
  - "块设备驱动"
  - "网络设备驱动"
  - "设备树绑定"
  - "DTS 节点"
  - "中断处理程序"
  - "DMA 驱动"
  - "Kbuild"
  - "Makefile 内核模块"
  - "insmod"
  - "file_operations"
  - "ioctl 实现"
  - "驱动 probe"
  - "module_init"
---

# Linux 内核驱动代码生成

根据设计文档或用户描述，生成符合内核编码规范的驱动代码和 Kbuild/Makefile。

## 核心原则

1. **内核规范** — 遵循 `Documentation/process/coding-style.rst`
2. **错误处理** — 所有内核 API 调用必须检查返回值，失败时逆序释放
3. **可移植性** — 优先使用标准内核 API，避免架构特定代码
4. **模块化** — 清晰的 probe/remove 生命周期，支持热插拔

## 前置条件

至少需要知道：

| 输入 | 示例 |
|------|------|
| 驱动类型 | 字符设备 / 平台驱动 / I2C / SPI / USB |
| 硬件接口 | 寄存器映射 / 中断号 / DMA 通道 |
| 用户空间接口 | read/write / ioctl / sysfs |

如果缺少输入，提示用户补充。

## 工作流

```
读取设计文档 → 确定驱动类型 → 加载对应参考文档
    → 选择模板 → 生成代码 → 自检清单 → 输出
```

### 阶段 1：需求分析

从输入中提取：驱动类型、硬件接口、用户空间接口、并发需求、设备树绑定。

### 阶段 2：驱动类型 → 参考文档 + 模板

**MANDATORY**: 读取对应参考文件后再生成代码。

| 驱动类型 | 参考文档 | 模板 |
|----------|----------|------|
| 字符设备 | [01_char_device.md](references/01_char_device.md) | [char_device.c](templates/char_device.c) |
| 平台驱动 | [02_platform_driver.md](references/02_platform_driver.md) | [platform_driver.c](templates/platform_driver.c) |
| 块设备 | [08_block_device.md](references/08_block_device.md) | [block_device.c](templates/block_device.c) |
| 网络设备 | [09_net_device.md](references/09_net_device.md) | [net_device.c](templates/net_device.c) |
| 中断处理 | [03_interrupt_handling.md](references/03_interrupt_handling.md) | — |
| DMA/内存 | [04_dma_and_memory.md](references/04_dma_and_memory.md) | — |
| Kbuild | [05_kbuild_makefile.md](references/05_kbuild_makefile.md) | [Makefile](templates/Makefile) |
| 内核 API | [06_kernel_apis.md](references/06_kernel_apis.md) | — |
| 设备模型 | [07_device_model.md](references/07_device_model.md) | — |

### 阶段 3：生成代码

**文件结构**：
```
mydriver/
├── Kconfig       # 配置选项（可选）
├── Makefile      # 模块编译规则
├── mydriver.c    # 驱动主文件
├── mydriver.h    # 私有头文件
└── mydriver_dt.h # 设备树定义（可选）
```

**关键编码规范**：Tab=8空格、行宽≤100列、小写+下划线命名、`/* */` 注释风格。

**模块框架**：
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init mydriver_init(void) { return platform_driver_register(&mydriver_driver); }
module_init(mydriver_init);

static void __exit mydriver_exit(void) { platform_driver_unregister(&mydriver_driver); }
module_exit(mydriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("Driver description");
```

**错误处理** — 使用 goto 集中清理路径，优先 devm_ 系列 API 自动释放资源。

### 阶段 4：自检清单

- 头文件完整（linux/module.h 等）
- MODULE_LICENSE("GPL")
- module_init / module_exit 定义
- 使用 devm_ 系列 API 自动管理资源
- kmalloc 返回值检查（中断上下文用 GFP_ATOMIC）
- 共享数据有适当锁保护（中断上下文不能使用 mutex）
- 持有 spinlock 时用 spin_lock_irqsave
- compatible 字符串与 DTS 一致
- DMA 缓冲区使用 dma_alloc_coherent 或 dma_map_single
- 无 deprecated API（request_region 等）

## 反模式清单

- **NEVER** 中断上下文调用睡眠函数（mutex, kmalloc(GFP_KERNEL), msleep）
- **NEVER** 忘记检查 platform_get_resource / platform_get_irq 返回值
- **NEVER** 硬编码中断号或寄存器地址（应从设备树获取）
- **NEVER** 使用 deprecated API
- **NEVER** 忘记 MODULE_LICENSE
- **NEVER** 持有 spinlock 时调用 schedule / msleep / mutex_lock
- **NEVER** 使用 printk 不带 KERN_* 前缀
- **NEVER** 忽略 devm_ioremap_resource 的 IS_ERR 检查

## 输出格式

1. **功能说明** — 驱动功能描述
2. **文件列表** — 所有生成的文件
3. **编译命令** — make 命令
4. **加载测试** — insmod / modprobe / rmmod
5. **设备树片段** — DTS 节点示例（如适用）
6. **完整源码** — 可直接编译
7. **注意事项** — 已知限制、依赖条件
