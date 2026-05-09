# ALib

ALib 是一个基于 C11 和 GNU 扩展实现的轻量级泛型容器库。它用宏、类型注册和函数表把 C 组织成接近 STL 的使用体验，提供统一的对象生命周期、迭代器接口和容器语义。

- 语言要求：C11+
- 编译器要求：支持 GNU 扩展的 gcc / clang / icc / armcc
- 许可证：GPLv3

## 特性

- 宏生成泛型容器，元素类型在编译期检查
- 用 `A_TYPE_REGISTER` 统一构造、析构、拷贝、比较、哈希行为
- 支持 `RAII(T)` 与 `cleanup` 风格自动析构
- 统一迭代器接口，容器都可用 `forEach` / `forEachRev`
- 无第三方依赖，内存分配器可通过弱符号覆盖
- 同时提供值语义对象、独占指针和引用计数指针
- 基于虚函数表的单继承类系统，支持运行时多态

## 和 GLib / STL 的定位对比

可以把 ALib 看成位于二者之间的一种折中方案：

| 维度 | ALib | GLib | STL |
|---|---|---|---|
| 目标语言 | C11 + GNU 扩展 | C | C++ |
| 泛型方式 | 宏实例化 + 类型注册 | 多数容器以 `gpointer` / 运行时约定为主 | 模板实例化 |
| 类型约束 | 偏编译期，常见误用可尽早暴露 | 偏运行时，靠 API 约定和使用者自律 | 强编译期类型系统 |
| 生命周期 | `A_TYPE_REGISTER` + `RAII(T)` + 值语义 | 以手动释放和指针所有权约定为主 | 原生 RAII + 值语义 |
| 容器风格 | 更接近 STL 的“对象 + 方法表”体验 | 更像通用 C 工具箱 | 标准 C++ 容器与算法体系 |
| 类/多态 | 内置轻量单继承类系统 | 通常配合 `GObject` 形成更完整对象系统 | 语言原生类、继承、模板 |
| 代价 | 依赖宏和 GNU 扩展，语法有一定心智负担 | 指针语义更重，类型信息较弱 | 必须接受 C++ 语言与工具链 |

如果只想快速理解三者差异，可以记成：

- 更像 GLib 的地方：仍然是 C 库，很多能力靠宏、约定和辅助基础设施补出来
- 更像 STL 的地方：强调值语义、统一容器接口、类型驱动的使用方式
- 和两者都不同的地方：ALib 同时把“容器 + 对象生命周期 + 轻量类系统”揉进了一套偏实验性的 C 风格接口

## 提供的模块

- `alib.h`：内存管理、异常状态、类型系统、RAII 宏
- `aiter.h`：统一迭代器协议
- `aclass.h`：单继承类系统，虚函数表与运行时多态
- `aline.h`：动态数组
- `alist.h`：双向链表
- `adeque.h`：双端队列
- `astack.h`：栈
- `aqueue.h`：队列
- `asortque.h`：有序队列
- `atree.h`：红黑树映射
- `ahash.h`：哈希表映射
- `astring.h`：字符串
- `aptr.h`：独占指针与共享指针

## 项目结构

```text
alib/
├── inc/           # 公共头文件
├── src/           # 库实现
├── test/          # 测试代码
├── sample/        # 示例代码
├── Makefile
├── README.md
└── DOC.md
```

## 快速开始

### 1. 构建静态库

```bash
make
```

成功后会生成 `libatlan.a`。

### 2. 安装到系统目录

```bash
make install PREFIX=/usr/local
```

安装后头文件位于 `PREFIX/include/alib`，静态库位于 `PREFIX/lib`。

### 3. 第一个程序

```c
#include <alib/alib.h>
#include <alib/aline.h>
#include <stdio.h>

ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));

int main(void) {
    RAII(ALine(int)) line = A_INIT(ALine(int));

    for (int i = 0; i < 5; ++i) {
        line.f->pushBack(&line, i * 10);
    }

    forEach(it, line) {
        printf("%d ", *it.p);
    }
    printf("\n");
    return 0;
}
```

已安装场景下可直接编译：

```bash
gcc -std=c11 -Wall -Wextra -O2 example.c -latlan -o example
```

如果你还没有安装库，只想在仓库目录里临时编译一个示例，可以先创建一个本地头文件别名目录：

