#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uHDRColor;
uniform float uThreshold;

void main() {
    vec3 color = texture(uHDRColor, vUv).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    vec3 bright = (brightness > uThreshold) ? color : vec3(0.0);
    fragColor = vec4(bright, 1.0);
}

