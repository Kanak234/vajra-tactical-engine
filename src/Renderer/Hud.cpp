#include "Renderer/Hud.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/Shader.h"

namespace vajra {

namespace {

// 3x5 bitmap font. Each glyph is five rows; the low three bits of each byte
// are the pixels, most-significant bit on the left.
struct Glyph { unsigned char rows[5]; };

Glyph glyphFor(char c) {
    switch (c) {
        case 'A': return {{2,5,7,5,5}};
        case 'B': return {{6,5,6,5,6}};
        case 'C': return {{3,4,4,4,3}};
        case 'D': return {{6,5,5,5,6}};
        case 'E': return {{7,4,6,4,7}};
        case 'F': return {{7,4,6,4,4}};
        case 'G': return {{3,4,5,5,3}};
        case 'H': return {{5,5,7,5,5}};
        case 'I': return {{7,2,2,2,7}};
        case 'J': return {{1,1,1,5,2}};
        case 'K': return {{5,5,6,5,5}};
        case 'L': return {{4,4,4,4,7}};
        case 'M': return {{5,7,7,5,5}};
        case 'N': return {{5,7,5,5,5}};
        case 'O': return {{2,5,5,5,2}};
        case 'P': return {{6,5,6,4,4}};
        case 'Q': return {{2,5,5,6,3}};
        case 'R': return {{6,5,6,5,5}};
        case 'S': return {{3,4,2,1,6}};
        case 'T': return {{7,2,2,2,2}};
        case 'U': return {{5,5,5,5,7}};
        case 'V': return {{5,5,5,2,2}};
        case 'W': return {{5,5,7,7,5}};
        case 'X': return {{5,5,2,5,5}};
        case 'Y': return {{5,5,2,2,2}};
        case 'Z': return {{7,1,2,4,7}};
        case '0': return {{7,5,5,5,7}};
        case '1': return {{2,6,2,2,7}};
        case '2': return {{7,1,7,4,7}};
        case '3': return {{7,1,3,1,7}};
        case '4': return {{5,5,7,1,1}};
        case '5': return {{7,4,7,1,7}};
        case '6': return {{7,4,7,5,7}};
        case '7': return {{7,1,1,1,1}};
        case '8': return {{7,5,7,5,7}};
        case '9': return {{7,5,7,1,7}};
        case '.': return {{0,0,0,0,2}};
        case ',': return {{0,0,0,2,4}};
        case ':': return {{0,2,0,2,0}};
        case '-': return {{0,0,7,0,0}};
        case '+': return {{0,2,7,2,0}};
        case '/': return {{1,1,2,4,4}};
        case '%': return {{5,1,2,4,5}};
        case '!': return {{2,2,2,0,2}};
        case '?': return {{7,1,3,0,2}};
        case '(': return {{1,2,2,2,1}};
        case ')': return {{4,2,2,2,4}};
        case '[': return {{3,2,2,2,3}};
        case ']': return {{6,2,2,2,6}};
        case '<': return {{1,2,4,2,1}};
        case '>': return {{4,2,1,2,4}};
        case '=': return {{0,7,0,7,0}};
        case '*': return {{5,2,5,0,0}};
        case '_': return {{0,0,0,0,7}};
        case '#': return {{5,7,5,7,5}};
        case ' ':
        default:  return {{0,0,0,0,0}};
    }
}

char upcase(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c; }

constexpr float kGlyphW = 3.0f;
constexpr float kGlyphH = 5.0f;
constexpr float kAdvance = 4.0f;   // glyph width plus one pixel of spacing

}  // namespace

void Hud::init() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, colour)));
    glBindVertexArray(0);
}

void Hud::shutdown() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vbo = m_vao = 0;
}

void Hud::begin(int screenWidth, int screenHeight) {
    m_width  = screenWidth;
    m_height = screenHeight;
    m_vertices.clear();
}

void Hud::rect(float x, float y, float w, float h, const glm::vec4& colour) {
    if (colour.a <= 0.0f) return;
    const glm::vec2 a{x, y};
    const glm::vec2 b{x + w, y};
    const glm::vec2 c{x + w, y + h};
    const glm::vec2 d{x, y + h};
    m_vertices.push_back({a, colour});
    m_vertices.push_back({b, colour});
    m_vertices.push_back({c, colour});
    m_vertices.push_back({a, colour});
    m_vertices.push_back({c, colour});
    m_vertices.push_back({d, colour});
}

float Hud::textWidth(const std::string& s, float scale) {
    return static_cast<float>(s.size()) * kAdvance * scale;
}

void Hud::text(const std::string& s, float x, float y, float scale, const glm::vec4& colour) {
    float cursor = x;
    for (const char raw : s) {
        const char c = upcase(raw);
        const Glyph g = glyphFor(c);
        for (int row = 0; row < static_cast<int>(kGlyphH); ++row) {
            const unsigned char bits = g.rows[row];
            for (int col = 0; col < static_cast<int>(kGlyphW); ++col) {
                if (bits & (1u << (2 - col))) {
                    rect(cursor + static_cast<float>(col) * scale,
                         y + static_cast<float>(row) * scale,
                         scale, scale, colour);
                }
            }
        }
        cursor += kAdvance * scale;
    }
}

void Hud::bar(float x, float y, float w, float h, float fraction,
              const glm::vec4& fill, const glm::vec4& back) {
    fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
    rect(x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, glm::vec4{0.0f, 0.0f, 0.0f, 0.55f});
    rect(x, y, w, h, back);
    rect(x, y, w * fraction, h, fill);
}

void Hud::crosshair(float cx, float cy, float gap, float length, const glm::vec4& colour) {
    const float t = 1.0f;
    rect(cx - gap - length, cy - t * 0.5f, length, t, colour);
    rect(cx + gap,          cy - t * 0.5f, length, t, colour);
    rect(cx - t * 0.5f, cy - gap - length, t, length, colour);
    rect(cx - t * 0.5f, cy + gap,          t, length, colour);
}

void Hud::end(Shader& shader) {
    if (m_vertices.empty() || !shader.valid()) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    const size_t bytes = m_vertices.size() * sizeof(Vertex);
    if (m_vertices.size() > m_capacity) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes), m_vertices.data(),
                     GL_STREAM_DRAW);
        m_capacity = m_vertices.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), m_vertices.data());
    }

    shader.bind();
    shader.set("uProjection", glm::ortho(0.0f, static_cast<float>(m_width),
                                         static_cast<float>(m_height), 0.0f));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

}  // namespace vajra
