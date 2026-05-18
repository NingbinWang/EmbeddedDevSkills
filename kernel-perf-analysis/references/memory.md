# 内存性能分析指南

## 全局系统检查

首先检查内存容量、回收、交换和缺页行为：

```bash
free -h
vmstat 1 5
sar -r 1 3
sar -B 1 3
```

重点关注以下指标：

- **可用内存**(`available`)，而非仅关注空闲内存(`free`)
- 交换空间使用情况及交换进出活动
- 主要缺页(major page faults)次数
- 内存回收压力
- OOM(Out-Of-Memory)杀进程事件或内存分配失败

辅助检查命令：

```bash
dmesg | tail -n 50
grep -i 'oom\\|killed process' /var/log/messages /var/log/syslog 2>/dev/null
```

## 进程级深入排查

识别占用内存或触发缺页的进程：

```bash
ps -eo pid,ppid,cmd,%mem,rss,vsz --sort=-rss | head
smem -rk
pidstat -r -p ALL 1 3
```

当怀疑特定进程存在问题时，检查其内存映射和增长模式：

```bash
pmap -x <pid> | tail -n 20
cat /proc/<pid>/status
cat /proc/<pid>/smaps_rollup
```

## 性能指标解读指南

- **低`available`内存 + 活跃的交换进出活动**：表明存在真实的内存压力
- **大量页缓存但`available`内存充足**：不一定构成问题
- **高主要缺页率或内存回收停顿**：即使在OOM发生前也可能导致延迟激增
- **重复的OOM事件**：表明内存配置不足或存在一个/多个失控进程
- **RSS持续增长或存在明显未释放内存分配的进程**：暗示应用程序层面存在内存泄漏或过度保留

## 内核态与用户态内存瓶颈区分

- **用户态瓶颈特征**：一个或多个进程主导RSS占用、存在内存泄漏或触发过度的内存分配/缺页
- **内核态瓶颈特征**：slab增长、内存回收行为、页缓存压力或其他内核管理的内存行为占主导地位
- **混合型瓶颈**：内存密集型应用程序可能迫使内核进行内存回收和交换风暴。应报告时间损耗所在的具体层面，并指明触发问题的进程