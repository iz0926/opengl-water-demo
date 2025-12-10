#pragma once

#include <vector>
#include "GL/glew.h"
#include "Math.hpp"

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
};

Mesh makeMesh(const std::vector<float> &interleavedPosNormal);
void destroyMesh(Mesh &m);

Mesh makeGroundMesh(float halfSize);
Mesh makeCubeMesh();
