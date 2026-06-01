/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib type system sample
 */

#include <alib/alib.h>
#include <alib/aclass.h>
#include <stdio.h>

//定义类的继承关系
//未指定父类，则自动继承于Atlan
AClass_Inherit(My_class);

AClass_Struct(My_class,
    char* s;
);

AClass_Function(My_class,
    //如果希望此函数能被子类重写，则不应使用const
    //void (* const print)(const My_class* self);
    void (*print)(const My_class* self);
);

//成员函数实例
static inline void My_class_print(const My_class* self){
    if(self != nullptr && self->s != nullptr){
        printf("My_class:%s\n", self->s);
    }else{
        aExcSet(AEXC_nullptr);
        printf("nullptr\n");
    }
}

//生成函数表
AClass_Generate(My_class, My_class_print);

//为My_class提供基础函数
//需要提供四个基础函数用于构造、拷贝构造、析构、比较
//这些函数是可选的，若没有定义，则会使用默认的处理方式
__weak void A_OBJ_INIT(My_class)(My_class* self){
    //构造函数中无需检查self是否为null
    printf("hello, alib\n");
    self->s = "alib sample with type system";
}

__weak void A_OBJ_COPY(My_class)(My_class* self, const My_class* that){
    printf("hello, alib\n");
    self->s = that->s;
}

__weak void A_OBJ_DEST(My_class)(My_class* self){
    self->s = nullptr;
    printf("bay, alib\n");

    //析构函数不允许发生异常
}

__weak int A_OBJ_CMPD(My_class)(const My_class* self, const My_class* that){
    return strcmp(self->s, that->s);
}

//注册类
A_CLASS_REGISTER(My_class);




//子类，继承于My_class
AClass_Inherit(My_class_sub, My_class);

AClass_Struct(My_class_sub);

AClass_Function(My_class_sub);

AClass_Generate(My_class_sub);

//成员函数实例
static inline void My_class_sub_print(const My_class_sub* _self){
    My_class* self = (void*)_self;
    if(self != nullptr && self->s != nullptr){
        printf("My_class_sub:%s\n", self->s);
    }else{
        aExcSet(AEXC_nullptr);
        printf("nullptr\n");
    }
}

//覆盖父类函数
__weak void A_SET_VTAB(My_class_sub)(My_class_sub* self){
    A_COVER_FUNC(self, My_class, print, My_class_sub_print);
}

__weak void A_OBJ_INIT(My_class_sub)(__noused My_class_sub* self){
    printf("hello, alib, sub class\n");
}

__weak void A_OBJ_COPY(My_class_sub)(__noused My_class_sub* self, __noused const My_class_sub* that){
    printf("hello, alib, sub class\n");
}

__weak void A_OBJ_DEST(My_class_sub)(__noused My_class_sub* self){
    printf("bay, alib, sub class\n");
}

//注册类
A_CLASS_REGISTER(My_class_sub);

int main(){
    RAII(My_class) my_class = A_INIT(My_class);
    if(aExcOccur()){
    }

    A_CALL(my_class).print(&my_class);

    RAII(My_class) my_class_1 = A_COPY(My_class, my_class);
    if(aExcOccur()){
    }

    RAII(My_class_sub) sub_class = A_INIT(My_class_sub);
    if(aExcOccur()){
    }

    //调用父类函数
    //此函数已被重写
    A_CALL(sub_class, My_class).print((void*)&sub_class);
}

