# ALib

ALib 是一个面向 C11 + GNU 扩展的底层工具库，定位上对标 GLib，但设计路线并不相同：
它把“泛型容器、对象生命周期、轻量类系统、进程内信号、线程兼容层、锁封装”组合进一套偏编译期驱动的接口里，目标是在纯 C 中获得比传统 `void*` 工具库更强的类型约束和更统一的值语义体验。

当前仓库产物为静态库：Linux/Unix 下默认生成 `libatlan.a`，MinGW-w64 交叉编译 Windows 目标时生成 `atlan.lib`。公共头文件位于 `inc/`，安装后按 `<alib/...>` 方式引用。

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
| 功能边界 | 容器、指针、类、信号、线程兼容层、锁 | 还覆盖主循环、IO、文件路径、模块装载、字符集等 |

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

其中 `athrd.h` 适合直接做线程创建、条件变量等待和线程局部存储；如果只是想在 ALib 风格代码里管理临界区、等待条件或限制并发配额，通常直接使用 `alock.h` 的 `AAutoKey`、`AMtxCnd`、`ASemaphore` 更顺手。

## 构建、测试与安装

ALib 使用 CMake 构建。推荐固定使用仓库根目录下的 `build/` 作为编译目录，配置文件放在 `cmake/`；顶层 `CMakeLists.txt` 会扫描 `cmake/*.cmake`，每个 `<name>.cmake` 都自动成为一个可用的 `CONFIG=<name>` 构建配置。

### 常用本机构建

Linux / Unix 默认配置会优先选择 `linux-gcc-make`（Linux）或 `unix-gcc-make`（其他 Unix）：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles"
cmake --build build
```

也可以显式指定配置：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" -DCONFIG=linux-gcc-make
cmake --build build
```

使用 Clang（GCC 风格命令行参数、Makefile）：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" -DCONFIG=linux-clang-make
cmake --build build
```

### 可用 CONFIG

当前仓库内置配置文件位于 `cmake/`：

- `linux-gcc-make`：Linux / Unix 本地 GCC，Makefile。
- `unix-gcc-make`：Unix 本地 GCC 别名，Makefile。
- `linux-clang-make`：Linux / Unix 本地 Clang，GCC 风格命令行参数，Makefile。
- `unix-clang-make`：Unix 本地 Clang 别名，GCC 风格命令行参数，Makefile。
- `windows-clang-cl-vs`：Windows 本地 `clang-cl`，Visual Studio 项目文件，VS 风格命令行参数；使用 `-T ClangCL`。
- `linux-windows-mingw64-make`：Linux 到 Windows 的 MinGW-w64 交叉编译，Makefile。
- `linux-windows-clang-cl-make`：Linux 到 Windows 的 `clang-cl` 交叉编译，Makefile。
- `linux-linux-gcc-cross-make`：Linux 到 Linux 的 GCC 交叉编译，Makefile；需要设置 `ALIB_LINUX_GCC_TRIPLET`。

新增编译配置时，只需在 `cmake/` 下增加新的 `<name>.cmake`，之后使用 `-DCONFIG=<name>` 即可。切换不同 generator、编译器或目标平台时建议使用 `cmake --fresh`，或清理 `build/` 后重新配置。

### 输出目录

默认会同时构建库、`sample/` 示例和 `test/` 测试。构建输出固定在：

- 中间文件：`build/obj`
- 编译期头文件：`build/inc/alib/*.h`
- 静态库：`build/lib`
- 示例程序：`build/sample`
- 测试程序：`build/test`

Linux / Unix 本地构建通常生成 `build/lib/libatlan.a`。Windows 目标构建通常生成 `build/lib/atlan.lib` 和 `.exe` 程序。

### 交叉编译

MinGW-w64 交叉编译 Windows x86_64 目标：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" \
  -DCONFIG=linux-windows-mingw64-make
cmake --build build
```

MinGW-w64 构建要求目标运行时提供 `timespec_get` / `TIME_UTC`，推荐使用 UCRT 兼容的 MinGW-w64 编译器/runtime。CMake 会在对应配置中检测该能力，并为 MinGW 目标公开定义 `_UCRT`、链接 `ucrtbase`。

Linux 到 Linux 的 GCC 交叉编译示例：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" \
  -DCONFIG=linux-linux-gcc-cross-make \
  -DALIB_LINUX_GCC_TRIPLET=aarch64-linux-gnu
cmake --build build
```

交叉编译通常只能验证目标程序已经生成；若要在 Linux / WSL2 中运行 Windows `.exe` 测试，需要额外安装 Wine，或复制到 Windows 环境运行。

### Windows 本机构建

Windows 本地配置使用 `clang-cl` 和 Visual Studio 项目文件：

```powershell
cmake --fresh -S . -B build -G "Visual Studio 17 2022" -T ClangCL -DCONFIG=windows-clang-cl-vs
cmake --build build --config Release
```

Windows 本地配置不生成安装规则。

### 编译示例和测试

```bash
cmake --build build --target samples
cmake --build build --target tests
ctest --test-dir build --output-on-failure
```

### 安装

使用 Makefile 配置时，可在 `build` 下执行 `make install`，或使用通用命令：

```bash
cmake --build build --target install
```

默认安装位置：

- Linux / Unix 本地编译：头文件安装到 `/usr/local/include/alib`，静态库安装到 `/usr/local/lib`。
- Linux 交叉编译：头文件安装到 `$HOME/.alib/inc/alib`，静态库安装到 `$HOME/.alib/lib`。
- Windows 本地编译：不安装。

如需覆盖默认安装前缀：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" \
  -DCONFIG=linux-gcc-make \
  -DCMAKE_INSTALL_PREFIX=<your-prefix>
cmake --build build
cmake --build build --target install
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
gcc -std=gnu11 -Wall -Wextra -O2 example.c -latlan -pthread -o example
```

如果只是想在仓库目录里临时编译（Linux / GCC）：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" -DCONFIG=linux-gcc-make
cmake --build build

gcc -std=gnu11 -Wall -Wextra -O2 -Ibuild/inc example.c build/lib/libatlan.a -pthread -o example
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

## 需要特别注意的语义

- `AString_new()` 和 `AText_new()` 都只是“包装已有 `char*`”，不会立刻拷贝；传入栈缓冲区或临时内存时，必须在其失效前复制到一个真正拥有内存的对象。
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
