#include "Math.hpp"

float dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

Vec3 normalize(const Vec3 &v) {
    const float len2 = dot(v, v);
    if (len2 <= 0.0f) return Vec3();
    const float invLen = 1.0f / std::sqrt(len2);
    return v * invLen;
}

float length(const Vec3 &v) {
    return std::sqrt(dot(v, v));
}

Mat4 Mat4::identity() {
    Mat4 r;
    r.m = {1, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 1, 0,
           0, 0, 0, 1};
    return r;
}

Mat4 Mat4::perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
    const float f = 1.0f / std::tan(fovYRadians * 0.5f);
    Mat4 r{};
    r.m = {f / aspect, 0, 0, 0,
           0, f, 0, 0,
           0, 0, (farZ + nearZ) / (nearZ - farZ), -1.0f,
           0, 0, (2.0f * farZ * nearZ) / (nearZ - farZ), 0};
    return r;
}

Mat4 Mat4::lookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up) {
    const Vec3 f = normalize(target - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 r = Mat4::identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);
    return r;
}

Mat4 Mat4::translate(const Vec3 &t) {
    Mat4 r = Mat4::identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

Mat4 Mat4::scale(const Vec3 &s) {
    Mat4 r = Mat4::identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

Mat4 Mat4::ortho(float left, float right, float bottom, float top, float nearZ, float farZ) {
    Mat4 r{};
    r.m = {
        2.0f / (right - left), 0, 0, 0,
        0, 2.0f / (top - bottom), 0, 0,
        0, 0, -2.0f / (farZ - nearZ), 0,
        -(right + left) / (right - left),
        -(top + bottom) / (top - bottom),
        -(farZ + nearZ) / (farZ - nearZ),
        1.0f
    };
    return r;
}

Mat4 operator*(const Mat4 &a, const Mat4 &b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c) {
        for (int rIdx = 0; rIdx < 4; ++rIdx) {
            r.m[c * 4 + rIdx] =
                a.m[0 * 4 + rIdx] * b.m[c * 4 + 0] +
                a.m[1 * 4 + rIdx] * b.m[c * 4 + 1] +
                a.m[2 * 4 + rIdx] * b.m[c * 4 + 2] +
                a.m[3 * 4 + rIdx] * b.m[c * 4 + 3];
        }
    }
    return r;
}

Vec4 operator*(const Mat4 &m, const Vec4 &v) {
    Vec4 r;
    r.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
    r.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
    r.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
    r.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
    return r;
}
