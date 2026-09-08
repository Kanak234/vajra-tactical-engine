#version 460 core
in vec2 vUV;
out vec4 FragColour;

uniform sampler2D uSource;
uniform float uThreshold;
uniform float uSoftKnee;

void main() {
    vec3 colour = texture(uSource, vUV).rgb;
    float brightness = max(colour.r, max(colour.g, colour.b));

    // Soft knee: a hard cutoff makes bloom pop on and off as objects cross the
    // threshold. This ramps in over a window instead.
    float knee = uThreshold * uSoftKnee + 0.0001;
    float soft = clamp(brightness - uThreshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(soft, brightness - uThreshold) / max(brightness, 0.0001);

    FragColour = vec4(colour * contribution, 1.0);
}
