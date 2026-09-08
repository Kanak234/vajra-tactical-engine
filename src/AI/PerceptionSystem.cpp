#include "AI/PerceptionSystem.h"

#include <algorithm>
#include <glm/gtx/norm.hpp>

#include "Scene/Components.h"

namespace vajra {

void PerceptionSystem::tick(entt::registry& registry, float dt,
                            LineOfSightFn losTest, void* userData) {
    // Resolve the player once per tick rather than per guard.
    glm::vec3 playerPos{0.0f};
    bool playerExists = false;
    for (auto [entity, transform] :
         registry.view<Transform, PlayerControlled>().each()) {
        playerPos = transform.position;
        playerExists = true;
        break;
    }
    if (!playerExists) return;

    auto guards = registry.view<Transform, Perception, Combatant>();
    for (auto [entity, transform, perception, combatant] : guards.each()) {
        const glm::vec3 toTarget = playerPos - transform.position;
        const float distSq = glm::length2(toTarget);

        bool visible = false;
        if (distSq <= perception.visionRange * perception.visionRange) {
            const glm::vec3 facing =
                glm::normalize(transform.rotation * glm::vec3{0.0f, 0.0f, -1.0f});
            const glm::vec3 dir = glm::normalize(toTarget);
            const float cosLimit = std::cos(glm::radians(perception.visionHalfAngle));

            if (glm::dot(facing, dir) >= cosLimit) {
                const glm::vec3 eye  = transform.position + glm::vec3{0.0f, 1.6f, 0.0f};
                const glm::vec3 head = playerPos + glm::vec3{0.0f, 1.5f, 0.0f};
                visible = losTest ? losTest(eye, head, userData) : true;
            }
        }

        perception.hasLineOfSight = visible;

        if (visible) {
            // Closer targets resolve faster; distant ones take a beat to read.
            const float dist  = std::sqrt(distSq);
            const float prox  = 1.0f - std::clamp(dist / perception.visionRange, 0.0f, 1.0f);
            perception.awareness += kGainPerSecond * (0.35f + prox) * dt;
            perception.lastKnownTargetPos = playerPos;
        } else {
            perception.awareness -= kDecayPerSecond * dt;
        }
        perception.awareness = std::clamp(perception.awareness, 0.0f, 1.0f);

        // Awareness drives the alert state machine.
        const AlertState previous = combatant.state;
        if (perception.awareness >= 0.99f) {
            combatant.state = AlertState::Combat;
        } else if (perception.awareness >= 0.45f) {
            combatant.state = visible ? AlertState::Combat : AlertState::Searching;
        } else if (perception.awareness > 0.05f) {
            combatant.state = AlertState::Suspicious;
        } else {
            combatant.state = AlertState::Idle;
        }

        combatant.stateTimer = (combatant.state == previous)
                             ? combatant.stateTimer + dt
                             : 0.0f;

        if (combatant.fireCooldown > 0.0f) combatant.fireCooldown -= dt;
    }
}

void PerceptionSystem::emitNoise(entt::registry& registry,
                                 const glm::vec3& origin, float loudness) {
    auto guards = registry.view<Transform, Perception>();
    for (auto [entity, transform, perception] : guards.each()) {
        const float range = perception.hearingRange * loudness;
        const float distSq = glm::length2(origin - transform.position);
        if (distSq > range * range) continue;

        const float falloff = 1.0f - std::sqrt(distSq) / range;
        perception.awareness = std::min(1.0f, perception.awareness + 0.5f * falloff);
        perception.lastKnownTargetPos = origin;
    }
}

}  // namespace vajra
