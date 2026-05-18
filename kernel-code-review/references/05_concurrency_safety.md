# 代码审查技能文件 - 并发与线程安全

本文档覆盖 Linux 内核并发安全的核心规范条款。

## 一、自旋锁

### 1.1 中断上下文或持锁状态禁止睡眠

**【描述】** 软硬中断上下文或持有 spin_lock 时，禁止调用可能睡眠的函数（kmalloc GFP_KERNEL、msleep、mutex_lock、I/O 操作）。

**【错误代码】**
```c
spin_lock(&my_lock);
void *p = kmalloc(100, GFP_KERNEL);  // 错误：可能睡眠
spin_unlock(&my_lock);
```

**【正确代码】**
```c
spin_lock(&my_lock);
void *p = kmalloc(100, GFP_ATOMIC);
spin_unlock(&my_lock);
```

### 1.2 IRQ 变体锁的使用

**【描述】** 如果变量在进程上下文和中断上下文都会被访问，进程上下文中必须使用 `spin_lock_irqsave` / `spin_unlock_irqrestore`，禁止本地中断。

**【错误代码】**
```c
// 中断处理函数中也会获取 my_lock
spin_lock(&my_lock);  // 错误：不禁止中断，可能死锁
dev->state = NEW_STATE;
spin_unlock(&my_lock);
```

**【正确代码】**
```c
unsigned long flags;
spin_lock_irqsave(&my_lock, flags);
dev->state = NEW_STATE;
spin_unlock_irqrestore(&my_lock, flags);
```

### 1.3 持锁时间最小化

**【描述】** 临界区应尽可能短，不要在持锁时做耗时操作（大块内存拷贝、I/O、等待）。

**【正确代码】**
```c
// 在锁外准备数据
data = prepare_data();

spin_lock(&lock);
list_add(&data->list, &my_list);
spin_unlock(&lock);
```

## 二、互斥锁

### 2.1 mutex 不能在中断上下文使用

**【描述】** `mutex_lock` 可能睡眠，禁止在中断/软中断/tasklet 上下文调用。

### 2.2 避免锁序反转

**【描述】** 多把锁必须按固定顺序获取，否则会死锁。

**【错误代码】**
```c
// 线程 A：lock1 → lock2
// 线程 B：lock2 → lock1  // 死锁！
```

**【正确代码】**
```c
// 统一顺序：lock1 → lock2
mutex_lock(&lock1);
mutex_lock(&lock2);
// ...
mutex_unlock(&lock2);
mutex_unlock(&lock1);
```

## 三、RCU（Read-Copy-Update）

### 3.1 RCU 读侧临界区

**【描述】** RCU 读侧不能睡眠，不能调用可能阻塞的函数。

**【正确代码】**
```c
rcu_read_lock();
struct my_data *p = rcu_dereference(global_ptr);
if (p)
    do_something(p->value);
rcu_read_unlock();
```

### 3.2 RCU 更新

**【描述】** 更新时先复制修改，再替换指针，最后等待宽限期后释放旧数据。

**【正确代码】**
```c
struct my_data *old, *new;

new = kmalloc(sizeof(*new), GFP_KERNEL);
*new = *old;
new->value = new_value;

rcu_assign_pointer(global_ptr, new);
synchronize_rcu();  /* 等待所有读者退出 */
kfree(old);
```

### 3.3 RCU 链表操作

**【正确代码】**
```c
/* 添加 */
struct my_entry *entry = kmalloc(sizeof(*entry), GFP_KERNEL);
entry->data = 42;
list_add_rcu(&entry->list, &my_list);

/* 删除 */
list_del_rcu(&entry->list);
synchronize_rcu();
kfree(entry);

/* 遍历 */
rcu_read_lock();
list_for_each_entry_rcu(pos, &my_list, list) {
    /* 不能睡眠 */
}
rcu_read_unlock();
```

## 四、原子操作

### 4.1 atomic_t 使用

**【描述】** 简单计数器用 `atomic_t` 替代锁。

