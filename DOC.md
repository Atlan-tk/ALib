# ALib 开发文档

本文以 `inc/` 中的公共头文件为准，系统说明 ALib 当前提供的模块、类型、宏和函数，以及它们的参数、返回值、异常语义与使用约定。

当前版本使用仓库内 GNU Makefile 构建，支持 Linux / Unix 本机构建、Linux 主机交叉编译 AArch64 / MIPS / Windows 目标。构建配置由顶层 `Makefile` 读取 `config/*.mk`。

文档范围约定：

- 以 `__` 开头的符号视为内部实现细节，不建议业务代码直接调用，本文不逐一展开。
- 其余函数、类型、函数式宏，以及容器/类生成宏都视为公共 API。
- “异常”统一指线程局部错误槽 `aErr*` 中的错误码；除特别说明外，ALib 不通过返回错误码表示失败。

## 1. 项目定位

ALib 是一个面向 C11 + GNU 扩展的底层工具库，核心目标不是复刻 GLib 全家桶，而是把下面几件事组合成一套统一接口：

- 编译期绑定元素类型的泛型容器
- 统一的对象初始化 / 拷贝 / 析构 / 比较 / 哈希协议
- 近似 RAII 的资源释放方式
- 轻量单继承类系统与虚函数表
- 进程内信号派发、线程兼容层和锁封装

一句话概括：ALib 更像“把 STL 式值语义和轻量对象模型带进 C 的实验性基础库”。

## 2. 仓库结构与构建

```text
ALib/
├── config/    # 构建配置文件，每个 <name>.mk 对应 CONFIG=<name>
├── inc/       # 公共头文件
├── src/       # 非模板实现与库目标 Makefile
├── sample/    # 示例程序与示例目标 Makefile
├── test/      # 回归测试与测试目标 Makefile
├── Makefile
├── README.md
└── DOC.md
```

常用本机构建命令：

```bash
make CONFIG=linux
```

默认约定：

- 顶层 `Makefile` 会优先使用显式传入的 `CONFIG=<name>`。
- 如果未传入 `CONFIG`，但仓库根目录存在 `.config`，会继续沿用 `.config`。
- 如果没有 `.config`，Linux 默认使用 `config/linux.mk`。
- 其他 Unix / Windows 本地构建尚未提供默认配置文件；可按需在 `config/` 下新增配置。
- 不支持 C11 或 GNU 扩展（如 `__auto_type`、`typeof`、`cleanup`、`weakref`）的编译器不属于当前支持范围。

当前内置配置：

- `linux`：Linux / Unix 本地默认构建，使用 `cc` 和 `ar`。
- `linux-clang`：Linux / Unix 本地 Clang 构建，使用 `clang` 和 `llvm-ar`。
- `linux-arm`：Linux 到 AArch64 的交叉编译，使用 `aarch64-linux-gnu-gcc`。
- `linux-mips`：Linux 到 MIPS 的交叉编译，使用 `mips-linux-uclibc-gnu-gcc`。

新增配置时，只需添加新的 `config/<name>.mk`，之后使用 `make CONFIG=<name>` 即可。切换不同配置时建议显式传入 `CONFIG`，或执行 `make disclean` 删除旧 `.config` 后重新构建。

默认产物：

- 中间文件目录：`build/`
- 编译期头文件链接：`build/.include/alib`
- 库输出目录：`build/out/`
- Linux / Unix 本地静态库：`build/out/libalib.a`
- Windows 目标静态库：`build/out/alib.lib`
- 示例程序：`build/out/sample_*.<out|exe>`
- 测试程序：`build/out/test_*.<out|exe>`

默认会同时构建库本身、`sample/` 下的示例程序和 `test/` 下的测试程序。

常用变体：

```bash
# Clang，本地 Unix / Linux
make CONFIG=linux-clang

# Linux -> AArch64
make CONFIG=linux-arm

# Linux -> MIPS
make CONFIG=linux-mips

```

交叉编译通常只能验证目标程序已经生成；如果要直接执行生成的 Windows `.exe` 测试，需要额外安装 Wine，或者复制到 Windows 环境运行。

安装规则：

- `linux` / `linux-clang`：头文件安装到 `/usr/local/include/alib/`，静态库安装到 `/usr/local/lib/`。
- `linux-arm` / `linux-mips` ：头文件安装到 `$HOME/.alib/include/alib/`，静态库安装到 `$HOME/.alib/lib/`。

安装和卸载命令：

```bash
make install
make uninstall
```

## 3. 先读这些通用约定

### 3.1 编译环境

ALib 依赖：

- C11
- GNU 扩展（`__auto_type`、`typeof`、`cleanup`、`weakref` 等）
- C11 线程 / 时间接口；优先复用系统 `<threads.h>`，缺失时由 `athrd.h` 在 POSIX / Win32 上补齐

因此推荐直接使用仓库内的 Makefile 构建，并在仓库根目录执行 `make CONFIG=<name>`。默认本地安装位置为 `/usr/local/include/alib/` 和 `/usr/local/lib/`，Linux 交叉编译默认安装到 `$HOME/.alib/include/alib/` 和 `$HOME/.alib/lib/`。如果目标平台或工具链不在内置配置中，请在 `config/` 下新增配置文件。

### 3.2 类型协议：所有容器都依赖 `A_TYPE_REGISTER`

对“普通类型”，ALib 通过下面五个名字约定类型行为：

- `A_OBJ_INIT(T)`
- `A_OBJ_DEST(T)`
- `A_OBJ_COPY(T)`
- `A_OBJ_CMPD(T)`
- `A_OBJ_HASH(T)`

再通过：

```c
A_TYPE_REGISTER(T);
```

把这些行为接入统一接口。容器、指针包装、哈希、树、RAII 宏都会复用这套协议。

如果用户没有自己实现这些函数，ALib 会回退到默认行为：

- init：清零
- dest：清零
- copy：按字节复制
- cmpd：基础类型自动比较，字符串按字符串比较，其余类型退回 `memcmp`
- hash：未定义时退回原始字节哈希

### 3.3 类协议：类类型使用 `A_CLASS_REGISTER`

对“类类型”，ALib 在普通类型协议上再加一层：

- 首字段为函数表指针 `f`
- 支持单继承
- 构造 / 拷贝 / 析构时自动处理父类链
- 多态析构通过 `f->dest` 完成

类定义最后要用：

```c
A_CLASS_REGISTER(T);
```

### 3.4 异常模型

ALib 不使用 C++ 异常，也不采用 `GError**` 风格。失败通过线程局部错误槽传递：

- `aErrClean()`：清空错误槽
- `aErrOccur()`：是否有错误
- `aErrSet(v)`：写入错误码
- `aErrGet()`：读取错误码
- `aTry(...)`：执行一段操作前先清空错误槽，操作结束后进入后续 `aHit` / `aExc` 分支
- `aHit(v)`：匹配指定错误码的捕获分支
- `aExc`：兜底捕获任意未被 `aHit` 匹配的错误

常见错误码：

- `AERR_nullptr`：空指针 / 空对象
- `AERR_overstep`：越界、取空、删除 / 取出不存在项
- `AERR_outdomain`：参数超出允许域
- `AERR_alloc_failed`：内存分配失败
- `AERR_init_failed`：初始化 / 拷贝构造失败
- `AERR_repeat_write`：重复写入或不允许的写时机
- `AERR_system_error`：线程 / 锁 / 系统调用失败
- `AERR_response_exc`：信号回调抛出了异常

`aTry` 系列宏的典型写法：

```c
aTry(
    line.f->pushBack(&line, value);
)aHit(AERR_alloc_failed){
    return 1;
}aExc{
    return 2;
}
```

`aTry` 会在执行前清空错误槽；`aHit` / `aExc` 分支执行后不会自动再次清空错误槽。需要继续执行成功路径时，下一次 `aTry` 会再次清空旧错误。

使用建议：每次关键操作后尽快检查错误槽，因为后续调用可能覆盖前一次错误。

### 3.5 所有权术语

本文统一使用这些词：

- 借用（borrowed）：只引用外部对象，不负责释放
- 拥有（owning）：对象负责释放其内部资源
- 深拷贝：复制后两份对象生命周期独立
- 浅拷贝：复制后共享底层资源或只是别名
- 取出（take/pop 到 `tar`）：把元素所有权交给调用方；调用方后续负责析构

### 3.6 迭代器失效规则

统一保守规则：

- 只要容器结构发生变化，旧迭代器就视为失效。
- 结构变化包括 `ins`、`rm`、`take`、`push*`、`pop*`、哈希 rehash、树旋转 / 节点删除等。
- 迭代器更适合“只读遍历”或“修改当前元素内容但不改容器结构”。

### 3.7 泛型容器的三步法

绝大多数容器都按下面三步生成：

```c
ALine_Define(MyType);
ALine_Generate(MyType);
A_TYPE_REGISTER(ALine(MyType));
```

映射容器则使用键值双类型：

```c
ATree_Define(KeyType, ValueType);
ATree_Generate(KeyType, ValueType);
A_TYPE_REGISTER(ATree(KeyType, ValueType));
```

### 3.8 容器方法的共同语义

序列容器和映射容器里反复出现这些规则：

- 插入型 API（`ins`、`push*`）先执行 `A_COPY`，因此容器保存的是值副本。
- `take` / `pop` 如果 `tar != nullptr`，元素会被“搬出”给调用方；如果 `tar == nullptr`，容器会直接析构该元素。
- `at` 返回的是容器内部元素地址；只在当前容器结构不变时有效。
- `getNumber` / `empty` 不设置异常。

## 4. 公共模块一览

| 头文件 | 主要内容 |
| --- | --- |
| `alib.h` | 类型协议、对象生命周期、异常槽、内存钩子、哈希辅助 |
| `aiter.h` | 统一迭代器协议 |
| `aclass.h` | 轻量类系统 |
| `aline.h` | 动态数组 |
| `alist.h` | 双向链表 |
| `adeque.h` | 分块双端队列 |
| `astack.h` | 栈 |
| `aqueue.h` | 队列 |
| `asortque.h` | 有序数组队列 |
| `atree.h` | 红黑树映射 |
| `ahash.h` | 哈希映射 |
| `astring.h` | 字符串对象、UTF-8 辅助函数与 UTF-16/UTF-32/GBK 基础转换 |
| `aptr.h` | 独占 / 共享指针包装 |
| `atimer.h` | `AClock` 时间点辅助、全局毫秒级定时任务 |
| `athrd.h` | C11 线程兼容层；线程、互斥锁、条件变量、TSS、`call_once` |
| `alock.h` | 互斥锁、递归锁、读写锁、自动解锁 token |
| `asignal.h` | 信号系统、接收者基类、异常收集器 |
| `afile.h` | POSIX 文件工具函数，以及统一的 `AFile` 文件/设备/socket 读写对象 |

## 5. 完整模块与 API 参考

### 5.1 `alib.h`

`alib.h` 是所有其他头文件的基础，定义了：

- 基础类型别名
- 异常槽
- 对象生命周期宏
- 类型 / 类注册协议
- 分配器钩子
- 哈希辅助函数

#### 5.1.1 基础类型与枚举

| API | 类别 | 说明 |
| --- | --- | --- |
| `cptr_t` | 类型别名 | `void*`；常用于“原始地址比较 / 传递” |
| `cstr_t` | 类型别名 | `char*`；默认比较为字符串比较，默认哈希为 `alib_hash_str` |
| `longlong` | 类型别名 | `long long` |
| `astr_t` | 结构体 | 字符串视图，字段为 `const char* s` 和 `uint32_t len` |
| `AERR_t` | 枚举 | 线程局部错误槽使用的错误码 |
| `Atlan` | 基类 | 所有类类型的隐式根基类，仅含 `f` 指针 |

#### 5.1.2 `astr_t` 相关 API

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `astr_new(const char* s)` | `s`：非空、以 `\0` 结尾的 C 字符串 | `astr_t` | 无 | 仅构造视图，不分配内存；`len` 不包含终止符 |

注意：`astr_new` 内部直接调用 `strlen(s)`，因此 `s == nullptr` 不属于受支持输入。

