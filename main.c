/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "cli.h"
#include "led.h"

int main(void)
{
    halInit();
    chSysInit();

    ledInit();
    cliInit();

    while (true) {
        cliCheck();
        chThdSleepMilliseconds(500);
    }
}
