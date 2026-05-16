# ALib 开发文档

本文基于当前仓库中的 `inc/`、`src/`、`sample/` 和 `test/` 代码重写，重点说明 ALib 现在已经实现了什么、接口语义是什么，以及它与 GLib 的设计差异。

## 1. 项目定位

ALib 的目标不是把 GLib 原样搬到 C11 之外，而是解决另一个问题：

在纯 C 里，如何让容器、对象和资源管理尽量具备下面这些特性：

- 容器类型在编译期就绑定元素类型
- 对象的初始化、拷贝、析构和比较有统一入口
- 容器默认按值拷贝、按值析构，而不是把生命周期全留给调用方
- 能用接近 RAII 的方式管理栈上对象和锁
- 在不引入完整运行时反射系统的前提下，提供一套轻量级单继承和虚函数表机制

因此，ALib 的定位可以描述为：

- 面向 C11 + GNU 扩展的底层工具库
- 在方向上对标 GLib，但范围更窄、更偏容器与对象管理
- 在使用体验上借鉴一部分 STL 的值语义和容器思路
- 在工程成熟度和生态广度上，明显比 GLib 更轻、更实验性

它更适合：

- 纯 C 项目中的内部基础库
- 对类型约束和对象生命周期有明确要求的模块化代码
- 希望在 C 中获得“泛型容器 + RAII 风格管理 + 轻量对象模型”的项目

它不追求：

- 纯 ISO C 的最小兼容集
- GLib 全家桶式的运行时设施
- 语言层级等价于 C++ STL / RAII / 类系统

## 2. 仓库结构与构建产物

```text
ALib/
├── inc/       # 公共头文件
├── src/       # 非模板部分实现
├── sample/    # 示例程序
├── test/      # 行为测试
├── Makefile
├── README.md
└── DOC.md
```

构建结果：

- 静态库：`libatlan.a`
- 安装头文件目录：`PREFIX/include/alib`
- 安装库目录：`PREFIX/lib`

常用命令：

```bash
make
make -C sample
make -C test
make install PREFIX=/usr/local
```

`sample/Makefile` 和 `test/Makefile` 会自动创建 `.local/include/alib -> inc/` 的符号链接，因此在仓库里直接构建示例和测试即可，不需要先执行系统级安装。

## 3. 核心设计

### 3.1 类型注册：`A_TYPE_REGISTER` 是整个库的中心

ALib 并没有 GLib 那样的通用运行时类型系统，而是通过宏约定每个类型的基础操作：

- 初始化：`A_OBJ_INIT(T)`
- 析构：`A_OBJ_DEST(T)`
- 拷贝：`A_OBJ_COPY(T)`
- 比较：`A_OBJ_CMPD(T)`
- 哈希：`A_OBJ_HASH(T)`

通过：

```c
A_TYPE_REGISTER(T);
```

把这些操作收束到统一入口后，库里的容器、指针包装和辅助宏都可以对任意已注册类型工作。

重要的是：这些基础函数不是全部强制要求提供。

如果你没有自己实现，ALib 会退回默认策略：

- init：零初始化
- dest：清零，不做额外释放
- copy：按字节复制
- cmpd：基础类型用自动比较，字符串用字符串比较，其余类型退回 `memcmp`
- hash：未定义时退回原始内存哈希

这是一种“编译期协议”，而不是 GLib/GObject 的运行时反射。

### 3.2 已内建注册的类型

`alib.h` 已经为这些常用类型准备了基础行为：

- 整数与浮点：`int`、`long`、`short`、`float`、`double`
- 定长整数：`int8_t` ~ `uint64_t`
- 布尔和字符：`bool`、`char`
- 指针/字符串别名：`cptr_t`、`cstr_t`
- 简单字符串视图：`astr_t`
- 基类：`Atlan`

因此诸如 `ALine(int)`、`AHash(int, int)` 这类实例化不需要先单独注册 `int`。

### 3.3 生命周期宏

ALib 把对象生命周期统一成几组宏：

- `A_INIT(T)`：创建并初始化一个栈上对象
- `A_COPY(T, obj)`：按该类型定义的 copy 规则复制对象
- `A_DEST(T, obj)`：析构一个栈上对象
- `A_NEW(T)`：在堆上分配并初始化对象
- `A_CPNEW(T, obj)`：在堆上复制构造对象
- `A_DELETE(T, p)`：析构并释放堆对象
- `A_MOVE(obj)`：移动对象并把原对象清零
- `RAII(T)`：用 GNU `cleanup` 在离开作用域时自动析构

