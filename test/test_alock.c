#include <alib/alock.h>
#include <assert.h>
#include <stdio.h>

typedef struct {
    AMtxCnd* lock;
    mtx_t state_mutex;
    int waiting;
    int go;
    int woke;
} CndWakeState;

typedef struct {
    ASemaphore* sem;
    mtx_t state_mutex;
    int active;
    int peak_active;
    int completed;
} SemaphoreLimitState;

typedef struct {
    ASemaphore* sem;
    mtx_t state_mutex;
    int ready;
    int acquired;
} SemaphoreResizeState;

static void sleep_ms(long ms) {
    struct timespec pause = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L,
    };

    assert(thrd_sleep(&pause, NULL) == 0);
}

static int locked_read(mtx_t* mutex, const int* value) {
    int current = 0;

    assert(mtx_lock(mutex) == thrd_success);
    current = *value;
    assert(mtx_unlock(mutex) == thrd_success);
    return current;
}

static void wait_until_at_least(mtx_t* mutex, const int* value, int expected, long timeout_ms) {
    long attempts = timeout_ms / 5;
    if (attempts < 1) {
        attempts = 1;
    }

    for (long i = 0; i < attempts; ++i) {
        if (locked_read(mutex, value) >= expected) {
            return;
        }
        sleep_ms(5);
    }

    assert(0 && "timed out waiting for worker state");
}

static int cnd_wait_worker(void* arg) {
    CndWakeState* state = arg;

    RAII(AAutoKey) key = AMtxCnd_lock(state->lock);
    assert(!aExcOccur());

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->waiting++;
    assert(mtx_unlock(&state->state_mutex) == thrd_success);

    while (!state->go) {
        AMtxCnd_wait(state->lock);
        assert(!aExcOccur());
    }

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->woke++;
    assert(mtx_unlock(&state->state_mutex) == thrd_success);
    return 0;
}

static int semaphore_limit_worker(void* arg) {
    SemaphoreLimitState* state = arg;

    RAII(AAutoKey) key = ASemaphore_lock(state->sem);
    assert(!aExcOccur());

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->active++;
    if (state->active > state->peak_active) {
        state->peak_active = state->active;
    }
    assert(mtx_unlock(&state->state_mutex) == thrd_success);

    sleep_ms(60);

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->active--;
    state->completed++;
    assert(mtx_unlock(&state->state_mutex) == thrd_success);
    return 0;
}

static int semaphore_resize_worker(void* arg) {
    SemaphoreResizeState* state = arg;

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->ready = 1;
    assert(mtx_unlock(&state->state_mutex) == thrd_success);

    RAII(AAutoKey) key = ASemaphore_lock(state->sem);
    assert(!aExcOccur());

    assert(mtx_lock(&state->state_mutex) == thrd_success);
    state->acquired = 1;
    assert(mtx_unlock(&state->state_mutex) == thrd_success);
    return 0;
}

static void test_amtxcnd_nullptr_guards(void) {
    aExcClean();
    AMtxCnd_awake(NULL);
    assert(aExcGet() == AEXC_nullptr);

    aExcClean();
    AMtxCnd_awake_all(NULL);
    assert(aExcGet() == AEXC_nullptr);

    aExcClean();
    AMtxCnd_wait(NULL);
    assert(aExcGet() == AEXC_nullptr);

    aExcClean();
}

static void test_amtxcnd_awake_all(void) {
    RAII(AMtxCnd) lock = A_INIT(AMtxCnd);
    CndWakeState state = {
        .lock = &lock,
    };
    thrd_t workers[2] = {0};
    int results[2] = {0};

    assert(!aExcOccur());
    assert(mtx_init(&state.state_mutex, mtx_plain) == thrd_success);

    for (int i = 0; i < 2; ++i) {
        assert(thrd_create(&workers[i], cnd_wait_worker, &state) == thrd_success);
    }

    wait_until_at_least(&state.state_mutex, &state.waiting, 2, 500);

    {
        RAII(AAutoKey) key = AMtxCnd_lock(&lock);
        assert(!aExcOccur());
        state.go = 1;
        AMtxCnd_awake_all(&lock);
        assert(!aExcOccur());
    }

    wait_until_at_least(&state.state_mutex, &state.woke, 2, 500);

    for (int i = 0; i < 2; ++i) {
        assert(thrd_join(workers[i], &results[i]) == thrd_success);
        assert(results[i] == 0);
    }

    mtx_destroy(&state.state_mutex);
}

static void test_semaphore_limits_concurrency(void) {
    RAII(ASemaphore) sem = A_INIT(ASemaphore);
    SemaphoreLimitState state = {
        .sem = &sem,
    };
    thrd_t workers[4] = {0};
    int results[4] = {0};

    assert(!aExcOccur());
    assert(mtx_init(&state.state_mutex, mtx_plain) == thrd_success);

    ASemaphore_setMax(&sem, 2);
    assert(!aExcOccur());

    for (int i = 0; i < 4; ++i) {
        assert(thrd_create(&workers[i], semaphore_limit_worker, &state) == thrd_success);
    }

    wait_until_at_least(&state.state_mutex, &state.completed, 4, 1500);

    for (int i = 0; i < 4; ++i) {
        assert(thrd_join(workers[i], &results[i]) == thrd_success);
        assert(results[i] == 0);
    }

    assert(locked_read(&state.state_mutex, &state.active) == 0);
    assert(locked_read(&state.state_mutex, &state.peak_active) == 2);
    assert(sem.count == 0);
    assert(sem.max == 2);

    mtx_destroy(&state.state_mutex);
}

static void test_semaphore_setmax_wakes_waiter(void) {
    RAII(ASemaphore) sem = A_INIT(ASemaphore);
    SemaphoreResizeState state = {
        .sem = &sem,
    };
    thrd_t worker = {0};
    int worker_result = -1;

    assert(!aExcOccur());
    assert(mtx_init(&state.state_mutex, mtx_plain) == thrd_success);

    ASemaphore_setMax(&sem, 1);
    assert(!aExcOccur());

    {
        RAII(AAutoKey) hold = ASemaphore_lock(&sem);
        assert(!aExcOccur());

        assert(thrd_create(&worker, semaphore_resize_worker, &state) == thrd_success);
        wait_until_at_least(&state.state_mutex, &state.ready, 1, 500);

        sleep_ms(20);
        assert(locked_read(&state.state_mutex, &state.acquired) == 0);

        ASemaphore_setMax(&sem, 2);
        assert(!aExcOccur());

        wait_until_at_least(&state.state_mutex, &state.acquired, 1, 500);
    }

    assert(thrd_join(worker, &worker_result) == thrd_success);
    assert(worker_result == 0);
    assert(sem.count == 0);
    assert(sem.max == 2);

    mtx_destroy(&state.state_mutex);
}

int main(void) {
    test_amtxcnd_nullptr_guards();
    test_amtxcnd_awake_all();
    test_semaphore_limits_concurrency();
    test_semaphore_setmax_wakes_waiter();
    puts("alock tests passed.");
    return 0;
}
