/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#define _base_multiplexer_methods_alone                                        \
    size_t (*get_channels_number)(void *instance);                             \
    msg_t (*set_channel)(void *instance, size_t channel);                      \
    size_t (*get_channel)(void *instance);                                     \
    msg_t (*reset)(void *instance);

#define _base_multiplexer_methods _base_multiplexer_methods_alone

struct BaseMultiplexerVMT {
    _base_multiplexer_methods
};

#define _base_multiplexer_data

typedef struct {
    const struct BaseMultiplexerVMT *vmt;
    _base_multiplexer_data
} BaseMultiplexer;

#define multiplexerGetChannelsNumber(ip) (ip)->vmt->get_channels_number(ip)

#define multiplexerSetChannel(ip, n) (ip)->vmt->set_channel(ip, n)

#define multiplexerGetChannel(ip) (ip)->vmt->get_channel(ip)

#define multiplexerReset(ip) (ip)->vmt->reset(ip)
