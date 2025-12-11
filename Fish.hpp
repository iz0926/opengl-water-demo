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
    Fish() : pos(), vel(), active(true), respawnTimer(0.0f), yawDeg(0.0f) {}
};

void initFish(std::vector<Fish> &fish, int count, float waterHeight);
void updateFish(std::vector<Fish> &fish, float dt, float timef, float waterHeight,
                const Boat &boat, const Rod &rod, const Vec3 &cubePos, const Vec3 &cube2Pos, int &fishCaught);
