#include "Renderer/Frustum.h"

#include <cmath>

namespace vajra {

void Frustum::extract(const glm::mat4& m) {
    // Gribb-Hartmann: each plane is a sum or difference of matrix rows.
    m_planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0],
                            m[2][3] + m[2][0], m[3][3] + m[3][0]);   // left
    m_planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0],
                            m[2][3] - m[2][0], m[3][3] - m[3][0]);   // right
    m_planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1],
                            m[2][3] + m[2][1], m[3][3] + m[3][1]);   // bottom
    m_planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1],
                            m[2][3] - m[2][1], m[3][3] - m[3][1]);   // top
    m_planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2],
                            m[2][3] + m[2][2], m[3][3] + m[3][2]);   // near
    m_planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2],
                            m[2][3] - m[2][2], m[3][3] - m[3][2]);   // far

    for (glm::vec4& plane : m_planes) {
        const float length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        if (length > 0.0f) plane /= length;
    }
}

bool Frustum::intersects(const glm::vec3& centre, const glm::vec3& halfExtents) const {
    for (const glm::vec4& plane : m_planes) {
        // Projected radius of the box onto the plane normal.
        const float radius = halfExtents.x * std::abs(plane.x) +
                             halfExtents.y * std::abs(plane.y) +
                             halfExtents.z * std::abs(plane.z);
        const float distance = plane.x * centre.x + plane.y * centre.y +
                               plane.z * centre.z + plane.w;
        if (distance < -radius) return false;
    }
    return true;
}

}  // namespace vajra
