#include "Renderer/Mesh.h"

#include <cmath>

namespace vajra {

namespace {
/// Deterministic hash noise — no dependency, same terrain every run.
float hashNoise(unsigned x, unsigned seed) {
    unsigned h = x * 374761393u + seed * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return static_cast<float>(h & 0xFFFFFF) / 16777215.0f;
}
}  // namespace

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_indexCount(static_cast<GLsizei>(indices.size())) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glGenBuffers(1, &m_instanceVbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, uv)));

    setupInstanceAttributes();
    glBindVertexArray(0);
}

void Mesh::setupInstanceAttributes() {
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);

    // A mat4 occupies four consecutive vec4 attribute slots.
    for (int i = 0; i < 4; ++i) {
        const GLuint location = 3 + static_cast<GLuint>(i);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              reinterpret_cast<void*>(offsetof(InstanceData, model) +
                                                      sizeof(glm::vec4) * static_cast<size_t>(i)));
        glVertexAttribDivisor(location, 1);
    }

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, albedoRoughness)));
    glVertexAttribDivisor(7, 1);

    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, params)));
    glVertexAttribDivisor(8, 1);
}

void Mesh::uploadInstances(const std::vector<InstanceData>& instances) {
    m_instanceCount = instances.size();
    if (m_instanceCount == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    const GLsizeiptr bytes = static_cast<GLsizeiptr>(m_instanceCount * sizeof(InstanceData));

    if (m_instanceCount > m_instanceCapacity) {
        glBufferData(GL_ARRAY_BUFFER, bytes, instances.data(), GL_STREAM_DRAW);
        m_instanceCapacity = m_instanceCount;
    } else {
        // Orphan the buffer so the driver never stalls waiting on last frame.
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(m_instanceCapacity * sizeof(InstanceData)),
                     nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, instances.data());
    }
}

void Mesh::drawInstances() const {
    if (m_instanceCount == 0 || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(m_instanceCount));
    glBindVertexArray(0);
}

void Mesh::drawSingle(const InstanceData& instance) {
    m_scratch.clear();
    m_scratch.push_back(instance);
    uploadInstances(m_scratch);
    drawInstances();
}

void Mesh::release() {
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = m_instanceVbo = 0;
    m_indexCount = 0;
    m_instanceCount = m_instanceCapacity = 0;
}

Mesh::~Mesh() { release(); }

Mesh::Mesh(Mesh&& o) noexcept
    : m_vao(o.m_vao), m_vbo(o.m_vbo), m_ebo(o.m_ebo), m_instanceVbo(o.m_instanceVbo),
      m_indexCount(o.m_indexCount), m_instanceCount(o.m_instanceCount),
      m_instanceCapacity(o.m_instanceCapacity), m_scratch(std::move(o.m_scratch)) {
    o.m_vao = o.m_vbo = o.m_ebo = o.m_instanceVbo = 0;
    o.m_indexCount = 0;
    o.m_instanceCount = o.m_instanceCapacity = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        release();
        m_vao = o.m_vao; m_vbo = o.m_vbo; m_ebo = o.m_ebo; m_instanceVbo = o.m_instanceVbo;
        m_indexCount = o.m_indexCount;
        m_instanceCount = o.m_instanceCount;
        m_instanceCapacity = o.m_instanceCapacity;
        m_scratch = std::move(o.m_scratch);
        o.m_vao = o.m_vbo = o.m_ebo = o.m_instanceVbo = 0;
        o.m_indexCount = 0;
        o.m_instanceCount = o.m_instanceCapacity = 0;
    }
    return *this;
}

