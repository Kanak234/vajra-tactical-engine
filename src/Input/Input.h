#pragma once
#include <SDL2/SDL.h>
#include <array>
#include <glm/glm.hpp>

namespace vajra {

/// Frame-coherent input snapshot. Systems ask "is it down / did it just go
/// down", so gameplay code never touches SDL event structs directly.
class Input {
public:
    void beginFrame() {
        m_prevKeys  = m_keys;
        m_prevMouse = m_mouse;
        m_mouseDelta = {0.0f, 0.0f};
        m_scroll = 0.0f;
    }

    void handleEvent(const SDL_Event& e) {
        switch (e.type) {
            case SDL_KEYDOWN:
                if (!e.key.repeat) m_keys[e.key.keysym.scancode] = true;
                break;
            case SDL_KEYUP:
                m_keys[e.key.keysym.scancode] = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button < kMouseButtons) m_mouse[e.button.button] = true;
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button < kMouseButtons) m_mouse[e.button.button] = false;
                break;
            case SDL_MOUSEMOTION:
                m_mouseDelta.x += static_cast<float>(e.motion.xrel);
                m_mouseDelta.y += static_cast<float>(e.motion.yrel);
                break;
            case SDL_MOUSEWHEEL:
                m_scroll += static_cast<float>(e.wheel.y);
                break;
            default: break;
        }
    }

    [[nodiscard]] bool key(SDL_Scancode k) const { return m_keys[k]; }
    [[nodiscard]] bool keyPressed(SDL_Scancode k) const {
        return m_keys[k] && !m_prevKeys[k];
    }
    [[nodiscard]] bool mouse(int b) const { return m_mouse[b]; }
    [[nodiscard]] bool mousePressed(int b) const {
        return m_mouse[b] && !m_prevMouse[b];
    }
    [[nodiscard]] glm::vec2 mouseDelta() const { return m_mouseDelta; }
    [[nodiscard]] float scroll() const { return m_scroll; }

private:
    static constexpr int kMouseButtons = 8;
    std::array<bool, SDL_NUM_SCANCODES> m_keys{};
    std::array<bool, SDL_NUM_SCANCODES> m_prevKeys{};
    std::array<bool, kMouseButtons>     m_mouse{};
    std::array<bool, kMouseButtons>     m_prevMouse{};
    glm::vec2 m_mouseDelta{0.0f};
    float     m_scroll = 0.0f;
};

}  // namespace vajra
