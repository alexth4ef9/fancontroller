/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include <stdint.h>

enum LEDS {
    LED1 = 0,
    LED2 = 1,
};

void ledInit(void);
void ledSet(enum LEDS n, uint8_t on, uint8_t off);
