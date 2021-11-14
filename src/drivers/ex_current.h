/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

#include "ex_sensors.h"

#define _base_current_methods_alone                                            \
    size_t (*get_shunt_channels_number)(void *instance);                       \
    size_t (*get_bus_channels_number)(void *instance);                         \
    msg_t (*read_shunt_raw)(void *instance, int32_t shunts[]);                 \
    msg_t (*read_bus_raw)(void *instance, int32_t bus[]);                      \
    msg_t (*read_shunt_cooked)(void *instance, float shunts[]);                \
    msg_t (*read_bus_cooked)(void *instance, float bus[]);                     \
    msg_t (*read_power_cooked)(void *instance, float power[]);                 \
    msg_t (*set_shunts)(void *instance, const float shunts[]);

#define _base_current_methods _base_sensor_methods _base_current_methods_alone

struct BaseCurrentVMT {
    _base_current_methods
};

#define _base_current_data _base_sensor_data

typedef struct {
    const struct BaseCurrentVMT *vmt;
    _base_current_data
} BaseCurrent;

#define currentGetShuntChannelsNumber(ip)                                      \
    (ip)->vmt->get_shunt_channels_number(ip)

#define currentGetBusChannelsNumber(ip) (ip)->vmt->get_bus_channels_number(ip)

#define currentReadRaw(ip, dp) (ip)->vmt->read_raw(ip, dp)

#define currentReadCooked(ip, dp) (ip)->vmt->read_cooked(ip, dp)

#define currentReadShuntRaw(ip, dp) (ip)->vmt->read_shunt_raw(ip, dp)

#define currentReadBusRaw(ip, dp) (ip)->vmt->read_bus_raw(ip, dp)

#define currentShuntReadCooked(ip, dp) (ip)->vmt->read_shunt_cooked(ip, dp)

#define currentBusReadCooked(ip, dp) (ip)->vmt->read_bus_cooked(ip, dp)

#define currentPowerReadCooked(ip, dp) (ip)->vmt->read_power_cooked(ip, dp)

#define currentSetShunts(ip, bp) (ip)->vmt->set_shunts(ip, bp)
