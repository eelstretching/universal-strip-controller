// What a single status light shows over time.
//
// A StatusLights instance gives each of its lights one LightPattern and asks
// it "what color should you be right now?" on every update(). New behaviors
// (a WiFi-activity flicker, an error blink, a breathing "idle" glow...) are
// new LightPattern subclasses; StatusLights itself doesn't change.
#ifndef USC_LIGHT_PATTERN_HPP
#define USC_LIGHT_PATTERN_HPP

#include <cstdint>

#include "color.h"

class LightPattern {
public:
    virtual ~LightPattern() = default;

    // The color this light should show at nowMs (milliseconds since boot).
    // Called once per StatusLights::update(), with nowMs never going
    // backwards between calls.
    virtual RGB colorAt(uint32_t nowMs) = 0;
};

// A steady color. Also how a light is turned off (SolidPattern(black)).
class SolidPattern : public LightPattern {
public:
    explicit SolidPattern(const RGB &color) : color_(color) {}

    RGB colorAt(uint32_t nowMs) override;

private:
    RGB color_;
};

// On for onMs, off for offMs, repeating. The phase is taken from the time
// since boot rather than from when the pattern was set, so every light
// blinking at the same rate blinks in step.
class BlinkPattern : public LightPattern {
public:
    BlinkPattern(const RGB &color, uint32_t onMs, uint32_t offMs)
        : color_(color), onMs_(onMs), offMs_(offMs) {}

    RGB colorAt(uint32_t nowMs) override;

private:
    RGB color_;
    uint32_t onMs_;
    uint32_t offMs_;
};

// Jumps to a new random brightness of one color every intervalMs, like
// a network-activity LED while data is moving.
class FlickerPattern : public LightPattern {
public:
    explicit FlickerPattern(const RGB &color, uint32_t intervalMs = 40)
        : color_(color), intervalMs_(intervalMs) {}

    RGB colorAt(uint32_t nowMs) override;

private:
    RGB color_;
    uint32_t intervalMs_;
    RGB current_{0, 0, 0};
    uint32_t nextChangeMs_ = 0;
    bool started_ = false;
};

#endif // USC_LIGHT_PATTERN_HPP
