# Firmware

Pico C SDK drivers and example firmware for the two I2C peripherals on this
board: the DS3231 real-time clock and the per-channel INA226 current/power
monitors.

## Drivers

- `drivers/ds3231` -- task-level RTC API: get/set the date and time, check
  whether the RTC lost power (and so can't be trusted), read the onboard
  temperature sensor. See `drivers/ds3231/include/ds3231.h`.
- `drivers/ina226` -- task-level current/voltage/power API: "how many amps
  is this channel drawing right now?" See `drivers/ina226/include/ina226.h`.

Neither exposes register addresses or raw I2C transactions in its public
API -- callers ask questions ("what time is it", "how many amps"), not poke
registers.

## Building

Requires the Pico SDK. Either point `PICO_SDK_PATH` at an existing checkout,
or let CMake fetch one:

```sh
cd firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
# or: cmake .. -DPICO_SDK_FETCH_FROM_GIT=on
make -j4
```

This builds two example firmware images (`rtc_time_example`,
`power_monitor_example`), each printing readings over USB serial.

## Before flashing on real hardware

Both examples have `TODO` comments marking values that need to be checked
against the schematic rather than trusted as written:

- **I2C peripheral and GPIO pins** -- both examples default to `i2c0` on
  GP4/GP5, which is a common convention but hasn't been confirmed against
  this board's actual `MCU_SDA`/`MCU_SCL` net routing on the RP2350B.
- **INA226 I2C address** -- the example uses the all-pins-low default
  (`0x40`). This board has one INA226 per channel, strapped to different
  addresses via A0/A1 -- check `Power.kicad_sch` for the real per-channel
  addresses.
- **`max_expected_amps`** -- set to whatever each channel's fuse/load is
  actually rated for. The shunt value (2 mΩ, Vishay WSK2512) is already
  correct, taken directly from `Power.kicad_sch`.

Also note `PICO_BOARD` is currently set to the stock `pico2` board
definition as a placeholder -- this is a custom RP2350B design, not a Pico 2
module, so a proper custom board header (correct flash size, VREG
configuration, default pin muxing) should replace it before this goes much
further.
