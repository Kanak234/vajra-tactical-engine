#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace vajra {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
};

/// Per-instance data. Everything the shader needs to draw one copy of a mesh
/// lives here, so an entire scene of identical geometry collapses into a
/// single draw call.
struct InstanceData {
    glm::mat4 model{1.0f};
    glm::vec4 albedoRoughness{0.8f, 0.8f, 0.8f, 0.8f};   // rgb albedo, a roughness
    glm::vec4 params{0.0f};                              // x emissive, y..w spare
};

/// GPU mesh with a built-in instance buffer. Every mesh is drawn instanced —
/// even one-offs upload a single instance — so there is exactly one vertex
/// layout and one shader variant to maintain instead of two of each.
class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    /// Streams instance data to the GPU. Call once per frame per mesh.
    void uploadInstances(const std::vector<InstanceData>& instances);
    /// Draws whatever was last uploaded.
    void drawInstances() const;
    /// Convenience for one-off geometry (ground, backdrop).
    void drawSingle(const InstanceData& instance);

    [[nodiscard]] GLsizei indexCount() const { return m_indexCount; }
    [[nodiscard]] size_t  instanceCount() const { return m_instanceCount; }

    static Mesh makeCube(float size = 1.0f);
    static Mesh makePlane(float size = 50.0f, int subdivisions = 1);
    /// Unit quad on the XY plane, used for billboard particles.
    static Mesh makeQuad(float size = 1.0f);
    /// Procedural mountain ring — the Himalayan backdrop, generated not authored.
    static Mesh makeMountainRing(float innerRadius, float outerRadius,
                                 int segments, float minHeight, float maxHeight,
                                 unsigned seed);

private:
    void release();
    void setupInstanceAttributes();

    GLuint  m_vao = 0, m_vbo = 0, m_ebo = 0, m_instanceVbo = 0;
    GLsizei m_indexCount = 0;
    size_t  m_instanceCount = 0;
    size_t  m_instanceCapacity = 0;
    std::vector<InstanceData> m_scratch;
};

}  // namespace vajra
