/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

#include "pca9546a.h"

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

static PCA9546ADriver drv;

void cmd_pca9546a(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: pca9546a" SHELL_NEWLINE_STR);
        return;
    }

    pca9546aObjectInit(&drv);
    pca9546aStart(&drv, &pca9546acfg);

    multiplexerReset(&drv);
    if (multiplexerGetChannel(&drv) == 0) {
        chprintf(chp, "PCA9546A reset OK" SHELL_NEWLINE_STR);
    } else {
        chprintf(chp, "PCA9546A reset FAILED" SHELL_NEWLINE_STR);
    }

    for (int i = 0; i < (int)multiplexerGetChannelsNumber(&drv); i++) {
        multiplexerSetChannel(&drv, (size_t)(1 << i));
        if (multiplexerGetChannel(&drv) == (size_t)(1 << i)) {
            chprintf(chp, "PCA9546A set channel %d OK" SHELL_NEWLINE_STR, i);
        } else {
            chprintf(
                chp, "PCA9546A set channel %d FAILED" SHELL_NEWLINE_STR, i);
        }
    }

    pca9546aStop(&drv);
}
