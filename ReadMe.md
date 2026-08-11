<div align="center">

# EmbeddedDevSkills — 嵌入式 Linux 开发技能集

> *为嵌入式 Linux 开发者提供代码生成、检视、性能诊断的 AI Skill 集合*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Claude Code](https://img.shields.io/badge/Claude%20Code-Skill-blueviolet)](https://claude.ai/code)
[![Skills](https://img.shields.io/badge/skills.sh-Compatible-green)](https://skills.sh)

</div>

---

## 技能列表

| 技能 | 用途 | 触发场景 |
|------|------|----------|
| **kernel-code-review** | 内核/驱动代码安全检视 | 代码 review、安全审计、编码规范检查 |
| **kernel-driver-code-gen** | 内核驱动代码生成 | 创建驱动框架、生成 Kbuild/Makefile |
| **kernel-perf-analysis** | Linux 性能诊断 | 系统变慢、高负载、OOM、I/O 阻塞 |
| **linuxc-code-gen** | 应用层 C 代码生成 | 系统编程、网络服务、多线程程序 |
| **time-based-scheduler** | 时间段任务调度 | 定时任务、分时段自动化工作流 |

---

## 安装

### 方式一：手动安装（推荐当前方式）

将技能目录复制到 Claude Code 的 skills 目录：

```bash
# 创建 skills 目录（如不存在）
mkdir -p ~/.claude/skills/

# 复制需要的技能（以 kernel-code-review 为例）
cp -r kernel-code-review ~/.claude/skills/

# 或复制全部技能
cp -r kernel-code-review kernel-driver-code-gen kernel-perf-analysis \
      linuxc-code-gen time-based-scheduler ~/.claude/skills/
```

### 方式二：插件化安装（后续支持）

后续将支持通过 skills.sh 一键安装。

---

## 各技能详解

### 1. kernel-code-review — 内核代码检视

基于假设检验方法论，对照 10 类编码规范逐条检查：

- **全量检视**：`review <文件/目录>` — 遍历所有规范类别
- **聚焦检视**：`review <文件/目录> <类别>` — 指定类别检查
- **快速扫描**：`quick-scan <文件/目录>` — 只查红线级别问题
- **自动修复**：`review --fix` — 检测 + 自动修复常见问题

覆盖类别：数值运算安全、内存/指针安全、资源管理、输入验证、并发安全、内核 API 使用、ABI 兼容性、字符串操作、敏感信息、编码风格。

参考规范：[Linux 内核代码风格](https://docs.kernel.org/translations/zh_CN/process/coding-style.html)

### 2. kernel-driver-code-gen — 驱动代码生成

根据设计文档或硬件规格，生成完整的驱动代码框架：

- 支持：字符设备、平台驱动、块设备、网络设备
- 包含：中断处理、DMA、设备模型（sysfs/kobject）
- 输出：`.c` 源文件 + `Kconfig` + `Makefile`
- 可选：设备树绑定（DTS 节点）

提供可编译的模板和完整的自检清单。

### 3. kernel-perf-analysis — 性能诊断

结构化诊断 CPU、内存、I/O、网络四子系统的性能瓶颈：

- **L1**：只读、低开销观察（默认，生产安全）
- **L2**：短时定向采样（需批准）
- **L3**：跟踪/抓包/eBPF（需明确授权）

明确回答：瓶颈在哪个子系统、内核态还是应用态、哪个进程负责。

参考：[简述 Linux 性能分析](https://github.com/simple-tec/linux-performance-annlysis-skill)

### 4. linuxc-code-gen — 应用 C 代码生成

生成符合 POSIX/Linux 规范的高质量 C 应用代码：

- 覆盖：文件 I/O、进程管理、多线程、网络服务、信号处理、IPC、守护进程、内存管理
- 模式：socket/epoll 事件驱动、pthread 并发、goto 集中清理
- 特点：EINTR 处理、async-signal-safe、编译即用

### 5. time-based-scheduler — 定时调度

交互式配置时间段与任务映射，自动执行：

- 预设时段：早间/上午/下午/晚间/夜间
- 支持：自定义时间范围、工作日/周末限定
- 任务类型：shell 命令 + skill 调用

---

## 目录结构

```
EmbeddedDevSkills/
├── ReadMe.md
├── LICENSE
├── kernel-code-review/         # 内核代码检视
│   ├── SKILL.md
│   └── references/            # 10 类编码规范
├── kernel-driver-code-gen/    # 驱动代码生成
│   ├── SKILL.md
│   ├── references/            # 驱动类型参考 + API 参考
│   └── templates/             # 驱动模板 + Makefile
├── kernel-perf-analysis/      # 性能诊断
│   ├── SKILL.md
│   └── references/            # CPU/内存/I/O/网络指南 + 报告模板
├── linuxc-code-gen/           # 应用 C 代码生成
│   ├── SKILL.md
│   └── references/            # 8 个功能域参考
├── time-based-scheduler/      # 定时任务调度
│   ├── SKILL.md
│   └── templates/             # 配置文件模板
└── test/                      # 测试用例
    ├── kernelmodule/          # 测试用内核模块
    └── result/                # 测试输出
```

---

## 更新日志

- **2026.08.11** — 全面优化：标准化 frontmatter、精简 SKILL.md、重构 time-based-scheduler、增强 perf-analysis
- **2026.04.22** — 新增 linuxc-code-gen 技能
- **2026.04.20** — 更新 kernel-code-review，完善检视描述

---

## 规划路线

- **kernel-driver-code-gen**：增加 I2C/SPI/SDIO/PCIe 驱动模板，完善 RDTree 支持
- **kernel-perf-analysis**：适配 busybox 环境，增加嵌入式领域专用诊断命令
- **linuxc-code-gen**：增加用户自定义代码规范模板
- **time-based-scheduler**：增加 Cron 表达式支持和持久化调度

---

## 致谢

- 参考 [AscendC](https://gitcode.com/Ascend) 的设计思路
- 参考 [简述 Linux 性能分析](https://github.com/simple-tec/linux-performance-annlysis-skill)

---

## 关于作者

**AlexKing** — 嵌入式开发老兵，分享开发工作经历。

| 平台 | 链接 |
|------|------|
| 💬 公众号 | 微信搜「图布技术说」 |

<img src="Images/qrcode.jpg" alt="公众号二维码" width="360">

---

## 许可证

MIT — 随便用，随便改，随便造。

<div align="center">

MIT License © [AlexKing Tuubu](https://github.com/NingbinWang/)

</div>
