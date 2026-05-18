/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#include <atext.h>

#include <errno.h>

#if defined(__has_include)
    #if __has_include(<iconv.h>)
        #include <iconv.h>
        #define __A_HAVE_ICONV 1
    #else
        #define __A_HAVE_ICONV 0
    #endif
#else
    #if defined(__C_POSIX__)
        #include <iconv.h>
        #define __A_HAVE_ICONV 1
    #else
        #define __A_HAVE_ICONV 0
    #endif
#endif

static inline uint32_t AText_calCap(uint32_t cap) {
    if (__a_unlikely(cap >= __aPagSize)) {
        cap = (cap / __aPagSize + (cap % __aPagSize ? 1U : 0U)) * __aPagSize;
    } else {
        uint32_t x;
        for (x = 8; x < cap; x *= 2) {}
        cap = x;
    }
    return cap;
}

static inline AText AText_empty_value(void) {
    return (AText){ .f = &A_FUNC_TAB(AText) };
}

static inline uint32_t utf8_seq_len(uint8_t lead) {
    if (lead < 0x80U) {
        return 1;
    }
    if (lead >= 0xC2U && lead <= 0xDFU) {
        return 2;
    }
    if (lead >= 0xE0U && lead <= 0xEFU) {
        return 3;
    }
    if (lead >= 0xF0U && lead <= 0xF4U) {
        return 4;
    }
    return 0;
}

static inline bool utf8_decode_strict(const char* s, uint32_t* used, uint32_t* cp) {
    uint8_t b0 = (uint8_t)s[0];
    if (b0 == 0) {
        *used = 0;
        *cp = 0;
        return true;
    }

    if (b0 < 0x80U) {
        *used = 1;
        *cp = b0;
        return true;
    }

    uint32_t len = utf8_seq_len(b0);
    if (len == 0) {
        return false;
    }

    uint8_t b1 = 0;
    uint8_t b2 = 0;
    uint8_t b3 = 0;

    if (len >= 2) {
        b1 = (uint8_t)s[1];
        if (b1 == 0 || (b1 & 0xC0U) != 0x80U) {
            return false;
        }
    }
    if (len >= 3) {
        b2 = (uint8_t)s[2];
        if (b2 == 0 || (b2 & 0xC0U) != 0x80U) {
            return false;
        }
    }
    if (len >= 4) {
        b3 = (uint8_t)s[3];
        if (b3 == 0 || (b3 & 0xC0U) != 0x80U) {
            return false;
        }
    }

    if (len == 2) {
        *cp = ((uint32_t)(b0 & 0x1FU) << 6) |
              (uint32_t)(b1 & 0x3FU);
    } else if (len == 3) {
        if ((b0 == 0xE0U && b1 < 0xA0U) || (b0 == 0xEDU && b1 >= 0xA0U)) {
            return false;
        }
        *cp = ((uint32_t)(b0 & 0x0FU) << 12) |
              ((uint32_t)(b1 & 0x3FU) << 6) |
              (uint32_t)(b2 & 0x3FU);
    } else {
        if ((b0 == 0xF0U && b1 < 0x90U) || (b0 == 0xF4U && b1 >= 0x90U)) {
            return false;
        }
        *cp = ((uint32_t)(b0 & 0x07U) << 18) |
              ((uint32_t)(b1 & 0x3FU) << 12) |
              ((uint32_t)(b2 & 0x3FU) << 6) |
              (uint32_t)(b3 & 0x3FU);
    }

    *used = len;
    return true;
}

static inline uint32_t utf8_next_len_lossy(const char* s) {
    uint32_t used = 0;
    uint32_t cp = 0;
    if (utf8_decode_strict(s, &used, &cp)) {
        (void)cp;
        return used;
    }
    return 1;
}

