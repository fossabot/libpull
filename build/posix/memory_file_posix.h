#ifndef MEMORY_FILE_POSIX_H_
#define MEMORY_FILE_POSIX_H_

#include "common/libpull.h"
#include "memory/memory.h"

struct mem_object_ {
    char* path;
    int fp;
    int start_offset;
    int end_offset;
};

enum memory_objects_enum {
    OBJ_FIRST = 0,
    OBJ_GOLD = 1,
    OBJ_RUN = 2,
    OBJ_1 = 3,
    OBJ_2 = 4,
    /// Other files used for testing
    TEST_MEMORY_FILE = 5,
    OBJ_LAST = 6
};

/******* Testing functions *******/
void override_memory_object(obj_id id, char* path, int start, int end);
/***** End Testing functions *****/

pull_error memory_open_impl(mem_object* ctx, obj_id obj, mem_mode_t mode);

int memory_read_impl(mem_object* ctx, void* memory_buffer, uint16_t size, uint32_t offset);

int memory_write_impl(mem_object* ctx, const void* memory_buffer, uint16_t size, uint32_t offset);

pull_error memory_flush_impl(mem_object* ctx);

pull_error memory_close_impl(mem_object* ctx);

#endif // MEMORY_FILE_POSIX_H_
