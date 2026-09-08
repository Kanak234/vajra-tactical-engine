#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vajra {

class Input;

/// Look-only camera. Position is driven by the character controller so the
/// player collides with the world instead of flying through it.
class Camera {
public:
    void updateLook(const Input& input, float dt);

    void setAspect(float a) { m_aspect = a; }
    void setPosition(const glm::vec3& p) { m_position = p; }
    void setFov(float degrees) { m_fovDeg = degrees; }

    /// Additive recoil kick, decayed every frame.
    void addRecoil(float pitchDegrees, float yawDegrees) {
        m_recoilPitch += pitchDegrees;
        m_recoilYaw   += yawDegrees;
    }

    [[nodiscard]] glm::mat4 view() const {
        return glm::lookAt(m_position, m_position + forward(), kWorldUp);
    }
    [[nodiscard]] glm::mat4 projection() const {
        return glm::perspective(glm::radians(m_fovDeg), m_aspect, m_near, m_far);
    }
    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 flatForward() const;
    [[nodiscard]] const glm::vec3& position() const { return m_position; }
    [[nodiscard]] float yaw() const { return m_yaw; }
    [[nodiscard]] float fov() const { return m_fovDeg; }

private:
    static constexpr glm::vec3 kWorldUp{0.0f, 1.0f, 0.0f};

    glm::vec3 m_position{0.0f, 1.7f, 6.0f};
    float m_yaw   = -90.0f;
    float m_pitch = 0.0f;

    float m_recoilPitch = 0.0f;
    float m_recoilYaw   = 0.0f;

    float m_fovDeg      = 70.0f;
    float m_aspect      = 16.0f / 9.0f;
    float m_near        = 0.05f;
    float m_far         = 500.0f;
    float m_sensitivity = 0.075f;
};

}  // namespace vajra
