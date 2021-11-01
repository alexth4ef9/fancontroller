/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "led.h"
#include "util.h"

typedef struct _ledpad_t ledpad_t;

struct _ledpad_t {
    stm32_gpio_t *port;
    uint32_t pad;
};

typedef struct _led_t led_t;

struct _led_t {
    uint8_t on;
    uint8_t off;
    uint8_t count;
    uint8_t state;
};

static const ledpad_t ledpads[] = {
    {PORT_D13_SCK_LED1, PAD_D13_SCK_LED1},
    {PORT_D3_LED2, PAD_D3_LED2},
};

static led_t ledstates[COUNTOF(ledpads)];

static THD_WORKING_AREA(waThreadLeds, 128);
static THD_FUNCTION(ThreadLeds, arg)
{
    (void)arg;
    chRegSetThreadName("leds");
    while (true) {
        for (size_t i = 0; i < COUNTOF(ledpads); i++) {
            if (ledstates[i].count == 0) {
                if (ledstates[i].state == 0) {
                    if (ledstates[i].on > 0) {
                        palSetPad(ledpads[i].port, ledpads[i].pad);
                    }
                    ledstates[i].count = ledstates[i].on;
                    ledstates[i].state = 1;
                } else {
                    if (ledstates[i].off > 0) {
                        palClearPad(ledpads[i].port, ledpads[i].pad);
                    }
                    ledstates[i].count = ledstates[i].off;
                    ledstates[i].state = 0;
                }
            }
            if (ledstates[i].count > 0) {
                ledstates[i].count--;
            }
        }
        chThdSleepMilliseconds(100);
    }
}

void ledInit(void)
{
    for (size_t i = 0; i < COUNTOF(ledpads); i++) {
        ledstates[i].on = 0;
        ledstates[i].off = 0;
        ledstates[i].count = 0;
        ledstates[i].state = 0;
    }
    chThdCreateStatic(
        waThreadLeds, sizeof(waThreadLeds), NORMALPRIO, ThreadLeds, NULL);
}

void ledSet(enum LEDS n, uint8_t on, uint8_t off)
{
    ledstates[n].on = on;
    ledstates[n].off = off;
    for (size_t i = 0; i < COUNTOF(ledpads); i++) {
        ledstates[i].count = 0;
        ledstates[i].state = 0;
    }
}