例子：

```c
RAII(AString) s = A_INIT(AString);
RAII(AString) cp = A_COPY(AString, s);
```

这也是 ALib 和 GLib 很不一样的地方：ALib 把“对象值语义”放到了容器和基础宏的第一位，而 GLib 大量对象 API 仍以显式申请/释放和指针持有约定为中心。

### 3.4 异常模型：线程局部错误槽，而不是异常栈

ALib 没有 C++ 异常，也不采用 `GError**` 风格 API。它使用一个线程局部整型错误槽：

- `aExcClean()`：清空错误槽
- `aExcOccur()`：判断是否存在错误
- `aExcSet(v)`：设置错误码
- `aExcGet()`：读取错误码

常见错误码包括：

- `AEXC_nullptr`
- `AEXC_overstep`
- `AEXC_outdomain`
- `AEXC_alloc_failed`
- `AEXC_init_failed`
- `AEXC_repeat_write`
- `AEXC_system_error`
- `AEXC_response_exc`

这个模型的特征是：

- 成本低，调用简单
- 适合库内部联动和 RAII 风格清理
- 不是多层堆叠错误模型
- 后续操作可能覆盖前一个错误，因此调用方需要尽快检查和消费

换句话说，ALib 的错误处理更像“线程本地状态位”，而不是 GLib 的 `GError` 对象链路。

### 3.5 分配器钩子

`src/alib.c` 里默认提供了弱符号：

- `alib_alloc`
- `alib_realloc`
- `alib_free`
- `alib_new`
- `alib_cpnew`
- `alib_delete`

默认实现直接转到 `malloc/realloc/free`，但你可以在自己的程序中重写这些符号，把 ALib 接到自定义分配器、内存池或审计层上。

这类钩子是全局策略，不像 GLib 某些容器是在实例层面注入析构/比较/哈希函数。

### 3.6 统一迭代器协议

所有序列容器和映射容器都暴露统一的迭代入口：

- `head()` / `tail()`：得到首尾迭代器
- `next()` / `prev()`：前进与后退
- `forEach(it, con)` / `forEachRev(it, con)`：正向/反向遍历

统一迭代器 `AIter(ContainerType)` 至少包含：

- `p`：当前元素指针
- `con`：所属容器
- `i`：逻辑索引
- `r`：部分容器内部使用的额外位置字段

对调用者来说，最常用的是：

```c
forEach(it, line) {
    printf("%d\n", *it.p);
}
```

映射容器还会提供 `getk(it)` 用来取出当前键。

### 3.7 迭代器失效规则

ALib 的迭代器本质上是“当前元素地址 + 容器指针 + 辅助位置”的轻量快照，不是为结构修改设计的稳定游标。

保守且推荐的使用规则是：

- 只要容器结构发生变化，就把已有迭代器视为失效
- 结构变化包括：`ins`、`rm`、`take`、`pushBack`、`pushFront`、`popBack`、`popFront`
- 对映射容器还包括：`ins`、`rm`、`take` 触发的节点替换、rehash、桶搬移或树结构调整
- 容器析构、整容器拷贝后替换、重新初始化后，旧迭代器也全部失效

原因很直接：

- 顺序容器可能扩容、缩容或搬移底层存储
- 链表和树虽然某些节点地址在个别操作后可能仍然存在，但当前迭代器并没有“删除后返回下一个位置”的契约
- 哈希表插入/删除可能触发 rehash，导致桶数组和元素地址整体变化
- 迭代器里的 `i`、`r` 和 `p` 都可能与修改后的容器状态不再一致

因此，ALib 迭代器更适合：

- 只读遍历
- 修改当前元素内容，但不改容器结构

不适合：

- 边遍历边插入元素
- 边遍历边删除元素
- 把当前迭代器长期缓存起来，跨容器修改后继续使用

如果确实需要一边扫描一边修改，建议改成下面这些模式：

- 顺序容器：先记下索引，再按索引做修改；或先收集待删元素，遍历结束后再删
- 映射容器：先收集键，再统一 `rm` / `take`
- `AList(T)`：即使有 `rm_p` / `take_p`，也应在删除后立即丢弃旧迭代器，不要继续依赖当前遍历状态

## 4. 泛型容器的生成方式

### 4.1 基本套路

ALib 的泛型容器不是模板实例化，也不是 `void*` 容器，而是用宏生成具体类型：

```c
ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));
```

对应关系通常是：

