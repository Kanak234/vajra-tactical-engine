#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace vajra {

class Mesh;

// ---------------------------------------------------------------- spatial ---
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    [[nodiscard]] glm::mat4 matrix() const {
        return glm::translate(glm::mat4{1.0f}, position)
             * glm::mat4_cast(rotation)
             * glm::scale(glm::mat4{1.0f}, scale);
    }
};

/// Previous-frame transform, kept so the renderer can interpolate between
/// fixed simulation steps instead of stuttering at non-60Hz refresh rates.
struct PrevTransform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

// ---------------------------------------------------------------- visual ----
struct MeshRenderer {
    const Mesh* mesh = nullptr;
    glm::vec3   albedo{0.8f};
    float       metallic  = 0.0f;
    float       roughness = 0.7f;
    bool        castsShadow = true;
};

struct PointLight {
    glm::vec3 colour{1.0f};
    float     intensity = 10.0f;
    float     radius    = 15.0f;
};

// ---------------------------------------------------------------- gameplay --
struct Health {
    float current = 100.0f;
    float max     = 100.0f;
    [[nodiscard]] bool alive() const { return current > 0.0f; }
};

struct RigidBody {
    glm::vec3 velocity{0.0f};
    float     mass = 80.0f;
    bool      onGround = false;
};

/// Sensory profile for an AI agent. Detection is a cone test plus a raycast,
/// with an awareness meter so spotting is gradual, not binary.
struct Perception {
    float visionRange     = 60.0f;
    float visionHalfAngle = 55.0f;   // degrees, half of the full FOV cone
    float hearingRange    = 25.0f;
    float awareness       = 0.0f;    // 0 = oblivious, 1 = fully alerted
    glm::vec3 lastKnownTargetPos{0.0f};
    bool  hasLineOfSight = false;
};

enum class AlertState { Idle, Suspicious, Searching, Combat, Retreating };

/// Marks an autonomous machine: no radio net, no hearing, but a far wider
/// field of view. Fighting them is a different problem to fighting people.
struct Machine {};

struct Combatant {
    AlertState state       = AlertState::Idle;
    float      stateTimer  = 0.0f;
    float      accuracy    = 0.55f;
    float      fireCooldown = 0.0f;
    int        squadId     = -1;
};

/// Cached A* result plus where along it the agent currently is.
struct NavAgent {
    std::vector<glm::vec3> path;
    size_t index = 0;
    float  repathTimer = 0.0f;
    float  speed = 2.0f;
    glm::vec3 goal{0.0f};
    bool   hasGoal = false;
};

/// Animation state for the procedural actor rig.
struct ActorAnim {
    float walkPhase = 0.0f;
    float walkSpeed = 0.0f;
    float crouch = 0.0f;
    float aim = 0.0f;
    float death = 0.0f;
    glm::vec3 lastPosition{0.0f};
};

struct Corpse {
    float timer = 0.0f;
    bool  discovered = false;
};

struct PatrolRoute {
    std::vector<glm::vec3> waypoints;
    size_t index = 0;
    bool   loop  = true;
    float  waitTimer = 0.0f;
};

struct Tag {
    std::string name;
};

/// A destructible mission target (comms relay, server rack, generator).
struct Objective {
    std::string id;
    std::string label;
    float radius = 2.5f;
    bool  requiresDestroy = false;
    bool  complete = false;
};

/// The zone the player must reach to finish the mission.
struct ExtractionZone {
    float radius = 4.0f;
    bool  active = false;
};

/// Marks the entity the camera and input drive.
struct PlayerControlled {};

}  // namespace vajra