#### 5.1.3 内存与对象分配钩子

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `alib_alloc(uint32_t size)` | 分配字节数 | `void*` 或 `nullptr` | 默认不主动写异常 | 默认转到 `malloc` |
| `alib_realloc(void* p, uint32_t size)` | 原地址、目标大小 | 新地址或 `nullptr` | 默认不主动写异常 | 默认转到 `realloc` |
| `alib_free(void* p)` | 待释放地址，可为 `nullptr` | 无 | 无 | 默认转到 `free` |
| `alib_new(uint32_t size, bool(*init_func)(void*))` | 大小、初始化函数 | 已初始化对象地址或 `nullptr` | `AERR_alloc_failed`；或 `init_func` 自己设置的异常 | 先分配，再调用初始化；初始化函数返回 `false` 或设置异常时会自动释放并返回 `nullptr` |
| `alib_cpnew(uint32_t size, const void* that, bool(*copy_func)(void*, const void*))` | 大小、源对象、拷贝函数 | 新对象地址或 `nullptr` | `AERR_alloc_failed`；或 `copy_func` 自己设置的异常 | 先分配，再复制；复制函数返回 `false` 或设置异常时会自动释放 |
| `alib_delete(void* p, void(*dest_func)(void*))` | 对象地址、析构函数 | 无 | 由 `dest_func` 决定 | `p == nullptr` 时直接返回；若 `dest_func != nullptr` 先析构再释放 |

平台差异：

- 非 Windows 平台：这些符号默认实现为弱符号；如果你在自己的目标文件里提供同名实现，链接后就会覆盖默认分配器。
- Windows 平台：由于不能依赖弱符号，`alib_alloc` / `alib_realloc` / `alib_free` / `alib_new` / `alib_cpnew` / `alib_delete` 在头文件里声明为函数指针变量；默认分别指向 `malloc` / `realloc` / `free` 及其上层包装逻辑。
- Windows 上如果只想替换底层内存来源，通常只改写 `alib_alloc`、`alib_realloc`、`alib_free` 三个函数指针即可；默认的 `alib_new`、`alib_cpnew`、`alib_delete` 会继续通过这些公开钩子完成分配与释放。

典型接入方式：

```c
/* 非 Windows：直接提供同名实现覆盖默认弱符号 */
void* alib_alloc(uint32_t size) {
    return my_pool_alloc(size);
}

void alib_free(void* p) {
    my_pool_free(p);
}
```

```c
/* Windows：在程序启动阶段改写函数指针 */
static void* my_alloc(uint32_t size) { return my_pool_alloc(size); }
static void* my_realloc(void* p, uint32_t size) { return my_pool_realloc(p, size); }
static void  my_free(void* p) { my_pool_free(p); }

static void install_alib_allocator(void) {
    alib_alloc = my_alloc;
    alib_realloc = my_realloc;
    alib_free = my_free;
}
```

#### 5.1.4 哈希辅助函数

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `alib_hash(const void* k, uint32_t size_k)` | 原始字节地址与长度 | `uint32_t` 哈希值 | 无 | FNV-1a 风格哈希；`k == nullptr` 或 `size_k == 0` 时返回 `0` |
| `alib_hash_str(const char* s)` | C 字符串，可为 `nullptr` | `uint32_t` 哈希值 | 无 | `s == nullptr` 时返回 `0` |

#### 5.1.5 异常槽 API

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `aErrClean()` | 无 | 无 | 清空当前线程错误槽 |
| `aErrOccur()` | 无 | `bool` | 当前线程是否存在错误 |
| `aErrSet(AERR_t v)` | 错误码 | 无 | 覆盖写入当前线程错误槽 |
| `aErrGet()` | 无 | `int` | 读取当前线程错误槽的当前值 |
| `aTry(...)` | 一段语句 | 无 | 执行前清空当前线程错误槽，执行后若无错误则跳过后续捕获分支 |
| `aHit(AERR_t v)` | 错误码 | 无 | 接在 `aTry` 后，当前错误码等于 `v` 时进入该分支 |
| `aExc` | 无 | 无 | 接在 `aTry` / `aHit` 后，兜底处理任意剩余错误 |

#### 5.1.6 常用对象生命周期宏

| API | 参数 | 表达式结果 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `A_INIT(T)` | `T`：已注册类型 | 返回一个 `T` 值对象 | 类型自己的 init 过程可能设置异常 | 适合栈对象初始化；失败时返回“已清理过”的零值对象 |
| `A_DEST(T, obj)` | `obj`：`T` 类型的非常量左值 | 析构 `obj` 并清零 | 类型自己的 dest 过程可能设置异常 | 对类类型会自动走多态析构 |
| `A_COPY(T, obj)` | `obj`：`T` 类型对象 | 返回一个复制后的 `T` | 类型自己的 copy 过程可能设置异常 | 失败时返回“已清理过”的零值对象 |
| `A_CMPD(T, obj0, obj1)` | 两个 `T` 对象 | `int`，`0` 表示相等 | 通常无；若比较函数自己写异常则会透出 | 基础排序约定：大于返回正，小于返回负 |
| `A_MOVE(obj)` | 非常量左值 | 返回旧值，并把源对象按字节清零 | 无 | 适合显式转移所有权 |
| `A_LEFT(obj)` | 右值表达式 | 把右值转成可取地址的临时左值 | 无 | 多用于需要左值语境的宏调用 |
| `RAII(T)` | 类型名 | 不是函数调用；用于声明变量 | 无 | 依赖 GNU `cleanup`；离开作用域自动析构 |
| `A_NEW(T)` | `T`：已注册类型 | `T*` 或 `nullptr` | `AERR_alloc_failed` / 类型 init 异常 | 堆上分配并初始化 |
| `A_CPNEW(T, obj)` | `obj`：`T` 对象 | `T*` 或 `nullptr` | `AERR_alloc_failed` / 类型 copy 异常 | 堆上复制构造 |
| `A_DELETE(T, p)` | `p`：`T*` | 无 | 类型 dest 异常可能透出 | 析构并释放堆对象 |

#### 5.1.7 类型 / 类注册宏

| API | 参数 | 返回 / 效果 | 说明 |
| --- | --- | --- | --- |
| `A_TYPE_REGISTER(T)` | 普通类型名 `T` | 生成 `T` 的统一包装函数与默认回退行为 | 普通结构体、容器实例、指针包装都用它接入对象协议 |
| `A_CLASS_REGISTER(T)` | 类类型名 `T` | 生成类类型包装函数并接入父类链、多态析构 | 只有使用 `AClass_*` 定义的类才应调用 |
| `A_OBJ_INIT(T)` / `A_OBJ_DEST(T)` / `A_OBJ_COPY(T)` / `A_OBJ_CMPD(T)` / `A_OBJ_HASH(T)` | 类型名 `T` | 这些宏本身不是运行时调用，而是“用户实现类型钩子时的命名规则” | 自定义类型时按这些名字实现函数 |
| `A_SET_VTAB(T)` | 类类型名 `T` | 类 vtable 初始化钩子的命名规则 | 仅类系统使用 |
| `A_FUNC(T)` / `A_FUNC_TAB(T)` | 类型名 `T` | 函数表类型名 / 函数表变量名的命名规则 | 主要用于容器和类生成宏 |

#### 5.1.8 其他公共工具宏

| API | 参数 | 返回 / 效果 | 说明 |
| --- | --- | --- | --- |
| `container_of(ptr, type, member)` | 成员指针、宿主类型、成员名 | 宿主结构体指针 | 经典 `container_of`，适合 intrusive 结构或手写对象系统 |

### 5.2 `aiter.h`

`aiter.h` 定义统一迭代器协议。所有支持迭代的容器都会暴露首尾迭代器、前后移动函数，以及统一的 `forEach` / `forEachRev` 宏。

#### 5.2.1 类型与生成宏

| API | 参数 | 返回 / 效果 | 说明 |
| --- | --- | --- | --- |
| `AIter(CT)` | 容器类型名 `CT` | 迭代器类型名 | 例如 `AIter(ALine(int))` |
| `AIter_Define(CT)` | 容器类型名 `CT` | 为该容器生成迭代器结构定义 | 一般由容器宏内部调用，用户很少手写 |

迭代器公共字段：

- `p`：当前元素指针；为空表示无效或到尾后位置
- `con`：所属容器指针
- `i`：逻辑索引
- `r`：某些容器内部使用的辅助位置字段

#### 5.2.2 迭代辅助宏

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AItHead(con)` | 容器对象左值 | 返回头迭代器 | 无 | 展开时只求值一次 |
| `AItTail(con)` | 容器对象左值 | 返回尾迭代器 | 无 | 展开时只求值一次 |
| `AItExist(it)` | 迭代器变量 | `bool` | 无 | 依赖 `it.con->f->getNumber()`；容器为空或越界时为假 |
| `AItNext(it)` | 迭代器变量 | 前进到下一个元素 | 无 | 依赖容器自己的 `next` 实现 |
| `AItPrev(it)` | 迭代器变量 | 后退到上一个元素 | 无 | 依赖容器自己的 `prev` 实现 |
| `forEach(it, con)` | 迭代器变量名、容器对象 | 正向遍历宏 | 无 | 等价于 `for (auto it = AItHead(...); AItExist(it); AItNext(it))` |
| `forEachRev(it, con)` | 迭代器变量名、容器对象 | 反向遍历宏 | 无 | 等价于 `for (auto it = AItTail(...); AItExist(it); AItPrev(it))` |

#### 5.2.3 使用建议

- `it.p` 的静态类型是当前容器元素指针类型，不需要手动转换。
- 结构修改后旧迭代器失效，应重新获取。
- 映射容器遍历值时，键要通过 `getk(it)` 取出。

### 5.3 `aclass.h`

`aclass.h` 提供轻量单继承类系统。它不是完整反射框架，只解决：

- 单继承
- 函数表
- 父类链初始化 / 拷贝 / 析构
- 多态析构
- 虚函数覆盖

#### 5.3.1 推荐定义顺序

```c
AClass_Inherit(MyType, BaseType);
AClass_Struct(MyType,
    int value;
);
AClass_Function(MyType,
    void (*const speak)(const MyType* self);
);
AClass_Generate(MyType, mytype_speak);
A_CLASS_REGISTER(MyType);
```

如果没有显式父类，`AClass_Inherit(T)` 会默认继承 `Atlan`。

#### 5.3.2 类定义宏

| API | 参数 | 返回 / 效果 | 异常 / 约束 | 说明 |
| --- | --- | --- | --- | --- |
| `AClass_Inherit(T, ...)` | `T`：当前类名；可选第 2 个参数：父类名 | 生成前置声明、函数表声明、继承关系 | 宏变参，最多 1 个父类；父类必须已经是类类型 | 省略父类时默认继承 `Atlan` |
| `AClass_Struct(T, ...)` | 类名、成员声明列表 | 定义类结构体 | 无 | 结构体首部会自动放入 `f` 与父类基布局 |
| `AClass_Function(T, ...)` | 类名、函数表字段列表 | 定义 `A_FUNC(T)` 结构体 | 无 | 结构体首部会自动保留父类函数表布局 |
| `AClass_Generate(T, ...)` | 类名、函数表初始化项 | 生成弱符号函数表 `A_FUNC_TAB(T)` | 宏变参 | 初始化项顺序必须与 `AClass_Function` 里新增字段一致 |
| `A_CLASS_REGISTER(T)` | 类名 | 把类接入对象系统和多态析构系统 | 父类链必须完整 | 生成构造 / 拷贝 / 析构包装函数 |

#### 5.3.3 虚函数调用与覆盖

| API | 参数 | 返回 / 效果 | 异常 / 约束 | 说明 |
| --- | --- | --- | --- | --- |
| `A_CALL(obj, ...)` | 对象值 `obj`；可选 1 个静态类型参数 | 返回函数表左值 | 宏变参，最多 1 个附加参数 | 常见写法：`A_CALL(obj).foo(&obj)` 或 `A_CALL(obj, Base).foo((Base*)&obj)` |
| `A_COVER_FUNC(self, T, name, func)` | 当前对象指针、要操作的类类型、函数表字段名、新函数指针 | 修改当前类的函数表槽位 | 无 | 通常只在 `A_SET_VTAB(T)` 中使用 |
| `A_SET_VTAB(T)` | 类名 | 自定义 vtable 初始化钩子的命名规则 | 无 | 若用户实现该函数，会在对象 init/copy 时调用，用于覆盖虚函数 |

#### 5.3.4 重要语义

- ALib 只支持单继承。
- 类对象的复制会先复制父类部分，再复制子类新增字段。
- `A_DEST(T, obj)` 对类类型会自动走多态析构，而不是单纯调用静态类型的 `A_OBJ_DEST(T)`。
- 不要把 `A_FUNC_TAB(T)` 当作线程安全可变全局对象随意改写；常规做法是在 `A_SET_VTAB(T)` 里局部覆盖槽位。

### 5.4 `aline.h` — `ALine(T)`

`ALine(T)` 是连续内存动态数组，适合随机访问、顺序遍历和头尾插入。

#### 5.4.1 生成方式

```c
ALine_Define(T);
ALine_Generate(T);
A_TYPE_REGISTER(ALine(T));
```

前提：`T` 已完成 `A_TYPE_REGISTER(T)` 或 `A_CLASS_REGISTER(T)`。

#### 5.4.2 结构与访问入口

- 容器对象类型：`ALine(T)`
- 函数表类型：`A_FUNC(ALine(T))`
- 统一调用入口：`line.f->method(&line, ...)`

#### 5.4.3 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ALine(T)` | `T`：元素类型 | 生成具体容器类型名 | 无 | 类型名宏 |
| `ALine_Define(T)` | 元素类型 | 声明容器结构和函数表 | 无 | 仅生成声明 |
| `ALine_Generate(T)` | 元素类型 | 生成静态内联实现 | 无 | 需在一个可见编译单元中展开 |
| `line.f->at(&line, index)` | `index: uint32_t` | `T*` 或 `nullptr` | 无 | 空容器时返回 `nullptr`；非空越界时会截断到尾元素 |
| `line.f->rm(&line, index)` | 索引 | 删除元素 | 一般无；空容器直接返回 | 非空越界时删除尾元素；删除时会 `A_DEST(T, element)` |
| `line.f->ins(&line, index, obj)` | 索引、待插入对象 | 插入副本 | `A_COPY(T,obj)` 的异常；`AERR_alloc_failed` | `index > num` 时按 `num` 处理，相当于追加 |
| `line.f->take(&line, index, tar)` | 索引、可选输出指针 | 取出元素 | 空容器时 `AERR_overstep` | `tar != nullptr` 时把元素交给调用方；否则直接析构 |
| `line.f->pushBack(&line, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AERR_alloc_failed` | 常用追加接口 |
| `line.f->pushFront(&line, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AERR_alloc_failed` | 可能触发搬移 |
| `line.f->popBack(&line, tar)` | 可选输出指针 | 弹出尾元素 | 空容器时 `AERR_overstep` | `tar == nullptr` 时容器直接析构该元素 |
| `line.f->popFront(&line, tar)` | 可选输出指针 | 弹出首元素 | 空容器时 `AERR_overstep` | 可能触发前移 |
| `line.f->getNumber(&line)` | 无 | `uint32_t` | 无 | 当前元素个数 |
| `line.f->empty(&line)` | 无 | `bool` | 无 | 是否为空 |
| `line.f->head(&line)` / `tail(&line)` | 无 | `AIter(ALine(T))` | 无 | 首 / 尾迭代器 |
| `line.f->next(&it)` / `prev(&it)` | 迭代器指针 | 推进迭代器 | 无 | 与 `AItNext` / `AItPrev` 一致 |

