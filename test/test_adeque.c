#include <alib/alib.h>
#include <alib/adeque.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

ADeque_Define(int);
ADeque_Generate(int);
A_TYPE_REGISTER(ADeque(int));

ADeque_Define(AString);
ADeque_Generate(AString);
A_TYPE_REGISTER(ADeque(AString));

static void test_adeque_int(void) {
    RAII(ADeque(int)) dq = A_INIT(ADeque(int));
    assert(!aExcOccur());
    assert(dq.f->empty(&dq));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        dq.f->pushBack(&dq, i);
        assert(!aExcOccur());
    }
    assert(dq.f->getNumber(&dq) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        int *p = dq.f->at(&dq, i);
        assert(p != NULL);
        assert(*p == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) dq2 = A_COPY(ADeque(int), dq);
    assert(!aExcOccur());
    assert(dq2.f->getNumber(&dq2) == 10);
    *dq2.f->at(&dq2, 0) = 999;
    assert(*dq.f->at(&dq, 0) == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    dq.f->popBack(&dq, NULL);
    assert(!aExcOccur());
    assert(dq.f->getNumber(&dq) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) empty = A_INIT(ADeque(int));
    empty.f->popBack(&empty, NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) big = A_INIT(ADeque(int));
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

static void test_adeque_astring(void) {
    RAII(ADeque(AString)) dq = A_INIT(ADeque(AString));
    assert(!aExcOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        RAII(AString) tmp = AString_new("deque");
        dq.f->pushBack(&dq, tmp);
        assert(!aExcOccur());
    }
    assert(dq.f->getNumber(&dq) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AString)) dq2 = A_COPY(ADeque(AString), dq);
    AString *ps2 = dq2.f->at(&dq2, 0);
    ps2->f->pushBack(ps2, 'Z');
    AString *ps1 = dq.f->at(&dq, 0);
    assert(strcmp(ps1->s, "deque") == 0);
    assert(strcmp(ps2->s, "dequeZ") == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AString)) empty = A_INIT(ADeque(AString));
    empty.f->popBack(&empty, NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AString)) big = A_INIT(ADeque(AString));
    for (int i = 0; i < 600; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "dq%d", i);
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
        snprintf(expected, sizeof(expected), "dq%d", i);
        assert(strcmp(ps->s, expected) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_adeque_int();
    test_adeque_astring();
    printf("All ADeque tests passed.\n");
    return 0;
}

