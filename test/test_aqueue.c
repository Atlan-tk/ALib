#include <alib/alib.h>
#include <alib/aqueue.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

AQueue_Define(int);
AQueue_Generate(int);
A_TYPE_REGISTER(AQueue(int));

AQueue_Define(AStr);
AQueue_Generate(AStr);
A_TYPE_REGISTER(AQueue(AStr));

static AStr owned_string(const char *s) {
    AStr out = A_INIT(AStr);
    AStr lit = AStr_new((char *)s);
    AStr_addBack(&out, lit.s);
    return out;
}

static void test_aqueue_int(void) {
    RAII(AQueue(int)) qu = A_INIT(AQueue(int));
    assert(qu.f->empty(&qu));
    assert(qu.f->getNumber(&qu) == 0);

    assert(qu.f->at(&qu, 0) == NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}

    for (int i = 1; i <= 5; ++i) {
        qu.f->push(&qu, i);
        assert(!aErrOccur());
    }

    assert(!qu.f->empty(&qu));
    assert(qu.f->getNumber(&qu) == 5);
    assert(*qu.f->at(&qu, 0) == 1);
    assert(*qu.f->at(&qu, 4) == 5);
    assert(*qu.f->at(&qu, 99) == 5);

    int out = -1;
    qu.f->pop(&qu, &out);
    assert(out == 1);
    qu.f->pop(&qu, &out);
    assert(out == 2);

    qu.f->push(&qu, 6);
    qu.f->push(&qu, 7);
    assert(qu.f->getNumber(&qu) == 5);

    const int expected_vals[] = {3, 4, 5, 6, 7};
    for (int i = 0; i < 5; ++i) {
        assert(*qu.f->at(&qu, (uint32_t)i) == expected_vals[i]);
    }

    int expected = 3;
    forEach(it, qu) {
        assert(*it.p == expected++);
    }
    assert(expected == 8);

    expected = 7;
    forEachRev(it, qu) {
        assert(*it.p == expected--);
    }
    assert(expected == 2);

    RAII(AQueue(int)) cp = A_COPY(AQueue(int), qu);
    *cp.f->at(&cp, 0) = 99;
    assert(*qu.f->at(&qu, 0) == 3);
    assert(*cp.f->at(&cp, 0) == 99);

    while (!qu.f->empty(&qu)) {
        qu.f->pop(&qu, NULL);
    }
    assert(qu.f->empty(&qu));

    qu.f->pop(&qu, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
}

static void test_aqueue_astring(void) {
    RAII(AQueue(AStr)) qu = A_INIT(AQueue(AStr));
    assert(qu.f->empty(&qu));

    RAII(AStr) one = owned_string("one");
    RAII(AStr) two = owned_string("two");
    qu.f->push(&qu, one);
    qu.f->push(&qu, two);
    assert(qu.f->getNumber(&qu) == 2);

    RAII(AQueue(AStr)) cp = A_COPY(AQueue(AStr), qu);
    AStr *front_copy = cp.f->at(&cp, 0);
    AStr_pushBack(front_copy, '!');
    assert(strcmp(qu.f->at(&qu, 0)->s, "one") == 0);
    assert(strcmp(cp.f->at(&cp, 0)->s, "one!") == 0);

    RAII(AStr) popped = A_INIT(AStr);
    qu.f->pop(&qu, &popped);
    assert(strcmp(popped.s, "one") == 0);
    assert(strcmp(qu.f->at(&qu, 0)->s, "two") == 0);
}

int main(void) {
    test_aqueue_int();
    test_aqueue_astring();
    printf("All AQueue tests passed.\n");
    return 0;
}
