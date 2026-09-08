#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace vajra {

class Shader;

/// Immediate-mode 2D overlay. Everything is batched into one dynamic buffer
/// and issued as a single draw call per frame. The font is a 3x5 bitmap baked
/// into the binary, so there is no FreeType dependency and no font asset to
/// ship or fail to load.
class Hud {
public:
    void init();
    void shutdown();

    void begin(int screenWidth, int screenHeight);
    void rect(float x, float y, float w, float h, const glm::vec4& colour);
    void text(const std::string& s, float x, float y, float scale, const glm::vec4& colour);
    void bar(float x, float y, float w, float h, float fraction,
             const glm::vec4& fill, const glm::vec4& back);
    void crosshair(float cx, float cy, float gap, float length, const glm::vec4& colour);
    void end(Shader& shader);

    /// Width in pixels a string would occupy at the given scale.
    [[nodiscard]] static float textWidth(const std::string& s, float scale);

private:
    struct Vertex {
        glm::vec2 position;
        glm::vec4 colour;
    };

    std::vector<Vertex> m_vertices;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    size_t m_capacity = 0;
    int m_width = 0;
    int m_height = 0;
};

}  // namespace vajra
