---
name: linuxc-code-gen
description: |
  Linux应用层C代码生成技能。根据需求文档或设计文档，生成符合POSIX/Linux规范的C代码。
  覆盖：系统调用、多线程、网络编程、IPC、信号处理、事件驱动、内存管理。
  TRIGGER when: 用户要求生成Linux C应用代码、系统编程、网络服务、多线程程序、守护进程等。
  关键词：Linux C、系统编程、POSIX、线程、socket、epoll、daemon、IPC、信号、mmap。
---

# Linux 应用层 C 代码生成

根据需求文档或用户描述，生成符合 POSIX/Linux 规范的高质量 C 应用代码。

## 核心原则

1. **安全优先** — 检查所有系统调用返回值，处理 EINTR，避免资源泄漏
2. **可移植性** — 优先使用 POSIX API，避免非标准扩展
3. **可维护性** — 清晰的错误处理路径，合理的模块划分
4. **性能意识** — 选择合适的 I/O 模型，避免不必要的拷贝

## 前置条件

调用此技能时，需提供以下信息之一：

| 输入 | 说明 |
|------|------|
| 需求文档路径 | 读取文档后分析需求，生成代码 |
| 功能描述 | 直接描述要实现的功能 |
| 设计文档路径 | 读取后按设计生成代码 |

如果缺少输入，提示用户补充。

## 工作流程

```
读取需求/设计文档 → 确定功能类型 → 加载对应参考文档
    → 选择代码模式 → 生成代码 → 自检清单 → 输出
```

### 阶段 1：需求分析

从输入中提取：

| 提取项 | 用途 |
|--------|------|
| 功能类型 | 选择代码模式（见阶段2） |
| 输入/输出 | 确定数据流 |
| 并发需求 | 决定单线程/多线程/事件驱动 |
| 网络需求 | 决定 TCP/UDP/Unix Socket |
| IPC 需求 | 决定管道/共享内存/消息队列 |
| 错误处理要求 | 决定错误策略 |

### 阶段 2：确定功能类型与参考文档

先读取 [references/GUIDE.md](references/GUIDE.md) 获取参考文件索引，然后加载对应文件。

| 功能类型 | 参考文档 | 代码模式 |
|----------|----------|----------|
| 文件 I/O | [references/file-io.md](references/file-io.md) | read/write/mmap 模式 |
| 进程管理 | [references/process.md](references/process.md) | fork/exec/wait 模式 |
| 多线程 | [references/threading.md](references/threading.md) | pthread 模式 |
| 网络服务 | [references/networking.md](references/networking.md) | socket/epoll 模式 |
| 信号处理 | [references/signals.md](references/signals.md) | signal/sigaction 模式 |
| IPC | [references/ipc.md](references/ipc.md) | pipe/shm/mq 模式 |
| 守护进程 | [references/daemon.md](references/daemon.md) | daemon 模式 |
| 内存管理 | [references/memory.md](references/memory.md) | malloc/mmap/pool 模式 |

**MANDATORY**: 读取对应参考文件后再生成代码。

### 阶段 3：生成代码

#### 代码结构规范

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
/* 按需添加其他头文件 */

/* 宏定义 */
#define ERR_EXIT(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

/* 类型定义 */

/* 函数声明 */

/* 实现 */

int main(int argc, char *argv[])
{
    /* 参数解析 */
    /* 初始化 */
    /* 主逻辑 */
    /* 清理 */
    return 0;
}
```

#### 错误处理规范

**所有系统调用必须检查返回值**：

```c
// 文件操作
int fd = open(path, O_RDONLY);
if (fd == -1) {
    fprintf(stderr, "open %s: %s\n", path, strerror(errno));
    return -1;
}

// 内存分配
void *buf = malloc(size);
if (!buf) {
    perror("malloc");
    return -1;
}

// 线程操作
int ret = pthread_create(&tid, NULL, worker, arg);
if (ret != 0) {
    fprintf(stderr, "pthread_create: %s\n", strerror(ret));
    return -1;
}
```

**EINTR 处理**：

```c
ssize_t n;
do {
    n = read(fd, buf, sizeof(buf));
} while (n == -1 && errno == EINTR);
```

#### 资源清理规范

使用 goto 集中清理：

```c
int do_work(void)
{
    int fd = -1;
    void *buf = NULL;
    int ret = -1;

    fd = open(path, O_RDONLY);
    if (fd == -1) goto cleanup;

    buf = malloc(size);
    if (!buf) goto cleanup;

    /* 业务逻辑 */
    ret = 0;

cleanup:
    if (buf) free(buf);
    if (fd >= 0) close(fd);
    return ret;
}
```

#### 多线程规范

```c
#include <pthread.h>

// 线程安全的错误处理
// pthread 函数返回错误码，不用 errno
int ret = pthread_mutex_lock(&mtx);
if (ret != 0) {
    fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(ret));
    return -1;
}

// 分离线程（不需要 join）
pthread_detach(tid);

// 或者显式 join
pthread_join(tid, NULL);
```

#### 网络编程规范

```c
// TCP 服务端
int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
if (srv_fd == -1) ERR_EXIT("socket");

int opt = 1;
setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr.s_addr = INADDR_ANY,
};

if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    ERR_EXIT("bind");

if (listen(srv_fd, SOMAXCONN) == -1)
    ERR_EXIT("listen");

// 使用 epoll 进行事件驱动
int epfd = epoll_create1(0);
struct epoll_event ev = { .events = EPOLLIN, .data.fd = srv_fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, srv_fd, &ev);
```

### 阶段 4：自检清单

生成代码后，逐项检查：

- [ ] 所有系统调用检查返回值
- [ ] 所有 malloc/free 配对
- [ ] 所有 open/close 配对
- [ ] 所有 pthread_create/join 或 detach
- [ ] EINTR 处理（read/write/accept/connect）
- [ ] 信号处理函数只调用 async-signal-safe 函数
- [ ] 网络地址使用 htonl/ntohs 等字节序转换
- [ ] 编译命令包含 -Wall -Wextra
- [ ] 无硬编码路径/端口（使用宏或参数）
- [ ] 清理路径覆盖所有错误分支

## 反模式清单

- **NEVER** 忽略系统调用返回值
- **NEVER** 在信号处理函数中调用 malloc/printf/lock 等非 async-signal-safe 函数
- **NEVER** 使用 sprintf，使用 snprintf
- **NEVER** 使用 strcpy/strcat，使用 strncpy/strncat 或更好 strlcpy/strlcat
- **NEVER** 在多线程程序中使用全局变量而不加保护
- **NEVER** 使用 sleep/usleep 进行同步（使用条件变量或定时器）
- **NEVER** 忽略 EINTR（被信号中断的系统调用）
- **NEVER** 在 main 之外使用 exit() 而不清理资源
- **NEVER** 使用 gets()（已废弃，无缓冲区溢出保护）
- **NEVER** 硬编码文件描述符数字

## 输出格式

输出包含：

1. **功能说明** — 一段话描述程序功能
2. **编译命令** — 完整的 gcc 命令
3. **使用方法** — 命令行参数说明
4. **完整源码** — 可直接编译的 C 文件
5. **注意事项** — 已知限制、依赖条件
