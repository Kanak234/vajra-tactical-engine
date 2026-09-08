#include "Renderer/RenderTarget.h"

#include "Core/Log.h"

namespace vajra {

bool RenderTarget::create(int width, int height, const std::vector<Attachment>& colours,
                          bool withDepthTexture) {
    destroy();
    m_spec = colours;
    m_hasDepth = withDepthTexture;
    m_width = width > 1 ? width : 1;
    m_height = height > 1 ? height : 1;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    std::vector<GLenum> drawBuffers;
    m_colourTextures.resize(colours.size(), 0);

    for (size_t i = 0; i < colours.size(); ++i) {
        glGenTextures(1, &m_colourTextures[i]);
        glBindTexture(GL_TEXTURE_2D, m_colourTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(colours[i].internalFormat),
                     m_width, m_height, 0, colours[i].format, colours[i].type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(colours[i].filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(colours[i].filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        const GLenum slot = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, slot, GL_TEXTURE_2D, m_colourTextures[i], 0);
        drawBuffers.push_back(slot);
    }

    if (drawBuffers.empty()) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    }

    if (withDepthTexture) {
        glGenTextures(1, &m_depthTexture);
        glBindTexture(GL_TEXTURE_2D, m_depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_width, m_height, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               m_depthTexture, 0);
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        VJ_ERROR("RenderTarget incomplete (0x%x) at %dx%d", status, m_width, m_height);
        destroy();
        return false;
    }
    return true;
}

void RenderTarget::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    if (width < 1 || height < 1) return;
    create(width, height, m_spec, m_hasDepth);
}

void RenderTarget::destroy() {
    if (!m_colourTextures.empty())
        glDeleteTextures(static_cast<GLsizei>(m_colourTextures.size()), m_colourTextures.data());
    m_colourTextures.clear();
    if (m_depthTexture) glDeleteTextures(1, &m_depthTexture);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    m_depthTexture = m_fbo = 0;
}

void RenderTarget::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void RenderTarget::bindDefault(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

}  // namespace vajra
