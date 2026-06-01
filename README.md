# ALib

ALib 是一个面向 C11 + GNU 扩展的底层工具库，定位上对标 GLib，但设计路线并不相同：
它把“泛型容器、对象生命周期、轻量类系统、进程内信号、线程兼容层、锁封装、简单文件读写”组合进一套偏编译期驱动的接口里，目标是在纯 C 中获得比传统 `void*` 工具库更强的类型约束和更统一的值语义体验。

当前仓库产物为静态库：Linux/Unix 下默认生成 `libalib.a`，MinGW-w64 交叉编译 Windows 目标时生成 `alib.lib`。公共头文件位于 `inc/`，安装后按 `<alib/...>` 方式引用。

## 项目定位

如果把 ALib 和 GLib 放在同一条线上看，可以把它理解为：

- ALib 仍然是 C 库，适合做底层组件和通用基础设施。
- ALib 不追求复刻 GLib 全家桶，而是聚焦容器、生命周期和对象组织方式。
- ALib 的核心卖点不是“更多运行时设施”，而是“更强的编译期类型绑定 + 更统一的对象语义”。
- ALib 更像“偏 STL 风格的 C 工具库”，而不是 “GLib 兼容层”。

一句话概括：GLib 更像成熟的 C 运行时工具生态，ALib 更像把泛型容器和值语义搬进 C 的实验型底层库。

## ALib 与 GLib 的差异

| 维度 | ALib | GLib / GObject 生态 |
| --- | --- | --- |
| 泛型方式 | `ALine_Define(T)` / `AHash_Define(K,V)` 这类宏在编译期绑定类型 | 大量接口以 `gpointer`、回调和运行时约定组织 |
| 生命周期 | `A_INIT` / `A_COPY` / `A_DEST` / `RAII(T)` 统一对象生命周期 | 以 `g_*_new/free`、引用计数和手动释放为主 |
| 容器语义 | 容器元素默认按值拷贝、按值析构 | 更常见的是指针/句柄语义，由调用方维护元素对象 |
| 类型信息来源 | `A_TYPE_REGISTER` / `A_CLASS_REGISTER` 约定 init/copy/dest/cmpd/hash | 由 API 约定、回调、`GType`/`GObject` 等运行时机制承担 |
| 对象系统 | 轻量单继承 + 虚函数表，无属性系统、无反射 | GObject 提供更完整的类型系统、信号、属性和 introspection |
| 字符串能力 | `AString` 处理字节串，`AText` 处理 UTF-8 文本与基础编码转换 | GLib 提供 `GString`、更完整的 UTF-8/Unicode、路径和文本工具 |
| 并发能力 | `athrd.h` 提供兼容 C11 `<threads.h>` 的线程原语，`alock.h` 再封装锁对象 | GLib 线程、主循环、异步设施更成熟 |
| 功能边界 | 容器、指针、类、信号、线程兼容层、锁、基础文件读写 | 还覆盖主循环、IO、文件路径、模块装载、字符集等 |

这意味着：

- 如果你需要 `GMainLoop`、更完整的 UTF-8/Unicode 工具、文件系统抽象、成熟的对象反射体系，优先考虑 GLib。
- 如果你明确只写 C，又希望容器和对象生命周期尽量统一、尽量类型化，ALib 更贴近它的目标场景。
- ALib 不是 GLib API 兼容层，也不试图替代整个 GLib 生态。

## 当前提供的模块

- `alib.h`：基础类型系统、异常槽、对象生命周期宏、分配器钩子、RAII 宏
- `aiter.h`：统一迭代器协议与 `forEach` / `forEachRev`
- `aclass.h`：轻量单继承类系统、虚函数表、多态调用
- `aline.h`：动态数组
- `alist.h`：双向链表
- `adeque.h`：分块双端队列
- `astack.h`：栈（基于双端队列）
- `aqueue.h`：队列（基于双端队列）
- `asortque.h`：有序数组队列
- `atree.h`：红黑树映射
- `ahash.h`：哈希映射
- `astring.h`：字符串对象
- `atext.h`：UTF-8 文本对象与 UTF-16/UTF-32/GBK 转换
- `aptr.h`：独占指针包装和共享指针包装
- `atimer.h`：`AClock` 时间点辅助和全局毫秒级定时任务调度
- `asignal.h`：进程内信号连接/派发系统
- `athrd.h`：兼容 C11 `<threads.h>` 的线程原语入口；优先复用系统实现，缺失时回退到 POSIX/Win32
- `alock.h`：基于 `athrd.h` 的互斥锁、递归锁、读写锁、条件变量锁、信号量及自动解锁辅助对象
- `afile.h`：文件读写器、追加器、内存读取器，以及设备对象雏形

其中 `athrd.h` 适合直接做线程创建、条件变量等待和线程局部存储；如果只是想在 ALib 风格代码里管理临界区、等待条件或限制并发配额，通常直接使用 `alock.h` 的 `AAutoKey`、`AMtxCnd`、`ASemaphore` 更顺手。

