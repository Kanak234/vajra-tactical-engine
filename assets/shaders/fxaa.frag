#version 460 core
in vec2 vUV;
out vec4 FragColour;

uniform sampler2D uSource;
uniform vec2 uTexelSize;

const float SPAN_MAX   = 8.0;
const float REDUCE_MUL = 1.0 / 8.0;
const float REDUCE_MIN = 1.0 / 128.0;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec3 rgbNW = texture(uSource, vUV + vec2(-1.0, -1.0) * uTexelSize).rgb;
    vec3 rgbNE = texture(uSource, vUV + vec2( 1.0, -1.0) * uTexelSize).rgb;
    vec3 rgbSW = texture(uSource, vUV + vec2(-1.0,  1.0) * uTexelSize).rgb;
    vec3 rgbSE = texture(uSource, vUV + vec2( 1.0,  1.0) * uTexelSize).rgb;
    vec3 rgbM  = texture(uSource, vUV).rgb;

    float lumaNW = luma(rgbNW), lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW), lumaSE = luma(rgbSE), lumaM = luma(rgbM);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Flat areas are left untouched, so texture detail survives.
    if (lumaMax - lumaMin < lumaMax * 0.0312) { FragColour = vec4(rgbM, 1.0); return; }

    vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                     ((lumaNW + lumaSW) - (lumaNE + lumaSE)));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * REDUCE_MUL, REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * uTexelSize;

    vec3 rgbA = 0.5 * (texture(uSource, vUV + dir * (1.0 / 3.0 - 0.5)).rgb +
                       texture(uSource, vUV + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(uSource, vUV - dir * 0.5).rgb +
                                     texture(uSource, vUV + dir * 0.5).rgb);

    float lumaB = luma(rgbB);
    FragColour = vec4((lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB, 1.0);
}