**【正确代码】**
```c
atomic_t refcount = ATOMIC_INIT(1);

atomic_inc(&refcount);
if (atomic_dec_and_test(&refcount)) {
    /* 引用归零，释放资源 */
    kfree(obj);
}
```

### 4.2 atomic_inc_not_zero 防止从零复活

**【正确代码】**
```c
if (!atomic_inc_not_zero(&obj->refcount))
    return -ENOENT;  /* 已被释放 */
```

## 五、内存屏障

### 5.1 编译器屏障

**【描述】** 防止编译器重排序。

```c
barrier();  /* 编译器屏障，不生成 CPU 屏障指令 */
```

### 5.2 SMP 内存屏障

**【描述】** 保证多核之间的内存可见性。

```c
/* 写屏障：保证屏障前的写操作在屏障后的写操作之前完成 */
smp_wmb();

/* 读屏障：保证屏障前的读操作在屏障后的读操作之前完成 */
smp_rmb();

/* 全屏障 */
smp_mb();

/* 典型场景：生产者-消费者 */
/* 生产者 */
data->payload = value;
smp_wmb();            /* 保证 payload 先于 ready 可见 */
data->ready = 1;

/* 消费者 */
if (data->ready) {
    smp_rmb();        /* 保证读到 ready 后再读 payload */
    use(data->payload);
}
```

### 5.3 使用 READ_ONCE / WRITE_ONCE

**【描述】** 防止编译器对共享变量的读写优化（合并、拆分、缓存到寄存器）。

```c
/* 在无锁读取时 */
int val = READ_ONCE(shared_var);

/* 在无锁写入时 */
WRITE_ONCE(shared_var, new_val);
```

## 六、workqueue 安全

### 6.1 cancel_work_sync 必须在没有锁时调用

**【描述】** `cancel_work_sync` 会等待 work 完成，如果 work 内部获取同一把锁会死锁。

**【错误代码】**
```c
mutex_lock(&lock);
cancel_work_sync(&work);  // 如果 work 中也 mutex_lock(&lock) → 死锁
mutex_unlock(&lock);
```

**【正确代码】**
```c
cancel_work_sync(&work);  /* 在锁外调用 */
mutex_lock(&lock);
/* ... */
mutex_unlock(&lock);
```

### 6.2 work 函数中可以睡眠

**【描述】** workqueue 运行在进程上下文，work 函数中可以使用 mutex、kmalloc(GFP_KERNEL)、I/O 等。但不能在中断上下文调用 schedule_work 后期望它立即执行。

## 七、per-CPU 变量

### 7.1 避免缓存行竞争

**【描述】** 高频更新的全局变量用 per-CPU 变量避免多核缓存行竞争。

**【正确代码】**
```c
DEFINE_PER_CPU(unsigned long, my_counter);

/* 使用 */
this_cpu_inc(my_counter);

/* 汇总 */
unsigned long total = 0;
for_each_possible_cpu(cpu)
    total += per_cpu(my_counter, cpu);
```

### 7.2 per-CPU 变量的抢占安全

**【描述】** 访问 per-CPU 变量时必须禁止抢占。

```c
/* get_cpu_var / put_cpu_var 自动禁止/恢复抢占 */
get_cpu_var(my_counter)++;
put_cpu_var(my_counter);
```

## 八、常见陷阱

1. **spin_lock 后调用 kmalloc(GFP_KERNEL)** — GFP_KERNEL 可能睡眠，持 spin_lock 时睡眠 = 死锁
2. **IRQ 变体锁忘记保存 flags** — spin_lock_irq 不保存 flags，嵌套调用会丢失中断状态
3. **RCU 读侧睡眠** — rcu_read_lock 内调用 mutex_lock 或 kmalloc(GFP_KERNEL)
4. **cancel_work_sync 在锁内调用** — work 也获取同一把锁 → 死锁
5. **smp_wmb/smp_rmb 方向错误** — 写屏障保护写操作，读屏障保护读操作
6. **atomic 操作不保证多个变量的原子性** — 两个 atomic_t 之间没有原子性保证
7. **per-CPU 变量在 preempt_enable 后使用** — 可能已切换到其他 CPU
