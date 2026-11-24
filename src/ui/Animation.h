#pragma once

#include <algorithm>
#include <cmath>

#include <imgui.h>

namespace diskscope::ui {

// Cubic ease-out: starts fast, settles gently
inline float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float p = 1.0f - t;
    return 1.0f - p * p * p;
}

// Cubic ease-in-out: smooth acceleration and deceleration
inline float EaseInOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    const float p = -2.0f * t + 2.0f;
    return 1.0f - (p * p * p) * 0.5f;
}

// Back ease-out: slight overshoot for snappy arrival
inline float EaseOutBack(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float p = t - 1.0f;
    return 1.0f + c3 * p * p * p + c1 * p * p;
}

// Frame-rate-independent smoothly animated scalar value
class AnimatedFloat {
public:
    explicit AnimatedFloat(float initial = 0.0f)
        : value_(initial), target_(initial) {}

    void set(float target, bool immediate = false) {
        target_ = target;
        if (immediate) {
            value_ = target;
        }
    }

    float update(float dt, float speed = 12.0f) {
        if (dt <= 0.0f) {
            return value_;
        }
        const float factor = 1.0f - std::exp(-speed * dt);
        value_ += (target_ - value_) * factor;
        if (std::abs(target_ - value_) < 0.0001f) {
            value_ = target_;
        }
        return value_;
    }

    float get() const { return value_; }
    float target() const { return target_; }

private:
    float value_ = 0.0f;
    float target_ = 0.0f;
};

// Clock for measuring animation elapsed time, driven by ImGui time
struct FrameTimer {
    float startTime = 0.0f;

    FrameTimer() {
        restart();
    }

    void restart() {
        startTime = static_cast<float>(ImGui::GetTime());
    }

    float elapsed() const {
        const float now = static_cast<float>(ImGui::GetTime());
        return (now >= startTime) ? (now - startTime) : 0.0f;
    }

    bool finished(float duration) const {
        return elapsed() >= duration;
    }
};

}  // namespace diskscope::ui
