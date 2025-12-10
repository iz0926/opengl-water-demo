#pragma once

#include "Math.hpp"

constexpr int kNumWaves = 4;

struct WaveParams {
    Vec3 dir; // x,z used
    float amp;
    float length;
    float steep;
    float speed;
};

extern const WaveParams kWaves[kNumWaves];

Vec3 evalGerstnerXZ(const Vec3 &xz, float time);
