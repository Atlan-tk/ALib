#include <alib/alib.h>
#include <alib/aline.h>
#include <alib/astring.h>
#include <assert.h>
#include <stdio.h>

/* 生成 int 和 AStr 的 ALine 实现 */
ALine_Define(int);
ALine_Generate(int);
A_TYPE_REGISTER(ALine(int));

ALine_Define(AStr);
ALine_Generate(AStr);
A_TYPE_REGISTER(ALine(AStr));

/* 测试 int 元素 */
static void test_aline_int(void) {
    /* 基本构造与空容器 */
    RAII(ALine(int)) line = A_INIT(ALine(int));
    assert(!aErrOccur());
    assert(line.f->empty(&line));
    assert(line.f->getNumber(&line) == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 插入元素 */
    for (int i = 0; i < 10; i++) {
        line.f->pushBack(&line, i);
        assert(!aErrOccur());
    }
    assert(line.f->getNumber(&line) == 10);
    assert(!line.f->empty(&line));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 随机访问 */
    for (int i = 0; i < 10; i++) {
        int *p = line.f->at(&line, i);
        assert(p != NULL);
        assert(*p == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 拷贝构造 */
    RAII(ALine(int)) line2 = A_COPY(ALine(int), line);
    assert(!aErrOccur());
    assert(line2.f->getNumber(&line2) == 10);
    for (int i = 0; i < 10; i++) {
        assert(*line2.f->at(&line2, i) == i);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 修改拷贝不应对原容器产生影响 */
    *line2.f->at(&line2, 0) = 999;
    assert(*line.f->at(&line, 0) == 0);
    assert(*line2.f->at(&line2, 0) == 999);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 删除元素 */
    line.f->popBack(&line, NULL);
    assert(!aErrOccur());
    assert(line.f->getNumber(&line) == 9);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 空容器 popBack 应产生异常 */
    RAII(ALine(int)) empty_line = A_INIT(ALine(int));
    empty_line.f->popBack(&empty_line, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据（>512） */
    RAII(ALine(int)) big = A_INIT(ALine(int));
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

/* 测试 AStr 元素 */
static void test_aline_astring(void) {
    RAII(ALine(AStr)) line = A_INIT(ALine(AStr));
    assert(!aErrOccur());
    assert(line.f->empty(&line));
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 插入普通字符串 */
    for (int i = 0; i < 10; i++) {
        RAII(AStr) tmp = AStr_new("constant");
        line.f->pushBack(&line, tmp);
        assert(!aErrOccur());
    }
    assert(line.f->getNumber(&line) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 访问并校验内容 */
    for (int i = 0; i < 10; i++) {
        AStr *ps = line.f->at(&line, i);
        assert(ps != NULL);
        assert(strcmp(ps->s, "constant") == 0);
    }

    /* 拷贝容器 */
    RAII(ALine(AStr)) line2 = A_COPY(ALine(AStr), line);
    assert(!aErrOccur());
    assert(line2.f->getNumber(&line2) == 10);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 修改拷贝容器中的元素（AStr 会触发写时拷贝） */
    AStr *ps2 = line2.f->at(&line2, 0);
    AStr_pushBack(ps2, 'X');
    assert(!aErrOccur());
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 原容器元素不应受影响 */
    AStr *ps1 = line.f->at(&line, 0);
    assert(strcmp(ps1->s, "constant") == 0);
    assert(strcmp(ps2->s, "constantX") == 0);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 空容器边界测试 */
    RAII(ALine(AStr)) empty_line = A_INIT(ALine(AStr));
    empty_line.f->popBack(&empty_line, NULL);
    assert(aErrOccur());
    aTry((void)0;)aExc{}
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    /* 大量数据（>512） */
    RAII(ALine(AStr)) big = A_INIT(ALine(AStr));
    for (int i = 0; i < 600; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "s%d", i);
        RAII(AStr) tmp = AStr_new(buf);
        RAII(AStr) elem = A_INIT(AStr);
        AStr_addBack(&elem, tmp.s);       /* 深拷贝到 elem */
        big.f->pushBack(&big, elem);
        assert(!aErrOccur());
    }
    assert(big.f->getNumber(&big) == 600);
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);

    for (int i = 0; i < 600; i++) {
        AStr *ps = big.f->at(&big, i);
        char expected[32];
        snprintf(expected, sizeof(expected), "s%d", i);
        assert(strcmp(ps->s, expected) == 0);
    }
    printf("------>>>%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

int main(void) {
    test_aline_int();
    test_aline_astring();
    printf("All ALine tests passed.\n");
    return 0;
}

