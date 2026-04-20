---
name: linux-performance-analysis
description: Diagnose Linux system performance problems and bottlenecks across CPU, memory, I/O, and network. Use when a user asks to analyze a Linux host that is slow, overloaded, laggy, timing out, dropping packets, swapping, blocked on disk, or showing high load/high latency, and the goal is to determine which subsystem is the bottleneck, whether the bottleneck is in kernel space or application space, and which process is responsible at the application layer.
---

# Linux Performance Analysis

## Overview

Use this skill to perform a structured Linux performance diagnosis. Always cover CPU, memory, I/O, and network, and drive the analysis from system-wide symptoms down to the specific process when the bottleneck is in application space.

Prefer read-only inspection for troubleshooting. If the user only provides command output, interpret that output with the same workflow instead of asking for a completely new dataset.

## Safety First (Low Intrusion Default)

Default to the least intrusive workflow, especially on production hosts.

- L1 (default): read-only, low-overhead observation only.
- L2 (conditional): short, targeted sampling with strict time limits.
- L3 (escalation): attach, tracing, packet capture, eBPF, or active load tests only with explicit user approval.

Before any L3 command:

1. Confirm the host role (production or non-production) and time window.
2. Explain expected overhead in one short line.
3. Set an explicit duration or sample cap.
4. Prefer one command that reduces uncertainty the most.

Avoid active load generation (for example `iperf3`) on production unless the user explicitly requests it and accepts the risk.

## Workflow

1. Establish the symptom and time window.
2. Capture a lightweight system-wide baseline.
3. Sweep CPU, memory, I/O, and network.
4. For any suspicious area, drill down from system level to process level.
5. Decide whether the bottleneck is primarily in kernel space or application space.
6. Deliver a conclusion that explicitly answers the required three questions.

## Baseline

Start with a quick baseline before deep dives. Reuse what the user already has when possible.

Typical baseline commands:

```bash
uname -a
uptime
top -b -n 1
vmstat 1 5
pidstat 1 3
```

Add the area-specific commands from the reference file that matches the current signal.

## Analysis Rules

Analyze the four areas even if one problem looks obvious. The first suspicious metric is not always the root cause.

For each area, follow the same pattern:

1. Check overall system pressure.
2. Confirm whether the pressure is sustained or bursty.
3. Identify the top contributing processes, threads, sockets, or devices.
4. Judge whether the hotspot is mostly kernel-side or application-side.
5. Record the evidence, not just the guess.

Treat these as common cross-signals:

- High `wa` often points to I/O, not CPU.
- High load average with low CPU usage can mean blocked tasks, usually I/O or lock waits.
- High `sy`, `si`, `hi`, retransmits, or backlog pressure often points to kernel/network handling.
- Swap, major page faults, reclaim stalls, or OOM events point to memory pressure.
- Network symptoms can be caused upstream by CPU saturation, socket backlog, or application read/write behavior.

## Kernel Vs Application Judgment

Use this rule of thumb when classifying the bottleneck:

- Classify as application space when the dominant cost is in user processes, user-space threads, request handlers, GC, query execution, serialization, compression, or other business/application logic.
- Classify as kernel space when the dominant cost is in scheduling, interrupts, softirqs, reclaim, filesystem/block layer, TCP/IP stack, driver handling, syscall-heavy behavior, or other kernel-mediated paths.
- If an application causes heavy kernel work, report both: the bottleneck layer is kernel space, and the triggering application process is the user-space process that is driving that kernel pressure.

## Required References

Read the matching reference file before doing a deep dive:

- CPU: [references/cpu.md](references/cpu.md)
- Memory: [references/memory.md](references/memory.md)
- I/O: [references/io.md](references/io.md)
- Network: [references/network.md](references/network.md)

Use [references/report-template.md](references/report-template.md) for the final write-up format.

## Output Contract

Always end with a concise conclusion section that clearly states:

1. The primary bottleneck is CPU, memory, I/O, or network.
2. The bottleneck in that area is mainly in kernel space or application space.
3. If application space is involved, which process is the main trigger.

If the evidence is incomplete, say what is confirmed, what is only suspected, and which single next command would most reduce uncertainty.
