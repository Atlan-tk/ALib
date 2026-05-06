/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <astring.h>

static inline uint32_t AString_calCap(uint32_t cap){
    if(__a_unlikely(cap >= __aPagSize)){
        cap = (cap / __aPagSize + (cap % __aPagSize ? 1 : 0)) * __aPagSize;
    }else{
        uint32_t x; for(x = 8; x < cap; x *= 2);
        cap = x;
    }
    return cap;
}

/* 字面量转为堆分配 */
static inline int AString_ensure_writable(AString* self) {
    if(__a_likely(self->noLiteral)) return 0;

    uint32_t new_cap = self->number + 1;  /* 至少容纳字符和'\0' */
    new_cap = AString_calCap(new_cap);


    char* new_s = alib_alloc(new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }

    if(__a_likely(self->s != nullptr)){
        memcpy(new_s, self->s, self->number + 1);
    }

    self->s = new_s;
    self->noLiteral = true;
    self->capacity = new_cap;
    return 0;
}

/* 扩展容量（假设已可写） */
static inline int AString_grow(AString* self, uint32_t new_cap) {
    if (__a_likely(new_cap <= self->capacity)) return 0;

    new_cap = AString_calCap(new_cap);

    char* new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}

/* 收缩容量（假设已可写） */
static int AString_sub(AString* self) {
    if (__a_likely(self->number + 1 >= (self->capacity + 1) / 2)) return 0;

    uint32_t new_cap = self->number + 1;
    new_cap = AString_calCap(new_cap);

    char * new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}


/* ---------- 虚函数实现 ---------- */

void AString_rm(AString* self, uint32_t index) {
    if (__a_unlikely(index >= self->number)) {
        return;
    }

    int ret = AString_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    memmove(self->s + index, self->s + index + 1, self->number - index);
    self->number--;
    AString_sub(self);
}

void AString_ins(AString* self, uint32_t index, char c) {
    if (__a_unlikely(index > self->number)) {
        aExcSet(AEXC_overstep);
        return;
    }

    int ret = AString_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    ret = AString_grow(self, self->number + 2);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    if(__a_unlikely(self->number == 0)){
        self->s[1] = '\0';
    }else{
        memmove(self->s + index + 1, self->s + index, self->number - index + 1);
    }
    self->s[index] = c, self->number++;
}

void AString_pushBack(AString* self, char c) {
    AString_ins(self, self->number, c);
}

void AString_pushFront(AString* self, char c) {
    AString_ins(self, 0, c);
}

char AString_popBack(AString* self) {
    if (__a_unlikely(self->number == 0)) {
        return '\0';
    }

    if(0 != AString_ensure_writable(self)) return '\0';

    char c = self->s[self->number - 1];
    self->s[self->number - 1] = '\0';
    self->number--;
    AString_sub(self);

    return c;
}

char AString_popFront(AString* self) {
    if (__a_unlikely(self->number == 0)) {
        return '\0';
    }

    if(0 != AString_ensure_writable(self)) return '\0';

    char c = self->s[0];
    memmove(self->s, self->s + 1, self->number);
    self->number--;
    AString_sub(self);

    return c;
}

void AString_addBack(AString* self, AString that) {
    bool flag = false;
    if(self->s == that.s){
        that = A_COPY(AString, that);
        flag = true;
    }

    if(__a_unlikely(that.number == 0)){
        if(flag) A_DEST(AString, that);
        return;
    }

    int ret = AString_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        if(flag) A_DEST(AString, that);
        return;
    }

    ret = AString_grow(self, self->number + that.number + 1);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        if(flag) A_DEST(AString, that);
        return;
    }

    memcpy(self->s + self->number, that.s, that.number + 1);
    self->number += that.number;

    if(flag) A_DEST(AString, that);
}

void AString_addFront(AString* self, AString that) {
    bool flag = false;
    if(self->s == that.s){
        that = A_COPY(AString, that);
        flag = true;
    }

    if(__a_unlikely(that.number == 0)){
        if(flag) A_DEST(AString, that);
        return;
    }

    int ret = AString_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        if(flag) A_DEST(AString, that);
        return;
    }

    ret = AString_grow(self, self->number + that.number + 1);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        if(flag) A_DEST(AString, that);
        return;
    }

    memmove(self->s + that.number, self->s, self->number + 1);
    memcpy(self->s, that.s, that.number);
    self->number += that.number;

    if(flag) A_DEST(AString, that);
}

void AString_truncate(AString* self, uint32_t index) {
    if(__a_unlikely(index >= self->number)){
        return;
    }

    if (__a_unlikely(self->number == 0)) {
        aExcSet(AEXC_overstep);
        return;
    }

    int ret = AString_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    self->number = index, self->s[index] = '\0';
    AString_sub(self);
}

static inline int AString_reCap(AString* self, uint32_t new_cap){
    if(__a_unlikely(new_cap < self->number + 1)){
        return AEXC_overstep;
    }

    int ret = 0;
    if(new_cap > self->capacity){
        ret = AString_grow(self, new_cap);
    }else{
        ret = AString_sub(self);
    }

    return ret;
}

/* ---------- 基础函数 ---------- */
void A_OBJ_INIT(AString)(AString* self) {
    self->f = &A_FUNC_TAB(AString);
}

void A_OBJ_DEST(AString)(AString* self) {
    if (self->noLiteral) {
        alib_free(self->s);
    }
}

void A_OBJ_COPY(AString)(AString* self, const AString* that) {
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

int A_OBJ_CMPD(AString)(const AString* self, const AString* that) {
    if(self->s == that->s) return 0;
    if(self->s != nullptr && that->s != nullptr){
        return strcmp(self->s, that->s);
    }else if(self->s == nullptr){
        return -1;
    }else{
        return 1;
    }
}

uint32_t A_OBJ_HASH(AString)(const AString* self){
    if(__a_unlikely(self == nullptr || self->s == nullptr)){
        return 0;
    }
    return alib_hash_str(self->s);
}



