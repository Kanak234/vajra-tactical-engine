#version 460 core
in vec2 vUV;
out vec4 FragColour;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uBloomStrength;
uniform float uVignette;

// ACES filmic tonemap (Narkowicz fit). Handles bright highlights far more
// gracefully than Reinhard, which desaturates everything toward white.
vec3 acesToneMap(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 colour = texture(uScene, vUV).rgb;
    colour += texture(uBloom, vUV).rgb * uBloomStrength;

    colour *= uExposure;
    colour = acesToneMap(colour);

    vec2 centred = vUV - 0.5;
    float vig = 1.0 - dot(centred, centred) * uVignette;
    colour *= clamp(vig, 0.0, 1.0);

    // Linear to sRGB. Done manually because the default framebuffer is not
    // sRGB-encoded once we stop rendering to it directly.
    colour = pow(colour, vec3(1.0 / 2.2));

    FragColour = vec4(colour, 1.0);
}
