#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "Math.hpp"
#include "GLHelpers.hpp"
#include "Mesh.hpp"
#include "Waves.hpp"
#include "Stone.hpp"
#include "Input.hpp"

namespace {

constexpr float kWaterHeight = 0.0f;

void glfwErrorCallback(int code, const char *desc) {
    std::cerr << "GLFW error " << code << ": " << desc << std::endl;
}

} // namespace

int main() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(1280, 720, "Water Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    g_lastCursorX = fbWidth * 0.5;
    g_lastCursorY = fbHeight * 0.5;
    g_firstMouse = true;
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Sky shader + fullscreen triangle
    const std::string skyVsSource = readFile("shaders/sky.vshader");
    const std::string skyFsSource = readFile("shaders/sky.fshader");
    GLuint skyVs = compileShader(GL_VERTEX_SHADER, skyVsSource);
    GLuint skyFs = compileShader(GL_FRAGMENT_SHADER, skyFsSource);
    GLuint skyProgram = linkProgram(skyVs, skyFs);
    const GLint locSkyTop = glGetUniformLocation(skyProgram, "uTopColor");
    const GLint locSkyHorizon = glGetUniformLocation(skyProgram, "uHorizonColor");
    const GLint locSkyUnderwater = glGetUniformLocation(skyProgram, "uUnderwater");

    GLuint fsQuadVao = 0;
    GLuint fsQuadVbo = 0;
    {
        float verts[] = {
            -1.0f, -1.0f,
             3.0f, -1.0f,
            -1.0f,  3.0f
        };
        glGenVertexArrays(1, &fsQuadVao);
        glGenBuffers(1, &fsQuadVbo);
        glBindVertexArray(fsQuadVao);
        glBindBuffer(GL_ARRAY_BUFFER, fsQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Scene shader (solid lit + fog + caustics + shadows)
    const std::string sceneVsSource = readFile("shaders/simple.vshader");
    const std::string sceneFsSource = readFile("shaders/simple.fshader");
    GLuint sceneVs = compileShader(GL_VERTEX_SHADER, sceneVsSource);
    GLuint sceneFs = compileShader(GL_FRAGMENT_SHADER, sceneFsSource);
    GLuint sceneProgram = linkProgram(sceneVs, sceneFs);

    const GLint locSceneViewProj = glGetUniformLocation(sceneProgram, "uViewProj");
    const GLint locSceneModel = glGetUniformLocation(sceneProgram, "uModel");
    const GLint locSceneLightDir = glGetUniformLocation(sceneProgram, "uLightDir");
    const GLint locSceneColor = glGetUniformLocation(sceneProgram, "uColor");
    const GLint locSceneEyePos = glGetUniformLocation(sceneProgram, "uEyePos");
    const GLint locSceneClipY = glGetUniformLocation(sceneProgram, "uClipY");
    const GLint locSceneUseClip = glGetUniformLocation(sceneProgram, "uUseClip");
    const GLint locSceneWaterHeight = glGetUniformLocation(sceneProgram, "uWaterHeight");
    const GLint locSceneUnderwater = glGetUniformLocation(sceneProgram, "uUnderwater");
    const GLint locSceneFogColorAbove = glGetUniformLocation(sceneProgram, "uFogColorAbove");
    const GLint locSceneFogColorBelow = glGetUniformLocation(sceneProgram, "uFogColorBelow");
    const GLint locSceneFogStart = glGetUniformLocation(sceneProgram, "uFogStart");
    const GLint locSceneFogEnd = glGetUniformLocation(sceneProgram, "uFogEnd");
    const GLint locSceneUnderFogDensity = glGetUniformLocation(sceneProgram, "uUnderFogDensity");
    const GLint locSceneLightVP = glGetUniformLocation(sceneProgram, "uLightVP");
    const GLint locSceneShadowMap = glGetUniformLocation(sceneProgram, "uShadowMap");
    const GLint locSceneTime = glGetUniformLocation(sceneProgram, "uTime");

    // Water shader
    const std::string waterVsSource = readFile("shaders/water.vshader");
    const std::string waterFsSource = readFile("shaders/water.fshader");
    GLuint waterVs = compileShader(GL_VERTEX_SHADER, waterVsSource);
    GLuint waterFs = compileShader(GL_FRAGMENT_SHADER, waterFsSource);
    GLuint waterProgram = linkProgram(waterVs, waterFs);

    const GLint locWaterViewProj = glGetUniformLocation(waterProgram, "uViewProj");
    const GLint locWaterModel = glGetUniformLocation(waterProgram, "uModel");
    const GLint locWaterTime = glGetUniformLocation(waterProgram, "uTime");
    const GLint locWaterMove = glGetUniformLocation(waterProgram, "uMove");
    const GLint locWaterDeepColor = glGetUniformLocation(waterProgram, "uDeepColor");
    const GLint locWaterLightDir = glGetUniformLocation(waterProgram, "uLightDir");
    const GLint locWaterEyePos = glGetUniformLocation(waterProgram, "uEyePos");
    const GLint locWaterRefl = glGetUniformLocation(waterProgram, "uReflectionTex");
    const GLint locWaterReflVP = glGetUniformLocation(waterProgram, "uReflectionVP");
    const GLint locWaterSceneTex = glGetUniformLocation(waterProgram, "uSceneTex");
    const GLint locWaterSceneDepth = glGetUniformLocation(waterProgram, "uSceneDepth");
    const GLint locWaterNear = glGetUniformLocation(waterProgram, "uNear");
    const GLint locWaterFar = glGetUniformLocation(waterProgram, "uFar");
    const GLint locWaterViewProjScene = glGetUniformLocation(waterProgram, "uViewProjScene");
    const GLint locWaterRoughness = glGetUniformLocation(waterProgram, "uRoughness");
    const GLint locWaterFresnelBias = glGetUniformLocation(waterProgram, "uFresnelBias");
    const GLint locWaterFresnelScale = glGetUniformLocation(waterProgram, "uFresnelScale");
    const GLint locWaterFoamColor = glGetUniformLocation(waterProgram, "uFoamColor");
    const GLint locWaterFoamIntensity = glGetUniformLocation(waterProgram, "uFoamIntensity");
    const GLint locWaterNormalMap = glGetUniformLocation(waterProgram, "uNormalMap");
    const GLint locWaterNormalScale = glGetUniformLocation(waterProgram, "uNormalScale");
    const GLint locWaterReflDistort = glGetUniformLocation(waterProgram, "uReflDistort");
    const GLint locWaterRefrDistort = glGetUniformLocation(waterProgram, "uRefrDistort");
    const GLint locWaterDudvMap = glGetUniformLocation(waterProgram, "uDudvMap");
    const GLint locWaterUnderwater = glGetUniformLocation(waterProgram, "uUnderwater");
    const GLint locWaterLightVP = glGetUniformLocation(waterProgram, "uLightVP");
    const GLint locWaterShadowMap = glGetUniformLocation(waterProgram, "uShadowMap");
    const GLint locWaterRippleCount = glGetUniformLocation(waterProgram, "uRippleCount");
    const GLint locWaterRipples = glGetUniformLocation(waterProgram, "uRipples[0]");

    // Shadow-only shader
    const std::string shadowVsSource = readFile("shaders/shadow.vshader");
    const std::string shadowFsSource = readFile("shaders/shadow.fshader");
    GLuint shadowVs = compileShader(GL_VERTEX_SHADER, shadowVsSource);
    GLuint shadowFs = compileShader(GL_FRAGMENT_SHADER, shadowFsSource);
    GLuint shadowProgram = linkProgram(shadowVs, shadowFs);
    const GLint locShadowLightVP = glGetUniformLocation(shadowProgram, "uLightVP");
    const GLint locShadowModel = glGetUniformLocation(shadowProgram, "uModel");

    // Post-process shaders (reuse fullscreen tri VAO)
    const std::string postVsSource = readFile("shaders/post.vshader");
    const std::string brightFsSource = readFile("shaders/brightpass.fshader");
    const std::string blurFsSource = readFile("shaders/blur.fshader");
    const std::string tonemapFsSource = readFile("shaders/tonemap.fshader");
    const std::string fxaaFsSource = readFile("shaders/fxaa.fshader");
    const std::string lightshaftFsSource = readFile("shaders/lightshaft.fshader");

    GLuint postVs = compileShader(GL_VERTEX_SHADER, postVsSource);
    GLuint brightFs = compileShader(GL_FRAGMENT_SHADER, brightFsSource);
    GLuint blurFs = compileShader(GL_FRAGMENT_SHADER, blurFsSource);
    GLuint tonemapFs = compileShader(GL_FRAGMENT_SHADER, tonemapFsSource);
    GLuint fxaaFs = compileShader(GL_FRAGMENT_SHADER, fxaaFsSource);
    GLuint lightshaftFs = compileShader(GL_FRAGMENT_SHADER, lightshaftFsSource);

    GLuint brightProgram = linkProgram(postVs, brightFs);
    GLuint blurProgram = linkProgram(postVs, blurFs);
    GLuint tonemapProgram = linkProgram(postVs, tonemapFs);
    GLuint fxaaProgram = linkProgram(postVs, fxaaFs);
    GLuint lightshaftProgram = linkProgram(postVs, lightshaftFs);

    // Bright-pass uniforms
    const GLint locBrightHDR = glGetUniformLocation(brightProgram, "uHDRColor");
    const GLint locBrightThreshold = glGetUniformLocation(brightProgram, "uThreshold");

    // Blur uniforms
    const GLint locBlurImage = glGetUniformLocation(blurProgram, "uImage");
    const GLint locBlurHorizontal = glGetUniformLocation(blurProgram, "uHorizontal");
    const GLint locBlurTexelSize = glGetUniformLocation(blurProgram, "uTexelSize");

    // Tonemap uniforms
    const GLint locToneHDR = glGetUniformLocation(tonemapProgram, "uHDRColor");
    const GLint locToneBloom = glGetUniformLocation(tonemapProgram, "uBloom");
    const GLint locToneExposure = glGetUniformLocation(tonemapProgram, "uExposure");
    const GLint locToneBloomStrength = glGetUniformLocation(tonemapProgram, "uBloomStrength");
    const GLint locToneGamma = glGetUniformLocation(tonemapProgram, "uGamma");

    // Light shaft uniforms
    const GLint locLightshaftDepth = glGetUniformLocation(lightshaftProgram, "uDepth");
    const GLint locLightshaftSunPos = glGetUniformLocation(lightshaftProgram, "uSunScreenPos");
    const GLint locLightshaftDecay = glGetUniformLocation(lightshaftProgram, "uDecay");
    const GLint locLightshaftDensity = glGetUniformLocation(lightshaftProgram, "uDensity");
    const GLint locLightshaftWeight = glGetUniformLocation(lightshaftProgram, "uWeight");
    const GLint locLightshaftExposure = glGetUniformLocation(lightshaftProgram, "uExposure");
    const GLint locLightshaftUnderwater = glGetUniformLocation(lightshaftProgram, "uUnderwater");

    // FXAA uniforms
    const GLint locFxaaImage = glGetUniformLocation(fxaaProgram, "uImage");
    const GLint locFxaaTexelSize = glGetUniformLocation(fxaaProgram, "uTexelSize");

    // Geometry: ground plane, cubes, water plane
    const float halfSize = 10.0f;
    Mesh ground = makeGroundMesh(halfSize);
    Mesh cube   = makeCubeMesh();
    Mesh cube2  = makeCubeMesh();
    Mesh waterMesh = makeGroundMesh(halfSize); // big plane at water height

    Vec3 cubePos(0.0f, 0.5f, 0.0f);
    Vec3 cube2Pos(2.5f, 0.5f, -1.5f);
    float cubeVelY = 0.0f;
    float cube2VelY = 0.0f;

    // Camera & timing
    Vec3 cameraPos(0.0f, 2.0f, 6.0f);
    const Vec3 worldUp(0.0f, 1.0f, 0.0f);
    double lastTime = glfwGetTime();

    // Buffers
    Framebuffer reflectionFb = makeReflectionBuffer(fbWidth, fbHeight);
    Framebuffer sceneFb      = makeSceneBuffer(fbWidth, fbHeight);
    Framebuffer hdrFb        = makeHdrBuffer(fbWidth, fbHeight);
    Framebuffer bloomFb[2]   = {
        makeBloomBuffer(fbWidth / 2, fbHeight / 2),
        makeBloomBuffer(fbWidth / 2, fbHeight / 2)
    };
    Framebuffer ldrFb        = makeLdrBuffer(fbWidth, fbHeight);
    ShadowMap shadowMap      = makeShadowMap(2048);

    GLuint waterNormalTex = createWaterNormalMap(256, 4.0f);
    GLuint waterDudvTex   = createDudvTexture(256);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        int newFbW = 0, newFbH = 0;
        glfwGetFramebufferSize(window, &newFbW, &newFbH);
        if (newFbW != fbWidth || newFbH != fbHeight) {
            fbWidth = newFbW;
            fbHeight = newFbH;
            glViewport(0, 0, fbWidth, fbHeight);
            destroyFramebuffer(reflectionFb);
            destroyFramebuffer(sceneFb);
            destroyFramebuffer(hdrFb);
            destroyFramebuffer(bloomFb[0]);
            destroyFramebuffer(bloomFb[1]);
            destroyFramebuffer(ldrFb);
            destroyShadowMap(shadowMap);

            reflectionFb = makeReflectionBuffer(fbWidth, fbHeight);
            sceneFb      = makeSceneBuffer(fbWidth, fbHeight);
            hdrFb        = makeHdrBuffer(fbWidth, fbHeight);
            bloomFb[0]   = makeBloomBuffer(fbWidth / 2, fbHeight / 2);
            bloomFb[1]   = makeBloomBuffer(fbWidth / 2, fbHeight / 2);
            ldrFb        = makeLdrBuffer(fbWidth, fbHeight);
            shadowMap    = makeShadowMap(2048);
        }

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        const float timef = static_cast<float>(now);

        // Camera orientation
        const float yawRad = g_yawDeg * (kPi / 180.0f);
        const float pitchRad = g_pitchDeg * (kPi / 180.0f);
        Vec3 forward(std::cos(yawRad) * std::cos(pitchRad),
                     std::sin(pitchRad),
                     std::sin(yawRad) * std::cos(pitchRad));
        forward = normalize(forward);
        Vec3 right = normalize(cross(forward, worldUp));
        Vec3 up = normalize(cross(right, forward));

        const float moveSpeed = (cameraPos.y < kWaterHeight) ? 2.2f : 4.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += forward * (moveSpeed * dt);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos += forward * (-moveSpeed * dt);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos += right * (-moveSpeed * dt);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * (moveSpeed * dt);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos += worldUp * (-moveSpeed * dt);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += worldUp * (moveSpeed * dt);

        const float turnSpeed = 60.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  g_yawDeg -= turnSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_yawDeg += turnSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    g_pitchDeg += turnSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  g_pitchDeg -= turnSpeed * dt;
        if (g_pitchDeg > 89.0f)  g_pitchDeg = 89.0f;
        if (g_pitchDeg < -89.0f) g_pitchDeg = -89.0f;

        // Throw charging (hold F)
        bool throwKeyNow = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
        if (throwKeyNow) {
            if (!g_throwCharging) {
                g_throwCharging = true;
                g_throwCharge = 0.0f;
            }
            const float chargeSpeed = 1.0f;
            g_throwCharge += chargeSpeed * dt;
            if (g_throwCharge > 1.0f) g_throwCharge = 1.0f;
        } else {
            if (g_throwCharging) {
                float t = g_throwCharge;
                float minAngle = 6.0f;
                float maxAngle = 22.0f;
                float angleDownDeg = maxAngle - t * (maxAngle - minAngle);

                float minSpeed = 6.0f;
                float maxSpeed = 20.0f;
                float speed = minSpeed + t * (maxSpeed - minSpeed);

                spawnStone(cameraPos, forward, up, angleDownDeg, speed);
                std::cout << "Throw: charge=" << g_throwCharge
                          << " angleDown=" << angleDownDeg
                          << " speed=" << speed << std::endl;
                g_throwCharging = false;
                g_throwCharge = 0.0f;
            }
        }

        // Allow going underwater; clamp only far below
        cameraPos.y = std::max(cameraPos.y, -10.0f);

        // Optional: camera bob was here; removed to allow diving underwater

        bool underwater = (cameraPos.y < kWaterHeight - 0.05f);

        // Time-of-day sun direction
        const float sunTime = timef * 0.05f;
        const float sunElevation = 0.6f;
        Vec3 sunDir(std::cos(sunTime) * std::cos(sunElevation),
                    std::sin(sunElevation),
                    std::sin(sunTime) * std::cos(sunElevation));
        sunDir = normalize(sunDir);

        // Matrices
        float aspect = fbWidth > 0 && fbHeight > 0 ?
                       static_cast<float>(fbWidth) / fbHeight :
                       16.0f / 9.0f;
        Vec3 viewPos = cameraPos;
        if (!underwater) {
            Vec3 surf = evalGerstnerXZ(Vec3(cameraPos.x, 0.0f, cameraPos.z), timef);
            viewPos.y += surf.y * 0.1f;
        }
        Mat4 view = Mat4::lookAt(viewPos, viewPos + forward, up);
        Mat4 proj = Mat4::perspective(60.0f * (kPi / 180.0f), aspect, 0.1f, 200.0f);
        Mat4 viewProj = proj * view;

        // Light view-projection for shadows
        Vec3 lightTarget(0.0f, 0.0f, 0.0f);
        Vec3 lightPos = lightTarget - sunDir * 30.0f;
        Mat4 lightView = Mat4::lookAt(lightPos, lightTarget, worldUp);
        Mat4 lightProj = Mat4::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 60.0f);
        Mat4 lightVP = lightProj * lightView;

        // Buoyancy update for floating cubes
        auto updateFloat = [&](Vec3 &pos, float &velY) {
            Vec3 sample(pos.x, 0.0f, pos.z);
        Vec3 surf = evalGerstnerXZ(sample, timef);
        float surfaceY = kWaterHeight + surf.y + rippleFieldHeight(sample, timef);
        float targetY = surfaceY + 0.3f;
        float dy = targetY - pos.y;
        float stiffness = 4.0f;
        float damping = 1.5f;
        velY += stiffness * dy * dt;
        velY *= std::exp(-damping * dt);
        pos.y += velY * dt;
        // Gentle lateral push from ripple slope
        Vec3 grad = rippleFieldGrad(sample, timef);
        pos.x += grad.x * 0.5f * dt;
        pos.z += grad.z * 0.5f * dt;
    };
        updateFloat(cubePos, cubeVelY);
        updateFloat(cube2Pos, cube2VelY);

        // Skipping stone motion (stones can hit cubes and add ripples)
        updateStones(dt, timef, kWaterHeight, cubePos, cube2Pos, cubeVelY, cube2VelY);
        pruneRipples(timef);

        // Model matrices for this frame
        Mat4 modelCube = Mat4::translate(cubePos);
        Mat4 modelCube2 = Mat4::translate(cube2Pos);
        const float tileSize = halfSize * 2.0f;
        const int tileRadius = 3;
        float baseX = std::floor(cameraPos.x / tileSize) * tileSize;
        float baseZ = std::floor(cameraPos.z / tileSize) * tileSize;

        // Reflection camera
        Vec3 reflPos = cameraPos;
        reflPos.y = 2.0f * kWaterHeight - cameraPos.y;
        Vec3 reflForward(forward.x, -forward.y, forward.z);
        Vec3 reflUp(up.x, -up.y, up.z);
        Mat4 reflView = Mat4::lookAt(reflPos, reflPos + reflForward, reflUp);
        Mat4 reflProj = proj;
        Mat4 reflViewProj = reflProj * reflView;

        // --------- Shadow map pass ---------
        glViewport(0, 0, shadowMap.width, shadowMap.height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);

        glUseProgram(shadowProgram);
        glUniformMatrix4fv(locShadowLightVP, 1, GL_FALSE, lightVP.m.data());

        // Ground tiles
        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;
                Mat4 modelGroundTile = Mat4::translate(Vec3(tileX, -1.0f, tileZ));
                glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, modelGroundTile.m.data());
                glBindVertexArray(ground.vao);
                glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
            }
        }

        // Cube 1
        glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, modelCube.m.data());
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube 2
        glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, modelCube2.m.data());
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Skipping stones
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, modelStone.m.data());
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --------- Scene prepass (color/depth for refraction) ---------
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFb.fbo);
        glViewport(0, 0, sceneFb.width, sceneFb.height);
        glClearColor(0.08f, 0.1f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(sceneProgram);
        glUniformMatrix4fv(locSceneViewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform3f(locSceneLightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(locSceneEyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(locSceneClipY, kWaterHeight);
        glUniform1i(locSceneUseClip, 0);
        glUniform1f(locSceneWaterHeight, kWaterHeight);
        glUniform1i(locSceneUnderwater, underwater ? 1 : 0);
        glUniform3f(locSceneFogColorAbove, 0.6f, 0.75f, 0.9f);
        glUniform3f(locSceneFogColorBelow, 0.02f, 0.10f, 0.14f);
        glUniform1f(locSceneFogStart, 20.0f);
        glUniform1f(locSceneFogEnd,   90.0f);
        glUniform1f(locSceneUnderFogDensity, 0.06f);
        glUniformMatrix4fv(locSceneLightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1f(locSceneTime, timef);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(locSceneShadowMap, 5);

        // Ground tiles
        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;
                Mat4 modelGroundTile = Mat4::translate(Vec3(tileX, -1.0f, tileZ));
                glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelGroundTile.m.data());
                glUniform3f(locSceneColor, 0.35f, 0.55f, 0.35f);
                glBindVertexArray(ground.vao);
                glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
            }
        }

        // Cube 1
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(locSceneColor, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube 2
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(locSceneColor, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Stones prepass
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(locSceneColor, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        glBindTexture(GL_TEXTURE_2D, sceneFb.colorTex);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        // --------- Reflection pass ---------
        glBindFramebuffer(GL_FRAMEBUFFER, reflectionFb.fbo);
        glViewport(0, 0, reflectionFb.width, reflectionFb.height);
        glClearColor(0.08f, 0.1f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CLIP_DISTANCE0);
        glCullFace(GL_FRONT);

        glUseProgram(sceneProgram);
        glUniformMatrix4fv(locSceneViewProj, 1, GL_FALSE, reflViewProj.m.data());
        glUniform3f(locSceneLightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(locSceneEyePos, reflPos.x, reflPos.y, reflPos.z);
        glUniform1f(locSceneClipY, kWaterHeight);
        glUniform1i(locSceneUseClip, 1);
        glUniformMatrix4fv(locSceneLightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1i(locSceneUnderwater, 0);
        glUniform1f(locSceneTime, timef);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(locSceneShadowMap, 5);

        // Ground tiles reflected
        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;
                Mat4 modelGroundTile = Mat4::translate(Vec3(tileX, -1.0f, tileZ));
                glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelGroundTile.m.data());
                glUniform3f(locSceneColor, 0.35f, 0.55f, 0.35f);
                glBindVertexArray(ground.vao);
                glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
            }
        }

        // Cube1
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(locSceneColor, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube2
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(locSceneColor, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Stones reflected
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(locSceneColor, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        glDisable(GL_CLIP_DISTANCE0);
        glCullFace(GL_BACK);

        glBindTexture(GL_TEXTURE_2D, reflectionFb.colorTex);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        // --------- HDR scene pass (sky + geometry + water) ---------
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFb.fbo);
        glViewport(0, 0, hdrFb.width, hdrFb.height);
        glClearColor(0.08f, 0.1f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Sky
        glDisable(GL_DEPTH_TEST);
        glUseProgram(skyProgram);
        float sunHeight = sunDir.y * 0.5f + 0.5f;
        sunHeight = std::max(0.0f, std::min(1.0f, sunHeight));

        Vec3 topDay(0.15f, 0.35f, 0.7f);
        Vec3 topSunset(0.08f, 0.05f, 0.2f);
        Vec3 horizonDay(0.5f, 0.7f, 0.9f);
        Vec3 horizonSunset(0.9f, 0.45f, 0.2f);

        Vec3 topColor(
            topSunset.x * (1 - sunHeight) + topDay.x * sunHeight,
            topSunset.y * (1 - sunHeight) + topDay.y * sunHeight,
            topSunset.z * (1 - sunHeight) + topDay.z * sunHeight
        );
        Vec3 horizonColor(
            horizonSunset.x * (1 - sunHeight) + horizonDay.x * sunHeight,
            horizonSunset.y * (1 - sunHeight) + horizonDay.y * sunHeight,
            horizonSunset.z * (1 - sunHeight) + horizonDay.z * sunHeight
        );

        glUniform3f(locSkyTop, topColor.x, topColor.y, topColor.z);
        glUniform3f(locSkyHorizon, horizonColor.x, horizonColor.y, horizonColor.z);
        glUniform1i(locSkyUnderwater, underwater ? 1 : 0);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // Scene geometry to HDR
        glUseProgram(sceneProgram);
        glUniformMatrix4fv(locSceneViewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform3f(locSceneLightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(locSceneEyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(locSceneClipY, kWaterHeight);
        glUniform1i(locSceneUseClip, 0);
        glUniform1f(locSceneWaterHeight, kWaterHeight);
        glUniform1i(locSceneUnderwater, underwater ? 1 : 0);
        glUniform3f(locSceneFogColorAbove, 0.6f, 0.75f, 0.9f);
        glUniform3f(locSceneFogColorBelow, 0.02f, 0.10f, 0.14f);
        glUniform1f(locSceneFogStart, 20.0f);
        glUniform1f(locSceneFogEnd,   90.0f);
        glUniform1f(locSceneUnderFogDensity, 0.06f);
        glUniformMatrix4fv(locSceneLightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1f(locSceneTime, timef);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(locSceneShadowMap, 5);

        // Ground tiles
        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;
                Mat4 modelGroundTile = Mat4::translate(Vec3(tileX, -1.0f, tileZ));
                glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelGroundTile.m.data());
                glUniform3f(locSceneColor, 0.35f, 0.55f, 0.35f);
                glBindVertexArray(ground.vao);
                glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
            }
        }

        // Cube1
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(locSceneColor, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube2
        glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(locSceneColor, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Skipping stones
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(locSceneModel, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(locSceneColor, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Water surface (tiled around the camera for "infinite" lake)
        glUseProgram(waterProgram);

        glUniformMatrix4fv(locWaterViewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform1f(locWaterTime, timef);
        glUniform1f(locWaterMove, timef * 0.03f);
        glUniform3f(locWaterDeepColor, 0.05f, 0.2f, 0.35f);
        glUniform3f(locWaterLightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(locWaterEyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniformMatrix4fv(locWaterReflVP, 1, GL_FALSE, reflViewProj.m.data());
        glUniformMatrix4fv(locWaterViewProjScene, 1, GL_FALSE, viewProj.m.data());
        glUniform1f(locWaterNear, 0.1f);
        glUniform1f(locWaterFar, 200.0f);
        glUniform1f(locWaterRoughness, 0.25f);
        glUniform1f(locWaterFresnelBias, 0.04f);
        glUniform1f(locWaterFresnelScale, 0.85f);
        glUniform3f(locWaterFoamColor, 0.8f, 0.85f, 0.9f);
        glUniform1f(locWaterFoamIntensity, 0.15f);
        glUniform1f(locWaterNormalScale, 0.5f);
        glUniform1f(locWaterReflDistort, 0.4f);
        glUniform1f(locWaterRefrDistort, 0.25f);
        glUniform1i(locWaterUnderwater, underwater ? 1 : 0);
        glUniformMatrix4fv(locWaterLightVP, 1, GL_FALSE, lightVP.m.data());
        {
            std::array<float, kMaxRipples * 4> rippleBuf{};
            int rippleCount = 0;
            for (int i = 0; i < kMaxRipples; ++i) {
                if (!g_ripples[i].active) continue;
                rippleBuf[rippleCount * 4 + 0] = g_ripples[i].pos.x;
                rippleBuf[rippleCount * 4 + 1] = g_ripples[i].pos.y;
                rippleBuf[rippleCount * 4 + 2] = g_ripples[i].pos.z;
                rippleBuf[rippleCount * 4 + 3] = g_ripples[i].startTime;
                rippleCount++;
                if (rippleCount >= kMaxRipples) break;
            }
            glUniform1i(locWaterRippleCount, rippleCount);
            if (rippleCount > 0 && locWaterRipples >= 0) {
                glUniform4fv(locWaterRipples, rippleCount, rippleBuf.data());
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionFb.colorTex);
        glUniform1i(locWaterRefl, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneFb.colorTex);
        glUniform1i(locWaterSceneTex, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sceneFb.depthTex);
        glUniform1i(locWaterSceneDepth, 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, waterNormalTex);
        glUniform1i(locWaterNormalMap, 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, waterDudvTex);
        glUniform1i(locWaterDudvMap, 4);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(locWaterShadowMap, 5);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(waterMesh.vao);

        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;

                Mat4 modelWater = Mat4::translate(Vec3(tileX, kWaterHeight, tileZ));
                glUniformMatrix4fv(locWaterModel, 1, GL_FALSE, modelWater.m.data());
                glDrawArrays(GL_TRIANGLES, 0, waterMesh.vertexCount);
            }
        }

        glDisable(GL_BLEND);
        glBindVertexArray(0);

        // --------- Post-process: HDR -> Bloom -> Tone map -> FXAA ---------
        glDisable(GL_DEPTH_TEST);

        // Bright-pass to bloomFb[0] (downsample)
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFb[0].fbo);
        glViewport(0, 0, bloomFb[0].width, bloomFb[0].height);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(brightProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrFb.colorTex);
        glUniform1i(locBrightHDR, 0);
        glUniform1f(locBrightThreshold, 1.2f);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Add light shafts into bloom buffer (additive)
        // Compute sun screen position
        Vec4 sunClip = viewProj * Vec4(lightPos.x, lightPos.y, lightPos.z, 1.0f);
        Vec2 sunScreen(sunClip.x / sunClip.w * 0.5f + 0.5f,
                       sunClip.y / sunClip.w * 0.5f + 0.5f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glUseProgram(lightshaftProgram);
        glUniform2f(locLightshaftSunPos, sunScreen.x, sunScreen.y);
        glUniform1f(locLightshaftDecay, 0.95f);
        glUniform1f(locLightshaftDensity, 0.9f);
        glUniform1f(locLightshaftWeight, 0.25f);
        glUniform1f(locLightshaftExposure, 0.6f);
        glUniform1i(locLightshaftUnderwater, underwater ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneFb.depthTex);
        glUniform1i(locLightshaftDepth, 0);
        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisable(GL_BLEND);

        // Blur ping-pong
        glUseProgram(blurProgram);
        glUniform2f(locBlurTexelSize,
                    1.0f / bloomFb[0].width,
                    1.0f / bloomFb[0].height);

        bool horizontal = true;
        bool firstIteration = true;
        const int blurPasses = 6;
        GLuint lastBloomTex = bloomFb[0].colorTex;

        for (int i = 0; i < blurPasses; ++i) {
            Framebuffer &targetFb = horizontal ? bloomFb[1] : bloomFb[0];
            Framebuffer &sourceFb = horizontal ? bloomFb[0] : bloomFb[1];

            glBindFramebuffer(GL_FRAMEBUFFER, targetFb.fbo);
            glViewport(0, 0, targetFb.width, targetFb.height);
            glClear(GL_COLOR_BUFFER_BIT);

            glUniform1i(locBlurHorizontal, horizontal ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            if (firstIteration) {
                glBindTexture(GL_TEXTURE_2D, bloomFb[0].colorTex);
                firstIteration = false;
            } else {
                glBindTexture(GL_TEXTURE_2D, sourceFb.colorTex);
            }
            glUniform1i(locBlurImage, 0);

            glBindVertexArray(fsQuadVao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            horizontal = !horizontal;
            lastBloomTex = targetFb.colorTex;
        }

        // Tone map into LDR buffer
        glBindFramebuffer(GL_FRAMEBUFFER, ldrFb.fbo);
        glViewport(0, 0, ldrFb.width, ldrFb.height);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(tonemapProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrFb.colorTex);
        glUniform1i(locToneHDR, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lastBloomTex);
        glUniform1i(locToneBloom, 1);

        glUniform1f(locToneExposure, 1.0f);
        glUniform1f(locToneBloomStrength, 0.8f);
        glUniform1f(locToneGamma, 2.2f);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // FXAA from LDR buffer to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbWidth, fbHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(fxaaProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ldrFb.colorTex);
        glUniform1i(locFxaaImage, 0);
        glUniform2f(locFxaaTexelSize,
                    1.0f / fbWidth,
                    1.0f / fbHeight);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
    }

    destroyMesh(ground);
    destroyMesh(cube);
    destroyMesh(cube2);
    destroyMesh(waterMesh);

    glDeleteProgram(sceneProgram);
    glDeleteProgram(waterProgram);
    glDeleteProgram(skyProgram);
    glDeleteProgram(shadowProgram);
    glDeleteProgram(brightProgram);
    glDeleteProgram(blurProgram);
    glDeleteProgram(tonemapProgram);
    glDeleteProgram(fxaaProgram);

    glDeleteShader(sceneVs);
    glDeleteShader(sceneFs);
    glDeleteShader(waterVs);
    glDeleteShader(waterFs);
    glDeleteShader(skyVs);
    glDeleteShader(skyFs);
    glDeleteShader(shadowVs);
    glDeleteShader(shadowFs);
    glDeleteShader(postVs);
    glDeleteShader(brightFs);
    glDeleteShader(blurFs);
    glDeleteShader(tonemapFs);
    glDeleteShader(fxaaFs);

    glDeleteVertexArrays(1, &fsQuadVao);
    glDeleteBuffers(1, &fsQuadVbo);

    destroyFramebuffer(reflectionFb);
    destroyFramebuffer(sceneFb);
    destroyFramebuffer(hdrFb);
    destroyFramebuffer(bloomFb[0]);
    destroyFramebuffer(bloomFb[1]);
    destroyFramebuffer(ldrFb);
    destroyShadowMap(shadowMap);

    glDeleteTextures(1, &waterNormalTex);
    glDeleteTextures(1, &waterDudvTex);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