## 构建、测试与安装

ALib 当前使用仓库内的 GNU Makefile 构建。顶层 `Makefile` 会读取 `config/*.mk` 中的配置，默认在 Linux 上使用 `config/linux.mk`，也可以通过 `CONFIG=<name>` 显式指定。

### 常用本机构建

```bash
make
```

使用 Clang：

```bash
make CONFIG=linux-clang
```

清理构建产物：

```bash
make clean
```

清理已保存的 `.config`：

```bash
make disclean
```

### 可用 CONFIG

当前仓库内置配置文件位于 `config/`：

- `linux`：Linux / Unix 本地默认构建，使用 `cc` 和 `ar`。
- `linux-clang`：Linux / Unix 本地 Clang 构建，使用 `clang` 和 `llvm-ar`。
- `linux-arm`：Linux 到 AArch64 的交叉编译，使用 `aarch64-linux-gnu-gcc`。
- `linux-mips`：Linux 到 MIPS 的交叉编译，使用 `mips-linux-uclibc-gnu-gcc`。

新增编译配置时，只需在 `config/` 下增加新的 `<name>.mk`，之后使用 `make CONFIG=<name>` 即可。顶层构建会把本次配置复制到 `.config`，之后未显式传入 `CONFIG` 时会继续沿用它。

### 输出目录

默认会同时构建库、`sample/` 示例和 `test/` 测试。构建输出固定在：

- 中间文件：`build/*.o`
- 临时头文件链接：`build/.include/alib`
- 静态库：`build/out/libalib.a` 或 `build/out/alib.lib`
- 示例程序：`build/out/sample_*.<out|exe>`
- 测试程序：`build/out/test_*.<out|exe>`

### 交叉编译

AArch64 示例：

```bash
make CONFIG=linux-arm
```

MIPS 示例：

```bash
make CONFIG=linux-mips
```

交叉编译通常只能验证目标程序已经生成；若要在 Linux / WSL2 中运行 Windows `.exe` 测试，需要额外安装 Wine，或复制到 Windows 环境运行。

### 安装

```bash
make install
```

默认安装位置：

- `linux` / `linux-clang`：头文件安装到 `/usr/local/include/alib/`，静态库安装到 `/usr/local/lib/`。
- `linux-arm` / `linux-mips` ：头文件安装到 `$HOME/.alib/include/alib/`，静态库安装到 `$HOME/.alib/lib/`。

卸载：

```bash
make uninstall
```

## 快速开始

下面的例子展示 `ALine(int)` 的定义、实例化和遍历：

```c
#include <alib/alib.h>
#include <alib/aline.h>
#include <stdio.h>

ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));

int main(void) {
    RAII(ALine(int)) line = A_INIT(ALine(int));
    if (aExcOccur()) {
        return 1;
    }

    for (int i = 0; i < 5; ++i) {
        line.f->pushBack(&line, i * 10);
        if (aExcOccur()) {
            return 1;
        }
    }

    forEach(it, line) {
        printf("%d ", *it.p);
    }
    printf("\n");
    return 0;
}
```

安装后可直接编译（Linux / GCC）：

```bash
gcc -std=gnu11 -Wall -Wextra -O2 example.c -lalib -lpthread -latomic -o example
```

如果只是想在仓库目录里临时编译（Linux / GCC）：

```bash
make CONFIG=linux

gcc -std=gnu11 -Wall -Wextra -O2 -Ibuild/.include example.c build/out/libalib.a -lpthread -latomic -o example
```

## 使用模式

### 1. 为自定义类型注册生命周期

ALib 的容器和指针包装依赖类型注册。对自定义类型，通常至少需要提供：

- `A_OBJ_INIT(T)`
- `A_OBJ_COPY(T)`
- `A_OBJ_DEST(T)`
- `A_OBJ_CMPD(T)`

然后用 `A_TYPE_REGISTER(T)` 注册。

如果某些函数不提供，ALib 会退回默认行为：零初始化、按字节复制、自动比较或原始内存比较。

### 2. 生成泛型容器

序列容器和映射容器都按“定义 + 生成 + 注册”三步走：

```c
AList_Define(AString);
AList_Generate(AString);
A_TYPE_REGISTER(AList(AString));
```

映射容器使用键值双类型：

```c
ATree_Define(int, AString);
ATree_Generate(int, AString);
A_TYPE_REGISTER(ATree(int, AString));
```

### 3. 统一使用对象语义

- 栈上对象：`A_INIT(T)` / `A_COPY(T, obj)` / `A_DEST(T, obj)`
- 堆上对象：`A_NEW(T)` / `A_CPNEW(T, obj)` / `A_DELETE(T, p)`
- 自动析构：`RAII(T)`

### 4. 在并发场景里优先用锁对象

