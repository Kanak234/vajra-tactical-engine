#include "Renderer/Camera.h"

#include <algorithm>
#include <cmath>

#include "Input/Input.h"

namespace vajra {

glm::vec3 Camera::forward() const {
    const float pitch = std::clamp(m_pitch + m_recoilPitch, -89.0f, 89.0f);
    const float yaw   = m_yaw + m_recoilYaw;
    const float cy = std::cos(glm::radians(yaw));
    const float sy = std::sin(glm::radians(yaw));
    const float cp = std::cos(glm::radians(pitch));
    const float sp = std::sin(glm::radians(pitch));
    return glm::normalize(glm::vec3{cy * cp, sp, sy * cp});
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), kWorldUp));
}

glm::vec3 Camera::flatForward() const {
    const glm::vec3 f = forward();
    const glm::vec3 flat{f.x, 0.0f, f.z};
    const float len = glm::length(flat);
    return len > 0.0001f ? flat / len : glm::vec3{0.0f, 0.0f, -1.0f};
}

void Camera::updateLook(const Input& input, float dt) {
    // Aiming down sights narrows the FOV, which also scales sensitivity —
    // otherwise zoomed aim feels twitchy and uncontrollable.
    const float sensScale = m_fovDeg / 70.0f;

    const glm::vec2 delta = input.mouseDelta();
    m_yaw   += delta.x * m_sensitivity * sensScale;
    m_pitch -= delta.y * m_sensitivity * sensScale;
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);

    // Recoil recovers smoothly rather than snapping back.
    const float recovery = std::min(1.0f, dt * 6.0f);
    m_recoilPitch -= m_recoilPitch * recovery;
    m_recoilYaw   -= m_recoilYaw   * recovery;
}

}  // namespace vajra
