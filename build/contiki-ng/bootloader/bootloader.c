#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "driverlib/flash.h"
#include "driverlib/vims.h"
#include "dev/watchdog.h"

#include "memory/metadata.h"
#include "memory/memory_objects.h"
#include "memory/memory.h"
#include "common/error.h"
#include "security/verifier.h"
#include "security/sha256.h"

#include "common/external.h"
#include "bootloader.h"
#include "../memory_headers.h"

#ifdef WITH_CRYPTOAUTHLIB
#include <cryptoauthlib.h>
#endif

#include "common/logger.h"

static const uint8_t x[32] = {
    0x8b,0x27,0x39,0x67,0x01,0x4b,0x1c,0xae,0xfe,0x8a,0x18,0x6e,0xea,0x27,0x86,0x34,
    0x0e,0xea,0x35,0x3d,0x8c,0x65,0xf6,0x59,0xfc,0xcb,0x23,0xd7,0xfa,0xab,0x7b,0x18
};
static const uint8_t y[32] = {
    0x14,0x75,0x33,0xec,0x17,0xb7,0x54,0x50,0xca,0x98,0x35,0xad,0x58,0xbe,0xd5,0xfa,
    0x48,0xbc,0xa0,0x24,0x81,0xba,0xfa,0x3d,0xcd,0x8d,0x5a,0x7f,0x40,0xbc,0x70,0x94
};

#define RECOVERY_IMAGE 0
#define MEMORY_OBJ_BUFFER_SIZE 0x100

static metadata mt;
static bootloader_ctx ctx;
static mem_object obj;
static obj_id id;
static mem_object obj_t, obj_dst_t;
uint8_t buffer[MEMORY_OBJ_BUFFER_SIZE];

static const obj_id internal_firmware = OBJ_RUN;

void flash_write_protect() {
    uint32_t page = 0;
    for (page=0; page <= 31; page++) {
        FlashProtectionSet(page, FLASH_WRITE_PROTECT);
    }
}

void pull_bootloader() {
    log_debug("Loading bootloader context\n");
    pull_error err = memory_open(&obj, BOOTLOADER_CTX);
    if (err) {
        log_error(err, "Opening Bootoader Ctx\n");
        goto error;
    }
    if (memory_read(&obj, &ctx, sizeof(ctx), 0x0) != sizeof(ctx)) {
        err = MEMORY_READ_ERROR;
        log_error(err, "Reading Bootoader Ctx\n");
        goto error;
    }
    if ((ctx.startup_flags & FIRST_BOOT) == FIRST_BOOT) {
        log_info("First Run\n");
        log_debug("Erasing slots\n");
        obj_id i;
        memset(&mt, 0x0, sizeof(mt));
        for (i = 0; memory_objects[i] >= 0; i++) {
            err = write_firmware_metadata(memory_objects[i], &mt, &obj_t);
            if (err) {
                log_error(err, "Error erasing firmware\n");
                goto error; 
            }
        }
#if RECOVERY_IMAGE
        log_debug("Storing Recovery Image\n");
        watchdog_stop();
        err = copy_firmware(internal_firmware, OBJ_GOLD, &obj_t, &obj_dst_t, buffer, MEMORY_OBJ_BUFFER_SIZE);
        watchdog_start();
        if (err) {
            log_error(err, "Error storing recovery image\n");
            goto error;
        }
#endif
        ctx.startup_flags = 0x0;
        if (memory_write(&obj, &ctx, sizeof(ctx), 0x0) != sizeof(ctx)) {
            log_warn("Error writing bootloader context\n");
            goto error;
        };
        log_info("Bootstrap success\n");
    }
    memory_close(&obj); // close bootloader context
    log_debug("Reading metadata of the internal image\n");
    err = read_firmware_metadata(internal_firmware, &mt, &obj_t);
    if (err) {
        log_error(err, "Failed reading internal firmware metadata\n");
        goto error;
    }
    log_info("Internal Firmware: %u\n", get_version(&mt));
    uint16_t version = 0; 
    err = get_newest_firmware(&id, &version, &obj_t);
    if (err) {
        log_error(err, "Failed getting newest firmware\n");
        goto error;
    }
    log_debug("Newest firmware is in slot %u with version %x\n", id, version);
    if (version > get_version(&mt)) {
        // TODO Here you should verify the signature before even copying it into
        // internal memory
        log_info("Updating system to version: %x\n", version);
        int page = 0;
        for (page=5; page<=5+26; page++) {
            if (FlashSectorErase(0x1000*page) != FAPI_STATUS_SUCCESS) {
                log_info("Error erasing page %d\n", page);
            }
        }
        if (page > 31) {
            err = copy_firmware(id, internal_firmware, &obj_t, &obj_dst_t, buffer, MEMORY_OBJ_BUFFER_SIZE);
            if (err) {
                log_error(err, "Failure updating to the new firmware\n");
            }
        }
    }
error:
    log_info("Validating internal image\n");
    watchdog_stop();
#ifdef WITH_CRYPTOAUTHLIB
    ATCA_STATUS status = atcab_init(&cfg_ateccx08a_i2c_default);
    if (status != ATCA_SUCCESS) {
        log_error(GENERIC_ERROR, "Failure initializing ATECC508A\n");
    }
#endif
    err = verify_object(internal_firmware, cryptoauthlib_digest_sha256, x, y, secp256r1, &obj_t, buffer, MEMORY_OBJ_BUFFER_SIZE);
#ifdef WITH_CRYPTOAUTHLIB
    if (status == ATCA_SUCCESS) {
        atcab_release();
    }
#endif
    watchdog_start();
    printf("Verification %s\n", err? "failed":"successfull");
    log_info("Protecting flash memory\n");
    flash_write_protect();
    log_info("Loading the internal firmware\n");
    load_object(internal_firmware, &mt);
}