1. `XXX_Define(...)`：声明容器结构和函数表
2. `XXX_Generate(...)`：生成具体类型的静态内联实现
3. `A_TYPE_REGISTER(XXX(...))`：让容器本身也成为可复制、可析构、可嵌套的对象

映射容器则使用键和值两个类型参数：

```c
ATree_Define(int, AString);
ATree_Generate(int, AString);
A_TYPE_REGISTER(ATree(int, AString));
```

### 4.2 类型行为如何传递到容器

容器不会保存“每个元素自己的回调指针”，而是直接复用元素类型的基础行为：

- 拷贝元素时调用 `A_COPY(T, obj)`
- 删除元素时调用 `A_DEST(T, obj)`
- 排序/比较时调用 `A_CMPD(T, lhs, rhs)`
- 哈希键时调用 `A_OBJ_HASH(T)` 或原始内存哈希

这也是 ALib 和 GLib 常见容器的一大差异：

- ALib：元素行为由“类型注册”统一决定
- GLib：元素行为常由“容器实例 + 用户回调”决定

### 4.3 边界语义需要特别记住

当前代码里，顺序容器大体分成两类越界规则：

- 容器为空：`at()`、`pop()`、`take()` 等会设置 `AEXC_overstep`
- 容器非空但索引过大：多数 `at(index)` 会把索引截断到最后一个元素

这和很多标准容器不同，文档和调用代码都应显式说明。

映射容器的规则更直接：

- `ATree(K,V)::at()` / `AHash(K,V)::at()` 找不到键时返回 `NULL`
- 同时会设置 `AEXC_overstep`

## 5. 模块详解

### 5.1 `ALine(T)`：动态数组

`ALine(T)` 基于连续内存数组实现，支持：

- `at`
- `rm`
- `ins`
- `take`
- `pushBack` / `pushFront`
- `popBack` / `popFront`
- `getNumber`
- `empty`

特点：

- 适合需要顺序访问和尾部追加的场景
- 元素以值方式存储
- 拷贝容器时会深拷贝元素（深度由元素类型自己的 copy 语义决定）
- 支持统一迭代器

### 5.2 `AList(T)`：双向链表

`AList(T)` 使用双向链表，接口与 `ALine(T)` 接近，但额外提供：

- `rm_p(self, p)`
- `take_p(self, p, tar)`

这两个接口直接按元素地址定位节点，效率更高，但也更危险。

安全使用约束：

- `p` 必须来自这个链表当前仍有效的节点
- 最稳妥的来源是同一容器的 `at()` 返回值或迭代器当前元素
- 一旦节点已删除、容器已析构、或容器经历重建，就不能继续持有旧地址

### 5.3 `ADeque(T)`、`AStack(T)`、`AQueue(T)`

这三者共享分块双端队列存储思路：

- `ADeque(T)`：通用双端队列，支持头尾压入/弹出
- `AStack(T)`：基于 `ADeque` 暴露 `push` / `pop` 的栈接口
- `AQueue(T)`：基于 `ADeque` 暴露 `push` / `pop` 的队列接口

它们的共同特点：

- 仍然是值语义容器
- 仍然可以 `A_COPY` 整个容器
- 仍然使用统一迭代器
- `at(index)` 也是非空时截断到尾元素

### 5.4 `ASortque(T)`：有序数组队列

`ASortque(T)` 本质上是“始终保持升序的数组容器”，核心接口包括：

- `ins(obj)`：按比较结果插入到合适位置
- `popMin` / `popMax`
- `rm` / `take`

语义要点：

- 有序性由 `A_CMPD(T, lhs, rhs)` 决定
- 插入不是稳定排序承诺，而是当前比较规则下的有序插入
- 非空时 `at(index >= num)` 同样截断到尾元素

如果元素比较规则本身不稳定或依赖外部状态，不要用它承载关键排序语义。

### 5.5 `ATree(K,V)`：红黑树映射

`ATree(K,V)` 是有序映射，支持：

- `at(k)`
- `rm(k)`
- `ins(k, v)`
- `take(k, tar)`
- `getk(it)`
- 顺序迭代

代码行为说明：

- 按 `K` 的比较函数组织红黑树
- 再次插入相同键时会替换已有键值对内容
- 遍历顺序是键的有序顺序
- `take(k, tar)` 只把值拿出来；键会在内部按类型语义析构

如果你希望稳定的有序遍历，`ATree` 是当前仓库里最合适的映射实现。

### 5.6 `AHash(K,V)`：哈希映射

