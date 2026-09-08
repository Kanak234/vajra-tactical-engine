#version 460 core
in vec2 vUV;
out vec4 FragColour;

uniform sampler2D uSource;
uniform vec2 uDirection;

// Nine-tap gaussian collapsed to five texture fetches using linear-filtering
// offsets — same result, roughly half the bandwidth.
const float WEIGHTS[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);
const float OFFSETS[3] = float[](0.0, 1.3846153846, 3.2307692308);

void main() {
    vec3 result = texture(uSource, vUV).rgb * WEIGHTS[0];
    for (int i = 1; i < 3; ++i) {
        vec2 offset = uDirection * OFFSETS[i];
        result += texture(uSource, vUV + offset).rgb * WEIGHTS[i];
        result += texture(uSource, vUV - offset).rgb * WEIGHTS[i];
    }
    FragColour = vec4(result, 1.0);
}
