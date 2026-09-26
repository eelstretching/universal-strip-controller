#include "light_pattern.hpp"

#include "pico/rand.h"

RGB SolidPattern::colorAt(uint32_t) { return color_; }

RGB BlinkPattern::colorAt(uint32_t nowMs) {
    uint32_t period = onMs_ + offMs_;
    if (period == 0) {
        return color_;
    }
    return (nowMs % period) < onMs_ ? color_ : RGB(0, 0, 0);
}

RGB FlickerPattern::colorAt(uint32_t nowMs) {
    // Signed difference so this keeps working across the ~49-day wrap of a
    // 32-bit millisecond counter.
    if (!started_ || static_cast<int32_t>(nowMs - nextChangeMs_) >= 0) {
        uint8_t level = static_cast<uint8_t>(get_rand_32());
        current_ = color_.scale8(level);
        nextChangeMs_ = nowMs + intervalMs_;
        started_ = true;
    }
    return current_;
}
