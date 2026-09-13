#include "ds3231.h"

#include <string.h>

// DS3231 has no address pins -- it's always at 0x68.
#define DS3231_I2C_ADDR 0x68u

#define REG_SECONDS 0x00u
#define REG_STATUS  0x0Fu
#define REG_TEMP_MSB 0x11u

#define STATUS_OSF_BIT 0x80u

static inline uint8_t bcd_to_bin(uint8_t bcd) {
    return (uint8_t)((bcd >> 4) * 10 + (bcd & 0x0F));
}

static inline uint8_t bin_to_bcd(uint8_t bin) {
    return (uint8_t)(((bin / 10) << 4) | (bin % 10));
}

// Sakamoto's algorithm. Returns 0 = Sunday .. 6 = Saturday, matching
// pico/util/datetime.h's datetime_t::dotw convention.
static int day_of_week(int year, int month, int day) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
        year -= 1;
    }
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

static bool write_regs(i2c_inst_t *i2c, uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[8];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    int ret = i2c_write_blocking(i2c, DS3231_I2C_ADDR, buf, len + 1, false);
    return ret == (int)(len + 1);
}

static bool read_regs(i2c_inst_t *i2c, uint8_t reg, uint8_t *data, size_t len) {
    int ret = i2c_write_blocking(i2c, DS3231_I2C_ADDR, &reg, 1, true);
    if (ret != 1) {
        return false;
    }
    ret = i2c_read_blocking(i2c, DS3231_I2C_ADDR, data, len, false);
    return ret == (int)len;
}

bool ds3231_init(ds3231_t *rtc, i2c_inst_t *i2c) {
    rtc->i2c = i2c;
    uint8_t seconds;
    return read_regs(i2c, REG_SECONDS, &seconds, 1);
}

bool ds3231_get_datetime(ds3231_t *rtc, datetime_t *out) {
    uint8_t buf[7];
    if (!read_regs(rtc->i2c, REG_SECONDS, buf, sizeof(buf))) {
        return false;
    }

    out->sec = (int8_t)bcd_to_bin(buf[0] & 0x7Fu);
    out->min = (int8_t)bcd_to_bin(buf[1] & 0x7Fu);

    if (buf[2] & 0x40u) {
        // 12-hour mode: bit 5 is AM/PM, bits 0-4 are the BCD hour.
        int hour = bcd_to_bin(buf[2] & 0x1Fu) % 12;
        if (buf[2] & 0x20u) {
            hour += 12;
        }
        out->hour = (int8_t)hour;
    } else {
        out->hour = (int8_t)bcd_to_bin(buf[2] & 0x3Fu);
    }

    out->day = (int8_t)bcd_to_bin(buf[4] & 0x3Fu);

    bool century = (buf[5] & 0x80u) != 0;
    out->month = (int8_t)bcd_to_bin(buf[5] & 0x1Fu);
    out->year = (int16_t)(bcd_to_bin(buf[6]) + (century ? 2100 : 2000));

    // We don't trust the chip's own day-of-week register (its 1-7 mapping
    // is whatever it was last told, not necessarily consistent with the
    // date), so derive it ourselves.
    out->dotw = (int8_t)day_of_week(out->year, out->month, out->day);

    return true;
}

bool ds3231_set_datetime(ds3231_t *rtc, const datetime_t *dt) {
    bool century = dt->year >= 2100;
    uint8_t year_in_century = (uint8_t)(dt->year - (century ? 2100 : 2000));

    uint8_t buf[7] = {
        bin_to_bcd((uint8_t)dt->sec),
        bin_to_bcd((uint8_t)dt->min),
        bin_to_bcd((uint8_t)dt->hour), // bit 6 = 0 selects 24-hour mode
        bin_to_bcd((uint8_t)(dt->dotw + 1)), // chip wants 1-7, meaning is ours alone
        bin_to_bcd((uint8_t)dt->day),
        (uint8_t)(bin_to_bcd((uint8_t)dt->month) | (century ? 0x80u : 0u)),
        bin_to_bcd(year_in_century),
    };

    if (!write_regs(rtc->i2c, REG_SECONDS, buf, sizeof(buf))) {
        return false;
    }

    // Setting the time is the one point where we can vouch for it, so
    // clear the "oscillator stopped" flag.
    uint8_t status;
    if (!read_regs(rtc->i2c, REG_STATUS, &status, 1)) {
        return false;
    }
    status &= (uint8_t)~STATUS_OSF_BIT;
    return write_regs(rtc->i2c, REG_STATUS, &status, 1);
}

bool ds3231_lost_power(ds3231_t *rtc) {
    uint8_t status;
    if (!read_regs(rtc->i2c, REG_STATUS, &status, 1)) {
        // If we can't even talk to the chip, its time certainly can't be
        // trusted -- report that as "lost power" rather than silently
        // returning false-i.e.-fine.
        return true;
    }
    return (status & STATUS_OSF_BIT) != 0;
}

bool ds3231_get_temperature_c(ds3231_t *rtc, float *celsius) {
    uint8_t buf[2];
    if (!read_regs(rtc->i2c, REG_TEMP_MSB, buf, sizeof(buf))) {
        return false;
    }
    // The two registers together are a single 10-bit two's-complement value
    // in units of 0.25 degrees; combine them before scaling rather than
    // treating the whole-degree and fractional parts as separately signed.
    int16_t raw = (int16_t)((int8_t)buf[0] * 4) | (int16_t)(buf[1] >> 6);
    *celsius = (float)raw * 0.25f;
    return true;
}
