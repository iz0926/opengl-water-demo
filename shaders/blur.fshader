#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uImage;
uniform int uHorizontal;
uniform vec2 uTexelSize;

void main() {
    vec2 dir = (uHorizontal == 1) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(uImage, vUv).rgb * weights[0];

    for (int i = 1; i < 5; ++i) {
        vec2 offset = dir * uTexelSize * float(i);
        result += texture(uImage, vUv + offset).rgb * weights[i];
        result += texture(uImage, vUv - offset).rgb * weights[i];
    }

    fragColor = vec4(result, 1.0);
}

