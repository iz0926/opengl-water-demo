#version 330 core

in vec2 vUv;
out vec4 fragColor;

uniform vec3 uTopColor;
uniform vec3 uHorizonColor;
uniform int  uUnderwater;
uniform float uSunHeight;

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
        // Bias colors by sun height: warmer near horizon/low sun, cooler at zenith
        float warm = smoothstep(1.0, 0.3, uSunHeight); // more warm when sun is low
        vec3 warmHorizon = vec3(0.95, 0.55, 0.3);
        vec3 warmTop     = vec3(0.25, 0.15, 0.35);
        vec3 horizonCol  = mix(uHorizonColor, warmHorizon, warm);
        vec3 topCol      = mix(uTopColor, warmTop, warm * 0.6);

        // Slightly deepen the zenith at night
        float night = smoothstep(0.35, 0.0, uSunHeight);
        vec3 nightTint = vec3(0.05, 0.08, 0.15);
        topCol = mix(topCol, nightTint, night);
        horizonCol = mix(horizonCol, nightTint, night * 0.5);

        vec3 col = mix(horizonCol, topCol, t);
        fragColor = vec4(col, 1.0);
    }
}
