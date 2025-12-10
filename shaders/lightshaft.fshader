#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uDepth;
uniform vec2 uSunScreenPos;
uniform float uDecay;
uniform float uDensity;
uniform float uWeight;
uniform float uExposure;
uniform int uUnderwater;

// Reconstruct a simple radial blur toward the sun position, attenuated by depth
void main() {
    vec2 texCoord = vUv;
    vec2 delta = (uSunScreenPos - texCoord) * uDensity;

    float illuminationDecay = 1.0;
    vec3 color = vec3(0.0);

    const int NUM_SAMPLES = 40;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        texCoord += delta;
        // stop if outside screen
        if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0)
            break;
        float depth = texture(uDepth, texCoord).r;
        float occlusion = depth < 1.0 ? 0.0 : 1.0;
        vec3 sample = vec3(occlusion);
        sample *= illuminationDecay * uWeight;
        color += sample;
        illuminationDecay *= uDecay;
    }

    // Softer underwater beams
    if (uUnderwater == 1) {
        color *= vec3(0.6, 0.8, 1.0);
    } else {
        color *= vec3(1.0, 0.95, 0.85);
    }

    fragColor = vec4(color * uExposure, 1.0);
}
