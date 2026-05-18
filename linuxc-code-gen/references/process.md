# 进程管理参考

## 进程创建

### fork + exec 模式

```c
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t pid = fork();
if (pid == -1) {
    perror("fork");
    return -1;
}

if (pid == 0) {
    // 子进程
    // 重定向 stdin/stdout/stderr（可选）
    dup2(fd_in, STDIN_FILENO);
    dup2(fd_out, STDOUT_FILENO);
    close(fd_in);
    close(fd_out);

    // 执行新程序
    execlp("ls", "ls", "-la", NULL);
    // exec 只在失败时返回
    perror("execlp");
    _exit(127);  // 不能用 exit()，避免刷新父进程的 stdio 缓冲区
} else {
    // 父进程
    int status;
    pid_t w = waitpid(pid, &status, 0);
    if (w == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("子进程退出码: %d\n", code);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("子进程被信号 %d 终止\n", sig);
    }
}
```

### 孤儿进程与僵尸进程

```c
// 双 fork 避免僵尸进程
pid_t pid1 = fork();
if (pid1 == 0) {
    pid_t pid2 = fork();
    if (pid2 == 0) {
        // 孙进程：真正的工作者
        do_work();
        _exit(0);
    }
    // 子进程：立即退出，孙进程成为孤儿
    _exit(0);
}

// 父进程等待子进程（不是孙进程）
waitpid(pid1, NULL, 0);
// 孙进程由 init/systemd 收养，自动回收
```

## 信号处理

```c
#include <signal.h>

// 信号处理函数（只能调用 async-signal-safe 函数）
static volatile sig_atomic_t g_running = 1;

void sig_handler(int sig)
{
    (void)sig;
    g_running = 0;  // 只设置标志
}

// 安装信号处理器
struct saction sa = {
    .sa_handler = sig_handler,
    .sa_flags = SA_RESTART,  // 自动重启被中断的系统调用
};
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);
sigaction(SIGTERM, &sa, NULL);

// 主循环
while (g_running) {
    // 工作...
}
```

## 管道

### 匿名管道（父子进程间）

```c
int pipefd[2];
if (pipe(pipefd) == -1) ERR_EXIT("pipe");

pid_t pid = fork();
if (pid == 0) {
    close(pipefd[0]);  // 关闭读端
    write(pipefd[1], "hello", 5);
    close(pipefd[1]);
    _exit(0);
}

close(pipefd[1]);  // 关闭写端
char buf[16];
ssize_t n = read(pipefd[0], buf, sizeof(buf));
close(pipefd[0]);
waitpid(pid, NULL, 0);
```

### popen（简化版）

```c
FILE *fp = popen("ls -la", "r");
if (!fp) ERR_EXIT("popen");

char line[256];
while (fgets(line, sizeof(line), fp)) {
    printf("%s", line);
}

int status = pclose(fp);
```

## 环境变量

```c
#include <stdlib.h>

const char *val = getenv("HOME");
if (!val) val = "/tmp";

setenv("MY_VAR", "value", 1);  // 1 = 覆盖
unsetenv("MY_VAR");
```

## 常见陷阱

1. **fork 后忘记关闭不需要的管道端** — 导致 read 永远阻塞
2. **exec 失败后用 exit() 而非 _exit()** — 会刷新父进程 stdio 缓冲区
3. **waitpid 返回 -1 不处理** — 可能是 EINTR
4. **信号处理函数中调用非 async-signal-safe 函数** — malloc/printf/lock 等都不安全
5. **fork 后多线程环境** — 只有调用线程在子进程中存活，其他线程的锁可能死锁
