#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uImage;
uniform vec2 uTexelSize;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec3 c  = texture(uImage, vUv).rgb;
    vec3 cN = texture(uImage, vUv + vec2(0.0, -uTexelSize.y)).rgb;
    vec3 cS = texture(uImage, vUv + vec2(0.0,  uTexelSize.y)).rgb;
    vec3 cE = texture(uImage, vUv + vec2( uTexelSize.x, 0.0)).rgb;
    vec3 cW = texture(uImage, vUv + vec2(-uTexelSize.x, 0.0)).rgb;

    float l  = luminance(c);
    float lN = luminance(cN);
    float lS = luminance(cS);
    float lE = luminance(cE);
    float lW = luminance(cW);

    float edgeH = abs(lW - lE);
    float edgeV = abs(lN - lS);
    float edge = edgeH + edgeV;

    const float threshold = 0.15;
    if (edge < threshold) {
        fragColor = vec4(c, 1.0);
        return;
    }

    vec3 avg = (cN + cS + cE + cW + c) / 5.0;
    fragColor = vec4(avg, 1.0);
}
