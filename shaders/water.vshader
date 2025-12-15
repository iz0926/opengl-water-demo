#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform mat4 uLightVP;
uniform float uTime;
uniform float uMove;
uniform int   uRippleCount;
uniform vec4  uRipples[32]; // xyz = center, w = start time

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUv;
out vec2 vDudvUv;
out float vCrest;
out vec4 vShadowCoord;

const float PI = 3.1415926535;
const int NUM_WAVES = 4;

const vec2 waveDir[NUM_WAVES] = vec2[NUM_WAVES](
    normalize(vec2( 1.0,  0.3)),
    normalize(vec2(-0.8,  0.6)),
    normalize(vec2( 0.3, -1.0)),
    normalize(vec2(-0.5, -0.9))
);

const float waveAmp[NUM_WAVES]     = float[NUM_WAVES](0.12, 0.08, 0.06, 0.04);
const float waveLength[NUM_WAVES] = float[NUM_WAVES](6.0, 4.0, 3.0, 2.5);
const float waveSteep[NUM_WAVES]  = float[NUM_WAVES](0.8, 0.7, 0.6, 0.5);
const float waveSpeed[NUM_WAVES]  = float[NUM_WAVES](1.2, 1.4, 1.6, 1.8);

float rippleHeight(vec2 posXZ, vec2 center, float age) {
    const float freq = 9.0;
    const float speed = 2.0;
    const float decay = 0.35;
    const float scale = 0.12;
    if (age < 0.0) return 0.0;
    vec2 d = posXZ - center;
    float r = length(d);
    float phase = freq * (r - speed * age);
    float envelope = exp(-decay * r) * exp(-0.18 * age);
    return scale * envelope * sin(phase);
}

vec3 evalGerstner(vec2 xz) {
    vec3 pos = vec3(xz.x, 0.0, xz.y);
    for (int i = 0; i < NUM_WAVES; ++i) {
        float k = 2.0 * PI / waveLength[i];
        float c = waveSpeed[i];
        vec2  D = waveDir[i];
        float A = waveAmp[i];
        float Q = waveSteep[i];

        float phase = k * dot(D, xz) + c * uTime;
        float cosP = cos(phase);
        float sinP = sin(phase);

        pos.x += (Q * A * D.x) * cosP;
        pos.z += (Q * A * D.y) * cosP;
        pos.y += A * sinP;
    }
    return pos;
}

void main() {
    vec2 xz = aPos.xz;
    vec3 p = evalGerstner(xz);

    const float eps = 0.15;
    vec3 px = evalGerstner(xz + vec2(eps, 0.0));
    vec3 pz = evalGerstner(xz + vec2(0.0, eps));

    // Add ring ripples from recent impacts
    float rippleY = 0.0;
    float rippleYx = 0.0;
    float rippleYz = 0.0;
    for (int i = 0; i < uRippleCount && i < 32; ++i) {
        vec2 center = uRipples[i].xz;
        float age = uTime - uRipples[i].w;
        rippleY  += rippleHeight(xz, center, age);
        rippleYx += rippleHeight(xz + vec2(eps, 0.0), center, age);
        rippleYz += rippleHeight(xz + vec2(0.0, eps), center, age);
    }
    p.y  += rippleY;
    px.y += rippleYx;
    pz.y += rippleYz;

    vec3 n  = normalize(cross(pz - p, px - p));

    float crest = pow(max(0.0, 1.0 - n.y), 3.0);

    vec4 worldPos = uModel * vec4(p, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal   = normalize(mat3(uModel) * n);

    // Higher tiling to shrink visible grid patterns in reflections (denser)
    vUv = xz * 0.1;
    vDudvUv = xz * 0.05 + vec2(uMove * 0.15, uMove * 0.11);
    vCrest = crest;

    vShadowCoord = uLightVP * worldPos;

    gl_Position = uViewProj * worldPos;
}
