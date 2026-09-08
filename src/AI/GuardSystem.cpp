#include "AI/GuardSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/gtx/norm.hpp>

#include "AI/Navigation.h"
#include "Physics/Collision.h"
#include "Scene/Components.h"

namespace vajra {

namespace {

float randUnit() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }

/// Rotate a guard smoothly toward a world direction on the XZ plane.
void faceTowards(Transform& transform, const glm::vec3& targetPos, float dt, float rate = 6.0f) {
    const glm::vec3 delta{targetPos.x - transform.position.x, 0.0f,
                          targetPos.z - transform.position.z};
    if (glm::length2(delta) < 0.0001f) return;

    const float desiredYaw = std::atan2(delta.x, -delta.z);
    const glm::quat target = glm::angleAxis(desiredYaw, glm::vec3{0.0f, 1.0f, 0.0f});
    transform.rotation = glm::slerp(transform.rotation, target, std::min(1.0f, dt * rate));
}

}  // namespace

void GuardSystem::followPath(entt::registry& registry, entt::entity entity,
                             const NavGrid& nav, const glm::vec3& goal,
                             float speed, float dt) {
    auto& transform = registry.get<Transform>(entity);
    auto& agent     = registry.get<NavAgent>(entity);

    agent.repathTimer -= dt;

    const bool goalMoved = !agent.hasGoal || glm::length2(goal - agent.goal) > 4.0f;
    if (goalMoved || agent.repathTimer <= 0.0f || agent.index >= agent.path.size()) {
        if (nav.findPath(transform.position, goal, agent.path)) {
            agent.index   = 0;
            agent.goal    = goal;
            agent.hasGoal = true;
        } else {
            agent.path.clear();
            agent.hasGoal = false;
        }
        agent.repathTimer = 0.6f + randUnit() * 0.4f;   // stagger repaths
    }

    if (agent.index >= agent.path.size()) return;

    const glm::vec3 waypoint = agent.path[agent.index];
    const glm::vec3 delta{waypoint.x - transform.position.x, 0.0f,
                          waypoint.z - transform.position.z};
    const float dist = glm::length(delta);

    if (dist < 0.45f) {
        ++agent.index;
        return;
    }

    transform.position += (delta / dist) * speed * dt;
    faceTowards(transform, waypoint, dt, 8.0f);
}

