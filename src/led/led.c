/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "led.h"

#include <stdbool.h>

typedef struct {
    stm32_gpio_t *port;
    uint32_t pad;
    uint8_t on;
    uint8_t off;
    uint8_t count;
    bool state;
} led_t;

struct _leds_t {
    size_t count;
    led_t leds[];
};

static THD_FUNCTION(ThreadLeds, arg)
{
    leds_t *leds = (leds_t *)arg;
    systime_t sleepuntil = chVTGetSystemTimeX();
    chRegSetThreadName("leds");
    while (true) {
        sleepuntil += TIME_MS2I(100);
        for (size_t i = 0; i < leds->count; i++) {
            if (leds->leds[i].count == 0) {
                if (!leds->leds[i].state) {
                    if (leds->leds[i].on > 0) {
                        palSetPad(leds->leds[i].port, leds->leds[i].pad);
                    }
                    leds->leds[i].count = leds->leds[i].on;
                    leds->leds[i].state = true;
                } else {
                    if (leds->leds[i].off > 0) {
                        palClearPad(leds->leds[i].port, leds->leds[i].pad);
                    }
                    leds->leds[i].count = leds->leds[i].off;
                    leds->leds[i].state = false;
                }
            }
            if (leds->leds[i].count > 0) {
                leds->leds[i].count--;
            }
        }
        chThdSleepUntil(sleepuntil);
    }
}

leds_t *ledStart(
    void *wsp, size_t size, tprio_t prio, const ledpad_t *pads, size_t n_pads)
{
    leds_t *leds = chCoreAlloc(sizeof(leds_t) + n_pads * sizeof(led_t));
    osalDbgAssert(leds, "failed to allocate leds instance");
    leds->count = n_pads;
    for (size_t i = 0; i < n_pads; i++) {
        leds->leds[i].port = pads[i].port;
        leds->leds[i].pad = pads[i].pad;
        leds->leds[i].on = 0;
        leds->leds[i].off = 0;
        leds->leds[i].count = 0;
        leds->leds[i].state = 0;
    }

    chThdCreateStatic(wsp, size, prio, ThreadLeds, leds);

    return leds;
}

void ledSet(leds_t *leds, int n, uint8_t on, uint8_t off)
{
    leds->leds[n].on = on;
    leds->leds[n].off = off;
    leds->leds[n].count = 0;
    leds->leds[n].state = 0;
}
