#pragma once
#include <glad/glad.h>
#include <vector>

namespace vajra {

/// Framebuffer with any number of colour attachments and an optional depth
/// texture. Owns its textures; resizing recreates them.
class RenderTarget {
public:
    struct Attachment {
        GLenum internalFormat = GL_RGBA16F;
        GLenum format         = GL_RGBA;
        GLenum type           = GL_FLOAT;
        GLenum filter         = GL_LINEAR;
    };

    RenderTarget() = default;
    ~RenderTarget() { destroy(); }
    RenderTarget(const RenderTarget&)            = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    bool create(int width, int height, const std::vector<Attachment>& colours,
                bool withDepthTexture);
    void resize(int width, int height);
    void destroy();

    void bind() const;
    static void bindDefault(int width, int height);

    [[nodiscard]] GLuint colour(size_t index = 0) const {
        return index < m_colourTextures.size() ? m_colourTextures[index] : 0;
    }
    [[nodiscard]] GLuint depth() const { return m_depthTexture; }
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] bool valid() const { return m_fbo != 0; }

private:
    GLuint m_fbo = 0;
    GLuint m_depthTexture = 0;
    std::vector<GLuint> m_colourTextures;
    std::vector<Attachment> m_spec;
    bool m_hasDepth = false;
    int  m_width = 0;
    int  m_height = 0;
};

}  // namespace vajra
