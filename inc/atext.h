/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __atext_h__
#define __atext_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"
#include "astring.h"

/* 计算字符数 */
uint32_t astrlen_u8(const char* s);
uint32_t astrlen_u16(const char* s);
uint32_t astrlen_u32(const char* s);
uint32_t astrlen_gbk(const char* s);

/* 4 字节字符，表示单个 UTF-8 字符 */
/* c[0]-c[3] 按低位到高位存储字节 */
/* 未使用的字节需设为 0x0 */
typedef struct {
    char c[4];
} Achar;
A_TYPE_REGISTER(Achar);

/* u8 字符使用的字节数 */
uint32_t Achar_used(Achar ch);
/* 将一个 u8 字符转换为 Achar */
/* 若 s 中不止一个字符则只取首字符 */
Achar Achar_new(const char* s);

/* utf8 字符串 */
typedef struct AText AText;
typedef struct A_FUNC(AText) A_FUNC(AText);
struct AText {
    const A_FUNC(AText)* f;
    bool noLiteral;      /* false 表示仅借用外部字符串 */
    uint32_t char_num;   /* UTF-8 字符数量，不包括 \0 */
    uint32_t byte_num;   /* 字节数量，不包括 \0 */
    uint32_t capacity;   /* buf 容量 */
    char* s;
};

struct A_FUNC(AText) {
    bool flag;
    void (*dest)(void*);
    /* 删除第 i 个字符 */
    void (*const rm)(AText* self, uint32_t index);
    /* 插入第 i 个字符 */
    void (*const ins)(AText* self, uint32_t index, Achar c);
    void (*const pushBack)(AText* self, Achar c);
    void (*const pushFront)(AText* self, Achar c);
    Achar (*const popBack)(AText* self);
    Achar (*const popFront)(AText* self);
    /* 拼接字符串 */
    void (*const addBack)(AText* self, AText that);
    void (*const addFront)(AText* self, AText that);
    /* 截断字符串，仅保留前 index 个字符 */
    void (*const truncate)(AText* self, uint32_t index);
    /* 获取 u8 字符数 */
    uint32_t (*const getNumber)(const AText* self);
    uint32_t (*const getCapacity)(const AText* self);
    bool (*const empty)(const AText* self);
};

void AText_rm(AText* self, uint32_t index);
void AText_ins(AText* self, uint32_t index, Achar c);
void AText_pushBack(AText* self, Achar c);
void AText_pushFront(AText* self, Achar c);
Achar AText_popBack(AText* self);
Achar AText_popFront(AText* self);
void AText_addBack(AText* self, AText that);
void AText_addFront(AText* self, AText that);
void AText_truncate(AText* self, uint32_t index);
int  AText_reCap(AText* self, uint32_t new_cap);

static inline uint32_t AText_getNumber(const AText* self) {
    return self->char_num;
}

__noused static inline uint32_t AText_getCapacity(const AText* self) {
    return self->capacity;
}

__noused static inline bool AText_empty(const AText* self) {
    return self->char_num == 0;
}

A_TYPE_REGISTER(AText);

static const A_FUNC(AText) A_FUNC_TAB(AText) = {
    true,
    (void*)__A_OBJ_DEST_FUNC_SELF(AText),
    AText_rm,
    AText_ins,
    AText_pushBack,
    AText_pushFront,
    AText_popBack,
    AText_popFront,
    AText_addBack,
    AText_addFront,
    AText_truncate,
    AText_getNumber,
    AText_getCapacity,
    AText_empty,
};

/* 将字面量转换为 AText，必须是 UTF-8 字符串 */
static inline AText AText_new(const char* s) {
    return (AText){
        .f = &A_FUNC_TAB(AText),
        .byte_num = s ? (uint32_t)strlen(s) : 0,
        .char_num = s ? astrlen_u8(s) : 0,
        .noLiteral = false,
        .capacity = 0,
        .s = (char*)s,
    };
}

/* 字符编码转换，输入必须为对应编码的字符串 */
AText AText_forU32(char* s);
AText AText_forU16(char* s);
AText AText_forGBK(char* s);

/* 字符编码转换 */
void AText_toU32(const AText* self, char* buf, uint32_t buf_size);
void AText_toU16(const AText* self, char* buf, uint32_t buf_size);
void AText_toGBK(const AText* self, char* buf, uint32_t buf_size);



__noused __weak void A_OBJ_INIT(AText)(AText* self) {
    self->f = &A_FUNC_TAB(AText);
}

__noused __weak void A_OBJ_DEST(AText)(AText* self) {
    if (self->noLiteral) {
        alib_free(self->s);
    }
}

__noused __weak void A_OBJ_COPY(AText)(AText* self, const AText* that) {
    self->f = that->f;
    if (that->noLiteral) {
        int ret = AText_reCap(self, that->byte_num + 1);
        if (ret != 0) {
            aExcSet(AEXC_init_failed);
            return;
        }
        if (that->byte_num != 0) {
            memcpy(self->s, that->s, that->byte_num);
        }
        self->s[that->byte_num] = '\0';
    } else {
        self->s = that->s;
        self->capacity = 0;
    }

    self->byte_num = that->byte_num;
    self->char_num = that->char_num;
    self->noLiteral = that->noLiteral;
}

__noused __weak int A_OBJ_CMPD(AText)(const AText* self, const AText* that) {
    if (self->s == that->s) {
        return 0;
    }
    if (self->s != nullptr && that->s != nullptr) {
        return strcmp(self->s, that->s);
    }
    if (self->s == nullptr) {
        return -1;
    }
    return 1;
}

__noused __weak uint32_t A_OBJ_HASH(AText)(const AText* self) {
    if (__a_unlikely(self == nullptr || self->s == nullptr)) {
        return 0;
    }
    return alib_hash_str(self->s);
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __atext_h__ */