`AHash(K,V)` 支持和 `ATree` 类似的外部接口，但内部使用哈希桶。

关键差异和注意事项：

- 同键 `ins(k, v)` 也是替换语义
- 键哈希优先使用 `A_OBJ_HASH(K)`；未提供时退回 `alib_hash()` 对原始字节做哈希
- 如果键类型带有填充字节、指针间接语义或自定义相等规则，强烈建议自己实现 `A_OBJ_HASH(K)` 和 `A_OBJ_CMPD(K)`
- 迭代顺序取决于桶布局和元素插入路径，不保证稳定
- `A_CMPD(AHash(...), lhs, rhs)` 比较的是迭代顺序下的元素序列，不等价于“数学意义上的无序集合相等”

最后一点非常重要：两个键值完全相同、但插入顺序不同的哈希表，比较结果可能不同。这是当前实现的既定行为。

### 5.7 `AString`：低层字符串对象

`AString` 不是 GLib `GString` 的对等替代，而是更轻的字节串对象。

接口包括：

- `rm`
- `ins`
- `pushBack` / `pushFront`
- `popBack` / `popFront`
- `addBack` / `addFront`
- `truncate`
- `getNumber`
- `getCapacity`
- `empty`

语义要点：

1. `AString_new(char* s)` 只是包装已有字符指针，不会立刻复制。  
2. `noLiteral == false` 时，字符串被视为非拥有型引用；首次写入会转成堆内存。  
3. 如果传入的是字符串字面量，这个行为很方便；如果传入的是栈缓冲区或临时内存，调用方必须在原缓冲区失效前完成一次真正的复制。  
4. `A_COPY(AString, s)` 对拥有型字符串会深拷贝，对字面量包装会共享指针；后续写入时再分配可写缓冲。  
5. 当前字符串语义是“字节串”，没有 UTF-8 校验、Unicode 处理和本地化辅助。

一个安全用法是：

```c
char buf[32];
snprintf(buf, sizeof(buf), "hello-%d", 7);

RAII(AString) owned = A_INIT(AString);
RAII(AString) tmp = AString_new(buf);
owned.f->addBack(&owned, tmp);
```

这会把 `buf` 的内容复制进 `owned`，而不是让 `owned` 直接悬空引用栈内存。

### 5.8 `APtr(T)` 与 `AShPtr(T)`

#### `APtr(T)`

`APtr(T)` 是“拥有型指针包装 + 弱别名复制”的语义：

- `APtrNew(T)`：分配并拥有一个 `T`
- `APtrCPNew(T, obj)`：复制构造并拥有一个 `T`
- 拷贝 `APtr(T)` 时，新对象只保存同一个原始指针，但 `strong_flag = false`

因此它并不等价于 `unique_ptr`：

- 复制不会转移所有权
- 弱副本析构时不会释放对象
- 只有强拥有者析构时才释放对象

如果你需要真正的所有权转移，请用 `A_MOVE` 或显式把旧对象置空，而不是直接拷贝。

#### `AShPtr(T)`

`AShPtr(T)` 是带原子引用计数的共享指针：

- `AShPtrNew(T)`：创建一个默认值对象，引用计数 1
- `AShPtrCPNew(T, obj)`：创建一个拷贝对象，引用计数 1
- `A_COPY(AShPtr(T), p)`：引用计数加一
- 最后一个共享者析构时释放底层对象

需要注意：

- 原子引用计数只保证 retain/release 本身安全
- 被共享对象内部并不会自动加锁
- 如果多个线程会同时修改 `*p.p`，仍需要外部同步

### 5.9 `AClass_*`：轻量单继承类系统

ALib 的类系统基于虚函数表和结构体首成员布局，提供：

- 单继承
- 构造/析构链
- 运行时多态析构
- 虚函数覆盖

常见宏：

- `AClass_Inherit(T, Base)`
- `AClass_Struct(T, ...)`
- `AClass_Function(T, ...)`
- `AClass_Generate(T, ...)`
- `A_CLASS_REGISTER(T)`
- `A_CALL(obj)` / `A_CALL(obj, Base)`
- `A_COVER_FUNC(self, Base, name, func)`

和 GLib/GObject 相比，它更轻也更窄：

- 只有单继承
- 没有属性系统
- 没有信号元信息
- 没有运行时反射或 introspection
- 更像“带 vtable 的对象协议”，而不是完整对象框架

这个系统适合内部抽象层，不适合对外暴露需要长期 ABI 演化的复杂类库。

### 5.10 `ASignal`、`AReceEnd` 与 `AExcCollector`

