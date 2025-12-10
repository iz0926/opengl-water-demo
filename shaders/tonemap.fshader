#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uHDRColor;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uBloomStrength;
uniform float uGamma;

void main() {
    vec3 hdrColor = texture(uHDRColor, vUv).rgb;
    vec3 bloomColor = texture(uBloom, vUv).rgb;

    hdrColor += bloomColor * uBloomStrength;

    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    mapped = vec3(1.0) - exp(-mapped * uExposure);

    // Color grading: cooler shadows, warmer highs
    float luma = max(max(mapped.r, mapped.g), mapped.b);
    vec3 shadows = pow(mapped, vec3(1.2));
    vec3 highlights = mapped;
    mapped = mix(shadows * vec3(0.95, 1.02, 1.05),
                 highlights * vec3(1.05, 1.0, 0.95),
                 smoothstep(0.3, 0.8, luma));

    // Vignette
    vec2 uv = vUv * 2.0 - 1.0;
    float r = dot(uv, uv);
    float vignette = smoothstep(1.0, 0.3, r);
    mapped *= vignette * 0.1 + 0.9;

    mapped = pow(mapped, vec3(1.0 / uGamma));

    fragColor = vec4(mapped, 1.0);
}
