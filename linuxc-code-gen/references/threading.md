# 多线程参考

## 线程基础

### 创建与等待

```c
#include <pthread.h>

void *worker(void *arg)
{
    int id = *(int *)arg;
    printf("线程 %d 开始\n", id);
    // 工作...
    printf("线程 %d 结束\n", id);
    return NULL;
}

// 创建线程
pthread_t tid;
int arg = 1;
int ret = pthread_create(&tid, NULL, worker, &arg);
if (ret != 0) {
    fprintf(stderr, "pthread_create: %s\n", strerror(ret));
    return -1;
}

// 等待线程结束
ret = pthread_join(tid, NULL);
if (ret != 0) {
    fprintf(stderr, "pthread_join: %s\n", strerror(ret));
}

// 或者分离线程（自动回收资源）
pthread_detach(tid);
```

### 线程属性

```c
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
pthread_attr_setstacksize(&attr, 4 * 1024 * 1024);  // 4MB 栈

pthread_t tid;
pthread_create(&tid, &attr, worker, arg);

pthread_attr_destroy(&attr);
```

## 互斥锁

```c
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// 或动态初始化
pthread_mutex_t mtx;
pthread_mutex_init(&mtx, NULL);  // NULL = 默认属性

// 加锁
int ret = pthread_mutex_lock(&mtx);
if (ret != 0) {
    fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(ret));
    return -1;
}

// 临界区
shared_data++;

// 解锁
pthread_mutex_unlock(&mtx);

// 销毁
pthread_mutex_destroy(&mtx);
```

### 互斥锁类型

```c
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);

// 默认：正常锁（同一线程重复加锁会死锁）
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

// 错误检查（同一线程重复加锁返回错误）
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

// 递归锁（同一线程可以多次加锁，需对应次数解锁）
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

pthread_mutex_init(&mtx, &attr);
pthread_mutexattr_destroy(&attr);
```

## 条件变量

```c
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

// 等待方
pthread_mutex_lock(&mtx);
while (!ready) {  // 必须用 while，防止虚假唤醒
    pthread_cond_wait(&cond, &mtx);  // 原子释放锁 + 等待
}
// 使用共享数据
pthread_mutex_unlock(&mtx);

// 通知方
pthread_mutex_lock(&mtx);
ready = 1;
pthread_cond_signal(&cond);  // 唤醒一个等待线程
// 或 pthread_cond_broadcast(&cond);  // 唤醒所有等待线程
pthread_mutex_unlock(&mtx);

// 清理
pthread_cond_destroy(&cond);
```

### 带超时的等待

```c
#include <time.h>

struct timespec ts;
clock_gettime(CLOCK_REALTIME, &ts);
ts.tv_sec += 5;  // 5 秒超时

int ret = pthread_cond_timedwait(&cond, &mtx, &ts);
if (ret == ETIMEDOUT) {
    printf("超时\n");
}
```

## 读写锁

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// 读锁（多个读者可以同时持有）
pthread_rwlock_rdlock(&rwlock);
// 读取共享数据
pthread_rwlock_unlock(&rwlock);

// 写锁（独占）
pthread_rwlock_wrlock(&rwlock);
// 修改共享数据
pthread_rwlock_unlock(&rwlock);
```

## 线程局部存储

```c
// 编译器关键字
__thread int tls_var = 0;

// POSIX API
pthread_key_t key;
pthread_key_create(&key, NULL);  // 第二个参数是析构函数

pthread_setspecific(key, (void *)42);
int val = (int)pthread_getspecific(key);

pthread_key_delete(key);
```

## 屏障

```c
pthread_barrier_t barrier;
pthread_barrier_init(&barrier, NULL, 4);  // 4 个线程

void *worker(void *arg)
{
    // 阶段 1 工作...
    int ret = pthread_barrier_wait(&barrier);
    if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
        // 只有一个线程会收到这个返回值
        printf("所有线程已同步\n");
    }
    // 阶段 2 工作...
    return NULL;
}

pthread_barrier_destroy(&barrier);
```

## 线程安全函数

| 函数 | 线程安全版本 |
|------|-------------|
| strtok | strtok_r |
| rand | rand_r |
| ctime | ctime_r |
| getpwnam | getpwnam_r |
| strerror | strerror_r |
| gmtime | gmtime_r |
| localtime | localtime_r |

## 常见陷阱

1. **pthread 函数返回错误码，不用 errno** — 用 strerror(ret) 而非 perror()
2. **条件变量必须在 while 循环中等待** — 防止虚假唤醒
3. **互斥锁必须在条件变量 wait 前加锁** — pthread_cond_wait 会原子释放
4. **不要在持有锁时调用 fork** — 子进程只复制调用线程，其他线程的锁会死锁
5. **线程栈大小有限** — 默认通常 2-8MB，大数组用 malloc
6. **PTHREAD_MUTEX_INITIALIZER 只能用于静态初始化** — 动态分配用 pthread_mutex_init
