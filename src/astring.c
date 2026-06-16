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

    uint32_t new_cap = self->length + 1;  /* 至少容纳字符和'\0' */
    new_cap = AStr_calCap(new_cap);

    char* new_s = alib_alloc(new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AERR_alloc_failed;
    }

    if(__a_likely(self->s != nullptr)){
        memcpy(new_s, self->s, self->length);
    }
    new_s[self->length] = '\0';

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
    if (__a_likely(self->length + 1 >= (self->capacity + 1) / 2)) return 0;

    uint32_t new_cap = self->length + 1;
    new_cap = AStr_calCap(new_cap);

    char * new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AERR_alloc_failed;
    }
    self->s = new_s, self->capacity = new_cap;
    return 0;
}



char AStr_at(const AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    if(__a_unlikely(self->length == 0)){
        return 0;
    }

    if (__a_unlikely(index >= self->length)) {
        index = self->length - 1;
    }

    char ch = self->s[index];
    return ch;
}

void AStr_set(AStr* self, uint32_t index, char ch) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(self->length == 0)){
        aErrSet(AERR_overstep);
        return;
    }

    if (__a_unlikely(index >= self->length)) {
        index = self->length - 1;
    }

    self->s[index] = ch;
}

void AStr_rm(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(index >= self->length)){
        return;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memmove(self->s + index, self->s + index + 1, self->length - index);
    self->length--;
    AStr_sub(self);
}

void AStr_ins(AStr* self, uint32_t index, char c) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(index > self->length)){
        index = self->length;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    ret = AStr_grow(self, self->length + 2);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    if(__a_unlikely(self->length == 0)){
        self->s[1] = '\0';
    }else{
        memmove(self->s + index + 1, self->s + index, self->length - index + 1);
    }
    self->s[index] = c, self->length++;
}

void AStr_pushBack(AStr* self, char c) {
    AStr_ins(self, self->length, c);
}

void AStr_pushFront(AStr* self, char c) {
    AStr_ins(self, 0, c);
}

char AStr_popBack(AStr* self) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    if (__a_unlikely(self->length == 0)) {
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aErrSet(AERR_alloc_failed);
        return '\0';
    }

    char c = self->s[self->length - 1];
    self->s[self->length - 1] = '\0';
    self->length--;
    AStr_sub(self);

    return c;
}

char AStr_popFront(AStr* self) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }

    if (__a_unlikely(self->length == 0)) {
        return '\0';
    }

    if(0 != AStr_ensure_writable(self)){
        aErrSet(AERR_alloc_failed);
        return '\0';
    }

    char c = self->s[0];
    memmove(self->s, self->s + 1, self->length);
    self->length--;
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

    ret = AStr_grow(self, self->length + len + 1);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memcpy(self->s + self->length, s, len + 1);
    self->length += len;
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

    ret = AStr_grow(self, self->length + len + 1);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memmove(self->s + len, self->s, self->length + 1);
    memcpy(self->s, s, len);
    self->length += len;
}

void AStr_truncate(AStr* self, uint32_t index) {
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(index >= self->length)){
        return;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    self->length = index, self->s[index] = '\0';
    AStr_sub(self);
}

