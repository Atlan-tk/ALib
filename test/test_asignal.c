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

typedef struct {
    int hits;
    int64_t last_id;
    int64_t last_value;
    const void* last_sender;
} Counter;

static void test_signal_type_helpers(void) {
    RAII(TestSignal) sig = A_INIT(TestSignal);
    ASignal *base = (void *)&sig;
    assert(!aExcOccur());
    assert(sig.f != NULL);
    base->id = 7;
    base->value = 9;
    base->sender = &sig;
    sig.extra = 11;
    assert(base->id == 7);
    assert(base->value == 9);
    assert(base->sender == &sig);
    assert(sig.extra == 11);
}

static void count_target(const ASignal *signal, void *addressee) {
    Counter *counter = addressee;
    assert(signal != NULL);
    assert(counter != NULL);
    counter->hits++;
    counter->last_id = signal->id;
    counter->last_value = signal->value;
    counter->last_sender = signal->sender;
}

static int g_duplicate_target_hits = 0;

static void duplicate_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
    g_duplicate_target_hits++;
}

static void test_signal_transmit_basic(void) {
    int64_t id = a_signal_alloc();
    assert(!aExcOccur());
    assert(id >= 0);

    Counter receiver0 = {0};
    Counter receiver1 = {0};

    RAII(ASignal) sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = id;
    sig.value = 42;
    sig.sender = &receiver0;

    a_signal_connection(id, &receiver0, count_target);
    assert(!aExcOccur());
    a_signal_connection(id, &receiver1, count_target);
    assert(!aExcOccur());

    a_signal_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver0.hits == 1);
    assert(receiver0.last_id == id);
    assert(receiver0.last_value == 42);
    assert(receiver0.last_sender == &receiver0);
    assert(receiver1.hits == 1);
    assert(receiver1.last_id == id);
    assert(receiver1.last_value == 42);
    assert(receiver1.last_sender == &receiver0);

    a_signal_disconnect(id, &receiver0);
    assert(!aExcOccur());

    a_signal_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver0.hits == 1);
    assert(receiver1.hits == 2);

    a_target_disconnect(&receiver1, id);
    assert(!aExcOccur());

    a_signal_transmit(&sig);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();
    assert(receiver0.hits == 1);
    assert(receiver1.hits == 2);
}

static void test_signal_duplicate_addressee_replaced(void) {
    int64_t id = a_signal_alloc();
    assert(!aExcOccur());
    assert(id >= 0);

    Counter receiver = {0};
    g_duplicate_target_hits = 0;

    a_signal_connection(id, &receiver, count_target);
    assert(!aExcOccur());

    a_signal_connection(id, &receiver, duplicate_target);
    assert(!aExcOccur());

    RAII(ASignal) sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = id;
    sig.value = 7;
    sig.sender = &receiver;

    a_signal_transmit(&sig);
    assert(!aExcOccur());
    assert(receiver.hits == 0);
    assert(g_duplicate_target_hits == 1);

    a_signal_disconnect_all(id);
    assert(!aExcOccur());

    a_signal_transmit(&sig);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();
    assert(receiver.hits == 0);
    assert(g_duplicate_target_hits == 1);
}

static void test_disconnect_all_helpers(void) {
    int64_t id0 = a_signal_alloc();
    int64_t id1 = a_signal_alloc();
    assert(!aExcOccur());
    assert(id0 >= 0);
    assert(id1 >= 0);

    int unknown = 0;
    Counter receiver = {0};

    a_signal_disconnect_all(id0);
    assert(!aExcOccur());
    a_target_disconnect_all(&unknown);
    assert(!aExcOccur());

    a_signal_connection(id0, &receiver, count_target);
    assert(!aExcOccur());
    a_signal_connection(id1, &receiver, count_target);
    assert(!aExcOccur());

    RAII(ASignal) sig0 = A_INIT(ASignal);
    assert(!aExcOccur());
    sig0.id = id0;
    sig0.value = 3;
    sig0.sender = &id0;

    RAII(ASignal) sig1 = A_INIT(ASignal);
    assert(!aExcOccur());
    sig1.id = id1;
    sig1.value = 5;
    sig1.sender = &id1;

    a_signal_transmit(&sig0);
    assert(!aExcOccur());
    a_signal_transmit(&sig1);
    assert(!aExcOccur());
    assert(receiver.hits == 2);

    a_target_disconnect_all(&receiver);
    assert(!aExcOccur());

    a_signal_transmit(&sig0);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();
    a_signal_transmit(&sig1);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();
    assert(receiver.hits == 2);

    a_signal_disconnect_all(id1);
    assert(!aExcOccur());
}

static void test_invalid_id_rejected(void) {
    int64_t id = a_signal_alloc();
    assert(!aExcOccur());
    int64_t invalid_id = id + 1;
    Counter receiver = {0};

    a_signal_connection(invalid_id, &receiver, count_target);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();

    a_signal_disconnect(invalid_id, &receiver);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();

    a_target_disconnect(&receiver, invalid_id);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();

    a_signal_disconnect_all(invalid_id);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();

    RAII(ASignal) sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = invalid_id;
    a_signal_transmit(&sig);
    assert(aExcGet() == AEXC_outdomain);
    aExcClean();
}

