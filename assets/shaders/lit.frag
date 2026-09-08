#version 460 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vLightSpacePos;
flat in vec4 vAlbedoRoughness;
flat in vec4 vParams;

uniform vec3  uCameraPos;
uniform vec3  uLightDir;
uniform vec3  uLightColour;
uniform vec3  uFogColour;
uniform vec2  uScreenSize;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uAmbientOcclusion;
uniform int   uUseAo;

out vec4 FragColour;

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return geometrySchlickGGX(max(dot(N, V), 0.0), roughness)
         * geometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float shadowFactor(vec3 N, vec3 L) {
    vec3 proj = vLightSpacePos.xyz / vLightSpacePos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    float bias = max(0.0016 * (1.0 - dot(N, L)), 0.00035);
    float sum = 0.0;
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            sum += texture(uShadowMap, vec3(proj.xy + vec2(x, y) * texel, proj.z - bias));
    return sum / 9.0;
}

void main() {
    vec3  albedo    = vAlbedoRoughness.rgb;
    float roughness = clamp(vAlbedoRoughness.a, 0.04, 1.0);
    float emissive  = vParams.x;

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04);

    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) /
                    (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
    vec3 kD = vec3(1.0) - F;
    float NdotL = max(dot(N, L), 0.0);

    float shadow = shadowFactor(N, L);
    vec3 radiance = uLightColour * 3.2 * shadow;
    vec3 direct = (kD * albedo / PI + specular) * radiance * NdotL;

    // Screen-space ambient occlusion only attenuates ambient light. Applying
    // it to direct light too is a common mistake and looks like dirt.
    float ao = 1.0;
    if (uUseAo == 1)
        ao = texture(uAmbientOcclusion, gl_FragCoord.xy / uScreenSize).r;

    vec3 ambient = albedo * mix(vec3(0.030, 0.036, 0.050),
                                vec3(0.100, 0.110, 0.135),
                                N.y * 0.5 + 0.5) * ao;

    vec3 colour = direct + ambient + albedo * emissive;

    float dist = length(uCameraPos - vWorldPos);
    float fog = 1.0 - exp(-dist * 0.009);
    colour = mix(colour, uFogColour * 2.4, clamp(fog, 0.0, 0.88));

    // No tonemapping here: the composite pass owns that, so this stays HDR.
    FragColour = vec4(colour, 1.0);
}
