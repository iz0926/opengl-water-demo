#include "Stone.hpp"
#include <cstdlib>
#include <cmath>

Stone g_stones[kMaxStones];
RippleEvent g_ripples[kMaxRipples];
static int g_rippleWriteIndex = 0;

inline float singleRippleHeight(float r, float age) {
    if (age < 0.0f || age > 8.0f) return 0.0f;
    const float freq = 9.0f;
    const float speed = 2.0f;
    const float decay = 0.35f;
    const float scale = 0.12f;
    float phase = freq * (r - speed * age);
    float envelope = std::exp(-decay * r) * std::exp(-0.18f * age);
    return scale * envelope * std::sin(phase);
}

void addRipple(const Vec3 &pos, float time) {
    RippleEvent &r = g_ripples[g_rippleWriteIndex];
    r.pos = pos;
    r.startTime = time;
    r.active = true;
    g_rippleWriteIndex = (g_rippleWriteIndex + 1) % kMaxRipples;
}

void pruneRipples(float time) {
    const float kLife = 8.0f;
    for (int i = 0; i < kMaxRipples; ++i) {
        if (!g_ripples[i].active) continue;
        if (time - g_ripples[i].startTime > kLife) {
            g_ripples[i].active = false;
        }
    }
}

float rippleFieldHeight(const Vec3 &pos, float time) {
    float h = 0.0f;
    for (int i = 0; i < kMaxRipples; ++i) {
        if (!g_ripples[i].active) continue;
        float age = time - g_ripples[i].startTime;
        if (age < 0.0f || age > 8.0f) continue;
        Vec3 d(pos.x - g_ripples[i].pos.x, 0.0f, pos.z - g_ripples[i].pos.z);
        float r = std::sqrt(d.x * d.x + d.z * d.z);
        h += singleRippleHeight(r, age);
    }
    return h;
}

Vec3 rippleFieldGrad(const Vec3 &pos, float time) {
    const float eps = 0.1f;
    float h0 = rippleFieldHeight(pos, time);
    float hx = rippleFieldHeight(Vec3(pos.x + eps, pos.y, pos.z), time);
    float hz = rippleFieldHeight(Vec3(pos.x, pos.y, pos.z + eps), time);
    return Vec3((hx - h0) / eps, 0.0f, (hz - h0) / eps);
}

void spawnStone(const Vec3 &cameraPos, const Vec3 &forward, const Vec3 &up,
                float angleDownDeg, float speed) {
    int idx = -1;
    for (int i = 0; i < kMaxStones; ++i) {
        if (!g_stones[i].active) { idx = i; break; }
    }
    if (idx < 0) {
        float oldestLife = -1.0f;
        for (int i = 0; i < kMaxStones; ++i) {
            if (g_stones[i].life > oldestLife) {
                oldestLife = g_stones[i].life;
                idx = i;
            }
        }
    }

    Stone &s = g_stones[idx];
    s.active = true;
    s.bounces = 0;
    s.life = 0.0f;

    Vec3 forwardFlat(forward.x, 0.0f, forward.z);
    if (length(forwardFlat) < 1e-3f) {
        forwardFlat = Vec3(0.0f, 0.0f, -1.0f);
    }
    forwardFlat = normalize(forwardFlat);

    float angleDown = angleDownDeg * (kPi / 180.0f);
    Vec3 throwDir = normalize(
        forwardFlat * std::cos(angleDown) +
        Vec3(0.0f, -std::sin(angleDown), 0.0f));

    s.vel = throwDir * speed;
    s.pos = cameraPos + forward * 0.5f + up * -0.1f;
}

void updateStones(float dt, float timef, float waterHeight,
                  Vec3 &cubePos, Vec3 &cube2Pos, float &cubeVelY, float &cube2VelY) {
    const Vec3 gravity(0.0f, -9.81f, 0.0f);

    for (int i = 0; i < kMaxStones; ++i) {
        Stone &s = g_stones[i];
        if (!s.active) continue;

        s.life += dt;
        if (s.life > 10.0f) {
            s.active = false;
            continue;
        }

        Vec3 oldPos = s.pos;
        Vec3 vel = s.vel;

        vel = vel + gravity * dt;
        Vec3 newPos = oldPos + vel * dt;

        Vec3 sample(newPos.x, 0.0f, newPos.z);
        Vec3 surf = evalGerstnerXZ(sample, timef);
        float waterY = waterHeight + surf.y;

        bool wasAbove = (oldPos.y > waterY);
        bool isBelowOrOn = (newPos.y <= waterY);

        if (wasAbove && isBelowOrOn && vel.y < 0.0f) {
            float vMag = length(vel);
            float horizMag = std::sqrt(vel.x * vel.x + vel.z * vel.z);
            float alpha = (horizMag > 1e-4f) ? std::atan2(-vel.y, horizMag) : 0.0f;
            float alphaDeg = alpha * 180.0f / kPi;

            const float minSpeed = 5.0f;
            const float maxAngleDeg = 30.0f;
            const int maxBounces = 8;

            if (vMag > minSpeed && alphaDeg < maxAngleDeg && s.bounces < maxBounces) {
                Vec3 n(0.0f, 1.0f, 0.0f);
                float vn = dot(vel, n);
                Vec3 vN = n * vn;
                Vec3 vT = vel - vN;

                const float normalRestitution = 0.6f;
                const float tangentBase = 0.9f;
                float tangentDamping = tangentBase - 0.04f * static_cast<float>(s.bounces);
                if (tangentDamping < 0.6f) tangentDamping = 0.6f;

                vN = vN * (-normalRestitution);
                vT = vT * tangentDamping;

                vel = vN + vT;
                newPos.y = waterY + 0.02f;

                s.bounces++;

                addRipple(Vec3(newPos.x, waterHeight, newPos.z), timef);
            } else {
                s.active = false;
                continue;
            }
        }

        auto tryHitCube = [&](Vec3 &cPos, float &cVelY) {
            const float half = 0.6f;
            if (std::fabs(newPos.x - cPos.x) < half &&
                std::fabs(newPos.y - cPos.y) < half &&
                std::fabs(newPos.z - cPos.z) < half) {
                cVelY += 2.5f;
                Vec3 dir = normalize(vel);
                cPos += dir * 0.08f;
                addRipple(newPos, timef);
                s.active = false;
            }
        };
        if (s.active) tryHitCube(cubePos, cubeVelY);
        if (s.active) tryHitCube(cube2Pos, cube2VelY);

        s.pos = newPos;
        s.vel = vel;
    }
}