#### 5.4.4 使用建议

- `at()` 返回内部地址，结构修改后立即失效；查找不到时返回 `nullptr`，不设置异常。
- `rm()` 在空容器上是静默 no-op，而 `pop*()` / `take()` 会报 `AERR_overstep`；两类 API 的边界风格不同。
- 如果你需要严格索引检查，请先比较 `index < getNumber()`。

### 5.5 `alist.h` — `AList(T)`

`AList(T)` 是双向链表，保留统一迭代器接口，并额外暴露按节点地址删除 / 取出的能力。

#### 5.5.1 生成方式

```c
AList_Define(T);
AList_Generate(T);
A_TYPE_REGISTER(AList(T));
```

#### 5.5.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AList(T)` / `AList_Define(T)` / `AList_Generate(T)` | 同 `ALine(T)` | 生成类型与实现 | 无 | 用法与 `ALine` 相同 |
| `list.f->at(&list, index)` | 索引 | `T*` 或 `nullptr` | 无 | 空容器时返回 `nullptr`；非空越界时截断到尾节点 |
| `list.f->rm(&list, index)` | 索引 | 删除节点 | 空容器时 `AERR_overstep` | 非空越界时删除尾节点 |
| `list.f->rm_p(&list, p)` | `p: T*` | 删除 `p` 所在节点 | `p == nullptr` 时 `AERR_overstep` | `p` 必须来自当前链表的有效节点；传入外部指针属于未受支持用法 |
| `list.f->ins(&list, index, obj)` | 索引、对象 | 插入副本 | `A_COPY` 异常；`AERR_alloc_failed` | `index > num` 时按 `num` 处理 |
| `list.f->take(&list, index, tar)` | 索引、可选输出 | 取出节点元素 | 空容器时 `AERR_overstep` | `tar == nullptr` 时直接析构 |
| `list.f->take_p(&list, p, tar)` | 元素地址、可选输出 | 取出指定节点元素 | `p == nullptr` 时 `AERR_overstep` | 同样要求 `p` 属于当前链表 |
| `list.f->pushBack(&list, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AERR_alloc_failed` | |
| `list.f->pushFront(&list, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AERR_alloc_failed` | |
| `list.f->popBack(&list, tar)` | 可选输出 | 弹出尾节点 | 空容器时 `AERR_overstep` | |
| `list.f->popFront(&list, tar)` | 可选输出 | 弹出头节点 | 空容器时 `AERR_overstep` | |
| `list.f->getNumber(&list)` / `empty(&list)` | 无 | 个数 / 是否为空 | 无 | |
| `head` / `tail` / `next` / `prev` | 见统一迭代器 | 迭代支持 | 无 | |

#### 5.5.3 `rm_p` / `take_p` 的安全边界

这两个 API 很方便，但必须满足：

- `p` 来自同一个 `AList(T)` 当前仍存在的节点；
- `p` 没有因为删除、容器析构或结构重建而失效；
- 删除后旧迭代器和旧地址都应立刻废弃。

最稳妥的来源是：

- `list.f->at(&list, index)`
- 当前遍历中的 `it.p`

### 5.6 `adeque.h` — `ADeque(T)`

`ADeque(T)` 是分块双端队列，兼顾头尾操作与较平滑的扩缩容成本。

#### 5.6.1 生成方式

```c
ADeque_Define(T);
ADeque_Generate(T);
A_TYPE_REGISTER(ADeque(T));
```

#### 5.6.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ADeque(T)` / `ADeque_Define(T)` / `ADeque_Generate(T)` | 元素类型 | 生成类型与实现 | 无 | |
| `deq.f->at(&deq, index)` | 索引 | `T*` 或 `nullptr` | 无 | 空容器时返回 `nullptr`；非空越界时截断到尾元素 |
| `deq.f->pushBack(&deq, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AERR_alloc_failed` | |
| `deq.f->pushFront(&deq, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AERR_alloc_failed` | |
| `deq.f->popBack(&deq, tar)` | 可选输出 | 弹出尾元素 | 空容器时 `AERR_overstep` | |
| `deq.f->popFront(&deq, tar)` | 可选输出 | 弹出首元素 | 空容器时 `AERR_overstep` | |
| `deq.f->getNumber(&deq)` / `empty(&deq)` | 无 | 个数 / 是否为空 | 无 | |
| `head` / `tail` / `next` / `prev` | 迭代器 | 迭代支持 | 无 | |

#### 5.6.3 语义补充

- `ADeque(T)` 只暴露双端接口，不提供中间插入 / 删除。
- `at(0)` 是当前队首，`at(getNumber()-1)` 是当前队尾。
- 内部是分块存储，`at()` 返回的指针同样会在结构修改后失效。

### 5.7 `astack.h` — `AStack(T)`

`AStack(T)` 建立在 `ADeque(T)` 之上，暴露栈语义。

#### 5.7.1 生成方式

```c
AStack_Define(T);
AStack_Generate(T);
A_TYPE_REGISTER(AStack(T));
```

#### 5.7.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AStack(T)` / `AStack_Define(T)` / `AStack_Generate(T)` | 元素类型 | 生成类型与实现 | 无 | |
| `stack.f->at(&stack, index)` | 索引 | `T*` 或 `nullptr` | 无 | 空栈时返回 `nullptr`；非空越界时截断到栈顶；`at(0)` 是底部，`at(last)` 是栈顶 |
| `stack.f->push(&stack, obj)` | 对象 | 压栈副本 | `A_COPY` 异常；`AERR_alloc_failed` | 对应底层 `pushBack` |
| `stack.f->pop(&stack, tar)` | 可选输出 | 弹出栈顶 | 空栈时 `AERR_overstep` | `tar == nullptr` 时直接析构弹出的元素 |
| `stack.f->getNumber(&stack)` / `empty(&stack)` | 无 | 个数 / 是否为空 | 无 | |
| `head` / `tail` / `next` / `prev` | 迭代器 | 迭代支持 | 无 | 正向遍历从底到顶 |

### 5.8 `aqueue.h` — `AQueue(T)`

`AQueue(T)` 同样建立在 `ADeque(T)` 之上，暴露 FIFO 队列语义。

#### 5.8.1 生成方式

```c
AQueue_Define(T);
AQueue_Generate(T);
A_TYPE_REGISTER(AQueue(T));
```

#### 5.8.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AQueue(T)` / `AQueue_Define(T)` / `AQueue_Generate(T)` | 元素类型 | 生成类型与实现 | 无 | |
| `queue.f->at(&queue, index)` | 索引 | `T*` 或 `nullptr` | 无 | 空队列时返回 `nullptr`；非空越界时截断到队尾；`at(0)` 是下一个将被弹出的元素 |
| `queue.f->push(&queue, obj)` | 对象 | 入队副本 | `A_COPY` 异常；`AERR_alloc_failed` | 对应底层 `pushBack` |
| `queue.f->pop(&queue, tar)` | 可选输出 | 弹出队首 | 空队列时 `AERR_overstep` | 对应底层 `popFront` |
| `queue.f->getNumber(&queue)` / `empty(&queue)` | 无 | 个数 / 是否为空 | 无 | |
| `head` / `tail` / `next` / `prev` | 迭代器 | 迭代支持 | 无 | 正向遍历顺序与出队顺序一致 |

### 5.9 `asortque.h` — `ASortque(T)`

`ASortque(T)` 是“始终保持升序”的数组容器，适合做小到中等规模、需要频繁取最小 / 最大值的有序集合。

#### 5.9.1 生成方式

```c
ASortque_Define(T);
ASortque_Generate(T);
A_TYPE_REGISTER(ASortque(T));
```

#### 5.9.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ASortque(T)` / `ASortque_Define(T)` / `ASortque_Generate(T)` | 元素类型 | 生成类型与实现 | 无 | |
| `sq.f->at(&sq, index)` | 索引 | `T*` 或 `nullptr` | 无 | 空容器时返回 `nullptr`；非空越界时截断到最大元素 |
| `sq.f->rm(&sq, index)` | 索引 | 删除一个元素 | 空容器时静默返回 | 非空越界时删除最大元素 |
| `sq.f->ins(&sq, obj)` | 对象 | 按序插入副本 | `A_COPY` 异常；`AERR_alloc_failed` | 排序依据是 `A_CMPD(T, lhs, rhs)`；允许重复值 |
| `sq.f->take(&sq, index, tar)` | 索引、可选输出 | 取出元素 | 空容器时 `AERR_overstep` | 非空越界时取最大元素 |
| `sq.f->popMax(&sq, tar)` | 可选输出 | 弹出最大元素 | 空容器时 `AERR_overstep` | 对应内部尾弹出 |
| `sq.f->popMin(&sq, tar)` | 可选输出 | 弹出最小元素 | 空容器时 `AERR_overstep` | 对应内部首弹出 |
| `sq.f->getNumber(&sq)` / `empty(&sq)` | 无 | 个数 / 是否为空 | 无 | |
| `head` / `tail` / `next` / `prev` | 迭代器 | 迭代支持 | 无 | 正向遍历顺序就是升序顺序 |

