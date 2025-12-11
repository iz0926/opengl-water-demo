#include "Rod.hpp"
#include <cmath>

void startRodCharge(Rod &r) {
    r.charging = true;
    r.charge = 0.0f;
}

void updateRodCharge(Rod &r, float dt) {
    if (!r.charging) return;
    const float chargeSpeed = 1.0f;
    r.charge += chargeSpeed * dt;
    if (r.charge > 1.0f) r.charge = 1.0f;
}

void releaseRod(Rod &r, const Vec3 &cameraPos, const Vec3 &forward, const Vec3 &right, const Vec3 &up, float waterHeight, float timef) {
    if (!r.charging) return;
    float t = r.charge;
    float minAngle = 8.0f;
    float maxAngle = 18.0f;
    float angleDownDeg = maxAngle - t * (maxAngle - minAngle);
    float minSpeed = 4.0f;
    float maxSpeed = 20.0f; // longer holds throw farther
    float speed = minSpeed + t * (maxSpeed - minSpeed);

    float angleRad = angleDownDeg * (kPi / 180.0f);
    Vec3 launchDir = normalize(Vec3(forward.x,
                                    forward.y - std::sin(angleRad),
                                    forward.z));
    if (length(launchDir) < 1e-4f) launchDir = Vec3(1.0f, 0.0f, 0.0f);

    r.active = true;
    r.flying = true;
    r.timer = 0.0f;
    r.pos = cameraPos + forward * 0.3f + right * 0.2f + up * 0.2f;
    r.vel = launchDir * speed;
    addRipple(Vec3(r.pos.x, waterHeight, r.pos.z), timef);

    r.charging = false;
    r.charge = 0.0f;
}

void updateRod(Rod &r, float dt, float timef, float waterHeight) {
    if (!r.active) return;

    r.timer += dt;
    // Ballistic motion with mild drag
    r.vel.y -= 9.81f * 0.25f * dt;
    float drag = std::exp(-1.5f * dt);
    r.vel = r.vel * drag;
    r.pos += r.vel * dt;

    Vec3 sample(r.pos.x, 0.0f, r.pos.z);
    Vec3 surf = evalGerstnerXZ(sample, timef);
    float surfaceY = waterHeight + surf.y + rippleFieldHeight(sample, timef);
    if (r.pos.y < surfaceY + 0.05f) {
        r.pos.y = surfaceY + 0.05f;
        r.vel.y = 0.0f;
        r.flying = false;
    }

    if (r.timer > 6.0f) {
        r.active = false;
        r.flying = false;
    }
}

bool isRodInWater(const Rod &r) {
    return r.active && !r.flying;
}
