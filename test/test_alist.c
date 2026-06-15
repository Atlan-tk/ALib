#include <alib/alib.h>
#include <alib/alist.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

AList_Define(int);
AList_Generate(int);
A_TYPE_REGISTER(AList(int));

AList_Define(AStr);
AList_Generate(AStr);
A_TYPE_REGISTER(AList(AStr));

static void install_tracked_allocators(void) {
}

static void assert_int_list_eq(const AList(int) *list, const int *expected, uint32_t n) {
    assert(list->f->getNumber(list) == n);
    for (uint32_t i = 0; i < n; i++) {
        int *p = list->f->at(list, i);
        assert(p != NULL);
        assert(*p == expected[i]);
    }
}

static void assert_astring_list_eq(const AList(AStr) *list, const char *const *expected, uint32_t n) {
    assert(list->f->getNumber(list) == n);
    for (uint32_t i = 0; i < n; i++) {
        AStr *p = list->f->at(list, i);
        assert(p != NULL);
        assert(strcmp(p->s, expected[i]) == 0);
    }
}

static AStr make_owned_astring(const char *s) {
    AStr out = A_INIT(AStr);
    AStr literal = AStr_new((char *)s);
    AStr_addBack(&out, literal.s);
    return out;
}

static void test_alist_int(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aErrOccur());
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        list.f->pushBack(&list, i);
        assert(!aErrOccur());
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
    assert(!aErrOccur());
    assert(list2.f->getNumber(&list2) == 10);
    *list2.f->at(&list2, 0) = 999;
    assert(*list.f->at(&list, 0) == 0);
    assert(*list2.f->at(&list2, 0) == 999);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->popBack(&list, NULL);
    assert(!aErrOccur());
    assert(list.f->getNumber(&list) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(int)) empty = A_INIT(AList(int));
    empty.f->popBack(&empty, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(int)) big = A_INIT(AList(int));
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
}

