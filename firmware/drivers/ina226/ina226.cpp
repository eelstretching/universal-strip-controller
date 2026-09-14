#include "ina226.hpp"

namespace {

constexpr uint8_t kRegConfig = 0x00u;
constexpr uint8_t kRegBusVoltage = 0x02u;
constexpr uint8_t kRegPower = 0x03u;
constexpr uint8_t kRegCurrent = 0x04u;
constexpr uint8_t kRegCalibration = 0x05u;

// Chip's power-on-reset default: continuous shunt + bus voltage conversion.
// Written explicitly at init so behaviour doesn't depend on whatever a
// previous boot (or a debugger session) left the chip configured as.
constexpr uint16_t kConfigContinuousDefault = 0x4127u;

constexpr float kBusVoltageLsbVolts = 0.00125f; // 1.25 mV per datasheet
constexpr float kPowerLsbFactor = 25.0f;        // power_lsb = 25 * current_lsb, per datasheet

} // namespace

Ina226::Ina226(i2c_inst_t *i2c, uint8_t addr, float shuntResistanceOhms, float maxExpectedAmps)
    : i2c_(i2c),
      addr_(addr),
      currentLsbAmps_(maxExpectedAmps / 32768.0f),
      // Per datasheet: CAL = 0.00512 / (current_lsb * r_shunt). Computed
      // here so init() only has to validate range and write it; a bad
      // shunt/current-range combination shows up as calibration_ landing
      // outside the chip's 16-bit register, checked in init().
      calibration_(0.00512f / (currentLsbAmps_ * shuntResistanceOhms)) {}

bool Ina226::writeReg16(uint8_t reg, uint16_t value) const {
    uint8_t buf[3] = {reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFFu)};
    int ret = i2c_write_blocking(i2c_, addr_, buf, sizeof(buf), false);
    return ret == static_cast<int>(sizeof(buf));
}

bool Ina226::readReg16(uint8_t reg, uint16_t *value) const {
    int ret = i2c_write_blocking(i2c_, addr_, &reg, 1, true);
    if (ret != 1) {
        return false;
    }
    uint8_t buf[2];
    ret = i2c_read_blocking(i2c_, addr_, buf, sizeof(buf), false);
    if (ret != static_cast<int>(sizeof(buf))) {
        return false;
    }
    *value = static_cast<uint16_t>((buf[0] << 8) | buf[1]);
    return true;
}

bool Ina226::init() const {
    if (calibration_ < 1.0f || calibration_ > 65535.0f) {
        // This shunt/current-range combination doesn't fit the chip's
        // 16-bit calibration register -- the caller passed values that
        // don't match real hardware.
        return false;
    }
    if (!writeReg16(kRegCalibration, static_cast<uint16_t>(calibration_))) {
        return false;
    }
    return writeReg16(kRegConfig, kConfigContinuousDefault);
}

std::optional<float> Ina226::readCurrentAmps() const {
    uint16_t raw;
    if (!readReg16(kRegCurrent, &raw)) {
        return std::nullopt;
    }
    return static_cast<float>(static_cast<int16_t>(raw)) * currentLsbAmps_;
}

std::optional<float> Ina226::readBusVoltageVolts() const {
    uint16_t raw;
    if (!readReg16(kRegBusVoltage, &raw)) {
        return std::nullopt;
    }
    return static_cast<float>(raw) * kBusVoltageLsbVolts;
}

std::optional<float> Ina226::readPowerWatts() const {
    uint16_t raw;
    if (!readReg16(kRegPower, &raw)) {
        return std::nullopt;
    }
    return static_cast<float>(raw) * (kPowerLsbFactor * currentLsbAmps_);
}

std::optional<Ina226::Reading> Ina226::read() const {
    // Bus voltage, power and current live in three contiguous registers, so
    // one auto-incrementing burst read gets a consistent snapshot of all
    // three in a single I2C transaction.
    uint8_t reg = kRegBusVoltage;
    int ret = i2c_write_blocking(i2c_, addr_, &reg, 1, true);
    if (ret != 1) {
        return std::nullopt;
    }
    uint8_t buf[6];
    ret = i2c_read_blocking(i2c_, addr_, buf, sizeof(buf), false);
    if (ret != static_cast<int>(sizeof(buf))) {
        return std::nullopt;
    }

    uint16_t busRaw = static_cast<uint16_t>((buf[0] << 8) | buf[1]);
    uint16_t powerRaw = static_cast<uint16_t>((buf[2] << 8) | buf[3]);
    int16_t currentRaw = static_cast<int16_t>((buf[4] << 8) | buf[5]);

    Reading out{};
    out.busVoltageVolts = static_cast<float>(busRaw) * kBusVoltageLsbVolts;
    out.powerWatts = static_cast<float>(powerRaw) * (kPowerLsbFactor * currentLsbAmps_);
    out.currentAmps = static_cast<float>(currentRaw) * currentLsbAmps_;
    return out;
}
