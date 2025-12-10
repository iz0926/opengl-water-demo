#pragma once

#include "GLFW/glfw3.h"

extern float g_yawDeg;
extern float g_pitchDeg;
extern bool g_firstMouse;
extern double g_lastCursorX;
extern double g_lastCursorY;
extern bool g_mouseCaptured;
extern bool g_throwCharging;
extern float g_throwCharge;

void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
