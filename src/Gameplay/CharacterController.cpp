#include "Gameplay/CharacterController.h"

#include <SDL2/SDL.h>
#include <algorithm>

#include "Input/Input.h"
#include "Physics/Collision.h"
#include "Renderer/Camera.h"

namespace vajra {

glm::vec3 CharacterController::halfExtents() const {
    const float height = (m_stance == Stance::Crouched) ? kCrouchHeight : kStandHeight;
    return glm::vec3{kRadius, height * 0.5f, kRadius};
}

float CharacterController::noiseLevel() const {
    if (m_stance == Stance::Crouched) return 0.15f;   // near-silent
    const float s = speed();
    if (s < 0.2f) return 0.0f;
    return m_sprinting ? 1.0f : 0.45f;
}

void CharacterController::update(const Input& input, Camera& camera,
                                 const PhysicsWorld& world, float dt) {
    // ---- stance ------------------------------------------------------------
    const bool wantsCrouch = input.key(SDL_SCANCODE_LCTRL) || input.key(SDL_SCANCODE_C);
    if (wantsCrouch) {
        m_stance = Stance::Crouched;
    } else if (m_stance == Stance::Crouched) {
        // Only stand back up if there is headroom — otherwise you would pop
        // through the ceiling of a crawlspace.
        const glm::vec3 standHalf{kRadius, kStandHeight * 0.5f, kRadius};
        const glm::vec3 centre = m_position + glm::vec3{0.0f, kStandHeight * 0.5f, 0.0f};
        bool blocked = false;
        const AABB probe = AABB::fromCentre(centre, standHalf * 0.98f);
        for (const AABB& b : world.boxes())
            if (probe.overlaps(b)) { blocked = true; break; }
        if (!blocked) m_stance = Stance::Standing;
    }

    m_sprinting = input.key(SDL_SCANCODE_LSHIFT) &&
                  m_stance == Stance::Standing && m_grounded;

    float targetSpeed = kWalkSpeed;
    if (m_stance == Stance::Crouched) targetSpeed = kCrouchSpeed;
    else if (m_sprinting)             targetSpeed = kSprintSpeed;

    // ---- desired direction from camera yaw ---------------------------------
    glm::vec3 wish{0.0f};
    const glm::vec3 fwd = camera.flatForward();
    const glm::vec3 rgt = glm::normalize(glm::cross(fwd, glm::vec3{0.0f, 1.0f, 0.0f}));
    if (input.key(SDL_SCANCODE_W)) wish += fwd;
    if (input.key(SDL_SCANCODE_S)) wish -= fwd;
    if (input.key(SDL_SCANCODE_D)) wish += rgt;
    if (input.key(SDL_SCANCODE_A)) wish -= rgt;

    const bool hasInput = glm::length(wish) > 0.0001f;
    if (hasInput) wish = glm::normalize(wish);

    // ---- horizontal acceleration & friction --------------------------------
    glm::vec3 horizontal{m_velocity.x, 0.0f, m_velocity.z};
    if (hasInput) {
        const glm::vec3 target = wish * targetSpeed;
        horizontal += (target - horizontal) * std::min(1.0f, kAccel * dt / std::max(1.0f, targetSpeed));
    } else {
        const float drop = kFriction * dt;
        const float len  = glm::length(horizontal);
        horizontal = (len > drop) ? horizontal * (1.0f - drop / len) : glm::vec3{0.0f};
    }
    m_velocity.x = horizontal.x;
    m_velocity.z = horizontal.z;

    // ---- gravity and jump --------------------------------------------------
    if (m_grounded && input.keyPressed(SDL_SCANCODE_SPACE) && m_stance == Stance::Standing) {
        m_velocity.y = kJumpSpeed;
        m_grounded = false;
    }
    m_velocity.y += kGravity * dt;
    m_velocity.y = std::max(m_velocity.y, -55.0f);   // terminal velocity

    // ---- integrate against the world ---------------------------------------
    const glm::vec3 half = halfExtents();
    const glm::vec3 centre = m_position + glm::vec3{0.0f, half.y, 0.0f};
    bool grounded = false;
    const glm::vec3 newCentre = world.moveCharacter(centre, half, m_velocity * dt, grounded);

    // Kill vertical velocity on contact so we do not accumulate fall speed.
    if (grounded && m_velocity.y < 0.0f) m_velocity.y = 0.0f;
    if (std::abs((newCentre.y - centre.y) - m_velocity.y * dt) > 0.0001f && m_velocity.y > 0.0f)
        m_velocity.y = 0.0f;   // hit a ceiling

    m_grounded = grounded;
    m_position = newCentre - glm::vec3{0.0f, half.y, 0.0f};

    // ---- eye height & camera -----------------------------------------------
    const float targetEye = (m_stance == Stance::Crouched) ? kCrouchEye : kStandEye;
    m_currentEyeHeight += (targetEye - m_currentEyeHeight) * std::min(1.0f, dt * 11.0f);
    camera.setPosition(eyePosition());
}

}  // namespace vajra
