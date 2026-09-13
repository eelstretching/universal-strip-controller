// INA226 current/voltage/power monitor driver.
//
// Like the DS3231 driver, this deliberately exposes questions
// ("how many amps right now?"), not registers. The one piece of chip detail
// callers can't avoid is ina226_init()'s shunt resistance and expected
// current range -- the chip needs both to scale its internal math, and
// there's no way to hide that without guessing wrong for your hardware.
#ifndef USC_INA226_H
#define USC_INA226_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// One INA226 on the bus. Multiple can share an I2C bus at different
// addresses (set by the chip's A0/A1 strapping pins), which is why the
// address and per-chip calibration live in the handle rather than being
// global.
typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
    float current_lsb_amps; // amps represented by 1 LSB of the current register
} ina226_t;

typedef struct {
    float bus_voltage_volts;
    float current_amps;
    float power_watts;
} ina226_reading_t;

// Configure one INA226 for the shunt resistor it's actually wired to and the
// largest current you ever expect it to see. Those two numbers are how the
// chip scales its internal current/power math, so getting them right (from
// your BOM and the load you're monitoring) matters more than anything else
// here.
//
// addr is the chip's 7-bit I2C address (0x40-0x4F depending on how A0/A1
// are strapped).
//
// Returns false if the chip doesn't answer on the bus.
bool ina226_init(ina226_t *dev, i2c_inst_t *i2c, uint8_t addr,
                  float shunt_resistance_ohms, float max_expected_amps);

// "How many amps is this thing drawing right now?" Positive current flows
// from IN+ to IN- through the shunt.
bool ina226_read_current_amps(ina226_t *dev, float *amps);

// Bus voltage at the load side of the shunt.
bool ina226_read_bus_voltage_volts(ina226_t *dev, float *volts);

// Power being delivered to the load (the chip computes this in hardware
// from the current and bus voltage measurements).
bool ina226_read_power_watts(ina226_t *dev, float *watts);

// All three readings together. Prefer this over calling the three
// functions above separately when you want a consistent snapshot -- it's
// also cheaper, since it's one round of register reads instead of three.
bool ina226_read(ina226_t *dev, ina226_reading_t *out);

#ifdef __cplusplus
}
#endif

#endif // USC_INA226_H
