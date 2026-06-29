#include <alib/atimer.h>
#include <alib/athrd.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    atomic_int fired;
    atomic_long fired_ms[8];
    AClock start;
} TimerProbe;

static void sleep_ms(long ms) {
    struct timespec delay = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000 * 1000L,
    };
    thrd_sleep(&delay, NULL);
}

static void timer_probe_reset(TimerProbe *probe) {
    atomic_store(&probe->fired, 0);
    for (int i = 0; i < 8; ++i) {
        atomic_store(&probe->fired_ms[i], -1);
    }
    probe->start = A_INIT(AClock);
    assert(!aErrOccur());
}

static bool timer_probe_cb(void *data) {
    TimerProbe *probe = data;
    int index = atomic_fetch_add(&probe->fired, 1);
    if (index >= 0 && index < 8) {
        AClock now = A_INIT(AClock);
        atomic_store(&probe->fired_ms[index], (long)AClock_msDiff(now, probe->start));
    }
    return true;
}

static bool timer_probe_stop_cb(void *data) {
    TimerProbe *probe = data;
    timer_probe_cb(probe);
    return false;
}

static void test_one_shot(void) {
    TimerProbe probe;
    timer_probe_reset(&probe);

    int64_t id = a_timer_addwork_one(80, timer_probe_cb, &probe);
    assert(!aErrOccur());
    assert(id > 0);

    sleep_ms(180);
    assert(atomic_load(&probe.fired) == 1);
    assert(atomic_load(&probe.fired_ms[0]) >= 60);
}

static void test_repeat_count(void) {
    TimerProbe probe;
    timer_probe_reset(&probe);

    int64_t id = a_timer_addwork(60, 3, timer_probe_cb, &probe);
    assert(!aErrOccur());
    assert(id > 0);

    sleep_ms(320);
    assert(atomic_load(&probe.fired) == 3);
    assert(atomic_load(&probe.fired_ms[0]) >= 40);
    assert(atomic_load(&probe.fired_ms[1]) >= 90);
    assert(atomic_load(&probe.fired_ms[2]) >= 140);
}

static void test_add_while_waiting(void) {
    TimerProbe probe;
    timer_probe_reset(&probe);

    int64_t first = a_timer_addwork_one(1000, timer_probe_cb, &probe);
    assert(!aErrOccur());
    assert(first > 0);

    sleep_ms(500);
    int64_t second = a_timer_addwork_one(300, timer_probe_cb, &probe);
    assert(!aErrOccur());
    assert(second > 0);

    sleep_ms(700);
    assert(atomic_load(&probe.fired) == 2);

    long second_ms = atomic_load(&probe.fired_ms[0]);
    long first_ms = atomic_load(&probe.fired_ms[1]);

    assert(second_ms >= 650);
    assert(second_ms <= 1100);
    assert(first_ms >= 900);
    assert(first_ms <= 1300);
}

static void test_remove_long_work(void) {
    TimerProbe probe;
    timer_probe_reset(&probe);

    int64_t id = a_timer_addwork_long(40, timer_probe_cb, &probe);
    assert(!aErrOccur());
    assert(id > 0);

    sleep_ms(170);
    int before_remove = atomic_load(&probe.fired);
    assert(before_remove >= 1);

    a_timer_rmwork(id);
    assert(!aErrOccur());

    int after_remove = atomic_load(&probe.fired);
    sleep_ms(180);
    assert(atomic_load(&probe.fired) == after_remove);
}

static void test_callback_can_stop_work(void) {
    TimerProbe probe;
    timer_probe_reset(&probe);

    int64_t id = a_timer_addwork_long(40, timer_probe_stop_cb, &probe);
    assert(!aErrOccur());
    assert(id > 0);

    sleep_ms(180);
    assert(atomic_load(&probe.fired) == 1);
}

int main(void) {
    test_one_shot();
    test_repeat_count();
    test_add_while_waiting();
    test_remove_long_work();
    test_callback_can_stop_work();
    printf("All ATimer tests passed.\n");
    return 0;
}
