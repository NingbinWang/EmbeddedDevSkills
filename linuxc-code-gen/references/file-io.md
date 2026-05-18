# 文件 I/O 参考

## 基础文件操作

### 打开与关闭

```c
#include <fcntl.h>
#include <unistd.h>

// 打开文件
int fd = open(path, O_RDONLY);          // 只读
int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // 写入，创建
int fd = open(path, O_RDWR | O_APPEND); // 追加
if (fd == -1) {
    perror("open");
    return -1;
}

// 关闭文件（必须检查，NFS 等远程文件系统可能延迟报错）
if (close(fd) == -1) {
    perror("close");
}
```

### 读写

```c
// 完整读取（处理 EINTR 和短读）
ssize_t read_all(int fd, void *buf, size_t count)
{
    size_t total = 0;
    while (total < count) {
        ssize_t n = read(fd, (char *)buf + total, count - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;  // EOF
        total += n;
    }
    return total;
}

// 完整写入（处理 EINTR 和短写）
ssize_t write_all(int fd, const void *buf, size_t count)
{
    size_t total = 0;
    while (total < count) {
        ssize_t n = write(fd, (const char *)buf + total, count - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return total;
}
```

### 定位

```c
off_t pos = lseek(fd, 0, SEEK_CUR);    // 当前位置
off_t pos = lseek(fd, 0, SEEK_SET);    // 文件开头
off_t pos = lseek(fd, 0, SEEK_END);    // 文件末尾
off_t pos = lseek(fd, -10, SEEK_CUR);  // 回退 10 字节
```

## 高级 I/O

### 分散/聚集 I/O (readv/writev)

```c
#include <sys/uio.h>

struct iovec iov[3];
iov[0].iov_base = header;
iov[0].iov_len = sizeof(header);
iov[1].iov_base = body;
iov[1].iov_len = body_len;
iov[2].iov_base = footer;
iov[2].iov_len = sizeof(footer);

ssize_t n = writev(fd, iov, 3);  // 一次系统调用写入三段
```

### 文件锁

```c
#include <fcntl.h>

struct flock fl = {
    .l_type = F_WRLCK,      // F_RDLCK / F_WRLCK / F_UNLCK
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 0,             // 0 = 整个文件
};

if (fcntl(fd, F_SETLKW, &fl) == -1) {  // F_SETLKW 阻塞等待
    perror("fcntl");
}
```

### 临时文件

```c
#include <stdlib.h>

char tmpl[] = "/tmp/myapp-XXXXXX";
int fd = mkstemp(tmpl);  // 创建并打开临时文件
if (fd == -1) ERR_EXIT("mkstemp");
// 使用完后
close(fd);
unlink(tmpl);
```

## 目录操作

```c
#include <sys/stat.h>
#include <dirent.h>

// 创建目录
mkdir(path, 0755);

// 遍历目录
DIR *dir = opendir(path);
if (!dir) ERR_EXIT("opendir");

struct dirent *entry;
while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0)
        continue;
    // 处理 entry->d_name
}
closedir(dir);
```

## 文件状态

```c
#include <sys/stat.h>

struct stat st;
if (stat(path, &st) == -1) ERR_EXIT("stat");

S_ISREG(st.st_mode)   // 普通文件
S_ISDIR(st.st_mode)   // 目录
S_ISLNK(st.st_mode)   // 符号链接
S_ISFIFO(st.st_mode)  // 管道
S_ISSOCK(st.st_mode)  // Socket

// 检查权限
if (access(path, R_OK) == -1) { /* 无读权限 */ }
```

## 常见陷阱

1. **短读/短写** — read/write 可能返回少于请求的字节数，必须循环
2. **EINTR** — 信号中断导致 read/write 返回 -1，必须重试
3. **close() 错误** — NFS 等场景下 close() 可能报错，必须检查
4. **O_CREAT 需要 mode** — 使用 O_CREAT 时必须提供第三个参数
5. **文件描述符泄漏** — fork 后子进程继承 fd，exec 前应关闭不需要的 fd（或设 FD_CLOEXEC）
