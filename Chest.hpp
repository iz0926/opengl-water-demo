#pragma once

#include "Math.hpp"
#include <cstdlib>

struct Chest {
    bool active = false;
    Vec3 pos{0.0f, 0.0f, 0.0f};
    double spawnTime = 0.0;
};

float frand(float a, float b);
void spawnChestNear(const Vec3 &center, Chest &ch, float radius, double now, float groundY);
bool chestExpired(const Chest &ch, double now, double ttlSeconds);
bool tryCollectChest(Chest &ch, const Vec3 &playerPos, float radius, int &prizes,
                     double now, double &nextChestTime, float respawnMin, float respawnMax);
