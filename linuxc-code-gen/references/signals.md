# 信号处理参考

## 信号安装

```c
#include <signal.h>

// 推荐使用 sigaction（而非 signal）
struct sigaction sa = {
    .sa_handler = handler,      // 或 .sa_sigaction 用于 SA_SIGINFO
    .sa_flags = SA_RESTART,     // 自动重启被中断的系统调用
};
sigemptyset(&sa.sa_mask);       // 处理期间额外屏蔽的信号

sigaction(SIGINT, &sa, NULL);
sigaction(SIGTERM, &sa, NULL);
```

## 信号处理函数规范

```c
// 只能使用 async-signal-safe 函数
// 安全：write, _exit, sig_atomic_t 变量
// 不安全：malloc, printf, lock, errno

static volatile sig_atomic_t g_shutdown = 0;

void handler(int sig)
{
    (void)sig;
    g_shutdown = 1;  // 只设置标志
}

// 主循环中检查
while (!g_shutdown) {
    // 正常工作...
}
```

## async-signal-safe 函数

POSIX 规定以下函数在信号处理中安全：

```
_exit, write, read, close, open, kill, getpid, sigaction,
sigprocmask, sigpending, alarm, pause, setitimer, fork,
execve, wait, waitpid, dup, dup2, pipe, socketpair,
accept, connect, send, recv, sendto, recvfrom, shutdown,
getuid, getgid, geteuid, getegid, setuid, setgid,
getaddrinfo, freeaddrinfo, sem_post
```

**不安全**：malloc, free, printf, fprintf, sprintf, strtok, rand, localtime, getpwnam, syslog

## 信号屏蔽

```c
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);

// 阻塞信号
sigprocmask(SIG_BLOCK, &mask, NULL);

// 解除阻塞
sigprocmask(SIG_UNBLOCK, &mask, NULL);

// 设置新的掩码
sigprocmask(SIG_SETMASK, &mask, NULL);

// 等待信号
sigset_t wait_mask;
sigfillset(&wait_mask);
sigdelset(&wait_mask, SIGINT);  // 只等 SIGINT
int sig = 0;
sigwait(&wait_mask, &sig);  // 同步等待信号
```

## 自管道技巧

将信号转换为可 epoll/select 的事件：

```c
static int g_pipefd[2];

void signal_handler(int sig)
{
    int saved_errno = errno;
    write(g_pipefd[1], &sig, sizeof(sig));  // async-signal-safe
    errno = saved_errno;
}

int setup_signal_pipe(void)
{
    if (pipe(g_pipefd) == -1) return -1;

    // 设为非阻塞
    int flags = fcntl(g_pipefd[0], F_GETFL, 0);
    fcntl(g_pipefd[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(g_pipefd[1], F_GETFL, 0);
    fcntl(g_pipefd[1], F_SETFL, flags | O_NONBLOCK);

    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = SA_RESTART,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    return g_pipefd[0];  // 返回读端，加入 epoll
}
```

## 常见陷阱

1. **信号处理函数中调用非 async-signal-safe 函数** — 可能死锁或崩溃
2. **用 signal() 而非 sigaction()** — signal 行为不可移植（System V vs BSD）
3. **忘记保存/恢复 errno** — 信号处理可能改变 errno
4. **信号丢失** — 标准信号不排队，连续同信号可能只触发一次。用 sigaction + SA_SIGINFO 检测
5. **多线程信号处理** — 信号会投递到任意线程，通常在主线程屏蔽所有信号，工作线程继承屏蔽