```bash
mkdir -p .local/include
ln -snf "$(pwd)/inc" .local/include/alib
gcc -std=c11 -Wall -Wextra -O2 -I.local/include example.c -L. -latlan -o example
```

## 容器生成模式

ALib 的容器是通过宏实例化的。典型流程是：

1. `XXX_Define(T)`：声明容器类型与函数表
2. `XXX_Generate(T)`：生成该类型对应的内联实现
3. `A_TYPE_REGISTER(XXX(T))`：让容器自己也能参与 RAII、拷贝与嵌套

例如：

```c
AList_Define(AString);
AList_Generate(AString);
A_TYPE_REGISTER(AList(AString));
```

映射类型使用键值双参数：

```c
ATree_Define(int, AString);
ATree_Generate(int, AString);
A_TYPE_REGISTER(ATree(int, AString));
```

## 类系统

ALib 提供了一套基于虚函数表的单继承类系统，允许在 C 中定义具有运行时多态的类层级。

### 定义与使用

```c
#include <alib/alib.h>
#include <alib/aclass.h>

// 1. 声明继承关系（未指定父类则继承自 Atlan）
AClass_Inherit(MyClass);

// 2. 定义类的数据成员
AClass_Struct(MyClass,
    char* s;
);

// 3. 定义类的虚函数表（声明可被子类重写的函数）
AClass_Function(MyClass,
    void (*print)(const MyClass* self);
);

// 4. 实现成员函数
static inline void MyClass_print(const MyClass* self) {
    printf("%s\n", self->s);
}

// 5. 生成虚函数表实例
AClass_Generate(MyClass, MyClass_print);

// 6. 实现生命周期函数
static inline void A_OBJ_INIT(MyClass)(MyClass* self) {
    self->s = "hello";
}
static inline void A_OBJ_DEST(MyClass)(MyClass* self) {
    // 清理资源
}
static inline void A_OBJ_COPY(MyClass)(MyClass* self, const MyClass* that) {
    self->s = that->s;
}
static inline int A_OBJ_CMPD(MyClass)(const MyClass* self, const MyClass* that) {
    return strcmp(self->s, that->s);
}

// 7. 注册类型
A_CLASS_REGISTER(MyClass);

// 使用
RAII(MyClass) obj = A_INIT(MyClass);
A_CALL(obj)->print(&obj);
```

### 子类与函数覆盖

```c
// 子类继承于 MyClass
AClass_Inherit(MyClassSub, MyClass);
AClass_Struct(MyClassSub);
AClass_Function(MyClassSub);
AClass_Generate(MyClassSub);

// 子类的 print 实现
static inline void MyClassSub_print(const MyClassSub* _self) {
    MyClass* self = (void*)_self;
    printf("sub: %s\n", self->s);
}

// 在 VTAB 初始化时覆盖父类函数
static inline void A_SET_VTAB(MyClassSub)(MyClassSub* self) {
    A_COVER_FUNC(self, MyClass, print, MyClassSub_print);
}

A_CLASS_REGISTER(MyClassSub);

// 调用覆盖后的函数
RAII(MyClassSub) sub = A_INIT(MyClassSub);
A_CALL(sub, MyClass)->print((void*)&sub);  // 输出: sub: hello
```

### 关键宏

| 宏 | 作用 |
|---|---|
| `AClass_Inherit(T, ...)` | 声明类 T 及其父类（可选，默认 Atlan） |
| `AClass_Struct(T, ...)` | 定义类的数据成员 |
| `AClass_Function(T, ...)` | 定义虚函数表扩展 |
| `AClass_Generate(T, ...)` | 生成虚函数表实例并绑定函数 |
| `A_CLASS_REGISTER(T)` | 注册类的生命周期函数 |
| `A_CALL(obj)` | 获取对象的虚函数表指针 |
| `A_CALL(obj, Base)` | 将对象视为基类并获取虚函数表 |
| `A_COVER_FUNC(self, Base, name, func)` | 在 VTAB 初始化时覆盖基类函数 |
| `A_SET_VTAB(T)` | 虚表初始化函数，用于覆盖父类虚函数 |

### 注意事项

- 只支持单继承，编译期 `static_assert` 强制约束
- 虚函数表中不打算被子类覆盖的函数应使用 `const` 函数指针
- 根基类 `Atlan` 自身只保存虚表指针；其根虚表至少包含 `flag` 和内部析构入口 `dest`
- 子类覆盖函数时，`A_SET_VTAB(T)` 在对象首次实例化时调用

