#include "common/libpull.h"
#include "network/receiver.h"
#include "memory/manifest.h"
#include "memory/memory_objects.h"

#include "mock_memory.h"

#include "memory_file_posix.h"
#include "transport_libcoap.h"

#include "sample_data.h"
#include "unity.h"

#define PROV_SERVER "localhost"

#define NO_MOCK(func) func##_StubWithCallback(func##_impl)

void tearDown(void) {
    NO_MOCK(memory_open);
    NO_MOCK(memory_flush);
    NO_MOCK(memory_write);
    NO_MOCK(memory_close);

    manifest_t invalid_mt;
    bzero(&invalid_mt, sizeof(manifest_t));
    mem_object obj_t;
    pull_error err = write_firmware_manifest(OBJ_2, &invalid_mt, &obj_t);
    TEST_ASSERT_TRUE(!err);
}

void test_get_firmware_invalid_memory_write(void) {
    NO_MOCK(memory_open);
    NO_MOCK(memory_flush);
    NO_MOCK(memory_close);

    txp_ctx txp;
    receiver_ctx rcv;
    pull_error err = txp_init(&txp, PROV_SERVER, 0, CONN_UDP, NULL);
    TEST_ASSERT_TRUE(!err);
    mem_object obj_t;
    err = receiver_open(&rcv, &txp, "firmware", OBJ_2, &obj_t);
    TEST_ASSERT_TRUE(!err);
    while (!rcv.firmware_received && !err) {
        err = receiver_chunk(&rcv);
        loop(&txp, 1000);
    }
    TEST_ASSERT_TRUE_MESSAGE(err == MEMORY_WRITE_ERROR, err_as_str(err));
}
