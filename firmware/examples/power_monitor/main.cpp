// Minimal demonstration of the Ina226 driver: "how many amps is this thing
// drawing right now?"
#include <cstdio>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "ina226.hpp"

// Confirmed by tracing Power.kicad_sch: all four INA226es share the same
// MCU_SDA/MCU_SCL bus as the RTC, on the RP2350B's GPIO38/GPIO39, matching
// the RP2350's fixed I2C1 SDA/SCL pin roles.
constexpr uint kPowerSdaPin = 38;
constexpr uint kPowerSclPin = 39;
constexpr uint kPowerI2cBaudrate = 100 * 1000;

// Per-channel addresses, confirmed against Power.kicad_sch's A0/A1 strapping
// (each pin tied directly to GND or VS -- no resistors needed, per the
// INA226 datasheet's address table):
//   channel 1 (U12): A1=GND A0=GND -> 0x40
//   channel 2 (U13): A1=GND A0=VS  -> 0x41
//   channel 3 (U14): A1=VS  A0=GND -> 0x44
//   channel 4 (U15): A1=VS  A0=VS  -> 0x45
// This example only demonstrates one channel; swap the address to read a
// different one, or construct a second Ina226 for each.
constexpr uint8_t kChannel1Addr = 0x40u;

// The real board's shunt is a 2 mOhm Vishay WSK2512 (Power.kicad_sch), but
// that's an expensive, special-order part -- for breadboard testing with a
// generic INA226 breakout, 0.01 ohm is a far easier find. This value MUST
// match whatever resistor is actually soldered on, or every reading here
// will be systematically wrong by the ratio of assumed-to-actual shunt
// value. Change back to 0.002f once this runs on the real board.
constexpr float kChannel1ShuntOhms = 0.01f;

// The INA226's shunt-voltage ADC saturates at +-81.92 mV regardless of
// shunt value, which puts a hard ceiling on measurable current for a given
// shunt: ~41 A at 2 mOhm, but only ~8.2 A at 0.01 ohm. maxExpectedAmps just
// needs to be at or under that ceiling (it sets the chip's current
// resolution -- a wildly wrong value either clips real readings or throws
// away precision); 5 A leaves comfortable margin for bench testing.
constexpr float kChannel1MaxAmps = 5.0f;

int main() {
    stdio_init_all();

    i2c_init(i2c1, kPowerI2cBaudrate);
    gpio_set_function(kPowerSdaPin, GPIO_FUNC_I2C);
    gpio_set_function(kPowerSclPin, GPIO_FUNC_I2C);
    gpio_pull_up(kPowerSdaPin);
    gpio_pull_up(kPowerSclPin);

    Ina226 channel1(i2c1, kChannel1Addr, kChannel1ShuntOhms, kChannel1MaxAmps);
    if (!channel1.init()) {
        printf("INA226 not responding, or shunt/current-range values don't fit "
               "its calibration register\n");
        while (true) {
            tight_loop_contents();
        }
    }

    while (true) {
        if (auto reading = channel1.read()) {
            printf("%.3f A, %.2f V, %.2f W\n", reading->currentAmps, reading->busVoltageVolts,
                   reading->powerWatts);
        } else {
            printf("failed to read INA226\n");
        }
        sleep_ms(500);
    }
}
