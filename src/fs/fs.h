/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include "hal_serial_nor.h"

typedef struct ch_thread thread_t;

thread_t* fsStart(void *wsp, size_t size, tprio_t prio, const SNORConfig* snorconfig);

int fsRead(thread_t *threadFs, const char* name, void* data, unsigned size);
int fsWrite(thread_t *threadFs, const char* name, const void* data, unsigned size);
int fsRename(thread_t *threadFs, const char* oldName, const char* newName);