int AStr_reCap(AStr* self, uint32_t new_cap){
    if(__a_unlikely(new_cap < self->length + 1)){
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

void AStr_insStr(AStr* self, uint32_t index, const char* s){
    if(__a_unlikely(self == nullptr || s == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(index > self->length)){
        index = self->length;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }
    int n = strlen(s);

    ret = AStr_grow(self, self->length + n + 1);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    if(__a_unlikely(self->length != 0)){
        memmove(self->s + index + n, self->s + index, self->length + 1 - index);
    }
    memcpy(self->s + index, s, n);
    self->length += n; self->s[self->length] = 0;
}

void AStr_rmStr(AStr* self, uint32_t index, uint32_t n){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    if(__a_unlikely(index >= self->length)){
        return;
    }

    if(__a_unlikely(index + n > self->length)){
        n = self->length - index;
    }

    int ret = AStr_ensure_writable(self);
    if(ret != 0){
        aErrSet(AERR_alloc_failed);
        return;
    }

    memmove(self->s + index, self->s + index + n, self->length + 1 - (index + n));
    self->length -= n; self->s[self->length] = '\0';
    AStr_sub(self);
}

static uint32_t autf8_len(const char* s);
static uint32_t AStr_u8at_i(const AStr* self, uint32_t index){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }
    if(__a_unlikely(self->s == nullptr)){
        return 0;
    }
    char* s = self->s; uint32_t x = 0;
    for(uint32_t n = 0; n < index && x < self->length; n++){
        aTry(x += autf8_len(s + x);)aExc{
            return 0;
        }
    }
    s += x;

    return x;
}
Achar AStr_u8at(const AStr* self, uint32_t index){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }
    if(__a_unlikely(self->s == nullptr)){
        return 0;
    }
    aTry(uint32_t i = AStr_u8at_i(self, index);)aExc{
        return 0;
    }
    return autf8char(self->s + i);
}
void AStr_u8rm(AStr* self, uint32_t index){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }
    char* s = self->s; uint32_t x = 0;
    for(uint32_t n = 0; n < index && x < self->length; n++){
        aTry(x += autf8_len(s + x);)aExc{
            return;
        }
    }
    s += x;

    AStr_rmStr(self, x, autf8_len(s));
}
static void AStr_u8ins_ch(AStr* self, uint32_t index, Achar ch){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }

    char* p = (void*)&ch;
    if(ch == 0){
        AStr_truncate(self, index);
        return;
    }
    for(uint32_t i = 0; i < sizeof(Achar) && p[i] != '\0'; i++){
        AStr_ins(self, index + i, p[i]);
    }
}
void AStr_u8ins(AStr* self, uint32_t index, Achar ch){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return;
    }
    if(__a_unlikely(AStr_ensure_writable(self) != 0)){
        aErrSet(AERR_alloc_failed);
        return;
    }
    char* s = self->s; uint32_t x = 0;
    for(uint32_t n = 0; n < index && x < self->length; n++){
        aTry(x += autf8_len(s + x);)aExc{
            return;
        }
    }
    s += x;

    AStr_u8ins_ch(self, x, ch);
}
void AStr_u8pushFront(AStr* self, Achar ch){
    AStr_u8ins(self, 0, ch);
}
void AStr_u8pushBack(AStr* self, Achar ch){
    AStr_u8ins(self, AEND, ch);
}
Achar AStr_u8popFront(AStr* self){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }
    if(__a_unlikely(self->length == 0)){
        return 0;
    }
    if(__a_unlikely(AStr_ensure_writable(self) != 0)){
        aErrSet(AERR_alloc_failed);
        return 0;
    }
    uint32_t i = 0;
    char* s = self->s + i;
    uint32_t len = autf8_len(s);
    Achar ret = autf8char(s);
    AStr_rmStr(self, i, len);
    return ret;
}
Achar AStr_u8popBack(AStr* self){
    if(__a_unlikely(self == nullptr)){
        aErrSet(AERR_nullptr);
        return 0;
    }
    if(__a_unlikely(self->length == 0)){
        return 0;
    }
    if(__a_unlikely(AStr_ensure_writable(self) != 0)){
        aErrSet(AERR_alloc_failed);
        return 0;
    }
    aTry(uint32_t n = autf8_num(self->s);)aExc{
        return 0;
    }
    if(__a_unlikely(n == 0)){
        return 0;
    }
    aTry(uint32_t i = AStr_u8at_i(self, n - 1);)aExc{
        return 0;
    }
    char* s = self->s + i;
    uint32_t len = autf8_len(s);
    Achar ret = autf8char(s);
    AStr_rmStr(self, i, len);
    return ret;
}



/* 字符编码操作 */
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

    uint32_t len = strlen(s);
    if(len == 0) return 0;

    unsigned char c = (unsigned char)*s;
    uint32_t n = 0;
    if (c <= 0x7F){
        n = 1;                // 0xxxxxxx
    }else if (c >= 0xC0 && c <= 0xDF){
        n = 2;   // 110xxxxx
    }else if (c >= 0xE0 && c <= 0xEF){
        n = 3;   // 1110xxxx
    }else if (c >= 0xF0 && c <= 0xF7){
        n = 4;   // 11110xxx
    }else{
        aErrSet(AERR_outdomain);
        return 0;
    }

    if(len < n){
        aErrSet(AERR_outdomain);
        return 0;
    }

    return n;
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
    char buf[256];

    iconv_t fd = iconv_open(tar_name, src_name);
    if(__a_unlikely(fd == (iconv_t)-1)){
        aErrSet(AERR_system_error);
        return A_INIT(AStr);
    }

    char* src = (char*)s; char* tar = buf;
    while((size_t)(s + len) > (size_t)src){
        memset(buf, 0, sizeof(buf)); tar = buf;
        size_t len_src = (size_t)(s + len) - (size_t)src;
        size_t len_tar = sizeof(buf);

        errno = 0;
        size_t ret = iconv(fd, &src, &len_src, &tar, &len_tar);
        if(ret == (size_t)-1 && errno != E2BIG){
            iconv_close(fd);
            aErrSet(AERR_system_error);
            return A_INIT(AStr);
        }
        aTry( AStr_push_s(&str, buf, sizeof(buf) - len_tar); )aExc{
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

/* u8字符转Achar */
Achar autf8char(const char* s){
    Achar ch = 0; char* p = (void*)&ch;
    uint32_t len = autf8_len(s);
    for(uint32_t i = 0; i < len; i++){
        p[i] = s[i];
    }
    return ch;
}


