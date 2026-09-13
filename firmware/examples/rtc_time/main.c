// Minimal demonstration of the ds3231 driver: "what time is it?"
#include <stdio.h>
#include <string.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "ds3231.h"

// Confirmed by tracing universal-strip-controller.kicad_sch: the DS3231's
// MCU_SDA/MCU_SCL net lands on the RP2350B's GPIO38/GPIO39, matching the
// RP2350's fixed I2C1 SDA/SCL pin roles.
#define RTC_I2C_PORT i2c1
#define RTC_SDA_PIN 38
#define RTC_SCL_PIN 39
#define RTC_I2C_BAUDRATE (100 * 1000)

// Seeds a datetime_t from this firmware's own build timestamp. Not a
// substitute for a real time source (wifi_rtc_sync's NTP sync, once wifi is
// wired up) -- it's only as accurate as "whenever this was compiled", and
// __DATE__/__TIME__ are the build machine's local time, not UTC -- but it
// turns a freshly-wired DS3231 from "reports garbage forever" into "starts
// ticking from roughly now", which is enough to confirm the wiring and
// coin-cell backup actually work before wifi is in the picture.
static void seed_datetime_from_build(datetime_t *dt) {
    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month_str[4] = {0};
    int day, year, hour, min, sec;
    sscanf(__DATE__, "%3s %d %d", month_str, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    const char *match = strstr(month_names, month_str);
    int month = match ? (int)((match - month_names) / 3) + 1 : 1;

    dt->year = (int16_t)year;
    dt->month = (int8_t)month;
    dt->day = (int8_t)day;
    dt->hour = (int8_t)hour;
    dt->min = (int8_t)min;
    dt->sec = (int8_t)sec;
    dt->dotw = 0; // ds3231_get_datetime recomputes this from the date on read
}

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
        printf("RTC lost power since it was last set -- seeding it from this "
               "firmware's build time (%s %s) so it has something to tick "
               "from; run wifi_rtc_sync once wifi is wired up for a real "
               "time sync\n", __DATE__, __TIME__);
        datetime_t seed;
        seed_datetime_from_build(&seed);
        if (!ds3231_set_datetime(&rtc, &seed)) {
            printf("failed to seed RTC\n");
        }
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
