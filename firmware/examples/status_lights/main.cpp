// Demonstrates the StatusLights driver: each light gets a different pattern,
// so one look at the board confirms the whole chain is wired and addressed
// in order.
#include <memory>

#include "pico/stdlib.h"

#include "status_lights.hpp"

#if !defined(PICO_DEFAULT_WS2812_PIN) || !defined(USC_STATUS_LIGHT_COUNT)
#error "board header doesn't define the status light pin/count"
#endif

int main() {
    stdio_init_all();

    StatusLights lights(PICO_DEFAULT_WS2812_PIN, USC_STATUS_LIGHT_COUNT);
    // No WiFi in this example, so there's no cyw43 PIO claim to wait for.
    lights.begin();

    const RGB red(255, 0, 0);
    const RGB green(0, 255, 0);
    const RGB blue(0, 0, 255);
    const RGB amber(255, 120, 0);

    lights.set(0, green);
    lights.setPattern(1, std::make_unique<BlinkPattern>(amber, 500, 500));
    lights.setPattern(2, std::make_unique<FlickerPattern>(blue));
    lights.setPattern(3, std::make_unique<BlinkPattern>(red, 100, 900));
    // Any further lights show a solid color, so the far end of the chain is
    // visible too.
    for (uint i = 4; i < lights.count(); i++) {
        lights.set(i, RGB(255, 255, 255));
    }

    while (true) {
        lights.update();
        sleep_ms(10);
    }
}
