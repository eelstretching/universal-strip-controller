// Board header for this project's custom RP2350B design
// (universal-strip-controller.kicad_sch / Power.kicad_sch), for use with
// PICO_BOARD=universal_strip_controller.
//
// This is NOT a Raspberry Pi board -- there is no stock pico-sdk header for
// this hardware, so this one is written from scratch against the schematic
// rather than adapted from pico2.h/pico2_w.h. In particular it must NOT
// define PICO_RP2350A: that macro (set by pico2.h/pico2_w.h, since Pico 2
// and Pico 2 W both use the 30-GPIO RP2350A package) caps NUM_BANK0_GPIOS
// at 30, which would silently break anything using this board's GPIO37+
// (the DS3231/INA226 I2C bus and the RM2 wifi module both live above 30).
// Leaving it undefined keeps the SDK's own default of 0, i.e. the 48-GPIO
// RP2350B package this board actually has.
//
// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_UNIVERSAL_STRIP_CONTROLLER_H
#define _BOARDS_UNIVERSAL_STRIP_CONTROLLER_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define UNIVERSAL_STRIP_CONTROLLER

// --- FLASH ---
// W25Q128JVSIQ (universal-strip-controller.kicad_sch), 128 Mbit = 16 MB.
// The W25Q080 boot_stage2 variant covers the whole W25Q family's QSPI
// fast-read enable sequence, not just the 8 Mbit part -- this is the same
// approach the SDK's own boards use for their (differently sized) Winbond
// flash.
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// --- I2C ---
// DS3231 RTC + four INA226 power monitors, confirmed by tracing
// universal-strip-controller.kicad_sch/Power.kicad_sch: the MCU_SDA/MCU_SCL
// nets land on GPIO38/GPIO39, matching the RP2350's fixed I2C1 SDA/SCL pin
// roles.
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 1
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 38
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 39
#endif

// --- RM2 wireless module (CYW43439) ---
// Confirmed by tracing universal-strip-controller.kicad_sch's RM2_SCLK/
// RM2_CS/RM2_DI_DO/RM2_BT_WL_ON nets to the RP2350B. Same 4-signal
// interface as the official Pico 2 W (boards/pico2_w.h), just on different
// GPIOs -- no separate host-wake/IRQ pin, matching Pico 2 W's use of the
// shared data line for that.
#ifndef CYW43_PIN_WL_DYNAMIC
#define CYW43_PIN_WL_DYNAMIC 0
#endif

#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 43u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 42u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 42u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 42u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 40u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 41u
#endif

#endif
