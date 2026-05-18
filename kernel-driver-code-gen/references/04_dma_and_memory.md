# DMA 与内核内存管理参考

## 内核内存分配

### 基础分配

```c
#include <linux/slab.h>
#include <linux/vmalloc.h>

/* 小块内存（< 页大小），物理连续 */
void *p = kmalloc(size, GFP_KERNEL);   /* 进程上下文 */
void *p = kmalloc(size, GFP_ATOMIC);   /* 中断上下文 */
void *p = kzalloc(size, GFP_KERNEL);   /* 零初始化 */

/* 大块内存，虚拟连续（物理不一定连续） */
void *p = vmalloc(size);  /* 可以睡眠 */

/* 释放 */
kfree(p);    /* kmalloc/kzalloc */
vfree(p);    /* vmalloc */
```

### devm 版本（自动释放）

```c
/* 设备关联的内存，设备移除时自动释放 */
void *p = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);

/* 不需要显式 kfree，probe 失败时自动清理 */
```

### 专用缓存（频繁分配/释放同类型对象）

```c
struct kmem_cache *cache;

/* 创建缓存 */
cache = kmem_cache_create("mydrv_obj",
                          sizeof(struct my_obj),
                          0, 0, NULL);

/* 分配 */
struct my_obj *obj = kmem_cache_alloc(cache, GFP_KERNEL);

/* 释放 */
kmem_cache_free(cache, obj);

/* 销毁缓存 */
kmem_cache_destroy(cache);
```

## DMA 映射

### 一致性 DMA（Streaming DMA）

```c
#include <linux/dma-mapping.h>

/* 设置 DMA 掩码 */
ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
if (ret) {
    dev_err(&pdev->dev, "DMA not supported\n");
    return ret;
}

/* 分配一致性 DMA 缓冲区（缓存一致，无需手动 sync） */
dma_addr_t dma_handle;
void *buf = dma_alloc_coherent(&pdev->dev, size, &dma_handle, GFP_KERNEL);
if (!buf)
    return -ENOMEM;

/* buf 是内核虚拟地址，dma_handle 是设备可见的物理地址 */
/* 设备使用 dma_handle，CPU 使用 buf */

/* 释放 */
dma_free_coherent(&pdev->dev, size, buf, dma_handle);
```

### 流式 DMA 映射

```c
/* 用于已有缓冲区的 DMA 映射 */
dma_addr_t dma_handle;

/* 映射（设备读取方向） */
dma_handle = dma_map_single(&pdev->dev, buf, size, DMA_TO_DEVICE);
if (dma_mapping_error(&pdev->dev, dma_handle)) {
    dev_err(&pdev->dev, "DMA map failed\n");
    return -EIO;
}

/* 使用 dma_handle 给设备... */

/* 解除映射 */
dma_unmap_single(&pdev->dev, dma_handle, size, DMA_TO_DEVICE);
```

### DMA 方向

| 方向 | 宏 | 含义 |
|------|-----|------|
| CPU → 设备 | DMA_TO_DEVICE | 设备读取数据 |
| 设备 → CPU | DMA_FROM_DEVICE | 设备写入数据 |
| 双向 | DMA_BIDIRECTIONAL | 双向传输（性能较差） |

### 流式 DMA 同步

```c
/* 映射后、设备访问前 */
dma_sync_single_for_device(&pdev->dev, dma_handle, size, DMA_TO_DEVICE);

/* 设备完成后、CPU 访问前 */
dma_sync_single_for_cpu(&pdev->dev, dma_handle, size, DMA_FROM_DEVICE);
```

### SG（Scatter-Gather）DMA

```c
/* 分散/聚集 DMA，用于非连续物理页 */
struct scatterlist sg;

sg_init_table(sg_list, nents);
sg_set_buf(&sg_list[0], buf1, len1);
sg_set_buf(&sg_list[1], buf2, len2);

/* 映射所有 scatter-gather 条目 */
int nents = dma_map_sg(&pdev->dev, sg_list, nents_orig, DMA_TO_DEVICE);
if (nents == 0)
    return -EIO;

/* 遍历映射后的条目 */
for_each_sg(sg_list, sg, nents, i) {
    dma_addr_t addr = sg_dma_address(sg);
    unsigned int len = sg_dma_len(sg);
    /* 给设备编程... */
}

/* 解除映射 */
dma_unmap_sg(&pdev->dev, sg_list, nents_orig, DMA_TO_DEVICE);
```

## DMA Pool（小块 DMA 内存）

```c
/* DMA 池，用于频繁分配小块 DMA 内存 */
struct dma_pool *pool;

pool = dma_pool_create("mydrv_pool", &pdev->dev,
                       size, alignment, boundary);

void *buf = dma_pool_alloc(pool, GFP_KERNEL, &dma_handle);

dma_pool_free(pool, buf, dma_handle);

dma_pool_destroy(pool);
```

## 页面分配

```c
#include <linux/gfp.h>

/* 分配 2^n 页 */
struct page *page = alloc_pages(GFP_KERNEL, order);  /* 2^order 页 */
void *addr = page_address(page);  /* 获取虚拟地址 */

/* 释放 */
__free_pages(page, order);
```

## 内存映射（ioremap）

```c
/* 物理地址 → 内核虚拟地址（用于访问设备寄存器） */
void __iomem *base = ioremap(phys_addr, size);
if (!base)
    return -ENOMEM;

/* 读写 I/O 内存 */
u32 val = readl(base + REG_OFFSET);
writel(val, base + REG_OFFSET);

/* 释放 */
iounmap(base);

/* 推荐使用 devm 版本 */
void __iomem *base = devm_ioremap(&pdev->dev, phys_addr, size);
/* 或者直接从资源获取 */
void __iomem *base = devm_ioremap_resource(&pdev->dev, res);
```

## 常见陷阱

1. **GFP_KERNEL vs GFP_ATOMIC** — 中断上下文必须用 GFP_ATOMIC
2. **dma_alloc_coherent 返回的地址不要用 virt_to_phys** — 直接用返回的 dma_handle
3. **流式 DMA 必须在设备访问前 map，完成后 unmap** — 避免 cache 不一致
4. **vmalloc 的内存不能用于 DMA** — 物理不连续
5. **dma_mapping_error 必须检查** — DMA 映射可能失败
6. **ioremap 的地址不能直接解引用** — 必须用 readl/writel