Mesh Mesh::makeCube(float size) {
    const float h = size * 0.5f;
    const glm::vec3 n[6] = {{0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
    const glm::vec3 faceVerts[6][4] = {
        {{-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}},
        {{ h,-h,-h},{-h,-h,-h},{-h, h,-h},{ h, h,-h}},
        {{-h,-h,-h},{-h,-h, h},{-h, h, h},{-h, h,-h}},
        {{ h,-h, h},{ h,-h,-h},{ h, h,-h},{ h, h, h}},
        {{-h, h, h},{ h, h, h},{ h, h,-h},{-h, h,-h}},
        {{-h,-h,-h},{ h,-h,-h},{ h,-h, h},{-h,-h, h}},
    };
    const glm::vec2 uvs[4] = {{0,0},{1,0},{1,1},{0,1}};

    std::vector<Vertex>   verts;
    std::vector<uint32_t> idx;
    verts.reserve(24);
    idx.reserve(36);
    for (int f = 0; f < 6; ++f) {
        const auto base = static_cast<uint32_t>(verts.size());
        for (int v = 0; v < 4; ++v) verts.push_back({faceVerts[f][v], n[f], uvs[v]});
        idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return Mesh{verts, idx};
}

Mesh Mesh::makeQuad(float size) {
    const float h = size * 0.5f;
    std::vector<Vertex> verts = {
        {{-h,-h,0},{0,0,1},{0,0}}, {{ h,-h,0},{0,0,1},{1,0}},
        {{ h, h,0},{0,0,1},{1,1}}, {{-h, h,0},{0,0,1},{0,1}}};
    std::vector<uint32_t> idx = {0,1,2, 0,2,3};
    return Mesh{verts, idx};
}

Mesh Mesh::makePlane(float size, int subdivisions) {
    const int   n    = subdivisions < 1 ? 1 : subdivisions;
    const float step = size / static_cast<float>(n);
    const float half = size * 0.5f;

    std::vector<Vertex>   verts;
    std::vector<uint32_t> idx;
    verts.reserve(static_cast<size_t>((n + 1) * (n + 1)));

    for (int z = 0; z <= n; ++z) {
        for (int x = 0; x <= n; ++x) {
            const float px = -half + static_cast<float>(x) * step;
            const float pz = -half + static_cast<float>(z) * step;
            verts.push_back({{px, 0.0f, pz}, {0.0f, 1.0f, 0.0f},
                             {static_cast<float>(x), static_cast<float>(z)}});
        }
    }
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const auto row = static_cast<uint32_t>(n + 1);
            const auto tl  = static_cast<uint32_t>(z) * row + static_cast<uint32_t>(x);
            const uint32_t tr = tl + 1, bl = tl + row, br = bl + 1;
            idx.insert(idx.end(), {tl, bl, br, tl, br, tr});
        }
    }
    return Mesh{verts, idx};
}

Mesh Mesh::makeMountainRing(float innerRadius, float outerRadius, int segments,
                            float minHeight, float maxHeight, unsigned seed) {
    std::vector<Vertex>   verts;
    std::vector<uint32_t> idx;

    const int n = segments < 8 ? 8 : segments;

    // Two concentric rings: the outer sits on the horizon, the inner is lifted
    // to a jagged ridgeline. Triangles between them read as distant peaks.
    for (int i = 0; i <= n; ++i) {
        const float t = static_cast<float>(i % n) / static_cast<float>(n);
        const float angle = t * 6.2831853f;
        const float cs = std::cos(angle);
        const float sn = std::sin(angle);

        // Layered noise gives big peaks with smaller ones between them.
        const float coarse = hashNoise(static_cast<unsigned>(i % n) / 4u, seed);
        const float fine   = hashNoise(static_cast<unsigned>(i % n), seed + 91u);
        const float height = minHeight + (maxHeight - minHeight) * (coarse * 0.72f + fine * 0.28f);

        verts.push_back({{cs * outerRadius, 0.0f, sn * outerRadius}, {0, 1, 0}, {t, 0.0f}});
        verts.push_back({{cs * innerRadius, height, sn * innerRadius}, {0, 1, 0}, {t, 1.0f}});
    }

    for (int i = 0; i < n; ++i) {
        const auto a = static_cast<uint32_t>(i * 2);
        const uint32_t b = a + 1, c = a + 2, d = a + 3;
        idx.insert(idx.end(), {a, c, b, b, c, d});
    }

    // Face normals so the peaks catch light as distinct facets.
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        Vertex& v0 = verts[idx[i]];
        Vertex& v1 = verts[idx[i + 1]];
        Vertex& v2 = verts[idx[i + 2]];
        const glm::vec3 face = glm::normalize(glm::cross(v1.position - v0.position,
                                                         v2.position - v0.position));
        v0.normal = v1.normal = v2.normal = face;
    }

    return Mesh{verts, idx};
}

}  // namespace vajra
