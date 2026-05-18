# 内存管理参考

## 堆内存

### malloc/calloc/realloc/free

```c
#include <stdlib.h>

// malloc：不初始化
void *p = malloc(size);
if (!p) {
    perror("malloc");
    return -1;
}

// calloc：零初始化
int *arr = calloc(count, sizeof(int));
if (!arr) {
    perror("calloc");
    return -1;
}

// realloc：调整大小
void *new_p = realloc(p, new_size);
if (!new_p) {
    // 原内存 p 仍然有效
    perror("realloc");
    free(p);
    return -1;
}
p = new_p;

// 释放
free(p);
p = NULL;  // 避免悬空指针
```

### 内存分配检查清单

```c
// 1. 检查返回值
void *buf = malloc(size);
if (!buf) goto cleanup;

// 2. 检查溢出（size * count 可能溢出）
if (count > SIZE_MAX / sizeof(*buf)) {
    errno = ENOMEM;
    goto cleanup;
}
buf = malloc(count * sizeof(*buf));

// 3. 大块内存考虑用 calloc（可能走 mmap，且零初始化）
buf = calloc(count, sizeof(*buf));
```

## 内存映射 (mmap)

### 匿名映射

```c
#include <sys/mman.h>

// 大块内存分配（避免堆碎片）
size_t len = 1024 * 1024 * 100;  // 100MB
void *p = mmap(NULL, len,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS,
               -1, 0);
if (p == MAP_FAILED) {
    perror("mmap");
    return -1;
}

// 使用...

// 释放
munmap(p, len);
```

### 文件映射

```c
int fd = open(path, O_RDONLY);
if (fd == -1) return -1;

struct stat st;
fstat(fd, &st);

void *p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
if (p == MAP_FAILED) {
    close(fd);
    return -1;
}

// 直接读取文件内容
char *data = (char *)p;

// 清理
munmap(p, st.st_size);
close(fd);
```

### 共享映射（进程间）

```c
// 创建共享内存对象
int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
ftruncate(fd, size);

void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
// 其他进程打开同一 shm_open 名字即可共享
```

## 内存池

### 简单固定大小池

```c
typedef struct {
    void *blocks;
    size_t block_size;
    size_t count;
    size_t free_count;
    void *free_list;
} Pool;

int pool_init(Pool *pool, size_t block_size, size_t count)
{
    pool->block_size = block_size < sizeof(void *) ? sizeof(void *) : block_size;
    pool->count = count;
    pool->free_count = count;
    pool->blocks = malloc(pool->block_size * count);
    if (!pool->blocks) return -1;

    // 构建空闲链表
    pool->free_list = NULL;
    for (size_t i = 0; i < count; i++) {
        void *block = (char *)pool->blocks + i * pool->block_size;
        *(void **)block = pool->free_list;
        pool->free_list = block;
    }
    return 0;
}

void *pool_alloc(Pool *pool)
{
    if (!pool->free_list) return NULL;
    void *block = pool->free_list;
    pool->free_list = *(void **)block;
    pool->free_count--;
    return block;
}

void pool_free(Pool *pool, void *block)
{
    *(void **)block = pool->free_list;
    pool->free_list = block;
    pool->free_count++;
}

void pool_destroy(Pool *pool)
{
    free(pool->blocks);
    pool->blocks = NULL;
}
```

## 对齐内存

```c
#include <stdlib.h>

// C11
void *p = aligned_alloc(64, size);  // 64 字节对齐

// POSIX
void *p;
posix_memalign(&p, 64, size);  // 对齐值必须是 2 的幂且是 sizeof(void*) 的倍数

free(p);
```

## 栈上变长数组（VLA）

```c
// C99/C11 支持，但不推荐用于大数组（栈溢出风险）
int n = get_count();
int arr[n];  // 栈上分配

// 更安全的替代方案
int *arr = calloc(n, sizeof(int));
if (!arr) return -1;
// 使用...
free(arr);
```

## mlock（锁定内存）

```c
#include <sys/mman.h>

// 锁定内存页，防止被换出（实时程序、安全敏感数据）
mlock(addr, len);
munlock(addr, len);

// 锁定所有未来分配
mlockall(MCL_CURRENT | MCL_FUTURE);
munlockall();
```

## 内存泄漏检测

### 工具

```bash
# Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./program

# AddressSanitizer（编译时启用）
gcc -fsanitize=address -g -o program program.c

# mtrace（glibc）
gcc -DMTRACE -o program program.c
MTRACE_OUTPUT=./mtrace.log ./program
mtrace ./program ./mtrace.log
```

### 自定义分配器（调试用）

```c
#ifdef MTRACE
#include <mcheck.h>
#endif

int main(void)
{
#ifdef MTRACE
    mtrace();
#endif
    // ...
#ifdef MTRACE
    muntrace();
#endif
    return 0;
}
```

## 常见陷阱

1. **malloc 后不检查返回值** — 内存不足时返回 NULL
2. **free 后继续使用** — 悬空指针，用后 free 或设 NULL
3. **double free** — 同一指针 free 两次，未定义行为
4. **内存泄漏** — malloc 后所有路径都要 free
5. **realloc 失败** — 原指针仍有效，不要直接 `p = realloc(p, size)`
6. **mmap 后忘记 munmap** — 导致虚拟地址空间泄漏
7. **栈上分配大数组** — 栈通常只有 2-8MB，大数组用 malloc/mmap
8. **calloc 不一定更快** — 但对大块内存可能走 mmap，且免去了手动零初始化
