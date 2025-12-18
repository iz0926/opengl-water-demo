#include "Mesh.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

Mesh makeMesh(const std::vector<float> &interleavedPosNormal, bool hasTexcoord) {
    Mesh mesh{};
    const int stride = hasTexcoord ? 8 : 6;
    mesh.vertexCount = static_cast<GLsizei>(interleavedPosNormal.size() / stride);
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleavedPosNormal.size() * sizeof(float),
                 interleavedPosNormal.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                          reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                          reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    if (hasTexcoord) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                              reinterpret_cast<void *>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

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

Mesh loadObjMesh(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open OBJ: " + path);
    }
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<float> verts;

    auto pushVertex = [&](int vIdx, int nIdx, int tIdx) {
        Vec3 p = positions.at(vIdx - 1);
        Vec3 n = nIdx > 0 ? normals.at(nIdx - 1) : Vec3(0.0f, 1.0f, 0.0f);
        Vec2 uv(0.0f, 0.0f);
        if (tIdx > 0 && tIdx - 1 < static_cast<int>(uvs.size())) {
            uv = uvs[tIdx - 1];
        }
        verts.push_back(p.x); verts.push_back(p.y); verts.push_back(p.z);
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        verts.push_back(uv.x); verts.push_back(uv.y);
    };

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "v") {
            Vec3 p;
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vn") {
            Vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(normalize(n));
        } else if (tag == "vt") {
            Vec2 t;
            iss >> t.x >> t.y;
            uvs.push_back(t);
        } else if (tag == "f") {
            std::vector<std::string> f;
            std::string token;
            while (iss >> token) f.push_back(token);
            auto parseTriplet = [&](const std::string &s, int &vIdx, int &tIdx, int &nIdx) {
                vIdx = nIdx = tIdx = 0;
                size_t firstSlash = s.find('/');
                size_t secondSlash = s.find('/', firstSlash + 1);
                vIdx = std::stoi(s.substr(0, firstSlash));
                if (secondSlash != std::string::npos && secondSlash > firstSlash) {
                    std::string tstr = s.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                    if (!tstr.empty()) tIdx = std::stoi(tstr);
                    std::string nstr = s.substr(secondSlash + 1);
                    if (!nstr.empty()) nIdx = std::stoi(nstr);
                }
            };
            auto emitTri = [&](int a, int b, int c, int ta, int tb, int tc, int na, int nb, int nc) {
                pushVertex(a, na, ta);
                pushVertex(b, nb, tb);
                pushVertex(c, nc, tc);
            };
            if (f.size() == 3) {
                int v[3], t[3], n[3];
                for (int i = 0; i < 3; ++i) parseTriplet(f[i], v[i], t[i], n[i]);
                emitTri(v[0], v[1], v[2], t[0], t[1], t[2], n[0], n[1], n[2]);
            } else if (f.size() == 4) {
                int v[4], t[4], n[4];
                for (int i = 0; i < 4; ++i) parseTriplet(f[i], v[i], t[i], n[i]);
                emitTri(v[0], v[1], v[2], t[0], t[1], t[2], n[0], n[1], n[2]);
                emitTri(v[0], v[2], v[3], t[0], t[2], t[3], n[0], n[2], n[3]);
            }
        }
    }
    return makeMesh(verts, true);
}
