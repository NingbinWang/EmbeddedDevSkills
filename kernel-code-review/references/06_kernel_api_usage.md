# 代码审查技能文件 - 内核API使用规范

本文档覆盖 Linux 内核常用 API 的安全使用规范，包括内存、设备模型、DMA 等。

## 一、内存分配

### 1.1 kmalloc 返回值必须检查

**【描述】** `kmalloc` / `kzalloc` / `vmalloc` / `kvmalloc` 可能返回 NULL。

**【错误代码】**
```c
void *buf = kmalloc(size, GFP_KERNEL);
memcpy(buf, src, size);  // 错误：未检查 NULL
```

**【正确代码】**
```c
void *buf = kmalloc(size, GFP_KERNEL);
if (!buf)
    return -ENOMEM;
```

### 1.2 kmalloc_array / kcalloc 防溢出

**【描述】** 数组分配用 `kmalloc_array(n, size, GFP)` 或 `kcalloc(n, size, GFP)`，不要手动 `n * size`。

**【错误代码】**
```c
int *arr = kmalloc(count * sizeof(int), GFP_KERNEL);  // count * sizeof(int) 可能溢出
```

**【正确代码】**
```c
int *arr = kmalloc_array(count, sizeof(int), GFP_KERNEL);
```

### 1.3 kmalloc vs vmalloc vs kvmalloc

| API | 物理连续 | 虚拟连续 | 适用场景 |
|-----|----------|----------|----------|
| kmalloc | ✓ | ✓ | 小块（< 页大小），DMA 缓冲区 |
| vmalloc | ✗ | ✓ | 大块（> 页大小），非 DMA |
| kvmalloc | 先尝试 kmalloc | 回退 vmalloc | 大小不确定 |

### 1.4 避免在内核使用浮点

**【描述】** 内核默认不保存 FPU 状态，使用 float/double 会导致协处理器状态破坏。用定点数或整数运算替代。

## 二、devm_ 自动资源管理

### 2.1 优先使用 devm_ 系列

**【描述】** 设备驱动中优先使用 `devm_` 系列 API，设备移除时自动释放资源。

| 普通版本 | devm 版本 | 说明 |
|----------|-----------|------|
| kmalloc | devm_kzalloc | 内存分配 |
| ioremap | devm_ioremap / devm_ioremap_resource | I/O 映射 |
| request_irq | devm_request_irq | 中断注册 |
| clk_get | devm_clk_get | 时钟获取 |
| regulator_get | devm_regulator_get | 电源调节器 |
| gpio_request | devm_gpiod_get | GPIO |

### 2.2 devm_ 不万能

**【描述】** 以下情况不能依赖 devm_ 自动释放：
- 需要在 remove 中提前释放的资源
- 非设备关联的全局资源
- 需要精确控制释放顺序的场景

## 三、DMA 安全

### 3.1 dma_mapping_error 必须检查

**【描述】** `dma_map_single` / `dma_map_page` 可能失败，必须检查。

**【错误代码】**
```c
dma_addr_t dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
hw_start_dma(dma);  // 错误：未检查映射是否成功
```

**【正确代码】**
```c
dma_addr_t dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma)) {
    dev_err(dev, "DMA map failed\n");
    return -EIO;
}
```

### 3.2 DMA map/unmap 必须配对

**【描述】** 每次 `dma_map_*` 必须有对应的 `dma_unmap_*`，否则 DMA 地址不会释放。

**【检查清单】**
- `dma_map_single` → `dma_unmap_single`
- `dma_map_page` → `dma_unmap_page`
- `dma_map_sg` → `dma_unmap_sg`
- 方向参数必须一致

### 3.3 DMA 方向正确性

**【描述】** DMA 方向必须与实际数据流一致：

| 方向 | 宏 | 含义 |
|------|-----|------|
| CPU → 设备 | DMA_TO_DEVICE | 设备读取数据 |
| 设备 → CPU | DMA_FROM_DEVICE | 设备写入数据 |
| 双向 | DMA_BIDIRECTIONAL | 双向（性能较差） |

### 3.4 流式 DMA 必须同步

**【描述】** 使用流式 DMA 时，必须在正确的时机做 cache 同步：

