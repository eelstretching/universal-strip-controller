// DS3231 real-time clock driver.
//
// This is a task-level API, not a register map: callers ask for the time,
// set the time, or check whether the time is trustworthy, and never see an
// I2C transaction or a BCD byte.
#ifndef USC_DS3231_H
#define USC_DS3231_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/util/datetime.h"

#ifdef __cplusplus
extern "C" {
#endif

// One DS3231 on the bus. The chip has a fixed I2C address (0x68, no address
// pins), so a handle is just the I2C peripheral it's wired to.
typedef struct {
    i2c_inst_t *i2c;
} ds3231_t;

// Bind a handle to the I2C peripheral the DS3231 is on. The caller must have
// already configured that peripheral (i2c_init(), SDA/SCL pin functions,
// pull-ups) before calling this.
//
// Returns false if the chip doesn't answer on the bus.
bool ds3231_init(ds3231_t *rtc, i2c_inst_t *i2c);

// Read the current date and time.
bool ds3231_get_datetime(ds3231_t *rtc, datetime_t *out);

// Set the date and time, e.g. after a fresh battery install or a sync
// against a trusted clock. Also clears the "lost power" flag.
bool ds3231_set_datetime(ds3231_t *rtc, const datetime_t *dt);

// True if the RTC can't vouch for its own time: oscillator has stopped at
// some point since it was last set (dead/missing backup battery, chip never
// set after first power-up, etc). Check this before trusting
// ds3231_get_datetime() after a power cycle.
bool ds3231_lost_power(ds3231_t *rtc);

// Chip's onboard temperature sensor, in degrees Celsius. Updated by the
// chip roughly every 64 seconds; not a substitute for a real ambient
// temperature sensor, but useful as a rough die/enclosure reading.
bool ds3231_get_temperature_c(ds3231_t *rtc, float *celsius);

#ifdef __cplusplus
}
#endif

#endif // USC_DS3231_H