ALib 提供了一个进程内信号系统，用于“信号 id -> 接收者回调”的连接和派发。

核心接口：

- `a_signal_alloc()`：分配新的信号 id
- `a_signal_connection(id, addressee, callback)`：连接回调
- `a_signal_disconnect(id, addressee)`：断开单个连接
- `a_signal_disconnect_all(id)`：删除某个信号 id 的全部连接
- `a_target_disconnect_all(addressee)`：删除某个接收者的全部连接
- `a_signal_transmit(signal, collector?)`：同步派发信号

相关类型：

- `ASignal`：信号基类，包含 `id`、`value`、`sender`
- `AReceEnd`：接收者基类，析构时自动 `disconnect_all`
- `AExcCollector`：收集回调执行过程中抛出的异常

当前实现特征：

- 子系统通过 `constructor` / `destructor` 自动启动和关闭
- 内部使用读写锁保护全局连接表
- 支持回调中再次发送信号（重入发送）
- 支持在回调中申请新的信号 id 和注册新的连接
- 不允许在信号回调执行过程中做破坏性的断连操作，否则会报 `AEXC_repeat_write`
- 同一个 `(id, addressee)` 只能注册一个连接，重复连接会报 `AEXC_repeat_write`

它和 GLib/GObject 信号最大的不同在于：

- 这里只是轻量级的进程内观察者机制
- 没有类型化参数列表、没有元对象层、没有信号声明系统
- 更适合内部模块通信，不适合公开的对象元编程接口

### 5.11 `ALock`、`AMtx`、`ARecursion`、`AMtxRW`

锁模块基于 C11 `<threads.h>`：

- `AMtx`：普通互斥锁
- `ARecursion`：递归互斥锁
- `AMtxRW`：读写锁
- `AAutoKey`：自动解锁辅助对象

使用方式：

```c
RAII(AAutoKey) key = AMtx_lock(&lock);
if (aExcOccur()) {
    return;
}
```

离开作用域时，`AAutoKey` 会自动调用对应的解锁函数。

这体现了 ALib 的一条主线：

- 即使底层还是 C API
- 也尽量把资源释放收束到统一的对象析构流程里

## 6. 与 GLib 的差异：设计层面逐项说明

### 6.1 容器：编译期类型绑定 vs 运行时回调绑定

GLib 容器常见模式是：

- 元素以 `gpointer` 进入容器
- 比较/销毁/哈希行为按容器实例传入
- 元素的真实类型更多靠调用者自己维护

ALib 的做法是：

- 容器实例化时就写死元素类型
- 元素行为统一来自 `A_TYPE_REGISTER`
- 容器内部默认按值拷贝、按值析构

优点：

- 调用端更整齐
- 类型错误更容易在编译期暴露
- 容器嵌套更自然

代价：

- 宏更重
- 调试展开后的代码更复杂
- ABI 和源码组织都更偏“头文件驱动”

### 6.2 生命周期：值语义优先 vs 指针语义优先

GLib 里你经常要明确区分：

- 这个函数是否接管所有权
- 这个容器放进去的是对象指针还是字节块
- 什么时候应该 `g_free` / `g_object_unref`

ALib 试图把这些都收束到类型的基础函数里，让容器和值对象默认遵守同一套规则。

这会让内部组件更统一，但也要求你认真为自定义类型定义 copy/dest 语义。

### 6.3 对象系统：轻量 vtable vs 完整 GObject 体系

ALib 的类系统只解决这些问题：

- 继承
- 虚函数
- 多态析构

GLib/GObject 体系还解决：

- 动态类型注册
- 属性
- 信号元数据
- 反射与绑定
- 大量成熟工具链协同

所以 ALib 更适合“我需要一个干净的内部多态层”，而不是“我要一个完整可扩展对象平台”。

### 6.4 字符串与文本：低层字节串 vs 丰富文本工具

`AString` 的能力重心是：

- 持有字符串
- 追加、插入、裁剪
- 按对象语义复制/析构

而 GLib 在文本层面有远比这丰富的能力：

- UTF-8 辅助
- Unicode 工具
- 路径、格式化、字符串数组等辅助设施

如果你的项目有明显的文本/国际化需求，ALib 不能代替 GLib 的这部分积累。

### 6.5 信号：内部观察者机制 vs GObject 信号系统

ALib 信号系统可以满足：

- 模块内发布/订阅
- 错误收集
- 自动断连

但它不提供：

