/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

#include "pca9546a.h"
#include "tmp117.h"

static const I2CConfig i2c2cfg = {
    0x10808dd3, /* stm32cubemx 100 kHz*/
    0,
    0,
};

static const PCA9546AConfig pca9546acfg = {
    &I2CD2,
    &i2c2cfg,
    PCA9546A_SAD_DEFAULT,
    PORT_DEV_RST_D26,
    PAD_DEV_RST_D26,
};

static PCA9546ADriver drv_multiplexer;

static const TMP117Config tmp117cfg = {
    &I2CD2,
    &i2c2cfg,
    TMP117_SAD_DEFAULT,
    TMP117_CONV_1000,
    TMP117_AVG_8,
};

static TMP117Driver drv_thermometer;

void cmd_tmp117(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: tmp117" SHELL_NEWLINE_STR);
        return;
    }

    pca9546aObjectInit(&drv_multiplexer);
    pca9546aStart(&drv_multiplexer, &pca9546acfg);

    int32_t raw;
    float cooked;

    for (int i = 0; i < (int)multiplexerGetChannelsNumber(&drv_multiplexer);
         i++) {
        multiplexerSetChannel(&drv_multiplexer, (size_t)(1 << i));
        tmp117ObjectInit(&drv_thermometer);
        tmp117Start(&drv_thermometer, &tmp117cfg);

        if (drv_thermometer.state == TMP117_READY) {
            chprintf(chp, "TMP117 %d found" SHELL_NEWLINE_STR, i);
            chprintf(chp,
                     "channels: %u" SHELL_NEWLINE_STR,
                     thermometerGetChannelsNumber(&drv_thermometer));

            thermometerReadRaw(&drv_thermometer, &raw);
            chprintf(chp, "raw: %d" SHELL_NEWLINE_STR, raw);
            thermometerReadCooked(&drv_thermometer, &cooked);
            chprintf(chp, "raw: %f" SHELL_NEWLINE_STR, (double)cooked);

            tmp117Stop(&drv_thermometer);
        } else {
            chprintf(chp, "TMP117 %d not found" SHELL_NEWLINE_STR, i);
        }
    }

    pca9546aStop(&drv_multiplexer);
}
