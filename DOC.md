# ALib 开发文档

本文以 `inc/` 中的公共头文件为准，系统说明 ALib 当前提供的模块、类型、宏和函数，以及它们的参数、返回值、异常语义与使用约定。

文档范围约定：

- 以 `__` 开头的符号视为内部实现细节，不建议业务代码直接调用，本文不逐一展开。
- 其余函数、类型、函数式宏，以及容器/类生成宏都视为公共 API。
- “异常”统一指线程局部错误槽 `aExc*` 中的错误码；除特别说明外，ALib 不通过返回错误码表示失败。

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
├── inc/       # 公共头文件
├── src/       # 非模板实现
├── sample/    # 示例程序
├── test/      # 回归测试
├── Makefile
├── README.md
└── DOC.md
```

常用命令：

```bash
make
make -C sample
make -C test
make install PREFIX=/usr/local
```

默认产物：

- 静态库：`libatlan.a`
- 安装头文件目录：`PREFIX/include/alib`
- 安装库目录：`PREFIX/lib`

## 3. 先读这些通用约定

### 3.1 编译环境

ALib 依赖：

- C11
- GNU 扩展（`__auto_type`、`typeof`、`cleanup`、`weakref` 等）
- C11 线程 / 时间接口；优先复用系统 `<threads.h>`，缺失时由 `athrd.h` 在 POSIX / Win32 上补齐

因此推荐直接使用仓库内的 `Makefile`，或者在外部工程中保持等价编译条件。

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

- `aExcClean()`：清空错误槽
- `aExcOccur()`：是否有错误
- `aExcSet(v)`：写入错误码
- `aExcGet()`：读取错误码

常见错误码：

- `AEXC_nullptr`：空指针 / 空对象
- `AEXC_overstep`：越界、取空、查无此项
- `AEXC_outdomain`：参数超出允许域
- `AEXC_alloc_failed`：内存分配失败
- `AEXC_init_failed`：初始化 / 拷贝构造失败
- `AEXC_repeat_write`：重复写入或不允许的写时机
- `AEXC_system_error`：线程 / 锁 / 系统调用失败
- `AEXC_response_exc`：信号回调抛出了异常

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
| `astring.h` | 字符串对象 |
| `aptr.h` | 独占 / 共享指针包装 |
| `athrd.h` | C11 线程兼容层；线程、互斥锁、条件变量、TSS、`call_once` |
| `alock.h` | 互斥锁、递归锁、读写锁、自动解锁 token |
| `asignal.h` | 信号系统、接收者基类、异常收集器 |

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
| `AEXC_t` | 枚举 | 线程局部错误槽使用的错误码 |
| `Atlan` | 基类 | 所有类类型的隐式根基类，仅含 `f` 指针 |

#### 5.1.2 `astr_t` 相关 API

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `astr_new(const char* s)` | `s`：非空、以 `\0` 结尾的 C 字符串 | `astr_t` | 无 | 仅构造视图，不分配内存；`len` 不包含终止符 |

注意：`astr_new` 内部直接调用 `strlen(s)`，因此 `s == nullptr` 不属于受支持输入。

#### 5.1.3 内存与对象分配钩子

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `alib_alloc(uint32_t size)` | 分配字节数 | `void*` 或 `nullptr` | 默认不主动写异常 | 默认弱符号，内部转到 `malloc` |
| `alib_realloc(void* p, uint32_t size)` | 原地址、目标大小 | 新地址或 `nullptr` | 默认不主动写异常 | 默认弱符号，内部转到 `realloc` |
| `alib_free(void* p)` | 待释放地址，可为 `nullptr` | 无 | 无 | 默认弱符号，内部转到 `free` |
| `alib_new(uint32_t size, void(*init_func)(void*))` | 大小、初始化函数 | 已初始化对象地址或 `nullptr` | `AEXC_alloc_failed`；或 `init_func` 自己设置的异常 | 先分配，再调用初始化；初始化失败会自动释放并返回 `nullptr` |
| `alib_cpnew(uint32_t size, const void* that, void(*copy_func)(void*, const void*))` | 大小、源对象、拷贝函数 | 新对象地址或 `nullptr` | `AEXC_alloc_failed`；或 `copy_func` 自己设置的异常 | 先分配，再复制；复制失败会自动释放 |
| `alib_delete(void* p, void(*dest_func)(void*))` | 对象地址、析构函数 | 无 | 由 `dest_func` 决定 | `p == nullptr` 时直接返回；若 `dest_func != nullptr` 先析构再释放 |

如果你要把 ALib 接到自定义分配器、对象池或调试分配层，通常就是重写这些弱符号。

#### 5.1.4 哈希辅助函数

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `alib_hash(const void* k, uint32_t size_k)` | 原始字节地址与长度 | `uint32_t` 哈希值 | 无 | FNV-1a 风格哈希；`k == nullptr` 或 `size_k == 0` 时返回 `0` |
| `alib_hash_str(const char* s)` | C 字符串，可为 `nullptr` | `uint32_t` 哈希值 | 无 | `s == nullptr` 时返回 `0` |

#### 5.1.5 异常槽 API

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `aExcClean()` | 无 | 无 | 清空当前线程错误槽 |
| `aExcOccur()` | 无 | `bool` | 当前线程是否存在错误 |
| `aExcSet(AEXC_t v)` | 错误码 | 无 | 覆盖写入当前线程错误槽 |
| `aExcGet()` | 无 | `int` | 读取当前线程错误槽的当前值 |

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
| `A_NEW(T)` | `T`：已注册类型 | `T*` 或 `nullptr` | `AEXC_alloc_failed` / 类型 init 异常 | 堆上分配并初始化 |
| `A_CPNEW(T, obj)` | `obj`：`T` 对象 | `T*` 或 `nullptr` | `AEXC_alloc_failed` / 类型 copy 异常 | 堆上复制构造 |
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
| `line.f->at(&line, index)` | `index: uint32_t` | `T*` 或 `nullptr` | 空容器时 `AEXC_overstep` | 非空越界时会截断到尾元素 |
| `line.f->rm(&line, index)` | 索引 | 删除元素 | 一般无；空容器直接返回 | 非空越界时删除尾元素；删除时会 `A_DEST(T, element)` |
| `line.f->ins(&line, index, obj)` | 索引、待插入对象 | 插入副本 | `A_COPY(T,obj)` 的异常；`AEXC_alloc_failed` | `index > num` 时按 `num` 处理，相当于追加 |
| `line.f->take(&line, index, tar)` | 索引、可选输出指针 | 取出元素 | 空容器时 `AEXC_overstep` | `tar != nullptr` 时把元素交给调用方；否则直接析构 |
| `line.f->pushBack(&line, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | 常用追加接口 |
| `line.f->pushFront(&line, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | 可能触发搬移 |
| `line.f->popBack(&line, tar)` | 可选输出指针 | 弹出尾元素 | 空容器时 `AEXC_overstep` | `tar == nullptr` 时容器直接析构该元素 |
| `line.f->popFront(&line, tar)` | 可选输出指针 | 弹出首元素 | 空容器时 `AEXC_overstep` | 可能触发前移 |
| `line.f->getNumber(&line)` | 无 | `uint32_t` | 无 | 当前元素个数 |
| `line.f->empty(&line)` | 无 | `bool` | 无 | 是否为空 |
| `line.f->head(&line)` / `tail(&line)` | 无 | `AIter(ALine(T))` | 无 | 首 / 尾迭代器 |
| `line.f->next(&it)` / `prev(&it)` | 迭代器指针 | 推进迭代器 | 无 | 与 `AItNext` / `AItPrev` 一致 |

#### 5.4.4 使用建议

- `at()` 返回内部地址，结构修改后立即失效。
- `rm()` 在空容器上是静默 no-op，而 `pop*()` / `take()` 会报 `AEXC_overstep`；两类 API 的边界风格不同。
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
| `list.f->at(&list, index)` | 索引 | `T*` 或 `nullptr` | 空容器时 `AEXC_overstep` | 非空越界时截断到尾节点 |
| `list.f->rm(&list, index)` | 索引 | 删除节点 | 空容器时 `AEXC_overstep` | 非空越界时删除尾节点 |
| `list.f->rm_p(&list, p)` | `p: T*` | 删除 `p` 所在节点 | `p == nullptr` 时 `AEXC_overstep` | `p` 必须来自当前链表的有效节点；传入外部指针属于未受支持用法 |
| `list.f->ins(&list, index, obj)` | 索引、对象 | 插入副本 | `A_COPY` 异常；`AEXC_alloc_failed` | `index > num` 时按 `num` 处理 |
| `list.f->take(&list, index, tar)` | 索引、可选输出 | 取出节点元素 | 空容器时 `AEXC_overstep` | `tar == nullptr` 时直接析构 |
| `list.f->take_p(&list, p, tar)` | 元素地址、可选输出 | 取出指定节点元素 | `p == nullptr` 时 `AEXC_overstep` | 同样要求 `p` 属于当前链表 |
| `list.f->pushBack(&list, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | |
| `list.f->pushFront(&list, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | |
| `list.f->popBack(&list, tar)` | 可选输出 | 弹出尾节点 | 空容器时 `AEXC_overstep` | |
| `list.f->popFront(&list, tar)` | 可选输出 | 弹出头节点 | 空容器时 `AEXC_overstep` | |
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
| `deq.f->at(&deq, index)` | 索引 | `T*` 或 `nullptr` | 空容器时 `AEXC_overstep` | 非空越界时截断到尾元素 |
| `deq.f->pushBack(&deq, obj)` | 对象 | 尾插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | |
| `deq.f->pushFront(&deq, obj)` | 对象 | 头插副本 | `A_COPY` 异常；`AEXC_alloc_failed` | |
| `deq.f->popBack(&deq, tar)` | 可选输出 | 弹出尾元素 | 空容器时 `AEXC_overstep` | |
| `deq.f->popFront(&deq, tar)` | 可选输出 | 弹出首元素 | 空容器时 `AEXC_overstep` | |
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
| `stack.f->at(&stack, index)` | 索引 | `T*` 或 `nullptr` | 空栈时 `AEXC_overstep` | 非空越界时截断到栈顶；`at(0)` 是底部，`at(last)` 是栈顶 |
| `stack.f->push(&stack, obj)` | 对象 | 压栈副本 | `A_COPY` 异常；`AEXC_alloc_failed` | 对应底层 `pushBack` |
| `stack.f->pop(&stack, tar)` | 可选输出 | 弹出栈顶 | 空栈时 `AEXC_overstep` | `tar == nullptr` 时直接析构弹出的元素 |
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
| `queue.f->at(&queue, index)` | 索引 | `T*` 或 `nullptr` | 空队列时 `AEXC_overstep` | 非空越界时截断到队尾；`at(0)` 是下一个将被弹出的元素 |
| `queue.f->push(&queue, obj)` | 对象 | 入队副本 | `A_COPY` 异常；`AEXC_alloc_failed` | 对应底层 `pushBack` |
| `queue.f->pop(&queue, tar)` | 可选输出 | 弹出队首 | 空队列时 `AEXC_overstep` | 对应底层 `popFront` |
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
| `sq.f->at(&sq, index)` | 索引 | `T*` 或 `nullptr` | 空容器时 `AEXC_overstep` | 非空越界时截断到最大元素 |
| `sq.f->rm(&sq, index)` | 索引 | 删除一个元素 | 空容器时静默返回 | 非空越界时删除最大元素 |
| `sq.f->ins(&sq, obj)` | 对象 | 按序插入副本 | `A_COPY` 异常；`AEXC_alloc_failed` | 排序依据是 `A_CMPD(T, lhs, rhs)`；允许重复值 |
| `sq.f->take(&sq, index, tar)` | 索引、可选输出 | 取出元素 | 空容器时 `AEXC_overstep` | 非空越界时取最大元素 |
| `sq.f->popMax(&sq, tar)` | 可选输出 | 弹出最大元素 | 空容器时 `AEXC_overstep` | 对应内部尾弹出 |
| `sq.f->popMin(&sq, tar)` | 可选输出 | 弹出最小元素 | 空容器时 `AEXC_overstep` | 对应内部首弹出 |
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
| `tree.f->at(&tree, key)` | 键值 `key` | `TV*` 或 `nullptr` | 键不存在时 `AEXC_overstep` | 返回内部值地址 |
| `tree.f->rm(&tree, key)` | 键值 | 删除键值对 | 键不存在时 `AEXC_overstep` | 删除时会析构键和值 |
| `tree.f->ins(&tree, key, value)` | 键、值 | 插入 / 替换 | `A_COPY` 异常；`AEXC_alloc_failed` | 同键再次插入是 upsert：旧键值对会被析构并被新副本替换 |
| `tree.f->take(&tree, key, tar)` | 键、可选输出值指针 | 删除并取出值 | 键不存在时 `AEXC_overstep` | 键始终由容器内部析构；`tar != nullptr` 时值交给调用方，否则直接析构值 |
| `tree.f->getNumber(&tree)` | 无 | `uint32_t` | 无 | 当前节点数 |
| `tree.f->empty(&tree)` | 无 | `bool` | 无 | 树根是否为空 |
| `tree.f->head(&tree)` / `tail(&tree)` | 无 | 迭代器 | 无 | `head` 指向最小键，`tail` 指向最大键 |
| `tree.f->next(&it)` / `prev(&it)` | 迭代器指针 | 前进 / 后退 | 无 | 中序遍历 |
| `tree.f->getk(it)` | 迭代器值 | `TK` | `it.p == nullptr` 时 `AEXC_overstep` | 按 C 的值返回语义取出当前键；不会调用 `A_COPY(TK, ...)` |

#### 5.10.3 使用建议

- 需要稳定有序遍历时，优先用 `ATree` 而不是 `AHash`。
- 如果键类型的“数学相等”与默认字节比较不一致，请显式实现 `A_OBJ_CMPD(TK)`。
- `getk(it)` 只是按 C 的值返回语义把键拿出来，不会走 `A_COPY(TK, ...)`。
- 对 POD / 标量键可以直接用；对 `AString` 这类带资源的键，应把返回值视作临时借用别名，若要长期保存请立刻做一次显式 `A_COPY(TK, key)`。

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
| `hash.f->at(&hash, key)` | 键值 | `TV*` 或 `nullptr` | 键不存在时 `AEXC_overstep` | 返回内部值地址 |
| `hash.f->rm(&hash, key)` | 键值 | 删除键值对 | 键不存在时 `AEXC_overstep` | 删除时析构键和值 |
| `hash.f->ins(&hash, key, value)` | 键、值 | 插入 / 替换 | `A_COPY` 异常；`AEXC_alloc_failed` | 同键再次插入会替换旧值；内部可能触发 rehash |
| `hash.f->take(&hash, key, tar)` | 键、可选输出值指针 | 删除并取出值 | 键不存在时 `AEXC_overstep` | 键总是由容器内部析构 |
| `hash.f->getNumber(&hash)` / `empty(&hash)` | 无 | 个数 / 是否为空 | 无 | |
| `hash.f->head(&hash)` / `tail(&hash)` | 无 | 迭代器 | 无 | 迭代顺序与桶布局相关，不稳定 |
| `hash.f->next(&it)` / `prev(&it)` | 迭代器指针 | 推进 / 后退 | 无 | |
| `hash.f->getk(it)` | 迭代器值 | `TK` | `it.p == nullptr` 时 `AEXC_overstep` | 按 C 的值返回语义取出当前键；不会调用 `A_COPY(TK, ...)` |

#### 5.11.4 重要语义

- `AHash` 的迭代顺序不保证稳定；不要依赖“插入顺序”或“键排序顺序”。
- `getk(it)` 同样只是按 C 的值返回语义取键；对拥有型键应把它视作临时借用别名，若要长期保存请立刻显式 `A_COPY(TK, key)`。
- `A_CMPD(AHash(...), lhs, rhs)` 比较的是当前迭代顺序下的键值序列，不等价于“集合意义上的相等”。
- `ins()` 是 upsert，而不是“拒绝重复键”。

### 5.12 `astring.h` — `AString`

`AString` 是低层字节串对象。它不做 Unicode 语义处理，重点是：

- 支持拥有 / 借用两种状态
- 在第一次写入借用字符串时自动转为可写堆内存
- 保持和 ALib 其他对象一致的复制 / 析构语义

#### 5.12.1 结构字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `f` | `const A_FUNC(AString)*` | 函数表 |
| `noLiteral` | `bool` | `false` 表示当前仅借用外部字符串；`true` 表示当前字符串缓冲区由对象拥有 |
| `number` | `uint32_t` | 当前字符数，不含终止符 |
| `capacity` | `uint32_t` | 当前缓冲容量，单位为字节；借用状态下一般为 `0` |
| `s` | `char*` | 字符串缓冲区地址，可为 `nullptr` |

#### 5.12.2 创建与查询 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AString_new(char* s)` | `s`：可为 `nullptr` 的 C 字符串指针 | 返回 `AString` 值对象 | 无 | 只做包装，不复制内容；`s` 若是栈缓冲区，必须在其失效前把内容复制到拥有型字符串 |
| `AString_getNumber(const AString* self)` | 指针 | `uint32_t` | 无 | 当前长度 |
| `AString_getCapacity(const AString* self)` | 指针 | `uint32_t` | 无 | 当前容量 |
| `AString_empty(const AString* self)` | 指针 | `bool` | 无 | 是否为空 |

#### 5.12.3 编辑 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AString_rm(AString* self, uint32_t index)` | 索引 | 删除一个字符 | 分配失败时 `AEXC_alloc_failed` | `index >= number` 时静默返回；若当前是借用字符串，写入前会先分配可写副本 |
| `AString_ins(AString* self, uint32_t index, char c)` | 索引、字符 | 插入字符 | `index > number` 时 `AEXC_overstep`；分配失败时 `AEXC_alloc_failed` | 支持在头部 / 中间 / 尾部插入 |
| `AString_pushBack(AString* self, char c)` | 字符 | 追加字符 | 见 `AString_ins` | 等价于在尾部插入 |
| `AString_pushFront(AString* self, char c)` | 字符 | 头插字符 | 见 `AString_ins` | |
| `AString_popBack(AString* self)` | 无 | 返回被弹出的字符，空串时返回 `'\0'` | 分配失败时 `AEXC_alloc_failed` | 空串时不报异常 |
| `AString_popFront(AString* self)` | 无 | 返回被弹出的字符，空串时返回 `'\0'` | 分配失败时 `AEXC_alloc_failed` | 空串时不报异常 |
| `AString_addBack(AString* self, AString that)` | 目标串、源串（按值传入） | 尾部拼接 | 分配失败时 `AEXC_alloc_failed` | 支持自拼接；若 `self` 与 `that` 指向同一缓冲，会先复制临时副本 |
| `AString_addFront(AString* self, AString that)` | 同上 | 头部拼接 | 分配失败时 `AEXC_alloc_failed` | 同样支持自拼接 |
| `AString_truncate(AString* self, uint32_t index)` | 截断位置 | 仅保留前 `index` 个字符 | 分配失败时 `AEXC_alloc_failed` | `index >= number` 时静默返回 |

#### 5.12.4 复制 / 析构语义

- `A_INIT(AString)` 创建空字符串对象；初始字段清零、无缓冲区，首次写入时再按需分配。
- `AString_new("literal")` 创建借用型字符串包装，不立刻分配内存。
- `A_COPY(AString, s)` 的行为：
  - 如果 `s` 是拥有型字符串，会深拷贝缓冲区；
  - 如果 `s` 是借用型字符串，会复制指针，仍保持借用型；后续首次写入时才转成拥有型。
- `A_DEST(AString, s)` 只在 `s.noLiteral == true` 时释放缓冲区。

#### 5.12.5 最重要的坑

`AString_new(buf)` 不是“复制字符串”，而是“借用 `buf`”。

安全模式：

```c
char buf[32] = "hello";
RAII(AString) owned = A_INIT(AString);
owned.f->addBack(&owned, AString_new(buf));
```

这样 `owned` 会变成真正拥有内容的字符串；如果只是把 `AString_new(buf)` 本身长时间保存，`buf` 一旦失效就会悬空。

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
| `A_DEST(APtr(T), ptr)` | `APtr(T)` 左值 | 可能析构并释放 `p` | 被释放对象的析构异常可能透出 | 只有 `strong_flag == true` 的那一份会释放对象 |

##### 使用警告

`APtr(T)` 不是 `unique_ptr` 的等价物。复制后：

- 原对象仍然是拥有者；
- 新对象只是观察者；
- 如果拥有者先析构，所有弱别名都会悬空。

如果要转移所有权，请用 `A_MOVE` 或手工重置原对象，不要直接复制。

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
| `A_COPY(AShPtr(T), sp)` | 共享指针对象 | 返回共享同一底层对象的新共享指针 | 极端情况下可能 `AEXC_init_failed` | 复制时原子增加引用计数 |
| `A_DEST(AShPtr(T), sp)` | 共享指针左值 | 递减引用计数 | 被管理对象析构异常可能透出 | 最后一个持有者析构时释放底层对象 |

##### 并发注意事项

- 引用计数增加 / 减少是原子安全的；
- 被共享对象 `*p` 自身并不会自动加锁；
- 多线程同时修改 `*sp.p` 时仍需外部同步。

### 5.14 `athrd.h` — C11 线程兼容层

`athrd.h` 的目标不是重新设计一套线程 API，而是给 ALib 及其使用者提供一个稳定的 C11 线程入口：

- 如果系统有 `<threads.h>`，则直接透传系统实现；
- 如果 `__STDC_NO_THREADS__` 生效，或标准库缺少 `<threads.h>`，则在 POSIX 上回退到 `pthread`，在 Windows 上回退到 Win32 线程原语；
- 上层代码始终使用 `thrd_t` / `mtx_t` / `cnd_t` / `tss_t` 这些 C11 风格名字，不需要为平台分支改业务接口。

#### 5.14.1 主要类型与常量

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

#### 5.14.2 兼容策略与时间语义

- 需要线程原语时，优先包含 `<alib/athrd.h>`，而不是直接依赖系统 `<threads.h>`；这样能保持与 ALib 内部相同的兼容路径。
- `thrd_sleep` 接收的是“相对时长” `duration`。
- `mtx_timedlock` 和 `cnd_timedwait` 接收的是“绝对 UTC 时间点” `time_point`；最常见写法是先用 `timespec_get(&ts, TIME_UTC)` 取得当前时间，再手工累加超时窗口。
- 如果你准备调用定时锁接口，仍建议在 `mtx_init` 时显式带上 `mtx_timed`，这样与系统 `<threads.h>` 路径的源代码兼容性最好。
- 若目标平台既没有系统 `<threads.h>`，又不属于当前 fallback 支持的 POSIX / Win32 范围，则会在编译期直接报错。

#### 5.14.3 线程 API

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

#### 5.14.4 互斥锁、条件变量与一次初始化

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

#### 5.14.5 线程特定存储（TSS）

| API | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `tss_create(tss_t* key, tss_dtor_t destructor)` | 输出 key、可选析构函数 | `thrd_*` 状态码 | 创建一个线程局部存储槽；线程退出时会对非空值调用析构器 |
| `tss_get(tss_t key)` | key | `void*` | 读取当前线程在该 key 上保存的值 |
| `tss_set(tss_t key, void* val)` | key、待保存值 | `thrd_*` 状态码 | 为当前线程写入局部值；传 `NULL` 等价于清空当前线程槽位 |
| `tss_delete(tss_t key)` | key | 无 | 删除整个 TSS key；不同线程的关联值后续不应再访问 |

#### 5.14.6 与 `alock.h` 的关系

- `athrd.h` 提供的是“原始线程原语”：线程创建、等待、条件变量、TLS、一次初始化。
- `alock.h` 则建立在 `athrd.h` 的 `mtx_t` / `cnd_t` 之上，把常见加锁模式封装成 ALib 风格对象和 `RAII(AAutoKey)`。
- 如果你需要创建线程、等待条件或直接操作 `tss_t`，用 `athrd.h`；如果你只需要在业务代码里保护临界区，通常直接用 `AMtx` / `ARecursion` / `AMtxRW` 更省心。

### 5.15 `alock.h` — 锁与自动解锁

`alock.h` 建立在 `athrd.h` 暴露的 `mtx_t` / `cnd_t` 之上，提供：

- `AMtx`：普通互斥锁
- `ARecursion`：递归互斥锁
- `AMtxRW`：读写锁
- `AAutoKey`：自动解锁 token

#### 5.15.1 类型总览

| API | 类别 | 说明 |
| --- | --- | --- |
| `ALock` | 类基类 | 所有锁类型的共同基类，内部持有 `mtx_t` |
| `AAutoKey` | 普通类型 | RAII 解锁 token，析构时执行保存的解锁函数 |
| `AMtx` | 类 | 普通互斥锁 |
| `ARecursion` | 类 | 递归互斥锁 |
| `AMtxRW` | 类 | 基于 `mtx_t + cnd_t` 实现的读写锁，带写者优先倾向 |

#### 5.15.2 `AAutoKey`

`AAutoKey` 常见用法：

```c
RAII(AAutoKey) key = AMtx_lock(&lock);
if (aExcOccur()) {
    return;
}
```

语义：

- `AAutoKey` 析构时，如果内部 `lock` 和 `unlock` 都非空，就自动解锁。
- `A_COPY(AAutoKey, key)` 会得到一个“空 token”，不会复制解锁责任；因此它天然近似不可复制。

#### 5.15.3 公共函数

| API | 参数 | 返回值 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `ALock_unlock(ALock* self)` | 锁基类指针 | 无 | 无 | 当前实现是空内联占位函数，通常不直接调用 |
| `ALock_uplock(ALock* self)` | 锁基类指针 | 无 | 无 | 当前实现是空内联占位函数，通常不直接调用 |
| `AMtx_lock(AMtx* self)` | 普通互斥锁指针 | `AAutoKey` | `AEXC_nullptr`、`AEXC_system_error` | 成功时返回带解锁器的 token |
| `ARecursion_lock(ARecursion* self)` | 递归锁指针 | `AAutoKey` | `AEXC_nullptr`、`AEXC_system_error` | 同一线程可重入 |
| `AMtxRW_rlock(AMtxRW* self)` | 读写锁指针 | `AAutoKey` | `AEXC_nullptr`、`AEXC_system_error` | 获取读锁；存在等待写者时新读者也会阻塞 |
| `AMtxRW_wlock(AMtxRW* self)` | 读写锁指针 | `AAutoKey` | `AEXC_nullptr`、`AEXC_system_error` | 获取写锁；需要等待现有读者和写者全部离开 |

#### 5.15.4 初始化 / 拷贝 / 析构语义

- `A_INIT(AMtx)` / `A_INIT(ARecursion)` / `A_INIT(AMtxRW)` 会创建一个新的未持有锁实例。
- `A_COPY(AMtx)` / `A_COPY(ARecursion)` / `A_COPY(AMtxRW)` **不会复制锁状态**，而是创建一把新的、未加锁的同类锁。
- 锁对象析构时会销毁底层系统锁 / 条件变量。

#### 5.15.5 使用建议

- 优先使用 `RAII(AAutoKey)` 管理解锁，不要手工模拟“多 return 点解锁”。
- 不要复制 `AAutoKey` 并试图让多个 token 共同管理一次加锁；复制后的 token 是空对象。
- `AMtxRW` 对写者更友好：只要有写者等待，新读者就会阻塞，避免写者饥饿。

### 5.16 `asignal.h` — 信号系统与接收者基类

`asignal.h` 提供进程内同步派发信号系统。它支持：

- 申请新的信号 id
- 连接 / 断开回调
- 同步发送信号
- 收集回调执行时抛出的异常
- 让接收者在析构时自动断连

#### 5.16.1 主要类型

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

#### 5.16.2 `ASignal`

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

#### 5.16.3 `AExcCollector`

`AExcCollector` 用于把回调执行过程中的异常收集起来，而不是只留下最终的 `AEXC_response_exc`。

公共 API：

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AExcCollector_pop(AExcCollector* collector)` | 收集器指针 | 返回一个 `AExcEnd` | 空列表时可能透出底层 `AEXC_overstep` | `collector == nullptr` 时返回零值记录；非空收集器按 LIFO 顺序弹出最后一条记录 |
| `AExcCollector_empty(const AExcCollector* collector)` | 收集器指针，可为 `nullptr` | `bool` | 无 | `collector == nullptr` 时返回 `true` |
| `AExcCollector_getNumber(const AExcCollector* collector)` | 收集器指针，可为 `nullptr` | `uint32_t` | 无 | `collector == nullptr` 时返回 `0` |
| `A_CALL(collector, AExcCollector).pop(...)` 等 | 对象方法形式 | 与上面一致 | 同上 | 类方法表只是这些函数的包装 |

`AExcCollector` 还暴露两个重要字段：

- `id`：本次 `a_signal_transmit` 正在派发的信号 id
- `exc`：收集器自身的异常标志；如果收集过程中连异常列表都无法追加（例如分配失败），该标志会被置位，派发过程也会中止继续收集

#### 5.16.4 信号系统公共 API

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `a_signal_alloc()` | 无 | 新的正整数信号 id；失败时通常为 `-1` | `AEXC_system_error` | 申请一个新的信号类型 id |
| `__a_signal_transmit(const ASignal* signal, AExcCollector* collector)` | 信号指针、可选收集器 | 发送信号 | `AEXC_system_error`、`AEXC_nullptr`、`AEXC_outdomain`、`AEXC_response_exc` | 低层实现函数；业务代码通常调用 `a_signal_transmit` 宏 |
| `a_signal_transmit(signal, ...)` | 变参宏：必传 `signal`，可选 1 个 `AExcCollector*` | 发送信号 | 同 `__a_signal_transmit` | 宏变参上限 1；会做编译期类型断言 |
| `a_signal_connection(Aint id, const void* addressee, void(*call)(const ASignal*, void*))` | 信号 id、接收者地址、回调函数 | 建立连接 | `AEXC_outdomain`、底层分配异常 | 同一个 `(id, addressee)` 若已存在连接，则会覆盖原有回调 |
| `a_signal_disconnect(Aint id, const void* addressee)` | 信号 id、接收者地址 | 删除一条连接 | `AEXC_outdomain` | 在回调执行期间调用会延迟到本轮派发结束后执行；“连接不存在但参数合法”通常静默返回 |
| `a_target_disconnect(const void* addressee, Aint id)` | 接收者地址、信号 id | 删除一条连接 | 同 `a_signal_disconnect` | 只是参数顺序相反的内联包装 |
| `a_signal_disconnect_all(Aint id)` | 信号 id | 删除该信号 id 的全部连接 | `AEXC_outdomain` | 若该 id 当前没有连接，通常静默返回 |
| `a_target_disconnect_all(const void* addressee)` | 接收者地址 | 删除该接收者的全部连接 | `AEXC_outdomain` | 在回调执行期间调用同样会延迟到本轮派发结束后执行 |

#### 5.16.5 `a_signal_transmit` 宏的变参规则

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

#### 5.16.6 运行时语义

- 发送信号是同步行为：所有当前连接的回调都在调用线程内执行。
- 派发开始前，系统会拷贝当前信号 id 对应的连接快照；因此“回调里新增的连接”不会参与当前这一轮派发，只会影响后续派发。
- 如果某个回调设置了异常：
  - `a_signal_transmit` 返回 `AEXC_response_exc` 表示有回调抛出异常（否则返回 0）；
  - 无收集器时：返回 `AEXC_response_exc`，但异常槽（`aExcGet()`）已在返回到达前被清理；
  - 有收集器时：返回 `AEXC_response_exc`，同时收集器中追加一条 `AExcEnd`，异常槽同样已清理。
- 收集器的记录顺序是”回调执行顺序追加、`pop` 时逆序弹出”。
- 向一个”已分配但当前无人连接”的合法 `id` 发送信号，会报 `AEXC_outdomain`。
- 向一个”从未分配过或超出当前范围”的 `id` 发送信号，同样会报 `AEXC_outdomain`。

#### 5.16.7 `AReceEnd`

`AReceEnd` 是接收者辅助基类，适合“对象析构时自动断开全部连接”的场景。

公共方法：

| API | 参数 | 返回值 / 效果 | 异常 | 说明 |
| --- | --- | --- | --- | --- |
| `AReceEnd_connection(const AReceEnd* self, Aint id, void(*call)(const ASignal*, void*))` | 接收者、信号 id、回调 | 建立连接 | `AEXC_nullptr`，以及 `a_signal_connection` 的异常 | 实际上就是把 `self` 当作 `addressee` |
| `AReceEnd_disconnect(const AReceEnd* self, Aint id)` | 接收者、信号 id | 删除该接收者在此 id 上的连接 | `AEXC_nullptr`，以及 `a_signal_disconnect` 的异常 | |
| `A_CALL(obj, AReceEnd).connection(...)` / `disconnect(...)` | 方法表调用 | 同上 | 同上 | 面向类对象的常用调用方式 |

析构语义：

- `AReceEnd` 的析构函数会调用 `a_target_disconnect_all(self)`；
- 因此只要你的接收者对象继承 `AReceEnd`，一般不需要手工在所有正常析构路径里逐条断连。

#### 5.16.8 使用约束（非常重要）

当前实现要求：

1. 已连接的接收者对象在析构前必须断开连接，或者直接继承 `AReceEnd`。  
2. 回调执行期间可以断连（`disconnect` / `disconnect_all`），但实际操作会延迟到本轮派发结束后执行。  
3. 同一个 `(id, addressee)` 重复调用 `a_signal_connection` / `AReceEnd_connection` 时，会更新为新的回调函数，而不是保留旧回调。  
4. 回调里不要析构任何仍处于连接表中的接收者对象，否则会制造悬空指针。  
5. 回调里不要等待其他线程执行信号连接 / 断开操作，否则可能形成锁等待。  

## 6. 示例与测试入口

示例程序：

- `sample/sample_type.c`：自定义类型与对象生命周期
- `sample/sample_aclass.c`：类系统与虚函数覆盖
- `sample/sample_aline.c`：容器生成、遍历与值语义
- `sample/sample_alock.c`：`AAutoKey` 与锁的 RAII 使用
- `sample/sample_asignal.c`：信号连接、派发与异常收集

回归测试：

- `test/test_sequence_api.c`：序列容器共性 API
- `test/test_map_api.c`：映射容器共性 API
- `test/test_athrd.c`：线程兼容层、定时锁、条件变量、TSS 与 `call_once`
- `test/test_asignal.c`：信号系统、重入发送、异常收集
- `test/test_aptr.c`：`APtr` / `AShPtr` 语义
- 其余 `test/test_*.c`：各模块单独行为测试

推荐最少验证命令：

```bash
make
make -C test
```

如果修改了示例相关接口，再补：

```bash
make -C sample
```

## 7. 常见误区速记

- `AString_new()` 只是借用，不是复制。
- `APtr(T)` 复制后不会转移所有权，只会得到弱别名。
- `AHash(K,V)` 的比较结果不等价于“无序集合相等”。
- 顺序容器的 `at(index)` 普遍是“空容器报错，非空越界截断到尾元素”。
- `take` / `pop` 把元素写到 `tar` 后，后续由调用方负责析构该元素。
- 迭代时一旦改容器结构，就应重新获取迭代器。
- `a_signal_transmit(signal, ...)` 只允许 0 或 1 个附加参数。
- `AClass_Inherit(T, ...)` 和 `A_CALL(obj, ...)` 都是宏变参，但附加参数上限都只有 1 个。