- 运行时信号描述
- 属性/参数元信息
- 绑定友好的反射能力

它更像一个“和类系统配套的内部通知机制”。

### 6.6 生态范围：ALib 更窄

当前仓库明确覆盖的是：

- 容器
- 字符串
- 指针包装
- 类系统
- 信号
- 锁

没有提供的典型 GLib 能力包括：

- 主循环
- 文件与路径工具集合
- 事件源与异步 IO
- 模块装载
- 丰富的字符集与文本辅助
- 公共 ABI 稳定承诺与大规模生态配套

因此“对标 GLib”更准确的理解应该是：

- ALib 想做 C 语言底层工具库
- 但它聚焦的是 GLib 里“容器/基础对象组织”这一段，而不是整条生态链

## 7. 使用建议与常见陷阱

### 7.1 为键类型显式实现比较和哈希

如果某个类型要作为 `AHash` 或 `ATree` 的键，最好显式提供：

- `A_OBJ_CMPD(T)`
- `A_OBJ_HASH(T)`

尤其是：

- 结构体里存在 padding
- 键语义不是简单按字节相等
- 键里含指针，但想按“指向内容”而不是“地址”比较

否则默认回退策略可能不符合你的业务语义。

### 7.2 `AString_new` 不等于复制字符串

`AString_new("abc")` 适合字面量。

`AString_new(buf)` 如果 `buf` 是栈数组或临时缓冲，只是借用了这个指针；必须尽快把它复制进拥有型 `AString`，否则会悬空。

### 7.3 析构函数不要抛异常

当前设计里，析构通常处在清理路径尾部；如果析构阶段再设置异常，会让调用栈中的错误语义变得很难判断。

实际使用中，应把 `A_OBJ_DEST(T)` 当成“不失败的清理函数”来写。

### 7.4 顺序容器的越界不是统一报错风格

`ALine` / `AList` / `ADeque` / `AStack` / `AQueue` / `ASortque` 的 `at(index)`：

- 空容器：报 `AEXC_overstep`
- 非空越界：通常截断到最后一个元素

如果你需要严格索引检查，请自己先比较 `index < getNumber()`。

### 7.5 迭代器不要用作结构修改游标

即使某些容器在某些操作下“看起来还能继续走”，也不要依赖这种偶然性。

更稳妥的规则是：

- 遍历时只读，或只改元素内容
- 插入/删除时重新获取迭代器
- 需要批量删改时，先收集索引、键或目标地址，再做第二轮修改

尤其不要写成这种模式：

```c
forEach(it, list) {
    list.f->rm_p(&list, it.p);   /* 删除后 it 已不再可靠 */
}
```

ALib 没有提供 STL 风格 “erase 后返回下一个有效迭代器” 的契约，因此删除当前元素后继续 `next(it)` 属于未受文档保障的用法。

### 7.6 `APtr` 复制不会转移所有权

很多使用者第一次看见 `APtr(T)` 会自然联想到唯一拥有者；但当前实现里：

- 原对象强拥有
- 拷贝对象弱引用

所以要么坚持不复制 `APtr`，要么在文档里明确这是“拥有者 + 观察者”模式，而不是“move-only 指针”。

### 7.7 哈希表比较不等价于集合比较

`AHash` 的比较是按当前迭代顺序逐项比较，不应把它当作“键值集合完全相同”的数学判定。

如果业务上需要集合相等，请自己按键逐项比较。

### 7.8 信号回调里不要做破坏性断连

当前实现明确限制：

- 不要在回调里断开当前连接对象
- 不要在回调里析构仍然连接着的接收者对象

如果需要自动清理接收者，优先让接收者继承 `AReceEnd`，把断连放到析构路径里处理。

## 8. API 速查表

这一节不重复展开设计背景，只按模块列出“最常用、最值得记住”的公开入口，方便查阅。

### 8.1 `alib.h`：对象、异常与内存

- 对象生命周期：`A_INIT(T)`、`A_COPY(T, obj)`、`A_DEST(T, obj)`
- 堆对象：`A_NEW(T)`、`A_CPNEW(T, obj)`、`A_DELETE(T, p)`
- 作用域析构：`RAII(T)`
- 移动语义：`A_MOVE(obj)`、`A_LEFT(obj)`
- 类型注册：`A_TYPE_REGISTER(T)`、`A_CLASS_REGISTER(T)`
- 异常槽：`aExcClean()`、`aExcOccur()`、`aExcSet(v)`、`aExcGet()`
- 分配器钩子：`alib_alloc()`、`alib_realloc()`、`alib_free()`、`alib_new()`、`alib_cpnew()`、`alib_delete()`
- 哈希辅助：`alib_hash()`、`alib_hash_str()`

