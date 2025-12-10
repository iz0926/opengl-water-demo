#include "Waves.hpp"

const WaveParams kWaves[kNumWaves] = {
    {Vec3(1.0f, 0.0f, 0.3f), 0.12f, 6.0f, 0.8f, 1.2f},
    {Vec3(-0.8f, 0.0f, 0.6f), 0.08f, 4.0f, 0.7f, 1.4f},
    {Vec3(0.3f, 0.0f, -1.0f), 0.06f, 3.0f, 0.6f, 1.6f},
    {Vec3(-0.5f, 0.0f, -0.9f), 0.04f, 2.5f, 0.5f, 1.8f}
};

Vec3 evalGerstnerXZ(const Vec3 &xz, float time) {
    Vec3 pos(xz.x, 0.0f, xz.z);
    for (int i = 0; i < kNumWaves; ++i) {
        float k = 2.0f * kPi / kWaves[i].length;
        float c = kWaves[i].speed;
        Vec3 D = normalize(Vec3(kWaves[i].dir.x, 0.0f, kWaves[i].dir.z));
        float A = kWaves[i].amp;
        float Q = kWaves[i].steep;

        float phase = k * (D.x * xz.x + D.z * xz.z) + c * time;
        float cosP = std::cos(phase);
        float sinP = std::sin(phase);

        pos.x += (Q * A * D.x) * cosP;
        pos.z += (Q * A * D.z) * cosP;
        pos.y += A * sinP;
    }
    return pos;
}
