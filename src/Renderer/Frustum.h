#pragma once
#include <array>
#include <glm/glm.hpp>

namespace vajra {

struct AABB;

/// View frustum extracted from a combined view-projection matrix, used to
/// reject geometry before it reaches the GPU. On a scene of a few hundred
/// boxes this typically halves the instance count for free.
class Frustum {
public:
    void extract(const glm::mat4& viewProjection);

    /// Conservative test: false only when the box is definitely outside.
    [[nodiscard]] bool intersects(const glm::vec3& centre, const glm::vec3& halfExtents) const;

private:
    // Left, right, bottom, top, near, far — each stored as xyz normal, w distance.
    std::array<glm::vec4, 6> m_planes{};
};

}  // namespace vajra
