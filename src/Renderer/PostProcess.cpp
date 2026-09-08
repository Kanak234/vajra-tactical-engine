#include "Renderer/PostProcess.h"

#include <cmath>
#include <cstdlib>

#include "Core/Log.h"

namespace vajra {

namespace {
constexpr int kKernelSize = 24;
constexpr int kNoiseSize  = 4;
constexpr int kBloomIterations = 3;
}  // namespace

void PostProcess::buildKernel() {
    m_kernel.clear();
    m_kernel.reserve(kKernelSize);

    auto rand01 = [] { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); };

    for (int i = 0; i < kKernelSize; ++i) {
        glm::vec3 sample{rand01() * 2.0f - 1.0f, rand01() * 2.0f - 1.0f, rand01()};
        sample = glm::normalize(sample) * rand01();

        // Cluster samples toward the origin: near-field occlusion matters more
        // than far, and this is what stops SSAO looking like uniform grey haze.
        float scale = static_cast<float>(i) / static_cast<float>(kKernelSize);
        scale = 0.1f + 0.9f * scale * scale;
        m_kernel.push_back(sample * scale);
    }

    // 4x4 tiled rotation noise, repeated across the screen.
    std::vector<glm::vec3> noise;
    noise.reserve(kNoiseSize * kNoiseSize);
    for (int i = 0; i < kNoiseSize * kNoiseSize; ++i)
        noise.emplace_back(rand01() * 2.0f - 1.0f, rand01() * 2.0f - 1.0f, 0.0f);

    if (m_noiseTexture) glDeleteTextures(1, &m_noiseTexture);
    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kNoiseSize, kNoiseSize, 0, GL_RGB,
                 GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

bool PostProcess::init(int width, int height, const std::string& shaderDir) {
    m_shaderDir = shaderDir;
    m_width = width;
    m_height = height;

    glGenVertexArrays(1, &m_fullscreenVao);   // attributeless fullscreen triangle

    reloadShaders();
    buildKernel();
    resize(width, height);

    m_valid = m_hdr.valid() && m_gbuffer.valid();
    if (!m_valid) VJ_ERROR("PostProcess failed to initialise - falling back to direct render");
    return m_valid;
}

void PostProcess::reloadShaders() {
    const std::string vs = m_shaderDir + "fullscreen.vert";
    m_prepass        = Shader{m_shaderDir + "prepass.vert", m_shaderDir + "prepass.frag"};
    m_ssaoShader     = Shader{vs, m_shaderDir + "ssao.frag"};
    m_blurShader     = Shader{vs, m_shaderDir + "ssao_blur.frag"};
    m_brightShader   = Shader{vs, m_shaderDir + "bright.frag"};
    m_gaussianShader = Shader{vs, m_shaderDir + "gaussian.frag"};
    m_compositeShader= Shader{vs, m_shaderDir + "composite.frag"};
    m_fxaaShader     = Shader{vs, m_shaderDir + "fxaa.frag"};
}

void PostProcess::resize(int width, int height) {
    if (width < 1 || height < 1) return;
    m_width = width;
    m_height = height;
    const int halfW = std::max(1, width / 2);
    const int halfH = std::max(1, height / 2);

    m_gbuffer.create(width, height, {{GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST}}, true);
    m_hdr.create(width, height, {{GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR}}, true);
    m_ssao.create(halfW, halfH, {{GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR}}, false);
    m_ssaoBlur.create(halfW, halfH, {{GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR}}, false);
    m_bloomA.create(halfW, halfH, {{GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR}}, false);
    m_bloomB.create(halfW, halfH, {{GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR}}, false);
    m_ldr.create(width, height, {{GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR}}, false);
}

void PostProcess::shutdown() {
    if (m_noiseTexture) glDeleteTextures(1, &m_noiseTexture);
    if (m_fullscreenVao) glDeleteVertexArrays(1, &m_fullscreenVao);
    m_noiseTexture = m_fullscreenVao = 0;
    m_gbuffer.destroy(); m_hdr.destroy(); m_ssao.destroy(); m_ssaoBlur.destroy();
    m_bloomA.destroy(); m_bloomB.destroy(); m_ldr.destroy();
}

void PostProcess::drawFullscreen() {
    // One oversized triangle rather than a quad: no diagonal seam, and the
    // vertices are generated in the shader from gl_VertexID.
    glBindVertexArray(m_fullscreenVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void PostProcess::beginPrepass() {
    m_gbuffer.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void PostProcess::endPrepass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::runSsao(const glm::mat4& projection, const glm::mat4& view,
                          float radius, float strength) {
    if (!m_ssaoEnabled || !m_ssaoShader.valid()) return;

    glDisable(GL_DEPTH_TEST);

    m_ssao.bind();
    m_ssaoShader.bind();
    m_ssaoShader.set("uProjection", projection);
    m_ssaoShader.set("uView", view);
    m_ssaoShader.set("uInverseProjection", glm::inverse(projection));
    m_ssaoShader.set("uRadius", radius);
    m_ssaoShader.set("uStrength", strength);
    m_ssaoShader.set("uNoiseScale",
                     glm::vec2(static_cast<float>(m_ssao.width()) / kNoiseSize,
                               static_cast<float>(m_ssao.height()) / kNoiseSize));
    for (size_t i = 0; i < m_kernel.size(); ++i) {
        const std::string name = "uKernel[" + std::to_string(i) + "]";
        m_ssaoShader.set(name.c_str(), m_kernel[i]);
    }
    m_ssaoShader.set("uDepth", 0);
    m_ssaoShader.set("uNormals", 1);
    m_ssaoShader.set("uNoise", 2);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_gbuffer.depth());
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_gbuffer.colour(0));
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    drawFullscreen();

    // 4x4 box blur removes the noise pattern without a separable two-pass blur.
    m_ssaoBlur.bind();
    m_blurShader.bind();
    m_blurShader.set("uSource", 0);
    m_blurShader.set("uTexelSize", glm::vec2(1.0f / static_cast<float>(m_ssao.width()),
                                             1.0f / static_cast<float>(m_ssao.height())));
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_ssao.colour(0));
    drawFullscreen();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void PostProcess::beginScene() {
    m_hdr.bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);   // matches the prepass depth exactly
}

void PostProcess::endScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDepthFunc(GL_LESS);
}

