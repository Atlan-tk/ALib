#include <alib/alib.h>
#include <alib/atree.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

ATree_Define(int, int);
ATree_Generate(int, int);
A_TYPE_REGISTER(ATree(int, int));

ATree_Define(AStr, AStr);
ATree_Generate(AStr, AStr);
A_TYPE_REGISTER(ATree(AStr, AStr));

static void test_atree_int(void) {
    RAII(ATree(int, int)) t = A_INIT(ATree(int, int));
    assert(!aErrOccur());
    assert(t.f->empty(&t));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 插入 */
    for (int i = 0; i < 10; i++) {
        t.f->ins(&t, i, i * 10);
        assert(!aErrOccur());
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
    assert(!aErrOccur());
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
    assert(!aErrOccur());
    assert(t.f->at(&t, 5) == NULL);
    assert(t.f->getNumber(&t) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除不存在的键应产生异常 */
    t.f->rm(&t, 5);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(ATree(int, int)) big = A_INIT(ATree(int, int));
    for (int i = 0; i < 600; i++) {
        big.f->ins(&big, i, i + 100);
        assert(!aErrOccur());
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
    RAII(ATree(AStr, AStr)) t = A_INIT(ATree(AStr, AStr));
    assert(!aErrOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "key%d", i);
        snprintf(vbuf, sizeof(vbuf), "val%d", i);
        RAII(AStr) k = A_INIT(AStr);
        RAII(AStr) v = A_INIT(AStr);
        {
            RAII(AStr) tk = AStr_new(kbuf);
            RAII(AStr) tv = AStr_new(vbuf);
            AStr_addBack(&k, tk.s);
            AStr_addBack(&v, tv.s);
        }
        t.f->ins(&t, k, v);
        assert(!aErrOccur());
    }
    assert(t.f->getNumber(&t) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝 */
    RAII(ATree(AStr, AStr)) t2 = A_COPY(ATree(AStr, AStr), t);
    assert(!aErrOccur());
    assert(t2.f->getNumber(&t2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除操作 */
    {
        RAII(AStr) delkey = A_INIT(AStr);
        {
            RAII(AStr) tmp = AStr_new("key5");
            AStr_addBack(&delkey, tmp.s);
        }
        t.f->rm(&t, delkey);
        assert(!aErrOccur());
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 迭代器顺序校验（字符串比较顺序） */
    int idx = 0;
    forEach(it, t) {
        AStr *val = it.p;
        assert(val != NULL);
        idx++;
    }
    assert(idx == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据 */
    RAII(ATree(AStr, AStr)) big = A_INIT(ATree(AStr, AStr));
    for (int i = 0; i < 600; i++) {
        char kbuf[32], vbuf[32];
        snprintf(kbuf, sizeof(kbuf), "k%03d", i);
        snprintf(vbuf, sizeof(vbuf), "v%03d", i);
        RAII(AStr) k = A_INIT(AStr);
        RAII(AStr) v = A_INIT(AStr);
        {
            RAII(AStr) tk = AStr_new(kbuf);
            RAII(AStr) tv = AStr_new(vbuf);
            AStr_addBack(&k, tk.s);
            AStr_addBack(&v, tv.s);
        }
        big.f->ins(&big, k, v);
        assert(!aErrOccur());
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

