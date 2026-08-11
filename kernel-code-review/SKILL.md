---
name: kernel-code-review
description: |
  Linux 内核与驱动 C 代码安全检视。基于假设检验方法论，对照 10 类编码规范逐条检查，
  支持全量检视、聚焦检视、快速扫描三种模式，输出结构化报告并支持自动修复。
triggers:
  - "代码检视"
  - "代码 review"
  - "代码审查"
  - "代码审计"
  - "安全规范检查"
  - "内核代码检查"
  - "checkpatch"
  - "内存泄漏检查"
  - "空指针检查"
  - "整数溢出检查"
  - "竞态条件检查"
  - "DMA 安全检查"
  - "编码规范"
  - "quick-scan"
---

# Linux 内核 C 代码检视

## 核心原则

1. **合规优先** — 所有检视映射至编码规范具体条款
2. **可审计** — 检视过程全程记录，问题可追溯
3. **实用导向** — 优先发现高危问题，过滤误报

## 调用方式

| 调用方式 | 行为 |
|----------|------|
| `review <文件/目录>` | 全量检视，遍历 10 类规范 |
| `review <文件/目录> <类别>` | 聚焦检视，只检查指定类别 |
| `review <代码片段> <类别>` | 片段检视（原始模式） |
| `quick-scan <文件/目录>` | 快速扫描，只查红线级别问题 |
| `review --fix` | 报告 + 自动应用修复 |

**类别关键词 → 参考文件映射**：

| 关键词 | 参考文件 |
|--------|----------|
| 数值运算、溢出、除零、回绕 | [01_numeric_operations.md](references/01_numeric_operations.md) |
| 内存、指针、空指针、野指针、UAF | [02_memory_pointer_safety.md](references/02_memory_pointer_safety.md) |
| 资源泄漏、内存泄漏、fd泄漏 | [03_resource_management.md](references/03_resource_management.md) |
| 输入验证、用户态、copy_from_user | [04_input_validation.md](references/04_input_validation.md) |
| 并发、锁、竞态、中断、RCU | [05_concurrency_safety.md](references/05_concurrency_safety.md) |
| 内核API、kmalloc、设备模型、devm | [06_kernel_api_usage.md](references/06_kernel_api_usage.md) |
| ABI、接口兼容性、符号导出 | [07_abi_compatibility.md](references/07_abi_compatibility.md) |
| 字符串、格式化、strcpy | [08_string_operations.md](references/08_string_operations.md) |
| 敏感信息、密码、硬编码 | [09_sensitive_information.md](references/09_sensitive_information.md) |
| 编码规范、命名、排版、风格 | [10_kernel_style_rules.md](references/10_kernel_style_rules.md) |

未指定类别时 → 全量检视。

## 工作流

### 模式 A：全量检视（默认）

```
1. 获取代码 → 2. [可选] 运行 checkpatch.pl
3. 逐类加载参考文件，按优先级检视：
   红线类（01数值、02内存、04输入验证）→ 最先
   高危类（03资源、05并发、06API）→ 其次
   规范类（07ABI、08字符串、09敏感、10风格）→ 最后
4. 汇总报告
```

### 模式 B：聚焦检视

```
1. 获取代码 → 2. 加载指定类别的参考文件
3. 只检查该类别下的规范条款 → 4. 输出该类别的问题
```

### 模式 C：快速扫描

```
1. 获取代码 → 2. 只检查红线清单
3. 只输出高危问题，不检查规范类问题
```

## 红线清单

以下问题无论上下文都必须报告：

1. `kmalloc/kzalloc/vmalloc` 返回值未检查 → 空指针解引用 → 内核崩溃
2. `copy_from_user/copy_to_user` 返回值未检查 → 任意地址读写
3. 中断上下文调用可能睡眠的函数 → 死锁
4. `kfree` 后继续使用指针（UAF）→ 任意代码执行
5. 用户态指针直接解引用 → 任意地址读写
6. 无符号整数回绕导致绕过检查 → 堆溢出
7. 有符号整数溢出 → 未定义行为
8. 除零 → 内核 Panic
9. `spin_lock` 与中断上下文共享变量未用 `irqsave` → 死锁
10. DMA 地址未检查 `dma_mapping_error` → 数据损坏

## 评分与误报过滤

| 等级 | 判定条件 |
|------|----------|
| **高** | 命中红线清单，或可直接导致崩溃/提权/信息泄露 |
| **中** | 违反规范但需特定条件触发，或防御缺失 |
| **低** | 编码风格问题、潜在可维护性问题 |

**误报过滤**：已有有效防御、上下文可证明不可能触发、内核已有保护机制、devm_ 自动管理 → 跳过。

## 自动修复

| 模式 | 修复 |
|------|------|
| `kmalloc` 未检查返回值 | 添加 `if (!ptr) return -ENOMEM;` |
| `copy_from_user` 未检查 | 添加 `if (copy_from_user(...)) return -EFAULT;` |
| `kfree` 后未置空 | 添加 `ptr = NULL;` |
| 中断上下文用 GFP_KERNEL | 改为 GFP_ATOMIC |
| `spin_lock` 未 irqsave | 改为 `spin_lock_irqsave` |
| 无符号回绕风险 | 改为先减后比较 |

## 检视分析方法

**假设检验驱动**：H0（安全）vs H1（存在风险）→ 收集证据 → 判定。

**分析要求**：跟踪函数定义、追踪数据流、验证返回值检查、审计 DMA/中断/并发安全。

## 输出格式

**默认简洁模式**：

```
## 代码检视报告

检视范围：[文件列表] | 检视类别：[全量/聚焦类别]
问题总数：高危 N / 中危 N / 低危 N

### 高危问题
1. [文件:行号] 问题描述
   风险代码：`引用` | 规范：02_memory_pointer_safety.md §1.1
   修复：`修复代码`
2. ...

### 中危问题 / ### 低危问题
...
```

**详细模式**（用户明确要求时启用）：增加证据链、调用链/数据流分析、规范条款引用。

## 注意事项

1. 检视前先读取对应规范文件，只按规范条款检查
2. 存疑问题标记"存疑"，供用户判断
3. 区分用户态和内核态规范差异
4. 驱动代码额外关注：设备模型、中断上下文、DMA、设备树
5. 不确定的问题宁可漏报也不误报
