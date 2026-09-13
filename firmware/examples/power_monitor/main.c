// Minimal demonstration of the ina226 driver: "how many amps is this thing
// drawing right now?"
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "ina226.h"

// Confirmed by tracing Power.kicad_sch: all four INA226es share the same
// MCU_SDA/MCU_SCL bus as the RTC, on the RP2350B's GPIO38/GPIO39, matching
// the RP2350's fixed I2C1 SDA/SCL pin roles.
#define POWER_I2C_PORT i2c1
#define POWER_SDA_PIN 38
#define POWER_SCL_PIN 39
#define POWER_I2C_BAUDRATE (100 * 1000)

// Default INA226 address with A1/A0 both strapped low (A0/A1 float to GND
// when left unconnected). NOTE: as of this writing, Power.kicad_sch doesn't
// actually strap A0/A1 differently per channel -- all four INA226s would
// sit at this same default address, which won't work once more than one is
// on the bus at once. Fine for wiring up a single breakout on a breadboard;
// worth fixing (give each channel a distinct address) before there are four
// real channels on one bus.
#define CHANNEL_1_ADDR 0x40u

// Each channel's shunt is a 2 mΩ Vishay WSK2512 (Power.kicad_sch). Adjust
// max_expected_amps to whatever this channel's fuse/load is actually rated
// for -- it sets the chip's current resolution, so a wildly wrong value
// either clips real readings or throws away precision.
#define CHANNEL_1_SHUNT_OHMS 0.002f
#define CHANNEL_1_MAX_AMPS 10.0f

int main(void) {
    stdio_init_all();

    i2c_init(POWER_I2C_PORT, POWER_I2C_BAUDRATE);
    gpio_set_function(POWER_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(POWER_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(POWER_SDA_PIN);
    gpio_pull_up(POWER_SCL_PIN);

    ina226_t channel_1;
    if (!ina226_init(&channel_1, POWER_I2C_PORT, CHANNEL_1_ADDR,
                      CHANNEL_1_SHUNT_OHMS, CHANNEL_1_MAX_AMPS)) {
        printf("INA226 not responding, or shunt/current-range values don't fit "
               "its calibration register\n");
        while (true) {
            tight_loop_contents();
        }
    }

    while (true) {
        ina226_reading_t reading;
        if (ina226_read(&channel_1, &reading)) {
            printf("%.3f A, %.2f V, %.2f W\n", reading.current_amps,
                   reading.bus_voltage_volts, reading.power_watts);
        } else {
            printf("failed to read INA226\n");
        }
        sleep_ms(500);
    }
}
