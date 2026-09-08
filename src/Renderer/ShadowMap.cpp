#include "Renderer/ShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Core/Log.h"

namespace vajra {

bool ShadowMap::init(int resolution) {
    m_resolution = resolution;

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_depthTexture);

    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, resolution, resolution, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // Border of 1.0 means "fully lit" outside the map, so distant geometry is
    // never wrongly shadowed.
    const float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    // Hardware comparison gives free 2x2 PCF on the sampler.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        VJ_ERROR("Shadow framebuffer incomplete (0x%x)", status);
        shutdown();
        return false;
    }
    return true;
}

void ShadowMap::shutdown() {
    if (m_depthTexture) glDeleteTextures(1, &m_depthTexture);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    m_depthTexture = m_fbo = 0;
}

void ShadowMap::beginDepthPass(const glm::vec3& lightDirection,
                               const glm::vec3& focusPoint, float extent) {
    const glm::vec3 dir = glm::normalize(lightDirection);
    const glm::vec3 eye = focusPoint - dir * (extent * 1.6f);

    glm::vec3 up{0.0f, 1.0f, 0.0f};
    if (std::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3{1.0f, 0.0f, 0.0f};

    const glm::mat4 lightView = glm::lookAt(eye, focusPoint, up);
    const glm::mat4 lightProj = glm::ortho(-extent, extent, -extent, extent,
                                           0.5f, extent * 4.0f);
    m_lightSpace = lightProj * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_resolution, m_resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    // Front-face culling during the depth pass removes most peter-panning.
    glCullFace(GL_FRONT);
}

void ShadowMap::endDepthPass(int screenWidth, int screenHeight) {
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

}  // namespace vajra