#### 5.9.3 排序语义

- 排序完全依赖 `A_CMPD(T, lhs, rhs)`。
- 相同元素允许重复插入。
- 对“相等元素”的相对顺序没有稳定性承诺，不要把它当成稳定排序容器。

### 5.10 `atree.h` — `ATree(TK, TV)`

`ATree(TK, TV)` 是红黑树有序映射。优点是键有序、遍历顺序稳定，且插入 / 删除 / 查找的渐进复杂度稳定。

#### 5.10.1 生成方式

```c
ATree_Define(TK, TV);
ATree_Generate(TK, TV);
A_TYPE_REGISTER(ATree(TK, TV));
```

前提：

- `TK` 已注册，并提供稳定的 `A_CMPD(TK, ...)`
- `TV` 已注册

#### 5.10.2 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ATree(TK,TV)` / `ATree_Define(TK,TV)` / `ATree_Generate(TK,TV)` | 键类型、值类型 | 生成类型与实现 | 无 | |
| `tree.f->at(&tree, key)` | 键值 `key` | `TV*` 或 `nullptr` | 无 | 键不存在时返回 `nullptr`；命中时返回内部值地址 |
| `tree.f->rm(&tree, key)` | 键值 | 删除键值对 | 键不存在时 `AERR_overstep` | 删除时会析构键和值 |
| `tree.f->ins(&tree, key, value)` | 键、值 | 插入 / 替换 | `A_COPY` 异常；`AERR_alloc_failed` | 同键再次插入是 upsert：旧键值对会被析构并被新副本替换 |
| `tree.f->take(&tree, key, tar)` | 键、可选输出值指针 | 删除并取出值 | 键不存在时 `AERR_overstep` | 键始终由容器内部析构；`tar != nullptr` 时值交给调用方，否则直接析构值 |
| `tree.f->getNumber(&tree)` | 无 | `uint32_t` | 无 | 当前节点数 |
| `tree.f->empty(&tree)` | 无 | `bool` | 无 | 树根是否为空 |
| `tree.f->head(&tree)` / `tail(&tree)` | 无 | 迭代器 | 无 | `head` 指向最小键，`tail` 指向最大键 |
| `tree.f->next(&it)` / `prev(&it)` | 迭代器指针 | 前进 / 后退 | 无 | 中序遍历 |
| `tree.f->getk(it)` | 迭代器值 | `TK` | `it.p == nullptr` 时 `AERR_overstep` | 按 C 的值返回语义取出当前键；不会调用 `A_COPY(TK, ...)` |

#### 5.10.3 使用建议

- 需要稳定有序遍历时，优先用 `ATree` 而不是 `AHash`。
- 如果键类型的“数学相等”与默认字节比较不一致，请显式实现 `A_OBJ_CMPD(TK)`。
- `getk(it)` 只是按 C 的值返回语义把键拿出来，不会走 `A_COPY(TK, ...)`。
- 对 POD / 标量键可以直接用；对 `AStr` 这类带资源的键，应把返回值视作临时借用别名，若要长期保存请立刻做一次显式 `A_COPY(TK, key)`。

### 5.11 `ahash.h` — `AHash(TK, TV)`

`AHash(TK, TV)` 是哈希映射，接口外形和 `ATree` 接近，但迭代顺序不稳定，更适合按键查找密集的场景。

#### 5.11.1 生成方式

```c
AHash_Define(TK, TV);
AHash_Generate(TK, TV);
A_TYPE_REGISTER(AHash(TK, TV));
```

#### 5.11.2 键类型要求

哈希映射对键类型比树更敏感。建议：

- 至少实现稳定的 `A_OBJ_CMPD(TK)`；
- 如果键不是纯 POD 或者有明确的“逻辑相等”定义，再显式实现 `A_OBJ_HASH(TK)`。

如果不实现 `A_OBJ_HASH(TK)`，ALib 会退回到 `alib_hash()` 对原始字节做哈希；这对包含 padding、指针、未规范化表示的结构体并不总是可靠。

#### 5.11.3 API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AHash(TK,TV)` / `AHash_Define(TK,TV)` / `AHash_Generate(TK,TV)` | 键类型、值类型 | 生成类型与实现 | 无 | |
| `hash.f->at(&hash, key)` | 键值 | `TV*` 或 `nullptr` | 无 | 键不存在时返回 `nullptr`；命中时返回内部值地址 |
| `hash.f->rm(&hash, key)` | 键值 | 删除键值对 | 键不存在时 `AERR_overstep` | 删除时析构键和值 |
| `hash.f->ins(&hash, key, value)` | 键、值 | 插入 / 替换 | `A_COPY` 异常；`AERR_alloc_failed` | 同键再次插入会替换旧值；内部可能触发 rehash |
| `hash.f->take(&hash, key, tar)` | 键、可选输出值指针 | 删除并取出值 | 键不存在时 `AERR_overstep` | 键总是由容器内部析构 |
| `hash.f->getNumber(&hash)` / `empty(&hash)` | 无 | 个数 / 是否为空 | 无 | |
| `hash.f->head(&hash)` / `tail(&hash)` | 无 | 迭代器 | 无 | 迭代顺序与桶布局相关，不稳定 |
| `hash.f->next(&it)` / `prev(&it)` | 迭代器指针 | 推进 / 后退 | 无 | |
| `hash.f->getk(it)` | 迭代器值 | `TK` | `it.p == nullptr` 时 `AERR_overstep` | 按 C 的值返回语义取出当前键；不会调用 `A_COPY(TK, ...)` |

#### 5.11.4 重要语义

- `AHash` 的迭代顺序不保证稳定；不要依赖“插入顺序”或“键排序顺序”。
- `getk(it)` 同样只是按 C 的值返回语义取键；对拥有型键应把它视作临时借用别名，若要长期保存请立刻显式 `A_COPY(TK, key)`。
- `A_CMPD(AHash(...), lhs, rhs)` 不依赖插入顺序；比较时会遍历 `lhs` 的键，并在 `rhs` 中按键查找对应项，再用键值对比较函数比较。
- 对“是否相等”而言，两个 `AHash` 只要键集合相同、对应值相等、元素数量相同，比较结果就是 `0`；非零结果的正负号仍可能受内部桶遍历顺序影响，不应作为稳定排序依据。
- `ins()` 是 upsert，而不是“拒绝重复键”。

### 5.12 `astring.h` — `AStr`

`AStr` 是低层字节串对象。它不做完整 Unicode 文本语义处理，重点是：

- 支持拥有 / 借用两种状态
- 在第一次写入借用字符串时自动转为可写堆内存
- 保持和 ALib 其他对象一致的复制 / 析构语义
- 提供 UTF-8 码点计数、UTF-8 字节索引，以及基于 `iconv` 的基础编码转换

`AStr` 的文本处理能力仅适合 UTF-8 字符串。对 GBK、UTF-16、UTF-32 或其他非 UTF-8 字节串，`AStr` 只承担“按字节存储 / 复制 / 析构”的角色，不提供字符级编辑、索引、比较或长度语义。

`Achar` 是 ALib 用于保存单个 UTF-8 字符字节序列的整数类型。当前定义为 `int`，低地址字节依次保存 UTF-8 字节，高位未使用字节填 `0`。例如 ASCII 字符占 1 字节，常用中文字符通常占 3 字节，最多保存 4 字节 UTF-8 序列。

#### 5.12.1 结构字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `noLiteral` | `bool` | `false` 表示当前仅借用外部字符串；`true` 表示当前字符串缓冲区由对象拥有 |
| `length` | `uint32_t` | 当前字节数，不含终止符 |
| `capacity` | `uint32_t` | 当前缓冲容量，单位为字节；借用状态下一般为 `0` |
| `s` | `char*` | 字符串缓冲区地址，可为 `nullptr` |

#### 5.12.2 创建与查询 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AStr_new(const char* s)` | `s`：可为 `nullptr` 的 C 字符串指针 | 返回 `AStr` 值对象 | 无 | 只做包装，不复制内容；`s` 若是栈缓冲区，必须在其失效前把内容复制到拥有型字符串 |
| `AStr_len(const AStr* self)` | 指针 | `uint32_t` | `AERR_nullptr` | 当前字节数 |
| `AStr_getCapacity(const AStr* self)` | 指针 | `uint32_t` | `AERR_nullptr` | 当前容量 |
| `AStr_empty(const AStr* self)` | 指针 | `bool` | `AERR_nullptr` | 是否为空 |
| `AStr_u8num(const AStr* self)` | UTF-8 字符串对象 | `uint32_t` | `AERR_nullptr`、`AERR_outdomain` | 当前 UTF-8 码点数 |

#### 5.12.3 编辑 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AStr_at(const AStr* self, uint32_t index)` | 索引 | 返回一个字节 | `AERR_nullptr` | 空串时返回 `0`；非空且 `index >= length` 时返回最后一个字节 |
| `AStr_set(AStr* self, uint32_t index, char c)` | 索引、字节 | 设置一个字节 | `AERR_nullptr`、空串时 `AERR_overstep` | `index >= length` 时写入最后一个字节 |
| `AStr_rm(AStr* self, uint32_t index)` | 索引 | 删除一个字节 | `AERR_nullptr`、分配失败时 `AERR_alloc_failed` | 空串或 `index >= length` 时静默返回；借用状态写入前会先分配可写副本 |
| `AStr_ins(AStr* self, uint32_t index, char c)` | 索引、字节 | 插入字节 | `AERR_nullptr`、`AERR_overstep`、`AERR_alloc_failed` | 支持在头部 / 中间 / 尾部插入 |
| `AStr_insStr(AStr* self, uint32_t index, const char* s)` | 索引、C 字符串 | 插入字符串 | `AERR_nullptr`、`AERR_alloc_failed` | `s` 按 `strlen` 计算长度；`index > length` 时追加到尾部 |
| `AStr_rmStr(AStr* self, uint32_t index, uint32_t n)` | 起始索引、字节数 | 删除一段字节 | `AERR_nullptr`、分配失败时 `AERR_alloc_failed` | 空串或 `index >= length` 时静默返回；超出尾部时截断到字符串末尾 |
| `AStr_pushBack(AStr* self, char c)` | 字节 | 尾部追加 | 见 `AStr_ins` | |
| `AStr_pushFront(AStr* self, char c)` | 字节 | 头部追加 | 见 `AStr_ins` | |
| `AStr_popBack(AStr* self)` | 无 | 返回尾字节，空串时返回 `'\0'` | `AERR_nullptr`、分配失败时 `AERR_alloc_failed` | 空串时不报异常 |
| `AStr_popFront(AStr* self)` | 无 | 返回首字节，空串时返回 `'\0'` | `AERR_nullptr`、分配失败时 `AERR_alloc_failed` | 空串时不报异常 |
| `AStr_addBack(AStr* self, const char* s)` | 目标串、C 字符串 | 尾部拼接 | `AERR_nullptr`、`AERR_alloc_failed` | `s` 按 `strlen` 计算长度 |
| `AStr_addFront(AStr* self, const char* s)` | 目标串、C 字符串 | 头部拼接 | `AERR_nullptr`、`AERR_alloc_failed` | `s` 按 `strlen` 计算长度 |
| `AStr_truncate(AStr* self, uint32_t index)` | 截断位置 | 仅保留前 `index` 个字节 | `AERR_nullptr`、分配失败时 `AERR_alloc_failed` | `index >= length` 时静默返回 |
| `AStr_reCap(AStr* self, uint32_t new_cap)` | 新容量 | 调整容量 | 返回 `AERR_overstep` 或分配错误码 | `new_cap` 必须能容纳当前内容和终止符 |

#### 5.12.4 复制 / 析构语义

- `A_INIT(AStr)` 创建空字符串对象；初始字段清零、无缓冲区，首次写入时再按需分配。
- `AStr_new("literal")` 创建借用型字符串包装，不立刻分配内存。
- `A_COPY(AStr, s)` 的行为：
  - 如果 `s` 是拥有型字符串，会深拷贝缓冲区；
  - 如果 `s` 是借用型字符串，会复制指针，仍保持借用型；后续首次写入时才转成拥有型。
