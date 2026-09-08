#pragma once
#include <array>
#include <chrono>
#include <cstddef>

namespace vajra {

/// Named CPU timing slots with a rolling average. Cheap enough to leave on in
/// release: one steady_clock read per scope.
class Profiler {
public:
    enum Slot { Frame, Simulate, Ai, Shadow, Prepass, Ssao, Scene, Post, Hud, SlotCount };

    static const char* name(Slot slot) {
        switch (slot) {
            case Frame:    return "FRAME";
            case Simulate: return "SIM";
            case Ai:       return "AI";
            case Shadow:   return "SHADOW";
            case Prepass:  return "PREPASS";
            case Ssao:     return "SSAO";
            case Scene:    return "SCENE";
            case Post:     return "POST";
            case Hud:      return "HUD";
            default:       return "?";
        }
    }

    void begin(Slot slot) { m_start[slot] = Clock::now(); }
    void end(Slot slot) {
        const double ms = std::chrono::duration<double, std::milli>(
                              Clock::now() - m_start[slot]).count();
        // Exponential moving average keeps the readout stable enough to read.
        m_average[slot] = m_average[slot] * 0.92 + ms * 0.08;
    }

    [[nodiscard]] double milliseconds(Slot slot) const { return m_average[slot]; }

private:
    using Clock = std::chrono::steady_clock;
    std::array<Clock::time_point, SlotCount> m_start{};
    std::array<double, SlotCount> m_average{};
};

/// RAII scope timer.
class ScopedTimer {
public:
    ScopedTimer(Profiler& profiler, Profiler::Slot slot)
        : m_profiler(profiler), m_slot(slot) { m_profiler.begin(slot); }
    ~ScopedTimer() { m_profiler.end(m_slot); }
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    Profiler& m_profiler;
    Profiler::Slot m_slot;
};

}  // namespace vajra
