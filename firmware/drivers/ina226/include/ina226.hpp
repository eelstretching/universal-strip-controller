// INA226 current/voltage/power monitor driver.
//
// Like the DS3231 driver, this deliberately exposes questions
// ("how many amps right now?"), not registers. The one piece of chip detail
// callers can't avoid is the constructor's shunt resistance and expected
// current range -- the chip needs both to scale its internal math, and
// there's no way to hide that without guessing wrong for your hardware.
#ifndef USC_INA226_HPP
#define USC_INA226_HPP

#include <cstdint>
#include <optional>

#include "hardware/i2c.h"

// One INA226 on the bus. Multiple can share an I2C bus at different
// addresses (set by the chip's A0/A1 strapping pins), which is why the
// address and per-chip calibration live in the instance rather than being
// global.
class Ina226 {
public:
    struct Reading {
        float busVoltageVolts;
        float currentAmps;
        float powerWatts;
    };

    // shuntResistanceOhms and maxExpectedAmps are how the chip scales its
    // internal current/power math, so getting them right (from your BOM
    // and the load you're monitoring) matters more than anything else here.
    //
    // addr is the chip's 7-bit I2C address (0x40-0x4F depending on how
    // A0/A1 are strapped).
    Ina226(i2c_inst_t *i2c, uint8_t addr, float shuntResistanceOhms, float maxExpectedAmps);

    // Writes the calibration this instance was constructed with to the
    // chip. Call once before using the rest of the API. Returns false if
    // the chip doesn't answer on the bus, or if the shunt/current-range
    // combination doesn't fit the chip's calibration register.
    bool init() const;

    // "How many amps is this thing drawing right now?" Positive current
    // flows from IN+ to IN- through the shunt. nullopt on an I2C error.
    std::optional<float> readCurrentAmps() const;

    // Bus voltage at the load side of the shunt. nullopt on an I2C error.
    std::optional<float> readBusVoltageVolts() const;

    // Power being delivered to the load (the chip computes this in
    // hardware from the current and bus voltage measurements). nullopt on
    // an I2C error.
    std::optional<float> readPowerWatts() const;

    // All three readings together. Prefer this over calling the three
    // methods above separately when you want a consistent snapshot -- it's
    // also cheaper, since it's one round of register reads instead of
    // three. nullopt on an I2C error.
    std::optional<Reading> read() const;

private:
    i2c_inst_t *i2c_;
    uint8_t addr_;
    float currentLsbAmps_; // amps represented by 1 LSB of the current register
    float calibration_;    // computed from shunt/max-current at construction; written by init()

    bool writeReg16(uint8_t reg, uint16_t value) const;
    bool readReg16(uint8_t reg, uint16_t *value) const;
};

#endif // USC_INA226_HPP
