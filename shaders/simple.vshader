#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform mat4 uLightVP;
uniform float uClipY;
uniform int   uUseClip;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vShadowCoord;
out float vHeight;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal   = mat3(uModel) * aNormal;
    vShadowCoord = uLightVP * worldPos;
    vHeight = worldPos.y;

    if (uUseClip == 1) {
        gl_ClipDistance[0] = worldPos.y - uClipY;
    } else {
        gl_ClipDistance[0] = 0.0;
    }

    gl_Position = uViewProj * worldPos;
}

