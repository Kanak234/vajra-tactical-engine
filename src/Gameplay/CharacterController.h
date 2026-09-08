#pragma once
#include <glm/glm.hpp>

namespace vajra {

class Input;
class Camera;
class PhysicsWorld;

enum class Stance { Standing, Crouched };

/// Capsule-free character controller: an AABB with gravity, ground detection,
/// acceleration-based movement and stance-dependent speed. Movement is slow on
/// purpose — the whole design rests on patience, not twitch.
class CharacterController {
public:
    void update(const Input& input, Camera& camera, const PhysicsWorld& world, float dt);

    void teleport(const glm::vec3& feetPosition) {
        m_position = feetPosition;
        m_velocity = glm::vec3{0.0f};
    }

    [[nodiscard]] glm::vec3 eyePosition() const {
        return m_position + glm::vec3{0.0f, m_currentEyeHeight, 0.0f};
    }
    [[nodiscard]] const glm::vec3& feetPosition() const { return m_position; }
    [[nodiscard]] Stance stance() const { return m_stance; }
    [[nodiscard]] bool grounded() const { return m_grounded; }
    [[nodiscard]] bool sprinting() const { return m_sprinting; }
    [[nodiscard]] float speed() const { return glm::length(glm::vec3{m_velocity.x, 0.0f, m_velocity.z}); }

    /// How much noise the player is currently making, 0..1. Guards hear this.
    [[nodiscard]] float noiseLevel() const;

    /// Half-extents of the collision box for the current stance.
    [[nodiscard]] glm::vec3 halfExtents() const;

private:
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};   // feet
    glm::vec3 m_velocity{0.0f};
    Stance    m_stance = Stance::Standing;
    bool      m_grounded  = false;
    bool      m_sprinting = false;
    float     m_currentEyeHeight = 1.65f;

    static constexpr float kStandHeight  = 1.80f;
    static constexpr float kCrouchHeight = 1.05f;
    static constexpr float kRadius       = 0.32f;
    static constexpr float kStandEye     = 1.65f;
    static constexpr float kCrouchEye    = 0.95f;

    static constexpr float kWalkSpeed   = 3.0f;
    static constexpr float kSprintSpeed = 5.8f;
    static constexpr float kCrouchSpeed = 1.3f;
    static constexpr float kAccel       = 42.0f;
    static constexpr float kFriction    = 12.0f;
    static constexpr float kGravity     = -19.6f;
    static constexpr float kJumpSpeed   = 4.6f;
};

}  // namespace vajra