- `AMtx` / `ARecursion` / `AMtxRW` 适合普通临界区、递归调用和读多写少的共享状态。
- `AMtxCnd` 把互斥锁和条件变量绑在一起：先 `AMtxCnd_lock()`，在谓词不满足时循环 `AMtxCnd_wait()`，状态更新后再 `AMtxCnd_awake()` 或 `AMtxCnd_awake_all()`。
- `ASemaphore` 把“占用一个名额 / 归还一个名额”封装进 `ASemaphore_lock()` 返回的 `AAutoKey`；首次使用前需要先 `ASemaphore_setMax()` 设置上限，之后可以按需动态调整容量。

### 5. 文件读取与写入

`afile.h` 提供面向对象风格的文件读写包装：

```c
RAII(ARFile) in = aFileOpen("input.bin");
RAII(AWFile) out = aFileCreate("output.bin");
char buf[256];
uint32_t n = in.f->read(&in, sizeof(buf), buf);
out.f->write(&out, n, buf);
```

常用入口：

- `aFileOpen(name)`：以读取模式打开文件，返回 `ARFile`。
- `aFileCreate(name)`：以写入模式创建或截断文件，返回 `AWFile`。
- `aFileAppend(name)`：以追加模式打开文件，返回 `APFile`。
- `aMemoryOpen(mem, size)`：把一段已有内存包装成只读 `ARFile`。
- `aDevOpen(name)` / `aDevOpen_nb(name)`：以阻塞 / 非阻塞模式打开设备，返回 `ADev`；POSIX 下包装 `open/read/write/ioctl`，Windows 下包装 `CreateFileA/ReadFile/WriteFile/DeviceIoControl`。

Windows 下 `ADev_ioctl()` 的第三个参数应传 `ADevIoctl*`，用于提供 `DeviceIoControl` 的输入缓冲、输出缓冲和返回字节数；POSIX 下则保持传统 `ioctl(fd, cmd, buf)` 语义。

## 需要特别注意的语义

- `AString_new()` 和 `AText_new()` 都只是“包装已有 `char*`”，不会立刻拷贝；传入栈缓冲区或临时内存时，必须在其失效前复制到一个真正拥有内存的对象。
- `aFileOpen()` / `aFileCreate()` / `aFileAppend()` 失败时会设置 `AEXC_file_noexist`；使用前应检查 `aExcOccur()`。
- `aMemoryOpen()` 只借用传入内存，不复制；调用方必须保证 `mem` 在 `ARFile` 使用期间有效。
- `aDevOpen()` / `aDevOpen_nb()` 失败时会设置 `AEXC_system_error`；Windows 非阻塞模式使用 overlapped I/O，当前实现遇到未完成的异步操作会返回 `0`。
- 顺序容器的 `at(index)` 在“容器非空但 index 越界”时，通常会截断到尾元素；只有空容器访问才会设置 `AEXC_overstep`。
- `AHash(K,V)` 和 `ATree(K,V)` 的 `ins()` 是 upsert 语义：同键再次插入会替换已有值。
- `a_signal_connection(id, addressee, call)` 对同一个 `(id, addressee)` 重复连接时，会覆盖原有回调，而不是忽略此次连接。
- `AHash(K,V)` 的遍历顺序依赖桶布局与插入路径，不应把它当成稳定顺序容器。
- 迭代器更适合只读遍历或就地修改元素值，不适合边遍历边插入/删除；只要容器结构发生变化，就应丢弃旧迭代器并重新获取。
- `APtr(T)` 不是 `unique_ptr` 的等价物：复制后新对象是弱别名，不会接管释放责任；如果需要转移包装对象本身，请使用 `APtrCvs(T, ptr)` 或 `A_MOVE`。
- `APtrCvs(T, ptr)` / `AShPtrCvs(T, sp)` 会按字节取走原包装对象并把原对象清零；它们不复制底层对象，`AShPtrCvs` 也不会增加引用计数，适合显式移动指针包装。
- `AReceEnd` 可以在析构时自动断开信号连接；信号回调内部也可以断连，但实际操作会延迟到本轮派发结束后执行。
- `AMtxCnd_wait()` 必须在已经持有同一把 `AMtxCnd_lock()` 的前提下使用，并且要放在 `while (...)` 循环里反复检查共享谓词。
- `ASemaphore` 初始化后默认 `max == 0`；第一次 `ASemaphore_lock()` 前要先 `ASemaphore_setMax()`，而且把上限调大且当前存在空位时，会唤醒等待中的线程重新竞争名额。

## 适合什么场景

更适合：

- 纯 C 项目中的基础容器层
- 需要统一资源释放、复制和比较语义的内部组件
- 想把 STL 式“值语义 + 泛型容器”迁移到 C 的代码库

不太适合：

- 需要跨平台 GUI/事件循环/IO/Unicode 的完整基础设施
- 需要稳定 ABI、长期生态支持和广泛第三方集成的公共库
- 需要尽量少宏、尽量少 GNU 扩展的保守 C 环境

## 更多文档

- 完整模块/API 手册：`DOC.md`
- 示例代码：`sample/`
- 行为回归测试：`test/`

## 许可证

GPLv3