- `A_DEST(AStr, s)` 只在 `s.noLiteral == true` 时释放缓冲区。

#### 5.12.5 UTF-8 辅助与编码转换 API

本节的 `autf8_*` 字符级辅助函数以 UTF-8 为前提。转换函数可以把 UTF-8 转成其他编码并用 `AStr` 保存结果，但这些非 UTF-8 结果在 `AStr` 中仍只是字节串；除显式转换回 UTF-8 外，不应对它们使用 UTF-8 码点统计或字符索引语义。

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `autf8_num(const char* s)` | UTF-8 C 字符串 | `uint32_t` | `AERR_nullptr`、`AERR_outdomain` | 统计 UTF-8 码点数；遇到非法起始字节时停止并返回已统计数量 |
| `autf8_index(const char* s, uint32_t index)` | UTF-8 C 字符串、码点索引 | `uint32_t` 字节偏移 | `AERR_nullptr`、`AERR_outdomain` | 返回第 `index` 个 UTF-8 码点的起始字节偏移 |
| `autf8char(const char* s)` | UTF-8 C 字符串当前位置 | `Achar` | `AERR_nullptr`、`AERR_outdomain` | 读取当前位置的一个 UTF-8 字符并打包为 `Achar` |
| `AStr_u8at(const AStr* self, uint32_t index)` | UTF-8 字符串对象、码点索引 | `Achar` | `AERR_nullptr`、非法 UTF-8 时 `AERR_outdomain` | 返回第 `index` 个 UTF-8 字符；`self == nullptr` 是异常；`index` 越界时返回 `0`，不视为错误 |
| `AStr_u8rm(AStr* self, uint32_t index)` | UTF-8 字符串对象、码点索引 | 删除一个 UTF-8 码点 | `AERR_nullptr`、`AERR_outdomain`、底层字节删除异常 | 按码点定位后删除对应字节序列 |
| `AStr_u8ins(AStr* self, uint32_t index, Achar ch)` | UTF-8 字符串对象、码点索引、UTF-8 字符 | 插入一个 UTF-8 字符，或截断 | `AERR_nullptr`、`AERR_outdomain`、底层字节插入异常 | `ch` 是由 `autf8char()` 或同布局方式构造的单个 UTF-8 字符；`ch == 0` 时从目标 UTF-8 字符位置截断 |
| `AStr_u8pushBack(AStr* self, Achar ch)` | UTF-8 字符串对象、UTF-8 字符 | 尾部追加一个 UTF-8 字符 | 见 `AStr_u8ins` | |
| `AStr_u8pushFront(AStr* self, Achar ch)` | UTF-8 字符串对象、UTF-8 字符 | 头部插入一个 UTF-8 字符 | 见 `AStr_u8ins` | |
| `AStr_u8popBack(AStr* self)` | UTF-8 字符串对象 | `Achar` | `AERR_nullptr`、`AERR_outdomain`、底层字节删除异常 | 删除并返回尾部 UTF-8 字符；空串或无可取字符时返回 `0` |
| `AStr_u8popFront(AStr* self)` | UTF-8 字符串对象 | `Achar` | `AERR_nullptr`、`AERR_outdomain`、底层字节删除异常 | 删除并返回头部 UTF-8 字符；空串或无可取字符时返回 `0` |
| `autf8_tou32(const char* s)` | UTF-8 C 字符串 | `AStr` | `AERR_nullptr`、`AERR_system_error`、`AERR_alloc_failed` | 转为 UTF-32 字节串 |
| `autf8_tou16(const char* s)` | UTF-8 C 字符串 | `AStr` | 同上 | 转为 UTF-16 字节串 |
| `autf8_togbk(const char* s)` | UTF-8 C 字符串 | `AStr` | 同上 | 转为 GBK 字节串 |
| `autf8_foru32(const char* s, uint32_t len)` | UTF-32 字节串、字节数 | `AStr` | 同上 | 按显式字节数转回 UTF-8；输入可包含内嵌 `\0` |
| `autf8_foru16(const char* s, uint32_t len)` | UTF-16 字节串、字节数 | `AStr` | 同上 | 按显式字节数转回 UTF-8；输入可包含内嵌 `\0` |
| `autf8_forgbk(const char* s, uint32_t len)` | GBK 字节串、字节数 | `AStr` | 同上 | 按显式字节数转回 UTF-8 |

注意：

- `autf8_tou16` 和 `autf8_tou32` 的返回值是二进制字节串，可能包含内嵌 `\0`，不要用 `strlen(result.s)` 判断长度，应使用 `result.length` 或 `AStr_len(&result)`。
- 当前转换使用系统 `iconv`，UTF-16/UTF-32 输出通常会包含 BOM；调用 `autf8_foru16` / `autf8_foru32` 时应把包含 BOM 的完整 `result.length` 传回去。
- `AStr_addBack` / `AStr_addFront` 仍按 `strlen` 处理输入，不适合拼接含内嵌 `\0` 的 UTF-16/UTF-32 结果；编码转换内部会按显式字节数写入。
- `Achar` 保存的是 UTF-8 原始字节，不是 Unicode 码点数值；不能直接把 `Achar` 当作 Unicode code point 做数值比较或范围判断。

#### 5.12.6 最重要的坑

`AStr_new(buf)` 不是“复制字符串”，而是“借用 `buf`”。

安全模式：

```c
char buf[32] = "hello";
RAII(AStr) owned = A_INIT(AStr);
AStr_addBack(&owned, buf);
```

这样 `owned` 会变成真正拥有内容的字符串；如果只是把 `AStr_new(buf)` 本身长时间保存，`buf` 一旦失效就会悬空。

### 5.13 `aptr.h` — `APtr(T)` 与 `AShPtr(T)`

`aptr.h` 提供两类指针包装：

- `APtr(T)`：强拥有者 + 弱别名模型
- `AShPtr(T)`：原子引用计数共享指针

#### 5.13.1 `APtr(T)`

##### 生成方式

```c
APtr_Define(T);
APtr_Generate(T);
A_TYPE_REGISTER(APtr(T));
```

##### 结构字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `p` | `T*` | 指向被管理对象 |
| `strong_flag` | `bool` | `true` 表示当前包装对象负责释放 `p`；`false` 表示只是弱别名 |

##### API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `APtr(T)` | 元素类型 | 生成类型名 | 无 | |
| `APtr_Define(T)` / `APtr_Generate(T)` | 元素类型 | 生成声明与实现 | 无 | |
| `APtrNew(T)` | 类型名 | 返回一个强拥有的 `APtr(T)` | `A_NEW(T)` 的异常 | 相当于为 `T` 执行默认构造并接管所有权 |
| `APtrCPNew(T, obj)` | `obj: T` | 返回一个强拥有的 `APtr(T)` | `A_CPNEW(T,obj)` 的异常 | 为 `obj` 的堆副本创建拥有者 |
| `A_COPY(APtr(T), ptr)` | `APtr(T)` 对象 | 返回一个新的 `APtr(T)` | 通常无 | 新对象仅保存同一个 `p`，但 `strong_flag == false` |
| `APtrCvs(T, ptr)` | `APtr(T)` 左值 | 返回接管原内容的新 `APtr(T)`，并把原对象清零 | 无 | 用于显式转移包装对象本身；不会复制或析构 `*p` |
| `A_DEST(APtr(T), ptr)` | `APtr(T)` 左值 | 可能析构并释放 `p` | 被释放对象的析构异常可能透出 | 只有 `strong_flag == true` 的那一份会释放对象 |

##### 使用警告

`APtr(T)` 不是 `unique_ptr` 的等价物。复制后：

- 原对象仍然是拥有者；
- 新对象只是观察者；
- 如果拥有者先析构，所有弱别名都会悬空。

如果要转移所有权，请使用 `APtrCvs(T, ptr)` 或 `A_MOVE`，不要直接复制。

`APtrCvs(T, ptr)` 是“转换 / 转移”（convert/transfer-style）辅助宏：它按字节取走 `ptr` 当前保存的 `p`、`dest` 和 `strong_flag`，随后把 `ptr` 清零。返回的新对象继承原对象的拥有状态；如果原对象是强拥有者，新对象负责后续释放；如果原对象是弱别名，新对象仍只是弱别名。调用后不要再通过原 `ptr` 访问旧指针。

示例：

```c
RAII(APtr(int)) owner = APtrCPNew(int, 7);
RAII(APtr(int)) moved = APtrCvs(int, owner);

/* owner 已被清零，不再拥有对象；moved 继承 strong_flag == true */
```

#### 5.13.2 `AShPtr(T)`

##### 生成方式

```c
AShPtr_Define(T);
AShPtr_Generate(T);
A_TYPE_REGISTER(AShPtr(T));
```

##### 结构字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `p` | `T*` | 指向共享对象的实际数据区 |

##### API 说明

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AShPtr(T)` | 元素类型 | 生成类型名 | 无 | |
| `AShPtr_Define(T)` / `AShPtr_Generate(T)` | 元素类型 | 生成声明与实现 | 无 | |
| `AShPtrNew(T)` | 类型名 | 返回引用计数为 1 的共享指针 | 分配 / 初始化异常 | 构造一个默认值对象 |
| `AShPtrCPNew(T, obj)` | `obj: T` | 返回引用计数为 1 的共享指针 | 分配 / 复制异常 | 构造 `obj` 的堆副本 |
| `A_COPY(AShPtr(T), sp)` | 共享指针对象 | 返回共享同一底层对象的新共享指针 | 极端情况下可能 `AERR_init_failed` | 复制时原子增加引用计数 |
| `AShPtrCvs(T, sp)` | `AShPtr(T)` 左值 | 返回接管原内容的新共享指针包装，并把原对象清零 | 无 | 不增加引用计数；用于移动包装对象本身 |
| `A_DEST(AShPtr(T), sp)` | 共享指针左值 | 递减引用计数 | 被管理对象析构异常可能透出 | 最后一个持有者析构时释放底层对象 |

##### `AShPtrCvs` 与复制的区别

`A_COPY(AShPtr(T), sp)` 会增加引用计数，得到另一个共同持有者；`AShPtrCvs(T, sp)` 不增加引用计数，而是把 `sp` 当前保存的 `p`、`dest`、`ref_count` 直接转移到新对象，并把 `sp` 清零。它适合把局部共享指针包装对象转交给另一个变量、容器元素或返回值，避免一次额外的引用计数增减。

示例：

```c
RAII(AShPtr(int)) sp = AShPtrCPNew(int, 7);
RAII(AShPtr(int)) moved = AShPtrCvs(int, sp);

/* sp 已被清零；moved 仍持有原 ref_count，不发生引用计数 +1 */
```

##### 并发注意事项

- 引用计数增加 / 减少是原子安全的；
- 被共享对象 `*p` 自身并不会自动加锁；
- 多线程同时修改 `*sp.p` 时仍需外部同步。

### 5.14 `atimer.h` — `AClock` 与毫秒级定时任务

`atimer.h` 提供两部分能力：

- `AClock`：基于 `struct timespec` 的时间点对象，支持差值、加法和单位转换。
- 全局毫秒级定时器：通过 `a_timer_addwork*` 注册一次性、有限次数或长期任务，后台线程按周期调用回调。

#### 5.14.1 `AClock`

`AClock` 是值类型，`A_INIT(AClock)` 会读取当前 UTC 时间点。常用辅助函数：

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AClock_nsDiff(a, b)` | 两个 `AClock` | 返回 `a - b` 的纳秒差 | 无 | 正值表示 `a` 晚于 `b` |
| `AClock_usDiff(a, b)` / `AClock_msDiff(a, b)` / `AClock_sDiff(a, b)` | 两个 `AClock` | 返回微秒 / 毫秒 / 秒差 | 无 | 由纳秒差截断换算 |
| `AClock_nsAdd(c, t)` | 时间点、纳秒数 | 返回加上 `t` 后的新时间点 | 无 | 不修改原对象 |
| `AClock_usAdd` / `AClock_msAdd` / `AClock_sAdd` | 时间点、对应单位数 | 返回加法后的时间点 | 无 | 逐级换算到纳秒 |
| `AClock_nsCvs(t)` / `AClock_msCvs(t)` 等 | 时长数值 | 返回对应 `timespec` 值包装 | 无 | 适合构造时长或绝对时间参数 |
| `AClock_refresh(&c)` | `AClock*` | 把对象刷新为当前时间 | `AERR_system_error` | `timespec_get` 失败时设置异常 |

