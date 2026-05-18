# IPC 参考

## 管道

### 匿名管道（父子进程）

```c
int pipefd[2];
if (pipe(pipefd) == -1) ERR_EXIT("pipe");

pid_t pid = fork();
if (pid == 0) {
    close(pipefd[0]);  // 子进程关闭读端
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execlp("ls", "ls", NULL);
    _exit(127);
}
close(pipefd[1]);  // 父进程关闭写端
// 从 pipefd[0] 读取子进程输出
```

### 命名管道 (FIFO)

```c
#include <sys/stat.h>

// 创建
mkfifo("/tmp/myfifo", 0666);

// 写端
int fd = open("/tmp/myfifo", O_WRONLY);
write(fd, data, len);
close(fd);

// 读端
int fd = open("/tmp/myfifo", O_RDONLY);
read(fd, buf, sizeof(buf));
close(fd);
```

## 共享内存

### POSIX 共享内存

```c
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

// 创建/打开
int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
if (fd == -1) ERR_EXIT("shm_open");

// 设置大小
ftruncate(fd, sizeof(SharedData));

// 映射
SharedData *shm = mmap(NULL, sizeof(SharedData),
                        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (shm == MAP_FAILED) ERR_EXIT("mmap");

// 使用...

// 清理
munmap(shm, sizeof(SharedData));
close(fd);
shm_unlink("/myshm");  // 只需在一个进程中调用
```

### System V 共享内存

```c
#include <sys/ipc.h>
#include <sys/shm.h>

key_t key = ftok("/tmp", 'A');
int shmid = shmget(key, size, IPC_CREAT | 0666);
if (shmid == -1) ERR_EXIT("shmget");

void *shm = shmat(shmid, NULL, 0);
if (shm == (void *)-1) ERR_EXIT("shmat");

// 使用...

shmdt(shm);
shmctl(shmid, IPC_RMID, NULL);  // 删除
```

## 信号量

### POSIX 有名信号量

```c
#include <semaphore.h>
#include <fcntl.h>

// 创建
sem_t *sem = sem_open("/mysem", O_CREAT, 0666, 1);  // 初始值 1

sem_wait(sem);      // P 操作（阻塞）
sem_trywait(sem);   // P 操作（非阻塞）
sem_timedwait(sem, &ts);  // P 操作（超时）
sem_post(sem);      // V 操作

sem_close(sem);
sem_unlink("/mysem");
```

### POSIX 无名信号量（线程间）

```c
sem_t sem;
sem_init(&sem, 0, 0);  // 0 = 线程间，初始值 0

sem_wait(&sem);  // 等待
sem_post(&sem);  // 通知

sem_destroy(&sem);
```

## 消息队列

### POSIX 消息队列

```c
#include <mqueue.h>

struct mq_attr attr = {
    .mq_maxmsg = 10,
    .mq_msgsize = 256,
};

// 创建/打开
mqd_t mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0666, &attr);

// 发送
mq_send(mq, msg, len, 0);  // 优先级 0

// 接收
char buf[256];
unsigned int prio;
ssize_t n = mq_receive(mq, buf, sizeof(buf), &prio);

// 超时接收
struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec += 5;
mq_timedreceive(mq, buf, sizeof(buf), &prio, &ts);

mq_close(mq);
mq_unlink("/myqueue");
```

## IPC 选择指南

| 场景 | 推荐方式 | 原因 |
|------|----------|------|
| 父子进程单向数据流 | 匿名管道 | 最简单 |
| 无关进程单向数据流 | 命名管道 | 简单，文件系统接口 |
| 高速大量数据共享 | 共享内存 | 零拷贝 |
| 同步/互斥 | 信号量 | 原子操作 |
| 结构化消息传递 | 消息队列 | 有边界、有优先级 |
| 事件通知 | eventfd | 轻量级，可 epoll |

## eventfd（轻量级事件通知）

```c
#include <sys/eventfd.h>

int efd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);

// 通知（写入计数）
uint64_t val = 1;
write(efd, &val, sizeof(val));

// 等待（读取计数）
read(efd, &val, sizeof(val));

// 可加入 epoll
```

## 常见陷阱

1. **共享内存没有自带同步** — 必须配合信号量或互斥锁
2. **POSIX IPC 名字必须以 / 开头** — 如 "/myshm"
3. **管道读端关闭后写端会收到 SIGPIPE** — 需要处理或忽略
4. **mmap 后忘记 munmap** — 导致内存泄漏
5. **System V IPC 资源不自动清理** — 进程崩溃后需手动 ipcrm
