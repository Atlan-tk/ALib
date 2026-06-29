#include <alib/alib.h>
#include <alib/adeque.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

ADeque_Define(int);
ADeque_Generate(int);
A_TYPE_REGISTER(ADeque(int));

ADeque_Define(AStr);
ADeque_Generate(AStr);
A_TYPE_REGISTER(ADeque(AStr));

static void test_adeque_int(void) {
    RAII(ADeque(int)) dq = A_INIT(ADeque(int));
    assert(!aErrOccur());
    assert(dq.f->empty(&dq));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        dq.f->pushBack(&dq, i);
        assert(!aErrOccur());
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
    assert(!aErrOccur());
    assert(dq2.f->getNumber(&dq2) == 10);
    *dq2.f->at(&dq2, 0) = 999;
    assert(*dq.f->at(&dq, 0) == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    dq.f->popBack(&dq, NULL);
    assert(!aErrOccur());
    assert(dq.f->getNumber(&dq) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) empty = A_INIT(ADeque(int));
    empty.f->popBack(&empty, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) big = A_INIT(ADeque(int));
    for (int i = 0; i < 600; i++) {
        big.f->pushBack(&big, i);
        assert(!aErrOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        assert(*big.f->at(&big, i) == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(int)) front = A_INIT(ADeque(int));
    front.f->pushFront(&front, 1);
    assert(!aErrOccur());
    front.f->popFront(&front, NULL);
    assert(!aErrOccur());
    assert(front.f->empty(&front));
    front.f->pushFront(&front, 2);
    assert(!aErrOccur());
    assert(front.f->getNumber(&front) == 1);
    assert(*front.f->at(&front, 0) == 2);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_adeque_astring(void) {
    RAII(ADeque(AStr)) dq = A_INIT(ADeque(AStr));
    assert(!aErrOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        RAII(AStr) tmp = AStr_new("deque");
        dq.f->pushBack(&dq, tmp);
        assert(!aErrOccur());
    }
    assert(dq.f->getNumber(&dq) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AStr)) dq2 = A_COPY(ADeque(AStr), dq);
    AStr *ps2 = dq2.f->at(&dq2, 0);
    AStr_pushBack(ps2, 'Z');
    AStr *ps1 = dq.f->at(&dq, 0);
    assert(strcmp(ps1->s, "deque") == 0);
    assert(strcmp(ps2->s, "dequeZ") == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AStr)) empty = A_INIT(ADeque(AStr));
    empty.f->popBack(&empty, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(ADeque(AStr)) big = A_INIT(ADeque(AStr));
    for (int i = 0; i < 600; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "dq%d", i);
        RAII(AStr) tmp = AStr_new(buf);
        RAII(AStr) elem = A_INIT(AStr);
        AStr_addBack(&elem, tmp.s);
        big.f->pushBack(&big, elem);
        assert(!aErrOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        AStr *ps = big.f->at(&big, i);
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
