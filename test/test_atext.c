#include <alib/alib.h>
#include <alib/atext.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char* k_utf8_a_ni_smile = "A\xE4\xBD\xA0\xF0\x9F\x98\x80";
static const char* k_utf8_ni = "\xE4\xBD\xA0";
static const char* k_utf8_hao = "\xE5\xA5\xBD";
static const char* k_utf8_smile = "\xF0\x9F\x98\x80";

static AText owned_text(const char* s) {
    AText out = A_INIT(AText);
    AText lit = AText_new((char*)s);
    out.f->addBack(&out, lit);
    assert(!aExcOccur());
    return out;
}

static void test_strlen_helpers(void) {
    uint16_t u16[] = { 0x0041, 0x4F60, 0xD83D, 0xDE00, 0x0000 };
    uint32_t u32[] = { 0x00000041U, 0x00004F60U, 0x0001F600U, 0x00000000U };
    char gbk[] = { 'A', (char)0xC4, (char)0xE3, '\0' };

    assert(astrlen_u8((char*)k_utf8_a_ni_smile) == 3);
    assert(astrlen_u16((char*)u16) == 3);
    assert(astrlen_u32((char*)u32) == 3);
    assert(astrlen_gbk(gbk) == 2);
}

static void test_achar(void) {
    Achar ni = Achar_new(k_utf8_ni);
    Achar smile = Achar_new(k_utf8_smile);

    assert(Achar_used(ni) == 3);
    assert(Achar_used(smile) == 4);
    assert(memcmp(ni.c, k_utf8_ni, 3) == 0);
    assert(memcmp(smile.c, k_utf8_smile, 4) == 0);
}

static void test_text_ops(void) {
    RAII(AText) text = AText_new((char*)k_utf8_ni);
    assert(text.f->getNumber(&text) == 1);
    assert(strcmp(text.s, k_utf8_ni) == 0);

    text.f->pushBack(&text, Achar_new(k_utf8_hao));
    assert(!aExcOccur());
    assert(text.char_num == 2);
    assert(strcmp(text.s, "\xE4\xBD\xA0\xE5\xA5\xBD") == 0);

    text.f->pushFront(&text, Achar_new("A"));
    assert(!aExcOccur());
    assert(text.char_num == 3);
    assert(strcmp(text.s, "A\xE4\xBD\xA0\xE5\xA5\xBD") == 0);

    text.f->ins(&text, 2, Achar_new(k_utf8_smile));
    assert(!aExcOccur());
    assert(text.char_num == 4);
    assert(strcmp(text.s, "A\xE4\xBD\xA0\xF0\x9F\x98\x80\xE5\xA5\xBD") == 0);

    text.f->rm(&text, 1);
    assert(!aExcOccur());
    assert(text.char_num == 3);
    assert(strcmp(text.s, "A\xF0\x9F\x98\x80\xE5\xA5\xBD") == 0);

    Achar back = text.f->popBack(&text);
    assert(Achar_used(back) == 3);
    assert(memcmp(back.c, k_utf8_hao, 3) == 0);
    assert(strcmp(text.s, "A\xF0\x9F\x98\x80") == 0);

    Achar front = text.f->popFront(&text);
    assert(Achar_used(front) == 1);
    assert(front.c[0] == 'A');
    assert(strcmp(text.s, k_utf8_smile) == 0);

    text.f->truncate(&text, 0);
    assert(text.f->empty(&text));
    assert(text.byte_num == 0);
    assert(strcmp(text.s, "") == 0);
}

static void test_copy_and_concat(void) {
    RAII(AText) base = AText_new((char*)k_utf8_ni);
    RAII(AText) copy = A_COPY(AText, base);

    copy.f->pushBack(&copy, Achar_new(k_utf8_hao));
    assert(!aExcOccur());
    assert(strcmp(base.s, k_utf8_ni) == 0);
    assert(strcmp(copy.s, "\xE4\xBD\xA0\xE5\xA5\xBD") == 0);

    RAII(AText) front = owned_text("A");
    RAII(AText) back = owned_text(k_utf8_smile);

    copy.f->addFront(&copy, front);
    copy.f->addBack(&copy, back);
    assert(!aExcOccur());
    assert(strcmp(copy.s, "A\xE4\xBD\xA0\xE5\xA5\xBD\xF0\x9F\x98\x80") == 0);

    copy.f->addBack(&copy, copy);
    assert(!aExcOccur());
    assert(copy.char_num == 8);
    assert(strcmp(copy.s,
                  "A\xE4\xBD\xA0\xE5\xA5\xBD\xF0\x9F\x98\x80"
                  "A\xE4\xBD\xA0\xE5\xA5\xBD\xF0\x9F\x98\x80") == 0);
}

static void test_utf16_utf32_convert(void) {
    uint16_t u16_src[] = { 0x0041, 0x4F60, 0xD83D, 0xDE00, 0x0000 };
    uint32_t u32_src[] = { 0x00000041U, 0x00004F60U, 0x0001F600U, 0x00000000U };
    uint16_t u16_out[8] = { 0 };
    uint32_t u32_out[8] = { 0 };

    RAII(AText) from_u16 = AText_forU16((char*)u16_src);
    assert(!aExcOccur());
    assert(strcmp(from_u16.s, k_utf8_a_ni_smile) == 0);

    RAII(AText) from_u32 = AText_forU32((char*)u32_src);
    assert(!aExcOccur());
    assert(strcmp(from_u32.s, k_utf8_a_ni_smile) == 0);

    AText_toU16(&from_u32, (char*)u16_out, sizeof(u16_out));
    assert(!aExcOccur());
    assert(memcmp(u16_src, u16_out, sizeof(u16_src)) == 0);

    AText_toU32(&from_u16, (char*)u32_out, sizeof(u32_out));
    assert(!aExcOccur());
    assert(memcmp(u32_src, u32_out, sizeof(u32_src)) == 0);
}

static void test_gbk_convert(void) {
    char gbk_src[] = { 'A', (char)0xC4, (char)0xE3, '\0' };
    char gbk_out[16] = { 0 };

    RAII(AText) from_gbk = AText_forGBK(gbk_src);
    if (aExcOccur()) {
        assert(aExcGet() == AEXC_invalid_function || aExcGet() == AEXC_system_error);
        aExcClean();
        return;
    }

    assert(strcmp(from_gbk.s, "A\xE4\xBD\xA0") == 0);

    AText_toGBK(&from_gbk, gbk_out, sizeof(gbk_out));
    assert(!aExcOccur());
    assert(strcmp(gbk_out, gbk_src) == 0);
}

int main(void) {
    test_strlen_helpers();
    test_achar();
    test_text_ops();
    test_copy_and_concat();
    test_utf16_utf32_convert();
    test_gbk_convert();
    printf("AText tests passed.\n");
    return 0;
}
