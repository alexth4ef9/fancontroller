/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "hal.h"

#include "tmp117.h"

#define EX_TMP117_CONFIG_AVG_SHIFT 5
#define EX_TMP117_CONFIG_AVG_MASK (0x0003 << EX_TMP117_CONFIG_AVG_SHIFT)
#define EX_TMP117_CONFIG_CONV_SHIFT 7
#define EX_TMP117_CONFIG_CONV_MASK (0x0007 << EX_TMP117_CONFIG_CONV_SHIFT)

#define EX_TMP117_MODE (EX_TMP117_CONFIG_MOD0 | EX_TMP117_CONFIG_MOD1)

#if (TMP117_USE_I2C)
static msg_t tmp117I2CReadRegister(I2CDriver *i2cp,
                                   tmp117_sad_t addr,
                                   uint8_t reg,
                                   uint16_t *rx)
{
    msg_t result =
        i2cMasterTransmitTimeout(i2cp, addr, &reg, 1, NULL, 0, TIME_INFINITE);
    if (result != MSG_OK) {
        return result;
    }
    result =
        i2cMasterReceiveTimeout(i2cp, addr, (uint8_t *)rx, 2, TIME_INFINITE);
    if (result != MSG_OK) {
        return result;
    }
    *rx = __REVSH(*rx);
    return result;
}

static msg_t tmp117I2CWriteRegister(I2CDriver *i2cp,
                                    tmp117_sad_t addr,
                                    uint8_t reg,
                                    uint16_t tx)
{
    msg_t result =
        i2cMasterTransmitTimeout(i2cp, addr, &reg, 1, NULL, 0, TIME_INFINITE);
    if (result != MSG_OK) {
        return result;
    }
    tx = __REVSH(tx);
    result = i2cMasterTransmitTimeout(
        i2cp, addr, (uint8_t *)&tx, 2, NULL, 0, TIME_INFINITE);
    return result;
}

static msg_t tmp117_poll_done(I2CDriver *i2cp, tmp117_sad_t addr)
{
    uint16_t mask;
    uint8_t reg = EX_TMP117_REG_CONFIG;

    msg_t result =
        i2cMasterTransmitTimeout(i2cp, addr, &reg, 1, NULL, 0, TIME_INFINITE);
    if (result != MSG_OK) {
        return result;
    }

    do {
#if TMP117_NICE_WAITING == TRUE
        osalThreadSleepMilliseconds(1);
#endif
        result = i2cMasterReceiveTimeout(
            i2cp, addr, (uint8_t *)&mask, 2, TIME_INFINITE);
        if (result != MSG_OK) {
            return result;
        }
    } while ((__REVSH(mask) & EX_TMP117_CONFIG_DATA_READY) != 0U);

    return MSG_OK;
}
#endif

static size_t sens_get_axes_number(void *ip)
{
    osalDbgCheck(ip != NULL);

    return 1;
}

static msg_t sens_read_raw(void *ip, int32_t axes[])
{
    uint16_t avg, conv;

    msg_t result = MSG_OK;

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((TMP117Driver *)ip)->state == TMP117_READY),
                  "sense_read_raw(), invalid state");

    avg =
        (((TMP117Driver *)ip)->config->avgmode << EX_TMP117_CONFIG_AVG_SHIFT) &
        EX_TMP117_CONFIG_AVG_MASK;
    conv = (((TMP117Driver *)ip)->config->cycletime &
            EX_TMP117_CONFIG_CONV_SHIFT) &
           EX_TMP117_CONFIG_CONV_MASK;

#if TMP117_USE_I2C
    osalDbgAssert((((TMP117Driver *)ip)->config->i2cp->state == I2C_READY),
                  "sense_read_raw(), channel not ready");
#if TMP117_SHARED_I2C
    i2cAcquireBus(((TMP117Driver *)ip)->config->i2cp);
    i2cStart(((TMP117Driver *)ip)->config->i2cp,
             ((TMP117Driver *)ip)->config->i2ccfg);
#endif

    result = tmp117I2CWriteRegister(((TMP117Driver *)ip)->config->i2cp,
                                    ((TMP117Driver *)ip)->config->slaveaddress,
                                    EX_TMP117_REG_CONFIG,
                                    avg | conv | EX_TMP117_MODE);

    if (result == MSG_OK) {
        result = tmp117_poll_done(((TMP117Driver *)ip)->config->i2cp,
                                  ((TMP117Driver *)ip)->config->slaveaddress);
    }

    if (result == MSG_OK) {
        int16_t value = 0;
        result =
            tmp117I2CReadRegister(((TMP117Driver *)ip)->config->i2cp,
                                  ((TMP117Driver *)ip)->config->slaveaddress,
                                  EX_TMP117_REG_TEMP_RESULT,
                                  (uint16_t *)&value);
        axes[0] = value;
    }

