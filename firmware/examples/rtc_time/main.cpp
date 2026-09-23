// Minimal demonstration of the Ds3231 driver: "what time is it?"
#include <cstdio>
#include <cstring>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "ds3231.hpp"

// Confirmed by tracing universal-strip-controller.kicad_sch: the DS3231's
// MCU_SDA/MCU_SCL net lands on the RP2350B's GPIO38/GPIO39, matching the
// RP2350's fixed I2C1 SDA/SCL pin roles.
constexpr uint kRtcSdaPin = 38;
constexpr uint kRtcSclPin = 39;
constexpr uint kRtcI2cBaudrate = 100 * 1000;

// Seeds a datetime_t from this firmware's own build timestamp. Not a
// substitute for a real time source (wifi_rtc_sync's NTP sync, once wifi is
// wired up) -- it's only as accurate as "whenever this was compiled", and
// __DATE__/__TIME__ are the build machine's local time, not UTC -- but it
// turns a freshly-wired DS3231 from "reports garbage forever" into "starts
// ticking from roughly now", which is enough to confirm the wiring and
// coin-cell backup actually work before wifi is in the picture.
datetime_t seedDatetimeFromBuild() {
    static const char *monthNames = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char monthStr[4] = {0};
    int day, year, hour, minute, sec;
    std::sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
    std::sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &sec);

    const char *match = std::strstr(monthNames, monthStr);
    int month = match ? static_cast<int>((match - monthNames) / 3) + 1 : 1;

    datetime_t dt{};
    dt.year = static_cast<int16_t>(year);
    dt.month = static_cast<int8_t>(month);
    dt.day = static_cast<int8_t>(day);
    dt.hour = static_cast<int8_t>(hour);
    dt.min = static_cast<int8_t>(minute);
    dt.sec = static_cast<int8_t>(sec);
    dt.dotw = 0; // Ds3231::getDatetime() recomputes this from the date on read
    return dt;
}

int main() {
    stdio_init_all();

    i2c_init(i2c1, kRtcI2cBaudrate);
    gpio_set_function(kRtcSdaPin, GPIO_FUNC_I2C);
    gpio_set_function(kRtcSclPin, GPIO_FUNC_I2C);
    // Most DS3231 breakout boards already have their own pull-ups; these
    // just add a little extra margin.
    gpio_pull_up(kRtcSdaPin);
    gpio_pull_up(kRtcSclPin);

    Ds3231 rtc(i2c1);
    if (!rtc.init()) {
        printf("DS3231 not responding on the bus\n");
        while (true) {
            tight_loop_contents();
        }
    }

    if (rtc.lostPower()) {
        printf("RTC lost power since it was last set -- seeding it from this "
               "firmware's build time (%s %s) so it has something to tick "
               "from; run wifi_rtc_sync once wifi is wired up for a real "
               "time sync\n",
               __DATE__, __TIME__);
        if (!rtc.setDatetime(seedDatetimeFromBuild())) {
            printf("failed to seed RTC\n");
        }
    }

    while (true) {
        if (auto now = rtc.getDatetime()) {
            char text[32];
            datetime_to_str(text, sizeof(text), &*now);
            printf("%s\n", text);
        } else {
            printf("failed to read RTC\n");
        }
        sleep_ms(1000);
    }
}
