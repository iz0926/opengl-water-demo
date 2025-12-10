#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform vec3 uTopColor;
uniform vec3 uHorizonColor;
uniform int  uUnderwater;

void main() {
    float t = clamp(vUv.y, 0.0, 1.0);

    if (uUnderwater == 1) {
        // Underwater gradient
        vec3 deepColor  = vec3(0.02, 0.08, 0.12);
        vec3 midColor   = vec3(0.04, 0.20, 0.30);
        vec3 surfaceCol = vec3(0.15, 0.40, 0.55);

        float y = clamp((t - 0.2) / 0.8, 0.0, 1.0);
        vec3 col = mix(deepColor, midColor, y);
        col = mix(col, surfaceCol, smoothstep(0.6, 1.0, y));

        fragColor = vec4(col, 1.0);
    } else {
        vec3 col = mix(uHorizonColor, uTopColor, t);
        fragColor = vec4(col, 1.0);
    }
}

