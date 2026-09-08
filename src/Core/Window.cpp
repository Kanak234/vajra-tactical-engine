#include "Core/Window.h"

#include <glad/glad.h>
#include <stdexcept>

#include "Core/Log.h"

namespace vajra {

namespace {
void APIENTRY glDebugCallback(GLenum, GLenum type, GLuint, GLenum severity,
                              GLsizei, const GLchar* message, const void*) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    if (type == GL_DEBUG_TYPE_ERROR) VJ_ERROR("GL: %s", message);
    else                            VJ_WARN ("GL: %s", message);
}
}  // namespace

Window::Window(const WindowSpec& spec)
    : m_width(spec.width), m_height(spec.height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, spec.glMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, spec.glMinor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

    m_window = SDL_CreateWindow(
        spec.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        spec.width, spec.height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
        throw std::runtime_error(std::string("GL context failed: ") + SDL_GetError());

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
        throw std::runtime_error("Failed to load OpenGL function pointers via GLAD");

    setVSync(spec.vsync);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, nullptr);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // Deliberately NOT enabling GL_FRAMEBUFFER_SRGB: the composite pass does
    // its own linear-to-sRGB conversion after tonemapping. Enabling both would
    // gamma-correct twice and wash the image out.

    VJ_INFO("GPU     : %s", glGetString(GL_RENDERER));
    VJ_INFO("Driver  : %s", glGetString(GL_VERSION));
}

Window::~Window() {
    if (m_context) SDL_GL_DeleteContext(m_context);
    if (m_window)  SDL_DestroyWindow(m_window);
    SDL_Quit();
}

}  // namespace vajra
