#version 330 core

uniform vec3 uDeepColor;
uniform vec3 uLightDir;
uniform vec3 uEyePos;

uniform sampler2D uReflectionTex;
uniform mat4 uReflectionVP;

uniform sampler2D uSceneTex;
uniform sampler2D uSceneDepth;
uniform float uNear;
uniform float uFar;
uniform mat4 uViewProjScene;

uniform sampler2D uNormalMap;
uniform float uNormalScale;
uniform float uReflDistort;
uniform float uRefrDistort;

uniform sampler2D uDudvMap;
uniform float uMove;
uniform float uRoughness;
uniform float uFresnelBias;
uniform float uFresnelScale;
uniform vec3  uFoamColor;
uniform float uFoamIntensity;
uniform float uTime;

uniform sampler2D uShadowMap;
uniform mat4 uLightVP;
uniform int  uUnderwater;
uniform int  uRippleCount;
uniform vec4 uRipples[32]; // xyz = center, w = start time

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUv;
in vec2 vDudvUv;
in float vCrest;
in vec4 vShadowCoord;

out vec4 fragColor;

float linearizeDepth(float z) {
    return (2.0 * uNear) / (uFar + uNear - z * (uFar - uNear));
}

float rippleHeight(vec2 posXZ, vec2 center, float age) {
    const float freq = 9.0;
    const float speed = 2.0;
    const float decay = 0.35;
    const float scale = 0.12;
    if (age < 0.0) return 0.0;
    vec2 d = posXZ - center;
    float r = length(d);
    float phase = freq * (r - speed * age);
    float envelope = exp(-decay * r) * exp(-0.18 * max(0.0, age));
    return scale * envelope * sin(phase);
}

vec3 rippleNormal(vec2 posXZ, vec2 center, float age) {
    float eps = 0.1;
    float h0 = rippleHeight(posXZ, center, age);
    float hx = rippleHeight(posXZ + vec2(eps, 0.0), center, age);
    float hz = rippleHeight(posXZ + vec2(0.0, eps), center, age);
    vec3 dx = vec3(eps, hx - h0, 0.0);
    vec3 dz = vec3(0.0, hz - h0, eps);
    return normalize(cross(dz, dx));
}

float shadowFactor(vec4 shadowCoord) {
    vec3 proj = shadowCoord.xyz / shadowCoord.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0)
        return 1.0;
    float closest = texture(uShadowMap, proj.xy).r;
    float current = proj.z;
    float bias = 0.002;
    float shadow = current - bias > closest ? 0.5 : 1.0;
    return shadow;
}

