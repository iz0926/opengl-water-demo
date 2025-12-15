#pragma once

#include <vector>
#include "Math.hpp"
#include "Boat.hpp"
#include "Rod.hpp"

struct Fish {
    Vec3 pos;
    Vec3 vel;
    bool active;
    float respawnTimer;
    float yawDeg;
    float prevYawDeg;
    float yawVel;
    bool caught;
    float caughtTimer;
    int state;          // 0 = idle, 1 = fighting (hooked)
    float fightTimer;
    float fightDuration;
    bool catchCounted;
    Fish()
        : pos(),
          vel(),
          active(true),
          respawnTimer(0.0f),
          yawDeg(0.0f),
          prevYawDeg(0.0f),
          yawVel(0.0f),
          caught(false),
          caughtTimer(0.0f),
          state(0),
          fightTimer(0.0f),
          fightDuration(0.0f),
          catchCounted(false) {}
};

void initFish(std::vector<Fish> &fish, int count, float waterHeight);
void updateFish(std::vector<Fish> &fish, float dt, float timef, float waterHeight,
                const Boat &boat, Rod &rod, const Vec3 &cubePos, const Vec3 &cube2Pos,
                const Vec3 &playerPos, bool playerUnderwater, int &fishCaught);