static int g_nested_hits = 0;
static int64_t g_inner_id = -1;

static void nested_target(const ASignal *signal, void *addressee) {
    (void)addressee;
    assert(signal != NULL);
    assert(signal->id == g_inner_id);
    g_nested_hits++;
}

static void outer_target(const ASignal *signal, void *addressee) {
    (void)signal;

    RAII(ASignal) nested = A_INIT(ASignal);
    assert(!aExcOccur());
    nested.id = g_inner_id;
    nested.value = 99;
    nested.sender = addressee;
    a_signal_transmit(&nested);
    assert(!aExcOccur());
}

static void test_signal_transmit_reentrant(void) {
    int64_t outer_id = a_signal_alloc();
    assert(!aExcOccur());
    g_inner_id = a_signal_alloc();
    assert(!aExcOccur());

    Counter receiver = {0};
    g_nested_hits = 0;
    a_signal_connection(outer_id, &receiver, outer_target);
    assert(!aExcOccur());
    a_signal_connection(g_inner_id, &receiver, nested_target);
    assert(!aExcOccur());

    RAII(ASignal) outer = A_INIT(ASignal);
    assert(!aExcOccur());
    outer.id = outer_id;
    outer.sender = &receiver;
    a_signal_transmit(&outer);
    assert(!aExcOccur());
    assert(g_nested_hits == 1);
}

static int g_registered_hits = 0;
static int64_t g_register_target_id = -1;
static int g_collect_target_hits = 0;

static void registered_target(const ASignal *signal, void *addressee) {
    (void)signal;
    (void)addressee;
    g_registered_hits++;
}

static void alloc_register_target(const ASignal *signal, void *addressee) {
    (void)signal;

    int64_t nested_id = a_signal_alloc();
    assert(!aExcOccur());
    assert(nested_id >= 0);

    a_signal_connection(g_register_target_id, addressee, registered_target);
    assert(!aExcOccur());
}

static void collect_target(const ASignal *signal, void *addressee) {
    (void)signal;
    int *hit_counter = addressee;

    (*hit_counter)++;
    g_collect_target_hits++;
    aExcSet(AEXC_outdomain);
}

static void test_signal_callback_alloc_and_register(void) {
    int receiver = 0;
    int64_t outer_id = a_signal_alloc();
    assert(!aExcOccur());
    g_register_target_id = a_signal_alloc();
    assert(!aExcOccur());

    g_registered_hits = 0;
    a_signal_connection(outer_id, &receiver, alloc_register_target);
    assert(!aExcOccur());

    RAII(ASignal) outer = A_INIT(ASignal);
    assert(!aExcOccur());
    outer.id = outer_id;
    outer.sender = &receiver;
    a_signal_transmit(&outer);
    assert(!aExcOccur());

    RAII(ASignal) registered = A_INIT(ASignal);
    assert(!aExcOccur());
    registered.id = g_register_target_id;
    registered.sender = &receiver;
    a_signal_transmit(&registered);
    assert(!aExcOccur());
    assert(g_registered_hits == 1);
}

static void test_signal_collect_exceptions(void) {
    int64_t id = a_signal_alloc();
    assert(!aExcOccur());
    assert(id >= 0);

    int receiver = 0;
    g_collect_target_hits = 0;

    a_signal_connection(id, &receiver, collect_target);
    assert(!aExcOccur());

    RAII(ASignal) sig = A_INIT(ASignal);
    assert(!aExcOccur());
    sig.id = id;
    sig.sender = &receiver;

    RAII(AExcCollector) collector = A_INIT(AExcCollector);
    assert(!aExcOccur());

    int ret = a_signal_transmit(&sig, &collector);
    assert(ret == AEXC_response_exc);
    assert(collector.id == id);
    assert(g_collect_target_hits == 1);
    assert(receiver == 1);
    assert(collector.list.f->getNumber(&collector.list) == 1);

    AExcEnd ev = AExcCollector_pop(&collector);
    assert(!aExcOccur());
    assert(ev.addressee == &receiver);
    assert(ev.exc_value == AEXC_outdomain);
    assert(collector.list.f->empty(&collector.list));

    ret = a_signal_transmit(&sig);
    assert(ret == AEXC_response_exc);
    assert(g_collect_target_hits == 2);
    assert(receiver == 2);

    a_signal_disconnect_all(id);
    assert(!aExcOccur());
}

int main(void) {
    test_signal_type_helpers();
    test_signal_transmit_basic();
    test_signal_duplicate_addressee_replaced();
    test_disconnect_all_helpers();
    test_invalid_id_rejected();
    test_signal_transmit_reentrant();
    test_signal_callback_alloc_and_register();
    test_signal_collect_exceptions();
    printf("All ASignal tests passed.\n");
    return 0;
}
