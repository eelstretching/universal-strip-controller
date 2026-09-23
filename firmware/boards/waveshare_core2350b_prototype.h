// Prototyping board: a bare Waveshare Core2350B RP2350B breakout, with this
// project's I2C1 bus (DS3231) wired to external breakout modules
// by hand rather than by PCB trace. Composes with, rather than edits,
// waveshare_core2350b.h (Waveshare's own board definition) and
// universal_strip_controller.h (the real board) -- all three stay
// independent so none of them drift out of sync with what they each
// actually describe.
//
// Select with -DPICO_BOARD=waveshare_core2350b_prototype.
//
// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_WAVESHARE_CORE2350B_PROTOTYPE_H
#define _BOARDS_WAVESHARE_CORE2350B_PROTOTYPE_H

// --- I2C ---
// Same GPIO42/GPIO43 I2C1 SDA/SCL assignment as universal_strip_controller.h
// (see that file's comments for why these two specific pins, not the
// RP2350's generic default I2C0 on GPIO4/5). Defined before including the
// base board header below, since its own PICO_DEFAULT_I2C_* only apply
// under an #ifndef guard.
#define PICO_DEFAULT_I2C 1
#define PICO_DEFAULT_I2C_SDA_PIN 42
#define PICO_DEFAULT_I2C_SCL_PIN 43

// Pulls in Waveshare's own board definition for everything that's actually
// specific to this physical board: RP2350B variant (confirms 48-GPIO
// PICO_RP2350A=0, same as the real design), flash chip/boot stage, SMPS
// mode pin, etc.
#include "boards/waveshare_core2350b.h"

// --- WIFI ---
// No RM2/CYW43439 module wired up to this breakout yet, so deliberately
// NOT declaring PICO_CYW43_SUPPORTED here: wifi_rtc_sync just won't build
// against this board target until there's real wifi hardware attached to
// GPIO38-41 to match. rtc_time_example (the only one that
// matters until then) doesn't need it.

#endif
