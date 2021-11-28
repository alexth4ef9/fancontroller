/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "hal.h"

#include "ina3221.h"

#define EX_INA3221_CONFIG_SHUNT_CT_SHIFT 3
#define EX_INA3221_CONFIG_SHUNT_CT_MASK                                        \
    (0x0007 << EX_INA3221_CONFIG_SHUNT_CT_SHIFT)
#define EX_INA3221_CONFIG_BUS_CT_SHIFT 6
#define EX_INA3221_CONFIG_BUS_CT_MASK (0x0007 << EX_INA3221_CONFIG_BUS_CT_SHIFT)
#define EX_INA3221_CONFIG_AVG_SHIFT 9
#define EX_INA3221_CONFIG_AVG_MASK (0x0007 << EX_INA3221_CONFIG_AVG_SHIFT)

#define EX_INA3221_CHANNEL_MODE                                                \
    (EX_INA3221_CONFIG_CH1EN | EX_INA3221_CONFIG_CH2EN |                       \
     EX_INA3221_CONFIG_CH3EN | EX_INA3221_CONFIG_MODE1 |                       \
     EX_INA3221_CONFIG_MODE2)


#if (INA3221_USE_I2C)
static msg_t ina3221I2CReadRegister(I2CDriver *i2cp,
                                    ina3221_sad_t addr,
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

static msg_t ina3221I2CWriteRegister(I2CDriver *i2cp,
                                     ina3221_sad_t addr,
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

static msg_t ina3221_poll_done(I2CDriver *i2cp, ina3221_sad_t addr)
{
    uint16_t mask;
    uint8_t reg = EX_INA3221_REG_MASK_ENABLE;

    msg_t result =
        i2cMasterTransmitTimeout(i2cp, addr, &reg, 1, NULL, 0, TIME_INFINITE);
    if (result != MSG_OK) {
        return result;
    }

    do {
#if INA3221_NICE_WAITING == TRUE
        osalThreadSleepMilliseconds(1);
#endif
        result = i2cMasterReceiveTimeout(
            i2cp, addr, (uint8_t *)&mask, 2, TIME_INFINITE);
        if (result != MSG_OK) {
            return result;
        }
    } while ((__REVSH(mask) & EX_INA3221_MASK_EN_CVRF) != 0U);

    return MSG_OK;
}
#endif

static size_t sens_get_axes_number(void *ip)
{
    osalDbgCheck(ip != NULL);

    return 2 * INA3221_NUM_CHANNELS;
}

static msg_t sens_read_raw(void *ip, int32_t axes[])
{
    static const uint8_t shunt_regs[INA3221_NUM_CHANNELS] = {
        EX_INA3221_REG_CHANNEL1_SHUNT_VOLTAGE,
        EX_INA3221_REG_CHANNEL2_SHUNT_VOLTAGE,
        EX_INA3221_REG_CHANNEL3_SHUNT_VOLTAGE,
    };
    static const uint8_t bus_regs[INA3221_NUM_CHANNELS] = {
        EX_INA3221_REG_CHANNEL1_BUS_VOLTAGE,
        EX_INA3221_REG_CHANNEL2_BUS_VOLTAGE,
        EX_INA3221_REG_CHANNEL3_BUS_VOLTAGE,
    };

    uint16_t avg, ctshunt, ctbus;

    msg_t result = MSG_OK;

    osalDbgCheck(ip != NULL);
    osalDbgAssert((((INA3221Driver *)ip)->state == INA3221_READY),
                  "sense_read_raw(), invalid state");

    avg = (((INA3221Driver *)ip)->config->avgmode
           << EX_INA3221_CONFIG_AVG_SHIFT) &
          EX_INA3221_CONFIG_AVG_MASK;
    ctshunt = (((INA3221Driver *)ip)->config->ctshunt &
               EX_INA3221_CONFIG_BUS_CT_SHIFT) &
              EX_INA3221_CONFIG_BUS_CT_MASK;
    ctbus = (((INA3221Driver *)ip)->config->ctbus &
             EX_INA3221_CONFIG_SHUNT_CT_SHIFT) &
            EX_INA3221_CONFIG_SHUNT_CT_MASK;

#if INA3221_USE_I2C
    osalDbgAssert((((INA3221Driver *)ip)->config->i2cp->state == I2C_READY),
                  "sense_read_raw(), channel not ready");
#if INA3221_SHARED_I2C
    i2cAcquireBus(((INA3221Driver *)ip)->config->i2cp);
    i2cStart(((INA3221Driver *)ip)->config->i2cp,
             ((INA3221Driver *)ip)->config->i2ccfg);
#endif

    result = ina3221I2CWriteRegister(
        ((INA3221Driver *)ip)->config->i2cp,
        ((INA3221Driver *)ip)->config->slaveaddress,
        EX_INA3221_REG_CONFIG,
        avg | ctbus | ctshunt | EX_INA3221_CHANNEL_MODE);

    if (result == MSG_OK) {
        result = ina3221_poll_done(((INA3221Driver *)ip)->config->i2cp,
                                   ((INA3221Driver *)ip)->config->slaveaddress);
    }
    for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
        if (result == MSG_OK) {
            uint16_t value = 0;
            result = ina3221I2CReadRegister(
                ((INA3221Driver *)ip)->config->i2cp,
                ((INA3221Driver *)ip)->config->slaveaddress,
                shunt_regs[i],
                &value);
            axes[i] = ((int16_t)value) / 8;
        }
        if (result == MSG_OK) {
            uint16_t value = 0;
            result = ina3221I2CReadRegister(
                ((INA3221Driver *)ip)->config->i2cp,
                ((INA3221Driver *)ip)->config->slaveaddress,
                bus_regs[i],
                &value);
            axes[i + INA3221_NUM_CHANNELS] = ((int16_t)value) / 8;
        }
    }

#if INA3221_SHARED_I2C
    i2cReleaseBus(((INA3221Driver *)ip)->config->i2cp);
#endif
#endif
    return result;
}

static msg_t sens_read_cooked(void *ip, float axes[])
{
    int32_t values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_raw(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            axes[i] = values[i] * EX_INA3221_SHUNT_LSB /
                      ((INA3221Driver *)ip)->shunts[i];
            axes[i + INA3221_NUM_CHANNELS] =
                values[i + INA3221_NUM_CHANNELS] * EX_INA3221_BUS_LSB;
        }
    }
    return result;
}

