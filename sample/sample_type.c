/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib type system sample
 */

#include <alib/alib.h>
#include <stdio.h>

//使用alib 类型系统需要先注册类型
//定义类型
typedef struct{
    char* s;
}My_struct;

//为My_struct提供基础函数
//需要提供四个基础函数用于构造、拷贝构造、析构、比较
//这些函数是可选的，若没有定义，则会使用默认的处理方式
__unused static inline void A_OBJ_INIT(My_struct)(My_struct* self){
    //构造函数中无需检查self是否为null
    printf("hello, alib\n");
    self->s = "alib sample with type system";
}

__unused static inline void A_OBJ_COPY(My_struct)(My_struct* self, const My_struct* that){
    printf("hello, alib\n");
    self->s = that->s;
}

__unused static inline void A_OBJ_DEST(My_struct)(My_struct* self){
    self->s = nullptr;
    printf("bay, alib\n");

    //析构函数不允许发生异常
}

__unused static inline int A_OBJ_CMPD(My_struct)(const My_struct* self, const My_struct* that){
    return strcmp(self->s, that->s);
}

//注册类型
A_TYPE_REGISTER(My_struct);

//int等基础类型已在库中注册过，无需注册
//A_TYPE_REGISTER(int);
//A_TYPE_REGISTER(double);

int main(){
    RAII(My_struct) my_struct = A_INIT(My_struct);
    if(aExcOccur()){
    }

    RAII(My_struct) my_struct_1 = A_COPY(My_struct, my_struct);
    if(aExcOccur()){
    }

    printf("%s\n", my_struct.s);
}

