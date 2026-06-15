/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <astring.h>
#include <iconv.h>
#include <errno.h>

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
        return AERR_alloc_failed;
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
        return AERR_alloc_failed;
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
        return AERR_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}


/* ---------- 虚函数实现 ---------- */
char AStr_at(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    if(__a_unlikely(self->number == 0)){
        aErrSet(AERR_overstep);
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
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(self->number == 0)){
        aErrSet(AERR_overstep);
        return;
    }

    if (__a_unlikely(index >= self->number)) {
        index = self->number - 1;
    }

    self->s[index] = ch;
}

void AStr_rm(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(self->number == 0)){
        aErrSet(AERR_overstep);
        return;
    }

    if (__a_unlikely(index >= self->number)) {
        index = self->number - 1;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memmove(self->s + index, self->s + index + 1, self->number - index);
    self->number--;
    AStr_sub(self);
}

void AStr_ins(AStr* self, uint32_t index, char c) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if (__a_unlikely(index > self->number)) {
        index = self->number;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + 2);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
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
        aErrSet(AERR_nullptr);
        return 0;
    }

    if (__a_unlikely(self->number == 0)) {
        aErrSet(AERR_overstep);
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aErrSet(AERR_alloc_failed);
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
        aErrSet(AERR_nullptr);
        return 0;
    }

    if (__a_unlikely(self->number == 0)) {
        aErrSet(AERR_overstep);
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aErrSet(AERR_alloc_failed);
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
        aErrSet(AERR_nullptr);
        return;
    }

    uint32_t len = strlen(s);
    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + len + 1);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memcpy(self->s + self->number, s, len + 1);
    self->number += len;
}

void AStr_addFront(AStr* self, const char* s) {
    if(__a_unlikely(self == nullptr || s == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    uint32_t len = strlen(s);
    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->number + len + 1);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memmove(self->s + len, self->s, self->number + 1);
    memcpy(self->s, s, len);
    self->number += len;
}

void AStr_truncate(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if (__a_unlikely(self->number == 0)) {
        aErrSet(AERR_overstep);
        return;
    }

    if(__a_unlikely(index >= self->number)){
        return;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    self->number = index, self->s[index] = '\0';
    AStr_sub(self);
}

int AStr_reCap(AStr* self, uint32_t new_cap){
    if(__a_unlikely(new_cap < self->number + 1)){
        return AERR_overstep;
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
        aErrSet(AERR_nullptr);
        return;
    }
    if(__a_unlikely(ch == 0)){
        return;
    }
    size_t i = 0;
    char* p = (char*)&ch;
    for( ; i < sizeof(Achar); i++){
        if(p[i] != 0) break;
    }

    for( ; i < sizeof(Achar); i++){
        AStr_pushBack(self, p[i]);
    }
}
static inline void AStr_push_s(AStr* self, const char* s, uint32_t len){
    if(__a_unlikely(self == nullptr || s == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }
    for(uint32_t i = 0; i < len; i++){
        aTry(AStr_pushBack(self, s[i]);)aExc{
            return;
        }
    }
}

/* 计算u8字符字节数 */
static inline uint32_t autf8_len(const char* s){
    if(__a_unlikely(s == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    unsigned char c = (unsigned char)*s;
    if (c <= 0x7F) return 1;                // 0xxxxxxx
    if (c >= 0xC0 && c <= 0xDF) return 2;   // 110xxxxx
    if (c >= 0xE0 && c <= 0xEF) return 3;   // 1110xxxx
    if (c >= 0xF0 && c <= 0xF7) return 4;   // 11110xxx

    aErrSet(AERR_outdomain);
    return 0;
}

/* 计算u8字符数 */
uint32_t autf8_num(const char* s){

    if(__a_unlikely(s == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    uint32_t n = 0;
    uint32_t len = 0;
    const char* p = s;
    while((size_t)p < (size_t)(s + strlen(s))){
        aTry(len += autf8_len(p); )aExc{
            return n;
        }
        p = s + len; n++;
    }
    return n;
}

/* 第index个u8字符位置 */
uint32_t autf8_index(const char* s, uint32_t index){
    if(__a_unlikely(s == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    uint32_t n = 0;
    uint32_t len = 0;
    const char* p = s;
    while(n < index){
        if(!((size_t)p < (size_t)(s + strlen(s)))){
            aErrSet(AERR_outdomain);
            return 0;
        }
        aTry( len += autf8_len(p); )aExc{
            return 0;
        }
        p = s + len; n++;
    }
    return len;
}

/* 字符编码转换 */
static AStr a_iconv(const char* s, const char* tar_name, const char* src_name, uint32_t len){
    RAII(AStr) str = A_INIT(AStr);
    if(__a_unlikely(s == nullptr)){
        aErrSet(AERR_nullptr);
        return A_INIT(AStr);
    }
    Achar buf[64];

    iconv_t fd = iconv_open(tar_name, src_name);
    if(__a_unlikely(fd == (iconv_t)-1)){
        aErrSet(AERR_system_error);
        return A_INIT(AStr);
    }

    char* src = (char*)s; char* tar = (void*)buf;
    while((size_t)(s + len) > (size_t)src){
        memset(buf, 0, sizeof(buf)); tar = (void*)buf;
        size_t len_src = (size_t)(s + len) - (size_t)src;
        size_t len_tar = sizeof(Achar) * 64;

        errno = 0;
        size_t ret = iconv(fd, &src, &len_src, &tar, &len_tar);
        if(ret == (size_t)-1 && errno != E2BIG){
            iconv_close(fd);
            aErrSet(AERR_system_error);
            return A_INIT(AStr);
        }
        aTry( AStr_push_s(&str, (void*)buf, sizeof(Achar) * 64 - len_tar); )aExc{
            iconv_close(fd);
            return A_INIT(AStr);
        }
    }
    iconv_close(fd);
    return A_MOVE(str);
}

AStr autf8_foru32(const char* s, uint32_t len){
    return a_iconv(s, "UTF8", "UTF32", len);
}
AStr autf8_foru16(const char* s, uint32_t len){
    return a_iconv(s, "UTF8", "UTF16", len);
}
AStr autf8_forgbk(const char* s, uint32_t len){
    return a_iconv(s, "UTF8", "GBK", len);
}
AStr autf8_tou32(const char* s){
    return a_iconv(s, "UTF32", "UTF8", strlen(s));
}
AStr autf8_tou16(const char* s){
    return a_iconv(s, "UTF16", "UTF8", strlen(s));
}
AStr autf8_togbk(const char* s){
    return a_iconv(s, "GBK", "UTF8", strlen(s));
}