### 8.2 `aiter.h`：统一迭代器

- 迭代器类型：`AIter(ContainerType)`
- 首尾迭代器：`AItHead(con)`、`AItTail(con)`
- 迭代推进：`AItNext(it)`、`AItPrev(it)`、`AItExist(it)`
- 遍历宏：`forEach(it, con)`、`forEachRev(it, con)`
- 访问当前元素：`it.p`
- 访问逻辑位置：`it.i`
- 使用建议：只把迭代器当遍历游标，不要把它当插入/删除游标

### 8.3 `aline.h`：`ALine(T)`

- 生成方式：`ALine_Define(T)`、`ALine_Generate(T)`、`A_TYPE_REGISTER(ALine(T))`
- 访问：`line.f->at(&line, index)`
- 插入/删除：`ins`、`rm`、`take`
- 双端操作：`pushBack`、`pushFront`、`popBack`、`popFront`
- 状态查询：`getNumber`、`empty`
- 典型场景：连续存储、随机访问、值语义数组

### 8.4 `alist.h`：`AList(T)`

- 生成方式：`AList_Define(T)`、`AList_Generate(T)`、`A_TYPE_REGISTER(AList(T))`
- 访问：`list.f->at(&list, index)`
- 按索引修改：`ins`、`rm`、`take`
- 按元素地址修改：`rm_p`、`take_p`
- 双端操作：`pushBack`、`pushFront`、`popBack`、`popFront`
- 状态查询：`getNumber`、`empty`
- 典型场景：频繁中间插入/删除，或需要稳定节点地址的链表场景

### 8.5 `adeque.h` / `astack.h` / `aqueue.h`

#### `ADeque(T)`

- 生成方式：`ADeque_Define(T)`、`ADeque_Generate(T)`、`A_TYPE_REGISTER(ADeque(T))`
- 访问：`at`
- 双端操作：`pushBack`、`pushFront`、`popBack`、`popFront`
- 状态查询：`getNumber`、`empty`

#### `AStack(T)`

- 生成方式：`AStack_Define(T)`、`AStack_Generate(T)`、`A_TYPE_REGISTER(AStack(T))`
- 栈接口：`push`、`pop`
- 额外能力：仍支持 `at`、统一迭代器、`getNumber`、`empty`

#### `AQueue(T)`

- 生成方式：`AQueue_Define(T)`、`AQueue_Generate(T)`、`A_TYPE_REGISTER(AQueue(T))`
- 队列接口：`push`、`pop`
- 额外能力：仍支持 `at`、统一迭代器、`getNumber`、`empty`

### 8.6 `asortque.h`：`ASortque(T)`

- 生成方式：`ASortque_Define(T)`、`ASortque_Generate(T)`、`A_TYPE_REGISTER(ASortque(T))`
- 访问：`at`
- 有序插入：`ins`
- 删除/取出：`rm`、`take`
- 最值弹出：`popMin`、`popMax`
- 状态查询：`getNumber`、`empty`
- 排序依据：元素类型的 `A_CMPD(T, lhs, rhs)`

### 8.7 `atree.h`：`ATree(K, V)`

- 生成方式：`ATree_Define(K, V)`、`ATree_Generate(K, V)`、`A_TYPE_REGISTER(ATree(K, V))`
- 查找：`tree.f->at(&tree, key)`
- 插入/替换：`ins`
- 删除/取出：`rm`、`take`
- 遍历辅助：`head`、`tail`、`next`、`prev`、`getk`
- 状态查询：`getNumber`、`empty`
- 典型场景：需要稳定有序遍历的映射

### 8.8 `ahash.h`：`AHash(K, V)`

- 生成方式：`AHash_Define(K, V)`、`AHash_Generate(K, V)`、`A_TYPE_REGISTER(AHash(K, V))`
- 查找：`hash.f->at(&hash, key)`
- 插入/替换：`ins`
- 删除/取出：`rm`、`take`
- 遍历辅助：`head`、`tail`、`next`、`prev`、`getk`
- 状态查询：`getNumber`、`empty`
- 键类型建议：显式实现 `A_OBJ_CMPD(K)`，必要时实现 `A_OBJ_HASH(K)`

### 8.9 `astring.h`：`AString`