当前实现没有对极大数乘法和负数归一化做完整保护，调用方应避免传入会溢出或生成非法 `timespec` 的参数。

#### 5.14.2 定时任务 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `a_timer_addwork(cycle, num, call, data)` | 周期 ms、执行次数、回调、用户数据 | 返回任务 id；失败返回 `-1` | `AERR_outdomain`、`AERR_system_error` 等 | `cycle` 和 `num` 必须非 0 |
| `a_timer_addwork_one(cycle, call, data)` | 周期 ms、回调、用户数据 | 注册一次性任务 | 同上 | 等价于 `num == 1` |
| `a_timer_addwork_long(cycle, call, data)` | 周期 ms、回调、用户数据 | 注册长期任务 | 同上 | 持续执行直到被删除或进程退出 |
| `a_timer_rmwork(id)` | 任务 id | 删除等待中或正在回调队列中的任务 | `AERR_outdomain` 等 | 删除正在执行的任务时，实际重入会被抑制 |

回调签名为：

```c
void callback(void *data);
```

定时器是进程内全局单例，通过构造函数自动启动、析构函数自动停止。用户通常只需要调用 `a_timer_addwork*` 和 `a_timer_rmwork`，不需要直接管理定时线程。

#### 5.14.3 调度语义

- 单位为毫秒，实际触发时间受系统调度、锁竞争和回调耗时影响，不适合硬实时场景。
- 重复任务采用 fixed-delay 风格：每次回调执行后重新等待 `cycle`，回调耗时不会被扣除。
- 定时器在调用用户回调时不持有内部锁，因此回调里可以继续添加或删除其他定时任务。
- `a_timer_rmwork(id)` 可以删除等待队列中的任务；如果目标任务正处于本轮回调队列，会标记为删除，避免后续重新入队。
- 回调中的异常槽会被定时器清理，不会直接传回注册线程；需要业务层自行记录错误状态。

示例：

```c
static void on_timer(void *data) {
    int *count = data;
    ++*count;
}

int count = 0;
int64_t id = a_timer_addwork(100, 3, on_timer, &count);
if (id < 0 || aErrOccur()) {
    /* 处理注册失败 */
}
```

### 5.15 `athrd.h` — C11 线程兼容层

`athrd.h` 的目标不是重新设计一套线程 API，而是给 ALib 及其使用者提供一个稳定的 C11 线程入口：

- 如果系统有 `<threads.h>`，则直接透传系统实现；
- 如果 `__STDC_NO_THREADS__` 生效，或标准库缺少 `<threads.h>`，则在 POSIX 上回退到 `pthread`，在 Windows 上回退到 Win32 线程原语；
- 上层代码始终使用 `thrd_t` / `mtx_t` / `cnd_t` / `tss_t` 这些 C11 风格名字，不需要为平台分支改业务接口。

#### 5.15.1 主要类型与常量

| API | 类别 | 说明 |
| --- | --- | --- |
| `thrd_start_t` | 函数指针类型 | `int (*)(void*)`；线程入口函数 |
| `tss_dtor_t` | 函数指针类型 | `void (*)(void*)`；线程退出时用于清理 TSS 值 |
| `thrd_t` | 线程句柄 | 线程标识 / 句柄；底层表示随平台变化 |
| `once_flag` / `ONCE_FLAG_INIT` | 一次初始化原语 | 与 `call_once` 配套，确保初始化逻辑只执行一次 |
| `mtx_t` | 互斥锁类型 | 支持普通锁、递归锁，以及定时加锁接口 |
| `cnd_t` | 条件变量类型 | 与 `mtx_t` 配套使用 |
| `tss_t` | 线程特定存储 key | 与 `tss_*` API 配套 |
| `thrd_success` / `thrd_busy` / `thrd_error` / `thrd_nomem` / `thrd_timedout` | 状态码 | 与 C11 `<threads.h>` 风格一致的返回值 |
| `mtx_plain` / `mtx_recursive` / `mtx_timed` | 锁类型标志 | 传给 `mtx_init` 的 bit flag |
| `TSS_DTOR_ITERATIONS` | 宏常量 | fallback 实现固定为 `4`；若直接走系统 `<threads.h>`，则以系统定义为准 |

#### 5.15.2 兼容策略与时间语义

- 需要线程原语时，优先包含 `<alib/athrd.h>`，而不是直接依赖系统 `<threads.h>`；这样能保持与 ALib 内部相同的兼容路径。
- `thrd_sleep` 接收的是“相对时长” `duration`。
- `mtx_timedlock` 和 `cnd_timedwait` 接收的是“绝对 UTC 时间点” `time_point`；最常见写法是先用 `timespec_get(&ts, TIME_UTC)` 取得当前时间，再手工累加超时窗口。
- 如果你准备调用定时锁接口，仍建议在 `mtx_init` 时显式带上 `mtx_timed`，这样与系统 `<threads.h>` 路径的源代码兼容性最好。
- 若目标平台既没有系统 `<threads.h>`，又不属于当前 fallback 支持的 POSIX / Win32 范围，则会在编译期直接报错。

#### 5.15.3 线程 API

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `thrd_create(thrd_t* thr, thrd_start_t func, void* arg)` | 输出线程句柄、入口函数、用户参数 | `thrd_*` 状态码 | 成功后启动新线程执行 `func(arg)` |
| `thrd_equal(thrd_t lhs, thrd_t rhs)` | 两个线程句柄 | `int` | 非零表示两者标识同一线程 |
| `thrd_current(void)` | 无 | `thrd_t` | 返回当前线程句柄 |
| `thrd_sleep(const struct timespec* duration, struct timespec* remaining)` | 相对睡眠时长、可选剩余时长输出 | `0` / `-1` / `-2` | `0` 表示睡满，`-1` 表示被中断，`-2` 表示参数非法或系统错误 |
| `thrd_exit(int res)` | 线程退出码 | 不返回 | 立刻结束当前线程；`res` 可由 `thrd_join` 取回 |
| `thrd_detach(thrd_t thr)` | 线程句柄 | `thrd_*` 状态码 | 把线程转为 detached；之后不再 `join` |
| `thrd_join(thrd_t thr, int* res)` | 线程句柄、可选结果输出 | `thrd_*` 状态码 | 阻塞到目标线程结束；若 `res != nullptr`，写回线程返回值 |
| `thrd_yield(void)` | 无 | 无 | 主动让出当前时间片 |

#### 5.15.4 互斥锁、条件变量与一次初始化

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `mtx_init(mtx_t* mutex, int type)` | 锁对象、类型标志 | `thrd_*` 状态码 | 初始化互斥锁；`type` 可组合 `mtx_plain` / `mtx_recursive` / `mtx_timed` |
| `mtx_lock(mtx_t* mutex)` | 锁对象 | `thrd_*` 状态码 | 阻塞直到获取锁 |
| `mtx_timedlock(mtx_t* mutex, const struct timespec* time_point)` | 锁对象、绝对 UTC 截止时间 | `thrd_*` 状态码 | 超时返回 `thrd_timedout` |
| `mtx_trylock(mtx_t* mutex)` | 锁对象 | `thrd_*` 状态码 | 无阻塞尝试加锁；占用中返回 `thrd_busy` |
| `mtx_unlock(mtx_t* mutex)` | 锁对象 | `thrd_*` 状态码 | 释放当前线程持有的锁 |
| `mtx_destroy(mtx_t* mutex)` | 锁对象 | 无 | 销毁已初始化锁 |
| `call_once(once_flag* flag, void (*func)(void))` | 一次初始化标志、初始化函数 | 无 | 无论多少线程并发调用，`func` 最多执行一次 |
| `cnd_init(cnd_t* cond)` | 条件变量对象 | `thrd_*` 状态码 | 初始化条件变量 |
| `cnd_signal(cnd_t* cond)` | 条件变量对象 | `thrd_*` 状态码 | 唤醒一个等待者 |
| `cnd_broadcast(cnd_t* cond)` | 条件变量对象 | `thrd_*` 状态码 | 唤醒全部等待者 |
| `cnd_wait(cnd_t* cond, mtx_t* mutex)` | 条件变量对象、已持有的锁 | `thrd_*` 状态码 | 原子地“释放锁并等待”；唤醒后重新持锁返回 |
| `cnd_timedwait(cnd_t* cond, mtx_t* mutex, const struct timespec* time_point)` | 条件变量对象、已持有的锁、绝对 UTC 截止时间 | `thrd_*` 状态码 | 超时返回 `thrd_timedout` |
| `cnd_destroy(cnd_t* cond)` | 条件变量对象 | 无 | 销毁条件变量 |

#### 5.15.5 线程特定存储（TSS）

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `tss_create(tss_t* key, tss_dtor_t destructor)` | 输出 key、可选析构函数 | `thrd_*` 状态码 | 创建一个线程局部存储槽；线程退出时会对非空值调用析构器 |
| `tss_get(tss_t key)` | key | `void*` | 读取当前线程在该 key 上保存的值 |
| `tss_set(tss_t key, void* val)` | key、待保存值 | `thrd_*` 状态码 | 为当前线程写入局部值；传 `NULL` 等价于清空当前线程槽位 |
| `tss_delete(tss_t key)` | key | 无 | 删除整个 TSS key；不同线程的关联值后续不应再访问 |

#### 5.15.6 与 `alock.h` 的关系

- `athrd.h` 提供的是“原始线程原语”：线程创建、等待、条件变量、TLS、一次初始化。
- `alock.h` 则建立在 `athrd.h` 的 `mtx_t` / `cnd_t` 之上，把常见加锁模式封装成 ALib 风格对象和 `RAII(AAutoKey)`。
- 如果你需要创建线程、等待条件或直接操作 `tss_t`，用 `athrd.h`；如果你只需要在业务代码里保护临界区，通常直接用 `AMtx` / `ARecursion` / `AMtxRW` 更省心。

### 5.16 `alock.h` — 锁与自动解锁

`alock.h` 建立在 `athrd.h` 暴露的 `mtx_t` / `cnd_t` 之上，提供：

- `AMtx`：普通互斥锁
- `ARecursion`：递归互斥锁
- `AMtxRW`：读写锁
- `AMtxCnd`：互斥锁 + 条件变量组合
- `ASemaphore`：基于 `AMtxCnd` 的计数信号量
- `AAutoKey`：自动解锁 token

#### 5.16.1 类型总览

| API | 类别 | 说明 |
| --- | --- | --- |
| `ALock` | 类基类 | 所有锁类型的共同基类，内部持有 `mtx_t` |
| `AAutoKey` | 普通类型 | RAII 解锁 token，析构时执行保存的解锁函数 |
| `AMtx` | 类 | 普通互斥锁 |
| `ARecursion` | 类 | 递归互斥锁 |
| `AMtxRW` | 类 | 基于 `mtx_t + cnd_t` 实现的读写锁，带写者优先倾向 |
| `AMtxCnd` | 类 | 在 `AMtx` 上追加一个 `cnd_t`，适合“谓词 + 等待队列”场景 |
| `ASemaphore` | 类 | 基于 `AMtxCnd` 的容量限制器；`count` 为当前占用数，`max` 为上限 |

#### 5.16.2 `AAutoKey`

`AAutoKey` 常见用法：

```c
RAII(AAutoKey) key = AMtx_lock(&lock);
if (aErrOccur()) {
    return;
}
```

语义：

- `AAutoKey` 析构时，如果内部 `lock` 和 `unlock` 都非空，就自动解锁。
- `A_COPY(AAutoKey, key)` 会得到一个“空 token”，不会复制解锁责任；因此它天然近似不可复制。