static inline uint32_t utf8_size_from_cp(uint32_t cp) {
    if (cp <= 0x7FU) {
        return 1;
    }
    if (cp <= 0x7FFU) {
        return 2;
    }
    if (cp >= 0xD800U && cp <= 0xDFFFU) {
        return 0;
    }
    if (cp <= 0xFFFFU) {
        return 3;
    }
    if (cp <= 0x10FFFFU) {
        return 4;
    }
    return 0;
}

static inline bool utf8_encode(uint32_t cp, char out[4], uint32_t* used) {
    uint32_t len = utf8_size_from_cp(cp);
    if (len == 0) {
        return false;
    }

    if (len == 1) {
        out[0] = (char)cp;
    } else if (len == 2) {
        out[0] = (char)(0xC0U | (cp >> 6));
        out[1] = (char)(0x80U | (cp & 0x3FU));
    } else if (len == 3) {
        out[0] = (char)(0xE0U | (cp >> 12));
        out[1] = (char)(0x80U | ((cp >> 6) & 0x3FU));
        out[2] = (char)(0x80U | (cp & 0x3FU));
    } else {
        out[0] = (char)(0xF0U | (cp >> 18));
        out[1] = (char)(0x80U | ((cp >> 12) & 0x3FU));
        out[2] = (char)(0x80U | ((cp >> 6) & 0x3FU));
        out[3] = (char)(0x80U | (cp & 0x3FU));
    }

    *used = len;
    return true;
}

static inline uint32_t u16_read(const char* s, uint32_t offset) {
    uint16_t v = 0;
    memcpy(&v, s + offset, sizeof(v));
    return v;
}

static inline uint32_t u32_read(const char* s, uint32_t offset) {
    uint32_t v = 0;
    memcpy(&v, s + offset, sizeof(v));
    return v;
}

static inline void u16_write(char* buf, uint32_t offset, uint16_t v) {
    memcpy(buf + offset, &v, sizeof(v));
}

static inline void u32_write(char* buf, uint32_t offset, uint32_t v) {
    memcpy(buf + offset, &v, sizeof(v));
}

static inline bool gbk_is_trail(uint8_t byte) {
    return (byte >= 0x40U && byte <= 0xFEU && byte != 0x7FU);
}

static inline Achar Achar_from_span(const char* s, uint32_t used) {
    Achar ch = { { 0, 0, 0, 0 } };
    if (used > 4) {
        used = 4;
    }
    if (used != 0) {
        memcpy(ch.c, s, used);
    }
    return ch;
}

static inline uint32_t AText_offset_of(const AText* self, uint32_t index) {
    uint32_t off = 0;
    uint32_t i = 0;

    while (off < self->byte_num && i < index) {
        off += utf8_next_len_lossy(self->s + off);
        i++;
    }

    return off;
}

static inline int AText_alloc_owned(AText* self, uint32_t byte_num, uint32_t char_num) {
    uint32_t cap = AText_calCap(byte_num + 1);
    char* buf = alib_alloc(cap);
    if (__a_unlikely(buf == nullptr)) {
        return AEXC_alloc_failed;
    }

    buf[byte_num] = '\0';
    self->f = &A_FUNC_TAB(AText);
    self->noLiteral = true;
    self->char_num = char_num;
    self->byte_num = byte_num;
    self->capacity = cap;
    self->s = buf;
    return 0;
}

/* 字面量转为堆分配 */
static inline int AText_ensure_writable(AText* self) {
    if (__a_likely(self->noLiteral)) {
        return 0;
    }

    uint32_t new_cap = AText_calCap(self->byte_num + 1);
    char* new_s = alib_alloc(new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }

    if (__a_likely(self->s != nullptr && self->byte_num != 0)) {
        memcpy(new_s, self->s, self->byte_num);
    }
    new_s[self->byte_num] = '\0';

    self->s = new_s;
    self->noLiteral = true;
    self->capacity = new_cap;
    return 0;
}

/* 扩展容量（假设已可写） */
static inline int AText_grow(AText* self, uint32_t new_cap) {
    if (__a_likely(new_cap <= self->capacity)) {
        return 0;
    }

    new_cap = AText_calCap(new_cap);

    char* new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s;
    self->capacity = new_cap;
    return 0;
}

