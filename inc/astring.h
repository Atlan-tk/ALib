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

typedef int Achar;
static inline void A_OBJ_INIT(Achar)(__noused Achar* self){}
static inline void A_OBJ_DEST(Achar)(__noused Achar* self){}
static inline void A_OBJ_COPY(Achar)(Achar* self, const Achar* that){ *self = *that; }
static inline int  A_OBJ_CMPD(Achar)(const Achar* self, const Achar* that){ return A_CMPD(int, *self, *that); }
A_TYPE_REGISTER(Achar);

typedef struct{
    char* s;
    bool noLiteral;     //是否为字面量, false指向字面量
    uint32_t number;    //字符数量，不包括\0
    uint32_t capacity;
}AStr;

char AStr_at(AStr* self, uint32_t index);
void AStr_rm(AStr* self, uint32_t index);
void AStr_set(AStr* self, uint32_t index, char c);
void AStr_ins(AStr* self, uint32_t index, char c);
void AStr_pushBack(AStr* self, char c);
void AStr_pushFront(AStr* self, char c);
char AStr_popBack(AStr* self);
char AStr_popFront(AStr* self);
void AStr_addBack(AStr* self, const char* s);
void AStr_addFront(AStr* self, const char* s);
void AStr_truncate(AStr* self, uint32_t index);
int  AStr_reCap(AStr* self, uint32_t new_cap);

__noused static uint32_t AStr_getNumber(const AStr* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    return self->number;
}
__noused static uint32_t AStr_getCapacity(const AStr* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }
    return self->capacity;
}
__noused static bool AStr_empty(const AStr* self){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return true;
    }
    return self->number == 0;
}

__noused static inline void A_OBJ_INIT(AStr)(AStr* self) {
    memset(self, 0, sizeof(AStr));
}

__noused static inline void A_OBJ_DEST(AStr)(AStr* self) {
    if (self->noLiteral) {
        alib_free(self->s);
    }
}

__noused static inline void A_OBJ_COPY(AStr)(AStr* self, const AStr* that) {
    if (that->noLiteral) {
        int ret = AStr_reCap(self, that->number + 1);
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

__noused static inline int A_OBJ_CMPD(AStr)(const AStr* self, const AStr* that) {
    if(self->s == that->s) return 0;
    if(self->s != nullptr && that->s != nullptr){
        return strcmp(self->s, that->s);
    }else if(self->s == nullptr){
        return -1;
    }else{
        return 1;
    }
}

A_TYPE_REGISTER(AStr);



/* 将字面量转换为AStr */
static inline AStr AStr_new(const char* s){
    return  (AStr){
        .s = (char*)s,
        .number = s ? strlen(s) : 0,
        .noLiteral = false,
        .capacity = 0,
    };
}



/* 字符编码操作 */
/* 计算u8字符数 */
uint32_t autf8_num(const char* s);

/* 第index个u8字符位置 */
uint32_t autf8_index(const char* s, uint32_t index);

/* 字符编码转换 */
AStr autf8_foru32(const char* s);
AStr autf8_foru16(const char* s);
AStr autf8_forgbk(const char* s);
AStr autf8_tou32(const char* s);
AStr autf8_tou16(const char* s);
AStr autf8_togbk(const char* s);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