#### 5.16.3 公共函数

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ALock_unlock(ALock* self)` | 锁基类指针 | 无 | 无 | 当前实现是空内联占位函数，通常不直接调用 |
| `ALock_uplock(ALock* self)` | 锁基类指针 | 无 | 无 | 当前实现是空内联占位函数，通常不直接调用 |
| `AMtx_lock(AMtx* self)` | 普通互斥锁指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 成功时返回带解锁器的 token |
| `ARecursion_lock(ARecursion* self)` | 递归锁指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 同一线程可重入 |
| `AMtxRW_rlock(AMtxRW* self)` | 读写锁指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 获取读锁；存在等待写者时新读者也会阻塞 |
| `AMtxRW_wlock(AMtxRW* self)` | 读写锁指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 获取写锁；需要等待现有读者和写者全部离开 |
| `AMtxCnd_lock(AMtxCnd* self)` | 条件锁指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 获取与条件变量配套的互斥锁 |
| `AMtxCnd_awake(AMtxCnd* self)` | 条件锁指针 | 无 | `AERR_nullptr`、`AERR_system_error` | 唤醒一个等待在该条件变量上的线程 |
| `AMtxCnd_awake_all(AMtxCnd* self)` | 条件锁指针 | 无 | `AERR_nullptr`、`AERR_system_error` | 广播唤醒全部等待线程 |
| `AMtxCnd_wait(AMtxCnd* self)` | 条件锁指针 | 无 | `AERR_nullptr`、`AERR_system_error` | 原子地释放互斥锁并等待；返回前重新持有该锁 |
| `ASemaphore_lock(ASemaphore* self)` | 信号量指针 | `AAutoKey` | `AERR_nullptr`、`AERR_system_error` | 当 `count < max` 时占用一个名额；token 析构时归还名额 |
| `ASemaphore_setMax(ASemaphore* self, uint32_t max)` | 信号量指针、上限 | 无 | `AERR_nullptr`、`AERR_system_error` | 线程安全地更新并发上限；调大后如出现空位会唤醒等待者 |

#### 5.16.4 `AMtxCnd`

`AMtxCnd` 把一把普通互斥锁和一个条件变量打包成一个类对象，适合把“共享状态 + 谓词等待”放进统一的 ALib 生命周期里。

典型模式：

```c
typedef struct {
    AMtxCnd lock;
    bool ready;
} SharedState;

RAII(AAutoKey) key = AMtxCnd_lock(&state.lock);
if (aErrOccur()) {
    return;
}

while (!state.ready) {
    AMtxCnd_wait(&state.lock);
    if (aErrOccur()) {
        return;
    }
}
```

使用要点：

- `AMtxCnd_wait()` 的前提是调用线程已经通过 `AMtxCnd_lock()` 持有同一把锁；等待时会暂时释放这把锁，返回前再重新持有它。
- 条件变量可能出现“伪唤醒”，或者被其他线程先一步消费掉条件，因此应始终用 `while (predicate_not_ready)` 而不是 `if`。
- `AMtxCnd_awake()` / `AMtxCnd_awake_all()` 只负责唤醒等待者，不会自动修改共享谓词；通常应先在持锁状态下更新谓词，再发出唤醒。
- `AMtxCnd_awake()`、`AMtxCnd_awake_all()`、`AMtxCnd_wait()` 都对 `nullptr` 做显式保护，失败时写入 `AERR_nullptr`。

#### 5.16.5 `ASemaphore`

`ASemaphore` 建立在 `AMtxCnd` 之上，可以把它看成“用 `AAutoKey` 归还名额”的计数信号量：

- `max`：允许同时占用的最大名额数
- `count`：当前已经被占用的名额数

最小用法：

```c
RAII(ASemaphore) sem = A_INIT(ASemaphore);
if (aErrOccur()) {
    return;
}

ASemaphore_setMax(&sem, 4);
if (aErrOccur()) {
    return;
}

RAII(AAutoKey) permit = ASemaphore_lock(&sem);
if (aErrOccur()) {
    return;
}
```

语义细节：

- `ASemaphore_lock()` 会在 `count >= max` 时阻塞等待；一旦成功进入临界区，`count` 递增 1，返回的 `AAutoKey` 析构时自动递减并尝试唤醒一个等待者。
- 新建或复制得到的 `ASemaphore` 默认 `count == 0` 且 `max == 0`；也就是说，它一开始等价于“关闭状态”，第一次使用前需要显式调用 `ASemaphore_setMax()`。
- `ASemaphore_setMax()` 可以在其他线程正在持有或等待该信号量时调用；如果把上限调大，并且更新后满足 `count < max`，当前实现会广播唤醒全部等待线程，让它们重新竞争名额。
- 如果把上限调小到小于当前 `count`，已拿到名额的线程不会被强制驱逐；后续新调用 `ASemaphore_lock()` 的线程会继续等待，直到已有持有者释放到 `count < max` 为止。

#### 5.16.6 初始化 / 拷贝 / 析构语义

- `A_INIT(AMtx)` / `A_INIT(ARecursion)` / `A_INIT(AMtxRW)` / `A_INIT(AMtxCnd)` / `A_INIT(ASemaphore)` 会创建一个新的未持有锁实例。
- `A_COPY(AMtx)` / `A_COPY(ARecursion)` / `A_COPY(AMtxRW)` / `A_COPY(AMtxCnd)` **不会复制锁状态**，而是创建一把新的、未加锁的同类锁。
- `A_COPY(ASemaphore)` 同样不会复制等待队列或占用状态；新对象的 `count`、`max` 都会重置为 `0`，需要重新调用 `ASemaphore_setMax()`。
- 锁对象析构时会销毁底层系统锁 / 条件变量；`AMtxCnd` 与 `AMtxRW` 还会在内部记录条件变量是否初始化成功，以保证初始化中途失败后析构仍然安全。

#### 5.16.7 使用建议

- 优先使用 `RAII(AAutoKey)` 管理解锁，不要手工模拟“多 return 点解锁”。
- 不要复制 `AAutoKey` 并试图让多个 token 共同管理一次加锁；复制后的 token 是空对象。
- `AMtxCnd_wait()` 始终放在谓词循环里使用；仅仅“被唤醒”并不等于条件已经满足。
- `ASemaphore` 的 `max == 0` 更接近“关闭闸门”而不是“无限并发”；忘记先设上限会让后续 `ASemaphore_lock()` 一直等待。
- `AMtxRW` 对写者更友好：只要有写者等待，新读者就会阻塞，避免写者饥饿。

### 5.17 `asignal.h` — 信号系统与接收者基类

`asignal.h` 提供进程内同步派发信号系统。它支持：

- 申请新的信号 id
- 连接 / 断开回调
- 同步发送信号
- 收集回调执行时抛出的异常
- 让接收者在析构时自动断连

#### 5.17.1 主要类型

| API | 类别 | 说明 |
| --- | --- | --- |
| `Aint` | 类型别名 | `int32_t`，用于信号 id 和部分信号相关字段 |
| `ASignal` | 类基类 | 信号基类，字段有 `id`、`value`、`sender` |
| `AExcEnd` | 结构体 | 一条回调异常记录，字段为 `addressee` 和 `exc_value` |
| `AExcCollector` | 类 | 回调异常收集器，内部维护 `ALine(AExcEnd)` |
| `AReceEnd` | 类基类 | 接收者辅助基类，析构时自动断开其全部连接 |

`AExcEnd` 字段说明：

- `addressee`：发生异常的接收者地址（`const void*`）
- `exc_value`：该回调设置的异常码

#### 5.17.2 `ASignal`

`ASignal` 是所有自定义信号的基类。字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | `Aint` | 信号类型 id，通常由 `a_signal_alloc()` 申请 |
| `value` | `Aint` | 信号值，用户自定义含义 |
| `sender` | `const void*` | 发送者地址，可为任意只读指针 |

自定义信号通常写法：

```c
AClass_Inherit(MySignal, ASignal);
AClass_Struct(MySignal,
    int extra;
);
AClass_Function(MySignal);
AClass_Generate(MySignal);
A_CLASS_REGISTER(MySignal);
```

#### 5.17.3 `AExcCollector`

`AExcCollector` 用于把回调执行过程中的异常收集起来，而不是只留下最终的 `AERR_response_exc`。

公共 API：

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AExcCollector_pop(AExcCollector* collector)` | 收集器指针 | 返回一个 `AExcEnd` | 空列表时可能透出底层 `AERR_overstep` | `collector == nullptr` 时返回零值记录；非空收集器按 LIFO 顺序弹出最后一条记录 |
| `AExcCollector_empty(const AExcCollector* collector)` | 收集器指针，可为 `nullptr` | `bool` | 无 | `collector == nullptr` 时返回 `true` |
| `AExcCollector_getNumber(const AExcCollector* collector)` | 收集器指针，可为 `nullptr` | `uint32_t` | 无 | `collector == nullptr` 时返回 `0` |
| `A_CALL(collector, AExcCollector).pop(...)` 等 | 对象方法形式 | 与上面一致 | 同上 | 类方法表只是这些函数的包装 |

`AExcCollector` 还暴露两个重要字段：

- `id`：本次 `a_signal_transmit` 正在派发的信号 id
- `exc`：收集器自身的异常标志；如果收集过程中连异常列表都无法追加（例如分配失败），该标志会被置位，派发过程也会中止继续收集

#### 5.17.4 信号系统公共 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `a_signal_alloc()` | 无 | 新的正整数信号 id；失败时通常为 `-1` | `AERR_system_error` | 申请一个新的信号类型 id |
| `__a_signal_transmit(const ASignal* signal, AExcCollector* collector)` | 信号指针、可选收集器 | 发送信号 | `AERR_system_error`、`AERR_nullptr`、`AERR_outdomain`、`AERR_response_exc` | 低层实现函数；业务代码通常调用 `a_signal_transmit` 宏 |
| `a_signal_transmit(signal, ...)` | 变参宏：必传 `signal`，可选 1 个 `AExcCollector*` | 发送信号 | 同 `__a_signal_transmit` | 宏变参上限 1；会做编译期类型断言 |
| `a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*))` | 信号 id、接收者地址、回调函数 | 建立连接 | `AERR_outdomain`、底层分配异常 | 同一个 `(id, addressee)` 若已存在连接，则会覆盖原有回调 |
| `a_signal_disconnect(Aint id, const void* addressee)` | 信号 id、接收者地址 | 删除一条连接 | `AERR_outdomain` | 在回调执行期间调用会延迟到本轮派发结束后执行；“连接不存在但参数合法”通常静默返回 |
| `a_target_disconnect(const void* addressee, Aint id)` | 接收者地址、信号 id | 删除一条连接 | 同 `a_signal_disconnect` | 只是参数顺序相反的内联包装 |
| `a_signal_disconnect_all(Aint id)` | 信号 id | 删除该信号 id 的全部连接 | `AERR_outdomain` | 若该 id 当前没有连接，通常静默返回 |
| `a_target_disconnect_all(const void* addressee)` | 接收者地址 | 删除该接收者的全部连接 | `AERR_outdomain` | 在回调执行期间调用同样会延迟到本轮派发结束后执行 |

#### 5.17.5 `a_signal_transmit` 宏的变参规则

宏定义等价于：

```c
a_signal_transmit(signal);
a_signal_transmit(signal, &collector);
```

规则：

- 只允许 0 或 1 个附加参数；
- 附加参数类型必须是 `AExcCollector*`；
- `signal` 必须是指向 `ASignal` 或其子类对象的指针；
- 这些约束由宏内的静态断言和类型断言在编译期检查。

#### 5.17.6 运行时语义

- 发送信号是同步行为：所有当前连接的回调都在调用线程内执行。
- 派发开始前，系统会拷贝当前信号 id 对应的连接快照；因此“回调里新增的连接”不会参与当前这一轮派发，只会影响后续派发。
- 如果某个回调设置了异常：
  - `a_signal_transmit` 返回 `AERR_response_exc` 表示有回调抛出异常（否则返回 0）；
  - 无收集器时：返回 `AERR_response_exc`，但异常槽（`aErrGet()`）已在返回到达前被清理；
  - 有收集器时：返回 `AERR_response_exc`，同时收集器中追加一条 `AExcEnd`，异常槽同样已清理。
- 收集器的记录顺序是”回调执行顺序追加、`pop` 时逆序弹出”。
- 向一个”已分配但当前无人连接”的合法 `id` 发送信号，会报 `AERR_outdomain`。
- 向一个”从未分配过或超出当前范围”的 `id` 发送信号，同样会报 `AERR_outdomain`。

#### 5.17.7 `AReceEnd`

`AReceEnd` 是接收者辅助基类，适合“对象析构时自动断开全部连接”的场景。

