/* SPDX-License-Identifier: GPL-3.0-only
   modified by alexth4ef9 for fan controller project
*/

/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_flash_device.c
 * @brief   Winbond W25Q serial flash driver code.
 *
 * @addtogroup WINBOND_W25Q
 * @{
 */

#include <string.h>

#include "hal.h"
#include "hal_serial_nor.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define PAGE_SIZE 256U
#define PAGE_MASK (PAGE_SIZE - 1U)

#if W25Q_USE_SUB_SECTORS == TRUE
#define SECTOR_SIZE 0x00001000U
#define CMD_SECTOR_ERASE W25Q_CMD_SECTOR_ERASE
#else
#define SECTOR_SIZE 0x00010000U
#define CMD_SECTOR_ERASE W25Q_CMD_BLOCK_ERASE_64K
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   N25Q128 descriptor.
 */
flash_descriptor_t snor_descriptor = {
    .attributes = FLASH_ATTR_ERASED_IS_ONE | FLASH_ATTR_REWRITABLE |
                  FLASH_ATTR_SUSPEND_ERASE_CAPABLE,
    .page_size = 256U,
    .sectors_count = 0U, /* It is overwritten.*/
    .sectors = NULL,
    .sectors_size = SECTOR_SIZE,
    .address = 0U,
    .size = 0U /* It is overwritten.*/

};

#if (SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI) || defined(__DOXYGEN__)
#if (WSPI_SUPPORTS_MEMMAP == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Fast read command for memory mapped mode.
 */
const wspi_command_t snor_memmap_read = {
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
    .cmd = W25Q_CMD_FAST_READ,
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
    .cmd = W25Q_CMD_FAST_READ_2O,
#else
    .cmd = W25Q_CMD_FAST_READ_4O,
#endif
    .addr = 0,
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
    .dummy = W25Q_READ_DUMMY_CYCLES,
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
    .dummy = W25Q_READ_DUMMY_CYCLES,
#else
    .dummy = W25Q_READ_DUMMY_CYCLES / 2,
#endif
    .cfg = WSPI_CFG_ADDR_SIZE_24 |
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
           WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
           WSPI_CFG_DATA_MODE_ONE_LINE |
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
           WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_ONE_LINES |
           WSPI_CFG_DATA_MODE_TWO_LINES |
#else
           WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_ONE_LINES |
           WSPI_CFG_DATA_MODE_FOUR_LINES |
#endif
           WSPI_CFG_ALT_MODE_FOUR_LINES | /* Always 4 lines, note.*/
           WSPI_CFG_ALT_SIZE_8 | WSPI_CFG_SIOO};
#endif
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI
/* Initial N25Q_CMD_READ_ID command.*/
static const wspi_command_t w25q_cmd_read_id = {
    .cmd = W25Q_CMD_READ_ID,
    .cfg = 0U |
#if W25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
           WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_DATA_MODE_ONE_LINE,
#elif W25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
           WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_DATA_MODE_ONE_LINES,
#else
           WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_DATA_MODE_ONE_LINES,
#endif
    .addr = 0,
    .alt = 0,
    .dummy = 0};

/* Initial W25Q_CMD_WRITE_ENABLE command.*/
static const wspi_command_t w25q_cmd_write_enable = {
    .cmd = W25Q_CMD_WRITE_ENABLE,
    .cfg = 0U |
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
           WSPI_CFG_CMD_MODE_ONE_LINE,
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
           WSPI_CFG_CMD_MODE_ONE_LINES,
#else
           WSPI_CFG_CMD_MODE_ONE_LINES,
#endif
    .addr = 0,
    .alt = 0,
    .dummy = 0};
#endif /* SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool w25q_find_id(const uint8_t *set, size_t size, uint8_t element)
{
    size_t i;

    for (i = 0; i < size; i++) {
        if (set[i] == element) {
            return true;
        }
    }
    return false;
}

static flash_error_t w25q_poll_status(SNORDriver *devp)
{
    uint8_t sts;

    do {
#if W25Q_NICE_WAITING == TRUE
        osalThreadSleepMilliseconds(1);
#endif
        /* Read status command.*/
        bus_cmd_receive(devp->config->busp, W25Q_CMD_READ_STATUS1, 1, &sts);
    } while ((sts & W25Q_FLAGS_BUSY) != 0U);

    return FLASH_NO_ERROR;
}

#if SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI
static void w25q_reset_memory(SNORDriver *devp)
{
    /* 1x W25Q_CMD_ENABLE_RESET command.*/
    static const wspi_command_t cmd_reset_enable = {
        .cmd = W25Q_CMD_ENABLE_RESET,
        .cfg = WSPI_CFG_CMD_MODE_ONE_LINE,
        .addr = 0,
        .alt = 0,
        .dummy = 0};

    /* 1x W25Q_CMD_RESET_DEVICE command.*/
    static const wspi_command_t cmd_reset_device = {
        .cmd = W25Q_CMD_RESET_DEVICE,
        .cfg = WSPI_CFG_CMD_MODE_ONE_LINE,
        .addr = 0,
        .alt = 0,
        .dummy = 0};

    wspiCommand(devp->config->busp, &cmd_reset_enable);
    wspiCommand(devp->config->busp, &cmd_reset_device);
}
#endif /* SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI */

static const uint8_t w25q_manufacturer_ids[] = W25Q_SUPPORTED_MANUFACTURE_IDS;
static const uint8_t w25q_memory_type_ids[] = W25Q_SUPPORTED_MEMORY_TYPE_IDS;

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void snor_device_init(SNORDriver *devp)
{

    /* Reading device ID.*/
    bus_cmd_receive(devp->config->busp,
                    W25Q_CMD_READ_ID,
                    sizeof devp->device_id,
                    devp->device_id);

#if SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI
    /* Reset device.*/
    w25q_reset_memory(devp);
#endif /* SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI */

    /* Checking if the device is white listed.*/
    osalDbgAssert(w25q_find_id(w25q_manufacturer_ids,
                               sizeof w25q_manufacturer_ids,
                               devp->device_id[0]),
                  "invalid manufacturer id");
    osalDbgAssert(w25q_find_id(w25q_memory_type_ids,
                               sizeof w25q_memory_type_ids,
                               devp->device_id[1]),
                  "invalid memory type id");

    /* Setting up the device size.*/
    snor_descriptor.sectors_count =
        (1U << (size_t)devp->device_id[2]) / SECTOR_SIZE;
    snor_descriptor.size = (size_t)snor_descriptor.sectors_count * SECTOR_SIZE;
}

flash_error_t
snor_device_read(SNORDriver *devp, flash_offset_t offset, size_t n, uint8_t *rp)
{

#if SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
    /* Normal read command in SPI mode.*/
    bus_cmd_addr_dummy_receive(devp->config->busp,
                               W25Q_CMD_FAST_READ,
                               offset,
                               W25Q_READ_DUMMY_CYCLES,
                               n,
                               rp);
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
    /* Fast read command in WSPI mode.*/
    bus_cmd_addr_dummy_receive(devp->config->busp,
                               W25Q_CMD_FAST_READ_2O,
                               offset,
                               W25Q_READ_DUMMY_CYCLES,
                               n,
                               rp);
#else
    /* Fast read command in WSPI mode.*/
    bus_cmd_addr_dummy_receive(devp->config->busp,
                               W25Q_CMD_FAST_READ_4O,
                               offset,
                               W25Q_READ_DUMMY_CYCLES / 2,
                               n,
                               rp);
#endif
#else
    /* Normal read command in SPI mode.*/
    bus_cmd_addr_receive(devp->config->busp, W25Q_CMD_READ_DATA, offset, n, rp);
#endif

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_program(SNORDriver *devp,
                                  flash_offset_t offset,
                                  size_t n,
                                  const uint8_t *pp)
{

    /* Data is programmed page by page.*/
    while (n > 0U) {
        flash_error_t err;

        /* Data size that can be written in a single program page operation.*/
        size_t chunk = (size_t)(((offset | PAGE_MASK) + 1U) - offset);
        if (chunk > n) {
            chunk = n;
        }

        /* Enabling write operation.*/
        bus_cmd(devp->config->busp, W25Q_CMD_WRITE_ENABLE);

        /* Page program command.*/
        bus_cmd_addr_send(
            devp->config->busp, W25Q_CMD_PAGE_PROGRAM, offset, chunk, pp);

        /* Wait for status and check errors.*/
        err = w25q_poll_status(devp);
        if (err != FLASH_NO_ERROR) {

            return err;
        }

        /* Next page.*/
        offset += chunk;
        pp += chunk;
        n -= chunk;
    }

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_start_erase_all(SNORDriver *devp)
{

    /* Enabling write operation.*/
    bus_cmd(devp->config->busp, W25Q_CMD_WRITE_ENABLE);

    /* Bulk erase command.*/
    bus_cmd(devp->config->busp, W25Q_CMD_CHIP_ERASE);

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_start_erase_sector(SNORDriver *devp,
                                             flash_sector_t sector)
{
    flash_offset_t offset = (flash_offset_t)(sector * SECTOR_SIZE);

    /* Enabling write operation.*/
    bus_cmd(devp->config->busp, W25Q_CMD_WRITE_ENABLE);

    /* Sector erase command.*/
    bus_cmd_addr(devp->config->busp, W25Q_CMD_SECTOR_ERASE, offset);

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_verify_erase(SNORDriver *devp, flash_sector_t sector)
{
    uint8_t cmpbuf[W25Q_COMPARE_BUFFER_SIZE];
    flash_offset_t offset;
    size_t n;

    /* Read command.*/
    offset = (flash_offset_t)(sector * SECTOR_SIZE);
    n = SECTOR_SIZE;
    while (n > 0U) {
        uint8_t *p;

#if SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI
#if N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI1L
        /* Normal read command in SPI mode.*/
        bus_cmd_addr_dummy_receive(devp->config->busp,
                                   W25Q_CMD_FAST_READ,
                                   offset,
                                   W25Q_READ_DUMMY_CYCLES,
                                   sizeof cmpbuf,
                                   cmpbuf);
#elif N25Q_BUS_MODE == N25Q_BUS_MODE_WSPI2L
        /* Fast read command in WSPI mode.*/
        bus_cmd_addr_dummy_receive(devp->config->busp,
                                   W25Q_CMD_FAST_READ_2O,
                                   offset,
                                   W25Q_READ_DUMMY_CYCLES,
                                   sizeof cmpbuf,
                                   cmpbuf);
#else
        /* Fast read command in WSPI mode.*/
        bus_cmd_addr_dummy_receive(devp->config->busp,
                                   W25Q_CMD_FAST_READ_4O,
                                   offset,
                                   W25Q_READ_DUMMY_CYCLES / 2,
                                   sizeof cmpbuf,
                                   cmpbuf);
#endif
#else
        /* Normal read command in SPI mode.*/
        bus_cmd_addr_receive(devp->config->busp,
                             W25Q_CMD_READ_DATA,
                             offset,
                             sizeof cmpbuf,
                             cmpbuf);
#endif

        /* Checking for erased state of current buffer.*/
        for (p = cmpbuf; p < &cmpbuf[W25Q_COMPARE_BUFFER_SIZE]; p++) {
            if (*p != 0xFFU) {
                /* Ready state again.*/
                devp->state = FLASH_READY;

                return FLASH_ERROR_VERIFY;
            }
        }

        offset += sizeof cmpbuf;
        n -= sizeof cmpbuf;
    }

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_query_erase(SNORDriver *devp, uint32_t *msec)
{
    uint8_t sts1, sts2;

    /* Read status 1 command.*/
    bus_cmd_receive(devp->config->busp, W25Q_CMD_READ_STATUS1, 1, &sts1);

    /* Read status 2 command.*/
    bus_cmd_receive(devp->config->busp, W25Q_CMD_READ_STATUS2, 1, &sts2);

    /* If the busy bit is set or the flash in a suspended state then
       report that the operation is still in progress.*/
    if (((sts1 & W25Q_FLAGS_BUSY) != 0U) ||
        ((sts2 & W25Q_FLAGS_SUSPEND) != 0U)) {

        /* Recommended time before polling again, this is a simplified
           implementation.*/
        if (msec != NULL) {
            *msec = 1U;
        }

        return FLASH_BUSY_ERASING;
    }

    return FLASH_NO_ERROR;
}

flash_error_t snor_device_read_sfdp(SNORDriver *devp,
                                    flash_offset_t offset,
                                    size_t n,
                                    uint8_t *rp)
{

    (void)devp;
    (void)rp;
    (void)offset;
    (void)n;

    return FLASH_NO_ERROR;
}

#if (SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI) || defined(__DOXYGEN__)
void snor_activate_xip(SNORDriver *devp) { (void)devp; }

void snor_reset_xip(SNORDriver *devp) { (void)devp; }
#endif /* SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI */

/** @} */
