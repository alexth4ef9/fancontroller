/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include "ex_current.h"

#define EX_INA3221_VERSION "1.0.0"

#define EX_INA3221_MAJOR 1
#define EX_INA3221_MINOR 0
#define EX_INA3221_PATCH 0


#define INA3221_NUM_CHANNELS 3


#define EX_INA3221_REG_CONFIG 0x01

#define EX_INA3221_REG_CHANNEL1_SHUNT_VOLTAGE 0x01
#define EX_INA3221_REG_CHANNEL1_BUS_VOLTAGE 0x02
#define EX_INA3221_REG_CHANNEL2_SHUNT_VOLTAGE 0x03
#define EX_INA3221_REG_CHANNEL2_BUS_VOLTAGE 0x04
#define EX_INA3221_REG_CHANNEL3_SHUNT_VOLTAGE 0x05
#define EX_INA3221_REG_CHANNEL3_BUS_VOLTAGE 0x06

#define EX_INA3221_REG_CHANNEL1_CRITICAL_LIMIT 0x07
#define EX_INA3221_REG_CHANNEL1_WARNING_LIMIT 0x08
#define EX_INA3221_REG_CHANNEL2_CRITICAL_LIMIT 0x09
#define EX_INA3221_REG_CHANNEL2_WARNING_LIMIT 0x0a
#define EX_INA3221_REG_CHANNEL3_CRITICAL_LIMIT 0x0b
#define EX_INA3221_REG_CHANNEL3_WARNING_LIMIT 0x0c

#define EX_INA3221_REG_SHUNT_VOLTAGE_SUM 0x0d
#define EX_INA3221_REG_SHUNT_VOLTAGE_LIMIT 0x0e
#define EX_INA3221_REG_MASK_ENABLE 0x0f
#define EX_INA3221_REG_POWER_VALID_UPPER_LIMIT 0x10
#define EX_INA3221_REG_POWER_VALID_LOWER_LIMIT 0x11

#define EX_INA3221_REG_MANUFACTURER_ID 0xfe
#define EX_INA3221_REG_DIE_ID 0xff

#define EX_INA3221_CONFIG_MODE1 (1 << 0)
#define EX_INA3221_CONFIG_MODE2 (1 << 1)
#define EX_INA3221_CONFIG_MODE3 (1 << 2)
#define EX_INA3221_CONFIG_SHUNT_CT0 (1 << 3)
#define EX_INA3221_CONFIG_SHUNT_CT1 (1 << 4)
#define EX_INA3221_CONFIG_SHUNT_CT2 (1 << 5)
#define EX_INA3221_CONFIG_BUS_CT0 (1 << 6)
#define EX_INA3221_CONFIG_BUS_CT1 (1 << 7)
#define EX_INA3221_CONFIG_BUS_CT2 (1 << 8)
#define EX_INA3221_CONFIG_AVG0 (1 << 9)
#define EX_INA3221_CONFIG_AVG1 (1 << 10)
#define EX_INA3221_CONFIG_AVG2 (1 << 11)
#define EX_INA3221_CONFIG_CH1EN (1 << 12)
#define EX_INA3221_CONFIG_CH2EN (1 << 13)
#define EX_INA3221_CONFIG_CH3EN (1 << 14)
#define EX_INA3221_CONFIG_RST (1 << 15)

#define EX_INA3221_SHUNT_LSB 0.00004f
#define EX_INA3221_BUS_LSB 0.008f

