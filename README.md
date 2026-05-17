# ALib

ALib 是一个面向 C11 + GNU 扩展的底层工具库，定位上对标 GLib，但设计路线并不相同：
它把“泛型容器、对象生命周期、轻量类系统、进程内信号、锁封装”组合进一套偏编译期驱动的接口里，目标是在纯 C 中获得比传统 `void*` 工具库更强的类型约束和更统一的值语义体验。

当前仓库产物为静态库 `libatlan.a`，公共头文件位于 `inc/`，安装后按 `<alib/...>` 方式引用。

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
| 字符串能力 | `AString` 是低层字节串封装，偏轻量 | GLib 提供 `GString`、UTF-8/Unicode、路径和文本工具 |
| 并发能力 | 基于 C11 `mtx_t`/`cnd_t` 的锁封装 | GLib 线程、主循环、异步设施更成熟 |
| 功能边界 | 容器、指针、类、信号、锁 | 还覆盖主循环、IO、文件路径、模块装载、字符集等 |

这意味着：

- 如果你需要 `GMainLoop`、UTF-8/Unicode 工具、文件系统抽象、成熟的对象反射体系，优先考虑 GLib。
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
- `aptr.h`：独占指针包装和共享指针包装
- `asignal.h`：进程内信号连接/派发系统
- `alock.h`：互斥锁、递归锁、读写锁及自动解锁辅助对象

## 构建、测试与安装

### 构建静态库

```bash
make
```

默认生成：

- `libatlan.a`

### 编译示例

```bash
make -C sample
```

### 编译测试

```bash
make -C test
```

仓库里的 `sample/Makefile` 和 `test/Makefile` 会自动创建 `.local/include/alib -> inc/` 的本地头文件映射，因此不需要先安装到系统目录。

### 安装

```bash
make install PREFIX=/usr/local
```

安装结果：

- 头文件：`/usr/local/include/alib/*.h`
- 静态库：`/usr/local/lib/libatlan.a`

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

安装后可直接编译：

```bash
gcc -std=c11 -Wall -Wextra -O2 example.c -latlan -o example
```

如果只是想在仓库目录里临时编译：

```bash
mkdir -p .local/include
ln -snf "$(pwd)/inc" .local/include/alib
gcc -std=c11 -Wall -Wextra -O2 -I.local/include example.c -L. -latlan -o example
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

- `AString_new()` 只是“包装已有 `char*`”，不会立刻拷贝；传入栈缓冲区或临时内存时，必须在其失效前复制到一个真正拥有内存的 `AString`。
- 顺序容器的 `at(index)` 在“容器非空但 index 越界”时，通常会截断到尾元素；只有空容器访问才会设置 `AEXC_overstep`。
- `AHash(K,V)` 和 `ATree(K,V)` 的 `ins()` 是 upsert 语义：同键再次插入会替换已有值。
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
