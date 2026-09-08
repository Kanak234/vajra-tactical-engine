#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in mat4 aModel;
layout (location = 7) in vec4 aAlbedoRoughness;
layout (location = 8) in vec4 aParams;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpace;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vLightSpacePos;
flat out vec4 vAlbedoRoughness;
flat out vec4 vParams;

void main() {
    vec4 world = aModel * vec4(aPosition, 1.0);
    vWorldPos  = world.xyz;
    vNormal    = mat3(transpose(inverse(aModel))) * aNormal;
    vUV        = aUV;
    vLightSpacePos = uLightSpace * world;
    vAlbedoRoughness = aAlbedoRoughness;
    vParams = aParams;
    gl_Position = uProjection * uView * world;
}
