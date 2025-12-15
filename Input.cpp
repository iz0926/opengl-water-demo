#include "Input.hpp"
#include <algorithm>

float g_yawDeg = -90.0f;
float g_pitchDeg = -15.0f;
bool g_firstMouse = true;
double g_lastCursorX = 0.0, g_lastCursorY = 0.0;
bool g_mouseCaptured = true;
bool g_inBoatMode = false;
float g_mouseSensitivity = 0.12f;
bool g_throwCharging = false;
float g_throwCharge = 0.0f;

void cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    if (!g_mouseCaptured) return;

    // In free mode, only rotate while right mouse button is held.
    if (!g_inBoatMode) {
        int rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (rightDown != GLFW_PRESS) {
            g_firstMouse = true; // reset so next drag doesn't jump
            return;
        }
    }

    if (g_firstMouse) {
        g_lastCursorX = xpos;
        g_lastCursorY = ypos;
        g_firstMouse = false;
    }
    double dx = xpos - g_lastCursorX;
    double dy = g_lastCursorY - ypos;
    g_lastCursorX = xpos;
    g_lastCursorY = ypos;

    g_yawDeg += static_cast<float>(dx) * g_mouseSensitivity;
    g_pitchDeg += static_cast<float>(dy) * g_mouseSensitivity;
    g_pitchDeg = std::clamp(g_pitchDeg, -89.0f, 89.0f);
}
