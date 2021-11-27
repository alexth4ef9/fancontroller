/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "hal.h"

#include "pca9546a.h"

static size_t set_channel_pca9546a(PCA9546ADriver *drv, size_t channel)
{
    uint8_t value = channel;
    return i2cMasterTransmitTimeout(drv->config->i2cp,
                                    drv->config->slaveaddress,
                                    &value,
                                    1,
                                    NULL,
                                    0,
                                    TIME_INFINITE);
}

static size_t get_channel_pca9546a(PCA9546ADriver *drv)
{
    size_t channel = SIZE_MAX;
    uint8_t value;
    msg_t result = i2cMasterReceiveTimeout(
        drv->config->i2cp, drv->config->slaveaddress, &value, 1, TIME_INFINITE);
    if (result == MSG_OK) {
        channel = value;
    } else {
        channel = SIZE_MAX;
    }
    return channel;
}

static void reset_pca9546a(PCA9546ADriver *drv)
{
    if (drv->config->resetport != NULL) {
        palClearPad(drv->config->resetport, drv->config->resetpad);
        osalThreadSleepMilliseconds(1);
        palSetPad(drv->config->resetport, drv->config->resetpad);
        osalThreadSleepMilliseconds(1);
    }
}

static size_t get_channels_number(void *ip)
{
    osalDbgCheck(ip != NULL);

    return EX_PCA9546A_NUM_CHANNELS;
}

static msg_t set_channel(void *ip, size_t channel)
{
    msg_t result = MSG_OK;

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((PCA9546ADriver *)ip)->state == PCA9546A_READY),
                  "set_channel(), invalid state");

#if PCA9546A_USE_I2C
#if PCA9546A_SHARED_I2C
    i2cAcquireBus(((PCA9546ADriver *)ip)->config->i2cp);
    i2cStart(((PCA9546ADriver *)ip)->config->i2cp,
             ((PCA9546ADriver *)ip)->config->i2ccfg);
#endif
    result = set_channel_pca9546a((PCA9546ADriver *)ip, channel);
#if PCA9546A_SHARED_I2C
    i2cReleaseBus(((PCA9546ADriver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static size_t get_channel(void *ip)
{
    size_t result = SIZE_MAX;

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((PCA9546ADriver *)ip)->state == PCA9546A_READY),
                  "get_channel(), invalid state");

#if PCA9546A_USE_I2C
#if PCA9546A_SHARED_I2C
    i2cAcquireBus(((PCA9546ADriver *)ip)->config->i2cp);
    i2cStart(((PCA9546ADriver *)ip)->config->i2cp,
             ((PCA9546ADriver *)ip)->config->i2ccfg);
#endif
    result = get_channel_pca9546a((PCA9546ADriver *)ip);
#if PCA9546A_SHARED_I2C
    i2cReleaseBus(((PCA9546ADriver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static msg_t reset(void *ip)
{
    osalDbgCheck(ip != NULL);
    osalDbgAssert((((PCA9546ADriver *)ip)->state == PCA9546A_READY),
                  "reset(), invalid state");
#if PCA9546A_USE_I2C
#if PCA9546A_SHARED_I2C
    i2cAcquireBus(((PCA9546ADriver *)ip)->config->i2cp);
#endif
    reset_pca9546a((PCA9546ADriver *)ip);
#if PCA9546A_SHARED_I2C
    i2cReleaseBus(((PCA9546ADriver *)ip)->config->i2cp);
#endif
#endif
    return MSG_OK;
}

static const struct BaseMultiplexerVMT vmt_basemultiplexer = {
    get_channels_number,
    set_channel,
    get_channel,
    reset,
};

void pca9546aObjectInit(PCA9546ADriver *devp)
{
    devp->vmt = &vmt_basemultiplexer;
    devp->config = NULL;
    devp->state = PCA9546A_STOP;
}

void pca9546aStart(PCA9546ADriver *devp, const PCA9546AConfig *config)
{
    osalDbgCheck((devp != NULL) && (config != NULL));
    osalDbgAssert((devp->state == PCA9546A_STOP) ||
                      (devp->state == PCA9546A_READY),
                  "pca9546aStart(), invalid state");
    devp->config = config;
#if PCA9546A_USE_I2C
#if PCA9546A_SHARED_I2C
    i2cAcquireBus((devp)->config->i2cp);
#endif
    i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
    if (get_channel_pca9546a(devp) >= (1 << EX_PCA9546A_NUM_CHANNELS)) {
        reset_pca9546a(devp);
        if (get_channel_pca9546a(devp) >= (1 << EX_PCA9546A_NUM_CHANNELS)) {
            devp->state = PCA9546A_NOTFOUND;
        }
    }
#if PCA9546A_SHARED_I2C
    i2cReleaseBus((devp)->config->i2cp);
#endif
#endif
    if (devp->state != PCA9546A_READY && devp->state != PCA9546A_NOTFOUND) {
        devp->state = PCA9546A_READY;
    }
}

void pca9546aStop(PCA9546ADriver *devp)
{
    osalDbgCheck(devp != NULL);
    osalDbgAssert((devp->state == PCA9546A_STOP) ||
                      (devp->state == PCA9546A_READY),
                  "pca9546aStop(), invalid state");
#if (PCA9546A_USE_I2C)
    if (devp->state == PCA9546A_STOP) {
#if PCA9546A_SHARED_I2C
        i2cAcquireBus((devp)->config->i2cp);
        i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
#endif
#if PCA9546A_SHARED_I2C
        i2cReleaseBus((devp)->config->i2cp);
#endif
    }
#endif
    if (devp->state != PCA9546A_STOP) {
        devp->state = PCA9546A_STOP;
    }
}
