/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include <stdint.h>

typedef struct {
    stm32_gpio_t *port;
    uint32_t pad;
} ledpad_t;

typedef struct _leds_t leds_t;

leds_t *ledStart(
    void *wsp, size_t size, tprio_t prio, const ledpad_t *pads, size_t n_pads);
void ledSet(leds_t *leds, int n, uint8_t on, uint8_t off);
