#include "status_lights.hpp"

#include "pico/time.h"

namespace {

const RGB kBlack(0, 0, 0);

} // namespace

// SK6812MINI uses WS2812 timing and GRB order, which is also Strip's default
// color order.
StatusLights::StatusLights(uint pin, uint count, uint8_t brightness)
    : count_(count), strip_(pin, count, WS2812), renderer_(brightness) {
    patterns_.reserve(count);
    for (uint i = 0; i < count; i++) {
        patterns_.push_back(std::make_unique<SolidPattern>(kBlack));
    }
    strip_.fill(kBlack);
    renderer_.add(&strip_);
}

void StatusLights::begin() {
    if (begun_) {
        return;
    }
    renderer_.setup();
    begun_ = true;
    needsRender_ = true;
}

void StatusLights::setBrightness(uint8_t brightness) {
    renderer_.setBrightness(brightness);
    needsRender_ = true;
}

void StatusLights::setPattern(uint index,
                              std::unique_ptr<LightPattern> pattern) {
    if (index >= count_ || !pattern) {
        return;
    }
    patterns_[index] = std::move(pattern);
}

void StatusLights::set(uint index, const RGB &color) {
    setPattern(index, std::make_unique<SolidPattern>(color));
}

void StatusLights::off(uint index) { set(index, kBlack); }

void StatusLights::allOff() {
    for (uint i = 0; i < count_; i++) {
        off(i);
    }
}

void StatusLights::update() {
    uint32_t nowMs = to_ms_since_boot(get_absolute_time());
    for (uint i = 0; i < count_; i++) {
        RGB color = patterns_[i]->colorAt(nowMs);
        if (!(color == strip_.get(i))) {
            strip_.putPixel(color, i);
            needsRender_ = true;
        }
    }

    // Only send when something changed: render() waits out the previous
    // frame's DMA and latch time, so there's no point paying that for an
    // identical frame.
    if (begun_ && needsRender_) {
        renderer_.render();
        needsRender_ = false;
    }
}
