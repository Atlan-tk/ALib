/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib lock sample
 */

#include <alib/alib.h>
#include <alib/athrd.h>
#include <alib/alock.h>
#include <stdio.h>

enum {
    WORKER_COUNT = 4,
    ITERATIONS_PER_WORKER = 25000,
    READY_WORKER_COUNT = 2,
};

typedef struct {
    AMtx* lock;
    int* counter;
    int iterations;
} CounterTask;

typedef struct {
    AMtxCnd* lock;
    mtx_t state_mutex;
    int waiting;
    int woke;
    bool ready;
    int shared_value;
} ReadyTask;

typedef struct {
    ASemaphore* sem;
    mtx_t state_mutex;
    int ready;
    int acquired;
} ResizeTask;

static void sleep_ms(long ms) {
    struct timespec pause = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L,
    };

    thrd_sleep(&pause, NULL);
}

static int locked_read(mtx_t* mutex, const int* value) {
    int current = 0;

    if (mtx_lock(mutex) != thrd_success) {
        return 0;
    }
    current = *value;
    mtx_unlock(mutex);
    return current;
}

static int wait_until_at_least(mtx_t* mutex, const int* value, int expected, long timeout_ms) {
    long attempts = timeout_ms / 5;
    if (attempts < 1) {
        attempts = 1;
    }

    for (long i = 0; i < attempts; ++i) {
        if (locked_read(mutex, value) >= expected) {
            return 0;
        }
        sleep_ms(5);
    }

    return -1;
}

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

static int ready_worker(void* arg) {
    ReadyTask* task = arg;

    RAII(AAutoKey) key = AMtxCnd_lock(task->lock);
    if (aExcOccur()) {
        return -1;
    }

    if (mtx_lock(&task->state_mutex) != thrd_success) {
        return -1;
    }
    task->waiting++;
    if (mtx_unlock(&task->state_mutex) != thrd_success) {
        return -1;
    }

    while (!task->ready) {
        AMtxCnd_wait(task->lock);
        if (aExcOccur()) {
            return -1;
        }
    }

    printf("AMtxCnd worker saw shared_value=%d\n", task->shared_value);

    if (mtx_lock(&task->state_mutex) != thrd_success) {
        return -1;
    }
    task->woke++;
    if (mtx_unlock(&task->state_mutex) != thrd_success) {
        return -1;
    }

    return 0;
}

