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
                const Boat &boat, Rod &rod, const Vec3 &cubePos, const Vec3 &cube2Pos,
                const Vec3 &playerPos, bool playerUnderwater, int &fishCaught) {
    const float swimSpeed = 1.2f;
    const float wanderYaw = 25.0f; // deg/sec
    const float avoidRadius = 2.0f;
    const float avoidCubeRadius = 1.2f;
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
                f.prevYawDeg = f.yawDeg;
                f.yawVel = 0.0f;
                f.caught = false;
                f.caughtTimer = 0.0f;
                f.state = 0;
                f.fightTimer = 0.0f;
                f.fightDuration = 0.0f;
                f.catchCounted = false;
            }
            continue;
        }

        if (f.caught) {
            f.caughtTimer += dt;
            if (f.caughtTimer > 1.0f) {
                f.caught = false;
                f.active = false;
                f.respawnTimer = 0.0f;
            }
            continue;
        }

        // Fighting state: attached to rod, wiggle until landed
        if (f.state == 1) {
            f.fightTimer += dt;

            // Attach head to lure position
            Vec3 dir = rod.vel;
            dir.y = 0.0f;
            if (length(dir) < 0.001f) dir = Vec3(1.0f, 0.0f, 0.0f);
            dir = normalize(dir);
            Vec3 right = Vec3(-dir.z, 0.0f, dir.x);

            float wiggleAmp = 0.5f;
            float wiggle = std::sin(f.fightTimer * 6.0f) * wiggleAmp;

            // Anchor mouth at lure; keep position fixed at mouth
            f.pos = rod.pos + dir * 0.35f + Vec3(0.0f, -0.1f, 0.0f);

            // Yaw-only sweep about the mouth so the tail swings left/right (~20 deg total)
            float baseYaw = std::atan2(dir.z, dir.x) * 180.0f / kPi;
            f.yawDeg = baseYaw + wiggle * 20.0f;
            f.prevYawDeg = f.yawDeg;
            f.yawVel = 0.0f;
            f.vel = Vec3(0.0f, 0.0f, 0.0f);
            f.pos.y = waterHeight - 0.2f;

            if (f.fightTimer > f.fightDuration) {
                f.caught = true;
                f.caughtTimer = 0.0f;
                f.state = 0;
                if (!f.catchCounted) {
                    fishCaught++;
                    f.catchCounted = true;
                }
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
        // Player scare when underwater and nearby
        if (playerUnderwater) {
            steerAway(Vec3(playerPos.x, 0.0f, playerPos.z), 2.5f, 1.2f, 0.7f);
        }

        // Wrap yaw
        if (f.yawDeg > 180.0f) f.yawDeg -= 360.0f;
        if (f.yawDeg < -180.0f) f.yawDeg += 360.0f;
        float yawDelta = f.yawDeg - f.prevYawDeg;
        if (yawDelta > 180.0f) yawDelta -= 360.0f;
        if (yawDelta < -180.0f) yawDelta += 360.0f;
        f.yawVel = yawDelta / dt;
        f.prevYawDeg = f.yawDeg;

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

        // Catch by launched red cube (rod) — only one fish per lure
        if (rod.active && !rod.hasCaught) {
            Vec3 d = f.pos - rod.pos;
            float dist = std::sqrt(d.x * d.x + d.z * d.z);
            const float catchRadiusLure = 0.6f;
            if (dist < catchRadiusLure) {
                // Start fighting state
                f.state = 1;
                f.fightTimer = 0.0f;
                f.fightDuration = frand(0.9f, 1.4f);
                f.catchCounted = false;
                rod.hasCaught = true;
                // Do not add a strong ripple; fighting shouldn't disturb the water
                continue;
            }
        }

    }
}
