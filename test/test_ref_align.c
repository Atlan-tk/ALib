#include <alib/alib.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    max_align_t value;
    int marker;
} RefAlignedPayload;

static void init_payload(void* p) {
    RefAlignedPayload* payload = p;
    payload->marker = 42;
}

static void copy_payload(void* p, const void* that) {
    *(RefAlignedPayload*)p = *(const RefAlignedPayload*)that;
}

static void test_ref_new_alignment(void) {
    RefAlignedPayload* payload = alib_ref_new(sizeof(RefAlignedPayload), init_payload);

    assert(payload != NULL);
    assert(((uintptr_t)payload % alignof(RefAlignedPayload)) == 0);
    assert(payload->marker == 42);

    RefAlignedPayload* alias = alib_ref_copy(payload);
    assert(alias == payload);

    alib_ref_delete(alias, NULL);
    alib_ref_delete(payload, NULL);
}

static void test_ref_new_for_copy_alignment(void) {
    const RefAlignedPayload source = {
        .marker = 7,
    };
    RefAlignedPayload* payload = alib_ref_new_for_copy(
        sizeof(RefAlignedPayload), &source, copy_payload
    );

    assert(payload != NULL);
    assert(((uintptr_t)payload % alignof(RefAlignedPayload)) == 0);
    assert(payload->marker == source.marker);

    alib_ref_delete(payload, NULL);
}

int main(void) {
    test_ref_new_alignment();
    test_ref_new_for_copy_alignment();
    printf("ref alignment tests passed.\n");
    return 0;
}
