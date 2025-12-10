#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vShadowCoord;
in float vHeight;

out vec4 fragColor;

uniform vec3 uColor;
uniform vec3 uLightDir;   // direction light travels (world)
uniform vec3 uEyePos;

uniform float uWaterHeight;
uniform int   uUnderwater;

// Fog controls
uniform vec3  uFogColorAbove;
uniform vec3  uFogColorBelow;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uUnderFogDensity;

// Shadow map
uniform sampler2D uShadowMap;

// Time for caustics
uniform float uTime;

float shadowFactor(vec4 shadowCoord) {
    vec3 proj = shadowCoord.xyz / shadowCoord.w;
    proj = proj * 0.5 + 0.5;

    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    float closest = texture(uShadowMap, proj.xy).r;
    float current = proj.z;
    float bias = 0.0015;
    float shadow = current - bias > closest ? 0.4 : 1.0;
    return shadow;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uEyePos - vWorldPos);

    float NdotL = max(dot(N, L), 0.0);
    float ambient = 0.25;

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    vec3 rimColor = vec3(0.6, 0.7, 1.0) * rim * 0.2;

    float shadow = shadowFactor(vShadowCoord);

    vec3 base = uColor;
    vec3 lit = base * (ambient + NdotL * shadow) + rimColor * shadow;
    vec3 color = lit;

    // Caustics: only below water, near surface
    if (vHeight < uWaterHeight + 0.5) {
        float depthBelow = uWaterHeight - vHeight;
        if (depthBelow > 0.0 && depthBelow < 4.0) {
            vec2 p = vWorldPos.xz * 1.2;
            float t = uTime * 1.4;
            float c0 = sin(p.x * 3.2 + t) * cos(p.y * 4.1 - t * 1.3);
            float c1 = sin(p.x * 5.7 - t * 0.9) * sin(p.y * 6.3 + t * 1.7);
            float caustic = c0 + c1;
            caustic = caustic * 0.5 + 0.5;
            caustic = pow(caustic, 4.0);

            float depthFade = 1.0 - clamp(depthBelow / 4.0, 0.0, 1.0);
            float intensity = 2.0 * depthFade;

            vec3 causticColor = vec3(1.6, 1.4, 1.0);
            color += causticColor * caustic * intensity;
        }
    }

    // Fog
    float dist = length(vWorldPos - uEyePos);
    vec3 finalColor = color;

    if (uUnderwater == 1) {
        float depthBelowCam = uWaterHeight - uEyePos.y;
        depthBelowCam = max(depthBelowCam, 0.0);
        float depthFactor = 1.0 + depthBelowCam * 0.4;

        float density = uUnderFogDensity * depthFactor;
        float fogAmount = 1.0 - exp(-density * dist);

        vec3 fogColor = mix(uFogColorBelow, uFogColorAbove,
                            clamp((vHeight - (uWaterHeight - 4.0)) / 4.0, 0.0, 1.0));
        finalColor = mix(color, fogColor, clamp(fogAmount, 0.0, 1.0));
    } else {
        float fogFactor = clamp((uFogEnd - dist) / (uFogEnd - uFogStart), 0.0, 1.0);
        float heightBlend = clamp((vHeight - (uWaterHeight - 5.0)) / 15.0, 0.0, 1.0);
        vec3 fogColor = mix(uFogColorBelow, uFogColorAbove, heightBlend);
        finalColor = mix(fogColor, color, fogFactor);
    }

    fragColor = vec4(finalColor, 1.0);
}
