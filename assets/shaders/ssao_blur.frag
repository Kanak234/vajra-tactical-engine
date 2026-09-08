#version 460 core
in vec2 vUV;
out float FragAO;

uniform sampler2D uSource;
uniform vec2 uTexelSize;

void main() {
    // 4x4 box blur exactly matching the noise tile size, which cancels the
    // rotation pattern completely.
    float sum = 0.0;
    for (int y = -2; y < 2; ++y)
        for (int x = -2; x < 2; ++x)
            sum += texture(uSource, vUV + vec2(x, y) * uTexelSize).r;
    FragAO = sum / 16.0;
}