#if TMP117_SHARED_I2C
    i2cReleaseBus(((TMP117Driver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static msg_t sens_read_cooked(void *ip, float axes[])
{
    int32_t values;

    msg_t result = sens_read_raw(ip, &values);
    if (result == MSG_OK) {
        axes[0] = values * EX_TMP117_TEMP_LSB;
    }
    return result;
}

static msg_t set_bias(void *ip, float biases[])
{
    msg_t result = MSG_OK;
#if TMP117_USE_I2C
    int16_t value;
#endif

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((TMP117Driver *)ip)->state == TMP117_READY),
                  "set_bias(), invalid state");

#if TMP117_USE_I2C
    osalDbgAssert((((TMP117Driver *)ip)->config->i2cp->state == I2C_READY),
                  "set_bias(), channel not ready");
#if TMP117_SHARED_I2C
    i2cAcquireBus(((TMP117Driver *)ip)->config->i2cp);
    i2cStart(((TMP117Driver *)ip)->config->i2cp,
             ((TMP117Driver *)ip)->config->i2ccfg);
#endif
    value = (int16_t)(biases[0] / EX_TMP117_TEMP_LSB);
    result = tmp117I2CWriteRegister(((TMP117Driver *)ip)->config->i2cp,
                                    ((TMP117Driver *)ip)->config->slaveaddress,
                                    EX_TMP117_REG_TEMP_OFFSET,
                                    value);

#if TMP117_SHARED_I2C
    i2cReleaseBus(((TMP117Driver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static msg_t reset_bias(void *ip)
{
    msg_t result = MSG_OK;

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((TMP117Driver *)ip)->state == TMP117_READY),
                  "set_bias(), invalid state");

#if TMP117_USE_I2C
    osalDbgAssert((((TMP117Driver *)ip)->config->i2cp->state == I2C_READY),
                  "set_bias(), channel not ready");
#if TMP117_SHARED_I2C
    i2cAcquireBus(((TMP117Driver *)ip)->config->i2cp);
    i2cStart(((TMP117Driver *)ip)->config->i2cp,
             ((TMP117Driver *)ip)->config->i2ccfg);
#endif
    result = tmp117I2CWriteRegister(((TMP117Driver *)ip)->config->i2cp,
                                    ((TMP117Driver *)ip)->config->slaveaddress,
                                    EX_TMP117_REG_TEMP_OFFSET,
                                    0);

#if TMP117_SHARED_I2C
    i2cReleaseBus(((TMP117Driver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static msg_t set_sensitivity(void *ip, float sensitivities[])
{
    (void)sensitivities;
    osalDbgCheck(ip != NULL);

    return MSG_OK;
}

static msg_t reset_sensitivity(void *ip)
{
    osalDbgCheck(ip != NULL);

    return MSG_OK;
}

static const struct BaseThermometerVMT vmt_basethermometer = {
    0,
    sens_get_axes_number,
    sens_read_raw,
    sens_read_cooked,
    set_bias,
    reset_bias,
    set_sensitivity,
    reset_sensitivity,
};

void tmp117ObjectInit(TMP117Driver *devp)
{

    devp->vmt = &vmt_basethermometer;
    devp->config = NULL;
    devp->state = TMP117_STOP;
}

void tmp117Start(TMP117Driver *devp, const TMP117Config *config)
{
#if TMP117_USE_I2C
    uint16_t id;
#endif
    osalDbgCheck((devp != NULL) && (config != NULL));
    osalDbgAssert((devp->state == TMP117_STOP) || (devp->state == TMP117_READY),
                  "tmp117Start(), invalid state");
    devp->config = config;
#if TMP117_USE_I2C
#if TMP117_SHARED_I2C
    i2cAcquireBus((devp)->config->i2cp);
#endif
    i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
    if (tmp117I2CReadRegister((devp)->config->i2cp,
                              (devp)->config->slaveaddress,
                              EX_TMP117_REG_DEV_ID,
                              &id) != MSG_OK ||
        id != EX_TMP117_DEV_ID) {
        devp->state = TMP117_NOTFOUND;
    }
#if TMP117_SHARED_I2C
    i2cReleaseBus((devp)->config->i2cp);
#endif
#endif
    if (devp->state != TMP117_READY && devp->state != TMP117_NOTFOUND) {
        devp->state = TMP117_READY;
    }
}

void tmp117Stop(TMP117Driver *devp)
{
    osalDbgCheck(devp != NULL);
    osalDbgAssert((devp->state == TMP117_STOP) || (devp->state == TMP117_READY),
                  "tmp117Stop(), invalid state");
#if (TMP117_USE_I2C)
    if (devp->state == TMP117_STOP) {
#if TMP117_SHARED_I2C
        i2cAcquireBus((devp)->config->i2cp);
        i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
#endif
#if TMP117_SHARED_I2C
        i2cReleaseBus((devp)->config->i2cp);
#endif
    }
#endif
    if (devp->state != TMP117_STOP) {
        devp->state = TMP117_STOP;
    }
}
