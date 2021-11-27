/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include "ex_multiplexer.h"

#define EX_PCA9546A_VERSION "1.0.0"

#define EX_PCA9546A_MAJOR 1
#define EX_PCA9546A_MINOR 0
#define EX_PCA9546A_PATCH 0


#define EX_PCA9546A_NUM_CHANNELS 4

#if !defined(PCA9546A_USE_I2C)
#define PCA9546A_USE_I2C TRUE
#endif

#if !defined(PCA9546A_SHARED_I2C)
#define PCA9546A_SHARED_I2C FALSE
#endif

#if PCA9546A_USE_I2C && !HAL_USE_I2C
#error "PCA9546A_USE_I2C requires HAL_USE_I2C"
#endif

#if PCA9546A_SHARED_I2C && !I2C_USE_MUTUAL_EXCLUSION
#error "PCA9546A_SHARED_I2C requires I2C_USE_MUTUAL_EXCLUSION"
#endif

typedef struct PCA9546ADriver PCA9546ADriver;

typedef enum {
    PCA9546A_SAD_0 = 0x70,
    PCA9546A_SAD_1 = 0x71,
    PCA9546A_SAD_2 = 0x72,
    PCA9546A_SAD_3 = 0x73,
    PCA9546A_SAD_4 = 0x74,
    PCA9546A_SAD_5 = 0x75,
    PCA9546A_SAD_6 = 0x76,
    PCA9546A_SAD_7 = 0x77,
    PCA9546A_SAD_DEFAULT = PCA9546A_SAD_0,
} pca9546a_sad_t;

typedef enum {
    PCA9546A_UNINIT = 0,
    PCA9546A_STOP = 1,
    PCA9546A_READY = 2,
    PCA9546A_NOTFOUND = 3,
} pca9546a_state_t;

typedef struct {
#if PCA9546A_USE_I2C
    I2CDriver *i2cp;
    const I2CConfig *i2ccfg;
    pca9546a_sad_t slaveaddress;
    stm32_gpio_t *resetport;
    uint32_t resetpad;
#endif
} PCA9546AConfig;

#define _pca9546a_methods_alone

#define _pca9546a_methods _base_object_methods _pca9546a_methods_alone

struct PCA9546AVMT {
    _pca9546a_methods
};

#define _pca9546a_data                                                         \
    pca9546a_state_t state;                                                    \
    const PCA9546AConfig *config;

struct PCA9546ADriver {
    const struct BaseMultiplexerVMT *vmt;
    BaseMultiplexer current_if;
    _pca9546a_data
};

#ifdef __cplusplus
extern "C" {
#endif
void pca9546aObjectInit(PCA9546ADriver *devp);
void pca9546aStart(PCA9546ADriver *devp, const PCA9546AConfig *config);
void pca9546aStop(PCA9546ADriver *devp);
#ifdef __cplusplus
}
#endif
