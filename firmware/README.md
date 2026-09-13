# Firmware

Pico C SDK drivers and example firmware for this board's RP2350B: the
DS3231 real-time clock, the per-channel INA226 current/power monitors, and
the RM2 wifi module used to keep the RTC synced over NTP.

Built and tested against pico-sdk 2.3.0.

## Board

`boards/universal_strip_controller.h` is a from-scratch pico-sdk board
header for this custom hardware (not adapted from `pico2.h`/`pico2_w.h` --
see the comment at the top of the file for why that matters). It carries
every pin/flash value this firmware relies on, confirmed by tracing
`universal-strip-controller.kicad_sch` and `Power.kicad_sch` (see
"Confirmed pins" below), so the examples don't hardcode them independently.

## Drivers

- `drivers/ds3231` -- task-level RTC API: get/set the date and time, check
  whether the RTC lost power (and so can't be trusted), read the onboard
  temperature sensor. See `drivers/ds3231/include/ds3231.h`.
- `drivers/ina226` -- task-level current/voltage/power API: "how many amps
  is this channel drawing right now?" See `drivers/ina226/include/ina226.h`.

Neither exposes register addresses or raw I2C transactions in its public
API -- callers ask questions ("what time is it", "how many amps"), not poke
registers.

## Examples

- `examples/rtc_time` -- reads the DS3231 once a second and prints it.
- `examples/power_monitor` -- reads one INA226 twice a second and prints
  amps/volts/watts.
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

`WIFI_SSID`/`WIFI_PASSWORD` are only needed for `wifi_rtc_sync`; the other
two examples build fine without them (with a warning).

## Confirmed pins

Traced directly from the schematics (wires, junctions and global labels
followed net-by-net to the RP2350B's pins), not assumed by convention:

| Signal | Net name in schematic | RP2350B pin |
|---|---|---|
| I2C1 SDA (DS3231 + all 4 INA226s) | `MCU_SDA` | GPIO38 |
| I2C1 SCL (DS3231 + all 4 INA226s) | `MCU_SCL` | GPIO39 |
| DS3231 `INT`/`SQW` | `RTC_INT` | GPIO37 |
| RM2 wifi module SPI clock | `RM2_SCLK` | GPIO40 |
| RM2 wifi module chip select | `RM2_CS` | GPIO41 |
| RM2 wifi module data (bidirectional) | `RM2_DI_DO` | GPIO42 |
| RM2 wifi module `WL_ON`/`BT_ON` | `RM2_BT_WL_ON` | GPIO43 |

**Important: the schematic's `MCU_SDA`/`MCU_SCL` net names are swapped
relative to the RP2350's fixed hardware I2C roles.** The RP2350 datasheet
(confirmed directly from the SDK's own `io_bank0.h` register definitions)
fixes GPIO38's I2C alternate function as **I2C1 SDA** and GPIO39's as
**I2C1 SCL** -- silicon-fixed, not something firmware can reassign. But the
schematic wires the net it calls `MCU_SDA` to GPIO39, and `MCU_SCL` to
GPIO38 -- the opposite pairing. Every example here uses the
hardware-correct roles (GPIO38=SDA, GPIO39=SCL) so the hardware I2C1
peripheral actually works; if you wire a breadboard by the schematic's net
*names* instead, I2C will not work. This looks like a genuine swap worth
fixing in the schematic (or documenting clearly) before the PCB is
fabricated -- right now, using the real hardware I2C1 peripheral and
matching the schematic's own net names are mutually exclusive.

## Other things found while tracing the schematic

Worth knowing about, though none of them block breadboard bring-up with
generic breakout modules:

- **The DS3231's own SDA/SCL pins aren't actually wired to the MCU_SDA/
  MCU_SCL nets in the current schematic.** SDA is floating (no wire at
  all); SCL has an explicit `no_connect` flag on it. The nearby pull-up
  resistors (R94/R95/R96) do land on the right global-label nets, but
  nothing connects from there to the DS3231 chip itself.
- **None of the four INA226s have their SCL pin wired to anything** --
  only SDA is connected. Their `A0`/`A1` address-strap pins are also all
  left floating, which means as drawn, all four would sit at the same
  default I2C address (`0x40`) -- a real collision once more than one is on
  the bus.

These read as unfinished routing rather than deliberate choices, given how
much of the rest of the design is complete. Worth a pass before this goes
to fab; doesn't affect testing on a breadboard with off-the-shelf breakout
boards, since those are wired by hand rather than by the PCB traces.

## `max_expected_amps` in the power monitor example

Set to whatever each channel's fuse/load is actually rated for -- it sets
the chip's current resolution, so a wildly wrong value either clips real
readings or throws away precision. The shunt value (2 mΩ, Vishay WSK2512)
is already correct, taken directly from `Power.kicad_sch`.
