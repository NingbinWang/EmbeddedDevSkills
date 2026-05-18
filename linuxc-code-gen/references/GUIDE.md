# Linux C 代码生成 — 参考文件索引

根据功能类型加载对应参考文件。每个文件包含 API 速查、代码模式和常见陷阱。

| 文件 | 覆盖内容 | 何时加载 |
|------|----------|----------|
| [file-io.md](file-io.md) | open/read/write/close, lseek, readv/writev, 文件锁, 目录操作, stat | 文件读写、目录遍历、临时文件 |
| [process.md](process.md) | fork/exec/wait, 管道, popen, 信号基础, 环境变量 | 进程创建、命令执行、父子进程通信 |
| [threading.md](threading.md) | pthread, 互斥锁, 条件变量, 读写锁, 屏障, TLS, 线程安全函数 | 多线程、并发、同步 |
| [networking.md](networking.md) | TCP/UDP socket, epoll (LT/ET), Unix Socket, socket 选项 | 网络服务、客户端、事件驱动 |
| [signals.md](signals.md) | sigaction, 信号屏蔽, async-signal-safe 函数, 自管道技巧 | 信号处理、优雅退出、事件通知 |
| [ipc.md](ipc.md) | 管道, FIFO, POSIX/System V 共享内存, 信号量, 消息队列, eventfd | 进程间通信、共享数据、同步 |
| [daemon.md](daemon.md) | 双 fork, PID 文件, syslog, 信号处理, systemd 集成 | 守护进程、后台服务 |
| [memory.md](memory.md) | malloc/calloc/realloc, mmap, 内存池, 对齐, mlock, 泄漏检测 | 内存管理、大块分配、性能优化 |

## 加载规则

1. 读取需求文档，确定功能类型
2. 加载对应的参考文件（可加载多个）
3. 按参考文件中的代码模式生成代码
4. 按参考文件中的"常见陷阱"进行自检
