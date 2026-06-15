#include <alib/alib.h>
#include <alib/astack.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

AStack_Define(int);
AStack_Generate(int);
A_TYPE_REGISTER(AStack(int));

AStack_Define(AStr);
AStack_Generate(AStr);
A_TYPE_REGISTER(AStack(AStr));

static AStr owned_string(const char *s) {
    AStr out = A_INIT(AStr);
    AStr lit = AStr_new((char *)s);
    AStr_addBack(&out, lit.s);
    return out;
}

static void test_astack_int(void) {
    RAII(AStack(int)) st = A_INIT(AStack(int));
    assert(st.f->empty(&st));
    assert(st.f->getNumber(&st) == 0);

    assert(st.f->at(&st, 0) == NULL);
    assert(!aErrOccur());

    for (int i = 1; i <= 5; ++i) {
        st.f->push(&st, i);
        assert(!aErrOccur());
    }

    assert(!st.f->empty(&st));
    assert(st.f->getNumber(&st) == 5);
    assert(*st.f->at(&st, 0) == 1);
    assert(*st.f->at(&st, 4) == 5);
    assert(*st.f->at(&st, 99) == 5);

    int expected = 1;
    forEach(it, st) {
        assert(*it.p == expected++);
    }
    assert(expected == 6);

    expected = 5;
    forEachRev(it, st) {
        assert(*it.p == expected--);
    }
    assert(expected == 0);

    RAII(AStack(int)) cp = A_COPY(AStack(int), st);
    assert(cp.f->getNumber(&cp) == 5);
    *cp.f->at(&cp, 4) = 99;
    assert(*st.f->at(&st, 4) == 5);
    assert(*cp.f->at(&cp, 4) == 99);

    int out = -1;
    st.f->pop(&st, &out);
    assert(out == 5);
    assert(st.f->getNumber(&st) == 4);

    st.f->pop(&st, NULL);
    assert(st.f->getNumber(&st) == 3);
    assert(*st.f->at(&st, 2) == 3);

    st.f->pop(&st, NULL);
    st.f->pop(&st, NULL);
    st.f->pop(&st, NULL);
    assert(st.f->empty(&st));

    st.f->pop(&st, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
}

static void test_astack_astring(void) {
    RAII(AStack(AStr)) st = A_INIT(AStack(AStr));
    assert(st.f->empty(&st));

    RAII(AStr) one = owned_string("one");
    RAII(AStr) two = owned_string("two");
    st.f->push(&st, one);
    st.f->push(&st, two);
    assert(st.f->getNumber(&st) == 2);

    RAII(AStack(AStr)) cp = A_COPY(AStack(AStr), st);
    AStr *top_copy = cp.f->at(&cp, 1);
    AStr_pushBack(top_copy, '!');
    assert(strcmp(st.f->at(&st, 1)->s, "two") == 0);
    assert(strcmp(cp.f->at(&cp, 1)->s, "two!") == 0);

    RAII(AStr) popped = A_INIT(AStr);
    st.f->pop(&st, &popped);
    assert(strcmp(popped.s, "two") == 0);
    assert(strcmp(st.f->at(&st, 0)->s, "one") == 0);
}

int main(void) {
    test_astack_int();
    test_astack_astring();
    printf("All AStack tests passed.\n");
    return 0;
}