static size_t get_shunt_channels_number(void *ip)
{
    osalDbgCheck(ip != NULL);

    return INA3221_NUM_CHANNELS;
}

static size_t get_bus_channels_number(void *ip)
{
    osalDbgCheck(ip != NULL);

    return INA3221_NUM_CHANNELS;
}

static msg_t read_shunt_raw(void *ip, int32_t shunts[])
{
    int32_t values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_raw(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            shunts[i] = values[i];
        }
    }
    return result;
}

static msg_t read_bus_raw(void *ip, int32_t bus[])
{
    int32_t values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_raw(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            bus[i] = values[i + INA3221_NUM_CHANNELS];
        }
    }
    return result;
}

static msg_t read_shunt_cooked(void *ip, float shunts[])
{
    float values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_cooked(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            shunts[i] = values[i];
        }
    }
    return result;
}

static msg_t read_bus_cooked(void *ip, float bus[])
{
    float values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_cooked(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            bus[i] = values[i + INA3221_NUM_CHANNELS];
        }
    }
    return result;
}

static msg_t read_power_cooked(void *ip, float power[])
{
    float values[2 * INA3221_NUM_CHANNELS];

    msg_t result = sens_read_cooked(ip, values);
    if (result == MSG_OK) {
        for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
            power[i] = values[i] * values[i + INA3221_NUM_CHANNELS];
        }
    }
    return result;
}

static msg_t set_shunts(void *ip, const float shunts[])
{
    osalDbgCheck(ip != NULL);

    for (int i = 0; i < INA3221_NUM_CHANNELS; i++) {
        ((INA3221Driver *)ip)->shunts[i] = shunts[i];
    }

    return MSG_OK;
}

static const struct BaseCurrentVMT vmt_basecurrent = {
    0,
    sens_get_axes_number,
    sens_read_raw,
    sens_read_cooked,
    get_shunt_channels_number,
    get_bus_channels_number,
    read_shunt_raw,
    read_bus_raw,
    read_shunt_cooked,
    read_bus_cooked,
    read_power_cooked,
    set_shunts,
};

void ina3221ObjectInit(INA3221Driver *devp)
{

    devp->vmt = &vmt_basecurrent;
    devp->config = NULL;
    devp->state = INA3221_STOP;
}

void ina3221Start(INA3221Driver *devp, const INA3221Config *config)
{
#if INA3221_USE_I2C
    uint16_t id;
#endif
    osalDbgCheck((devp != NULL) && (config != NULL));
    osalDbgAssert((devp->state == INA3221_STOP) ||
                      (devp->state == INA3221_READY),
                  "ina3221Start(), invalid state");
    devp->config = config;
#if INA3221_USE_I2C
#if INA3221_SHARED_I2C
    i2cAcquireBus((devp)->config->i2cp);
#endif
    i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
    if (ina3221I2CReadRegister((devp)->config->i2cp,
                               (devp)->config->slaveaddress,
                               EX_INA3221_REG_MANUFACTURER_ID,
                               &id) != MSG_OK ||
        id != EX_INA3221_MANUFACTURER_ID) {
        devp->state = INA3221_NOTFOUND;
    }
    if (ina3221I2CReadRegister((devp)->config->i2cp,
                               (devp)->config->slaveaddress,
                               EX_INA3221_REG_DIE_ID,
                               &id) != MSG_OK ||
        id != EX_INA3221_DIE_ID) {
        devp->state = INA3221_NOTFOUND;
    }
#if INA3221_SHARED_I2C
    i2cReleaseBus((devp)->config->i2cp);
#endif
#endif
    if (devp->state != INA3221_READY && devp->state != INA3221_NOTFOUND) {
        devp->state = INA3221_READY;
    }
}

void ina3221Stop(INA3221Driver *devp)
{
    osalDbgCheck(devp != NULL);
    osalDbgAssert((devp->state == INA3221_STOP) ||
                      (devp->state == INA3221_READY),
                  "ina3221Stop(), invalid state");
#if (INA3221_USE_I2C)
    if (devp->state == INA3221_STOP) {
#if INA3221_SHARED_I2C
        i2cAcquireBus((devp)->config->i2cp);
        i2cStart((devp)->config->i2cp, (devp)->config->i2ccfg);
#endif
#if INA3221_SHARED_I2C
        i2cReleaseBus((devp)->config->i2cp);
#endif
    }
#endif
    if (devp->state != INA3221_STOP) {
        devp->state = INA3221_STOP;
    }
}