/* 收缩容量（假设已可写） */
static int AText_sub(AText* self) {
    if (__a_likely(self->byte_num + 1 >= (self->capacity + 1) / 2)) {
        return 0;
    }

    uint32_t new_cap = AText_calCap(self->byte_num + 1);
    char* new_s = alib_realloc(self->s, new_cap);
    if (__a_unlikely(new_s == nullptr)) {
        return AEXC_alloc_failed;
    }
    self->s = new_s;
    self->capacity = new_cap;
    return 0;
}

static inline int AText_reCap(AText* self, uint32_t new_cap) {
    if (__a_unlikely(new_cap < self->byte_num + 1)) {
        return AEXC_overstep;
    }

    if (new_cap > self->capacity) {
        return AText_grow(self, new_cap);
    }
    return AText_sub(self);
}

#if __A_HAVE_ICONV
static iconv_t AText_iconv_open_gbk_to_utf8(void) {
    const char* const from_names[] = { "GBK", "CP936", "GB18030" };
    uint32_t i;
    for (i = 0; i < (uint32_t)(sizeof(from_names) / sizeof(from_names[0])); ++i) {
        iconv_t cd = iconv_open("UTF-8", from_names[i]);
        if (cd != (iconv_t)-1) {
            return cd;
        }
    }
    return (iconv_t)-1;
}

static iconv_t AText_iconv_open_utf8_to_gbk(void) {
    const char* const to_names[] = { "GBK", "CP936" };
    uint32_t i;
    for (i = 0; i < (uint32_t)(sizeof(to_names) / sizeof(to_names[0])); ++i) {
        iconv_t cd = iconv_open(to_names[i], "UTF-8");
        if (cd != (iconv_t)-1) {
            return cd;
        }
    }
    return (iconv_t)-1;
}
#endif

/*******************************************************************************/
uint32_t astrlen_u8(char* s) {
    uint32_t count = 0;
    uint32_t off = 0;

    if (__a_unlikely(s == nullptr)) {
        return 0;
    }

    while (s[off] != '\0') {
        off += utf8_next_len_lossy(s + off);
        count++;
    }

    return count;
}

uint32_t astrlen_u16(char* s) {
    uint32_t count = 0;
    uint32_t off = 0;

    if (__a_unlikely(s == nullptr)) {
        return 0;
    }

    for (;;) {
        uint32_t u0 = u16_read(s, off);
        if (u0 == 0) {
            break;
        }

        off += 2;
        if (u0 >= 0xD800U && u0 <= 0xDBFFU) {
            uint32_t u1 = u16_read(s, off);
            if (u1 >= 0xDC00U && u1 <= 0xDFFFU) {
                off += 2;
            }
        }
        count++;
    }

    return count;
}

uint32_t astrlen_u32(char* s) {
    uint32_t count = 0;
    uint32_t off = 0;

    if (__a_unlikely(s == nullptr)) {
        return 0;
    }

    while (u32_read(s, off) != 0) {
        off += 4;
        count++;
    }

    return count;
}

uint32_t astrlen_gbk(char* s) {
    uint32_t count = 0;
    uint32_t off = 0;

    if (__a_unlikely(s == nullptr)) {
        return 0;
    }

    while (s[off] != '\0') {
        uint8_t b0 = (uint8_t)s[off];
        if (b0 < 0x80U) {
            off += 1;
        } else if (s[off + 1] != '\0' && gbk_is_trail((uint8_t)s[off + 1])) {
            off += 2;
        } else {
            off += 1;
        }
        count++;
    }

    return count;
}

uint32_t Achar_used(Achar ch) {
    uint32_t used = 0;
    while (used < 4 && ch.c[used] != '\0') {
        used++;
    }
    return used;
}

