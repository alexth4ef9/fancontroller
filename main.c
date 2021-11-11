/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "cli.h"
#include "fs.h"
#include "led.h"
#include "util.h"

static const SPIConfig spiconfig2 = {
    .circular = false,
    .end_cb = NULL,
    .ssport = PORT_SPI2_NSS_D31,
    .sspad = PAD_SPI2_NSS_D31,
    .cr1 = 0,            /* 18MHz, Mode 0 */
    .cr2 = SPI_CR2_SSOE, /* Single Master */
};

static const SNORConfig snorconfig1 = {
    .busp = &SPID2,
    .buscfg = &spiconfig2,
};

static THD_WORKING_AREA(waThreadFs, 2048);
static thread_t *threadFs;

static const ledpad_t ledpads[] = {
    {PORT_D13_SCK_LED1, PAD_D13_SCK_LED1},
    {PORT_D3_LED2, PAD_D3_LED2},
};

static THD_WORKING_AREA(waThreadLeds, 128);
static leds_t *leds;

int main(void)
{
    halInit();
    chSysInit();

    threadFs =
        fsStart(waThreadFs, sizeof(waThreadFs), NORMALPRIO, &snorconfig1);
    leds = ledStart(waThreadLeds,
                    sizeof(waThreadLeds),
                    NORMALPRIO,
                    ledpads,
                    COUNTOF(ledpads));
    cliStart(threadFs, leds, 0);

    while (true) {
        cliCheck();
        chThdSleepMilliseconds(500);
    }
}
