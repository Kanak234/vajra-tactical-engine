#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace vajra {

/// Single-cascade directional shadow map. One cascade is enough for a station
/// interior; the light matrix is fitted around the camera so resolution stays
/// where the player is looking rather than spread across the whole level.
class ShadowMap {
public:
    bool init(int resolution = 2048);
    void shutdown();

    /// Binds the depth framebuffer and sets the viewport. Draw occluders after.
    void beginDepthPass(const glm::vec3& lightDirection, const glm::vec3& focusPoint,
                        float extent = 45.0f);
    void endDepthPass(int screenWidth, int screenHeight);

    void bindTexture(GLenum unit) const {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    }

    [[nodiscard]] const glm::mat4& lightSpaceMatrix() const { return m_lightSpace; }
    [[nodiscard]] bool valid() const { return m_fbo != 0; }

private:
    GLuint m_fbo = 0;
    GLuint m_depthTexture = 0;
    int    m_resolution = 0;
    glm::mat4 m_lightSpace{1.0f};
};

}  // namespace vajra
