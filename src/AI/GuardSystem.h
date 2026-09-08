#pragma once
#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>

namespace vajra {

class PhysicsWorld;
class NavGrid;
class PerceptionSystem;

/// Events the guards raise so the application can play sound and spawn tracers
/// without the AI knowing anything about audio or rendering.
struct GuardEvents {
    std::function<void(const glm::vec3& from, const glm::vec3& to)> onShotFired;
    std::function<void(float amount)>                               onPlayerHit;
    std::function<void(const glm::vec3& at)>                         onAlarmRaised;
};

/// Drives guard behaviour on top of the alert state produced by
/// PerceptionSystem: patrol, investigate, path to the player, take shots, and
/// share contacts with their squad over a simulated radio net.
class GuardSystem {
public:
    struct PlayerRef {
        glm::vec3 eyePosition{0.0f};
        glm::vec3 feetPosition{0.0f};
        float     health = 100.0f;
        float     noise  = 0.0f;
        bool      alive  = true;
    };

    void tick(entt::registry& registry, const PhysicsWorld& world, const NavGrid& nav,
              PlayerRef& player, float dt, const GuardEvents& events);

    /// True once any guard has reached full alert — drives the HUD warning and
    /// the "sector alerted" mission modifier.
    [[nodiscard]] bool sectorAlerted() const { return m_sectorAlerted; }
    [[nodiscard]] int  guardsAlive() const { return m_guardsAlive; }
    void reset() { m_sectorAlerted = false; m_alarmCooldown = 0.0f; }

private:
    void followPath(entt::registry& registry, entt::entity entity,
                    const NavGrid& nav, const glm::vec3& goal, float speed, float dt);

    bool m_sectorAlerted = false;
    float m_alarmCooldown = 0.0f;
    int  m_guardsAlive = 0;
};

}  // namespace vajra