static int resize_worker(void* arg) {
    ResizeTask* task = arg;

    if (mtx_lock(&task->state_mutex) != thrd_success) {
        return -1;
    }
    task->ready = 1;
    if (mtx_unlock(&task->state_mutex) != thrd_success) {
        return -1;
    }

    RAII(AAutoKey) permit = ASemaphore_lock(task->sem);
    if (aExcOccur()) {
        return -1;
    }

    if (mtx_lock(&task->state_mutex) != thrd_success) {
        return -1;
    }
    task->acquired = 1;
    if (mtx_unlock(&task->state_mutex) != thrd_success) {
        return -1;
    }

    sleep_ms(40);
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

    RAII(AMtxCnd) ready_lock = A_INIT(AMtxCnd);
    if (print_exc("A_INIT(AMtxCnd)") != 0) {
        return 1;
    }

    ReadyTask ready_task = {
        .lock = &ready_lock,
    };
    thrd_t ready_workers[READY_WORKER_COUNT];

    if (mtx_init(&ready_task.state_mutex, mtx_plain) != thrd_success) {
        fprintf(stderr, "mtx_init failed for ReadyTask\n");
        return 1;
    }

    for (int i = 0; i < READY_WORKER_COUNT; ++i) {
        if (thrd_create(&ready_workers[i], ready_worker, &ready_task) != thrd_success) {
            fprintf(stderr, "thrd_create failed for ready worker %d\n", i);
            mtx_destroy(&ready_task.state_mutex);
            return 1;
        }
    }

    if (wait_until_at_least(&ready_task.state_mutex,
            &ready_task.waiting, READY_WORKER_COUNT, 500) != 0) {
        fprintf(stderr, "ready workers did not start waiting in time\n");
        mtx_destroy(&ready_task.state_mutex);
        return 1;
    }

    {
        RAII(AAutoKey) key = AMtxCnd_lock(&ready_lock);
        if (aExcOccur()) {
            mtx_destroy(&ready_task.state_mutex);
            return 1;
        }

        ready_task.shared_value = 42;
        ready_task.ready = true;
        AMtxCnd_awake_all(&ready_lock);
        if (print_exc("AMtxCnd_awake_all") != 0) {
            mtx_destroy(&ready_task.state_mutex);
            return 1;
        }
    }

    for (int i = 0; i < READY_WORKER_COUNT; ++i) {
        int worker_ret = 0;
        if (thrd_join(ready_workers[i], &worker_ret) != thrd_success || worker_ret != 0) {
            fprintf(stderr, "thrd_join failed for ready worker %d\n", i);
            mtx_destroy(&ready_task.state_mutex);
            return 1;
        }
    }

    printf("AMtxCnd woke %d workers with shared_value=%d\n",
            locked_read(&ready_task.state_mutex, &ready_task.woke),
            ready_task.shared_value);
    mtx_destroy(&ready_task.state_mutex);

    RAII(ASemaphore) semaphore = A_INIT(ASemaphore);
    if (print_exc("A_INIT(ASemaphore)") != 0) {
        return 1;
    }

    ResizeTask resize_task = {
        .sem = &semaphore,
    };
    thrd_t resize_thread = {0};

    if (mtx_init(&resize_task.state_mutex, mtx_plain) != thrd_success) {
        fprintf(stderr, "mtx_init failed for ResizeTask\n");
        return 1;
    }

    ASemaphore_setMax(&semaphore, 1);
    if (print_exc("ASemaphore_setMax(1)") != 0) {
        mtx_destroy(&resize_task.state_mutex);
        return 1;
    }

    {
        RAII(AAutoKey) hold = ASemaphore_lock(&semaphore);
        if (print_exc("ASemaphore_lock(initial)") != 0) {
            mtx_destroy(&resize_task.state_mutex);
            return 1;
        }

        if (thrd_create(&resize_thread, resize_worker, &resize_task) != thrd_success) {
            fprintf(stderr, "thrd_create failed for resize worker\n");
            mtx_destroy(&resize_task.state_mutex);
            return 1;
        }

        if (wait_until_at_least(&resize_task.state_mutex, &resize_task.ready, 1, 500) != 0) {
            fprintf(stderr, "resize worker did not start in time\n");
            mtx_destroy(&resize_task.state_mutex);
            return 1;
        }

        sleep_ms(20);
        printf("ASemaphore before resize: acquired=%d max=%u\n",
                locked_read(&resize_task.state_mutex, &resize_task.acquired),
                semaphore.max);

        ASemaphore_setMax(&semaphore, 2);
        if (print_exc("ASemaphore_setMax(2)") != 0) {
            mtx_destroy(&resize_task.state_mutex);
            return 1;
        }

        if (wait_until_at_least(&resize_task.state_mutex, &resize_task.acquired, 1, 500) != 0) {
            fprintf(stderr, "resize worker did not acquire permit in time\n");
            mtx_destroy(&resize_task.state_mutex);
            return 1;
        }
        printf("ASemaphore after resize: acquired=%d max=%u\n",
                locked_read(&resize_task.state_mutex, &resize_task.acquired),
                semaphore.max);

        {
            int worker_ret = 0;
            if (thrd_join(resize_thread, &worker_ret) != thrd_success || worker_ret != 0) {
                fprintf(stderr, "thrd_join failed for resize worker\n");
                mtx_destroy(&resize_task.state_mutex);
                return 1;
            }
        }
    }

    mtx_destroy(&resize_task.state_mutex);

    printf("ASemaphore resize sample completed.\n");
    return 0;
}
