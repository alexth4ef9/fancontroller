/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include "ex_thermometer.h"

#define EX_TMP117_VERSION "1.0.0"

#define EX_TMP117_MAJOR 1
#define EX_TMP117_MINOR 0
#define EX_TMP117_PATCH 0


#define EX_TMP117_REG_TEMP_RESULT 0x00
#define EX_TMP117_REG_CONFIG 0x01
#define EX_TMP117_REG_THIGH_LIMIT 0x02
#define EX_TMP117_REG_TLOW_LIMIT 0x03
#define EX_TMP117_REG_EEPROM_UL 0x04
#define EX_TMP117_REG_EEPROM1 0x05
#define EX_TMP117_REG_EEPROM2 0x06
#define EX_TMP117_REG_TEMP_OFFSET 0x07
#define EX_TMP117_REG_EEPROM3 0x08
#define EX_TMP117_REG_DEV_ID 0x0f

#define EX_TMP117_TEMP_LSB 0.0078125f

#define EX_TMP117_CONFIG_RESET (1 << 1)
#define EX_TMP117_CONFIG_DR_ALERT (1 << 2)
#define EX_TMP117_CONFIG_POL (1 << 3)
#define EX_TMP117_CONFIG_T_NALERT (1 << 4)
#define EX_TMP117_CONFIG_AVG0 (1 << 5)
#define EX_TMP117_CONFIG_AVG1 (1 << 6)
#define EX_TMP117_CONFIG_CONV0 (1 << 7)
#define EX_TMP117_CONFIG_CONV1 (1 << 8)
#define EX_TMP117_CONFIG_CONV2 (1 << 9)
#define EX_TMP117_CONFIG_MOD0 (1 << 10)
#define EX_TMP117_CONFIG_MOD1 (1 << 11)
#define EX_TMP117_CONFIG_EEPROM_BUSY (1 << 12)
#define EX_TMP117_CONFIG_DATA_READY (1 << 13)
#define EX_TMP117_CONFIG_LOW_ALERT (1 << 14)
#define EX_TMP117_CONFIG_HIGH_ALERT (1 << 15)

#define EX_TMP117_DEV_ID 0x0117


#if !defined(TMP117_USE_I2C)
#define TMP117_USE_I2C TRUE
#endif

#if !defined(TMP117_SHARED_I2C)
#define TMP117_SHARED_I2C FALSE
#endif

#if !defined(TMP117_NICE_WAITING)
#define TMP117_NICE_WAITING TRUE
#endif

#if TMP117_USE_I2C && !HAL_USE_I2C
#error "TMP117_USE_I2C requires HAL_USE_I2C"
#endif

#if TMP117_SHARED_I2C && !I2C_USE_MUTUAL_EXCLUSION
#error "TMP117_SHARED_I2C requires I2C_USE_MUTUAL_EXCLUSION"
#endif

typedef struct TMP117Driver TMP117Driver;

typedef enum {
    TMP117_SAD_0 = 0x48,
    TMP117_SAD_1 = 0x49,
    TMP117_SAD_2 = 0x4a,
    TMP117_SAD_3 = 0x4b,
    TMP117_SAD_DEFAULT = TMP117_SAD_0,
} tmp117_sad_t;

typedef enum {
    TMP117_UNINIT = 0,
    TMP117_STOP = 1,
    TMP117_READY = 2,
    TMP117_NOTFOUND = 3,
} tmp117_state_t;

typedef enum {
    TMP117_AVG_0 = 0,
    TMP117_AVG_8 = 1,
    TMP117_AVG_32 = 2,
    TMP117_AVG_64 = 3,
} tmp117_average_mode_t;

typedef enum {
    TMP117_CONV_15_5 = 0,
    TMP117_CONV_125 = 1,
    TMP117_CONV_250 = 2,
    TMP117_CONV_500 = 3,
    TMP117_CONV_1000 = 4,
    TMP117_CONV_4000 = 5,
    TMP117_CONV_8000 = 6,
    TMP117_CONV_16000 = 7,
} tmp117_conversion_cycle_time_t;

typedef struct {
#if TMP117_USE_I2C
    I2CDriver *i2cp;
    const I2CConfig *i2ccfg;
    tmp117_sad_t slaveaddress;
    tmp117_conversion_cycle_time_t cycletime;
    tmp117_average_mode_t avgmode;
#endif
} TMP117Config;

#define _tmp117_methods_alone

#define _tmp117_methods _base_object_methods _tmp117_methods_alone

struct TMP117VMT {
    _tmp117_methods
};

#define _tmp117_data                                                           \
    tmp117_state_t state;                                                      \
    const TMP117Config *config;

struct TMP117Driver {
    const struct BaseThermometerVMT *vmt;
    BaseThermometer current_if;
    _tmp117_data
};

#ifdef __cplusplus
extern "C" {
#endif
void tmp117ObjectInit(TMP117Driver *devp);
void tmp117Start(TMP117Driver *devp, const TMP117Config *config);
void tmp117Stop(TMP117Driver *devp);
#ifdef __cplusplus
}
#endif
