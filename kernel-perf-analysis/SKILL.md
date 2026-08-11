---
name: kernel-perf-analysis
description: |
  诊断 Linux 系统在 CPU、内存、I/O 和网络方面的性能问题与瓶颈。
  从系统级症状驱动分析，定位瓶颈所在子系统、判断内核态还是应用态、确定责任进程。
triggers:
  - "性能分析"
  - "性能诊断"
  - "性能瓶颈"
  - "系统变慢"
  - "CPU 高"
  - "内存不足"
  - "OOM"
  - "I/O 慢"
  - "磁盘阻塞"
  - "网络延迟"
  - "丢包"
  - "高负载"
  - "perf 分析"
  - "vmstat"
  - "负载高"
  - "响应慢"
  - "过载"
  - "卡顿"
---

# Linux 性能诊断

结构化诊断 CPU、内存、I/O、网络四子系统的性能瓶颈。

## 安全分级（低侵入默认）

| 级别 | 说明 | 触发条件 |
|------|------|----------|
| **L1**（默认）| 只读、低开销观察 | 始终可用 |
| **L2** | 短时定向采样，严格限时 | 用户批准 |
| **L3** | 附加、跟踪、抓包、eBPF、负载测试 | 明确批准 + 时间窗口确认 |

L3 执行前必须：确认主机角色和时间窗口 → 说明预期开销 → 设置采样上限。

## 工作流

```
确定症状和时间窗口
  → 捕获轻量级系统基线
  → 扫描 CPU、内存、I/O、网络
  → 深入可疑子系统到进程级
  → 判断内核态 vs 应用态
  → 输出结论
```

## 基线命令

```bash
uname -a && uptime
top -b -n 1
vmstat 1 5
pidstat 1 3
```

根据当前信号添加子系统特定命令。

## 四子系统分析规则

**即使一个区域看起来明显，也要检查全部四个** — 第一个可疑指标不一定是根因。

通用模式：
1. 检查整体系统压力
2. 确认持续性还是突发性
3. 识别主要贡献进程
4. 判断热点在内核态还是应用态
5. 记录证据

### 各子系统快速参考

| 子系统 | 系统级命令 | 进程级命令 | 查看参考 |
|--------|-----------|-----------|----------|
| CPU | `top`, `mpstat`, `vmstat` | `pidstat -u -t`, `ps --sort=-%cpu` | [cpu.md](references/cpu.md) |
| 内存 | `free -h`, `vmstat` | `pidstat -r`, `pmap`, `/proc/<pid>/smaps` | [memory.md](references/memory.md) |
| I/O | `iostat`, `iotop`, `vmstat` | `pidstat -d`, `lsof`, `/proc/<pid>/io` | [io.md](references/io.md) |
| 网络 | `ss`, `netstat`, `sar -n` | `ss -tnp`, `nstat`, `tcpdump`(L3) | [network.md](references/network.md) |

### 交叉信号

- 高 `wa` → 通常指向 I/O，非 CPU
- 高负载 + 低 CPU 使用率 → 被阻塞的任务，I/O 或锁等待
- 高 `sy`/`si`/`hi`/重传 → 内核/网络处理
- Swap/主缺页/回收停滞/OOM → 内存压力
- 网络症状可能由 CPU 饱和、套接字积压或应用读写行为引起

## 内核态 vs 应用态判断

| 瓶颈层 | 特征 |
|--------|------|
| **应用态** | 成本在用户进程、业务逻辑、GC、序列化 |
| **内核态** | 成本在调度、中断、软中断、文件系统/块层、TCP/IP 栈 |
| **混合** | 应用层行为触发内核压力 → 报告两者 |

## 深度分析参考

在执行深入诊断前，**必须读取匹配的参考文件**：

- CPU: [references/cpu.md](references/cpu.md)
- 内存: [references/memory.md](references/memory.md)
- I/O: [references/io.md](references/io.md)
- 网络: [references/network.md](references/network.md)

报告格式：使用 [references/report-template.md](references/report-template.md)。

## 输出格式

始终以结论收尾，明确陈述：

1. **主要瓶颈**：CPU / 内存 / I/O / 网络
2. **瓶颈层面**：内核空间 / 应用程序空间
3. **触发进程**：主要应用进程名称，或标注"未识别"

如证据不完整 → 说明已确认 + 仅怀疑的内容 + 最能减少不确定性的下一步命令。
