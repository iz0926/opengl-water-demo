#include "Chest.hpp"
#include <cmath>

float frand(float a, float b) {
    return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

void spawnChestNear(const Vec3 &center, Chest &ch, float radius, double now, float groundY) {
    ch.active = true;
    ch.spawnTime = now;
    ch.pos.x = center.x + frand(-radius, radius);
    ch.pos.z = center.z + frand(-radius, radius);
    ch.pos.y = groundY + 0.4f; // sit slightly above seabed so the whole mesh is visible
}

bool chestExpired(const Chest &ch, double now, double ttlSeconds) {
    return ch.active && (now - ch.spawnTime) > ttlSeconds;
}

bool tryCollectChest(Chest &ch, const Vec3 &playerPos, float radius, int &prizes,
                     double now, double &nextChestTime, float respawnMin, float respawnMax) {
    if (!ch.active) return false;
    Vec3 d = playerPos - ch.pos;
    float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
    if (dist2 < radius * radius) {
        ch.active = false;
        prizes += 1;
        nextChestTime = now + frand(respawnMin, respawnMax);
        return true;
    }
    return false;
}