#define EX_INA3221_MASK_EN_CVRF (1 << 0)
#define EX_INA3221_MASK_EN_TCF (1 << 1)
#define EX_INA3221_MASK_EN_PVF (1 << 2)
#define EX_INA3221_MASK_EN_WF1 (1 << 3)
#define EX_INA3221_MASK_EN_WF2 (1 << 4)
#define EX_INA3221_MASK_EN_WF3 (1 << 5)
#define EX_INA3221_MASK_EN_SF (1 << 6)
#define EX_INA3221_MASK_EN_CF1 (1 << 7)
#define EX_INA3221_MASK_EN_CF2 (1 << 8)
#define EX_INA3221_MASK_EN_CF3 (1 << 9)
#define EX_INA3221_MASK_EN_CEN (1 << 10)
#define EX_INA3221_MASK_EN_WEN (1 << 11)
#define EX_INA3221_MASK_EN_SCC1 (1 << 12)
#define EX_INA3221_MASK_EN_SCC2 (1 << 13)
#define EX_INA3221_MASK_EN_SCC3 (1 << 14)

#define EX_INA3221_MANUFACTURER_ID 0x5449
#define EX_INA3221_DIE_ID 0x3220


#if !defined(INA3221_USE_I2C)
#define INA3221_USE_I2C TRUE
#endif

#if !defined(INA3221_SHARED_I2C)
#define INA3221_SHARED_I2C FALSE
#endif

#if !defined(INA3221_NICE_WAITING)
#define INA3221_NICE_WAITING TRUE
#endif

#if INA3221_USE_I2C && !HAL_USE_I2C
#error "INA3221_USE_I2C requires HAL_USE_I2C"
#endif

#if INA3221_SHARED_I2C && !I2C_USE_MUTUAL_EXCLUSION
#error "INA3221_SHARED_I2C requires I2C_USE_MUTUAL_EXCLUSION"
#endif

typedef struct INA3221Driver INA3221Driver;

typedef enum {
    INA3221_SAD_0 = 0x40,
    INA3221_SAD_1 = 0x41,
    INA3221_SAD_2 = 0x42,
    INA3221_SAD_3 = 0x43,
    INA3221_SAD_DEFAULT = INA3221_SAD_0,
} ina3221_sad_t;

typedef enum {
    INA3221_UNINIT = 0,
    INA3221_STOP = 1,
    INA3221_READY = 2,
    INA3221_NOTFOUND = 3,
} ina3221_state_t;

typedef enum {
    INA3221_CT_140 = 0,
    INA3221_CT_204 = 1,
    INA3221_CT_332 = 2,
    INA3221_CT_588 = 3,
    INA3221_CT_1100 = 4,
    INA3221_CT_2116 = 5,
    INA3221_CT_4156 = 6,
    INA3221_CT_8244 = 7,
} ina3221_conversion_time_t;

typedef enum {
    INA3221_AVG_1 = 0,
    INA3221_AVG_4 = 1,
    INA3221_AVG_16 = 2,
    INA3221_AVG_64 = 3,
    INA3221_AVG_128 = 4,
    INA3221_AVG_256 = 5,
    INA3221_AVG_512 = 6,
    INA3221_AVG_1024 = 7,
} ina3221_average_mode_t;

typedef struct {
#if INA3221_USE_I2C
    I2CDriver *i2cp;
    const I2CConfig *i2ccfg;
    ina3221_sad_t slaveaddress;
    ina3221_conversion_time_t ctshunt;
    ina3221_conversion_time_t ctbus;
    ina3221_average_mode_t avgmode;
#endif
} INA3221Config;

#define _ina3221_methods_alone

#define _ina3221_methods _base_object_methods _ina3221_methods_alone

struct INA3221VMT {
    _ina3221_methods
};

#define _ina3221_data                                                          \
    ina3221_state_t state;                                                     \
    const INA3221Config *config;                                               \
    float shunts[INA3221_NUM_CHANNELS];

struct INA3221Driver {
    const struct BaseCurrentVMT *vmt;
    BaseCurrent current_if;
    _ina3221_data
};

#ifdef __cplusplus
extern "C" {
#endif
void ina3221ObjectInit(INA3221Driver *devp);
void ina3221Start(INA3221Driver *devp, const INA3221Config *config);
void ina3221Stop(INA3221Driver *devp);
#ifdef __cplusplus
}
#endif
