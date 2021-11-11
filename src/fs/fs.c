/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

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

static THD_FUNCTION(ThreadFs, arg)
{
    const SNORConfig *snorconfig = (SNORConfig *)arg;
    SNORDriver snor;
    const flash_descriptor_t *desc;

    struct lfs_config lfscfg = {
        .context = &snor,

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
    lfs_t lfs;

    uint8_t *read_buffer;
    uint8_t *prog_buffer;
    uint8_t *lookahead_buffer;
    uint8_t *file_buffer;

    chRegSetThreadName("fs");

    spiStart(snorconfig->busp, snorconfig->buscfg);
    snorObjectInit(&snor);
    snorStart(&snor, snorconfig);

    desc = flashGetDescriptor(&snor);

    read_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(read_buffer, "failed to allocate lfs read_buffer");
    prog_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(read_buffer, "failed to allocate lfs prog_buffer");
    lookahead_buffer = chCoreAllocAligned(desc->page_size, sizeof(uint32_t));
    osalDbgAssert(read_buffer, "failed to allocate lfs lookahead_buffer");
    file_buffer = chCoreAlloc(desc->page_size);
    osalDbgAssert(file_buffer, "failed to allocate lfs file_buffer");

    lfscfg.read_size = desc->page_size;
    lfscfg.prog_size = desc->page_size;
    lfscfg.block_size = desc->sectors_size;
    lfscfg.block_count = desc->sectors_count;
    lfscfg.cache_size = desc->page_size;
    lfscfg.lookahead_size = desc->page_size;
    lfscfg.read_buffer = read_buffer;
    lfscfg.prog_buffer = prog_buffer;
    lfscfg.lookahead_buffer = lookahead_buffer;

    if (lfs_mount(&lfs, &lfscfg)) {
        lfs_format(&lfs, &lfscfg);
        lfs_mount(&lfs, &lfscfg);
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

thread_t *
fsStart(void *wsp, size_t size, tprio_t prio, const SNORConfig *snorconfig)
{
    return chThdCreateStatic(
        wsp, size, prio, ThreadFs, (void *)(const void *)snorconfig);
}

int fsRead(thread_t *threadFs, const char *name, void *data, unsigned size)
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

int fsWrite(thread_t *threadFs,
            const char *name,
            const void *data,
            unsigned size)
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

int fsRename(thread_t *threadFs, const char *oldName, const char *newName)
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
