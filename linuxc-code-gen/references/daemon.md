# 守护进程参考

## 经典双 fork 方式

```c
#include <sys/stat.h>
#include <fcntl.h>

int daemonize(void)
{
    // 第一次 fork：脱离终端
    pid_t pid = fork();
    if (pid == -1) return -1;
    if (pid > 0) _exit(0);  // 父进程退出

    // 创建新会话
    if (setsid() == -1) return -1;

    // 第二次 fork：防止会话领头进程获取控制终端
    pid = fork();
    if (pid == -1) return -1;
    if (pid > 0) _exit(0);

    // 设置文件权限掩码
    umask(0022);

    // 切换到根目录（避免阻止文件系统卸载）
    if (chdir("/") == -1) return -1;

    // 关闭标准文件描述符，重定向到 /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd = open("/dev/null", O_RDWR);  // fd = 0
    if (fd != STDIN_FILENO) return -1;
    dup2(fd, STDOUT_FILENO);  // fd = 1
    dup2(fd, STDERR_FILENO);  // fd = 2
    if (fd > STDERR_FILENO) close(fd);

    return 0;
}
```

## 使用 daemon() 函数（glibc）

```c
#include <unistd.h>

// 简化版，但不可移植（BSD/glibc 扩展）
if (daemon(0, 0) == -1) ERR_EXIT("daemon");
// 参数：nochdir=0（切换到/），noclose=0（重定向到/dev/null）
```

## PID 文件

```c
int write_pid_file(const char *path)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d\n", getpid());

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return -1;

    ssize_t n = write(fd, buf, strlen(buf));
    close(fd);
    return (n > 0) ? 0 : -1;
}

// 清理
unlink(pid_path);
```

## 日志（syslog）

```c
#include <syslog.h>

// 打开日志
openlog("mydaemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);

// 写日志
syslog(LOG_INFO, "服务启动，监听端口 %d", port);
syslog(LOG_WARNING, "连接数接近上限: %d/%d", cur, max);
syslog(LOG_ERR, "数据库连接失败: %s", strerror(errno));

// 关闭
closelog();
```

### 日志级别

| 级别 | 说明 |
|------|------|
| LOG_EMERG | 系统不可用 |
| LOG_ALERT | 需要立即处理 |
| LOG_ERR | 错误 |
| LOG_WARNING | 警告 |
| LOG_NOTICE | 正常但重要 |
| LOG_INFO | 信息 |
| LOG_DEBUG | 调试 |

## 信号处理（优雅退出）

```c
static volatile sig_atomic_t g_running = 1;

void shutdown_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

// 安装信号处理器
struct sigaction sa = {
    .sa_handler = shutdown_handler,
    .sa_flags = 0,  // 不用 SA_RESTART，让阻塞调用返回
};
sigemptyset(&sa.sa_mask);
sigaction(SIGTERM, &sa, NULL);
sigaction(SIGINT, &sa, NULL);
sigaction(SIGQUIT, &sa, NULL);

// 忽略 SIGHUP（终端关闭时）
signal(SIGHUP, SIG_IGN);

// 主循环
while (g_running) {
    // 接受连接、处理请求...
    // 阻塞调用（accept/epoll_wait）会因信号返回 -1 + EINTR
}

// 清理
syslog(LOG_INFO, "服务关闭");
closelog();
unlink(pid_path);
```

## systemd 集成

### 服务文件 /etc/systemd/system/mydaemon.service

```ini
[Unit]
Description=My Daemon
After=network.target

[Service]
Type=forking
PIDFile=/run/mydaemon.pid
ExecStart=/usr/local/bin/mydaemon
ExecStop=/bin/kill -TERM $MAINPID
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Type=simple 模式（推荐）

不需要 daemon()，直接前台运行：

```c
int main(void)
{
    // 不调用 daemon()，systemd 管理生命周期
    setup_signals();
    openlog("mydaemon", LOG_PID, LOG_DAEMON);
    // 主循环...
}
```

## 常见陷阱

1. **忘记 umask** — 可能导致创建的文件权限不对
2. **忘记重定向 stdin/stdout/stderr** — 某些库会写 stdout 导致异常
3. **PID 文件残留** — 进程异常退出后 PID 文件还在，启动时需要处理
4. **不检查已运行实例** — 用 flock 或 PID 文件防止重复启动
5. **systemd 环境下用 daemon()** — Type=forking 需要，Type=simple 不需要
