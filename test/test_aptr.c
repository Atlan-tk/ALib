#include <alib/alib.h>
#include <alib/aptr.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

APtr_Define(int);
APtr_Generate(int);
A_TYPE_REGISTER(APtr(int));

APtr_Define(AStr);
APtr_Generate(AStr);
A_TYPE_REGISTER(APtr(AStr));

AShPtr_Define(int);
AShPtr_Generate(int);
A_TYPE_REGISTER(AShPtr(int));

AShPtr_Define(AStr);
AShPtr_Generate(AStr);
A_TYPE_REGISTER(AShPtr(AStr));

static void test_aptr_int(void) {
    RAII(APtr(int)) p = APtrNew(int);
    assert(!aExcOccur());
    assert(p.p != NULL);
    assert(p.strong_flag);
    assert(*p.p == 0);

    RAII(APtr(int)) cp = APtrCPNew(int, 7);
    assert(!aExcOccur());
    assert(cp.p != NULL);
    assert(cp.strong_flag);
    assert(*cp.p == 7);

    RAII(APtr(int)) weak = A_COPY(APtr(int), cp);
    assert(!aExcOccur());
    assert(weak.p == cp.p);
    assert(!weak.strong_flag);

    *weak.p = 19;
    assert(*cp.p == 19);

    A_DEST(APtr(int), weak);
    assert(weak.p == NULL);
    assert(!weak.strong_flag);
    assert(*cp.p == 19);
}

static void test_aptr_astring(void) {
    RAII(AStr) src = AStr_new("hello");
    RAII(APtr(AStr)) p = APtrCPNew(AStr, src);

    assert(!aExcOccur());
    assert(p.p != NULL);
    assert(p.strong_flag);
    assert(strcmp(p.p->s, "hello") == 0);

    AStr_pushBack(&src, '!');
    assert(!aExcOccur());
    assert(strcmp(src.s, "hello!") == 0);
    assert(strcmp(p.p->s, "hello") == 0);

    AStr_pushBack(p.p, '?');
    assert(!aExcOccur());
    assert(strcmp(p.p->s, "hello?") == 0);
    assert(strcmp(src.s, "hello!") == 0);
}

static void test_ashptr_int(void) {
    RAII(AShPtr(int)) owner = AShPtrNew(int);
    assert(!aExcOccur());
    assert(owner.p != NULL);
    assert(*owner.p == 0);

    *owner.p = 11;

    RAII(AShPtr(int)) alias = A_COPY(AShPtr(int), owner);
    assert(!aExcOccur());
    assert(alias.p == owner.p);
    assert(*alias.p == 11);

    *alias.p = 23;
    assert(*owner.p == 23);

    A_DEST(AShPtr(int), alias);
    assert(alias.p == NULL);
    assert(*owner.p == 23);

    RAII(AShPtr(int)) cp = AShPtrCPNew(int, 7);
    assert(!aExcOccur());
    assert(cp.p != NULL);
    assert(*cp.p == 7);
}

static void test_ashptr_astring(void) {
    RAII(AStr) src = AStr_new("share");
    RAII(AShPtr(AStr)) owner = AShPtrCPNew(AStr, src);

    assert(!aExcOccur());
    assert(owner.p != NULL);
    assert(strcmp(owner.p->s, "share") == 0);

    AStr_pushBack(&src, '!');
    assert(!aExcOccur());
    assert(strcmp(src.s, "share!") == 0);
    assert(strcmp(owner.p->s, "share") == 0);

    RAII(AShPtr(AStr)) alias = A_COPY(AShPtr(AStr), owner);
    assert(!aExcOccur());
    assert(alias.p == owner.p);

    AStr_pushBack(alias.p, '?');
    assert(!aExcOccur());
    assert(strcmp(alias.p->s, "share?") == 0);
    assert(strcmp(owner.p->s, "share?") == 0);
    assert(strcmp(src.s, "share!") == 0);
}

int main(void) {
    test_aptr_int();
    test_aptr_astring();
    test_ashptr_int();
    test_ashptr_astring();
    printf("All APtr/AShPtr tests passed.\n");
    return 0;
}
