/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <astring.h>

static inline uint32_t AStr_calCap(uint32_t cap){
    if(__a_unlikely(cap >= __aPagSize)){
        cap = (cap / __aPagSize + (cap % __aPagSize ? 1 : 0)) * __aPagSize;
    }else{
        uint32_t x; for(x = 8; x < cap; x *= 2);
        cap = x;
    }
    return cap;
}

/* 字面量转为堆分配 */
static inline int AStr_ensure_writable(AStr* self) {
    if(__a_likely(self->noLiteral)) return 0;

    uint32_t new_cap = self->number + 1;  /* 至少容纳字符和'\0' */
    new_cap = AStr_calCap(new_cap);

    char* new_s = alib_alloc(new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }

    if(__a_likely(self->s != nullptr)){
        memcpy(new_s, self->s, self->number);
    }
    new_s[self->number] = '\0';

    self->s = new_s;
    self->noLiteral = true;
    self->capacity = new_cap;
    return 0;
}

/* 扩展容量（假设已可写） */
static inline int AStr_grow(AStr* self, uint32_t new_cap) {
    if (__a_likely(new_cap <= self->capacity)) return 0;

    new_cap = AStr_calCap(new_cap);

    char* new_s = self->s == nullptr ? alib_alloc(new_cap) : alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}

/* 收缩容量（假设已可写） */
static int AStr_sub(AStr* self) {
    if (__a_likely(self->number + 1 >= (self->capacity + 1) / 2)) return 0;

    uint32_t new_cap = self->number + 1;
    new_cap = AStr_calCap(new_cap);

    char * new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}


/* ---------- 虚函数实现 ---------- */
char AStr_at(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if(__a_unlikely(self->number == 0)){
        aExcSet(AEXC_overstep);
        return 0;
    }

    if (__a_unlikely(index >= self->number)) {
        index = self->number - 1;
    }

    char ch = self->s[index];
    return ch;
}

void AStr_set(AStr* self, uint32_t index, char ch) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(self->number == 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    if (__a_unlikely(index >= self->number)) {
        index = self->number - 1;
    }

    self->s[index] = ch;
}

void AStr_rm(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if(__a_unlikely(self->number == 0)){
        aExcSet(AEXC_overstep);
        return;
    }

    if (__a_unlikely(index >= self->number)) {
        index = self->number - 1;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    memmove(self->s + index, self->s + index + 1, self->number - index);
    self->number--;
    AStr_sub(self);
}

void AStr_ins(AStr* self, uint32_t index, char c) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if (__a_unlikely(index > self->number)) {
        index = self->number;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + 2);
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

void AStr_pushBack(AStr* self, char c) {
    AStr_ins(self, self->number, c);
}

void AStr_pushFront(AStr* self, char c) {
    AStr_ins(self, 0, c);
}

char AStr_popBack(AStr* self) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if (__a_unlikely(self->number == 0)) {
        aExcSet(AEXC_overstep);
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aExcSet(AEXC_alloc_failed);
        return '\0';
    }

    char c = self->s[self->number - 1];
    self->s[self->number - 1] = '\0';
    self->number--;
    AStr_sub(self);

    return c;
}

char AStr_popFront(AStr* self) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return 0;
    }

    if (__a_unlikely(self->number == 0)) {
        aExcSet(AEXC_overstep);
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aExcSet(AEXC_alloc_failed);
        return '\0';
    }

    char c = self->s[0];
    memmove(self->s, self->s + 1, self->number);
    self->number--;
    AStr_sub(self);

    return c;
}

void AStr_addBack(AStr* self, const char* s) {
    if(__a_unlikely(self == nullptr || s == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    uint32_t len = strlen(s);
    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + len + 1);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    memcpy(self->s + self->number, s, len + 1);
    self->number += len;
}

void AStr_addFront(AStr* self, const char* s) {
    if(__a_unlikely(self == nullptr || s == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    uint32_t len = strlen(s);
    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + len + 1);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    memmove(self->s + len, self->s, self->number + 1);
    memcpy(self->s, s, len);
    self->number += len;
}

void AStr_truncate(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }

    if (__a_unlikely(self->number == 0)) {
        aExcSet(AEXC_overstep);
        return;
    }

    if(__a_unlikely(index >= self->number)){
        return;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aExcSet(AEXC_alloc_failed);
        return;
    }

    self->number = index, self->s[index] = '\0';
    AStr_sub(self);
}

int AStr_reCap(AStr* self, uint32_t new_cap){
    if(__a_unlikely(new_cap < self->number + 1)){
        return AEXC_overstep;
    }

    int ret = 0;
    if(new_cap > self->capacity){
        ret = AStr_grow(self, new_cap);
    }else{
        ret = AStr_sub(self);
    }

    return ret;
}

uint32_t A_OBJ_HASH(AStr)(const AStr* self){
    if(__a_unlikely(self == nullptr || self->s == nullptr)){
        return 0;
    }
    return alib_hash_str(self->s);
}



/* 字符编码操作 */
/* pushback */
static inline void AStr_pushAchar(AStr* self, Achar ch){
    if(__a_unlikely(self == nullptr)){
        aExcSet(AEXC_nullptr);
        return;
    }
    if(__a_unlikely(ch == 0)){
        return;
    }
    char* p = &ch;
    if(*p != 0){ AStr_pushBack(self, *p); p++; }
    if(*p != 0){ AStr_pushBack(self, *p); p++; }
    if(*p != 0){ AStr_pushBack(self, *p); p++; }
    if(*p != 0){ AStr_pushBack(self, *p); p++; }
}

/* 计算u8字符数 */
uint32_t autf8_num(const char* s){
}

/* 第index个u8字符位置 */
uint32_t autf8_index(const char* s, uint32_t index){
}

/* 字符编码转换 */
AStr autf8_foru32(char* s){
}

AStr autf8_foru16(char* s){
}

AStr autf8_forgbk(char* s){
}

/* 字符编码转换 */
AStr autf8_tou32(const char* s){
}

AStr autf8_tou16(const char* s){
}

AStr autf8_togbk(const char* s){
}



