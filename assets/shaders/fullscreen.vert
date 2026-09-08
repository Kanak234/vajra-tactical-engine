#version 460 core
// Attributeless fullscreen triangle. One oversized triangle instead of a quad
// avoids the diagonal seam where the two halves meet and saves a draw of
// redundant fragments.
out vec2 vUV;
void main() {
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
