#pragma once
#include <chrono>

namespace vajra {

/// Fixed-step accumulator. Physics/AI tick at a constant rate while the
/// renderer runs free; alpha() lets you interpolate transforms when drawing.
class StepClock {
public:
    explicit StepClock(double fixedStep = 1.0 / 60.0) : m_step(fixedStep) {
        m_last = Clock::now();
    }

    /// Call once per frame. Returns wall-clock delta in seconds.
    double beginFrame() {
        const auto now = Clock::now();
        double dt = std::chrono::duration<double>(now - m_last).count();
        m_last = now;
        if (dt > 0.25) dt = 0.25;   // avoid spiral-of-death after a stall
        m_accumulator += dt;
        m_frameDelta = dt;
        return dt;
    }

    /// Drain one fixed step: while (clock.consumeStep()) { ...simulate... }
    bool consumeStep() {
        if (m_accumulator < m_step) return false;
        m_accumulator -= m_step;
        return true;
    }

    [[nodiscard]] double fixedStep()  const { return m_step; }
    [[nodiscard]] double frameDelta() const { return m_frameDelta; }
    [[nodiscard]] float  alpha()      const {
        return static_cast<float>(m_accumulator / m_step);
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_last;
    double m_step;
    double m_accumulator = 0.0;
    double m_frameDelta  = 0.0;
};

}  // namespace vajra
