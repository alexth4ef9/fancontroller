/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#define _base_async_methods_alone msg_t (*start_acquire)(void *instance);

#define _base_async_methods _base_async_methods_alone

struct BaseAsyncVMT {
    _base_async_methods
};

#define _base_async_data bool started;

typedef struct {
    const struct BaseAsyncVMT *vmt;
    _base_async_data
} BaseAsync;

#define asyncStart(ip) (ip)->vmt->start(ip)
