#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in mat4 aModel;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vViewNormal;

void main() {
    mat4 modelView = uView * aModel;
    // Normal matrix in view space. inverse/transpose is fine here: the prepass
    // is cheap and non-uniform scale is common in a box-built level.
    vViewNormal = normalize(mat3(transpose(inverse(modelView))) * aNormal);
    gl_Position = uProjection * modelView * vec4(aPosition, 1.0);
}
