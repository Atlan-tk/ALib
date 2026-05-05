#include <alib/alib.h>
#include <alib/atree.h>
#include <alib/ahash.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

ATree_Define(int, int);
ATree_Generate(int, int);
A_TYPE_REGISTER(ATree(int, int));

AHash_Define(int, int);
AHash_Generate(int, int);
A_TYPE_REGISTER(AHash(int, int));

ATree_Define(AString, AString);
ATree_Generate(AString, AString);
A_TYPE_REGISTER(ATree(AString, AString));

AHash_Define(AString, AString);
AHash_Generate(AString, AString);
A_TYPE_REGISTER(AHash(AString, AString));

static AString owned_string(const char *s) {
    AString out = A_INIT(AString);
    AString lit = AString_new((char *)s);
    out.f->addBack(&out, lit);
    return out;
}

static void test_atree_int_api(void) {
    RAII(ATree(int, int)) tr = A_INIT(ATree(int, int));
    assert(tr.f->at(&tr, 1) == NULL);
    assert(aExcOccur());
    aExcClean();

    tr.f->ins(&tr, 3, 30);
    tr.f->ins(&tr, 1, 10);
    tr.f->ins(&tr, 2, 20);
    tr.f->ins(&tr, 2, 200);
    assert(tr.f->getNumber(&tr) == 3);
    assert(*tr.f->at(&tr, 2) == 200);

    const int keys_fwd[] = {1, 2, 3};
    const int vals_fwd[] = {10, 200, 30};
    int idx = 0;
    forEach(it, tr) {
        assert(tr.f->getk(it) == keys_fwd[idx]);
        assert(*it.p == vals_fwd[idx]);
        idx++;
    }
    assert(idx == 3);

    const int keys_rev[] = {3, 2, 1};
    idx = 0;
    forEachRev(it, tr) {
        assert(tr.f->getk(it) == keys_rev[idx++]);
    }
    assert(idx == 3);

    RAII(ATree(int, int)) cp = A_COPY(ATree(int, int), tr);
    *cp.f->at(&cp, 1) = 111;
    assert(*tr.f->at(&tr, 1) == 10);
    assert(*cp.f->at(&cp, 1) == 111);

    int taken = -1;
    tr.f->take(&tr, 2, &taken);
    assert(taken == 200);
    assert(tr.f->getNumber(&tr) == 2);
    assert(tr.f->at(&tr, 2) == NULL);
    assert(aExcOccur());
    aExcClean();

    tr.f->rm(&tr, 42);
    assert(aExcOccur());
    aExcClean();
}

static void test_ahash_int_api(void) {
    RAII(AHash(int, int)) hs = A_INIT(AHash(int, int));
    assert(hs.f->at(&hs, 1) == NULL);
    assert(aExcOccur());
    aExcClean();

    hs.f->ins(&hs, 1, 10);
    hs.f->ins(&hs, 2, 20);
    hs.f->ins(&hs, 3, 30);
    hs.f->ins(&hs, 2, 200);
    assert(hs.f->getNumber(&hs) == 3);
    assert(*hs.f->at(&hs, 2) == 200);

    bool seen[4] = {false};
    int count = 0;
    int sum = 0;
    forEach(it, hs) {
        int k = hs.f->getk(it);
        int v = *it.p;
        assert(k >= 1 && k <= 3);
        assert(!seen[k]);
        seen[k] = true;
        assert(v == (k == 2 ? 200 : k * 10));
        sum += v;
        count++;
    }
    assert(count == 3);
    assert(sum == 240);
    assert(seen[1] && seen[2] && seen[3]);

    memset(seen, 0, sizeof(seen));
    count = 0;
    forEachRev(it, hs) {
        int k = hs.f->getk(it);
        assert(k >= 1 && k <= 3);
        assert(!seen[k]);
        seen[k] = true;
        count++;
    }
    assert(count == 3);
    assert(seen[1] && seen[2] && seen[3]);

    RAII(AHash(int, int)) cp = A_COPY(AHash(int, int), hs);
    *cp.f->at(&cp, 1) = 111;
    assert(*hs.f->at(&hs, 1) == 10);
    assert(*cp.f->at(&cp, 1) == 111);

    int taken = -1;
    hs.f->take(&hs, 2, &taken);
    assert(taken == 200);
    assert(hs.f->getNumber(&hs) == 2);
    assert(hs.f->at(&hs, 2) == NULL);
    assert(aExcOccur());
    aExcClean();

    hs.f->rm(&hs, 42);
    assert(aExcOccur());
    aExcClean();
}

static void test_atree_astring_override(void) {
    RAII(ATree(AString, AString)) tr = A_INIT(ATree(AString, AString));
    RAII(AString) key0 = owned_string("k");
    RAII(AString) val0 = owned_string("v0");
    RAII(AString) key1 = owned_string("k");
    RAII(AString) val1 = owned_string("v1");

    tr.f->ins(&tr, key0, val0);
    tr.f->ins(&tr, key1, val1);
    assert(tr.f->getNumber(&tr) == 1);
    assert(strcmp(tr.f->at(&tr, key1)->s, "v1") == 0);

    RAII(AString) taken = A_INIT(AString);
    tr.f->take(&tr, key1, &taken);
    assert(strcmp(taken.s, "v1") == 0);
    assert(tr.f->empty(&tr));
}

static void test_ahash_astring_override(void) {
    RAII(AHash(AString, AString)) hs = A_INIT(AHash(AString, AString));
    RAII(AString) key0 = owned_string("k");
    RAII(AString) val0 = owned_string("v0");
    RAII(AString) key1 = owned_string("k");
    RAII(AString) val1 = owned_string("v1");

    hs.f->ins(&hs, key0, val0);
    hs.f->ins(&hs, key1, val1);
    assert(hs.f->getNumber(&hs) == 1);
    assert(strcmp(hs.f->at(&hs, key1)->s, "v1") == 0);

    RAII(AString) taken = A_INIT(AString);
    hs.f->take(&hs, key1, &taken);
    assert(strcmp(taken.s, "v1") == 0);
    assert(hs.f->empty(&hs));
}

int main(void) {
    test_atree_int_api();
    test_ahash_int_api();
    test_atree_astring_override();
    test_ahash_astring_override();
    printf("All map API tests passed.\n");
    return 0;
}