```c
/* CPU 写完数据，准备让设备读取 */
dma_sync_single_for_device(dev, dma, len, DMA_TO_DEVICE);

/* 设备写完数据，CPU 准备读取 */
dma_sync_single_for_cpu(dev, dma, len, DMA_FROM_DEVICE);
```

### 3.5 DMA 缓冲区不能用 vmalloc

**【描述】** `vmalloc` 分配的内存物理不连续，不能用于 DMA。必须用 `kmalloc`、`dma_alloc_coherent` 或 `dma_alloc_attrs`。

### 3.6 dma_alloc_coherent 不需要手动同步

**【描述】** `dma_alloc_coherent` 返回的缓冲区是硬件和 CPU 一致的，不需要额外的 cache 同步操作。

## 四、设备树 API

### 4.1 of_property_read 返回值检查

**【错误代码】**
```c
u32 val;
of_property_read_u32(np, "reg-offset", &val);
// 如果属性不存在，val 是未定义的
```

**【正确代码】**
```c
u32 val;
if (of_property_read_u32(np, "reg-offset", &val)) {
    dev_err(dev, "missing reg-offset\n");
    return -EINVAL;
}
```

### 4.2 of_match_table 必须有 sentinel

**【描述】** `of_device_id` 数组必须以空元素 `{ }` 结尾。

### 4.3 compatible 字符串必须精确匹配

**【描述】** 设备树中的 compatible 字符串必须与驱动中的完全一致，包括大小写。

## 五、设备模型

### 5.1 sysfs 属性返回值

**【描述】** `show` 回调必须用 `sysfs_emit`（不是 `snprintf`），返回值是 `sysfs_emit` 的返回值。

**【错误代码】**
```c
static ssize_t my_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", val);  // 错误
}
```

**【正确代码】**
```c
static ssize_t my_show(struct device *dev, struct device_attribute *attr, char *buf) {
    return sysfs_emit(buf, "%d\n", val);
}
```

### 5.2 sysfs store 返回 count

**【描述】** `store` 回调成功时必须返回 `count`，不是实际处理的字节数。

### 5.3 device_create 检查 IS_ERR

**【描述】** `device_create` 失败返回 `ERR_PTR`，不是 NULL。

## 六、时钟与电源

### 6.1 clk_prepare_enable 返回值

**【描述】** `clk_prepare_enable` 可能失败，必须检查。

### 6.2 regulator 操作顺序

**【正确顺序】**
```c
reg = devm_regulator_get(dev, "vdd");
regulator_enable(reg);
/* 使用 ... */
regulator_disable(reg);
```

## 七、GPIO

### 7.1 devm_gpiod_get 优于 gpio_request

**【描述】** 新驱动应使用 `devm_gpiod_get` 系列（基于描述符），不要用旧的 `gpio_request`。

### 7.2 GPIO 方向设置

**【正确代码】**
```c
struct gpio_desc *reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
if (IS_ERR(reset))
    return PTR_ERR(reset);

/* 输出高电平 */
gpiod_set_value(reset, 1);
```

## 八、printk 与日志

### 8.1 高频路径用限速日志

**【描述】** 可能频繁触发的错误路径使用 `_ratelimited` 版本。

```c
dev_err_ratelimited(dev, "Error occurred\n");
pr_err_ratelimited("System error\n");
```

### 8.2 设备相关日志用 dev_ 前缀

**【描述】** 有 `struct device` 时用 `dev_err` / `dev_warn` / `dev_info`，自动附加设备名称。

## 九、常见陷阱

1. **kmalloc 未检查返回值** — 内核崩溃（空指针解引用）
2. **dma_map_single 未检查 dma_mapping_error** — 数据损坏
3. **流式 DMA 未做 cache 同步** — CPU 读到过期数据，设备读到脏数据
4. **vmalloc 缓冲区用于 DMA** — 物理不连续，DMA 传输数据错乱
5. **sysfs show 用 sprintf 而非 sysfs_emit** — 可能缓冲区溢出
6. **of_property_read_u32 不检查返回值** — 读到未定义值
7. **device_create 返回值用 NULL 检查** — 应该用 IS_ERR
8. **clk_prepare_enable 忽略返回值** — 时钟未使能，外设不工作
9. **devm_gpiod_get 返回的错误用 NULL 检查** — 应该用 IS_ERR
10. **高频路径用普通 dev_err** — 日志洪水淹没有用信息