公共方法：

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AReceEnd_connection(const AReceEnd* self, Aint id, void(*call)(const ASignal*, void*))` | 接收者、信号 id、回调 | 建立连接 | `AERR_nullptr`，以及 `a_signal_connection` 的异常 | 实际上就是把 `self` 当作 `addressee` |
| `AReceEnd_disconnect(const AReceEnd* self, Aint id)` | 接收者、信号 id | 删除该接收者在此 id 上的连接 | `AERR_nullptr`，以及 `a_signal_disconnect` 的异常 | |
| `A_CALL(obj, AReceEnd).connection(...)` / `disconnect(...)` | 方法表调用 | 同上 | 同上 | 面向类对象的常用调用方式 |

析构语义：

- `AReceEnd` 的析构函数会调用 `a_target_disconnect_all(self)`；
- 因此只要你的接收者对象继承 `AReceEnd`，一般不需要手工在所有正常析构路径里逐条断连。

#### 5.17.8 使用约束（非常重要）

当前实现要求：

1. 已连接的接收者对象在析构前必须断开连接，或者直接继承 `AReceEnd`。
2. 回调执行期间可以断连（`disconnect` / `disconnect_all`），但实际操作会延迟到本轮派发结束后执行。
3. 同一个 `(id, addressee)` 重复调用 `a_signal_connection` / `AReceEnd_connection` 时，会更新为新的回调函数，而不是保留旧回调。
4. 回调里不要析构任何仍处于连接表中的接收者对象，否则会制造悬空指针。
5. 回调里不要等待其他线程执行信号连接 / 断开操作，否则可能形成锁等待。

### 5.18 `afile.h` — 文件工具与 `AFile`

`afile.h` 当前提供 POSIX 文件系统工具函数，以及基于文件描述符的 `AFile` 值对象。`AFile` 析构时自动关闭实例 fd，并通过内部共享节点管理同一路径的进程内读写锁、模式互斥和 POSIX `fcntl` 文件锁。普通文件、设备和 socket 都通过同一个 `AFile` 读写接口暴露。

#### 5.18.1 文件系统工具函数

| API | 说明 |
| --- | --- |
| `af_mkdir(name)` | 递归创建目录，行为接近 `mkdir -p` |
| `af_rm(name)` / `af_rm_r(name)` | 删除普通文件 / 递归删除文件或目录 |
| `af_cp(name, target)` / `af_cp_r(name, target)` | 复制普通文件 / 递归复制目录 |
| `af_mv(name, new_name)` | 移动或重命名 |
| `af_touch(name)` | 创建空文件；已存在时保留内容 |
| `af_chmod(name, p)` / `af_chmod_r(name, p)` | 修改权限位；`p` 使用 POSIX mode 值，例如 `0644`、`0755` |
| `af_path_exist(name)` | 判断路径是否存在 |
| `af_isfile(name)` / `af_isdir(name)` / `af_isdev(name)` | 判断目标类型；`af_isdev` 包含字符设备、块设备、FIFO 和 socket |
| `af_dir_extract(name)` | 提取上层目录，返回 `AStr`；根目录返回根目录，末尾不带 `/` |
| `af_name_extract(name)` | 提取文件或目录名，返回 `AStr`；末尾不带 `/` |
| `af_path_absolute(name)` | 返回绝对路径 `AStr`；相对路径基于当前目录拼接 |
| `af_ls(dir)` | 列目录，返回 `ALine(AStr)`；结果为不含 `.` 和 `..` 的绝对路径列表 |
| `af_get_info(name)` | 返回 `AFileInfo`，包含设备、inode、权限、大小和时间信息 |

这些函数失败时通常设置 `AERR_nullptr`、`AERR_outdomain`、`AERR_repeat_write` 或 `AERR_system_error`。

#### 5.18.2 `AFile` 类型

| 类型 | 说明 |
| --- | --- |
| `AFileNode` | 内部共享节点，保存绝对路径、进程内读写锁、文件锁、模式、引用数和主 fd |
| `AFile` | 对外文件对象，保存实例 fd、路径和共享节点引用 |

`AFile` 方法：

| API | 说明 |
| --- | --- |
| `AFile_read(self, size, target)` | 从当前位置读取，返回字节数；失败返回 `0` 并设置异常 |
| `AFile_write(self, size, source)` | 从当前位置写入 |
| `AFile_read_pos(self, offset, size, target)` | 使用 `pread` 按位置读取，不改变当前偏移 |
| `AFile_write_pos(self, offset, size, source)` | 使用 `pwrite` 按位置写入 |
| `AFile_ioctl(self, cmd, buf)` | 仅 `rw` 模式允许，封装 POSIX `ioctl` |
| `AFile_close(self)` | 幂等关闭；`RAII(AFile)` 会自动调用 |
| `AFile_open(self, type, name, mod)` | 低层打开入口，在已初始化的 `AFile` 上打开文件、设备或 socket |
| `AFile_register(self, fd, type, name, mod)` | 把已有 fd 注册为 `AFile`；内部会复制/接管对应实例 fd |
| `AFile_getmod(self)` / `AFile_gettype(self)` | 返回共享节点保存的打开模式和类型，失败返回 `-1` |

#### 5.18.3 打开入口

| API | 说明 |
| --- | --- |
| `aFileInOpen(name)` | 只读打开已有文件；不存在时失败，不创建文件 |
| `aFileOutOpen(name)` | 写入打开普通文件，创建并截断；始终保持独占文件锁直到关闭 |
| `aFileEndOpen(name)` | 追加打开普通文件，创建但不截断 |
| `aDevInOpen(name, noblock)` | 只读打开设备或路径；设备可启用 `O_NONBLOCK` |
| `aDevOutOpen(name, exclusive)` | 写入打开设备或路径 |
| `aDevInOutOpen(name, noblock, exclusive)` | 读写打开设备或路径；可用于 `ioctl` 和位置读写 |
| `aSocketTcpServerOpen(name)` / `aSocketTcpClientOpen(name)` | 打开 TCP 服务端或客户端；`name` 形如 `"127.0.0.1|8290"` |
| `aSocketUdpServerOpen(name)` / `aSocketUdpClientOpen(name)` | 打开 UDP 服务端或客户端 |
| `aSocketUnixServerOpen(name)` / `aSocketUnixClientOpen(name)` | 打开 Unix domain socket；`name` 是 socket 文件路径 |
| `aSocketRawServerOpen(name)` / `aSocketRawClientOpen(name)` | 打开 raw socket；通常需要系统权限 |
| `aSocketTcpAccept(server)` | 从 TCP server `AFile` 接受一个客户端连接，返回新的 `AFile` |
| `aSocketUnixAccept(server)` | 从 Unix domain server `AFile` 接受一个客户端连接，返回新的 `AFile` |

同一路径或 socket 名称在进程内按模式互斥：已用一种模式打开后，再用不兼容模式打开会失败并设置异常。推荐把返回值写成 `RAII(AFile)`，并在使用前检查 `aErrOccur()`。

打开模式：

| 模式 | 行为 |
| --- | --- |
| `__afmod_read` | 允许读；普通文件按 `O_RDONLY` 或 `O_RDWR` 打开 |
| `__afmod_write` | 允许写；普通文件按 `O_WRONLY` 或 `O_RDWR` 打开 |
| `__afmod_creat` | 创建普通文件；仅在写模式下有效 |
| `__afmod_appent` | 追加写入，对应 `O_APPEND` |
| `__afmod_truncate` | 语义上表示截断；当前普通文件在 `__afmod_creat` 且非追加时会截断 |
| `__afmod_noblock` | 非阻塞打开，对应 `O_NONBLOCK`；仅在读相关模式下保留 |
| `__afmod_exclusive` | 独占打开；写相关模式可用，持有写锁直到关闭 |

打开类型：

| 类型 | 行为 |
| --- | --- |
| `__aftype_file` | 普通文件 |
| `__aftype_device` | 设备或特殊文件 |
| `__aftype_socket` | socket；低层名称形如 `tcp|server|127.0.0.1|8290`、`unix|client|/tmp/a.sock` |

#### 5.18.4 使用示例

```c
#include <alib/afile.h>

int main(void) {
    RAII(AFile) in = aFileInOpen("input.bin");
    if (aErrOccur()) return 1;

    RAII(AFile) out = aFileOutOpen("output.bin");
    if (aErrOccur()) return 1;

    char buf[256];
    uint32_t n = in.f->read(&in, sizeof(buf), buf);
    out.f->write(&out, n, buf);
    return aErrOccur() ? 1 : 0;
}
```

#### 5.18.5 使用约束

- 同一个 `AFile*` 不应和 `AFile_close()` 并发使用；需要并发时应复制/重新打开独立对象。
- `AFile` 当前只在 POSIX 分支定义；Windows 旧设备对象接口已不再是本节描述的 API。
- `AFile` 的 IO 线程安全/进程安全语义建立在所有参与方都使用 ALib 的 `AFile` API 打开和读写同一路径的前提上；如果有代码绕过 ALib 直接使用 POSIX fd、`FILE*` 或其他库访问同一文件，ALib 无法协调这些外部操作。
- `af_*` 文件系统工具函数只在当前进程内使用全局锁串行化调用，不保证跨进程安全；如果其他进程同时移动、删除、复制、创建或 chmod 同一路径，调用方需要自行协调。
- 独占打开会延迟关闭相关 fd，以维持 POSIX 进程级文件锁语义。跨进程文件锁使用 `F_SETLKW`，遇到其他进程持锁时可能阻塞等待；进程内同路径模式冲突会直接失败。
- `aFileOutOpen()` 会截断普通文件；追加写请使用 `aFileEndOpen()`。
- `aFileInOpen()` 不创建不存在的文件。
- 路径 key 来自 `af_path_absolute()`；已存在路径优先使用 `realpath()`，不存在路径会基于当前目录拼接。
- IP socket 地址只接受数字 IPv4/IPv6 字符串，不做域名解析；需要域名时调用方应先自行解析。
- `sample/sample_afile_socket.c` 是长期运行的 TCP 回显示例，服务端和客户端会循环收发 `hello` / `yes`，需要手动停止；它不是自动退出型测试。

## 6. 示例与测试入口

示例程序：

- `sample/sample_type.c`：自定义类型与对象生命周期
- `sample/sample_aclass.c`：类系统与虚函数覆盖
- `sample/sample_aline.c`：容器生成、遍历与值语义
- `sample/sample_alock.c`：`AAutoKey`、`AMtxCnd`、`ASemaphore` 与锁的 RAII 使用
- `sample/sample_asignal.c`：信号连接、派发与异常收集
- `sample/sample_atimer.c`：一次性任务、有限次数周期任务与 `AClock` 时间差

回归测试：

- `test/test_sequence_api.c`：序列容器共性 API
- `test/test_map_api.c`：映射容器共性 API
- `test/test_alock.c`：`AMtxCnd` 唤醒、`ASemaphore` 并发上限与动态扩容
- `test/test_athrd.c`：线程兼容层、定时锁、条件变量、TSS 与 `call_once`
- `test/test_atimer.c`：一次性任务、重复次数、中途新增任务和删除长期任务
- `test/test_asignal.c`：信号系统、重入发送、异常收集
- `test/test_aptr.c`：`APtr` / `AShPtr` 语义
- 其余 `test/test_*.c`：各模块单独行为测试

推荐最少验证命令：

```bash
make CONFIG=linux
```

如果修改了示例相关接口，再补：

```bash
make CONFIG=linux
```

## 7. 常见误区速记

- `AStr_new()` 只是借用，不是复制。
- `APtr(T)` 复制后不会转移所有权，只会得到弱别名。
- `AHash(K,V)` 的相等性比较不依赖插入顺序；但非零比较结果的正负号不适合作为稳定排序依据。
- 顺序容器的 `at(index)` 普遍是“空容器返回 `NULL` 且不报错，非空越界截断到尾元素”。
- `take` / `pop` 把元素写到 `tar` 后，后续由调用方负责析构该元素。
- 迭代时一旦改容器结构，就应重新获取迭代器。
- `a_signal_transmit(signal, ...)` 只允许 0 或 1 个附加参数。
- `AClass_Inherit(T, ...)` 和 `A_CALL(obj, ...)` 都是宏变参，但附加参数上限都只有 1 个。
