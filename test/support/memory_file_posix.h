#ifndef _MEMORY_FILE_POSIX_H
#define _MEMORY_FILE_POSIX_H

#include "memory/memory.h"
#include "common/external.h"

#ifdef POSIX

struct mem_object_ {
    char* path;
    int fp;
    int start_offset;
    int end_offset;
};

enum memory_objects_enum {
    OBJ_FIRST = 0,
    OBJ_GOLD,          
    OBJ_RUN,      
    OBJ_1,      
    OBJ_2,  
    /// Other files used for testing
    TEST_MEMORY_FILE,
    OBJ_LAST
};

/******* Testing functions *******/
void override_memory_object(obj_id id, char* path, int start, int end);
/***** End Testing functions *****/

pull_error memory_open_impl(mem_object* ctx, obj_id obj);

int memory_read_impl(mem_object* ctx, void* memory_buffer, uint16_t size, uint32_t offset);

int memory_write_impl(mem_object* ctx, const void* memory_buffer, uint16_t size, uint32_t offset);

pull_error memory_flush_impl(mem_object* ctx);

pull_error memory_close_impl(mem_object* ctx);

#endif // POSIX
#endif // _MEMORY_FILE_POSIX_H
