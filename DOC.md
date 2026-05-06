# ALib 开发文档

## 目录

- [1. 项目定位](#1-项目定位)
- [2. 构建与安装](#2-构建与安装)
- [3. 头文件与包含路径](#3-头文件与包含路径)
- [4. 五分钟上手](#4-五分钟上手)
- [5. 核心设计](#5-核心设计)
- [6. 类型系统详解](#6-类型系统详解)
- [7. 类系统](#7-类系统)
- [8. 对象生命周期](#8-对象生命周期)
- [9. 异常模型](#9-异常模型)
- [10. 统一迭代器](#10-统一迭代器)
- [11. 容器说明](#11-容器说明)
- [12. 字符串与指针](#12-字符串与指针)
- [13. 自定义类型](#13-自定义类型)
- [14. 内存分配器与弱符号](#14-内存分配器与弱符号)
- [15. 测试与本地验证](#15-测试与本地验证)
- [16. 使用约定与注意事项](#16-使用约定与注意事项)

---

## 1. 项目定位

ALib 试图解决这样一个问题：在纯 C 里，如何让容器像 C++ STL 一样具有一致的值语义、类型约束、统一遍历方式和清晰的资源管理边界。

它的做法不是引入运行时元数据，而是把以下几件事放到编译期完成：

- 用宏实例化容器类型
- 用类型注册宏约定对象的生命周期接口
- 用函数表把不同容器统一到相似的操作模型上
- 用 GNU `cleanup` 扩展提供近似 RAII 的使用体验
- 用虚函数表实现单继承类系统和运行时多态

因此，ALib 更适合：

- 想在 C 项目里获得更强容器抽象的人
- 想研究"宏泛型 + 对象语义"设计的人
- 教学、实验、小中型基础组件封装

它不追求的方向包括：

- 纯标准 C 兼容
- 零宏心智负担
- 与 C++ STL 完全一致的边界语义

---

## 2. 构建与安装

### 2.1 构建静态库

在仓库根目录执行：

```bash
make
```

默认会生成：

- `libatlan.a`

根 `Makefile` 使用的主要参数：

```make
CPPFLAGS := -Iinc
CFLAGS   := -std=c11 -Wall -Wextra -Werror -O2 -fPIC
```

### 2.2 安装

```bash
make install PREFIX=/usr/local
```

安装结果：

- 头文件：`/usr/local/include/alib/*.h`
- 静态库：`/usr/local/lib/libatlan.a`

卸载：

```bash
make uninstall PREFIX=/usr/local
```

### 2.3 适合谁用哪种方式

- 想把 ALib 当作系统库使用：先 `make install`
- 想在当前仓库里临时验证：不安装也可以，但需要手动准备头文件别名目录

---

## 3. 头文件与包含路径

### 3.1 公共包含方式

对外使用时，推荐统一写成：

```c
#include <alib/alib.h>
#include <alib/aline.h>
#include <alib/astring.h>
#include <alib/aclass.h>
```

这对应安装后的目录布局：

```text
PREFIX/include/
└── alib/
    ├── alib.h
    ├── aclass.h
    ├── aline.h
    └── ...
```

### 3.2 仓库内临时编译

如果你还没安装库，但想直接在仓库根目录编译一个示例，可以这样做：

```bash
mkdir -p .local/include
ln -snf "$(pwd)/inc" .local/include/alib
gcc -std=c11 -Wall -Wextra -O2 -I.local/include example.c -L. -latlan -o example
```

这样 `.local/include/alib` 会映射到仓库里的 `inc/`。

### 3.3 为什么测试和示例默认更适合"安装后"使用

`test/` 和 `sample/` 中的源码使用的是 `<alib/...>` 形式，而对应 Makefile 默认没有把当前仓库根目录的 `inc/` 和 `libatlan.a` 自动暴露给编译器和链接器。

因此：

- 安装后直接 `make -C test` / `make -C sample` 最省心
- 未安装时，建议手动编译单个测试或示例

---

## 4. 五分钟上手

### 4.1 最小示例：动态数组

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
        line.f->pushBack(&line, i + 1);
    }

    forEach(it, line) {
        printf("%d ", *it.p);
    }
    printf("\n");
    return 0;
}
```

### 4.2 最小示例：红黑树映射

```c
#include <alib/alib.h>
#include <alib/atree.h>
#include <alib/astring.h>
#include <stdio.h>

ATree_Define(int, AString);
ATree_Generate(int, AString);
A_TYPE_REGISTER(ATree(int, AString));

int main(void) {
    RAII(ATree(int, AString)) tree = A_INIT(ATree(int, AString));

    RAII(AString) hello = A_INIT(AString);
    RAII(AString) lit = AString_new("hello");
    hello.f->addBack(&hello, &lit);

    tree.f->ins(&tree, 1, hello);

    AString *value = tree.f->at(&tree, 1);
    if (value != NULL) {
        printf("%s\n", value->s);
    }
    return 0;
}
```

### 4.3 最小示例：类系统

```c
#include <alib/alib.h>
#include <alib/aclass.h>
#include <stdio.h>

AClass_Inherit(Animal);

AClass_Struct(Animal,
    char* name;
);

AClass_Function(Animal,
    void (*speak)(const Animal* self);
);

static inline void Animal_speak(const Animal* self) {
    printf("%s makes a sound\n", self->name);
}

AClass_Generate(Animal, Animal_speak);

static inline void A_OBJ_INIT(Animal)(Animal* self) {
    self->name = "animal";
}
static inline void A_OBJ_DEST(Animal)(Animal* self) {}
static inline void A_OBJ_COPY(Animal)(Animal* self, const Animal* that) {
    self->name = that->name;
}
static inline int A_OBJ_CMPD(Animal)(const Animal* self, const Animal* that) {
    return strcmp(self->name, that->name);
}

A_CLASS_REGISTER(Animal);

int main(void) {
    RAII(Animal) a = A_INIT(Animal);
    A_CALL(a)->speak(&a);
    return 0;
}
```

### 4.4 你需要记住的三件事

- 容器通常拥有元素，插入时会拷贝传入对象
- `RAII(T)` 会在作用域结束时自动析构
- 出错后请检查 `aExcOccur()` / `aExcGet()`

---

## 5. 核心设计

### 5.1 类型注册

ALib 的所有复合对象都依赖统一的生命周期协议：

```c
A_TYPE_REGISTER(T);
```

注册后，类型 `T` 会获得以下约定接口：

- `A_OBJ_INIT(T)`：初始化
- `A_OBJ_DEST(T)`：析构
- `A_OBJ_COPY(T)`：拷贝
- `A_OBJ_CMPD(T)`：比较
- `A_OBJ_HASH(T)`：哈希

这几个接口并不要求你全部显式实现：

- 对原生标量类型，库已经内建默认实现
- 对自定义类型，你可以只实现需要覆盖的部分
- 容器内部只依赖这些统一入口，不关心元素真实类型

### 5.2 为什么容器还要再注册一次

例如：

```c
ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));
```

前两步只是生成 `ALine(int)` 这个容器类型本身。
第三步是让 `ALine(int)` 也成为一个"可构造、可拷贝、可析构"的对象，以便：

- 用 `RAII(ALine(int))`
- 用 `A_COPY(ALine(int), obj)`
- 把容器继续嵌套到别的容器或对象里

### 5.3 函数表与接口统一

所有容器都把操作组织在 `this->f` 函数表里，例如：

```c
line.f->pushBack(&line, 42);
list.f->popFront(&list, NULL);
```

这带来两个好处：

- API 风格统一
- 迭代器和工具宏可以依赖共同约定工作

### 5.4 各容器底层实现

- `ALine`：连续内存数组，支持头尾留白优化
- `ADeque`：按页分块的双端队列
- `AStack` / `AQueue`：`ADeque` 的受限接口包装
- `AList`：双向链表
- `ASortque`：基于数组的有序容器
- `ATree`：红黑树映射
- `AHash`：桶数组 + 桶内顺序存储
- `AString`：轻量字符串对象

---

## 6. 类型系统详解

### 6.1 类型注册机制

`A_TYPE_REGISTER(T)` 宏展开后做两件事：

1. **声明五个带有 weakref 的静态函数**，这些函数作为 fallback 存在：如果用户定义了同名强符号函数，链接器会解析到用户的实现；如果未定义，weakref 保持为 `nullptr`。

2. **生成五个包装函数**（`__A_OBJ_INIT_FUNC_SELF(T)`、`__A_OBJ_DEST_FUNC_SELF(T)` 等），这些是库内部和容器调用的真正入口。它们负责：
   - 空指针检查（设置 `AEXC_nullptr`）
   - 空对象保护（memset 为零的对象不析构，防止重复释放）
   - 调用用户的强符号实现（如果存在）
   - 异常发生时回滚（init/copy 中途失败会自动调用 dest）

### 6.2 基础类型注册

库已为所有 C 基础类型预注册了生命周期函数：

- 整数族：`int`, `long`, `short`, `char`, `bool`, `int8_t` ~ `int64_t`, `uint8_t` ~ `uint64_t`
- 浮点族：`float`, `double`
- 指针族：`cptr_t`, `cstr_t`, `astr_t`
- 特殊：`size_t`, `void`

这些类型的 `init` 为零初始化，`copy` 为值复制，`cmpd` 使用 `_Generic` 自动推导比较方式。

### 6.3 Weakref 机制

`A_TYPE_REGISTER` 的核心创新在于用 `__attribute__((weakref))` 实现"可选接口"：

```c
static void __A_OBJ_DEST(T)(T* self) __weakref(A_OBJ_DEST(T));
```

- 如果用户定义了 `A_OBJ_DEST(T)`（强符号），`__A_OBJ_DEST(T)` 解析为该地址
- 如果用户没有定义，`__A_OBJ_DEST(T)` 为 `nullptr`
- 包装函数在调用前检查是否为 `nullptr`，实现了"定义就用、不定义就跳过"

这比函数指针表更高效——链接器直接解析为强符号地址，无间接调用开销。同时提供了真正的"零开销抽象"——不定义就不付出任何代价。

---

## 7. 类系统

### 7.1 概述

头文件：`inc/aclass.h`

ALib 的类系统基于虚函数表（vtable）实现，在编译期通过宏生成类型安全的类层级。这是一个轻量级的单继承 C 对象模型，不依赖任何运行时反射。

核心特性：
- 单继承层级
- 虚函数表，支持运行时多态
- 延迟初始化（首次实例化时填充虚表）
- 子类可覆盖父类虚函数
- 通过 `A_CALL` 在基类视角调用子类方法

### 7.2 根基类 Atlan

所有类最终都继承自 `Atlan`，它是最小的根类型：

```c
typedef struct Atlan Atlan;
typedef struct A_FUNC(Atlan) A_FUNC(Atlan);

struct Atlan {
    const A_FUNC(Atlan)* f;  // 虚函数表指针
};

struct A_FUNC(Atlan) {
    bool flag;  // 延迟初始化标记，true 表示虚表已初始化
};
```

`Atlan` 的虚表在编译期就已填充完毕（`flag = true`），是唯一不需要延迟初始化的类。

### 7.3 定义类的四步流程

#### 第一步：声明继承关系

```c
AClass_Inherit(T);           // 继承自 Atlan（默认）
AClass_Inherit(T, Base);     // 继承自 Base（单继承）
```

这个宏做了以下事情：
- `typedef struct T T;` 和 `typedef struct A_FUNC(T) A_FUNC(T);`
- 声明外部虚表实例 `extern A_FUNC(T) A_FUNC_TAB(T);`
- 通过 `__AClass_Inherit(T, Base, Atlan)` 建立继承链：
  - `typedef Base __A_CLASS_BASE(T)`：记录基类类型
  - `typedef A_FUNC(Base) __A_FUNC_BASE(T)`：记录基类虚表类型
  - 保存基类的 init/dest/copy/cmpd 函数指针

#### 第二步：定义结构体

```c
AClass_Struct(T,
    int x;
    char* name;
);
```

展开为：

```c
struct T {
    union {
        const A_FUNC(T)* f;                // 虚表指针
        __A_CLASS_BASE(T) __base__;        // 基类数据
    };
    int x;
    char* name;
};
```

`union` 确保虚表指针在结构体首部，与基类布局兼容。

#### 第三步：定义虚函数表

```c
AClass_Function(T,
    void (*print)(const T* self);
    int  (*calc)(T* self, int n);
);
```

展开为：

```c
struct A_FUNC(T) {
    union {
        bool flag;                       // 延迟初始化标记
        __A_FUNC_BASE(T) __base__;       // 基类虚表
    };
    void (*print)(const T* self);
    int  (*calc)(T* self, int n);
};
```

虚表前部通过 union 嵌入基类虚表，保证内存布局兼容。不打算被子类覆盖的函数建议用 `const` 修饰函数指针。

#### 第四步：生成虚表并绑定函数

```c
AClass_Generate(T, func1, func2, ...);
```

展开为：

```c
__weak A_FUNC(T) A_FUNC_TAB(T) = { false, func1, func2, ... };
```

关键点：
- `flag = false` 表示虚表尚未初始化
- `__weak` 允许用户提供同名强符号覆盖
- `__VA_ARGS__` 按声明顺序填充虚表中的函数指针
- 首次实例化时，`A_OBJ_INIT` 会自动将基类虚表内容复制过来，再将 `flag` 设为 `true`，实现延迟初始化

### 7.4 类注册：A_CLASS_REGISTER

```c
A_CLASS_REGISTER(T);
```

这是类系统的 `A_TYPE_REGISTER` 等效宏。它比普通类型注册多了：

- **`A_SET_VTAB(T)`**：虚表初始化函数（子类在此覆盖父类虚函数）
- **构造链**：`INIT` 时先清零内存，然后复制基类虚表，再调用 `A_SET_VTAB`（如果首次初始化），最后调用用户的 `A_OBJ_INIT`
- **析构链**：`DEST` 时先调用用户 `A_OBJ_DEST`，再调用基类的 `DEST`，确保逐层析构
- **拷贝链**：`COPY` 时先复制基类虚表，然后 `A_SET_VTAB`，再调用用户 `A_OBJ_COPY`

### 7.5 子类与函数覆盖

定义子类：

```c
AClass_Inherit(MyClassSub, MyClass);
AClass_Struct(MyClassSub);           // 可添加新成员
AClass_Function(MyClassSub);         // 可扩展新虚函数
AClass_Generate(MyClassSub);
```

覆盖父类虚函数：

```c
// 1. 定义子类的函数实现
static inline void MyClassSub_print(const MyClassSub* _self) {
    MyClass* self = (void*)_self;   // 向上转型
    printf("sub: %s\n", self->s);
}

// 2. 实现 A_SET_VTAB 覆盖虚表
static inline void A_SET_VTAB(MyClassSub)(MyClassSub* self) {
    A_COVER_FUNC(self, MyClass, print, MyClassSub_print);
}

// 3. 注册
A_CLASS_REGISTER(MyClassSub);
```

`A_COVER_FUNC(self, Base, name, func)` 本质是：
```c
A_FUNC(Base)* tab = (void*)(self->f);
tab->name = (void*)func;
```

直接修改虚表中的函数指针，实现运行时多态。

### 7.6 调用虚函数

```c
// 直接调用
A_CALL(obj)->print(&obj);

// 以基类视角调用（触发子类覆盖的方法）
A_CALL(sub_obj, MyClass)->print((void*)&sub_obj);
```

`A_CALL(obj, Base)` 将对象的虚表转为基类类型指针，调用子类覆盖的实现。

### 7.7 类系统关键宏一览

| 宏 | 作用 |
|---|---|
| `AClass_Inherit(T, ...)` | 声明类 T，可选指定父类（默认 Atlan） |
| `AClass_Struct(T, ...)` | 定义类 T 的数据成员 |
| `AClass_Function(T, ...)` | 定义类 T 的虚函数表 |
| `AClass_Generate(T, ...)` | 生成类 T 的虚表实例，绑定函数 |
| `A_CLASS_REGISTER(T)` | 注册类 T 的完整生命周期 |
| `A_CALL(obj)` | 获取对象的虚函数表 |
| `A_CALL(obj, Base)` | 以基类 Base 的视角获取虚函数表 |
| `A_COVER_FUNC(self, Base, name, func)` | 覆盖基类虚表中的函数 |
| `A_SET_VTAB(T)` | 虚表初始化函数，子类在此覆盖父类方法 |

### 7.8 完整示例：动物类层级

```c
#include <alib/alib.h>
#include <alib/aclass.h>
#include <stdio.h>

/* ========== 基类 Animal ========== */
AClass_Inherit(Animal);

AClass_Struct(Animal,
    char* name;
);

AClass_Function(Animal,
    void (*speak)(const Animal* self);
);

static inline void Animal_speak(const Animal* self) {
    printf("%s makes a sound\n", self->name);
}

AClass_Generate(Animal, Animal_speak);

static inline void A_OBJ_INIT(Animal)(Animal* self) {
    self->name = "unknown";
}
static inline void A_OBJ_DEST(Animal)(Animal* self) {}
static inline void A_OBJ_COPY(Animal)(Animal* self, const Animal* that) {
    self->name = that->name;
}
static inline int A_OBJ_CMPD(Animal)(const Animal* self, const Animal* that) {
    return strcmp(self->name, that->name);
}

A_CLASS_REGISTER(Animal);

/* ========== 子类 Dog ========== */
AClass_Inherit(Dog, Animal);

AClass_Struct(Dog,
    char* breed;
);

AClass_Function(Dog,
    void (*wag)(const Dog* self);
);

static inline void Dog_speak(const Dog* _self) {
    Animal* self = (void*)_self;
    printf("%s barks!\n", self->name);
}

static inline void Dog_wag(const Dog* _self) {
    Animal* self = (void*)_self;
    printf("%s wags its tail\n", self->name);
}

AClass_Generate(Dog, Dog_speak, Dog_wag);

static inline void A_SET_VTAB(Dog)(Dog* self) {
    // Dog 的 speak 由 Dog_speak 覆盖
}

static inline void A_OBJ_INIT(Dog)(Dog* self) {
    self->name = "dog";
    self->breed = "golden retriever";
}
static inline void A_OBJ_DEST(Dog)(Dog* self) {}
static inline void A_OBJ_COPY(Dog)(Dog* self, const Dog* that) {
    self->name = that->name;
    self->breed = that->breed;
}
static inline int A_OBJ_CMPD(Dog)(const Dog* self, const Dog* that) {
    return strcmp(self->name, that->name);
}

A_CLASS_REGISTER(Dog);

/* ========== 使用 ========== */
int main() {
    RAII(Dog) dog = A_INIT(Dog);
    A_CALL(dog)->speak((void*)&dog);       // 输出: dog barks!
    A_CALL(dog)->wag(&dog);                // 输出: dog wags its tail

    RAII(Animal) animal = A_INIT(Animal);
    A_CALL(animal)->speak(&animal);        // 输出: unknown makes a sound

    return 0;
}
```

### 7.9 类系统注意事项

- **单继承限制**：`static_assert` 强制，尝试多继承会编译失败
- **匿名 union 与布局**：结构体首部使用 `union { f; __base__; }`，确保向下转型时内存布局与基类前部一致
- **延迟初始化**：每个类首次实例化时 `flag` 为 `false`，此时会触发虚表初始化（复制基类虚表 + 调用 `A_SET_VTAB`），之后 `flag` 设为 `true`，后续实例化跳过此步骤
- **析构顺序**：先用户析构 → 再基类析构，保证逐层清理
- **虚函数不可覆盖**：如果某函数不打算被子类覆盖，应声明为 `const` 函数指针
- **向上转型**：子类调用基类虚函数时，需显式 `(void*)` 转换

---

## 8. 对象生命周期

### 8.1 栈上对象

```c
RAII(AString) s = A_INIT(AString);
```

这等价于：

- 先调用构造逻辑
- 离开作用域时自动调用析构逻辑

对应常用宏：

```c
A_INIT(T)
A_COPY(T, obj)
A_DEST(T, obj)
RAII(T)
A_MOVE(obj)
```

### 8.2 堆上对象

```c
AString *p = A_NEW(AString);
A_DELETE(AString, p);
```

适合需要显式控制堆生命周期的场景。

### 8.3 容器的所有权语义

把对象插入容器时，容器会拷贝元素而不是直接偷走实参：

```c
line.f->pushBack(&line, obj);
```

语义上更接近"值插入"。

取出元素时有两种常见方式：

```c
line.f->popBack(&line, NULL);   // 删除并销毁元素
line.f->popBack(&line, &obj);   // 删除并把元素拷贝/转移到 obj
```

### 8.4 什么时候用 `A_MOVE`

当你明确要把一个临时对象按"搬移后清零"的方式转交给别的对象或容器时，可以使用：

```c
some_obj = A_MOVE(tmp);
```

它不会做深拷贝，而是把源对象按字节搬走后清零。

---

## 9. 异常模型

ALib 的"异常"本质上是线程局部错误码，不会中断控制流。

### 9.1 常用接口

```c
bool aExcOccur(void);
int  aExcGet(void);
void aExcSet(enum AEXC_t v);
void aExcClean(void);
```

### 9.2 常见错误码

- `AEXC_nullptr`：空指针
- `AEXC_overstep`：越界或元素不存在
- `AEXC_outdomain`：参数不在定义域内
- `AEXC_init_failed`：初始化失败
- `AEXC_alloc_failed`：内存分配失败
- `AEXC_invalid_function`：缺少必要函数

### 9.3 推荐用法

```c
line.f->pushBack(&line, value);
if (aExcOccur()) {
    fprintf(stderr, "error=%d\n", aExcGet());
    aExcClean();
}
```

建议规则：

- 一次逻辑操作后立即检查错误
- 处理完成后及时 `aExcClean()`
- 不要把旧错误状态带到后续流程里

---

## 10. 统一迭代器

ALib 为容器定义了一致的迭代协议。

### 10.1 常用宏

```c
AItHead(container)
AItTail(container)
AItNext(it)
AItPrev(it)
AItExist(it)
forEach(it, container)
forEachRev(it, container)
```

### 10.2 示例

```c
forEach(it, line) {
    printf("%d\n", *it.p);
}

forEachRev(it, line) {
    printf("%d\n", *it.p);
}
```

### 10.3 迭代器字段含义

统一底层迭代器包含：

- `p`：当前元素地址
- `con`：所属容器
- `i`：线性序号
- `r`：容器内部辅助位置，哈希表会用到

通常你不需要手动操作这些字段，只需要使用宏。

---

## 11. 容器说明

### 11.1 ALine：动态数组

头文件：`inc/aline.h`

特点：

- 连续存储
- 支持随机访问
- 支持头尾压入弹出
- 通过前后留白减少头部插入时的搬移频率

常见接口：

- `at`
- `rm`
- `ins`
- `take`
- `pushBack`
- `pushFront`
- `popBack`
- `popFront`

复杂度概览：

- `at`：O(1)
- 头尾插入/删除：均摊 O(1)
- 中间插入/删除：O(n)

### 11.2 AList：双向链表

头文件：`inc/alist.h`

特点：

- 节点独立分配
- 头尾插入删除稳定
- 适合频繁中间删除，不适合高频随机访问

复杂度概览：

- 头尾插入/删除：O(1)
- 按索引访问：O(n)

### 11.3 ADeque：双端队列

头文件：`inc/adeque.h`

特点：

- 基于分页块数组实现
- 头尾操作均摊 O(1)
- 随机访问 O(1)

适合：

- 双端推入/弹出频繁
- 又希望比链表有更好的局部性

### 11.4 AStack：栈

头文件：`inc/astack.h`

本质上是对 `ADeque` 的受限封装，暴露：

- `push`
- `pop`
- `at`
- 迭代接口

适合 LIFO 场景。

### 11.5 AQueue：队列

头文件：`inc/aqueue.h`

同样基于 `ADeque`，暴露：

- `push`
- `pop`
- `at`
- 迭代接口

适合 FIFO 场景。

### 11.6 ASortque：有序队列

头文件：`inc/asortque.h`

特点：

- 底层仍是数组
- 插入时自动按比较函数定位位置
- 适合数据量中等、读多写少的有序场景

复杂度概览：

- 查找插入位置：O(log n) 级思路 + 局部线性修正
- 真正插入搬移：O(n)
- 取最小/最大：接近 O(1)

### 11.7 ATree：红黑树映射

头文件：`inc/atree.h`

特点：

- 键值映射
- 自动保持有序
- 提供按键查找、插入、删除、遍历
- 迭代顺序为键顺序

使用方式：

```c
ATree_Define(int, AString);
ATree_Generate(int, AString);
A_TYPE_REGISTER(ATree(int, AString));
```

主要接口：

- `at(k)`：按键取值
- `ins(k, v)`：插入或覆盖
- `rm(k)`：删除
- `take(k, &v)`：删除并取出值
- `getk(it)`：从迭代器读取当前键

### 11.8 AHash：哈希表映射

头文件：`inc/ahash.h`

特点：

- 键值映射
- 平均情况下按键操作较快
- 遍历顺序依赖桶布局，不承诺按键有序

使用方式：

```c
AHash_Define(AString, int);
AHash_Generate(AString, int);
A_TYPE_REGISTER(AHash(AString, int));
```

注意：

- `AHash` 的比较依赖迭代顺序
- 同一组元素若插入顺序不同，比较结果未必相等

---

## 12. 字符串与指针

### 12.1 AString

头文件：`inc/astring.h`

`AString` 的关键字段：

- `s`：字符数据
- `number`：长度，不含 `\0`
- `capacity`：容量
- `noLiteral`：是否拥有可写堆内存

最常见的两种创建方式：

```c
RAII(AString) s0 = A_INIT(AString);       // 空字符串对象
RAII(AString) s1 = AString_new("hello"); // 非拥有包装
```

`AString_new` 创建的是一个"非拥有、通常指向字面量"的包装对象：

- 初始时不拷贝底层字符
- 第一次写操作时才转为可写堆缓冲
- 若传入的是外部临时缓冲区，请确保生命周期足够长，或尽快转为拥有型字符串

常用接口：

- `rm`
- `ins`
- `pushBack`
- `pushFront`
- `popBack`
- `popFront`
- `addBack`
- `addFront`
- `truncate`

把字面量安全地变成拥有型字符串的推荐方式：

```c
RAII(AString) lit = AString_new("hello");
RAII(AString) own = A_INIT(AString);
own.f->addBack(&own, &lit);
```

### 12.2 APtr：独占指针

头文件：`inc/aptr.h`

`APtr(T)` 提供简单的独占所有权语义：

- 默认构造时分配一个 `T`
- 拷贝时只复制裸指针，并把新对象标记成非 strong
- 析构时只有 strong 对象会真正释放底层资源

它更像一个轻量资源句柄，而不是完整的智能指针框架。

### 12.3 AShPtr：共享指针

头文件：`inc/aptr.h`

`AShPtr(T)` 基于引用计数内存块：

- 初始化时分配一个按 `max_align_t` 对齐的引用计数头，再跟随 payload
- 拷贝时增加引用计数
- 析构时减少计数，归零后释放

适合多个对象共享同一份大对象数据。

---

## 13. 自定义类型

### 13.1 一个完整示例

```c
#include <alib/alib.h>
#include <alib/astring.h>

typedef struct {
    AString name;
    int age;
} Person;

static inline void A_OBJ_INIT(Person)(Person *this) {
    this->name = A_INIT(AString);
    this->age = 0;
}

static inline void A_OBJ_DEST(Person)(Person *this) {
    A_DEST(AString, this->name);
}

static inline void A_OBJ_COPY(Person)(Person *this, const Person *that) {
    this->name = A_COPY(AString, that->name);
    this->age = that->age;
}

static inline int A_OBJ_CMPD(Person)(const Person *lhs, const Person *rhs) {
    int ret = A_CMPD(AString, lhs->name, rhs->name);
    if (ret == 0) {
        ret = lhs->age == rhs->age ? 0 : (lhs->age > rhs->age ? 1 : -1);
    }
    return ret;
}

A_TYPE_REGISTER(Person);
```

然后就可以把它放进容器：

```c
AList_Define(Person);
AList_Generate(Person);
A_TYPE_REGISTER(AList(Person));
```

### 13.2 设计建议

给自定义类型实现生命周期时，建议遵守：

- `INIT` 负责构造出一个有效空对象
- `DEST` 只做清理，不再抛新错误
- `COPY` 做深拷贝，不共享易变资源
- `CMPD` 保持自反、对称、传递
- `HASH` 若自定义，尽量与 `CMPD` 语义一致

---

## 14. 内存分配器与弱符号

ALib 在 `alib.c` 中把基础分配接口定义成弱符号：

```c
void  alib_free(void* p);
void* alib_alloc(uint32_t size);
void* alib_realloc(void* p, uint32_t size);
```

这意味着你可以在自己的工程里提供同名强符号，替换默认实现：

```c
#include <stdint.h>
#include <stdlib.h>

void *alib_alloc(uint32_t size) {
    return my_pool_alloc(size);
}

void alib_free(void *p) {
    my_pool_free(p);
}

void *alib_realloc(void *p, uint32_t size) {
    return my_pool_realloc(p, size);
}
```

适合：

- 接入项目统一分配器
- 调试内存行为
- 在嵌入式环境做定制分配

---

## 15. 测试与本地验证

### 15.1 安装后运行仓库自带测试

```bash
make
make install PREFIX=/usr/local
make -C test
```

### 15.2 安装后运行示例

```bash
make -C sample
```

### 15.3 不安装时手动验证单个模块

以 `ATree` 测试为例：

```bash
make
mkdir -p .local/include
ln -snf "$(pwd)/inc" .local/include/alib
gcc -std=c11 -Wall -Wextra -Werror -g -O0 \
    -I.local/include test/test_atree.c -L. -latlan -o .local/test_atree
./.local/test_atree
```

这种方式特别适合：

- 调试某个具体模块
- 做回归验证
- CI 中只针对某个测试文件单独编译运行

---

## 16. 使用约定与注意事项

### 16.1 关于越界访问

对于多个顺序容器，`at(index)` 的行为不是完全 STL 风格：

- 空容器上访问通常会设置异常并返回空值
- 非空容器上若 `index >= size`，很多实现会把索引截断到最后一个元素

因此，若你希望严格边界检查，请在调用前自己判断索引。

### 16.2 关于 `AString_new`

`AString_new` 更像"非拥有包装"，不是立即深拷贝：

- 传字面量是安全的
- 传临时栈缓冲区后若对象比缓冲区活得更久，就不安全
- 需要长期保存时，请显式复制到拥有型 `AString`

### 16.3 关于哈希表比较

`AHash` 的比较基于迭代顺序，而不是抽象集合语义。

也就是说：

- 元素完全相同
- 但插入顺序不同
- 比较结果仍可能不同

如果你的业务要的是"键值集合是否相同"，请自行按键比较。

### 16.4 关于 GNU 扩展依赖

ALib 依赖：

- `typeof`
- `_Generic`
- `__attribute__((cleanup))`
- 弱符号与弱引用

因此它不适合直接要求严格 MSVC 兼容或纯 ISO C 的场景。

### 16.5 关于多线程

错误状态是 `thread_local` 的，线程之间不会共享同一份异常值；但容器本身并没有做内部并发保护。

如果多个线程同时读写同一个对象，请自行加锁。

### 16.6 关于类系统

- 只支持单继承，尝试多继承会在编译期报错
- 子类调用基类虚函数时需要向上转型 `(void*)`
- 构造函数中不检查 `self == nullptr`，由包装函数统一处理
- 析构函数内部不允许设置新的异常状态

---

如果你只需要快速上手，请优先看 `README.md`。
如果你要把 ALib 集成到项目、扩展类型或分析容器语义，请以本文件为准。
