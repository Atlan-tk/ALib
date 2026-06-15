#include <alib/alib.h>
#include <alib/aline.h>
#include <alib/alist.h>
#include <alib/adeque.h>
#include <alib/asortque.h>
#include <assert.h>
#include <stdio.h>

ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));

AList_Define(int);
AList_Generate(int);
A_TYPE_REGISTER(AList(int));

ADeque_Define(int);
ADeque_Generate(int);
A_TYPE_REGISTER(ADeque(int));

ASortque_Define(int);
ASortque_Generate(int);
A_TYPE_REGISTER(ASortque(int));

static void test_aline_full_api(void) {
    RAII(ALine(int)) line = A_INIT(ALine(int));
    assert(line.f->at(&line, 0) == NULL);
    assert(!aErrOccur());

    line.f->pushFront(&line, 2);
    line.f->pushFront(&line, 1);
    line.f->pushBack(&line, 4);
    line.f->ins(&line, 2, 3);
    assert(line.f->getNumber(&line) == 4);

    const int expected0[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
        assert(*line.f->at(&line, (uint32_t)i) == expected0[i]);
    }
    assert(*line.f->at(&line, 99) == 4);

    int taken = -1;
    line.f->take(&line, 1, &taken);
    assert(taken == 2);

    const int expected1[] = {1, 3, 4};
    for (int i = 0; i < 3; ++i) {
        assert(*line.f->at(&line, (uint32_t)i) == expected1[i]);
    }

    line.f->rm(&line, 0);
    assert(*line.f->at(&line, 0) == 3);
    assert(*line.f->at(&line, 1) == 4);

    int front = -1, back = -1;
    line.f->popFront(&line, &front);
    line.f->popBack(&line, &back);
    assert(front == 3);
    assert(back == 4);
    assert(line.f->empty(&line));

    for (int i = 0; i < 5; ++i) {
        line.f->pushBack(&line, i);
    }

    int expected = 0;
    forEach(it, line) {
        assert(*it.p == expected++);
    }
    assert(expected == 5);

    expected = 4;
    forEachRev(it, line) {
        assert(*it.p == expected--);
    }
    assert(expected == -1);
}

static void test_alist_full_api(void) {
    RAII(AList(int)) list = A_INIT(AList(int));
    assert(list.f->at(&list, 0) == NULL);
    assert(!aErrOccur());

    list.f->pushFront(&list, 2);
    list.f->pushFront(&list, 1);
    list.f->pushBack(&list, 4);
    list.f->ins(&list, 2, 3);
    assert(list.f->getNumber(&list) == 4);

    const int expected0[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
        assert(*list.f->at(&list, (uint32_t)i) == expected0[i]);
    }
    assert(*list.f->at(&list, 99) == 4);

    int taken = -1;
    list.f->take(&list, 1, &taken);
    assert(taken == 2);

    const int expected1[] = {1, 3, 4};
    for (int i = 0; i < 3; ++i) {
        assert(*list.f->at(&list, (uint32_t)i) == expected1[i]);
    }

    list.f->rm(&list, 0);
    assert(*list.f->at(&list, 0) == 3);
    assert(*list.f->at(&list, 1) == 4);

    int front = -1, back = -1;
    list.f->popFront(&list, &front);
    list.f->popBack(&list, &back);
    assert(front == 3);
    assert(back == 4);
    assert(list.f->empty(&list));

    for (int i = 0; i < 5; ++i) {
        list.f->pushBack(&list, i);
    }

    int expected = 0;
    forEach(it, list) {
        assert(*it.p == expected++);
    }
    assert(expected == 5);

    expected = 4;
    forEachRev(it, list) {
        assert(*it.p == expected--);
    }
    assert(expected == -1);
}

static void test_adeque_full_api(void) {
    RAII(ADeque(int)) deq = A_INIT(ADeque(int));
    assert(deq.f->at(&deq, 0) == NULL);
    assert(!aErrOccur());

    deq.f->pushFront(&deq, 2);
    deq.f->pushFront(&deq, 1);
    deq.f->pushBack(&deq, 3);
    deq.f->pushBack(&deq, 4);
    assert(deq.f->getNumber(&deq) == 4);

    const int expected0[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
        assert(*deq.f->at(&deq, (uint32_t)i) == expected0[i]);
    }
    assert(*deq.f->at(&deq, 99) == 4);

    int expected = 1;
    forEach(it, deq) {
        assert(*it.p == expected++);
    }
    assert(expected == 5);

    expected = 4;
    forEachRev(it, deq) {
        assert(*it.p == expected--);
    }
    assert(expected == 0);

    int front = -1, back = -1;
    deq.f->popFront(&deq, &front);
    deq.f->popBack(&deq, &back);
    assert(front == 1);
    assert(back == 4);
    assert(*deq.f->at(&deq, 0) == 2);
    assert(*deq.f->at(&deq, 1) == 3);

    RAII(ADeque(int)) cp = A_COPY(ADeque(int), deq);
    *cp.f->at(&cp, 0) = 99;
    assert(*deq.f->at(&deq, 0) == 2);
    assert(*cp.f->at(&cp, 0) == 99);
}

static void test_asortque_full_api(void) {
    RAII(ASortque(int)) sq = A_INIT(ASortque(int));
    assert(sq.f->at(&sq, 0) == NULL);
    assert(!aErrOccur());

    sq.f->ins(&sq, 4);
    sq.f->ins(&sq, 1);
    sq.f->ins(&sq, 3);
    sq.f->ins(&sq, 2);
    assert(sq.f->getNumber(&sq) == 4);

    const int expected0[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
        assert(*sq.f->at(&sq, (uint32_t)i) == expected0[i]);
    }
    assert(*sq.f->at(&sq, 99) == 4);

    int taken = -1;
    sq.f->take(&sq, 1, &taken);
    assert(taken == 2);

    int minv = -1, maxv = -1;
    sq.f->popMin(&sq, &minv);
    sq.f->popMax(&sq, &maxv);
    assert(minv == 1);
    assert(maxv == 4);
    assert(sq.f->getNumber(&sq) == 1);
    assert(*sq.f->at(&sq, 0) == 3);

    sq.f->ins(&sq, 2);
    sq.f->ins(&sq, 5);
    sq.f->ins(&sq, 1);
    const int expected1[] = {1, 2, 3, 5};
    for (int i = 0; i < 4; ++i) {
        assert(*sq.f->at(&sq, (uint32_t)i) == expected1[i]);
    }

    int expected = 1;
    forEach(it, sq) {
        assert(*it.p == expected1[expected - 1]);
        expected++;
    }
    assert(expected == 5);

    const int expected_rev[] = {5, 3, 2, 1};
    int idx = 0;
    forEachRev(it, sq) {
        assert(*it.p == expected_rev[idx++]);
    }
    assert(idx == 4);

    sq.f->rm(&sq, 1);
    const int expected2[] = {1, 3, 5};
    for (int i = 0; i < 3; ++i) {
        assert(*sq.f->at(&sq, (uint32_t)i) == expected2[i]);
    }
}

int main(void) {
    test_aline_full_api();
    test_alist_full_api();
    test_adeque_full_api();
    test_asortque_full_api();
    printf("All sequence API tests passed.\n");
    return 0;
}
