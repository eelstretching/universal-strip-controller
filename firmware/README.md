# Firmware

Pico SDK drivers and example firmware for this board's RP2350B: the
DS3231 real-time clock and the RM2 wifi module used to keep the RTC synced over NTP.

Drivers are C++ classes (this project targets C++17); the SDK glue
underneath is still C, as pico-sdk itself is.

Built and tested against pico-sdk 2.3.0.

## Board

`boards/universal_strip_controller.h` is a from-scratch pico-sdk board
header for this custom hardware (not adapted from `pico2.h`/`pico2_w.h` --
see the comment at the top of the file for why that matters). It carries
every pin/flash value this firmware relies on, confirmed against
`universal-strip-controller.kicad_sch`'s netlist (see
"Confirmed pins" below), so the examples don't hardcode them independently.

## Drivers

- `drivers/ds3231` -- a `Ds3231` class: get/set the date and time, check
  whether the RTC lost power (and so can't be trusted), read the onboard
  temperature sensor. See `drivers/ds3231/include/ds3231.hpp`.

It doesn't expose register addresses or raw I2C transactions in its public
API -- callers ask questions (`rtc.getDatetime()`), not poke
registers. Fallible reads return `std::optional` rather than an
out-param plus a bool, e.g.:

```cpp
Ds3231 rtc(i2c1);
rtc.init();
if (auto now = rtc.getDatetime()) {
    // use *now
}
```

## Examples

- `examples/rtc_time` -- reads the DS3231 once a second and prints it.
- `examples/wifi_rtc_sync` -- connects to WiFi, fetches the time over NTP
  (request/response handling adapted from pico-sdk's own
  `pico_w/wifi/ntp_client` example), writes it to the DS3231, then prints
  the RTC's time once a second to show it's actually ticking with the
  synced value.

## Building

Requires the Pico SDK. Either point `PICO_SDK_PATH` at an existing 2.3.0+
checkout, or let CMake fetch one:

```sh
cd firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk -DWIFI_SSID=yourssid -DWIFI_PASSWORD=yourpassword
# or: cmake .. -DPICO_SDK_FETCH_FROM_GIT=on -DWIFI_SSID=... -DWIFI_PASSWORD=...
make -j4
```

`WIFI_SSID`/`WIFI_PASSWORD` are only needed for `wifi_rtc_sync`; `rtc_time` builds
fine without them (with a warning).

## Confirmed pins

Taken from KiCad's exported netlist of `universal-strip-controller.kicad_sch`
(not hand-traced -- an earlier hand trace got these wrong):

| Signal | Net name in schematic | RP2350B pin |
|---|---|---|
| I2C1 SDA (DS3231) | `MCU_SDA` | GPIO42 |
| I2C1 SCL (DS3231) | `MCU_SCL` | GPIO43 |
| DS3231 `INT`/`SQW` | `RTC_INT` | GPIO44 |
| RM2 wifi module `WL_ON`/`BT_ON` | `RM2_BT_WL_ON` | GPIO38 |
| RM2 wifi module data (bidirectional) | `RM2_DI_DO` | GPIO39 |
| RM2 wifi module chip select | `RM2_CS` | GPIO40 |
| RM2 wifi module SPI clock | `RM2_SCLK` | GPIO41 |

The RP2350's I2C alternate functions are fixed by GPIO number (n mod 4:
0 = I2C0 SDA, 1 = I2C0 SCL, 2 = I2C1 SDA, 3 = I2C1 SCL), so GPIO42/GPIO43
are I2C1 SDA/SCL, matching the net names.
