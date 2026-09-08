#version 460 core
in vec2 vUV;
out float FragAO;

uniform sampler2D uDepth;
uniform sampler2D uNormals;
uniform sampler2D uNoise;

uniform mat4  uProjection;
uniform mat4  uInverseProjection;
uniform vec2  uNoiseScale;
uniform float uRadius;
uniform float uStrength;

const int KERNEL_SIZE = 24;
uniform vec3 uKernel[KERNEL_SIZE];

// Rebuild view-space position from the depth buffer rather than storing a
// position target — one less full-resolution RGBA16F buffer in memory.
vec3 viewPositionFromDepth(vec2 uv) {
    float depth = texture(uDepth, uv).r;
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = uInverseProjection * clip;
    return view.xyz / view.w;
}

void main() {
    float depth = texture(uDepth, vUV).r;
    if (depth >= 1.0) { FragAO = 1.0; return; }   // skybox / cleared pixels

    vec3 position = viewPositionFromDepth(vUV);
    vec3 normal   = normalize(texture(uNormals, vUV).xyz * 2.0 - 1.0);
    vec3 randomVec = normalize(vec3(texture(uNoise, vUV * uNoiseScale).xy, 0.0));

    // Gram-Schmidt: build a tangent basis rotated per-pixel by the noise tile,
    // which trades banding for high-frequency noise the blur can remove.
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        vec3 samplePos = position + TBN * uKernel[i] * uRadius;

        vec4 offset = uProjection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) continue;

        float sampleDepth = viewPositionFromDepth(offset.xy).z;

        // Range check stops distant geometry darkening near geometry across
        // a depth discontinuity — the classic SSAO halo artifact.
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(position.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + 0.025 ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE)) * uStrength;
    FragAO = clamp(occlusion, 0.0, 1.0);
}
