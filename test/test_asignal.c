#include <alib/alib.h>
#include <alib/asignal.h>
#include <assert.h>
#include <stdio.h>

typedef struct TestSignal TestSignal;

AClass_Inherit(TestSignal, ASignal);
AClass_Struct(TestSignal,
    int extra;
);
AClass_Function(TestSignal);
AClass_Generate(TestSignal);
A_CLASS_REGISTER(TestSignal);

static void noop_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
}

static void test_signal_type_helpers(void) {
    RAII(TestSignal) sig = A_INIT(TestSignal);
    ASignal *base = (void *)&sig;
    assert(!aExcOccur());
    assert(sig.f != NULL);
    base->id = 7;
    base->value = 9;
    sig.extra = 11;
    assert(base->id == 7);
    assert(base->value == 9);
    assert(sig.extra == 11);

    int receiver = 0;
    ASignalEnd src = {
        .addressee = &receiver,
        .call = noop_target,
    };
    ASignalEnd dst = A_COPY(ASignalEnd, src);
    assert(!aExcOccur());
    assert(dst.addressee == &receiver);
    assert(dst.call == noop_target);

    ASignalSystem sys0 = A_INIT(ASignalSystem);
    sys0.count = 5;

    ASignalSystem sys1 = A_COPY(ASignalSystem, sys0);
    assert(!aExcOccur());
    assert(sys1.count == 5);
    assert(A_CMPD(ASignalSystem, sys0, sys1) == 0);

    sys1.count = 7;
    assert(A_CMPD(ASignalSystem, sys0, sys1) < 0);

    A_DEST(ASignalSystem, sys0);
    A_DEST(ASignalSystem, sys1);
}

static void count_target(const ASignal *signal, void *addressee) {
    int *counter = addressee;
    assert(signal != NULL);
    assert(counter != NULL);
    (*counter)++;
}

static int g_duplicate_target_hits = 0;

static void duplicate_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
    g_duplicate_target_hits++;
}

static void test_signal_transmit_basic(void) {
    int64_t id = a_signal_system_alloc();
    assert(!aExcOccur());
    assert(id >= 0);

    int receiver0 = 0;
    int receiver1 = 0;
    a_signal_system_register(id, &receiver0, count_target);
    assert(!aExcOccur());
    a_signal_system_register(id, &receiver1, count_target);
    assert(!aExcOccur());

    ASignal sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = id;
    sig.value = 42;
    sig.sender = &receiver0;
    sig.name = "basic";
    sig.code = "sig-basic";

    a_signal_system_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver0 == 1);
    assert(receiver1 == 1);
}

static void test_signal_reject_duplicate_addressee(void) {
    int64_t id = a_signal_system_alloc();
    assert(!aExcOccur());
    assert(id >= 0);

    int receiver = 0;
    g_duplicate_target_hits = 0;

    a_signal_system_register(id, &receiver, count_target);
    assert(!aExcOccur());

    a_signal_system_register(id, &receiver, duplicate_target);
    assert(aExcGet() == AEXC_repeat_write);
    aExcClean();

    ASignal sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = id;

    a_signal_system_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver == 1);
    assert(g_duplicate_target_hits == 0);

    a_signal_system_unregister(id, &receiver);
    assert(!aExcOccur());

    a_signal_system_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver == 1);
    assert(g_duplicate_target_hits == 0);
}

static int g_nested_hits = 0;
static int64_t g_inner_id = -1;

static void nested_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
    g_nested_hits++;
}

static void outer_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;

    ASignal nested = A_INIT(ASignal);
    assert(!aExcOccur());
    nested.id = g_inner_id;
    a_signal_system_transmit(&nested);
    assert(!aExcOccur());
}

static void test_signal_transmit_reentrant(void) {
    int64_t outer_id = a_signal_system_alloc();
    assert(!aExcOccur());
    g_inner_id = a_signal_system_alloc();
    assert(!aExcOccur());

    int receiver = 0;
    g_nested_hits = 0;
    a_signal_system_register(outer_id, &receiver, outer_target);
    assert(!aExcOccur());
    a_signal_system_register(g_inner_id, &receiver, nested_target);
    assert(!aExcOccur());

    ASignal outer = A_INIT(ASignal);
    assert(!aExcOccur());
    outer.id = outer_id;
    a_signal_system_transmit(&outer);
    assert(!aExcOccur());
    assert(g_nested_hits == 1);
}

static int g_registered_hits = 0;
static int64_t g_register_target_id = -1;

static void registered_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
    g_registered_hits++;
}

static void alloc_register_target(const ASignal *signal, void *addressee) {
    (void)signal;

    int64_t nested_id = a_signal_system_alloc();
    assert(!aExcOccur());
    assert(nested_id >= 0);

    a_signal_system_register(g_register_target_id, addressee, registered_target);
    assert(!aExcOccur());
}

static void test_signal_callback_alloc_and_register(void) {
    int receiver = 0;
    int64_t outer_id = a_signal_system_alloc();
    assert(!aExcOccur());
    g_register_target_id = a_signal_system_alloc();
    assert(!aExcOccur());

    g_registered_hits = 0;
    a_signal_system_register(outer_id, &receiver, alloc_register_target);
    assert(!aExcOccur());

    ASignal outer = A_INIT(ASignal);
    assert(!aExcOccur());
    outer.id = outer_id;
    a_signal_system_transmit(&outer);
    assert(!aExcOccur());

    ASignal registered = A_INIT(ASignal);
    assert(!aExcOccur());
    registered.id = g_register_target_id;
    a_signal_system_transmit(&registered);
    assert(!aExcOccur());
    assert(g_registered_hits == 1);
}

int main(void) {
    test_signal_type_helpers();
    test_signal_transmit_basic();
    test_signal_reject_duplicate_addressee();
    test_signal_transmit_reentrant();
    test_signal_callback_alloc_and_register();
    printf("All ASignal tests passed.\n");
    return 0;
}