Achar Achar_new(const char* s) {
    uint32_t used = 0;
    uint32_t cp = 0;

    if (__a_unlikely(s == nullptr || s[0] == '\0')) {
        return Achar_from_span("", 0);
    }

    if (utf8_decode_strict(s, &used, &cp)) {
        (void)cp;
        return Achar_from_span(s, used);
    }
    return Achar_from_span(s, 1);
}

/*******************************************************************************/
void A_OBJ_INIT(AText)(AText* self) {
    self->f = &A_FUNC_TAB(AText);
}

void A_OBJ_DEST(AText)(AText* self) {
    if (self->noLiteral) {
        alib_free(self->s);
    }
}

void A_OBJ_COPY(AText)(AText* self, const AText* that) {
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

int A_OBJ_CMPD(AText)(const AText* self, const AText* that) {
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

uint32_t A_OBJ_HASH(AText)(const AText* self) {
    if (__a_unlikely(self == nullptr || self->s == nullptr)) {
        return 0;
    }
    return alib_hash_str(self->s);
}

/*******************************************************************************/
void AText_rm(AText* self, uint32_t index) {
    if (__a_unlikely(index >= self->char_num)) {
        return;
    }

    int ret = AText_ensure_writable(self);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        return;
    }

    uint32_t off = AText_offset_of(self, index);
    uint32_t used = utf8_next_len_lossy(self->s + off);
    memmove(self->s + off, self->s + off + used, self->byte_num - off - used + 1);
    self->byte_num -= used;
    self->char_num--;
    AText_sub(self);
}

void AText_ins(AText* self, uint32_t index, Achar c) {
    uint32_t used = Achar_used(c);
    if (__a_unlikely(index > self->char_num)) {
        aExcSet(AEXC_overstep);
        return;
    }
    if (__a_unlikely(used == 0)) {
        return;
    }

    int ret = AText_ensure_writable(self);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        return;
    }

    ret = AText_grow(self, self->byte_num + used + 1);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        return;
    }

    uint32_t off = AText_offset_of(self, index);
    if (self->byte_num == 0) {
        self->s[used] = '\0';
    } else {
        memmove(self->s + off + used, self->s + off, self->byte_num - off + 1);
    }
    memcpy(self->s + off, c.c, used);
    self->byte_num += used;
    self->char_num++;
}

void AText_pushBack(AText* self, Achar c) {
    AText_ins(self, self->char_num, c);
}

void AText_pushFront(AText* self, Achar c) {
    AText_ins(self, 0, c);
}

Achar AText_popBack(AText* self) {
    if (__a_unlikely(self->char_num == 0)) {
        return Achar_from_span("", 0);
    }

    if (AText_ensure_writable(self) != 0) {
        aExcSet(AEXC_alloc_failed);
        return Achar_from_span("", 0);
    }

    uint32_t off = AText_offset_of(self, self->char_num - 1);
    uint32_t used = self->byte_num - off;
    Achar out = Achar_from_span(self->s + off, used);
    self->s[off] = '\0';
    self->byte_num = off;
    self->char_num--;
    AText_sub(self);
    return out;
}

Achar AText_popFront(AText* self) {
    if (__a_unlikely(self->char_num == 0)) {
        return Achar_from_span("", 0);
    }

    if (AText_ensure_writable(self) != 0) {
        aExcSet(AEXC_alloc_failed);
        return Achar_from_span("", 0);
    }

    uint32_t used = utf8_next_len_lossy(self->s);
    Achar out = Achar_from_span(self->s, used);
    memmove(self->s, self->s + used, self->byte_num - used + 1);
    self->byte_num -= used;
    self->char_num--;
    AText_sub(self);
    return out;
}

