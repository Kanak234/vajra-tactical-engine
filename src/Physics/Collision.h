#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace vajra {

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    [[nodiscard]] glm::vec3 centre() const { return (min + max) * 0.5f; }
    [[nodiscard]] glm::vec3 extents() const { return (max - min) * 0.5f; }
    /// Strict inequality on purpose: two boxes that merely *touch* are not
    /// overlapping. With <= a character standing exactly on the ground counts
    /// as intersecting it, which freezes movement and marks every navigation
    /// cell as blocked.
    [[nodiscard]] bool overlaps(const AABB& o) const {
        return min.x < o.max.x && max.x > o.min.x &&
               min.y < o.max.y && max.y > o.min.y &&
               min.z < o.max.z && max.z > o.min.z;
    }
    static AABB fromCentre(const glm::vec3& c, const glm::vec3& halfExtents) {
        return AABB{c - halfExtents, c + halfExtents};
    }
};

struct RayHit {
    bool      hit      = false;
    float     distance = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    int       index    = -1;   // index into the world's box list, -1 if none
};

/// Standalone ray/AABB test, used for shooting at dynamic targets (guards)
/// that are not part of the static world.
bool rayAabb(const glm::vec3& origin, const glm::vec3& direction,
             const AABB& box, float maxDistance, float& outDistance);

/// Static AABB collision world. Deliberately not Bullet: for a level built from
/// boxes and crates this is faster, has zero dependencies, and is trivial to
/// debug. Swap in Bullet later only if you add ragdolls or vehicles.
class PhysicsWorld {
public:
    void clear() { m_boxes.clear(); }
    int  addBox(const AABB& box) {
        m_boxes.push_back(box);
        return static_cast<int>(m_boxes.size()) - 1;
    }
    [[nodiscard]] const std::vector<AABB>& boxes() const { return m_boxes; }

    /// Slab-method raycast against every box. Returns the nearest hit.
    [[nodiscard]] RayHit raycast(const glm::vec3& origin, const glm::vec3& direction,
                                 float maxDistance) const;

    /// True when nothing blocks the segment. Used by AI vision.
    [[nodiscard]] bool lineOfSight(const glm::vec3& from, const glm::vec3& to) const;

    /// Moves an axis-aligned character box by `delta`, resolving one axis at a
    /// time. Axis-separated resolution is what stops you sticking on corners.
    [[nodiscard]] glm::vec3 moveCharacter(const glm::vec3& position,
                                          const glm::vec3& halfExtents,
                                          const glm::vec3& delta,
                                          bool& outGrounded) const;

private:
    [[nodiscard]] bool overlapsAny(const AABB& box) const;
    [[nodiscard]] float resolveAxis(const glm::vec3& position, const glm::vec3& halfExtents,
                                    int axis, float amount) const;

    std::vector<AABB> m_boxes;
};

}  // namespace vajra
