#include "common/external.h"
#include "common/logger.h"
#include "common/error.h"

#include "contiki.h"
#include "memory_firefly.h"
#include "dev/rom-util.h" // cc2538 specific file
#include "dev/flash.h" // cc2538 specific file
#include "cpu.h"
#include <string.h>

#define PAGE_SIZE 2048 // 2048 according to 2538 datasheet

#define BUFFER_SIZE PAGE_SIZE

static uint32_t buffer_offset = 0; // it is signed
static uint16_t buffer_full = 0;
static uint8_t buffer[BUFFER_SIZE];

const int8_t memory_objects[] = { OBJ_RUN, OBJ_1, OBJ_2, OBJ_END};

#define INITIAL_OFFSET 0x200000
#define BOOTLOADER_PAGES 0 // 19
#define BOOTLOADER_CTX_PAGES 0 // 1
#define IMAGE_PAGES 64
int memory_object_start[] = {
    [BOOTLOADER] = INITIAL_OFFSET,
    [BOOTLOADER_CTX] = INITIAL_OFFSET + (BOOTLOADER_PAGES * PAGE_SIZE),
    [OBJ_RUN] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + (BOOTLOADER_CTX_PAGES * PAGE_SIZE),
    [OBJ_1] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE + IMAGE_PAGES*PAGE_SIZE,
    [OBJ_2] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE + 2*IMAGE_PAGES*PAGE_SIZE
};
int memory_object_end[] = {
    [BOOTLOADER] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE,
    [BOOTLOADER_CTX] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE,
    [OBJ_RUN] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE + IMAGE_PAGES*PAGE_SIZE,
    [OBJ_1] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE + 2*IMAGE_PAGES*PAGE_SIZE,
    [OBJ_2] = INITIAL_OFFSET + BOOTLOADER_PAGES*PAGE_SIZE + BOOTLOADER_CTX_PAGES*PAGE_SIZE + 3*IMAGE_PAGES*PAGE_SIZE
};

pull_error memory_open_impl(mem_object* ctx, obj_id obj, mem_mode_t mode) {
    address_t offset;
    ctx->mode = mode;
    ctx->start_offset = memory_object_start[obj];
    ctx->end_offset = memory_object_end[obj];
    if (mode == WRITE_ALL) {
        for (offset = ctx->start_offset; offset < ctx->end_offset; offset += PAGE_SIZE) {
            int ret = rom_util_page_erase(offset, BUFFER_SIZE);
            if (ret != 0) {
                log_error(MEMORY_ERASE_ERROR, "Error erasing erasing page\n");
                return MEMORY_FLUSH_ERROR;
            }            
        }
    }
    return PULL_SUCCESS;
}

int memory_read_impl(mem_object* ctx, void* memory_buffer, uint16_t size, uint32_t offset) {
    uint32_t* address = (uint32_t*) ctx->start_offset+offset;
    if (memcpy(memory_buffer, address, size) != memory_buffer) {
        log_error(MEMORY_READ_ERROR, "Error reading internal memory\n");
        return -1;
    }
    return size;
}
pull_error memory_flush_impl(mem_object* ctx) {
    if (buffer_full == 0) {
        return PULL_SUCCESS;
    }
    // Using cc2538 ROM functions to interact with the flash
    // http://www.ti.com/lit/ug/swru333a/swru333a.pdf
    if (rom_util_page_erase(buffer_offset, BUFFER_SIZE) != 0) {
        log_error(MEMORY_ERASE_ERROR, "Error erasing erasing page\n");
        return MEMORY_FLUSH_ERROR;
    }
    INTERRUPTS_DISABLE();
    int32_t ret = rom_util_program_flash((uint32_t*) buffer, buffer_offset, buffer_full);
    INTERRUPTS_ENABLE();
    if (ret != 0) {
        log_error(MEMORY_WRITE_ERROR, "Error writing into flash\n");
        return MEMORY_FLUSH_ERROR;
    }
    log_debug("Flushing buffer of size %d\n", buffer_full);
    buffer_full = 0;
    buffer_offset = 0;
    return PULL_SUCCESS;
}

int memory_write_impl(mem_object* ctx, const void* memory_buffer, uint16_t size, uint32_t offset) {
    // I do not consider the case when the size of the data is bigger than the
    // bufffer
    if (size > BUFFER_SIZE) {
        return -1;
    }
    // Buffered I/O
    // 1. Check if we are writing in the page pointer by the buffer
    if ((uint32_t) ctx->start_offset+offset < buffer_offset+BUFFER_SIZE &&
            (uint32_t) ctx->start_offset+offset > buffer_offset) {
        if (buffer_full+size <= BUFFER_SIZE) {
            memcpy(&buffer[buffer_full], memory_buffer, size); /// XXX Check for errors
            buffer_full+=size; // Questo è un errore, perchè magari stavo sovrascrivendo qualcosa
        } else {
            uint16_t chunk_size = BUFFER_SIZE-buffer_full;
            memcpy(&buffer[buffer_full], memory_buffer, chunk_size); /// XXX Check for errors
            if (memory_flush_impl(ctx) != PULL_SUCCESS) {
                log_error(MEMORY_WRITE_ERROR, "Error while writing the buffer\n");
                return -1;
            }
            buffer_offset = ctx->start_offset+offset+chunk_size;
            memcpy(&buffer[buffer_full], memory_buffer+chunk_size, size-chunk_size); /// XXX Check for errors
        }
        return size;
    } // 2. if not, flush the page pointer by the buffer and start filling the buffer again
    pull_error err = memory_flush_impl(ctx);
    if (err) {
        log_error(err, "Error while writing the buffer\n");
        return -1;
    }
    buffer_offset = ctx->start_offset+offset;
    memcpy(&buffer[buffer_full], memory_buffer, size); /// XXX Check for errors
    buffer_full+=size;
    return size;
}

pull_error memory_close_impl(mem_object* ctx) {
    if (buffer_full != 0) {
        return memory_flush(ctx);
    }
    return PULL_SUCCESS;
}
