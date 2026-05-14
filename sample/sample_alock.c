/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib lock sample
 */

#include <alib/alib.h>
#include <alib/alock.h>
#include <stdio.h>
#include <threads.h>

enum {
    WORKER_COUNT = 4,
    ITERATIONS_PER_WORKER = 25000,
};

typedef struct {
    AMtx* lock;
    int* counter;
    int iterations;
} CounterTask;

static int print_exc(const char* step) {
    if (!aExcOccur()) {
        return 0;
    }

    fprintf(stderr, "%s failed, aExc=%d\n", step, aExcGet());
    return -1;
}

static int counter_worker(void* arg) {
    CounterTask* task = arg;

    for (int i = 0; i < task->iterations; ++i) {
        RAII(AAutoKey) key = AMtx_lock(task->lock);
        if (aExcOccur()) {
            return -1;
        }

        (*task->counter)++;
    }

    return 0;
}

static int recursive_sum(ARecursion* lock, int n) {
    RAII(AAutoKey) key = ARecursion_lock(lock);
    if (aExcOccur()) {
        return -1;
    }

    if (n == 0) {
        return 0;
    }

    int tail = recursive_sum(lock, n - 1);
    if (tail < 0) {
        return tail;
    }

    return n + tail;
}

static int pair_write(AMtxRW* lock, int* left, int* right, int new_left, int new_right) {
    RAII(AAutoKey) key = AMtxRW_wlock(lock);
    if (aExcOccur()) {
        return -1;
    }

    *left = new_left;
    *right = new_right;
    return 0;
}

static int pair_read(AMtxRW* lock, const int* left, const int* right, int* out_left, int* out_right) {
    RAII(AAutoKey) key = AMtxRW_rlock(lock);
    if (aExcOccur()) {
        return -1;
    }

    *out_left = *left;
    *out_right = *right;
    return 0;
}

int main(void) {
    RAII(AMtx) counter_lock = A_INIT(AMtx);
    if (print_exc("A_INIT(AMtx)") != 0) {
        return 1;
    }

    int counter = 0;
    thrd_t workers[WORKER_COUNT];
    CounterTask tasks[WORKER_COUNT];

    for (int i = 0; i < WORKER_COUNT; ++i) {
        tasks[i] = (CounterTask){
            .lock = &counter_lock,
            .counter = &counter,
            .iterations = ITERATIONS_PER_WORKER,
        };

        if (thrd_create(&workers[i], counter_worker, &tasks[i]) != thrd_success) {
            fprintf(stderr, "thrd_create failed at worker %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < WORKER_COUNT; ++i) {
        int worker_ret = 0;
        if (thrd_join(workers[i], &worker_ret) != thrd_success || worker_ret != 0) {
            fprintf(stderr, "thrd_join failed at worker %d\n", i);
            return 1;
        }
    }

    printf("AMtx protected counter = %d (expected %d)\n",
            counter, WORKER_COUNT * ITERATIONS_PER_WORKER);

    RAII(ARecursion) recursion_lock = A_INIT(ARecursion);
    if (print_exc("A_INIT(ARecursion)") != 0) {
        return 1;
    }

    int sum = recursive_sum(&recursion_lock, 5);
    if (sum < 0 || print_exc("recursive_sum") != 0) {
        return 1;
    }
    printf("ARecursion recursive_sum(5) = %d\n", sum);

    RAII(AMtxRW) pair_lock = A_INIT(AMtxRW);
    if (print_exc("A_INIT(AMtxRW)") != 0) {
        return 1;
    }

    int left = 0;
    int right = 0;
    int snapshot_left = 0;
    int snapshot_right = 0;

    if (pair_write(&pair_lock, &left, &right, 10, 20) != 0 ||
            print_exc("AMtxRW_wlock") != 0) {
        return 1;
    }
    if (pair_read(&pair_lock, &left, &right, &snapshot_left, &snapshot_right) != 0 ||
            print_exc("AMtxRW_rlock") != 0) {
        return 1;
    }
    printf("AMtxRW snapshot #1: left=%d right=%d sum=%d\n",
            snapshot_left, snapshot_right, snapshot_left + snapshot_right);

    if (pair_write(&pair_lock, &left, &right, 7, 9) != 0 ||
            print_exc("AMtxRW_wlock") != 0) {
        return 1;
    }
    if (pair_read(&pair_lock, &left, &right, &snapshot_left, &snapshot_right) != 0 ||
            print_exc("AMtxRW_rlock") != 0) {
        return 1;
    }
    printf("AMtxRW snapshot #2: left=%d right=%d sum=%d\n",
            snapshot_left, snapshot_right, snapshot_left + snapshot_right);

    return 0;
}
