/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __astring_h__
#define __astring_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"

typedef struct AString AString;
typedef struct A_FUNC(AString) A_FUNC(AString);

struct AString{
    const A_FUNC(AString)* f;
    bool noLiteral;     //是否为字面量, false指向字面量
    uint32_t number;    //字符数量，不包括\0
    uint32_t capacity;
    char* s;
};

struct A_FUNC(AString){
    bool flag;
    void (*dest)(void*);
    void (*const rm)(AString* self, uint32_t index);
    void (*const ins)(AString* self, uint32_t index, char c);
    void (*const pushBack)(AString* self, char c);
    void (*const pushFront)(AString* self, char c);
    char (*const popBack)(AString* self);
    char (*const popFront)(AString* self);
    /* 拼接字符串*/
    void (*const addBack)(AString* self, AString that);
    void (*const addFront)(AString* self, AString that);
    /* 截断字符串，仅保留前index个字符*/
    void  (*const truncate)(AString* self, uint32_t index);

    uint32_t (*const getNumber)(const AString* self);
    uint32_t (*const getCapacity)(const AString* self);
    bool (*const empty)(const AString* self);
};

void AString_rm(AString* self, uint32_t index);
void AString_ins(AString* self, uint32_t index, char c);
void AString_pushBack(AString* self, char c);
void AString_pushFront(AString* self, char c);
char AString_popBack(AString* self);
char AString_popFront(AString* self);
void AString_addBack(AString* self, AString that);
void AString_addFront(AString* self, AString that);
void AString_truncate(AString* self, uint32_t index);
int  AString_reCap(AString* self, uint32_t new_cap);

__noused static uint32_t AString_getNumber(const AString* self){
    return self->number;
}
__noused static uint32_t AString_getCapacity(const AString* self){
    return self->capacity;
}
__noused static bool AString_empty(const AString* self){
    return self->number == 0;
}

__noused __weak uint32_t A_OBJ_HASH(AString)(const AString* self){
    if(__a_unlikely(self == nullptr || self->s == nullptr)){
        return 0;
    }
    return alib_hash_str(self->s);
}
A_TYPE_REGISTER(AString);

static const A_FUNC(AString) A_FUNC_TAB(AString) = {
    true,
    (void*)__A_OBJ_DEST_FUNC_SELF(AString),
    AString_rm,
    AString_ins,
    AString_pushBack,
    AString_pushFront,
    AString_popBack,
    AString_popFront,
    AString_addBack,
    AString_addFront,
    AString_truncate,
    AString_getNumber,
    AString_getCapacity,
    AString_empty,
};

/* 将字面量转换为AString */
static inline AString AString_new(const char* s){
    return  (AString){
        .f = &A_FUNC_TAB(AString),
        .number = s ? strlen(s) : 0,
        .noLiteral = false,
        .capacity = 0,
        .s = (char*)s,
    };
}


__noused __weak void A_OBJ_INIT(AString)(AString* self) {
    self->f = &A_FUNC_TAB(AString);
}

__noused __weak void A_OBJ_DEST(AString)(AString* self) {
    if (self->noLiteral) {
        alib_free(self->s);
    }
}

__noused __weak void A_OBJ_COPY(AString)(AString* self, const AString* that) {
    self->f = that->f;
    if (that->noLiteral) {
        int ret = AString_reCap(self, that->number + 1);
        if(ret != 0){
            aExcSet(AEXC_init_failed);
            return;
        }
        memcpy(self->s, that->s, that->number + 1);
        self->number = that->number;
    } else {
        self->s = that->s;
        self->capacity = 0;
    }

    self->number = that->number;
    self->noLiteral = that->noLiteral;
}

__noused __weak int A_OBJ_CMPD(AString)(const AString* self, const AString* that) {
    if(self->s == that->s) return 0;
    if(self->s != nullptr && that->s != nullptr){
        return strcmp(self->s, that->s);
    }else if(self->s == nullptr){
        return -1;
    }else{
        return 1;
    }
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

