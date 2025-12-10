#pragma once

#include "Math.hpp"
#include "Waves.hpp"
#include <vector>

struct Stone {
    Vec3 pos;
    Vec3 vel;
    int bounces;
    float life;
    bool active;

    Stone() : pos(), vel(), bounces(0), life(0.0f), active(false) {}
};

struct RippleEvent {
    Vec3 pos;
    float startTime;
    bool active;
    RippleEvent() : pos(), startTime(0.0f), active(false) {}
};

constexpr int kMaxStones = 8;
constexpr int kMaxRipples = 32;

extern Stone g_stones[kMaxStones];
extern RippleEvent g_ripples[kMaxRipples];

void addRipple(const Vec3 &pos, float time);
void pruneRipples(float time);
float rippleFieldHeight(const Vec3 &pos, float time);
Vec3 rippleFieldGrad(const Vec3 &pos, float time);

void spawnStone(const Vec3 &cameraPos, const Vec3 &forward, const Vec3 &up,
                float angleDownDeg, float speed);
void updateStones(float dt, float timef, float waterHeight,
                  Vec3 &cubePos, Vec3 &cube2Pos, float &cubeVelY, float &cube2VelY);