void GuardSystem::tick(entt::registry& registry, const PhysicsWorld& world,
                       const NavGrid& nav, PlayerRef& player, float dt,
                       const GuardEvents& events) {
    m_guardsAlive = 0;
    if (m_alarmCooldown > 0.0f) m_alarmCooldown -= dt;

    bool anyoneInCombat = false;
    glm::vec3 sharedContact{0.0f};
    bool hasSharedContact = false;

    // ---- pass one: individual behaviour ------------------------------------
    auto guards = registry.view<Transform, Perception, Combatant, Health, NavAgent>();
    for (auto [entity, transform, perception, combatant, health, agent] : guards.each()) {
        if (!health.alive()) continue;
        ++m_guardsAlive;

        const bool isMachine = registry.all_of<Machine>(entity);

        // Hearing: the player's own movement noise. Machines have no microphones.
        if (player.alive && player.noise > 0.0f && !isMachine) {
            const float heard = perception.hearingRange * player.noise;
            const float d2 = glm::length2(player.feetPosition - transform.position);
            if (d2 < heard * heard) {
                perception.awareness = std::min(1.0f, perception.awareness + 0.30f * dt);
                perception.lastKnownTargetPos = player.feetPosition;
            }
        }

        switch (combatant.state) {
            case AlertState::Idle: {
                // Patrol the assigned route.
                if (auto* route = registry.try_get<PatrolRoute>(entity)) {
                    if (route->waypoints.size() >= 2) {
                        if (route->waitTimer > 0.0f) {
                            route->waitTimer -= dt;
                        } else {
                            const glm::vec3 target = route->waypoints[route->index];
                            followPath(registry, entity, nav, target, 1.5f, dt);
                            if (glm::length2(glm::vec3{target.x - transform.position.x, 0.0f,
                                                       target.z - transform.position.z}) < 0.9f) {
                                route->index = (route->index + 1) % route->waypoints.size();
                                route->waitTimer = 1.5f + randUnit() * 2.0f;
                                agent.hasGoal = false;
                            }
                        }
                    }
                }
                break;
            }

            case AlertState::Suspicious: {
                // Stop and look toward whatever caught their attention.
                faceTowards(transform, perception.lastKnownTargetPos, dt, 3.0f);
                break;
            }

            case AlertState::Searching: {
                // Move to the last known position and sweep around it.
                followPath(registry, entity, nav, perception.lastKnownTargetPos, 2.6f, dt);
                if (combatant.stateTimer > 12.0f) {
                    perception.awareness = std::max(0.0f, perception.awareness - 0.25f * dt);
                }
                break;
            }

            case AlertState::Combat: {
                if (!isMachine) {
                    anyoneInCombat = true;
                    sharedContact = perception.lastKnownTargetPos;
                    hasSharedContact = true;
                }

                if (!m_sectorAlerted && m_alarmCooldown <= 0.0f) {
                    m_sectorAlerted = true;
                    m_alarmCooldown = 5.0f;
                    if (events.onAlarmRaised) events.onAlarmRaised(transform.position);
                }

                faceTowards(transform, player.eyePosition, dt, 9.0f);

                const float dist = glm::length(player.feetPosition - transform.position);

                // Close to an effective range but keep some distance.
                if (dist > 18.0f)
                    followPath(registry, entity, nav, player.feetPosition, 3.1f, dt);
                else
                    agent.hasGoal = false;

                // Fire only with line of sight and after a reaction delay —
                // instant accurate return fire feels unfair and unreadable.
                const glm::vec3 muzzle = transform.position + glm::vec3{0.0f, 1.5f, 0.0f};

                // Re-test the shot line from the muzzle, not the eyes: a guard
                // whose head clears a crate but whose rifle does not should
                // hold fire rather than shoot through the cover.
                const bool canShoot = perception.hasLineOfSight &&
                                      world.lineOfSight(muzzle, player.eyePosition);

                if (canShoot && combatant.stateTimer > 0.45f &&
                    combatant.fireCooldown <= 0.0f && player.alive) {
                    const glm::vec3 toPlayer = glm::normalize(player.eyePosition - muzzle);

                    // Accuracy falls off with range; guards miss more when you move.
                    const float rangePenalty = std::clamp(dist / 45.0f, 0.0f, 1.0f);
                    const float chance = combatant.accuracy * (1.0f - rangePenalty * 0.65f);
                    const bool  onTarget = randUnit() < chance;

                    // Misses are scattered perpendicular to the aim line, so
                    // tracers still read as "shot at me" instead of random.
                    glm::vec3 lateral = glm::cross(toPlayer, glm::vec3{0.0f, 1.0f, 0.0f});
                    if (glm::length2(lateral) < 0.001f) lateral = glm::vec3{1.0f, 0.0f, 0.0f};
                    lateral = glm::normalize(lateral);
                    const glm::vec3 vertical = glm::cross(lateral, toPlayer);

                    const glm::vec3 impact = onTarget
                        ? player.eyePosition
                        : player.eyePosition + lateral * ((randUnit() - 0.5f) * 3.0f)
                                             + vertical * ((randUnit() - 0.5f) * 2.0f);

                    if (events.onShotFired) events.onShotFired(muzzle, impact);

                    if (onTarget) {
                        const float damage = 9.0f + randUnit() * 7.0f;
                        player.health -= damage;
                        if (events.onPlayerHit) events.onPlayerHit(damage);
                        if (player.health <= 0.0f) { player.health = 0.0f; player.alive = false; }
                    }
                    combatant.fireCooldown = 0.55f + randUnit() * 0.7f;
                }
                break;
            }

            case AlertState::Retreating:
            default:
                break;
        }
    }

    // ---- pass two: squad radio net -----------------------------------------
    // A guard in contact tells his squad. They do not instantly know where you
    // are; they get a suspicion bump and the last known position, which sends
    // them searching. This is what makes one loud shot cascade.
    if (anyoneInCombat && hasSharedContact) {
        for (auto [entity, transform, perception, combatant, health, agent] : guards.each()) {
            if (!health.alive()) continue;
            if (combatant.state == AlertState::Combat) continue;
            // Autonomous units are not on the human comms channel, so a squad
            // callout never reaches them. Sneaking past a drone stays possible
            // even after the human garrison is fully alerted.
            if (registry.all_of<Machine>(entity)) continue;
            if (glm::length2(sharedContact - transform.position) > 120.0f * 120.0f) continue;

            perception.awareness = std::max(perception.awareness, 0.55f);
            perception.lastKnownTargetPos = sharedContact;
        }
    }
}

}  // namespace vajra
