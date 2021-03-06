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
 * @file    hal_flash_device.h
 * @brief   Winbond W25Q serial flash driver header.
 *
 * @addtogroup WINBOND_W25Q
 * @{
 */

#ifndef HAL_FLASH_DEVICE_H
#define HAL_FLASH_DEVICE_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Device capabilities
 * @{
 */
#define SNOR_DEVICE_SUPPORTS_XIP FALSE
/** @} */

/**
 * @name    Device identification
 * @{
 */
#define W25Q_SUPPORTED_MANUFACTURE_IDS                                         \
    {                                                                          \
        0xef                                                                   \
    }
#define W25Q_SUPPORTED_MEMORY_TYPE_IDS                                         \
    {                                                                          \
        0x70                                                                   \
    }
/** @} */

/**
 * @name    Command codes
 * @{
 */
#define W25Q_CMD_WRITE_ENABLE 0x06
#define W25Q_CMD_VOLATILE_SR_WRITE_ENABLE 0x50
#define W25Q_CMD_WRITE_DISABLE 0x04
#define W25Q_CMD_RELEASE_POWERDOWN 0xab
#define W25Q_CMD_READ_MANUFACTURER_DEVICE_ID 0x90
#define W25Q_CMD_READ_ID 0x9f
#define W25Q_CMD_READ_UNIQUE_ID 0x4b
#define W25Q_CMD_READ_DATA 0x03
#define W25Q_CMD_FAST_READ 0xb0
#define W25Q_CMD_PAGE_PROGRAM 0x02
#define W25Q_CMD_SECTOR_ERASE 0x20
#define W25Q_CMD_BLOCK_ERASE_32K 0x52
#define W25Q_CMD_BLOCK_ERASE_64K 0xd8
#define W25Q_CMD_CHIP_ERASE 0xc7
#define W25Q_CMD_READ_STATUS1 0x05
#define W25Q_CMD_WRITE_STATUS1 0x01
#define W25Q_CMD_READ_STATUS2 0x35
#define W25Q_CMD_WRITE_STATUS2 0x31
#define W25Q_CMD_READ_STATUS3 0x15
#define W25Q_CMD_WRITE_STATUS3 0x11
#define W25Q_CMD_READ_SFDP 0x5a
#define W25Q_CMD_ERASE_SECURITY 0x44
#define W25Q_CMD_PROGRAM_SECURITY 0x42
#define W25Q_CMD_READ_SECURITY 0x48
#define W25Q_CMD_GLOBAL_BLOCK_LOCK 0x7e
#define W25Q_CMD_GLOBAL_BLOCK_UNLOCK 0x98
#define W25Q_CMD_READ_BLOCK_LOCK 0x3d
#define W25Q_CMD_INDIVIDUAL_BLOCK_LOCK 0x36
#define W25Q_CMD_INDIVIDUAL_BLOCK_UNLOCK 0x39
#define W25Q_CMD_ERASE_PROGRAM_SUSPEND 0x75
#define W25Q_CMD_ERASE_PROGRAM_RESUME 0x7a
#define W25Q_CMD_POWERDOWN 0xb9
#define W25Q_CMD_ENABLE_RESET 0x66
#define W25Q_CMD_RESET_DEVICE 0x99

#define W25Q_CMD_FAST_READ_2O 0x3b
#define W25Q_CMD_FAST_READ_2IO 0xbb
#define W25Q_CMD_READ_MANUFACTURER_DEVICE_ID_2IO 0x92

#define W25Q_CMD_PAGE_READ_4 0x32
#define W25Q_CMD_FAST_READ_4O 0x6b
#define W25Q_CMD_READ_MANUFACTURER_DEVICE_ID_4 0x94
#define W25Q_CMD_FAST_READ_4IO 0xeb
#define W25Q_CMD_SET_BURST_WITH_WRAP 0x77
/** @} */

/**
 * @name    Flags status register bits
 * @{
 */
#define W25Q_FLAGS_BUSY 0x01U
#define W25Q_FLAGS_WRITE_ENABLE 0x02U
#define W25Q_FLAGS_BLOCK_PROTECT_0 0x04U
#define W25Q_FLAGS_BLOCK_PROTECT_1 0x08U
#define W25Q_FLAGS_BLOCK_PROTECT_2 0x10U
#define W25Q_FLAGS_BLOCK_PROTECT_3 0x20U
#define W25Q_FLAGS_TOP_BOTTOM_PROTECT 0x40U

#define W25Q_FLAGS_STATUS_REGISTER_LOCK 0x01U
#define W25Q_FLAGS_QUAD_ENABLE 0x02U
#define W25Q_FLAGS_SECURITY_LOCK_1 0x08U
#define W25Q_FLAGS_SECURITY_LOCK_2 0x10U
#define W25Q_FLAGS_SECURITY_LOCK_3 0x20U
#define W25Q_FLAGS_COMPLEMENT_PROTECT 0x40U
#define W25Q_FLAGS_SUSPEND 0x80U

#define W25Q_FLAGS_WRITE_PROTECT_SELECTION 0x04U
#define W25Q_FLAGS_OUTPUT_DRIVER_STRENGTH_0 0x20U
#define W25Q_FLAGS_OUTPUT_DRIVER_STRENGTH_1 0x40U
/** @} */

/**
 * @name    Bus interface modes.
 * @{
 */
#define W25Q_BUS_MODE_WSPI1L 1U
#define W25Q_BUS_MODE_WSPI2L 2U
#define W25Q_BUS_MODE_WSPI4L 4U
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Device bus mode to be used.
 * #note    if @p W25Q_SWITCH_WIDTH is @p FALSE then this is the bus mode
 *          that the device is expected to be using.
 * #note    if @p W25Q_SWITCH_WIDTH is @p TRUE then this is the bus mode
 *          that the device will be switched in.
 * @note    This option is only valid in WSPI bus mode.
 */
#if !defined(W25Q_BUS_MODE) || defined(__DOXYGEN__)
#define W25Q_BUS_MODE W25Q_BUS_MODE_WSPI4L
#endif

/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts delays into the flash waiting
 *          routines releasing some extra CPU time for threads with lower
 *          priority, this may slow down the driver a bit however.
 */
#if !defined(W25Q_NICE_WAITING) || defined(__DOXYGEN__)
#define W25Q_NICE_WAITING TRUE
#endif

/**
 * @brief   Uses 4kB sub-sectors rather than 64kB sectors.
 */
#if !defined(W25Q_USE_SUB_SECTORS) || defined(__DOXYGEN__)
#define W25Q_USE_SUB_SECTORS FALSE
#endif

/**
 * @brief   Size of the compare buffer.
 * @details This buffer is allocated in the stack frame of the function
 *          @p flashVerifyErase() and its size must be a power of two.
 *          Larger buffers lead to better verify performance but increase
 *          stack usage for that function.
 */
#if !defined(W25Q_COMPARE_BUFFER_SIZE) || defined(__DOXYGEN__)
#define W25Q_COMPARE_BUFFER_SIZE 32
#endif

/**
 * @brief   Number of dummy cycles for fast read (1..15).
 * @details This is the number of dummy cycles to be used for fast read
 *          operations.
 */
#if !defined(W25Q_READ_DUMMY_CYCLES) || defined(__DOXYGEN__)
#define W25Q_READ_DUMMY_CYCLES 8
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (W25Q_COMPARE_BUFFER_SIZE & (W25Q_COMPARE_BUFFER_SIZE - 1)) != 0
#error "invalid W25Q_COMPARE_BUFFER_SIZE value"
#endif

#if (W25Q_READ_DUMMY_CYCLES < 1) || (W25Q_READ_DUMMY_CYCLES > 15)
#error "invalid W25Q_READ_DUMMY_CYCLES value (1..15)"
#endif

#if (W25Q_BUS_MODE == W25Q_BUS_MODE_WSPI4L) || defined(__DOXYGEN__)
/**
 * @brief   WSPI settings for command only.
 */
#define SNOR_WSPI_CFG_CMD                                                      \
    (WSPI_CFG_CMD_MODE_ON_LINES | WSPI_CFG_ADDR_MODE_NONE |                    \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

/**
 * @brief   WSPI settings for command and address.
 */
#define SNOR_WSPI_CFG_CMD_ADDR                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_ONE_LINES |              \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

/**
 * @brief   WSPI settings for command and data.
 */
#define SNOR_WSPI_CFG_CMD_DATA                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_NONE |                   \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_FOUR_LINES |                  \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

/**
 * @brief   WSPI settings for command, address and data.
 */
#define SNOR_WSPI_CFG_CMD_ADDR_DATA                                            \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_ONE_LINES |              \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_FOUR_LINES |                  \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

#elif W25Q_BUS_MODE == W25Q_BUS_MODE_WSPI2L
#define SNOR_WSPI_CFG_CMD                                                      \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_NONE |                   \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_ADDR                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_ONE_LINES |              \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_DATA                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINES | WSPI_CFG_ADDR_MODE_NONE |                   \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_TWO_LINES |                   \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_ADDR_DATA                                            \
    (WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |                \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_ONE_LINE |                    \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

#elif W25Q_BUS_MODE == W25Q_BUS_MODE_WSPI1L
#define SNOR_WSPI_CFG_CMD                                                      \
    (WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_NONE |                    \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_ADDR                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |                \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8 |  \
     WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_DATA                                                 \
    (WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_NONE |                    \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_ONE_LINE |                    \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

#define SNOR_WSPI_CFG_CMD_ADDR_DATA                                            \
    (WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |                \
     WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_ONE_LINE |                    \
     WSPI_CFG_CMD_SIZE_8 | WSPI_CFG_ADDR_SIZE_24)

#else
#error "invalid W25Q_BUS_MODE setting"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern flash_descriptor_t snor_descriptor;
#endif

#if (SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI) && (WSPI_SUPPORTS_MEMMAP == TRUE)
extern const wspi_command_t snor_memmap_read;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void snor_device_init(SNORDriver *devp);
flash_error_t snor_device_read(SNORDriver *devp,
                               flash_offset_t offset,
                               size_t n,
                               uint8_t *rp);
flash_error_t snor_device_program(SNORDriver *devp,
                                  flash_offset_t offset,
                                  size_t n,
                                  const uint8_t *pp);
flash_error_t snor_device_start_erase_all(SNORDriver *devp);
flash_error_t snor_device_start_erase_sector(SNORDriver *devp,
                                             flash_sector_t sector);
flash_error_t snor_device_verify_erase(SNORDriver *devp, flash_sector_t sector);
flash_error_t snor_device_query_erase(SNORDriver *devp, uint32_t *msec);
flash_error_t snor_device_read_sfdp(SNORDriver *devp,
                                    flash_offset_t offset,
                                    size_t n,
                                    uint8_t *rp);
#if (SNOR_BUS_DRIVER == SNOR_BUS_DRIVER_WSPI) &&                               \
    (SNOR_DEVICE_SUPPORTS_XIP == TRUE)
void snor_activate_xip(SNORDriver *devp);
void snor_reset_xip(SNORDriver *devp);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_FLASH_DEVICE_H */

/** @} */