- 字面量/借用包装：`AString_new(char *s)`
- 基本编辑：`rm`、`ins`、`truncate`
- 头尾操作：`pushBack`、`pushFront`、`popBack`、`popFront`
- 拼接：`addBack`、`addFront`
- 状态查询：`getNumber`、`getCapacity`、`empty`
- 重要字段：`s`、`number`、`capacity`、`noLiteral`

### 8.10 `aptr.h`：`APtr(T)` 与 `AShPtr(T)`

#### `APtr(T)`

- 生成方式：`APtr_Define(T)`、`APtr_Generate(T)`、`A_TYPE_REGISTER(APtr(T))`
- 创建：`APtrNew(T)`、`APtrCPNew(T, obj)`
- 复制语义：复制后得到弱别名，`strong_flag == false`
- 主要字段：`p`、`strong_flag`

#### `AShPtr(T)`

- 生成方式：`AShPtr_Define(T)`、`AShPtr_Generate(T)`、`A_TYPE_REGISTER(AShPtr(T))`
- 创建：`AShPtrNew(T)`、`AShPtrCPNew(T, obj)`
- 复制语义：复制时原子增加引用计数
- 主要字段：`p`

### 8.11 `aclass.h`：轻量类系统

- 继承声明：`AClass_Inherit(T)`、`AClass_Inherit(T, Base)`
- 数据结构：`AClass_Struct(T, ...)`
- 虚函数表：`AClass_Function(T, ...)`
- 生成虚表：`AClass_Generate(T, ...)`
- 注册类：`A_CLASS_REGISTER(T)`
- 调用虚函数：`A_CALL(obj)`、`A_CALL(obj, Base)`
- 覆盖父类函数：`A_COVER_FUNC(self, Base, name, func)`
- 自定义虚表初始化：`A_SET_VTAB(T)`

### 8.12 `asignal.h`：信号系统

- 基类：`ASignal`
- 接收者辅助基类：`AReceEnd`
- 异常收集器：`AExcCollector`
- 申请信号 id：`a_signal_alloc()`
- 发送信号：`a_signal_transmit(signal)`、`a_signal_transmit(signal, &collector)`
- 建立连接：`a_signal_connection(id, addressee, callback)`
- 断开连接：`a_signal_disconnect(id, addressee)`、`a_target_disconnect(addressee, id)`
- 批量断开：`a_signal_disconnect_all(id)`、`a_target_disconnect_all(addressee)`
- 收集异常：`AExcCollector_pop()`、`AExcCollector_empty()`、`AExcCollector_getNumber()`

### 8.13 `alock.h`：锁与自动解锁

- 基类：`ALock`
- 自动解锁辅助对象：`AAutoKey`
- 普通互斥锁：`AMtx`、`AMtx_lock()`
- 递归锁：`ARecursion`、`ARecursion_lock()`
- 读写锁：`AMtxRW`、`AMtxRW_rlock()`、`AMtxRW_wlock()`
- 推荐用法：`RAII(AAutoKey) key = AMtx_lock(&lock);`

## 9. 示例与测试入口

可以直接参考这些文件理解现有 API：

- `sample/sample_type.c`：类型注册与对象生命周期
- `sample/sample_aclass.c`：类系统与虚函数覆盖
- `sample/sample_aline.c`：动态数组与泛型容器
- `sample/sample_asignal.c`：信号派发与异常收集
- `sample/sample_alock.c`：锁与 `AAutoKey`

回归测试覆盖的重点包括：

- `test/test_sequence_api.c`：序列容器统一 API
- `test/test_map_api.c`：映射容器统一 API
- `test/test_asignal.c`：信号连接、重入发送、异常收集
- `test/test_aptr.c`：独占/共享指针包装语义

建议在修改容器或对象语义后至少执行：

```bash
make
make -C test
```

如果需要同时看使用方式，再补：

```bash
make -C sample
```

## 10. 总结

ALib 的特色不在于“覆盖 GLib 的全部能力”，而在于把下面几件事捏成了同一套风格：

- 编译期绑定类型的泛型容器
- 统一的 init/copy/dest/cmpd/hash 协议
- RAII 风格的对象与锁管理
- 轻量类系统
- 进程内信号机制

因此它最适合作为：

- 纯 C 项目里的内部基础库
- 希望获得更强类型化容器接口的工具层
- 研究 C 里“值语义 + 宏泛型 + 轻量对象模型”这条路线的代码库

如果你的需求已经超出这个边界，尤其是需要主循环、文本工具、成熟对象反射或广泛生态，那么 GLib 仍然是更稳妥、更成熟的选择。