void main() {
    vec3 baseN = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uEyePos - vWorldPos);

    float fresnelBase = uFresnelBias + uFresnelScale * pow(1.0 - max(dot(baseN, V), 0.0), 3.0);
    vec3 waterTint = mix(uDeepColor, vec3(0.45, 0.65, 0.85), fresnelBase + 0.1);

    vec2 dudv = texture(uDudvMap, vDudvUv).rg * 2.0 - 1.0;
    dudv *= 0.03;

    // Reflection UV
    vec4 clipRefl = uReflectionVP * vec4(vWorldPos, 1.0);
    vec3 ndcRefl = clipRefl.xyz / max(clipRefl.w, 0.0001);
    vec2 uvRefl = ndcRefl.xy * 0.5 + 0.5;
    uvRefl.y = 1.0 - uvRefl.y;
    uvRefl += dudv * uReflDistort;
    uvRefl = clamp(uvRefl, vec2(0.002), vec2(0.998));

    // Refraction UV
    vec4 clipScene = uViewProjScene * vec4(vWorldPos, 1.0);
    vec2 uvScene = clipScene.xy / clipScene.w * 0.5 + 0.5;
    uvScene.y = 1.0 - uvScene.y;
    uvScene += dudv * uRefrDistort;
    uvScene = clamp(uvScene, vec2(0.002), vec2(0.998));

    vec3 sceneCol = texture(uSceneTex, uvScene).rgb;
    float sceneDepth = linearizeDepth(texture(uSceneDepth, uvScene).r);

    float ndcZWater = clipScene.z / clipScene.w * 0.5 + 0.5;
    float surfaceDepth = linearizeDepth(ndcZWater);
    float waterDepth = max(sceneDepth - surfaceDepth, 0.0);
    float depthNorm = clamp(waterDepth * 0.6, 0.0, 1.0);

    vec3 refractedBase = mix(sceneCol, uDeepColor, depthNorm);

    // Two-scale normal detail
    vec3 mapN1 = texture(uNormalMap, vUv * 1.0 + vec2(uMove * 0.05, 0.0)).xyz * 2.0 - 1.0;
    vec3 mapN2 = texture(uNormalMap, vUv * 2.3 + vec2(0.0, uMove * 0.07)).xyz * 2.0 - 1.0;
    vec3 mapN = normalize(mix(mapN1, mapN2, 0.5));
    mapN.xy *= uNormalScale;
    mapN = normalize(mapN);

    vec3 upVec = (abs(baseN.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
   vec3 T = normalize(cross(upVec, baseN));
   vec3 B = cross(baseN, T);
   vec3 N = normalize(T * mapN.x + baseN * mapN.y + B * mapN.z);

    // Impact-driven ring ripples
    vec2 posXZ = vWorldPos.xz;
    vec3 rippleN = N;
    float rippleMix = 0.0;
    for (int i = 0; i < uRippleCount && i < 32; ++i) {
        float age = uTime - uRipples[i].w;
        if (age < 0.0 || age > 8.0) continue;
        vec2 center = uRipples[i].xz;
        float h = rippleHeight(posXZ, center, age);
        float w = clamp(abs(h) * 25.0, 0.0, 1.0);
        if (w > rippleMix) {
            rippleMix = w;
            rippleN = rippleNormal(posXZ, center, age);
        }
    }
    N = normalize(mix(N, rippleN, rippleMix));

    float diffN = max(dot(N, normalize(-uLightDir)), 0.0);
    float fresnelN = uFresnelBias + uFresnelScale * pow(1.0 - max(dot(N, V), 0.0), 3.0);

    float sh = shadowFactor(vShadowCoord);

    vec3 litWater = waterTint * (0.25 + diffN * sh);

    float NoV = max(dot(N, V), 0.0);
    float angleFactor = 1.0 - NoV;
    float maxRadius = 0.02;
    float baseRadius = 0.002;
    float blurRadius = (baseRadius + maxRadius * uRoughness) * angleFactor;

    vec3 reflCenter = texture(uReflectionTex, uvRefl).rgb;
    vec3 refl = reflCenter;
    if (blurRadius > 0.0001) {
        vec2 offX = vec2(blurRadius, 0.0);
        vec2 offY = vec2(0.0, blurRadius);
        vec3 sum = reflCenter;
        sum += texture(uReflectionTex, clamp(uvRefl + offX, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uReflectionTex, clamp(uvRefl - offX, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uReflectionTex, clamp(uvRefl + offY, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uReflectionTex, clamp(uvRefl - offY, vec2(0.002), vec2(0.998))).rgb;
        refl = sum / 5.0;
    }

    float refrBlurRadius = blurRadius * 0.4;
    vec3 refrCol = refractedBase;
    if (refrBlurRadius > 0.0001) {
        vec2 offX = vec2(refrBlurRadius, 0.0);
        vec2 offY = vec2(0.0, refrBlurRadius);
        vec3 sum = refractedBase;
        sum += texture(uSceneTex, clamp(uvScene + offX, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uSceneTex, clamp(uvScene - offX, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uSceneTex, clamp(uvScene + offY, vec2(0.002), vec2(0.998))).rgb;
        sum += texture(uSceneTex, clamp(uvScene - offY, vec2(0.002), vec2(0.998))).rgb;
        refrCol = sum / 5.0;
    }

    float reflectMix = clamp(fresnelN + 0.25, 0.0, 1.0) * (1.0 - 0.5 * uRoughness);
    vec3 color = mix(litWater, refl, reflectMix);

    float refrMix = 0.35 + 0.2 * uRoughness;
    color = mix(color, refrCol, refrMix);

    float tilt = 1.0 - abs(dot(N, vec3(0.0, 1.0, 0.0)));
    float foamTilt = smoothstep(0.1, 0.35, tilt);
    float shallow = 1.0 - depthNorm;
    float foamDepthMask = smoothstep(0.2, 0.9, shallow);
    float foamCrest = smoothstep(0.15, 0.5, vCrest);

    float foamNoise = texture(uDudvMap, vUv * 2.0 + vec2(uMove * 0.5, uMove * 0.3)).r;
    foamNoise = foamNoise * 2.0 - 1.0;
    foamNoise = clamp(foamNoise * 0.5 + 0.5, 0.0, 1.0);

    float foamAmount = (foamTilt * 0.5 + foamDepthMask * 0.4 + foamCrest * 1.1) *
                       foamNoise * uFoamIntensity;
    foamAmount = clamp(foamAmount, 0.0, 1.0);
    color = mix(color, uFoamColor, foamAmount);

    float sss = pow(max(dot(-L, V), 0.0), 3.0);
    vec3 sssColor = vec3(0.7, 0.9, 0.7);
    float sssStrength = (1.0 - depthNorm) * 0.6;
    color += sssColor * sss * sssStrength * sh;

    float alpha = mix(0.4, 0.9, depthNorm);
    alpha = clamp(alpha, 0.4, 0.95);
    if (uUnderwater == 1) {
        alpha *= 0.4;
    }

    fragColor = vec4(color, alpha);
}
