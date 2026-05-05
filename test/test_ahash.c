#include <alib/alib.h>
#include <alib/ahash.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

AHash_Define(int, int);
AHash_Generate(int, int);
A_TYPE_REGISTER(AHash(int, int));

AHash_Define(AString, AString);
AHash_Generate(AString, AString);
A_TYPE_REGISTER(AHash(AString, AString));

static void test_ahash_int(void) {
    RAII(AHash(int, int)) h = A_INIT(AHash(int, int));
    assert(!aExcOccur());
    assert(h.f->empty(&h));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 插入 */
    for (int i = 0; i < 10; i++) {
        h.f->ins(&h, i, i * 10);
        assert(!aExcOccur());
    }
    assert(h.f->getNumber(&h) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 查找 */
    for (int i = 0; i < 10; i++) {
        int *pv = h.f->at(&h, i);
        assert(pv != NULL);
        assert(*pv == i * 10);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 查找不存在的键 */
    int *pv = h.f->at(&h, 999);
    assert(pv == NULL);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝 */
    aExcClean();
    RAII(AHash(int, int)) h2 = A_COPY(AHash(int, int), h);
    assert(!aExcOccur());
    assert(h2.f->getNumber(&h2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
    for (int i = 0; i < 10; i++) {
        int *p = h2.f->at(&h2, i);
        assert(p != NULL && *p == i * 10);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除 */
    h.f->rm(&h, 5);
    assert(!aExcOccur());
    assert(h.f->at(&h, 5) == NULL);
    assert(h.f->getNumber(&h) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除不存在的键应设置异常 */
    h.f->rm(&h, 5);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(AHash(int, int)) big = A_INIT(AHash(int, int));
    for (int i = 0; i < 600; i++) {
        big.f->ins(&big, i, i + 1000);
        assert(!aExcOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        int *p = big.f->at(&big, i);
        assert(p != NULL && *p == i + 1000);
    }
}

static void test_ahash_astring(void) {
    RAII(AHash(AString, AString)) h = A_INIT(AHash(AString, AString));
    assert(!aExcOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 准备键值对 */
    for (int i = 0; i < 10; i++) {
        char keybuf[32], valbuf[32];
        snprintf(keybuf, sizeof(keybuf), "key%d", i);
        snprintf(valbuf, sizeof(valbuf), "val%d", i);
        RAII(AString) k = A_INIT(AString);
        RAII(AString) v = A_INIT(AString);
        {
            RAII(AString) tk = AString_new(keybuf);
            RAII(AString) tv = AString_new(valbuf);
            k.f->addBack(&k, tk);
            v.f->addBack(&v, tv);
        }
        h.f->ins(&h, k, v);
        assert(!aExcOccur());
    }
    assert(h.f->getNumber(&h) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 查找并验证值 */
    for (int i = 0; i < 10; i++) {
        char expected[32];
        snprintf(expected, sizeof(expected), "val%d", i);
        RAII(AString) key_lookup = A_INIT(AString);
        {
            char keybuf[32];
            snprintf(keybuf, sizeof(keybuf), "key%d", i);
            RAII(AString) tk = AString_new(keybuf);
            key_lookup.f->addBack(&key_lookup, tk);
        }
        AString *pv = h.f->at(&h, key_lookup);
        assert(pv != NULL);
        assert(strcmp(pv->s, expected) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝 */
    RAII(AHash(AString, AString)) h2 = A_COPY(AHash(AString, AString), h);
    assert(!aExcOccur());
    assert(h2.f->getNumber(&h2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除 */
    {
        RAII(AString) delkey = A_INIT(AString);
        RAII(AString) tmp = AString_new("key5");
        delkey.f->addBack(&delkey, tmp);
        h.f->rm(&h, delkey);
        assert(!aExcOccur());
        assert(h.f->at(&h, delkey) == NULL);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(AHash(AString, AString)) big = A_INIT(AHash(AString, AString));
    for (int i = 0; i < 600; i++) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "k%d", i);
        snprintf(vbuf, sizeof(vbuf), "v%d", i);
        RAII(AString) k = A_INIT(AString);
        RAII(AString) v = A_INIT(AString);
        {
            RAII(AString) tk = AString_new(kbuf);
            RAII(AString) tv = AString_new(vbuf);
            k.f->addBack(&k, tk);
            v.f->addBack(&v, tv);
        }
        big.f->ins(&big, k, v);
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
    /* 抽样验证 */
    for (int i = 0; i < 600; i += 100) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "k%d", i);
        snprintf(vbuf, sizeof(vbuf), "v%d", i);
        RAII(AString) lookup = A_INIT(AString);
        {
            RAII(AString) tmp = AString_new(kbuf);
            lookup.f->addBack(&lookup, tmp);
        }
        AString *pv = big.f->at(&big, lookup);
        assert(pv != NULL);
        assert(strcmp(pv->s, vbuf) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_ahash_int();
    test_ahash_astring();
    printf("All AHash tests passed.\n");
    return 0;
}

