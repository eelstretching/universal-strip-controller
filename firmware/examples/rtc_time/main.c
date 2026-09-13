// Minimal demonstration of the ds3231 driver: "what time is it?"
#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "ds3231.h"

// TODO: confirm against universal-strip-controller.kicad_sch -- the DS3231
// sits on the MCU_SDA/MCU_SCL net. Update these to whichever I2C peripheral
// and GPIOs that net is actually routed to on this board's RP2350B.
#define RTC_I2C_PORT i2c0
#define RTC_SDA_PIN 4
#define RTC_SCL_PIN 5
#define RTC_I2C_BAUDRATE (100 * 1000)

int main(void) {
    stdio_init_all();

    i2c_init(RTC_I2C_PORT, RTC_I2C_BAUDRATE);
    gpio_set_function(RTC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL_PIN, GPIO_FUNC_I2C);
    // The board already has pull-ups on this net (see the schematic's "RTC
    // SDA/SCL pull-up" resistors); these just add a little extra margin.
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
