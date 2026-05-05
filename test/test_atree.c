#include <alib/alib.h>
#include <alib/atree.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

ATree_Define(int, int);
ATree_Generate(int, int);
A_TYPE_REGISTER(ATree(int, int));

ATree_Define(AString, AString);
ATree_Generate(AString, AString);
A_TYPE_REGISTER(ATree(AString, AString));

static void test_atree_int(void) {
    RAII(ATree(int, int)) t = A_INIT(ATree(int, int));
    assert(!aExcOccur());
    assert(t.f->empty(&t));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 插入 */
    for (int i = 0; i < 10; i++) {
        t.f->ins(&t, i, i * 10);
        assert(!aExcOccur());
    }
    assert(t.f->getNumber(&t) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 查找 */
    for (int i = 0; i < 10; i++) {
        int *pv = t.f->at(&t, i);
        assert(pv != NULL);
        assert(*pv == i * 10);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝 */
    RAII(ATree(int, int)) t2 = A_COPY(ATree(int, int), t);
    assert(!aExcOccur());
    assert(t2.f->getNumber(&t2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
    for (int i = 0; i < 10; i++) {
        int *p = t2.f->at(&t2, i);
        assert(p != NULL && *p == i * 10);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 迭代器验证顺序 */
    int expected = 0;
    forEach(it, t) {
        assert(*it.p == expected * 10);
        expected++;
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除 */
    t.f->rm(&t, 5);
    assert(!aExcOccur());
    assert(t.f->at(&t, 5) == NULL);
    assert(t.f->getNumber(&t) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除不存在的键应产生异常 */
    t.f->rm(&t, 5);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(ATree(int, int)) big = A_INIT(ATree(int, int));
    for (int i = 0; i < 600; i++) {
        big.f->ins(&big, i, i + 100);
        assert(!aExcOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int count = 0;
    forEach(it, big) {
        assert(*it.p == count + 100);
        count++;
    }
    assert(count == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_atree_astring(void) {
    RAII(ATree(AString, AString)) t = A_INIT(ATree(AString, AString));
    assert(!aExcOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "key%d", i);
        snprintf(vbuf, sizeof(vbuf), "val%d", i);
        RAII(AString) k = A_INIT(AString);
        RAII(AString) v = A_INIT(AString);
        {
            RAII(AString) tk = AString_new(kbuf);
            RAII(AString) tv = AString_new(vbuf);
            k.f->addBack(&k, tk);
            v.f->addBack(&v, tv);
        }
        t.f->ins(&t, k, v);
        assert(!aExcOccur());
    }
    assert(t.f->getNumber(&t) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝 */
    RAII(ATree(AString, AString)) t2 = A_COPY(ATree(AString, AString), t);
    assert(!aExcOccur());
    assert(t2.f->getNumber(&t2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除操作 */
    {
        RAII(AString) delkey = A_INIT(AString);
        {
            RAII(AString) tmp = AString_new("key5");
            delkey.f->addBack(&delkey, tmp);
        }
        t.f->rm(&t, delkey);
        assert(!aExcOccur());
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 迭代器顺序校验（字符串比较顺序） */
    int idx = 0;
    forEach(it, t) {
        AString *val = it.p;
        assert(val != NULL);
        idx++;
    }
    assert(idx == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(ATree(AString, AString)) big = A_INIT(ATree(AString, AString));
    for (int i = 0; i < 600; i++) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "k%03d", i);
        snprintf(vbuf, sizeof(vbuf), "v%03d", i);
        RAII(AString) k = A_INIT(AString);
        RAII(AString) v = A_INIT(AString);
        {
            RAII(AString) tk = AString_new(kbuf);
            RAII(AString) tv = AString_new(vbuf);
            k.f->addBack(&k, tk);
            v.f->addBack(&v, tv);
        }
        big.f->ins(&big, k, v);
        assert(!aExcOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_atree_int();
    test_atree_astring();
    printf("All ATree tests passed.\n");
    return 0;
}

