#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "Renderer/RenderTarget.h"
#include "Renderer/Shader.h"

namespace vajra {

/// The full post-processing chain:
///   depth+normal prepass -> SSAO -> blur -> forward HDR -> bloom -> tonemap -> FXAA
/// Everything runs off two ping-pong half-resolution targets plus one
/// full-resolution HDR buffer, so the memory cost stays flat.
class PostProcess {
public:
    bool init(int width, int height, const std::string& shaderDir);
    void resize(int width, int height);
    void shutdown();
    void reloadShaders();

    // -- pass control ---------------------------------------------------------
    void beginPrepass();                       // renders depth + view normals
    void endPrepass();
    void runSsao(const glm::mat4& projection, const glm::mat4& view, float radius, float strength);
    void beginScene();                         // HDR forward target
    void endScene();
    /// Bloom + ACES tonemap + vignette + FXAA, straight to the backbuffer.
    void resolveToScreen(int screenWidth, int screenHeight, float exposure,
                         float bloomStrength, float vignette);

    [[nodiscard]] GLuint ssaoTexture() const { return m_ssaoBlur.colour(0); }
    [[nodiscard]] GLuint depthTexture() const { return m_gbuffer.depth(); }
    [[nodiscard]] Shader& prepassShader() { return m_prepass; }
    [[nodiscard]] bool valid() const { return m_valid; }
    void setEnabled(bool ssao, bool bloom, bool fxaa) {
        m_ssaoEnabled = ssao; m_bloomEnabled = bloom; m_fxaaEnabled = fxaa;
    }
    [[nodiscard]] bool ssaoEnabled() const { return m_ssaoEnabled; }
    [[nodiscard]] bool bloomEnabled() const { return m_bloomEnabled; }
    [[nodiscard]] bool fxaaEnabled() const { return m_fxaaEnabled; }

private:
    void drawFullscreen();
    void buildKernel();

    RenderTarget m_gbuffer;      // view normals + depth
    RenderTarget m_hdr;          // scene colour, RGBA16F
    RenderTarget m_ssao;         // AO, half res, single channel
    RenderTarget m_ssaoBlur;
    RenderTarget m_bloomA;       // half res ping
    RenderTarget m_bloomB;       // half res pong
    RenderTarget m_ldr;          // tonemapped, pre-FXAA

    Shader m_prepass, m_ssaoShader, m_blurShader, m_brightShader;
    Shader m_gaussianShader, m_compositeShader, m_fxaaShader;

    GLuint m_fullscreenVao = 0;
    GLuint m_noiseTexture = 0;
    std::vector<glm::vec3> m_kernel;

    std::string m_shaderDir;
    int  m_width = 0, m_height = 0;
    bool m_valid = false;
    bool m_ssaoEnabled = true;
    bool m_bloomEnabled = true;
    bool m_fxaaEnabled = true;
};

}  // namespace vajra
