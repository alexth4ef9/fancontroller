/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"
#include "hal_serial_nor.h"

#include "fs.h"
#include "lfs.h"

struct cmdRead {
    const char *name;
    void *data;
    unsigned size;
};

struct cmdWrite {
    const char *name;
    const void *data;
    unsigned size;
};

struct cmdRename {
    const char *oldName;
    const char *newName;
};

enum FSCOMMAND {
    FSREAD,
    FSWRITE,
    FSRENAME,
};

struct cmdFs {
    enum FSCOMMAND cmd;
    union {
        struct cmdRead read;
        struct cmdWrite write;
        struct cmdRename rename;
    };
};

static const SPIConfig spiconfig2 = {
    .circular = false,
    .end_cb = NULL,
    .ssport = PORT_SPI2_NSS_D31,
    .sspad = PAD_SPI2_NSS_D31,
    .cr1 = 0,            /* 18MHz, Mode 0 */
    .cr2 = SPI_CR2_SSOE, /* Single Master */
};

static const SNORConfig snorconfig1 = {
    .busp = &SPID2,
    .buscfg = &spiconfig2,
};

static SNORDriver snor1;

int snor_read(const struct lfs_config *c,
              lfs_block_t block,
              lfs_off_t off,
              void *buffer,
              lfs_size_t size)
{
    SNORDriver *snor = (SNORDriver *)c->context;
    const flash_descriptor_t *desc = flashGetDescriptor(snor);
    flash_error_t ferr =
        flashRead(snor, block * desc->sectors_size + off, size, buffer);
    return ferr == FLASH_NO_ERROR ? 0 : LFS_ERR_IO;
}

int snor_prog(const struct lfs_config *c,
              lfs_block_t block,
              lfs_off_t off,
              const void *buffer,
              lfs_size_t size)
{
    SNORDriver *snor = (SNORDriver *)c->context;
    const flash_descriptor_t *desc = flashGetDescriptor(snor);
    flash_error_t ferr =
        flashProgram(snor, block * desc->sectors_size + off, size, buffer);
    return ferr == FLASH_NO_ERROR ? 0 : LFS_ERR_IO;
}

int snor_erase(const struct lfs_config *c, lfs_block_t block)
{
    SNORDriver *snor = (SNORDriver *)c->context;
    flash_error_t ferr = flashStartEraseSector(snor, block);
    if (ferr != FLASH_NO_ERROR) {
        return LFS_ERR_IO;
    }
    ferr = flashWaitErase((BaseFlash *)snor);
    return ferr == FLASH_NO_ERROR ? 0 : LFS_ERR_IO;
}

int snor_sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

static struct lfs_config lfscfg1 = {
    .context = &snor1,

    .read = snor_read,
    .prog = snor_prog,
    .erase = snor_erase,
    .sync = snor_sync,

    .read_size = 0,
    .prog_size = 0,
    .block_size = 0,
    .block_count = 0,
    .cache_size = 0,
    .lookahead_size = 0,
    .block_cycles = 500,
};

static THD_WORKING_AREA(waThreadFs, 1024);
static THD_FUNCTION(ThreadFs, arg)
{
    struct lfs_config *lfscfg = (struct lfs_config *)arg;
    const flash_descriptor_t *desc;
    lfs_t lfs;
    uint8_t *read_buffer;
    uint8_t *prog_buffer;
    uint8_t *lookahead_buffer;
    uint8_t *file_buffer;

    chRegSetThreadName("fs");

    spiStart(snorconfig1.busp, snorconfig1.buscfg);
    snorObjectInit(&snor1);
    snorStart(&snor1, &snorconfig1);

    desc = flashGetDescriptor(&snor1);

    read_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(read_buffer, "failed to allocate lfs read_buffer");
    prog_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(read_buffer, "failed to allocate lfs prog_buffer");
    lookahead_buffer = chCoreAllocAligned(desc->page_size, sizeof(uint32_t));
    osalDbgAssert(read_buffer, "failed to allocate lfs lookahead_buffer");
    file_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(file_buffer, "failed to allocate lfs file_buffer");

    lfscfg->read_size = desc->page_size;
    lfscfg->prog_size = desc->page_size;
    lfscfg->block_size = desc->sectors_size;
    lfscfg->block_count = desc->sectors_count;
    lfscfg->cache_size = desc->page_size;
    lfscfg->lookahead_size = desc->page_size;
    lfscfg->read_buffer = read_buffer;
    lfscfg->prog_buffer = prog_buffer;
    lfscfg->lookahead_buffer = lookahead_buffer;

    if (lfs_mount(&lfs, lfscfg)) {
        lfs_format(&lfs, lfscfg);
        lfs_mount(&lfs, lfscfg);
    }

    while (true) {
        thread_t *caller = chMsgWait();
        struct cmdFs *cmd = (struct cmdFs *)chMsgGet(caller);
        int result = -1;

        switch (cmd->cmd) {
        case FSREAD: {
            struct lfs_file_config fcfg = {
                .buffer = file_buffer,
            };
            lfs_file_t file;
            if (lfs_file_opencfg(
                    &lfs, &file, cmd->read.name, LFS_O_RDONLY, &fcfg) == 0) {
                result =
                    lfs_file_read(&lfs, &file, cmd->read.data, cmd->read.size);
                lfs_file_close(&lfs, &file);
            }
        } break;
        case FSWRITE: {
            struct lfs_file_config fcfg = {
                .buffer = file_buffer,
            };
            lfs_file_t file;
            if (lfs_file_opencfg(&lfs,
                                 &file,
                                 cmd->read.name,
                                 LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC,
                                 &fcfg) == 0) {
                result = lfs_file_write(
                    &lfs, &file, cmd->write.data, cmd->write.size);
                lfs_file_close(&lfs, &file);
            }
        } break;
        case FSRENAME: {
            result = lfs_rename(&lfs, cmd->rename.oldName, cmd->rename.newName);
        } break;
        }

        chMsgRelease(caller, result);
    }
}

static thread_t *threadFs;

void fsInit(void)
{
    threadFs = chThdCreateStatic(
        waThreadFs, sizeof(waThreadFs), NORMALPRIO, ThreadFs, &lfscfg1);
}

int fsRead(const char *name, void *data, unsigned size)
{
    struct cmdFs cmd = {
        .cmd = FSREAD,
        .read =
            {
                .name = name,
                .data = data,
                .size = size,
            },
    };

    return chMsgSend(threadFs, (msg_t)&cmd);
}

int fsWrite(const char *name, const void *data, unsigned size)
{
    struct cmdFs cmd = {
        .cmd = FSWRITE,
        .write =
            {
                .name = name,
                .data = data,
                .size = size,
            },
    };

    return chMsgSend(threadFs, (msg_t)&cmd);
}

int fsRename(const char *oldName, const char *newName)
{
    struct cmdFs cmd = {
        .cmd = FSRENAME,
        .rename =
            {
                .oldName = oldName,
                .newName = newName,
            },
    };

    return chMsgSend(threadFs, (msg_t)&cmd);
}
