#pragma once

#include <string>
#include <vector>
#include "GL/glew.h"
#include "Math.hpp"

std::string readFile(const std::string &path);
GLuint compileShader(GLenum type, const std::string &source);
GLuint linkProgram(GLuint vs, GLuint fs);

struct Framebuffer {
    GLuint fbo = 0;
    GLuint colorTex = 0;
    GLuint depthRbo = 0;
    GLuint depthTex = 0;
    int width = 0;
    int height = 0;
};

Framebuffer makeReflectionBuffer(int width, int height);
Framebuffer makeSceneBuffer(int width, int height);
Framebuffer makeHdrBuffer(int width, int height);
Framebuffer makeBloomBuffer(int width, int height);
Framebuffer makeLdrBuffer(int width, int height);
void destroyFramebuffer(Framebuffer &fb);

struct ShadowMap {
    GLuint fbo = 0;
    GLuint depthTex = 0;
    int width = 0;
    int height = 0;
};

ShadowMap makeShadowMap(int size);
void destroyShadowMap(ShadowMap &sm);

GLuint createWaterNormalMap(int size, float freq);
GLuint createDudvTexture(int size);
