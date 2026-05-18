# 中断处理参考

## 中断注册

### devm_request_irq（推荐）

```c
#include <linux/interrupt.h>

static irqreturn_t mydrv_isr(int irq, void *data)
{
    struct mydrv_priv *priv = data;

    /* 读中断状态寄存器 */
    u32 status = readl(priv->regs + IRQ_STATUS_REG);

    if (!(status & IRQ_MASK))
        return IRQ_NONE;  /* 不是我们的中断 */

    /* 清除中断 */
    writel(status, priv->regs + IRQ_CLEAR_REG);

    /* 处理中断 */
    if (status & IRQ_RX_READY)
        mydrv_handle_rx(priv);
    if (status & IRQ_TX_DONE)
        mydrv_handle_tx(priv);

    return IRQ_HANDLED;
}

/* 在 probe 中注册 */
ret = devm_request_irq(&pdev->dev, irq, mydrv_isr,
                       IRQF_SHARED, "mydrv", priv);
```

### 线程化中断

```c
/* 中断处理在内核线程中运行，可以睡眠 */
static irqreturn_t mydrv_threaded_isr(int irq, void *data)
{
    struct mydrv_priv *priv = data;

    /* 可以使用 mutex、I2C、SPI 等可能睡眠的 API */
    mutex_lock(&priv->lock);
    mydrv_process_data(priv);
    mutex_unlock(&priv->lock);

    return IRQ_HANDLED;
}

static irqreturn_t mydrv_hard_isr(int irq, void *data)
{
    /* 硬中断：只做最少的工作（读状态、确认中断） */
    struct mydrv_priv *priv = data;
    u32 status = readl(priv->regs + IRQ_STATUS_REG);

    if (!status)
        return IRQ_NONE;

    /* 禁用中断，交给线程处理 */
    writel(0, priv->regs + IRQ_ENABLE_REG);
    priv->irq_status = status;

    return IRQ_WAKE_THREAD;  /* 唤醒线程化处理 */
}

/* 注册硬中断 + 线程化中断 */
ret = devm_request_threaded_irq(&pdev->dev, irq,
                                 mydrv_hard_isr,       /* 硬中断 */
                                 mydrv_threaded_isr,    /* 线程 */
                                 IRQF_SHARED, "mydrv", priv);
```

## 中断下半部

### tasklet（已废弃，用 workqueue 替代）

```c
/* 内核 6.x+ 已废弃 tasklet，推荐用 workqueue */
```

### workqueue

```c
#include <linux/workqueue.h>

static void mydrv_work_handler(struct work_struct *work)
{
    struct mydrv_priv *priv = container_of(work, struct mydrv_priv, work);

    /* 可以睡眠 */
    mutex_lock(&priv->lock);
    mydrv_process_data(priv);
    mutex_unlock(&priv->lock);
}

/* 初始化 */
INIT_WORK(&priv->work, mydrv_work_handler);

/* 在中断处理中调度 */
static irqreturn_t mydrv_isr(int irq, void *data)
{
    struct mydrv_priv *priv = data;

    /* 硬中断中：只记录状态，调度 work */
    priv->irq_status = readl(priv->regs + IRQ_STATUS_REG);
    writel(priv->irq_status, priv->regs + IRQ_CLEAR_REG);

    schedule_work(&priv->work);

    return IRQ_HANDLED;
}

/* 清理 */
cancel_work_sync(&priv->work);
```

### 延迟工作

```c
#include <linux/workqueue.h>

static void mydrv_delayed_work(struct work_struct *work)
{
    struct mydrv_priv *priv = container_of(to_delayed_work(work),
                                           struct mydrv_priv, dwork);
    /* 定时处理 */
}

INIT_DELAYED_WORK(&priv->dwork, mydrv_delayed_work);

/* 延迟 100ms 执行 */
schedule_delayed_work(&priv->dwork, msecs_to_jiffies(100));

/* 取消 */
cancel_delayed_work_sync(&priv->dwork);
```

### 内核定时器

```c
#include <linux/timer.h>

static void mydrv_timer_fn(struct timer_list *t)
{
    struct mydrv_priv *priv = from_timer(priv, t, timer);

    /* 软中断上下文，不能睡眠 */
    mydrv_poll_status(priv);

    /* 重新启动定时器 */
    mod_timer(&priv->timer, jiffies + msecs_to_jiffies(1000));
}

/* 初始化 */
timer_setup(&priv->timer, mydrv_timer_fn, 0);

/* 启动 */
mod_timer(&priv->timer, jiffies + msecs_to_jiffies(1000));

/* 删除 */
del_timer_sync(&priv->timer);
```

## 中断上下文规则

| 上下文 | 可以做的 | 不能做的 |
|--------|----------|----------|
| 硬中断 | spinlock, readl/writel, schedule_work | mutex, kmalloc(GFP_KERNEL), sleep, I2C/SPI |
| 线程化中断 | 以上全部 + mutex, I2C/SPI, kmalloc | — |
| tasklet | spinlock, readl/writel | mutex, kmalloc(GFP_KERNEL), sleep |
| workqueue | 以上全部 | — |
| 定时器 | spinlock, readl/writel, schedule_work | mutex, sleep |

## 中断亲和性

```c
/* 绑定中断到特定 CPU */
irq_set_affinity_hint(irq, cpumask_of(2));

/* 查询中断分配在哪些 CPU 上 */
cat /proc/interrupts
```

## 中断统计

```bash
# 查看中断计数
cat /proc/interrupts

# 查看软中断统计
cat /proc/softirqs
```

## 常见陷阱

1. **硬中断中不能睡眠** — mutex, kmalloc(GFP_KERNEL), msleep 都不行
2. **IRQF_SHARED 要求 ISR 正确判断** — 共享中断时必须检查是否是自己的设备
3. **devm_request_irq 自动释放** — 但如果在 remove 中需要提前禁用，用 disable_irq
4. **中断风暴** — 如果 ISR 一直返回 IRQ_HANDLED 且不屏蔽中断源，会导致系统卡死
5. **del_timer_sync 可能死锁** — 不要在定时器回调中调用，用 del_timer
