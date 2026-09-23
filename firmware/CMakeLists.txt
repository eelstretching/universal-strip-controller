cmake_minimum_required(VERSION 3.13)

# This board is a custom RP2350B design (see ../universal-strip-controller.kicad_sch
# and ../Power.kicad_sch) -- not an off-the-shelf Pico/Pico2/Pico2 W module,
# so it gets its own board header (boards/universal_strip_controller.h)
# rather than reusing a stock one. Using the stock "pico2" board here would
# be actively wrong, not just approximate: pico2.h sets PICO_RP2350A, which
# caps the SDK at 30 GPIOs, and this board's I2C bus and RM2 wifi module
# both live on GPIO38 and up (only present on the 48-GPIO RP2350B this
# board actually has).
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)
set(PICO_BOARD universal_strip_controller CACHE STRING "Board type")

# PICO_BOARD_HEADER_DIRS only tells CMake where to look up the *selected*
# board file by name -- it does NOT add that directory to the compiler's
# include search path. A board header that does #include "boards/other.h"
# (the officially-supported way to compose board headers, e.g. how
# pico2_w.h can be included by others) needs that to actually resolve at
# compile time. The SDK's own boards get away with this only because
# ${PICO_SDK_PATH}/src/boards/include is unconditionally on every target's
# include path; ours never was, so waveshare_core2350b_prototype.h's
# #include "boards/waveshare_core2350b.h" would 404 without this.
include_directories(${CMAKE_CURRENT_LIST_DIR})

# WiFi credentials for the wifi_rtc_sync example, e.g.:
#   cmake .. -DWIFI_SSID=... -DWIFI_PASSWORD=...
set(WIFI_SSID "" CACHE STRING "WiFi SSID for wifi_rtc_sync")
set(WIFI_PASSWORD "" CACHE STRING "WiFi password for wifi_rtc_sync")

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

add_subdirectory(examples/rtc_time)
add_subdirectory(examples/wifi_rtc_sync)
