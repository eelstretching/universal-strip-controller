// Minimal demonstration of the ina226 driver: "how many amps is this thing
// drawing right now?"
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "ina226.h"

// TODO: confirm against Power.kicad_sch -- same MCU_SDA/MCU_SCL bus as the
// RTC. Update to match this board's actual I2C peripheral/GPIOs.
#define POWER_I2C_PORT i2c0
#define POWER_SDA_PIN 4
#define POWER_SCL_PIN 5
#define POWER_I2C_BAUDRATE (100 * 1000)

// Default INA226 address with A1/A0 both strapped low. This board has one
// INA226 per channel at different addresses (see the A0/A1 strapping in
// Power.kicad_sch) -- pick the right one for the channel you're reading.
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
