# CPU Analysis

## System-Wide Checks

Start with overall CPU pressure and scheduler signals:

```bash
top -b -n 1
mpstat -P ALL 1 3
vmstat 1 5
pidstat -u -t 1 3
sar -u 1 3
```

Focus on:

- `%us`: user-space CPU pressure
- `%sy`: kernel CPU pressure
- `%wa`: waiting on I/O, usually not a pure CPU bottleneck
- `%hi` and `%si`: hardware/software interrupt pressure
- run queue, load average, and context switches
- single-core hotspots versus whole-machine saturation

## Process Drill-Down

Find which process or thread consumes CPU:

```bash
ps -eo pid,ppid,cmd,%cpu,%mem --sort=-%cpu | head
top -H -b -n 1
pidstat -u -t -p ALL 1 3
```

If one process is clearly hot, use low-intrusion checks first:

```bash
pidstat -u -t -p <pid> 1 5
cat /proc/<pid>/sched
```

Escalation only (explicit approval, bounded duration):

```bash
perf top -p <pid>
perf record -F 49 -g -p <pid> -- sleep 10
perf report
timeout 10s strace -tt -T -p <pid>
```

## Interpretation

- High `%us` with hot user threads usually means application-space CPU bottleneck.
- High `%sy` means kernel work is expensive; inspect syscalls, locks, networking, filesystem, and interrupts.
- High `%si` or `%hi` suggests interrupt pressure, often network or driver related.
- High load with modest `%us` and `%sy` means do not stop at CPU; check I/O wait, blocked tasks, and lock contention.
- If total CPU is high but no single long-lived process stands out, look for short-lived worker churn, fork storms, or thread bursts.

## Kernel Vs Application

- Application space: hot functions are in the process itself; `perf` shows user functions dominating.
- Kernel space: `perf` or `top` shows kernel functions, softirq, scheduler, TCP stack, or syscall paths dominating.
- Mixed case: an application may trigger kernel bottlenecks through excessive syscalls, networking, or filesystem activity. Report both the kernel hotspot and the process causing it.
