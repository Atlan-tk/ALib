#include <alib/athrd.h>
#include <assert.h>
#include <stdio.h>

typedef struct {
    mtx_t mutex;
    cnd_t cond;
    int waiting;
    int go;
    int completed;
} WaitGroup;

typedef struct {
    WaitGroup *group;
    thrd_t main_thread;
    int id;
} WorkerArgs;

typedef struct {
    mtx_t mutex;
    cnd_t cond;
    int done;
} DetachedState;

typedef struct {
    mtx_t *mutex;
    int try_result;
    int timed_result;
} TimedLockArgs;

static once_flag g_once = ONCE_FLAG_INIT;
static int g_once_hits = 0;
static tss_t g_tls_key;

static struct timespec deadline_after_ms(long ms) {
    struct timespec now = {0};
    int rc = timespec_get(&now, TIME_UTC);
    assert(rc == TIME_UTC);

    now.tv_sec += ms / 1000;
    now.tv_nsec += (ms % 1000) * 1000000L;
    if (now.tv_nsec >= 1000000000L) {
        now.tv_sec++;
        now.tv_nsec -= 1000000000L;
    }
    return now;
}

static void once_init(void) {
    g_once_hits++;
}

static int wait_worker(void *arg) {
    WorkerArgs *worker = arg;
    assert(worker != NULL);
    assert(!thrd_equal(thrd_current(), worker->main_thread));

    call_once(&g_once, once_init);
    assert(tss_set(g_tls_key, worker) == thrd_success);
    assert(tss_get(g_tls_key) == worker);

    assert(mtx_lock(&worker->group->mutex) == thrd_success);
    worker->group->waiting++;
    assert(cnd_broadcast(&worker->group->cond) == thrd_success);

    while (!worker->group->go) {
        assert(cnd_wait(&worker->group->cond, &worker->group->mutex) == thrd_success);
    }

    worker->group->completed++;
    assert(cnd_broadcast(&worker->group->cond) == thrd_success);
    assert(mtx_unlock(&worker->group->mutex) == thrd_success);
    return worker->id * 10;
}

static int detached_worker(void *arg) {
    DetachedState *state = arg;

    assert(mtx_lock(&state->mutex) == thrd_success);
    state->done = 1;
    assert(cnd_signal(&state->cond) == thrd_success);
    assert(mtx_unlock(&state->mutex) == thrd_success);
    return 0;
}

static int timedlock_worker(void *arg) {
    TimedLockArgs *state = arg;
    struct timespec deadline = deadline_after_ms(30);

    state->try_result = mtx_trylock(state->mutex);
    assert(state->try_result == thrd_busy);

    state->timed_result = mtx_timedlock(state->mutex, &deadline);
    if (state->timed_result == thrd_success) {
        assert(mtx_unlock(state->mutex) == thrd_success);
    }
    return 0;
}

static void test_sleep_and_current(void) {
    struct timespec zero = {0};

    assert(thrd_equal(thrd_current(), thrd_current()));
    assert(thrd_sleep(&zero, NULL) == 0);
}

static void test_recursive_mutex(void) {
    mtx_t mutex = {0};

    assert(mtx_init(&mutex, mtx_plain | mtx_recursive) == thrd_success);
    assert(mtx_lock(&mutex) == thrd_success);
    assert(mtx_trylock(&mutex) == thrd_success);
    assert(mtx_unlock(&mutex) == thrd_success);
    assert(mtx_unlock(&mutex) == thrd_success);
    mtx_destroy(&mutex);
}

static void test_threads_once_tss_and_condition(void) {
    WaitGroup group = {0};
    WorkerArgs workers[2] = {0};
    thrd_t threads[2] = {0};
    int results[2] = {0};

    g_once_hits = 0;

    assert(tss_create(&g_tls_key, NULL) == thrd_success);
    assert(mtx_init(&group.mutex, mtx_plain) == thrd_success);
    assert(cnd_init(&group.cond) == thrd_success);

    for (int i = 0; i < 2; ++i) {
        workers[i].group = &group;
        workers[i].main_thread = thrd_current();
        workers[i].id = i + 1;
        assert(thrd_create(&threads[i], wait_worker, &workers[i]) == thrd_success);
    }

    assert(mtx_lock(&group.mutex) == thrd_success);
    while (group.waiting < 2) {
        assert(cnd_wait(&group.cond, &group.mutex) == thrd_success);
    }
    group.go = 1;
    assert(cnd_broadcast(&group.cond) == thrd_success);
    while (group.completed < 2) {
        assert(cnd_wait(&group.cond, &group.mutex) == thrd_success);
    }
    assert(mtx_unlock(&group.mutex) == thrd_success);

    for (int i = 0; i < 2; ++i) {
        assert(thrd_join(threads[i], &results[i]) == thrd_success);
        assert(results[i] == (i + 1) * 10);
    }

    assert(g_once_hits == 1);

    cnd_destroy(&group.cond);
    mtx_destroy(&group.mutex);
    tss_delete(g_tls_key);
}

static void test_detach(void) {
    DetachedState state = {0};
    thrd_t thread = {0};

    assert(mtx_init(&state.mutex, mtx_plain) == thrd_success);
    assert(cnd_init(&state.cond) == thrd_success);
    assert(thrd_create(&thread, detached_worker, &state) == thrd_success);
    assert(thrd_detach(thread) == thrd_success);

    assert(mtx_lock(&state.mutex) == thrd_success);
    while (!state.done) {
        assert(cnd_wait(&state.cond, &state.mutex) == thrd_success);
    }
    assert(mtx_unlock(&state.mutex) == thrd_success);

    cnd_destroy(&state.cond);
    mtx_destroy(&state.mutex);
}

static void test_timed_mutex(void) {
    mtx_t mutex = {0};
    thrd_t thread = {0};
    TimedLockArgs args = {0};
    int thread_result = -1;
    struct timespec hold = {
        .tv_sec = 0,
        .tv_nsec = 60000000L,
    };

    assert(mtx_init(&mutex, mtx_plain | mtx_timed) == thrd_success);
    assert(mtx_lock(&mutex) == thrd_success);

    args.mutex = &mutex;
    assert(thrd_create(&thread, timedlock_worker, &args) == thrd_success);
    assert(thrd_sleep(&hold, NULL) == 0);
    assert(mtx_unlock(&mutex) == thrd_success);
    assert(thrd_join(thread, &thread_result) == thrd_success);
    assert(thread_result == 0);
    assert(args.try_result == thrd_busy);
    assert(args.timed_result == thrd_timedout);

    mtx_destroy(&mutex);
}

static void test_timed_condition(void) {
    mtx_t mutex = {0};
    cnd_t cond = {0};
    int wait_result = thrd_success;

    assert(mtx_init(&mutex, mtx_plain | mtx_timed) == thrd_success);
    assert(cnd_init(&cond) == thrd_success);
    assert(mtx_lock(&mutex) == thrd_success);

    do {
        struct timespec deadline = deadline_after_ms(20);
        wait_result = cnd_timedwait(&cond, &mutex, &deadline);
    } while (wait_result == thrd_success);
    assert(wait_result == thrd_timedout);

    assert(mtx_unlock(&mutex) == thrd_success);
    cnd_destroy(&cond);
    mtx_destroy(&mutex);
}

int main(void) {
    test_sleep_and_current();
    test_recursive_mutex();
    test_threads_once_tss_and_condition();
    test_detach();
    test_timed_mutex();
    test_timed_condition();
    puts("athrd tests passed.");
    return 0;
}
