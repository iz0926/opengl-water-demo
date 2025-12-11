#pragma once

#include "Math.hpp"
#include "Stone.hpp"

struct Rod {
    Vec3 pos;
    Vec3 vel;
    float timer;
    bool active;
    bool flying;
    bool charging;
    float charge;

    Rod() : pos(), vel(), timer(0.0f), active(false), flying(false), charging(false), charge(0.0f) {}
};

void startRodCharge(Rod &r);
void updateRodCharge(Rod &r, float dt);
void releaseRod(Rod &r, const Vec3 &cameraPos, const Vec3 &forward, const Vec3 &right, const Vec3 &up, float waterHeight, float timef);
void updateRod(Rod &r, float dt, float timef, float waterHeight);
bool isRodInWater(const Rod &r);
