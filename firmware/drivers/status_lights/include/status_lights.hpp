// The board's chain of RGB status lights (SK6812MINI, on Status-LEDs.kicad_sch).
//
// Callers say what each light should be doing -- a steady color, a blink, a
// flicker -- and call update() regularly; they never see pixels, PIO state
// machines or DMA. Underneath, it's a PicoLEDs Strip rendered by a PicoLEDs
// Renderer.
#ifndef USC_STATUS_LIGHTS_HPP
#define USC_STATUS_LIGHTS_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "Renderer.h"
#include "Strip.h"
#include "color.h"
#include "light_pattern.hpp"

class StatusLights {
public:
    // Status lights sit right next to whoever is looking at the board, so
    // they default to fairly dim.
    static constexpr uint8_t kDefaultBrightness = 32;

    // count lights daisy-chained on the data line at pin. All lights start
    // off. Doesn't touch any hardware until begin().
    StatusLights(uint pin, uint count,
                 uint8_t brightness = kDefaultBrightness);

    // The Renderer holds a pointer to our Strip, so an instance can't be
    // copied or moved.
    StatusLights(const StatusLights &) = delete;
    StatusLights &operator=(const StatusLights &) = delete;

    // Claims a PIO state machine and a DMA channel for the lights.
    //
    // If the firmware uses WiFi, call this *after* cyw43_arch_init(): the
    // cyw43 driver needs a specific PIO state machine, and this claims the
    // first free one it finds, so claiming first can take the one cyw43
    // needs.
    void begin();

    uint count() const { return count_; }

    // Overall brightness for every light, 0-255.
    void setBrightness(uint8_t brightness);

    // Gives light index a new pattern, replacing (and destroying) whatever
    // it was showing. Out-of-range indexes are ignored.
    void setPattern(uint index, std::unique_ptr<LightPattern> pattern);

    // Shorthands for the common cases.
    void set(uint index, const RGB &color);
    void off(uint index);
    void allOff();

    // Advances every light's pattern to the current time and sends the
    // result to the lights if anything changed. Call this regularly from
    // the main loop -- every 10-20 ms is plenty for smooth patterns.
    //
    // Not safe to call concurrently with setPattern() and friends: drive
    // all of them from one context (e.g. the main loop), not from an IRQ
    // or WiFi callback.
    void update();

private:
    uint count_;
    Strip strip_;
    Renderer renderer_;
    std::vector<std::unique_ptr<LightPattern>> patterns_;
    bool begun_ = false;
    bool needsRender_ = true;
};

#endif // USC_STATUS_LIGHTS_HPP
