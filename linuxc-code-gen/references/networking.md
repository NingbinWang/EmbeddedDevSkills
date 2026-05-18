# 网络编程参考

## TCP 服务端

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int create_server(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;

    // 允许端口复用
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        close(fd);
        return -1;
    }

    return fd;
}
```

## TCP 客户端

```c
int connect_to(const char *host, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(fd);
        return -1;
    }

    return fd;
}
```

## epoll 事件驱动

```c
#include <sys/epoll.h>

#define MAX_EVENTS 64

int epfd = epoll_create1(0);
if (epfd == -1) ERR_EXIT("epoll_create1");

// 添加监听 fd
struct epoll_event ev = {
    .events = EPOLLIN,
    .data.fd = srv_fd,
};
epoll_ctl(epfd, EPOLL_CTL_ADD, srv_fd, &ev);

// 事件循环
struct epoll_event events[MAX_EVENTS];
while (running) {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        if (errno == EINTR) continue;
        ERR_EXIT("epoll_wait");
    }

    for (int i = 0; i < nfds; i++) {
        int fd = events[i].data.fd;

        if (fd == srv_fd) {
            // 新连接
            int client = accept(srv_fd, NULL, NULL);
            if (client == -1) continue;

            // 设为非阻塞
            int flags = fcntl(client, F_GETFL, 0);
            fcntl(client, F_SETFL, flags | O_NONBLOCK);

            ev.events = EPOLLIN | EPOLLET;  // 边沿触发
            ev.data.fd = client;
            epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
        } else {
            // 客户端数据
            char buf[4096];
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) {
                // 连接关闭或错误
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            } else {
                // 处理数据
                write(fd, buf, n);  // echo
            }
        }
    }
}
```

## 边沿触发 vs 水平触发

| 模式 | 标志 | 行为 |
|------|------|------|
| 水平触发（默认） | 0 | 只要 fd 可读/可写就持续通知 |
| 边沿触发 | EPOLLET | 仅在状态变化时通知一次，必须一次性读完 |

**边沿触发必须**：
1. 使用非阻塞 fd
2. 循环读取直到返回 EAGAIN
3. 循环写入直到数据发送完毕或返回 EAGAIN

```c
// 边沿触发读取
while (1) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n == -1) {
        if (errno == EAGAIN) break;  // 数据读完
        // 错误处理
        break;
    }
    if (n == 0) {
        // 连接关闭
        break;
    }
    // 处理 buf[0..n-1]
}
```

## UDP

```c
int fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr.s_addr = INADDR_ANY,
};
bind(fd, (struct sockaddr *)&addr, sizeof(addr));

// 接收
struct sockaddr_in from;
socklen_t fromlen = sizeof(from);
ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                     (struct sockaddr *)&from, &fromlen);

// 发送
sendto(fd, buf, n, 0,
       (struct sockaddr *)&from, sizeof(from));
```

## Unix Socket

```c
#include <sys/un.h>

int fd = socket(AF_UNIX, SOCK_STREAM, 0);

struct sockaddr_un addr = { .sun_family = AF_UNIX };
strncpy(addr.sun_path, "/tmp/my.sock", sizeof(addr.sun_path) - 1);

// 服务端
unlink("/tmp/my.sock");  // 清理旧 socket
bind(fd, (struct sockaddr *)&addr, sizeof(addr));
listen(fd, 5);

// 客户端
connect(fd, (struct sockaddr *)&addr, sizeof(addr));
```

## socket 选项速查

```c
// 地址复用
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 端口复用（Linux 3.9+）
setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

// TCP_NODELAY（禁用 Nagle 算法）
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

// 发送/接收超时
struct timeval tv = { .tv_sec = 5 };
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

// 保活
setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
```

## 常见陷阱

1. **字节序** — 网络字节序是大端，用 htons/htonl/ntohs/ntohl 转换
2. **SIGPIPE** — 向已关闭的 socket 写入会收到 SIGPIPE，程序可能意外终止。用 signal(SIGPIPE, SIG_IGN) 或 send(..., MSG_NOSIGNAL)
3. **accept 返回 -1** — 可能是 ECONNABORTED 或 EINTR，应该继续循环
4. **短写** — send 可能只发送部分数据，需要循环
5. **非阻塞 connect** — 返回 -1 且 errno == EINPROGRESS 表示连接中，用 epoll 监听可写事件
