cmake_minimum_required(VERSION 3.13)

# This board is a custom RP2350B design (see ../universal-strip-controller.kicad_sch),
# not an off-the-shelf Pico/Pico2 module. "pico2" is a placeholder board
# definition: it gets the platform and toolchain defaults right, but should
# be replaced with a proper custom pico-sdk board header (flash size, VREG
# setup, default pin muxing) once this board's bring-up specifics are
# pinned down.
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(PICO_PLATFORM rp2350 CACHE STRING "Target platform")

include(pico_sdk_import.cmake)

project(universal_strip_controller_firmware C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# datetime_t (and datetime_to_str()) are normally only built into the SDK on
# RP2040, where they back the chip's own battery-backed RTC hardware
# (PICO_CONFIG default: PICO_INCLUDE_RTC_DATETIME=1 on RP2040, 0 elsewhere).
# This board's RP2350B has no such internal RTC -- timekeeping comes from
# the external DS3231 instead -- but datetime_t is still the natural,
# SDK-native struct for our driver's API, so pull it in explicitly. This
# must be set before pico_sdk_init() so the SDK's own pico_util sources are
# compiled with it too.
add_compile_definitions(PICO_INCLUDE_RTC_DATETIME=1)

pico_sdk_init()

add_subdirectory(drivers/ds3231)
add_subdirectory(drivers/ina226)

add_subdirectory(examples/rtc_time)
add_subdirectory(examples/power_monitor)
