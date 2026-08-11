---
name: linuxc-code-gen
description: |
  根据需求或设计文档，生成符合 POSIX/Linux 规范的高质量 C 应用代码。
  覆盖系统调用、多线程、网络编程、IPC、信号处理、守护进程、内存管理。
triggers:
  - "生成 Linux C 代码"
  - "系统编程"
  - "写一个守护进程"
  - "多线程程序"
  - "socket 编程"
  - "epoll 服务"
  - "IPC 通信"
  - "信号处理"
  - "共享内存"
  - "内存映射"
  - "管道通信"
  - "TCP 服务端"
  - "UDP 通信"
  - "pthread"
  - "fork"
  - "mmap"
---

# Linux 应用层 C 代码生成

根据需求文档或用户描述，生成符合 POSIX/Linux 规范的高质量 C 应用代码。

## 核心原则

1. **安全优先** — 检查所有系统调用返回值，处理 EINTR，避免资源泄漏
2. **可移植性** — 优先使用 POSIX API，避免非标准扩展
3. **可维护性** — 清晰的错误处理路径，合理的模块划分
4. **性能意识** — 选择合适的 I/O 模型，避免不必要的拷贝

## 前置条件

需提供以下之一：需求文档路径 / 功能描述 / 设计文档路径。缺少时提示用户补充。

## 工作流

```
读取需求/设计文档 → 确定功能类型 → 加载对应参考文档
    → 选择代码模式 → 生成代码 → 自检清单 → 输出
```

### 阶段 1：需求分析

提取：功能类型、输入/输出、并发需求、网络需求、IPC 需求、错误策略。

### 阶段 2：功能类型 → 参考文档

先读取 [GUIDE.md](references/GUIDE.md) 获取索引，再加载对应文件。**MANDATORY**: 读取参考文件后再生成。

| 功能类型 | 参考文档 | 代码模式 |
|----------|----------|----------|
| 文件 I/O | [file-io.md](references/file-io.md) | read/write/mmap |
| 进程管理 | [process.md](references/process.md) | fork/exec/wait |
| 多线程 | [threading.md](references/threading.md) | pthread |
| 网络服务 | [networking.md](references/networking.md) | socket/epoll |
| 信号处理 | [signals.md](references/signals.md) | signal/sigaction |
| IPC | [ipc.md](references/ipc.md) | pipe/shm/mq |
| 守护进程 | [daemon.md](references/daemon.md) | daemon |
| 内存管理 | [memory.md](references/memory.md) | malloc/mmap/pool |

### 阶段 3：代码生成规范

**文件头模板**：
```c
/*
 * 文件名: xxx.c
 * 描述: 功能简述
 * 编译: gcc -Wall -Wextra -o xxx xxx.c -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* 宏 → 类型定义 → 函数声明 → 实现 → main() */
```

**关键规范**：

错误处理 — 所有系统调用检查返回值：
```c
int fd = open(path, O_RDONLY);
if (fd == -1) { fprintf(stderr, "open %s: %s\n", path, strerror(errno)); return -1; }
```

EINTR 处理 — 循环重试：
```c
ssize_t n;
do { n = read(fd, buf, sizeof(buf)); } while (n == -1 && errno == EINTR);
```

资源清理 — goto 集中清理：
```c
int do_work(void) {
    int fd = -1, ret = -1;
    void *buf = NULL;
    fd = open(path, O_RDONLY);
    if (fd == -1) goto cleanup;
    buf = malloc(size);
    if (!buf) goto cleanup;
    ret = 0;  // success
cleanup:
    if (buf) free(buf);
    if (fd >= 0) close(fd);
    return ret;
}
```

### 阶段 4：自检清单

- [ ] 所有系统调用检查返回值
- [ ] 所有 malloc/free 配对
- [ ] 所有 open/close 配对
- [ ] 所有 pthread_create 有对应 join 或 detach
- [ ] EINTR 处理（read/write/accept/connect）
- [ ] 信号处理函数只用 async-signal-safe 函数
- [ ] 网络地址使用 htonl/ntohs 字节序转换
- [ ] 编译命令含 `-Wall -Wextra`
- [ ] 无硬编码路径/端口（使用宏或参数）
- [ ] 清理路径覆盖所有错误分支

## 反模式清单

- **NEVER** 忽略系统调用返回值
- **NEVER** 信号处理函数中调用 malloc/printf/lock
- **NEVER** 使用 sprintf → 用 snprintf
- **NEVER** 使用 strcpy/strcat → 用 strncpy/strncat 或 strlcpy/strlcat
- **NEVER** 多线程中无保护使用全局变量
- **NEVER** 使用 sleep/usleep 做同步 → 用条件变量/定时器
- **NEVER** 忽略 EINTR
- **NEVER** main 外使用 exit() 不清理资源
- **NEVER** 使用 gets()
- **NEVER** 硬编码文件描述符

## 输出格式

1. **功能说明** — 程序功能描述
2. **编译命令** — 完整的 gcc 命令
3. **使用方法** — 命令行参数说明
4. **完整源码** — 可直接编译的 C 文件
5. **注意事项** — 已知限制、依赖条件
