#include "Physics/Collision.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace vajra {

bool rayAabb(const glm::vec3& origin, const glm::vec3& direction,
             const AABB& box, float maxDistance, float& outDistance) {
    const glm::vec3 dir = glm::normalize(direction);
    float tmin = 0.0f;
    float tmax = maxDistance;

    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(dir[axis]) < 1e-6f) {
            if (origin[axis] < box.min[axis] || origin[axis] > box.max[axis]) return false;
            continue;
        }
        const float inv = 1.0f / dir[axis];
        float t1 = (box.min[axis] - origin[axis]) * inv;
        float t2 = (box.max[axis] - origin[axis]) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }
    outDistance = tmin;
    return true;
}

RayHit PhysicsWorld::raycast(const glm::vec3& origin, const glm::vec3& direction,
                             float maxDistance) const {
    RayHit best;
    best.distance = maxDistance;

    const glm::vec3 dir = glm::normalize(direction);
    const glm::vec3 invDir{
        dir.x != 0.0f ? 1.0f / dir.x : std::numeric_limits<float>::infinity(),
        dir.y != 0.0f ? 1.0f / dir.y : std::numeric_limits<float>::infinity(),
        dir.z != 0.0f ? 1.0f / dir.z : std::numeric_limits<float>::infinity()};

    for (size_t i = 0; i < m_boxes.size(); ++i) {
        const AABB& b = m_boxes[i];

        float tmin = 0.0f;
        float tmax = best.distance;
        int   hitAxis = -1;
        float hitSign = 1.0f;

        bool missed = false;
        for (int axis = 0; axis < 3 && !missed; ++axis) {
            const float o  = origin[axis];
            const float id = invDir[axis];

            if (std::isinf(id)) {
                if (o < b.min[axis] || o > b.max[axis]) missed = true;
                continue;
            }

            float t1 = (b.min[axis] - o) * id;
            float t2 = (b.max[axis] - o) * id;
            float sign = -1.0f;
            if (t1 > t2) { std::swap(t1, t2); sign = 1.0f; }

            if (t1 > tmin) { tmin = t1; hitAxis = axis; hitSign = sign; }
            tmax = std::min(tmax, t2);
            if (tmin > tmax) missed = true;
        }

        if (missed || hitAxis < 0 || tmin < 0.0f || tmin >= best.distance) continue;

        best.hit      = true;
        best.distance = tmin;
        best.point    = origin + dir * tmin;
        best.normal   = glm::vec3{0.0f};
        best.normal[hitAxis] = hitSign;
        best.index    = static_cast<int>(i);
    }

    return best;
}

bool PhysicsWorld::lineOfSight(const glm::vec3& from, const glm::vec3& to) const {
    const glm::vec3 delta = to - from;
    const float dist = glm::length(delta);
    if (dist < 0.0001f) return true;
    const RayHit hit = raycast(from, delta / dist, dist);
    return !hit.hit;
}

bool PhysicsWorld::overlapsAny(const AABB& box) const {
    for (const AABB& b : m_boxes)
        if (box.overlaps(b)) return true;
    return false;
}

float PhysicsWorld::resolveAxis(const glm::vec3& position, const glm::vec3& halfExtents,
                                int axis, float amount) const {
    if (amount == 0.0f) return 0.0f;

    glm::vec3 target = position;
    target[axis] += amount;
    AABB moved = AABB::fromCentre(target, halfExtents);
    if (!overlapsAny(moved)) return amount;

    // Blocked: binary-search the largest safe step. Eight iterations puts us
    // within ~0.4% of the surface, which is below any visible gap.
    float lo = 0.0f;
    float hi = amount;
    for (int i = 0; i < 8; ++i) {
        const float mid = (lo + hi) * 0.5f;
        glm::vec3 probe = position;
        probe[axis] += mid;
        if (overlapsAny(AABB::fromCentre(probe, halfExtents))) hi = mid;
        else                                                    lo = mid;
    }
    return lo;
}

glm::vec3 PhysicsWorld::moveCharacter(const glm::vec3& position,
                                      const glm::vec3& halfExtents,
                                      const glm::vec3& delta,
                                      bool& outGrounded) const {
    // Substep long moves. Resolution only tests the destination, so a single
    // large step can pass clean through a thin wall — the classic tunnelling
    // bug. Capping each substep below the character's own radius makes that
    // impossible regardless of speed or frame hitches.
    const float travel = glm::length(delta);
    const float maxStep = std::min({halfExtents.x, halfExtents.y, halfExtents.z}) * 0.8f;
    const int substeps = std::max(1, static_cast<int>(std::ceil(travel / std::max(maxStep, 0.01f))));

    if (substeps > 1) {
        glm::vec3 current = position;
        const glm::vec3 slice = delta / static_cast<float>(substeps);
        bool grounded = false;
        for (int i = 0; i < substeps; ++i) {
            bool stepGrounded = false;
            current = moveCharacter(current, halfExtents, slice, stepGrounded);
            grounded = grounded || stepGrounded;
        }
        outGrounded = grounded;
        return current;
    }

    glm::vec3 pos = position;

    pos.x += resolveAxis(pos, halfExtents, 0, delta.x);
    pos.z += resolveAxis(pos, halfExtents, 2, delta.z);

    const float appliedY = resolveAxis(pos, halfExtents, 1, delta.y);
    pos.y += appliedY;

    // Grounded when we tried to move down but were stopped, or a probe just
    // below the feet finds geometry.
    outGrounded = false;
    if (delta.y <= 0.0f && appliedY > delta.y - 0.0001f && appliedY > delta.y * 0.999f) {
        // fell the full requested distance: still check for ground contact
    }
    if (delta.y < 0.0f && appliedY > delta.y + 0.0001f) outGrounded = true;

    glm::vec3 probe = pos;
    probe.y -= 0.06f;
    if (overlapsAny(AABB::fromCentre(probe, halfExtents))) outGrounded = true;

    return pos;
}

}  // namespace vajra