## 对象语义

可以把 ALib 看成"容器拥有元素"的值语义库：

- `pushBack` / `pushFront` / `ins` 会拷贝传入对象
- `pop` / `take` 可选择把元素取出到调用方，也可以直接销毁
- `A_COPY(T, obj)` 是深拷贝入口
- `RAII(T)` 会在离开作用域时调用析构逻辑

常用宏：

- `A_INIT(T)`：构造一个栈上对象
- `A_COPY(T, obj)`：深拷贝一个对象
- `A_DEST(T, obj)`：手动析构一个非 `const` 左值，并把该对象清零为空状态
- `A_NEW(T)` / `A_DELETE(T, p)`：堆上对象创建与销毁；`A_DELETE` 不会自动把指针变量置空
- `RAII(T)`：自动析构
- `A_MOVE(obj)`：按"搬移后清零"语义转移一个非 `const` 左值
- `A_LEFT(obj)`：把右值包装成临时左值，便于传给要求左值的宏

常见组合：

```c
RAII(AString) s = A_INIT(AString);
/* ... */
A_DEST(AString, s);   // 提前释放，并把 s 置为空对象

AString *p = A_NEW(AString);
/* ... */
A_DELETE(AString, p);
p = nullptr;          // 若后续仍会访问变量，建议手动置空
```

### 对象生命周期速览

| 路径 | 常见创建方式 | 谁拥有对象 | 常见释放方式 | 备注 |
|---|---|---|---|---|
| 栈对象 | `T obj = A_INIT(T);` / `RAII(T) obj = A_INIT(T);` | 当前作用域 | `RAII` 自动析构，或 `A_DEST(T, obj)` 提前释放 | `A_DEST` 后对象会被清零，适合提前释放资源 |
| 堆对象 | `T *p = A_NEW(T);` | 调用方 | `A_DELETE(T, p);` | `A_DELETE` 只释放 `p` 指向的对象，不会自动把指针变量置空 |
| 容器元素 | `pushBack` / `pushFront` / `ins` / `set` | 容器内部副本 | 容器删除元素、整体析构，或 `take` / `pop` 取出 | 插入时会拷贝元素，不会直接偷走传入实参 |

把它理解成三句话会更直观：

- 栈对象：默认跟随作用域，必要时可用 `A_DEST` 提前结束生命周期
- 堆对象：由你显式创建和释放，释放后若变量还要继续使用，记得手动 `p = nullptr`
- 容器元素：所有权一旦进入容器，就由容器负责析构；若想把旧值搬出来再处理，可配合 `A_MOVE` / `take`

## 异常模型

ALib 不使用 `setjmp/longjmp`，也不抛出 C++ 异常。失败通过线程局部错误码报告：

```c
if (aExcOccur()) {
    int err = aExcGet();
    aExcClean();
}
```

常见错误包括：

- `AEXC_nullptr`
- `AEXC_overstep`
- `AEXC_init_failed`
- `AEXC_alloc_failed`
- `AEXC_invalid_function`

## 重要使用约定

- 库依赖 GNU 扩展，不是纯 ISO C 头文件库
- `A_DEST(T, obj)` 与 `A_MOVE(obj)` 只接受非 `const` 左值；需要把右值交给这类宏时可用 `A_LEFT(...)`
- `AString_new("...")` 创建的是非拥有字符串包装；发生写操作或显式深拷贝后才会转成堆内存
- 若把外部缓冲区包装成 `AString` 并长期存入容器，请先转成拥有型字符串
- 顺序容器的 `at(index)` 在非空时通常会把越界索引截断到最后一个元素，而不是统一报错
- `AHash` 的比较语义依赖迭代顺序，而不是数学意义上的"无序集合相等"

## 测试与示例

仓库自带测试和示例源码，但它们默认面向"已安装头文件和库"的布局。

推荐流程：

```bash
make
make install PREFIX=/usr/local
make -C test
make -C sample
```

如果你只是想在当前仓库里做本地验证，建议参考 `DOC.md` 中的"本地不安装验证"说明手动编译。

## 文档

- `README.md`：快速了解项目与上手方式
- `DOC.md`：详细开发文档、架构说明和 API 约定
