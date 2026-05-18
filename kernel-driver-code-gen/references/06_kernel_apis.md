# 内核 API 参考

## 自旋锁

```c
#include <linux/spinlock.h>

spinlock_t lock;
spin_lock_init(&lock);

/* 进程上下文（不允许睡眠） */
spin_lock(&lock);
/* 临界区 */
spin_unlock(&irq_lock);

/* 中断上下文（必须保存中断状态） */
unsigned long flags;
spin_lock_irqsave(&lock, flags);
/* 临界区 */
spin_unlock_irqrestore(&lock, flags);
```

## 互斥锁

```c
#include <linux/mutex.h>

struct mutex mtx;
mutex_init(&mtx);

mutex_lock(&mtx);
/* 可以睡眠的临界区 */
mutex_unlock(&mtx);

/* 尝试加锁（不阻塞） */
if (mutex_trylock(&mtx)) {
    /* 获取到锁 */
    mutex_unlock(&mtx);
}

/* 销毁 */
mutex_destroy(&mtx);
```

## 读写锁

```c
#include <linux/rwlock.h>

rwlock_t rwlock;
rwlock_init(&rwlock);

/* 读锁（多个读者可同时持有） */
read_lock(&rwlock);
/* 读取 */
read_unlock(&rwlock);

/* 写锁（独占） */
write_lock(&rwlock);
/* 写入 */
write_unlock(&rwlock);

/* 读写信号量（可睡眠版本） */
#include <linux/rwsem.h>

struct rw_semaphore rwsem;
init_rwsem(&rwsem);

down_read(&rwsem);
/* 读取 */
up_read(&rwsem);

down_write(&rwsem);
/* 写入 */
up_write(&rwsem);
```

## 原子操作

```c
#include <linux/atomic.h>

atomic_t count = ATOMIC_INIT(0);

atomic_inc(&count);
atomic_dec(&count);
atomic_add(5, &count);
atomic_sub(3, &count);
int val = atomic_read(&count);

/* 原子比较交换 */
atomic_cmpxchg(&count, old_val, new_val);

/* 位操作 */
unsigned long flags;
set_bit(0, &flags);
clear_bit(0, &flags);
change_bit(0, &flags);
test_bit(0, &flags);
```

## 完成量

```c
#include <linux/completion.h>

struct completion done;
init_completion(&done);

/* 等待方 */
wait_for_completion(&done);
/* 或带超时 */
if (wait_for_completion_timeout(&done, msecs_to_jiffies(1000)) == 0) {
    /* 超时 */
}

/* 通知方 */
complete(&done);       /* 唤醒一个等待者 */
complete_all(&done);   /* 唤醒所有等待者 */

/* 重新初始化 */
reinit_completion(&done);
```

## 等待队列

```c
#include <linux/wait.h>

DECLARE_WAIT_QUEUE_HEAD(wq);
int condition = 0;

/* 等待方（进程上下文） */
wait_event(wq, condition);                    /* 无限等待 */
wait_event_timeout(wq, condition, timeout);   /* 带超时 */
wait_event_interruptible(wq, condition);      /* 可被信号中断 */

/* 唤醒方 */
condition = 1;
wake_up(&wq);                  /* 唤醒一个 */
wake_up_all(&wq);              /* 唤醒所有 */
```

## 链表

```c
#include <linux/list.h>

struct my_entry {
    int data;
    struct list_head list;
};

LIST_HEAD(my_list);

/* 添加 */
struct my_entry *entry = kmalloc(sizeof(*entry), GFP_KERNEL);
entry->data = 42;
list_add(&entry->list, &my_list);        /* 头部添加 */
list_add_tail(&entry->list, &my_list);   /* 尾部添加 */

/* 遍历 */
struct my_entry *pos;
list_for_each_entry(pos, &my_list, list) {
    pr_info("data: %d\n", pos->data);
}

/* 安全遍历（可删除元素） */
struct my_entry *tmp;
list_for_each_entry_safe(pos, tmp, &my_list, list) {
    if (pos->data == 42) {
        list_del(&pos->list);
        kfree(pos);
    }
}

/* 删除 */
list_del(&entry->list);
kfree(entry);
```

## 内核定时器

```c
#include <linux/timer.h>

struct timer_list my_timer;

void timer_callback(struct timer_list *t)
{
    struct my_priv *priv = from_timer(priv, t, timer);
    /* 处理 */
}

/* 初始化 */
timer_setup(&my_timer, timer_callback, 0);

/* 启动（相对超时） */
mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));

/* 更新 */
mod_timer(&my_timer, jiffies + msecs_to_jiffies(500));

/* 删除（等待正在运行的回调完成） */
del_timer_sync(&my_timer);
```

## 延迟函数

```c
#include <linux/delay.h>
#include <linux/jiffies.h>

/* 忙等待（短延迟，中断上下文可用） */
udelay(100);      /* 微秒 */
ndelay(1000);     /* 纳秒 */

/* 睡眠等待（只能在进程上下文） */
msleep(100);      /* 毫秒（至少） */
msleep_interruptible(100);  /* 可被信号中断 */
ssleep(1);        /* 秒 */

/* jiffies */
unsigned long timeout = jiffies + msecs_to_jiffies(5000);
while (time_before(jiffies, timeout)) {
    /* 轮询 */
    msleep(10);
}
```

## 打印

```c
#include <linux/printk.h>

/* 带设备信息的打印（推荐） */
dev_info(dev, "probe successful\n");
dev_warn(dev, "timeout waiting for ready\n");
dev_err(dev, "failed to alloc: %d\n", ret);
dev_dbg(dev, "val=0x%08x\n", val);  /* 需要 DEBUG 定义 */

/* 通用打印 */
pr_info("module loaded\n");
pr_warn("deprecated API\n");
pr_err("error: %d\n", ret);
pr_debug("debug info\n");  /* 需要 DEBUG 或 CONFIG_DYNAMIC_DEBUG */

/* 紧急 */
pr_emerg("system is dead\n");
pr_crit("critical error\n");
```

## 内核时间

```c
#include <linux/jiffies.h>
#include <linux/ktime.h>

/* jiffies */
unsigned long start = jiffies;
/* ... */
unsigned long elapsed = jiffies - start;
unsigned long ms = jiffies_to_msecs(elapsed);

/* ktime（高精度） */
ktime_t start = ktime_get();
/* ... */
s64 elapsed_ns = ktime_to_ns(ktime_sub(ktime_get(), start));
s64 elapsed_us = elapsed_ns / 1000;
```

## 常见陷阱

1. **spin_lock 后不能睡眠** — mutex_lock, kmalloc(GFP_KERNEL), msleep 都不行
2. **mutex_lock 后可以用 spin_lock** — 但反过来不行
3. **wait_event 的条件必须在唤醒后变化** — 否则虚假唤醒
4. **list_for_each_entry 中删除元素用 _safe 版本** — 否则遍历损坏
5. **udelay 精度不高** — 对于非常短的延迟，用 ndelay 或读寄存器
