#include "Mesh.hpp"

Mesh makeMesh(const std::vector<float> &interleavedPosNormal) {
    Mesh mesh{};
    mesh.vertexCount = static_cast<GLsizei>(interleavedPosNormal.size() / 6);
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleavedPosNormal.size() * sizeof(float),
                 interleavedPosNormal.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return mesh;
}

void destroyMesh(Mesh &m) {
    if (m.vbo) glDeleteBuffers(1, &m.vbo);
    if (m.vao) glDeleteVertexArrays(1, &m.vao);
    m = Mesh{};
}

Mesh makeGroundMesh(float halfSize) {
    std::vector<float> groundData = {
        -halfSize, 0.0f, -halfSize, 0, 1, 0,
        -halfSize, 0.0f,  halfSize, 0, 1, 0,
         halfSize, 0.0f,  halfSize, 0, 1, 0,

        -halfSize, 0.0f, -halfSize, 0, 1, 0,
         halfSize, 0.0f,  halfSize, 0, 1, 0,
         halfSize, 0.0f, -halfSize, 0, 1, 0
    };
    return makeMesh(groundData);
}

Mesh makeCubeMesh() {
    std::vector<float> cubeData = {
        // +X
        0.5f, -0.5f, -0.5f, 1, 0, 0,
        0.5f,  0.5f, -0.5f, 1, 0, 0,
        0.5f,  0.5f,  0.5f, 1, 0, 0,
        0.5f, -0.5f, -0.5f, 1, 0, 0,
        0.5f,  0.5f,  0.5f, 1, 0, 0,
        0.5f, -0.5f,  0.5f, 1, 0, 0,
        // -X
        -0.5f, -0.5f,  0.5f, -1, 0, 0,
        -0.5f,  0.5f,  0.5f, -1, 0, 0,
        -0.5f,  0.5f, -0.5f, -1, 0, 0,
        -0.5f, -0.5f,  0.5f, -1, 0, 0,
        -0.5f,  0.5f, -0.5f, -1, 0, 0,
        -0.5f, -0.5f, -0.5f, -1, 0, 0,
        // +Y
        -0.5f, 0.5f, -0.5f, 0, 1, 0,
         0.5f, 0.5f, -0.5f, 0, 1, 0,
         0.5f, 0.5f,  0.5f, 0, 1, 0,
        -0.5f, 0.5f, -0.5f, 0, 1, 0,
         0.5f, 0.5f,  0.5f, 0, 1, 0,
        -0.5f, 0.5f,  0.5f, 0, 1, 0,
        // -Y
        -0.5f, -0.5f,  0.5f, 0, -1, 0,
         0.5f, -0.5f,  0.5f, 0, -1, 0,
         0.5f, -0.5f, -0.5f, 0, -1, 0,
        -0.5f, -0.5f,  0.5f, 0, -1, 0,
         0.5f, -0.5f, -0.5f, 0, -1, 0,
        -0.5f, -0.5f, -0.5f, 0, -1, 0,
        // +Z
        -0.5f, -0.5f, 0.5f, 0, 0, 1,
         0.5f, -0.5f, 0.5f, 0, 0, 1,
         0.5f,  0.5f, 0.5f, 0, 0, 1,
        -0.5f, -0.5f, 0.5f, 0, 0, 1,
         0.5f,  0.5f, 0.5f, 0, 0, 1,
        -0.5f,  0.5f, 0.5f, 0, 0, 1,
        // -Z
        -0.5f,  0.5f, -0.5f, 0, 0, -1,
         0.5f,  0.5f, -0.5f, 0, 0, -1,
         0.5f, -0.5f, -0.5f, 0, 0, -1,
        -0.5f,  0.5f, -0.5f, 0, 0, -1,
         0.5f, -0.5f, -0.5f, 0, 0, -1,
        -0.5f, -0.5f, -0.5f, 0, 0, -1
    };
    return makeMesh(cubeData);
}
