# ALib

ALib 是一个面向 C11 + GNU 扩展的底层工具库，定位上对标 GLib，但设计路线并不相同：
它把“泛型容器、对象生命周期、轻量类系统、进程内信号、线程兼容层、锁封装”组合进一套偏编译期驱动的接口里，目标是在纯 C 中获得比传统 `void*` 工具库更强的类型约束和更统一的值语义体验。

当前仓库产物为静态库：Linux 下默认生成 `libatlan.a`，Windows 目标统一生成 `atlan.lib`。公共头文件位于 `inc/`，安装后按 `<alib/...>` 方式引用。

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
- `asignal.h`：进程内信号连接/派发系统
- `athrd.h`：兼容 C11 `<threads.h>` 的线程原语入口；优先复用系统实现，缺失时回退到 POSIX/Win32
- `alock.h`：基于 `athrd.h` 的互斥锁、递归锁、读写锁及自动解锁辅助对象

其中 `athrd.h` 适合直接做线程创建、条件变量等待和线程局部存储；如果只是想在 ALib 风格代码里管理临界区，通常直接使用 `alock.h` 的 `AAutoKey`/`AMtx` 更顺手。

## 构建、测试与安装

### 构建静态库

默认编译器约定：

- Linux：默认优先使用 `gcc`
- Windows：默认优先使用 `clang-cl`（MSVC 风格命令参数）
- 非 Linux/Unix/Windows 目标平台：配置阶段直接提示无法编译
- 不支持 C11 或 GNU 扩展（如 `__auto_type`、`typeof`、`cleanup`、`weakref`）的编译器：配置阶段直接提示无法编译

```bash
mkdir -p build
cd build
cmake ..
make
```

默认生成：

- Linux 静态库目标：`libatlan.a`
- Windows 静态库目标：`atlan.lib`（统一不带 `lib` 前缀）
- 中间文件目录：`build/obj`
- 临时头文件映射：`build/inc -> inc`，并额外生成 `build/alib -> inc` 以兼容 `<alib/...>` 引用
- 目标文件目录：库在 `build/lib`，示例在 `build/sample`，测试在 `build/test`

默认会同时构建库本身、`sample/` 下的示例程序和 `test/` 下的测试程序。

Windows 下按本文 `cmake ..` + `make` 流程操作时，建议先确认这些前提：

- 已安装 GNU Make，并且命令行里执行 `make --version` 能成功
- 当前 CMake 生成器确实产出 Makefile；如果 CMake 默认选成了 Visual Studio 生成器，需要改成 `Unix Makefiles`，例如 `cmake -G "Unix Makefiles" ..`
- 已进入 Visual Studio / Build Tools 提供的开发者命令行环境，使 `clang-cl`、`link.exe`、`lib.exe` 以及 Windows SDK 头文件和库都可用
- `make install` 默认会写入 `C:\Program Files (x86)\alib`，通常需要管理员权限；如果不想提权，请在配置阶段改用自定义 `CMAKE_INSTALL_PREFIX`

### toolchains 目录

仓库内当前提供这些工具链文件：

- `cmake/toolchains/linux-gcc.cmake`：Linux 本机构建，显式使用 `gcc`
- `cmake/toolchains/linux-clang.cmake`：Linux 本机构建，显式使用 `clang`
- `cmake/toolchains/windows-clang-cl.cmake`：Windows 本机构建，显式使用 `clang-cl`
- `cmake/toolchains/mingw64.cmake`：Linux 主机交叉编译 Windows，使用 `mingw-w64`

Linux 下如果要显式切换编译器，可直接这样用：

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/linux-gcc.cmake
make
```

或：

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/linux-clang.cmake
make
```

### Linux 下交叉编译 Windows

仓库内已提供现成工具链文件：`cmake/toolchains/mingw64.cmake`。

前提：

- 已安装 `mingw-w64`
- 命令行里能直接执行 `x86_64-w64-mingw32-gcc --version`

构建命令：

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/mingw64.cmake
make
```

默认产物：

- 静态库：`build/lib/atlan.lib`
- 示例程序：`build/sample/*.exe`
- 测试程序：`build/test/*.exe`

如果本机安装了 `wine`，可进一步在 Linux 下执行这些 `.exe` 做基础验证。

Windows 本机如果要显式指定 `clang-cl`，可使用：

```bash
mkdir build
cd build
cmake -G "Unix Makefiles" .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/windows-clang-cl.cmake
make
```

### 编译示例

```bash
mkdir -p build
cd build
cmake ..
make samples
```

### 编译测试

```bash
mkdir -p build
cd build
cmake ..
make tests
```

### 运行测试

```bash
cd build
ctest --output-on-failure
```

### 安装

```bash
cd build
make install
```

默认安装结果：

- Linux：
  - 头文件：`/usr/local/include/alib/*.h`
  - 静态库：`/usr/local/lib/libatlan.a`
- Windows：
  - 头文件：`C:\Program Files (x86)\alib\include\alib\*.h`
  - 静态库：`C:\Program Files (x86)\alib\lib\atlan.lib`

如需覆盖默认安装前缀：

```bash
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<your-prefix> ..
make
make install
```

Windows 下安装后建议补充环境变量：

- CMake 项目：把 `C:\Program Files (x86)\alib` 加到 `CMAKE_PREFIX_PATH`
- 直接调用 `clang-cl`：把 `C:\Program Files (x86)\alib\include` 加到 `INCLUDE`，把 `C:\Program Files (x86)\alib\lib` 加到 `LIB`

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
mkdir -p build
cd build
cmake ..
make
cd ..
gcc -std=gnu11 -Wall -Wextra -O2 -Ibuild example.c build/lib/libatlan.a -pthread -o example
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

## 需要特别注意的语义

- `AString_new()` 和 `AText_new()` 都只是“包装已有 `char*`”，不会立刻拷贝；传入栈缓冲区或临时内存时，必须在其失效前复制到一个真正拥有内存的对象。
- 顺序容器的 `at(index)` 在“容器非空但 index 越界”时，通常会截断到尾元素；只有空容器访问才会设置 `AEXC_overstep`。
- `AHash(K,V)` 和 `ATree(K,V)` 的 `ins()` 是 upsert 语义：同键再次插入会替换已有值。
- `a_signal_connection(id, addressee, call)` 对同一个 `(id, addressee)` 重复连接时，会覆盖原有回调，而不是忽略此次连接。
- `AHash(K,V)` 的遍历顺序依赖桶布局与插入路径，不应把它当成稳定顺序容器。
- 迭代器更适合只读遍历或就地修改元素值，不适合边遍历边插入/删除；只要容器结构发生变化，就应丢弃旧迭代器并重新获取。
- `APtr(T)` 不是 `unique_ptr` 的等价物：复制后新对象是弱别名，不会接管释放责任；如果需要转移所有权，请显式使用 `A_MOVE` 或避免复制。
- `AReceEnd` 可以在析构时自动断开信号连接；信号回调内部也可以断连，但实际操作会延迟到本轮派发结束后执行。

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
