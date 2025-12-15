#pragma once

#include <array>
#include <cmath>

constexpr float kPi = 3.1415926535f;

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float xx, float yy) : x(xx), y(yy) {}
};

struct Vec3 {
    float x, y, z;
    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    constexpr Vec3 operator+(const Vec3 &o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    constexpr Vec3 operator-(const Vec3 &o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    constexpr Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 &operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}
};

float dot(const Vec3 &a, const Vec3 &b);
Vec3 cross(const Vec3 &a, const Vec3 &b);
Vec3 normalize(const Vec3 &v);
float length(const Vec3 &v);

struct Mat4 {
    std::array<float, 16> m{};

    static Mat4 identity();
    static Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ);
    static Mat4 lookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up);
    static Mat4 translate(const Vec3 &t);
    static Mat4 scale(const Vec3 &s);
    static Mat4 rotateX(float radians);
    static Mat4 rotateY(float radians);
    static Mat4 rotateZ(float radians);
    static Mat4 ortho(float left, float right, float bottom, float top, float nearZ, float farZ);
};

Mat4 operator*(const Mat4 &a, const Mat4 &b);
Vec4 operator*(const Mat4 &m, const Vec4 &v);
