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

The RP2350 datasheet (confirmed directly from the SDK's own `io_bank0.h`
register definitions) fixes GPIO38's I2C alternate function as **I2C1
SDA** and GPIO39's as **I2C1 SCL** -- silicon-fixed, not something firmware
can reassign. The `MCU_SDA`/`MCU_SCL` net names above now match that
(GPIO38=SDA, GPIO39=SCL): the schematic originally had them swapped
(`MCU_SDA` wired to GPIO39, `MCU_SCL` to GPIO38), which has since been
fixed directly in `universal-strip-controller.kicad_sch` by swapping the
two global labels at the MCU end -- everything else on each net (pull-up
resistors, the DS3231, all four INA226s) picked up the correct net
automatically, since KiCad global labels connect by name project-wide
regardless of physical wire path.

**This fix is schematic-only -- the PCB layout has not been re-synced.**
Run KiCad's "Update PCB from Schematic" before laying out `.kicad_pcb`
further, or the two will disagree about the SDA/SCL assignment.

## Other things found while tracing the schematic

- **Fixed:** the DS3231's own SDA/SCL pins weren't actually wired to the
  MCU_SDA/MCU_SCL nets (SDA was floating; SCL had an explicit `no_connect`
  flag). Both now have a direct stub to the corresponding global label.
- **Fixed:** none of the four INA226s had their SCL pin wired to anything
  -- only SDA was connected. Each one turned out to be one short stub wire
  away from an already-correctly-wired pull-up/label network (the rest of
  that network was already in place), so each channel just needed that
  one missing wire added.
- **Fixed:** the four INA226s' `A0`/`A1` address pins weren't wired to
  match the address scheme the schematic itself documents (text
  annotations next to each chip: `0x40`/`0x41`/`0x44`/`0x45` via A1×A0 ∈
  {GND, VS}). Channels 1 and 2 already had A1 correctly left floating
  (`no_connect`, reads as GND) for their `A1: GND` requirement; channels 3
  and 4 (U14, U15) had that same `no_connect` on A1 even though their
  documented addresses need A1 tied to VS -- removed those and wired A1 to
  each chip's own VS rail instead. Wired A0 the same way per channel
  (direct tie to GND via `no_connect`, or to VS via a wire -- no resistors
  needed, per the INA226 datasheet's address table). Verified all four
  chips now resolve to their documented, distinct addresses.

None of this blocks breadboard bring-up with generic breakout modules,
since those are wired by hand rather than by the PCB traces.

## `max_expected_amps` in the power monitor example

Set to whatever each channel's fuse/load is actually rated for -- it sets
the chip's current resolution, so a wildly wrong value either clips real
readings or throws away precision. The shunt value (2 mΩ, Vishay WSK2512)
is already correct, taken directly from `Power.kicad_sch`.
