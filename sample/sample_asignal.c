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
typedef struct PingSource PingSource;
typedef struct PingReceiver PingReceiver;

AClass_Inherit(PingSignal, ASignal);
AClass_Struct(PingSignal,
    int payload;
);
AClass_Function(PingSignal);
AClass_Generate(PingSignal);
A_CLASS_REGISTER(PingSignal);

AClass_Inherit(PingSource, ATranEnd);
AClass_Struct(PingSource,
    const char* name;
);
AClass_Function(PingSource);
AClass_Generate(PingSource);
static void A_OBJ_INIT(PingSource)(PingSource* self) {
    self->name = "source";
}
A_CLASS_REGISTER(PingSource);

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

static Aint ping_id(const PingSource* source) {
    return A_CALL(*source, ATranEnd).getID((const ATranEnd*)source);
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

static void drain_signal_exceptions(ASignal* base) {
    if (base->exc_list == NULL) {
        return;
    }

    while (!base->exc_list->f->empty(base->exc_list)) {
        AResponseExc ev = base->f->popExc(base);
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
        base->f->setExcList(base);
        if (print_exc("setExcList") != 0) {
            return -1;
        }
    }

    A_CALL(*source, ATranEnd).transmit(base);
    if (aExcGet() == AEXC_response_exc) {
        printf("emit_ping(payload=%d) collected callback errors:\n", payload);
        drain_signal_exceptions(base);
        aExcClean();
        return 0;
    }

    return print_exc("transmit");
}

int main(void) {
    RAII(PingSource) source = A_INIT(PingSource);
    if (print_exc("A_INIT(PingSource)") != 0) {
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

    source.name = "ticker";
    fast.name = "fast";
    guard.name = "guard";
    guard.reject_negative = true;

    Aint id = ping_id(&source);
    if (print_exc("getID") != 0) {
        return 1;
    }

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
    printf("PingSource/PingReceiver inherit ATranEnd/AReceEnd, so they auto-disconnect on scope exit.\n");
    return 0;
}
