#include "ina226.h"

#define REG_CONFIG       0x00u
#define REG_BUS_VOLTAGE  0x02u
#define REG_POWER        0x03u
#define REG_CURRENT      0x04u
#define REG_CALIBRATION  0x05u

// Chip's power-on-reset default: continuous shunt + bus voltage conversion.
// Written explicitly at init so behaviour doesn't depend on whatever a
// previous boot (or a debugger session) left the chip configured as.
#define CONFIG_CONTINUOUS_DEFAULT 0x4127u

#define BUS_VOLTAGE_LSB_VOLTS 0.00125f // 1.25 mV per datasheet
#define POWER_LSB_FACTOR 25.0f         // power_lsb = 25 * current_lsb, per datasheet

static bool write_reg16(ina226_t *dev, uint8_t reg, uint16_t value) {
    uint8_t buf[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFFu)};
    int ret = i2c_write_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    return ret == (int)sizeof(buf);
}

static bool read_reg16(ina226_t *dev, uint8_t reg, uint16_t *value) {
    int ret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, true);
    if (ret != 1) {
        return false;
    }
    uint8_t buf[2];
    ret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (ret != (int)sizeof(buf)) {
        return false;
    }
    *value = (uint16_t)((buf[0] << 8) | buf[1]);
    return true;
}

bool ina226_init(ina226_t *dev, i2c_inst_t *i2c, uint8_t addr,
                  float shunt_resistance_ohms, float max_expected_amps) {
    dev->i2c = i2c;
    dev->addr = addr;

    // Per datasheet: CAL = 0.00512 / (current_lsb * r_shunt), with
    // current_lsb chosen so the 16-bit signed current register can just
    // reach max_expected_amps.
    dev->current_lsb_amps = max_expected_amps / 32768.0f;
    float calibration = 0.00512f / (dev->current_lsb_amps * shunt_resistance_ohms);
    if (calibration < 1.0f || calibration > 65535.0f) {
        // This shunt/current-range combination doesn't fit the chip's
        // 16-bit calibration register -- the caller passed values that
        // don't match real hardware.
        return false;
    }

    if (!write_reg16(dev, REG_CALIBRATION, (uint16_t)calibration)) {
        return false;
    }
    return write_reg16(dev, REG_CONFIG, CONFIG_CONTINUOUS_DEFAULT);
}

bool ina226_read_current_amps(ina226_t *dev, float *amps) {
    uint16_t raw;
    if (!read_reg16(dev, REG_CURRENT, &raw)) {
        return false;
    }
    *amps = (float)(int16_t)raw * dev->current_lsb_amps;
    return true;
}

bool ina226_read_bus_voltage_volts(ina226_t *dev, float *volts) {
    uint16_t raw;
    if (!read_reg16(dev, REG_BUS_VOLTAGE, &raw)) {
        return false;
    }
    *volts = (float)raw * BUS_VOLTAGE_LSB_VOLTS;
    return true;
}

bool ina226_read_power_watts(ina226_t *dev, float *watts) {
    uint16_t raw;
    if (!read_reg16(dev, REG_POWER, &raw)) {
        return false;
    }
    *watts = (float)raw * (POWER_LSB_FACTOR * dev->current_lsb_amps);
    return true;
}

bool ina226_read(ina226_t *dev, ina226_reading_t *out) {
    // Bus voltage, power and current live in three contiguous registers, so
    // one auto-incrementing burst read gets a consistent snapshot of all
    // three in a single I2C transaction.
    uint8_t reg = REG_BUS_VOLTAGE;
    int ret = i2c_write_blocking(dev->i2c, dev->addr, &reg, 1, true);
    if (ret != 1) {
        return false;
    }
    uint8_t buf[6];
    ret = i2c_read_blocking(dev->i2c, dev->addr, buf, sizeof(buf), false);
    if (ret != (int)sizeof(buf)) {
        return false;
    }

    uint16_t bus_raw = (uint16_t)((buf[0] << 8) | buf[1]);
    uint16_t power_raw = (uint16_t)((buf[2] << 8) | buf[3]);
    int16_t current_raw = (int16_t)((buf[4] << 8) | buf[5]);

    out->bus_voltage_volts = (float)bus_raw * BUS_VOLTAGE_LSB_VOLTS;
    out->power_watts = (float)power_raw * (POWER_LSB_FACTOR * dev->current_lsb_amps);
    out->current_amps = (float)current_raw * dev->current_lsb_amps;
    return true;
}
