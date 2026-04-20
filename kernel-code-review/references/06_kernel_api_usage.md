# 代码审查技能文件 - 内核API使用规范

本文档例举Linux C(内核与驱动)安全编码规范中内核API及设备模型相关条款。

## 一、内核API与设备模型

### 1.1 避免在内核中使用浮点数运算

**【描述】**
Linux内核默认不保存FPU状态，如果强行在内核运行浮点运算会导致协处理器状态被破坏。如非必须（且手动通过 `kernel_fpu_begin()` 和 `kernel_fpu_end()` 包围的话），驱动和内核通常禁止使用 `float` / `double` 类型和操作。

**【正确代码示例】**
尽量将其转换为定点数或者纯整数运算。

### 1.2 正确使用 `printk` 日志级别和限速

**【描述】**
不要在可能频繁触发的地方直接使用普通的 `printk` / `pr_err` / `dev_err`，这可能会导致系统日志洪水 (log flood)。

**【正确代码示例】**
```c
// 使用带速率限制的日志打印
dev_err_ratelimited(dev, "Error occurred\n");
// 或者是
pr_err_ratelimited("System error\n");
```

### 1.3 `kmalloc` vs `vmalloc` 的选择

**【描述】**
- 当需要在物理上完全连续的内存区域（例如用于某些DMA操作）或较小的内存分配时，请使用 `kmalloc`。频繁的大块 `kmalloc` 容易遇到碎片化导致分配失败。
- 如果只需要虚拟地址连续，且大小大于 1 页（4KB），更倾向于使用 `vmalloc`，或考虑使用 `kvmalloc`，它会首选 `kmalloc`，若失败再回退到 `vmalloc`。

### 1.4 不要假定 `sysfs` 的读取属性中字符串一定有 NULL 终止

**【描述】**
通过 `sysfs` 回调函数如 `store()` 接收的用户输入 buf ，不一定是保证 `\0` 结尾的。

**【正确代码示例】**
使用 `sysfs_match_string` 或者显式限制边界 `strnlen(buf, count)` 或者将最后一个字符强制设置为 `\0` 来确保安全。 

### 1.5 DMA 操作与 Cache 同步

**【描述】**
如果驱动存在 DMA 操作，请使用 `dma_alloc_coherent` 获取一致性内存，或者在每次 DMA 传输前后调用 `dma_map_single` / `dma_unmap_single` / `dma_sync_single_for_cpu` 等API手动进行Cache同步，禁止直接传递裸物理地址做DMA。