static void test_alist_rm_p(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aErrOccur());

    for (int i = 0; i < 5; i++) {
        list.f->pushBack(&list, i);
        assert(!aErrOccur());
    }

    int *mid = list.f->at(&list, 2);
    list.f->rm_p(&list, mid);
    assert(!aErrOccur());
    {
        const int expected[] = {0, 1, 3, 4};
        assert_int_list_eq(&list, expected, 4);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int *head = list.f->at(&list, 0);
    list.f->rm_p(&list, head);
    assert(!aErrOccur());
    {
        const int expected[] = {1, 3, 4};
        assert_int_list_eq(&list, expected, 3);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int *tail = list.f->at(&list, list.f->getNumber(&list) - 1);
    list.f->rm_p(&list, tail);
    assert(!aErrOccur());
    {
        const int expected[] = {1, 3};
        assert_int_list_eq(&list, expected, 2);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->rm_p(&list, list.f->at(&list, 0));
    assert(!aErrOccur());
    {
        const int expected[] = {3};
        assert_int_list_eq(&list, expected, 1);
    }

    list.f->rm_p(&list, list.f->at(&list, 0));
    assert(!aErrOccur());
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->rm_p(&list, NULL);
    assert(aErrGet() == AERR_overstep);
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_take_p(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aErrOccur());

    for (int i = 0; i < 5; i++) {
        list.f->pushBack(&list, i);
        assert(!aErrOccur());
    }

    int taken = -1;
    list.f->take_p(&list, list.f->at(&list, 2), &taken);
    assert(!aErrOccur());
    assert(taken == 2);
    {
        const int expected[] = {0, 1, 3, 4};
        assert_int_list_eq(&list, expected, 4);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aErrOccur());
    assert(taken == 0);
    {
        const int expected[] = {1, 3, 4};
        assert_int_list_eq(&list, expected, 3);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->take_p(&list, list.f->at(&list, list.f->getNumber(&list) - 1), NULL);
    assert(!aErrOccur());
    {
        const int expected[] = {1, 3};
        assert_int_list_eq(&list, expected, 2);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aErrOccur());
    assert(taken == 1);
    {
        const int expected[] = {3};
        assert_int_list_eq(&list, expected, 1);
    }

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aErrOccur());
    assert(taken == 3);
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = 1234;
    list.f->take_p(&list, NULL, &taken);
    assert(aErrGet() == AERR_overstep);
    assert(taken == 0);
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_astring(void) {
    RAII(AList(AStr)) list = A_INIT(AList(AStr));
    assert(!aErrOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 10; i++) {
        RAII(AStr) tmp = AStr_new("hello");
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    assert(list.f->getNumber(&list) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AStr)) list2 = A_COPY(AList(AStr), list);
    assert(!aErrOccur());
    AStr *ps2 = list2.f->at(&list2, 0);
    AStr_pushBack(ps2, '!');
    AStr *ps1 = list.f->at(&list, 0);
    assert(strcmp(ps1->s, "hello") == 0);
    assert(strcmp(ps2->s, "hello!") == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AStr)) empty = A_INIT(AList(AStr));
    empty.f->popBack(&empty, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    RAII(AList(AStr)) big = A_INIT(AList(AStr));
    for (int i = 0; i < 600; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "item%d", i);
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
        snprintf(expected, sizeof(expected), "item%d", i);
        assert(strcmp(ps->s, expected) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_astring_rm_p(void) {
    RAII(AList(AStr)) list = A_INIT(AList(AStr));
    assert(!aErrOccur());

    {
        RAII(AStr) tmp = make_owned_astring("alpha");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    {
        RAII(AStr) tmp = make_owned_astring("beta");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    {
        RAII(AStr) tmp = make_owned_astring("gamma");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }

    {
        AStr *mid = list.f->at(&list, 1);
        list.f->rm_p(&list, mid);
        assert(!aErrOccur());
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AStr *head = list.f->at(&list, 0);
        list.f->rm_p(&list, head);
        assert(!aErrOccur());
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AStr literal = AStr_new("literal");
        list.f->pushBack(&list, literal);
        assert(!aErrOccur());
    }
    {
        AStr *tail = list.f->at(&list, 1);
        list.f->rm_p(&list, tail);
        assert(!aErrOccur());
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        RAII(AStr) tmp = make_owned_astring("delta");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    {
        AStr *tail = list.f->at(&list, 1);
        list.f->rm_p(&list, tail);
        assert(!aErrOccur());
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AStr *last = list.f->at(&list, 0);
        list.f->rm_p(&list, last);
        assert(!aErrOccur());
        assert(list.f->empty(&list));
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_astring_take_p(void) {
    RAII(AList(AStr)) list = A_INIT(AList(AStr));
    assert(!aErrOccur());

    {
        RAII(AStr) tmp = make_owned_astring("alpha");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    {
        RAII(AStr) tmp = make_owned_astring("beta");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }
    {
        RAII(AStr) tmp = make_owned_astring("gamma");
        assert(!aErrOccur());
        list.f->pushBack(&list, tmp);
        assert(!aErrOccur());
    }

    {
        RAII(AStr) taken = A_INIT(AStr);
        list.f->take_p(&list, list.f->at(&list, 1), &taken);
        assert(!aErrOccur());
        assert(strcmp(taken.s, "beta") == 0);
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AStr literal = AStr_new("literal");
        list.f->pushBack(&list, literal);
        assert(!aErrOccur());
    }
    {
        RAII(AStr) taken = A_INIT(AStr);
        list.f->take_p(&list, list.f->at(&list, 2), &taken);
        assert(!aErrOccur());
        assert(strcmp(taken.s, "literal") == 0);
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        list.f->take_p(&list, list.f->at(&list, 0), NULL);
        assert(!aErrOccur());
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        RAII(AStr) taken = A_INIT(AStr);
        list.f->take_p(&list, list.f->at(&list, 0), &taken);
        assert(!aErrOccur());
        assert(strcmp(taken.s, "gamma") == 0);
        assert(list.f->empty(&list));
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        RAII(AStr) taken = A_INIT(AStr);
        list.f->take_p(&list, NULL, &taken);
        assert(aErrGet() == AERR_overstep);
        assert(taken.s == NULL);
        aTry((void)0;)aExc{}
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    install_tracked_allocators();
    test_alist_int();
    test_alist_rm_p();
    test_alist_take_p();
    test_alist_astring();
    test_alist_astring_rm_p();
    test_alist_astring_take_p();
    printf("All AList tests passed.\n");
    return 0;
}
