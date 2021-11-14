/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

#include "ina3221.h"

static const I2CConfig i2c1cfg = {
    0x00702681, /* stm32cubemx 400 kHz*/
    0,
    0,
};

static const INA3221Config ina3221cfg = {
    &I2CD1,
    &i2c1cfg,
    INA3221_SAD_DEFAULT,
    INA3221_CT_1100,
    INA3221_CT_204,
    INA3221_AVG_4,
};

static const float shunts[] = {
    0.1f,
    0.1f,
    0.1f,
};

static INA3221Driver drv;

void cmd_ina3221(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: ina3221" SHELL_NEWLINE_STR);
        return;
    }

    ina3221ObjectInit(&drv);
    ina3221Start(&drv, &ina3221cfg);

    chprintf(chp,
             "shunt channels: %u" SHELL_NEWLINE_STR,
             currentGetShuntChannelsNumber(&drv));
    chprintf(chp,
             "bus channels: %u" SHELL_NEWLINE_STR,
             currentGetBusChannelsNumber(&drv));

    currentSetShunts(&drv, shunts);

    int32_t raw[6];
    float cooked[6];

    currentReadRaw(&drv, raw);
    for (int i = 0; i < 6; i++) {
        chprintf(chp, "raw channel %d: %d" SHELL_NEWLINE_STR, i, raw[i]);
    }

    currentReadCooked(&drv, cooked);
    for (int i = 0; i < 6; i++) {
        chprintf(chp,
                 "cooked channel %d: %f" SHELL_NEWLINE_STR,
                 i,
                 (double)cooked[i]);
    }

    currentPowerReadCooked(&drv, cooked);
    for (int i = 0; i < 3; i++) {
        chprintf(
            chp, "cooked power %d: %f" SHELL_NEWLINE_STR, i, (double)cooked[i]);
    }

    ina3221Stop(&drv);
}
