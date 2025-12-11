#pragma once

#include "Math.hpp"
#include "Waves.hpp"
#include "Stone.hpp"

struct Boat {
    Vec3 pos;
    float yawDeg;
    float speed;
    bool active;

    Boat() : pos(), yawDeg(0.0f), speed(0.0f), active(true) {}
};

Vec3 boatForward(const Boat &b);
Vec3 boatForwardVisual(const Boat &b); // use if the model nose is rotated in local space
void updateBoat(Boat &b, float dt, bool forward, bool back, bool turnL, bool turnR,
                float waterHeight, float timef);
void pushCubesWithBoat(const Boat &b, Vec3 &cubePos, Vec3 &cube2Pos);
