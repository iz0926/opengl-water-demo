#include "Input.hpp"
#include <algorithm>

float g_yawDeg = -90.0f;
float g_pitchDeg = -15.0f;
bool g_firstMouse = true;
double g_lastCursorX = 0.0, g_lastCursorY = 0.0;
bool g_mouseCaptured = true;
bool g_throwCharging = false;
float g_throwCharge = 0.0f;

void cursorPosCallback(GLFWwindow *, double xpos, double ypos) {
    if (!g_mouseCaptured)
        return;
    if (g_firstMouse) {
        g_lastCursorX = xpos;
        g_lastCursorY = ypos;
        g_firstMouse = false;
    }
    double dx = xpos - g_lastCursorX;
    double dy = g_lastCursorY - ypos;
    g_lastCursorX = xpos;
    g_lastCursorY = ypos;

    const float sensitivity = 0.12f;
    g_yawDeg += static_cast<float>(dx) * sensitivity;
    g_pitchDeg += static_cast<float>(dy) * sensitivity;
    g_pitchDeg = std::clamp(g_pitchDeg, -89.0f, 89.0f);
}
