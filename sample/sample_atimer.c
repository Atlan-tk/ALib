#include <alib/atimer.h>
#include <alib/athrd.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    const char *name;
    atomic_int *count;
    AClock start;
} TimerEvent;

static void sleep_ms(long ms) {
    struct timespec delay = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000 * 1000L,
    };
    thrd_sleep(&delay, NULL);
}

static void print_timer_event(void *data) {
    TimerEvent *event = data;
    AClock now = A_INIT(AClock);
    int n = atomic_fetch_add(event->count, 1) + 1;
    printf("%s fired #%d at %lld ms\n",
        event->name,
        n,
        (long long)AClock_msDiff(now, event->start));
}

int main(void) {
    atomic_int once_count = 0;
    atomic_int repeat_count = 0;
    AClock start = A_INIT(AClock);

    TimerEvent once = { .name = "one-shot", .count = &once_count, .start = start };
    TimerEvent repeat = { .name = "repeat", .count = &repeat_count, .start = start };

    int64_t once_id = a_timer_addwork_one(120, print_timer_event, &once);
    int64_t repeat_id = a_timer_addwork(80, 3, print_timer_event, &repeat);
    if (aErrOccur() || once_id < 0 || repeat_id < 0) {
        fprintf(stderr, "failed to schedule timer work: %d\n", aErrGet());
        return 1;
    }

    while (atomic_load(&once_count) < 1 || atomic_load(&repeat_count) < 3) {
        sleep_ms(20);
    }

    printf("timer sample done\n");
    return 0;
}
