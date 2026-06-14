
#include <alib/alib.h>
#include <alib/asortque.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

/* 生成 int 和 AStr 的有序数组实现 */
ASortque_Define(int);
ASortque_Generate(int);
A_TYPE_REGISTER(ASortque(int));

ASortque_Define(AStr);
ASortque_Generate(AStr);
A_TYPE_REGISTER(ASortque(AStr));

/* ---------- int 元素测试 ---------- */
static void test_asortque_int(void) {
    /* 1. 基本构造与空容器 */
    RAII(ASortque(int)) sq = A_INIT(ASortque(int));
    assert(!aExcOccur());
    assert(sq.f->empty(&sq));
    assert((int)sq.f->getNumber(&sq) == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 2. 插入元素（乱序） */
    int values[] = {5, 2, 8, 1, 9, 3};
    int n = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < n; i++) {
        sq.f->ins(&sq, values[i]);
        assert(!aExcOccur());
    }
    assert((int)sq.f->getNumber(&sq) == n);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 3. 按索引访问，验证升序 */
    for (int i = 0; i < n; i++) {
        int *p = sq.f->at(&sq, i);
        assert(p != NULL);
        if (i > 0) {
            assert(sq.f->at(&sq, i - 1)[0] <= *p);
        }
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 超出索引的访问：ASortque 将 index 截断到 num-1，不会触发异常 */
    int *last = sq.f->at(&sq, n + 10);
    assert(last != NULL);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 4. 空容器越界行为：at 返回 NULL 并设置异常 */
    RAII(ASortque(int)) empty_sq = A_INIT(ASortque(int));
    int *null_p = empty_sq.f->at(&empty_sq, 0);
    assert(null_p == NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* popMin/popMax 在空容器上应设置异常 */
    int tmp = 0;
    empty_sq.f->popMin(&empty_sq, &tmp);
    assert(aExcOccur());
    aExcClean();
    empty_sq.f->popMax(&empty_sq, &tmp);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 5. 拷贝构造 */
    RAII(ASortque(int)) sq2 = A_COPY(ASortque(int), sq);
    assert(!aExcOccur());
    assert((int)sq2.f->getNumber(&sq2) == n);
    for (int i = 0; i < n; i++) {
        assert(*sq2.f->at(&sq2, i) == *sq.f->at(&sq, i));
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 修改拷贝不应对原容器产生影响 */
    *sq2.f->at(&sq2, 0) = 999;
    assert(*sq.f->at(&sq, 0) != 999);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 6. 删除元素 */
    int before_rm = (int)sq.f->getNumber(&sq);
    sq.f->rm(&sq, 0);  // 删除最小元素
    assert(!aExcOccur());
    assert((int)sq.f->getNumber(&sq) == before_rm - 1);
    /* 新最小应 >= 原来次小 */
    assert(*sq.f->at(&sq, 0) >= 2);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 7. popMin / popMax */
    int min_val = 0;
    sq.f->popMin(&sq, &min_val);
    assert(!aExcOccur());
    assert(min_val <= *sq.f->at(&sq, 0)); // 删掉了当前最小，min_val应<=剩下的最小
    int max_val = 0;
    sq.f->popMax(&sq, &max_val);
    assert(!aExcOccur());
    assert(max_val >= *sq.f->at(&sq, (int)sq.f->getNumber(&sq) - 1));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 8. 迭代器遍历（应保持升序） */
    int prev = *sq.f->at(&sq, 0);
    forEach(it, sq) {
        assert(*it.p >= prev);
        prev = *it.p;
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 9. 大量数据测试 (>512) */
    RAII(ASortque(int)) big = A_INIT(ASortque(int));
    const int BIG_N = 600;
    for (int i = 0; i < BIG_N; i++) {
        big.f->ins(&big, BIG_N - i);  // 降序插入
        assert(!aExcOccur());
    }
    assert((int)big.f->getNumber(&big) == BIG_N);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
    /* 最终应为升序 1..600 */
    int cnt = 0;
    forEach(it, big) {
        assert(*it.p == cnt + 1);
        cnt++;
    }
    assert(cnt == BIG_N);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

/* ---------- AStr 元素测试 ---------- */
static void test_asortque_astring(void) {
    /* 1. 基本构造 */
    RAII(ASortque(AStr)) sq = A_INIT(ASortque(AStr));
    assert(!aExcOccur());
    assert(sq.f->empty(&sq));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 2. 插入字符串（乱序），应自动按字典序排序 */
    char* words[] = {"banana", "apple", "cherry", "blueberry", "apricot"};
    int n = sizeof(words) / sizeof(words[0]);
    for (int i = 0; i < n; i++) {
        RAII(AStr) elem = AStr_new(words[i]);  // 字面量包装
        sq.f->ins(&sq, elem);
        assert(!aExcOccur());
    }
    assert((int)sq.f->getNumber(&sq) == n);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 验证排序：接口返回的 AStr 指针所指向的字符串应非降序 */
    for (int i = 1; i < n; i++) {
        AStr *a = sq.f->at(&sq, i - 1);
        AStr *b = sq.f->at(&sq, i);
        assert(a != NULL && b != NULL);
        assert(strcmp(a->s, b->s) <= 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 空容器 at 触发异常 */
    RAII(ASortque(AStr)) empty_sq = A_INIT(ASortque(AStr));
    AStr *null_s = empty_sq.f->at(&empty_sq, 0);
    assert(null_s == NULL);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 空容器 popMin/popMax 触发异常 */
    AStr dummy = AStr_new("");
    empty_sq.f->popMin(&empty_sq, &dummy);
    assert(aExcOccur());
    aExcClean();
    empty_sq.f->popMax(&empty_sq, &dummy);
    assert(aExcOccur());
    aExcClean();
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 3. 拷贝构造 */
    RAII(ASortque(AStr)) sq2 = A_COPY(ASortque(AStr), sq);
    assert(!aExcOccur());
    assert((int)sq2.f->getNumber(&sq2) == n);
    for (int i = 0; i < n; i++) {
        AStr *orig = sq.f->at(&sq, i);
        AStr *copy = sq2.f->at(&sq2, i);
        assert(strcmp(orig->s, copy->s) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 4. 修改拷贝不影响原容器（AStr 写时拷贝） */
    AStr *first_copy = sq2.f->at(&sq2, 0);
    AStr_pushBack(first_copy, 'X');
    AStr *first_orig = sq.f->at(&sq, 0);
    assert(strstr(first_orig->s, "X") == NULL);
    assert(strstr(first_copy->s, "X") != NULL);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 5. 删除元素 */
    int before = (int)sq.f->getNumber(&sq);
    sq.f->rm(&sq, 0);
    assert((int)sq.f->getNumber(&sq) == before - 1);
    /* 新最小应 >= 原来的次小 */
    assert(strcmp(sq.f->at(&sq, 0)->s, "apricot") <= 0);  // 取决于排序结果，仅确保不倒退
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 6. popMin / popMax */
    AStr min_val = A_INIT(AStr);
    sq.f->popMin(&sq, &min_val);
    assert(!aExcOccur());
    assert(strcmp(min_val.s, sq.f->at(&sq, 0)->s) <= 0);
    AStr max_val = A_INIT(AStr);
    sq.f->popMax(&sq, &max_val);
    assert(!aExcOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    int last_idx = (int)sq.f->getNumber(&sq) - 1;
    assert(strcmp(max_val.s, sq.f->at(&sq, last_idx)->s) >= 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 7. 大量数据测试 (>512) */
    RAII(ASortque(AStr)) big = A_INIT(ASortque(AStr));
    const int BIG_N = 600;
    char buf[32];
    /* 降序插入编号字符串 "str599" .. "str0" */
    for (int i = BIG_N - 1; i >= 0; i--) {
        snprintf(buf, sizeof(buf), "str%d", i);
        RAII(AStr) elem = AStr_new(buf);
        big.f->ins(&big, elem);
        assert(!aExcOccur());
    }
    assert((int)big.f->getNumber(&big) == BIG_N);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
    /* 遍历验证字典序升序: "str0", "str1", ..., "str599" */
    int cnt = 0;
    forEach(it, big) {
        snprintf(buf, sizeof(buf), "str%d", cnt);
        AStr *v = it.p;
        assert(strcmp(v->s, buf) == 0);
        cnt++;
    }
    assert(cnt == BIG_N);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_asortque_int();
    test_asortque_astring();
    printf("All ASortque tests passed.\n");
    return 0;
}

