#include <alib/alib.h>
#include <alib/alist.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

AList_Define(int);
AList_Generate(int);
A_TYPE_REGISTER(AList(int));

AList_Define(AString);
AList_Generate(AString);
A_TYPE_REGISTER(AList(AString));

static uint32_t g_alib_free_calls = 0;

static void tracked_alib_free(void *p) {
    if (p != NULL) {
        g_alib_free_calls++;
    }
    free(p);
}

static void *tracked_alib_alloc(uint32_t size) {
    return malloc(size);
}

static void *tracked_alib_realloc(void *p, uint32_t size) {
    return realloc(p, size);
}

void alib_free(void *p) {
    tracked_alib_free(p);
}

void *alib_alloc(uint32_t size) {
    return tracked_alib_alloc(size);
}

void *alib_realloc(void *p, uint32_t size) {
    return tracked_alib_realloc(p, size);
}

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

static void assert_astring_list_eq(const AList(AString) *list, const char *const *expected, uint32_t n) {
    assert(list->f->getNumber(list) == n);
    for (uint32_t i = 0; i < n; i++) {
        AString *p = list->f->at(list, i);
        assert(p != NULL);
        assert(strcmp(p->s, expected[i]) == 0);
    }
}

static AString make_owned_astring(const char *s) {
    AString out = A_INIT(AString);
    AString literal = AString_new((char *)s);
    out.f->addBack(&out, literal);
    return out;
}

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

