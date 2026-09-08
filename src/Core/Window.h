#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <string>

namespace vajra {

struct WindowSpec {
    std::string title  = "Vajra";
    int         width  = 1600;
    int         height = 900;
    bool        vsync  = true;
    int         glMajor = 4;
    int         glMinor = 6;
};

/// Owns the SDL window + OpenGL core-profile context. RAII: the context and
/// window die with the object, so there is no shutdown() to forget to call.
class Window {
public:
    explicit Window(const WindowSpec& spec);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window&&)      = delete;

    void swap() const { SDL_GL_SwapWindow(m_window); }
    void setVSync(bool on) const { SDL_GL_SetSwapInterval(on ? 1 : 0); }

    [[nodiscard]] SDL_Window* handle() const { return m_window; }
    [[nodiscard]] int  width()  const { return m_width; }
    [[nodiscard]] int  height() const { return m_height; }
    [[nodiscard]] float aspect() const {
        return m_height ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
    }

    /// Called by Application when SDL reports a resize.
    void onResize(int w, int h) { m_width = w; m_height = h; }

    /// Lock the cursor to the window for FPS-style mouselook.
    static void captureMouse(bool capture) {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    }

private:
    SDL_Window*   m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    int m_width  = 0;
    int m_height = 0;
};

}  // namespace vajra
