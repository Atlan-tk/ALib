#include <alib/alib.h>
#include <alib/alist.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

AList_Define(int);
AList_Generate(int);
A_TYPE_REGISTER(AList(int));

AList_Define(AString);
AList_Generate(AString);
A_TYPE_REGISTER(AList(AString));

static void test_alist_int(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aExcOccur());
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        list.f->pushBack(&list, i);
        assert(!aExcOccur());
    }
    assert(list.f->getNumber(&list) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        int *p = list.f->at(&list, i);
        assert(p != NULL);
        assert(*p == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(int)) list2 = A_COPY(AList(int), list);
    assert(!aExcOccur());
    assert(list2.f->getNumber(&list2) == 10);
    *list2.f->at(&list2, 0) = 999;
    assert(*list.f->at(&list, 0) == 0);
    assert(*list2.f->at(&list2, 0) == 999);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->popBack(&list, NULL);
    assert(!aExcOccur());
    assert(list.f->getNumber(&list) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(int)) empty = A_INIT(AList(int));
    empty.f->popBack(&empty, NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(int)) big = A_INIT(AList(int));
    for (int i = 0; i < 600; i++) {
        big.f->pushBack(&big, i);
        assert(!aExcOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        assert(*big.f->at(&big, i) == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_astring(void) {
    RAII(AList(AString)) list = A_INIT(AList(AString));
    assert(!aExcOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        RAII(AString) tmp = AString_new("hello");
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    assert(list.f->getNumber(&list) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AString)) list2 = A_COPY(AList(AString), list);
    assert(!aExcOccur());
    AString *ps2 = list2.f->at(&list2, 0);
    ps2->f->pushBack(ps2, '!');
    AString *ps1 = list.f->at(&list, 0);
    assert(strcmp(ps1->s, "hello") == 0);
    assert(strcmp(ps2->s, "hello!") == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AString)) empty = A_INIT(AList(AString));
    empty.f->popBack(&empty, NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AString)) big = A_INIT(AList(AString));
    for (int i = 0; i < 600; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "item%d", i);
        RAII(AString) tmp = AString_new(buf);
        RAII(AString) elem = A_INIT(AString);
        elem.f->addBack(&elem, tmp);
        big.f->pushBack(&big, elem);
        assert(!aExcOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        AString *ps = big.f->at(&big, i);
        char expected[32];
        snprintf(expected, sizeof(expected), "item%d", i);
        assert(strcmp(ps->s, expected) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_alist_int();
    test_alist_astring();
    printf("All AList tests passed.\n");
    return 0;
}