static void test_alist_rm_p(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aExcOccur());

    for (int i = 0; i < 5; i++) {
        list.f->pushBack(&list, i);
        assert(!aExcOccur());
    }

    int *mid = list.f->at(&list, 2);
    list.f->rm_p(&list, mid);
    assert(!aExcOccur());
    {
        const int expected[] = {0, 1, 3, 4};
        assert_int_list_eq(&list, expected, 4);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int *head = list.f->at(&list, 0);
    list.f->rm_p(&list, head);
    assert(!aExcOccur());
    {
        const int expected[] = {1, 3, 4};
        assert_int_list_eq(&list, expected, 3);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int *tail = list.f->at(&list, list.f->getNumber(&list) - 1);
    list.f->rm_p(&list, tail);
    assert(!aExcOccur());
    {
        const int expected[] = {1, 3};
        assert_int_list_eq(&list, expected, 2);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->rm_p(&list, list.f->at(&list, 0));
    assert(!aExcOccur());
    {
        const int expected[] = {3};
        assert_int_list_eq(&list, expected, 1);
    }

    list.f->rm_p(&list, list.f->at(&list, 0));
    assert(!aExcOccur());
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->rm_p(&list, NULL);
    assert(aExcGet() == AEXC_overstep);
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_take_p(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(!aExcOccur());

    for (int i = 0; i < 5; i++) {
        list.f->pushBack(&list, i);
        assert(!aExcOccur());
    }

    int taken = -1;
    list.f->take_p(&list, list.f->at(&list, 2), &taken);
    assert(!aExcOccur());
    assert(taken == 2);
    {
        const int expected[] = {0, 1, 3, 4};
        assert_int_list_eq(&list, expected, 4);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aExcOccur());
    assert(taken == 0);
    {
        const int expected[] = {1, 3, 4};
        assert_int_list_eq(&list, expected, 3);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    list.f->take_p(&list, list.f->at(&list, list.f->getNumber(&list) - 1), NULL);
    assert(!aExcOccur());
    {
        const int expected[] = {1, 3};
        assert_int_list_eq(&list, expected, 2);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aExcOccur());
    assert(taken == 1);
    {
        const int expected[] = {3};
        assert_int_list_eq(&list, expected, 1);
    }

    taken = -1;
    list.f->take_p(&list, list.f->at(&list, 0), &taken);
    assert(!aExcOccur());
    assert(taken == 3);
    assert(list.f->empty(&list));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    taken = 1234;
    list.f->take_p(&list, NULL, &taken);
    assert(aExcGet() == AEXC_overstep);
    assert(taken == 0);
    aExcClean();
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

static void test_alist_astring_rm_p(void) {
    RAII(AList(AString)) list = A_INIT(AList(AString));
    assert(!aExcOccur());

    {
        RAII(AString) tmp = make_owned_astring("alpha");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    {
        RAII(AString) tmp = make_owned_astring("beta");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    {
        RAII(AString) tmp = make_owned_astring("gamma");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }

    {
        uint32_t frees_before = g_alib_free_calls;
        AString *mid = list.f->at(&list, 1);
        list.f->rm_p(&list, mid);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 2);
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        uint32_t frees_before = g_alib_free_calls;
        AString *head = list.f->at(&list, 0);
        list.f->rm_p(&list, head);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 2);
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AString literal = AString_new("literal");
        uint32_t frees_before_push = g_alib_free_calls;
        list.f->pushBack(&list, literal);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before_push);
    }
    {
        uint32_t frees_before = g_alib_free_calls;
        AString *tail = list.f->at(&list, 1);
        list.f->rm_p(&list, tail);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 1);
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        RAII(AString) tmp = make_owned_astring("delta");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    {
        uint32_t frees_before = g_alib_free_calls;
        AString *tail = list.f->at(&list, 1);
        list.f->rm_p(&list, tail);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 2);
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        uint32_t frees_before = g_alib_free_calls;
        AString *last = list.f->at(&list, 0);
        list.f->rm_p(&list, last);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 2);
        assert(list.f->empty(&list));
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

static void test_alist_astring_take_p(void) {
    RAII(AList(AString)) list = A_INIT(AList(AString));
    assert(!aExcOccur());

    {
        RAII(AString) tmp = make_owned_astring("alpha");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    {
        RAII(AString) tmp = make_owned_astring("beta");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }
    {
        RAII(AString) tmp = make_owned_astring("gamma");
        assert(!aExcOccur());
        list.f->pushBack(&list, tmp);
        assert(!aExcOccur());
    }

    {
        uint32_t frees_before = g_alib_free_calls;
        RAII(AString) taken = A_INIT(AString);
        list.f->take_p(&list, list.f->at(&list, 1), &taken);
        assert(!aExcOccur());
        assert(strcmp(taken.s, "beta") == 0);
        assert(g_alib_free_calls == frees_before + 1);
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        AString literal = AString_new("literal");
        list.f->pushBack(&list, literal);
        assert(!aExcOccur());
    }
    {
        uint32_t frees_before = g_alib_free_calls;
        RAII(AString) taken = A_INIT(AString);
        list.f->take_p(&list, list.f->at(&list, 2), &taken);
        assert(!aExcOccur());
        assert(strcmp(taken.s, "literal") == 0);
        assert(g_alib_free_calls == frees_before + 1);
        {
            const char *expected[] = {"alpha", "gamma"};
            assert_astring_list_eq(&list, expected, 2);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        uint32_t frees_before = g_alib_free_calls;
        list.f->take_p(&list, list.f->at(&list, 0), NULL);
        assert(!aExcOccur());
        assert(g_alib_free_calls == frees_before + 2);
        {
            const char *expected[] = {"gamma"};
            assert_astring_list_eq(&list, expected, 1);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        uint32_t frees_before = g_alib_free_calls;
        RAII(AString) taken = A_INIT(AString);
        list.f->take_p(&list, list.f->at(&list, 0), &taken);
        assert(!aExcOccur());
        assert(strcmp(taken.s, "gamma") == 0);
        assert(g_alib_free_calls == frees_before + 1);
        assert(list.f->empty(&list));
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    {
        RAII(AString) taken = A_INIT(AString);
        uint32_t frees_before = g_alib_free_calls;
        list.f->take_p(&list, NULL, &taken);
        assert(aExcGet() == AEXC_overstep);
        assert(taken.s == NULL);
        assert(g_alib_free_calls == frees_before);
        aExcClean();
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