void AText_addBack(AText* self, AText that) {
    bool alias = false;
    if (self->s == that.s) {
        that = A_COPY(AText, that);
        alias = true;
    }

    if (__a_unlikely(that.byte_num == 0)) {
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    int ret = AText_ensure_writable(self);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    ret = AText_grow(self, self->byte_num + that.byte_num + 1);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    memcpy(self->s + self->byte_num, that.s, that.byte_num + 1);
    self->byte_num += that.byte_num;
    self->char_num += that.char_num;

    if (alias) {
        A_DEST(AText, that);
    }
}

void AText_addFront(AText* self, AText that) {
    bool alias = false;
    if (self->s == that.s) {
        that = A_COPY(AText, that);
        alias = true;
    }

    if (__a_unlikely(that.byte_num == 0)) {
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    int ret = AText_ensure_writable(self);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    ret = AText_grow(self, self->byte_num + that.byte_num + 1);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        if (alias) {
            A_DEST(AText, that);
        }
        return;
    }

    memmove(self->s + that.byte_num, self->s, self->byte_num + 1);
    memcpy(self->s, that.s, that.byte_num);
    self->byte_num += that.byte_num;
    self->char_num += that.char_num;

    if (alias) {
        A_DEST(AText, that);
    }
}

void AText_truncate(AText* self, uint32_t index) {
    if (__a_unlikely(index >= self->char_num)) {
        return;
    }

    int ret = AText_ensure_writable(self);
    if (ret != 0) {
        aExcSet(AEXC_alloc_failed);
        return;
    }

    uint32_t off = AText_offset_of(self, index);
    self->byte_num = off;
    self->char_num = index;
    self->s[off] = '\0';
    AText_sub(self);
}

/*******************************************************************************/
AText AText_forU32(char* s) {
    AText out = AText_empty_value();
    uint32_t in_off = 0;
    uint32_t byte_num = 0;
    uint32_t char_num = 0;

    if (__a_unlikely(s == nullptr)) {
        return out;
    }

    for (;;) {
        uint32_t cp = u32_read(s, in_off);
        if (cp == 0) {
            break;
        }

        uint32_t used = utf8_size_from_cp(cp);
        if (used == 0) {
            aExcSet(AEXC_outdomain);
            return out;
        }

        byte_num += used;
        char_num++;
        in_off += 4;
    }

    if (byte_num == 0) {
        return out;
    }

    if (AText_alloc_owned(&out, byte_num, char_num) != 0) {
        aExcSet(AEXC_alloc_failed);
        return AText_empty_value();
    }

    in_off = 0;
    uint32_t out_off = 0;
    while (out_off < byte_num) {
        uint32_t cp = u32_read(s, in_off);
        uint32_t used = 0;
        if (!utf8_encode(cp, out.s + out_off, &used)) {
            A_DEST(AText, out);
            aExcSet(AEXC_outdomain);
            return AText_empty_value();
        }
        out_off += used;
        in_off += 4;
    }
    out.s[byte_num] = '\0';
    return out;
}

AText AText_forU16(char* s) {
    AText out = AText_empty_value();
    uint32_t in_off = 0;
    uint32_t byte_num = 0;
    uint32_t char_num = 0;

    if (__a_unlikely(s == nullptr)) {
        return out;
    }

    for (;;) {
        uint32_t cp = 0;
        uint32_t u0 = u16_read(s, in_off);
        if (u0 == 0) {
            break;
        }

        if (u0 >= 0xD800U && u0 <= 0xDBFFU) {
            uint32_t u1 = u16_read(s, in_off + 2);
            if (!(u1 >= 0xDC00U && u1 <= 0xDFFFU)) {
                aExcSet(AEXC_outdomain);
                return out;
            }
            cp = 0x10000U + (((u0 - 0xD800U) << 10) | (u1 - 0xDC00U));
            in_off += 4;
        } else if (u0 >= 0xDC00U && u0 <= 0xDFFFU) {
            aExcSet(AEXC_outdomain);
            return out;
        } else {
            cp = u0;
            in_off += 2;
        }

        uint32_t used = utf8_size_from_cp(cp);
        if (used == 0) {
            aExcSet(AEXC_outdomain);
            return out;
        }

        byte_num += used;
        char_num++;
    }

    if (byte_num == 0) {
        return out;
    }

    if (AText_alloc_owned(&out, byte_num, char_num) != 0) {
        aExcSet(AEXC_alloc_failed);
        return AText_empty_value();
    }

    in_off = 0;
    uint32_t out_off = 0;
    while (out_off < byte_num) {
        uint32_t cp = 0;
        uint32_t u0 = u16_read(s, in_off);
        uint32_t used = 0;

        if (u0 >= 0xD800U && u0 <= 0xDBFFU) {
            uint32_t u1 = u16_read(s, in_off + 2);
            cp = 0x10000U + (((u0 - 0xD800U) << 10) | (u1 - 0xDC00U));
            in_off += 4;
        } else {
            cp = u0;
            in_off += 2;
        }

        if (!utf8_encode(cp, out.s + out_off, &used)) {
            A_DEST(AText, out);
            aExcSet(AEXC_outdomain);
            return AText_empty_value();
        }
        out_off += used;
    }
    out.s[byte_num] = '\0';
    return out;
}

AText AText_forGBK(char* s) {
    AText out = AText_empty_value();

    if (__a_unlikely(s == nullptr)) {
        return out;
    }

    uint32_t src_len = (uint32_t)strlen(s);
    if (src_len == 0) {
        return out;
    }

#if __A_HAVE_ICONV
    iconv_t cd = AText_iconv_open_gbk_to_utf8();
    if (cd == (iconv_t)-1) {
        aExcSet(AEXC_system_error);
        return out;
    }

    uint32_t max_out = src_len * 3 + 1;
    if (AText_alloc_owned(&out, max_out - 1, astrlen_gbk(s)) != 0) {
        iconv_close(cd);
        aExcSet(AEXC_alloc_failed);
        return AText_empty_value();
    }

    char* in_buf = s;
    size_t in_left = src_len;
    char* out_buf = out.s;
    size_t out_left = out.capacity - 1;

    size_t ret = iconv(cd, &in_buf, &in_left, &out_buf, &out_left);
    iconv_close(cd);
    if (ret == (size_t)-1 || in_left != 0) {
        A_DEST(AText, out);
        if (errno == E2BIG) {
            aExcSet(AEXC_overstep);
        } else if (errno == EILSEQ || errno == EINVAL) {
            aExcSet(AEXC_outdomain);
        } else {
            aExcSet(AEXC_system_error);
        }
        return AText_empty_value();
    }

    out.byte_num = (uint32_t)(out_buf - out.s);
    out.char_num = astrlen_u8(out.s);
    out.s[out.byte_num] = '\0';
    AText_sub(&out);
    return out;
#else
    uint32_t i;
    for (i = 0; i < src_len; ++i) {
        if (((uint8_t)s[i]) >= 0x80U) {
            aExcSet(AEXC_invalid_function);
            return out;
        }
    }

    if (AText_alloc_owned(&out, src_len, src_len) != 0) {
        aExcSet(AEXC_alloc_failed);
        return AText_empty_value();
    }
    memcpy(out.s, s, src_len + 1);
    return out;
#endif
}

static inline void AText_zero_buf(char* buf, uint32_t buf_size, uint32_t zero_bytes) {
    if (buf_size == 0) {
        return;
    }
    memset(buf, 0, buf_size < zero_bytes ? buf_size : zero_bytes);
}

void AText_toU32(const AText* self, char* buf, uint32_t buf_size) {
    uint32_t need = 4;
    uint32_t off = 0;

    if (__a_unlikely(self == nullptr || buf == nullptr)) {
        aExcSet(AEXC_nullptr);
        return;
    }

    while (off < self->byte_num) {
        uint32_t used = 0;
        uint32_t cp = 0;
        if (!utf8_decode_strict(self->s + off, &used, &cp) || used == 0) {
            AText_zero_buf(buf, buf_size, 4);
            aExcSet(AEXC_outdomain);
            return;
        }
        need += 4;
        off += used;
    }

    if (__a_unlikely(buf_size < need)) {
        AText_zero_buf(buf, buf_size, 4);
        aExcSet(AEXC_overstep);
        return;
    }

    off = 0;
    uint32_t out_off = 0;
    while (off < self->byte_num) {
        uint32_t used = 0;
        uint32_t cp = 0;
        utf8_decode_strict(self->s + off, &used, &cp);
        u32_write(buf, out_off, cp);
        off += used;
        out_off += 4;
    }
    u32_write(buf, out_off, 0);
}

void AText_toU16(const AText* self, char* buf, uint32_t buf_size) {
    uint32_t need = 2;
    uint32_t off = 0;

    if (__a_unlikely(self == nullptr || buf == nullptr)) {
        aExcSet(AEXC_nullptr);
        return;
    }

    while (off < self->byte_num) {
        uint32_t used = 0;
        uint32_t cp = 0;
        if (!utf8_decode_strict(self->s + off, &used, &cp) || used == 0) {
            AText_zero_buf(buf, buf_size, 2);
            aExcSet(AEXC_outdomain);
            return;
        }
        need += (cp <= 0xFFFFU) ? 2U : 4U;
        off += used;
    }

    if (__a_unlikely(buf_size < need)) {
        AText_zero_buf(buf, buf_size, 2);
        aExcSet(AEXC_overstep);
        return;
    }

    off = 0;
    uint32_t out_off = 0;
    while (off < self->byte_num) {
        uint32_t used = 0;
        uint32_t cp = 0;
        utf8_decode_strict(self->s + off, &used, &cp);
        if (cp <= 0xFFFFU) {
            u16_write(buf, out_off, (uint16_t)cp);
            out_off += 2;
        } else {
            uint32_t v = cp - 0x10000U;
            u16_write(buf, out_off, (uint16_t)(0xD800U | (v >> 10)));
            u16_write(buf, out_off + 2, (uint16_t)(0xDC00U | (v & 0x3FFU)));
            out_off += 4;
        }
        off += used;
    }
    u16_write(buf, out_off, 0);
}

void AText_toGBK(const AText* self, char* buf, uint32_t buf_size) {
    if (__a_unlikely(self == nullptr || buf == nullptr)) {
        aExcSet(AEXC_nullptr);
        return;
    }

    if (__a_unlikely(buf_size == 0)) {
        aExcSet(AEXC_overstep);
        return;
    }

    if (self->byte_num == 0) {
        buf[0] = '\0';
        return;
    }

#if __A_HAVE_ICONV
    iconv_t cd = AText_iconv_open_utf8_to_gbk();
    if (cd == (iconv_t)-1) {
        buf[0] = '\0';
        aExcSet(AEXC_system_error);
        return;
    }

    char* in_buf = self->s;
    size_t in_left = self->byte_num;
    char* out_buf = buf;
    size_t out_left = buf_size - 1;
    *out_buf = '\0';

    size_t ret = iconv(cd, &in_buf, &in_left, &out_buf, &out_left);
    iconv_close(cd);
    if (ret == (size_t)-1 || in_left != 0) {
        buf[0] = '\0';
        if (errno == E2BIG) {
            aExcSet(AEXC_overstep);
        } else if (errno == EILSEQ || errno == EINVAL) {
            aExcSet(AEXC_outdomain);
        } else {
            aExcSet(AEXC_system_error);
        }
        return;
    }

    *out_buf = '\0';
#else
    uint32_t off = 0;
    uint32_t out_off = 0;
    while (off < self->byte_num) {
        uint32_t used = 0;
        uint32_t cp = 0;
        if (!utf8_decode_strict(self->s + off, &used, &cp) || used == 0) {
            buf[0] = '\0';
            aExcSet(AEXC_outdomain);
            return;
        }
        if (cp >= 0x80U) {
            buf[0] = '\0';
            aExcSet(AEXC_invalid_function);
            return;
        }
        if (out_off + 1 >= buf_size) {
            buf[0] = '\0';
            aExcSet(AEXC_overstep);
            return;
        }
        buf[out_off++] = (char)cp;
        off += used;
    }
    buf[out_off] = '\0';
#endif
}
