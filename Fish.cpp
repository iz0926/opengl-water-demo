#include "Fish.hpp"
#include "Stone.hpp"
#include "Rod.hpp"
#include <cstdlib>
#include <cmath>

static float frand(float a, float b) {
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return a + (b - a) * t;
}

void initFish(std::vector<Fish> &fish, int count, float waterHeight) {
    fish.clear();
    fish.resize(count);
    for (size_t i = 0; i < fish.size(); ++i) {
        bool placed = false;
        int tries = 0;
        while (!placed && tries < 16) {
            ++tries;
            Fish &f = fish[i];
            f.pos = Vec3(frand(-12.0f, 12.0f), waterHeight - frand(0.2f, 0.8f), frand(-12.0f, 12.0f));
            f.vel = Vec3(frand(-1.0f, 1.0f), 0.0f, frand(-1.0f, 1.0f));
            f.active = true;
            f.respawnTimer = 0.0f;
            f.yawDeg = std::atan2(f.vel.z, f.vel.x) * 180.0f / kPi;
            // Separation check against already placed fish
            bool collide = false;
            for (size_t j = 0; j < i; ++j) {
                Vec3 d = f.pos - fish[j].pos;
                float dist2 = d.x * d.x + d.z * d.z;
                if (dist2 < 1.0f) { collide = true; break; }
            }
            if (!collide) placed = true;
        }
    }
}

void updateFish(std::vector<Fish> &fish, float dt, float timef, float waterHeight,
                const Boat &boat, const Rod &rod, const Vec3 &cubePos, const Vec3 &cube2Pos, int &fishCaught) {
    const float swimSpeed = 1.2f;
    const float wanderYaw = 25.0f; // deg/sec
    const float avoidRadius = 2.0f;
    const float avoidCubeRadius = 1.2f;
    const float catchRadiusBoat = 1.2f;
    Vec3 boatXZ(boat.pos.x, 0.0f, boat.pos.z);

    for (auto &f : fish) {
        if (!f.active) {
            f.respawnTimer += dt;
            if (f.respawnTimer > 2.0f) {
                f.pos = Vec3(frand(-12.0f, 12.0f), waterHeight - frand(0.2f, 0.8f), frand(-12.0f, 12.0f));
                f.vel = Vec3(frand(-1.0f, 1.0f), 0.0f, frand(-1.0f, 1.0f));
                f.active = true;
                f.respawnTimer = 0.0f;
                f.yawDeg = std::atan2(f.vel.z, f.vel.x) * 180.0f / kPi;
            }
            continue;
        }

        // Simple wander in yaw (steering)
        // Small random turn occasionally
        float wanderChance = frand(0.0f, 1.0f);
        if (wanderChance < 0.03f) { // ~3% chance per frame
            f.yawDeg += frand(-15.0f, 15.0f);
        } else {
            f.yawDeg += frand(-wanderYaw, wanderYaw) * dt;
        }

        // Avoid boat and cubes
        auto steerAway = [&](const Vec3 &fromPos, float radius, float turnStrengthNear, float turnStrengthFar) {
            Vec3 to = f.pos - fromPos;
            float dist = std::sqrt(to.x * to.x + to.z * to.z);
            if (dist < radius && dist > 0.0001f) {
                Vec3 away = to * (1.0f / dist);
                float awayYaw = std::atan2(away.z, away.x) * 180.0f / kPi;
                float delta = awayYaw - f.yawDeg;
                while (delta > 180.0f) delta -= 360.0f;
                while (delta < -180.0f) delta += 360.0f;
                float strength = (dist < radius * 0.6f) ? turnStrengthNear : turnStrengthFar;
                f.yawDeg += delta * strength;
            }
        };

        steerAway(boatXZ, avoidRadius, 1.0f, 0.5f);
        steerAway(Vec3(cubePos.x, 0.0f, cubePos.z), avoidCubeRadius, 0.9f, 0.45f);
        steerAway(Vec3(cube2Pos.x, 0.0f, cube2Pos.z), avoidCubeRadius, 0.9f, 0.45f);

        // Wrap yaw
        if (f.yawDeg > 180.0f) f.yawDeg -= 360.0f;
        if (f.yawDeg < -180.0f) f.yawDeg += 360.0f;

        // Move forward along heading
        float yawRad = f.yawDeg * (kPi / 180.0f);
        Vec3 desiredVel(std::cos(yawRad), 0.0f, -std::sin(yawRad));
        desiredVel = desiredVel * swimSpeed;

        // Smooth velocity toward desired to reduce twitchiness
        const float accel = 4.0f;
        f.vel = f.vel + (desiredVel - f.vel) * (1.0f - std::exp(-accel * dt));

        f.pos += f.vel * dt;
        f.pos.y = waterHeight - 0.3f + 0.15f * std::sin(timef + f.pos.x * 0.5f);

        // Keep within bounds
        const float bounds = 14.0f;
        f.pos.x = std::clamp(f.pos.x, -bounds, bounds);
        f.pos.z = std::clamp(f.pos.z, -bounds, bounds);

        // Heading already defines velocity

        // Catch by boat hull
        Vec3 toBoatCatch = f.pos - boatXZ;
        float distBoat = std::sqrt(toBoatCatch.x * toBoatCatch.x + toBoatCatch.z * toBoatCatch.z);
        if (distBoat < catchRadiusBoat) {
            f.active = false;
            f.respawnTimer = 0.0f;
            fishCaught++;
            addRipple(Vec3(f.pos.x, waterHeight, f.pos.z), timef);
            continue;
        }
        // Catch by launched red cube (rod)
        if (rod.active) {
            Vec3 d = f.pos - rod.pos;
            float dist = std::sqrt(d.x * d.x + d.z * d.z);
            const float catchRadiusLure = 0.6f;
            if (dist < catchRadiusLure) {
                f.active = false;
                f.respawnTimer = 0.0f;
                fishCaught++;
                addRipple(Vec3(f.pos.x, waterHeight, f.pos.z), timef);
                continue;
            }
        }

    }
}
