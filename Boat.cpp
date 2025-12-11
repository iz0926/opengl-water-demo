#include "Boat.hpp"
#include <cmath>

Vec3 boatForward(const Boat &b) {
    // Adjusted so forward aligns with the visual bow (model is rotated relative to yaw)
    float rad = (b.yawDeg + 90.0f) * (kPi / 180.0f);
    return normalize(Vec3(std::cos(rad), 0.0f, std::sin(rad)));
}

Vec3 boatForwardVisual(const Boat &b) {
    // Model nose appears rotated 90 degrees relative to physics forward
    float rad = (b.yawDeg + 90.0f) * (kPi / 180.0f);
    return normalize(Vec3(std::cos(rad), 0.0f, std::sin(rad)));
}

void updateBoat(Boat &b, float dt, bool forward, bool back, bool turnL, bool turnR,
                float waterHeight, float timef) {
    if (!b.active) return;

    const float accel = 4.0f;
    const float maxSpeed = 8.0f;
    const float turnSpeed = 60.0f;
    const float sideAccel = 2.5f;

    if (forward) b.speed += accel * dt;
    if (back)    b.speed -= accel * dt;
    b.speed = std::clamp(b.speed, -maxSpeed * 0.4f, maxSpeed);

    if (turnL) b.yawDeg += turnSpeed * dt;
    if (turnR) b.yawDeg -= turnSpeed * dt;

    // Integrate position in XZ
    Vec3 fwd = boatForward(b);
    Vec3 right = Vec3(-fwd.z, 0.0f, fwd.x);
    float sideDir = 0.0f;
    if (turnR) sideDir += 1.0f;
    if (turnL) sideDir -= 1.0f;
    b.pos += fwd * (b.speed * dt);
    if (sideDir != 0.0f) {
        b.pos += right * (sideDir * sideAccel * dt);
    }

    // Damping
    b.speed *= std::exp(-1.2f * dt);

    // Bob with waves + ripples
    Vec3 sample(b.pos.x, 0.0f, b.pos.z);
    Vec3 surf = evalGerstnerXZ(sample, timef);
    float yTarget = waterHeight + surf.y + rippleFieldHeight(sample, timef) + 0.05f;
    b.pos.y = b.pos.y * 0.8f + yTarget * 0.2f;
}

static void pushSingleCube(const Boat &b, Vec3 &cubePos) {
    Vec3 boatXZ(b.pos.x, 0.0f, b.pos.z);
    Vec3 cubeXZ(cubePos.x, 0.0f, cubePos.z);
    Vec3 d = cubeXZ - boatXZ;
    float dist = length(d);
    if (dist < 1e-4f) return;

    const float boatRadius = 1.5f;
    const float cubeRadius = 0.9f * std::sqrt(2.0f);
    float minDist = boatRadius + cubeRadius;
    if (dist < minDist) {
        Vec3 n = d * (1.0f / dist);
        float penetration = minDist - dist;
        cubePos.x += n.x * penetration;
        cubePos.z += n.z * penetration;
    }
}

void pushCubesWithBoat(const Boat &b, Vec3 &cubePos, Vec3 &cube2Pos) {
    pushSingleCube(b, cubePos);
    pushSingleCube(b, cube2Pos);

    // Prevent cubes from overlapping each other (simple sphere check in XZ)
    Vec3 d(cube2Pos.x - cubePos.x, 0.0f, cube2Pos.z - cubePos.z);
    float dist2 = dot(d, d);
    const float cubeRadius = 0.7f * std::sqrt(2.0f);
    float minDist = 2.0f * cubeRadius;
    if (dist2 < minDist * minDist && dist2 > 1e-6f) {
        float dist = std::sqrt(dist2);
        Vec3 n = d * (1.0f / dist);
        float penetration = minDist - dist;
        // Move both cubes apart equally
        cubePos  = cubePos - n * (penetration * 0.5f);
        cube2Pos = cube2Pos + n * (penetration * 0.5f);
    }
}