void PostProcess::resolveToScreen(int screenWidth, int screenHeight, float exposure,
                                  float bloomStrength, float vignette) {
    glDisable(GL_DEPTH_TEST);

    GLuint bloomTexture = 0;

    if (m_bloomEnabled && m_brightShader.valid() && m_gaussianShader.valid()) {
        // Bright pass into half-res.
        m_bloomA.bind();
        m_brightShader.bind();
        m_brightShader.set("uSource", 0);
        m_brightShader.set("uThreshold", 1.05f);
        m_brightShader.set("uSoftKnee", 0.55f);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_hdr.colour(0));
        drawFullscreen();

        // Separable gaussian, ping-ponging horizontal and vertical.
        const glm::vec2 texel{1.0f / static_cast<float>(m_bloomA.width()),
                              1.0f / static_cast<float>(m_bloomA.height())};
        bool horizontal = true;
        for (int i = 0; i < kBloomIterations * 2; ++i) {
            RenderTarget& dst = horizontal ? m_bloomB : m_bloomA;
            RenderTarget& src = horizontal ? m_bloomA : m_bloomB;
            dst.bind();
            m_gaussianShader.bind();
            m_gaussianShader.set("uSource", 0);
            m_gaussianShader.set("uDirection",
                                 horizontal ? glm::vec2(texel.x, 0.0f) : glm::vec2(0.0f, texel.y));
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, src.colour(0));
            drawFullscreen();
            horizontal = !horizontal;
        }
        bloomTexture = horizontal ? m_bloomA.colour(0) : m_bloomB.colour(0);
    }

    // Tonemap + bloom + vignette. Goes to the LDR buffer if FXAA follows,
    // otherwise straight to the screen.
    const bool useFxaa = m_fxaaEnabled && m_fxaaShader.valid() && m_ldr.valid();
    if (useFxaa) m_ldr.bind();
    else         RenderTarget::bindDefault(screenWidth, screenHeight);

    m_compositeShader.bind();
    m_compositeShader.set("uScene", 0);
    m_compositeShader.set("uBloom", 1);
    m_compositeShader.set("uExposure", exposure);
    m_compositeShader.set("uBloomStrength", bloomTexture ? bloomStrength : 0.0f);
    m_compositeShader.set("uVignette", vignette);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_hdr.colour(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTexture ? bloomTexture : m_hdr.colour(0));
    drawFullscreen();

    if (useFxaa) {
        RenderTarget::bindDefault(screenWidth, screenHeight);
        m_fxaaShader.bind();
        m_fxaaShader.set("uSource", 0);
        m_fxaaShader.set("uTexelSize", glm::vec2(1.0f / static_cast<float>(m_ldr.width()),
                                                 1.0f / static_cast<float>(m_ldr.height())));
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_ldr.colour(0));
        drawFullscreen();
    }

    glEnable(GL_DEPTH_TEST);
}

}  // namespace vajra
