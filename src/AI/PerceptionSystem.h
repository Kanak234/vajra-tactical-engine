#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace vajra {

/// Builds the awareness value every guard uses to decide whether it has seen
/// the player. Deliberately gradual: distance and angle scale how fast
/// awareness climbs, and it decays when line of sight breaks. That decay
/// window is what makes "break contact and reposition" a viable tactic.
class PerceptionSystem {
public:
    /// `losTest` returns true when nothing blocks the segment a->b.
    /// It is injected so this system stays independent of the physics backend.
    using LineOfSightFn = bool (*)(const glm::vec3& from, const glm::vec3& to, void* userData);

    void tick(entt::registry& registry, float dt,
              LineOfSightFn losTest = nullptr, void* userData = nullptr);

    /// Broadcast a noise event; every guard in range gets a suspicion bump.
    void emitNoise(entt::registry& registry, const glm::vec3& origin, float loudness);

private:
    static constexpr float kGainPerSecond  = 1.6f;
    static constexpr float kDecayPerSecond = 0.35f;
};

}  // namespace vajra
