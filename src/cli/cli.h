/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

typedef struct ch_thread thread_t;
typedef struct _leds_t leds_t;

void cliStart(thread_t *threadFsSettings, leds_t *leds, int statusLed);
void cliCheck(void);
