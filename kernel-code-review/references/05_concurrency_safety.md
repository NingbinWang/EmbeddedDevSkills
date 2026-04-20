# 代码审查技能文件 - 并发与线程安全

本文档例举Linux C(内核与驱动)安全编码规范中并发相关条款。

## 一、并发安全

### 1.1 不要使用睡眠函数在中断上下文或持锁状态

**【描述】**
在软硬中断上下文，或者持有自旋锁(`spin_lock`)的情况下，绝对禁止调用任何可能引起当前线程休眠的函数（例如 `kmalloc` 时传入 `GFP_KERNEL`、进行 `msleep`、获取 `mutex_lock`，或者触发可能休眠的I/O操作）。

**【风险】**
会导致内核死锁或巨大的调度延迟 (Scheduling while atomic)。

**【错误代码示例】**
```c
spin_lock(&my_lock);
// 错误：GFP_KERNEL 可能引起休眠调度，但当前持有自旋锁
void *p = kmalloc(100, GFP_KERNEL); 
spin_unlock(&my_lock);
```

**【正确代码示例】**
```c
// 可以改用 GFP_ATOMIC
spin_lock(&my_lock);
void *p = kmalloc(100, GFP_ATOMIC); 
spin_unlock(&my_lock);
```

### 1.2 避免竞态条件 (Race Conditions)

**【描述】**
对全局对象、设备驱动状态机变量的并发读写需要加锁（`spin_lock`, `mutex`）或者使用原子类型 (`atomic_t`)。不可做没有保护的读-改-写操作。

**【错误代码示例】**
```c
// 多个线程并发执行可能导致 count 计数不对
if (dev->count < MAX_COUNT) {
    dev->count++;
    do_something();
}
```

### 1.3 正确使用自旋锁的 IRQ 变体

**【描述】**
如果一个变量在进程上下文和中断/软中断上下文都会被访问，则进程上下文中保护这个变量的自旋锁必须使用 `spin_lock_irqsave`，以禁止本地中断并保存中断状态，防止中断内再获取同一把锁导致的死锁。

**【正确代码示例】**
```c
unsigned long flags;
spin_lock_irqsave(&dev->lock, flags);
dev->state = NEW_STATE;
spin_unlock_irqrestore(&dev->lock, flags);
```
