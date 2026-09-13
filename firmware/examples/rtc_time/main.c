// Minimal demonstration of the ds3231 driver: "what time is it?"
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "ds3231.h"

// Confirmed by tracing universal-strip-controller.kicad_sch: the DS3231's
// MCU_SDA/MCU_SCL net lands on the RP2350B's GPIO39/GPIO38.
//
// IMPORTANT: the RP2350 datasheet fixes GPIO38 as I2C1 SDA and GPIO39 as
// I2C1 SCL -- the opposite of what the schematic's net names suggest. The
// pins below follow the hardware-correct roles (required for the hardware
// I2C1 peripheral to work at all), not the schematic's net names. See
// firmware/README.md for the full explanation; this looks like a genuine
// swap worth fixing in the schematic before this board is fabricated.
#define RTC_I2C_PORT i2c1
#define RTC_SDA_PIN 38
#define RTC_SCL_PIN 39
#define RTC_I2C_BAUDRATE (100 * 1000)

int main(void) {
    stdio_init_all();

    i2c_init(RTC_I2C_PORT, RTC_I2C_BAUDRATE);
    gpio_set_function(RTC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL_PIN, GPIO_FUNC_I2C);
    // Most DS3231 breakout boards already have their own pull-ups; these
    // just add a little extra margin.
    gpio_pull_up(RTC_SDA_PIN);
    gpio_pull_up(RTC_SCL_PIN);

    ds3231_t rtc;
    if (!ds3231_init(&rtc, RTC_I2C_PORT)) {
        printf("DS3231 not responding on the bus\n");
        while (true) {
            tight_loop_contents();
        }
    }

    if (ds3231_lost_power(&rtc)) {
        printf("RTC lost power since it was last set -- time below is not trustworthy "
               "until ds3231_set_datetime() is called\n");
    }

    while (true) {
        datetime_t now;
        if (ds3231_get_datetime(&rtc, &now)) {
            char text[32];
            datetime_to_str(text, sizeof(text), &now);
            printf("%s\n", text);
        } else {
            printf("failed to read RTC\n");
        }
        sleep_ms(1000);
    }
}
