/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib aline sample
 */

#include <alib/alib.h>
#include <alib/aline.h>
#include <stdio.h>

typedef struct{
    char* s;
}My_struct;

__unused static inline void A_OBJ_INIT(My_struct)(My_struct* self){
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
}

__unused static inline int A_OBJ_CMPD(My_struct)(const My_struct* self, const My_struct* that){
    return strcmp(self->s, that->s);
}

A_TYPE_REGISTER(My_struct);

//定义容器
ALine_Define(int);ALine_Generate(int);A_TYPE_REGISTER(ALine(int));
ALine_Define(My_struct);ALine_Generate(My_struct);A_TYPE_REGISTER(ALine(My_struct));

int main(){
    RAII(ALine(My_struct)) line = A_INIT(ALine(My_struct));
    if(aExcOccur()){
        printf("create ALine(%s) AOM err\n", __A_Str(My_struct));
        return -1;
    }

    for(int i = 0; i < 10; i++){
        RAII(My_struct) obj = A_INIT(My_struct);
        if(aExcOccur()){
            return -1;
        }

        //添加元素
        line.f->pushBack(&line, obj);
    }

    RAII(ALine(int)) line_int = A_INIT(ALine(int));
    if(aExcOccur()){
        printf("create ALine(%s) AOM err\n", __A_Str(int));
        return -1;
    }

    for(int i = 0; i < 10; i++){
        //添加元素
        line_int.f->pushBack(&line_int, i);
    }

    forEach(it, line_int){
        printf("%d ", *it.p);
    }
    printf("\n");
}

