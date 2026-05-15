/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

/*
 * alib signal sample
 */

#include <alib/alib.h>
#include <alib/asignal.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct PingSignal PingSignal;
typedef struct PingReceiver PingReceiver;

typedef struct {
    Aint id;
    const char* name;
} PingSource;

AClass_Inherit(PingSignal, ASignal);
AClass_Struct(PingSignal,
    int payload;
);
AClass_Function(PingSignal);
AClass_Generate(PingSignal);
A_CLASS_REGISTER(PingSignal);

AClass_Inherit(PingReceiver, AReceEnd);
AClass_Struct(PingReceiver,
    const char* name;
    int total;
    bool reject_negative;
);
AClass_Function(PingReceiver);
AClass_Generate(PingReceiver);
static void A_OBJ_INIT(PingReceiver)(PingReceiver* self) {
    self->name = "receiver";
    self->total = 0;
    self->reject_negative = false;
}
A_CLASS_REGISTER(PingReceiver);

static int print_exc(const char* step) {
    if (!aExcOccur()) {
        return 0;
    }

    fprintf(stderr, "%s failed, aExc=%d\n", step, aExcGet());
    return -1;
}

static int ping_source_init(PingSource* source, const char* name) {
    if (source == NULL) {
        aExcSet(AEXC_nullptr);
        return -1;
    }

    source->id = a_signal_alloc();
    source->name = name;
    return print_exc("a_signal_alloc");
}

static Aint ping_id(const PingSource* source) {
    if (source == NULL) {
        aExcSet(AEXC_nullptr);
        return -1;
    }
    return source->id;
}

static void ping_target(const ASignal* base, void* addressee) {
    const PingSignal* signal = (const PingSignal*)base;
    const PingSource* source = (const PingSource*)base->sender;
    PingReceiver* receiver = addressee;

    if (receiver->reject_negative && signal->payload < 0) {
        printf("[%s] reject payload %d from %s\n",
                receiver->name, signal->payload, source->name);
        aExcSet(AEXC_outdomain);
        return;
    }

    receiver->total += signal->payload;
    printf("[%s] received payload %d from %s, total=%d\n",
            receiver->name, signal->payload, source->name, receiver->total);
}

static void drain_signal_exceptions(AExcCollector* collector) {
    while (!collector->list.f->empty(&collector->list)) {
        AExcEnd ev = AExcCollector_pop(collector);
        const PingReceiver* receiver = ev.addressee;
        printf("  collected exception from %s: %d\n",
                receiver->name, ev.exc_value);
    }
}

static int emit_ping(PingSource* source, int payload, bool collect_exc) {
    RAII(PingSignal) signal = A_INIT(PingSignal);
    if (print_exc("A_INIT(PingSignal)") != 0) {
        return -1;
    }

    ASignal* base = (ASignal*)&signal;
    base->id = ping_id(source);
    base->value = payload;
    base->sender = source;
    signal.payload = payload;

    if (collect_exc) {
        RAII(AExcCollector) collector = A_INIT(AExcCollector);

        a_signal_transmit(base, &collector);
        if (aExcGet() == AEXC_response_exc) {
            printf("emit_ping(payload=%d) collected callback errors for id=%d:\n",
                    payload, collector.id);
            aExcClean();
            drain_signal_exceptions(&collector);
            return 0;
        }

        return print_exc("a_signal_transmit");
    }

    a_signal_transmit(base);
    return print_exc("a_signal_transmit");
}

int main(void) {
    PingSource source = {0};
    if (ping_source_init(&source, "ticker") != 0) {
        return 1;
    }

    RAII(PingReceiver) fast = A_INIT(PingReceiver);
    if (print_exc("A_INIT(PingReceiver fast)") != 0) {
        return 1;
    }

    RAII(PingReceiver) guard = A_INIT(PingReceiver);
    if (print_exc("A_INIT(PingReceiver guard)") != 0) {
        return 1;
    }

    fast.name = "fast";
    guard.name = "guard";
    guard.reject_negative = true;

    Aint id = ping_id(&source);

    printf("Ping signal id = %d\n", id);

    A_CALL(fast, AReceEnd).connection((const AReceEnd*)&fast, id, ping_target);
    if (print_exc("connect fast") != 0) {
        return 1;
    }

    A_CALL(guard, AReceEnd).connection((const AReceEnd*)&guard, id, ping_target);
    if (print_exc("connect guard") != 0) {
        return 1;
    }

    if (emit_ping(&source, 3, false) != 0) {
        return 1;
    }

    if (emit_ping(&source, -2, true) != 0) {
        return 1;
    }

    printf("final totals: fast=%d guard=%d\n", fast.total, guard.total);
    printf("PingReceiver inherits AReceEnd and auto-disconnects on scope exit; sender id is owned by user code.\n");
    return 0;
}
