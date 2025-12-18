#include <array>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "Math.hpp"
#include "GLHelpers.hpp"
#include "Mesh.hpp"
#include "Waves.hpp"
#include "Stone.hpp"
#include "Input.hpp"
#include "Boat.hpp"
#include "Fish.hpp"
#include "Rod.hpp"
#include "Chest.hpp"
#include "Audio.hpp"
#include "Audio.hpp"

namespace {

constexpr float kWaterHeight = 0.0f;
constexpr float kGroundY = -1.3f;

void glfwErrorCallback(int code, const char *desc) {
    std::cerr << "GLFW error " << code << ": " << desc << std::endl;
}

struct MenuResult {
    bool resume = false;
    bool exit = false;
};

MenuResult drawEscMenu(int fbWidth,
                       bool audioReady,
                       Audio &audio,
                       float &mouseSensitivity,
                       float &bgmVolume,
                       bool &bgmMuted,
                       int &bgmCueIndex,
                       const std::vector<double> &bgmCues) {
    MenuResult result;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(fbWidth), 260.0f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Controls", nullptr, flags);
    ImGui::Text("Controls:");
    ImGui::BulletText("W/A/S/D: move (free mode)");
    ImGui::BulletText("Right-drag: look (free mode)");
    ImGui::BulletText("B: toggle boat mode and drive");
    ImGui::BulletText("V: switch controlled cube");
    ImGui::BulletText("Left mouse: press/hold to push selected cube, release to pop");
    ImGui::BulletText("R: cast red cube (catch fish)");
    ImGui::BulletText("F: throw skipping stone");
    ImGui::BulletText("Swim into glowing chest to collect prize");
    ImGui::Separator();

    ImGui::Text("Look sensitivity:"); ImGui::SameLine();
    {
        float avail = ImGui::GetContentRegionAvail().x;
        float sliderWidth = std::max(200.0f, avail - 2000.0f);
        ImGui::PushItemWidth(sliderWidth);
        ImGui::SliderFloat("##look_sens", &mouseSensitivity, 0.02f, 0.4f, "%.2f");
        ImGui::PopItemWidth();
    }

    ImGui::Text("BGM volume:"); ImGui::SameLine();
    {
        float buttonSpace = 120.0f; // room for mute + skip buttons
        float avail = ImGui::GetContentRegionAvail().x;
        float sliderWidth = std::max(200.0f, avail - buttonSpace - 2000.0f);
        ImGui::PushItemWidth(sliderWidth);
        ImGui::SliderFloat("##bgm_vol", &bgmVolume, 0.0f, 1.0f, "%.2f");
        ImGui::PopItemWidth();
    }
    ImGui::SameLine(0.0f, 10.0f);
    const char *volLabel = bgmMuted ? "[vol off]" : "[vol on]";
    if (ImGui::Button(volLabel)) {
        bgmMuted = !bgmMuted;
        if (audioReady) audio.play("click", 0, -1, 96);
    }
    ImGui::SameLine();
    if (ImGui::Button("<<")) {
        if (bgmCueIndex > 0 && audioReady) {
            bgmCueIndex--;
            audio.setMusicPosition(bgmCues[bgmCueIndex]);
            audio.play("click", 0, -1, 96);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(">>")) {
        if (bgmCueIndex + 1 < static_cast<int>(bgmCues.size()) && audioReady) {
            bgmCueIndex++;
            audio.setMusicPosition(bgmCues[bgmCueIndex]);
            audio.play("click", 0, -1, 96);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Resume")) {
        if (audioReady) audio.play("click", 0, -1, 96);
        result.resume = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit")) {
        if (audioReady) audio.play("click", 0, -1, 96);
        result.exit = true;
    }
    ImGui::End();
    return result;
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

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Sky shader + fullscreen triangle
    const std::string skyVsSource = readFile("shaders/sky.vshader");
    const std::string skyFsSource = readFile("shaders/sky.fshader");
    GLuint skyVs = compileShader(GL_VERTEX_SHADER, skyVsSource);
    GLuint skyFs = compileShader(GL_FRAGMENT_SHADER, skyFsSource);
    GLuint skyProgram = linkProgram(skyVs, skyFs);
    struct SkyUniforms {
        GLint top;
        GLint horizon;
        GLint underwater;
        GLint sunHeight;
    } skyU{
        glGetUniformLocation(skyProgram, "uTopColor"),
        glGetUniformLocation(skyProgram, "uHorizonColor"),
        glGetUniformLocation(skyProgram, "uUnderwater"),
        glGetUniformLocation(skyProgram, "uSunHeight"),
    };

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
    struct SceneUniforms {
        GLint viewProj;
        GLint model;
        GLint lightDir;
        GLint color;
        GLint texture;
        GLint useTexture;
        GLint eyePos;
        GLint clipY;
        GLint useClip;
        GLint waterHeight;
        GLint underwater;
        GLint fogColorAbove;
        GLint fogColorBelow;
        GLint fogStart;
        GLint fogEnd;
        GLint underFogDensity;
        GLint lightVP;
        GLint shadowMap;
        GLint time;
    } sceneU{
        glGetUniformLocation(sceneProgram, "uViewProj"),
        glGetUniformLocation(sceneProgram, "uModel"),
        glGetUniformLocation(sceneProgram, "uLightDir"),
        glGetUniformLocation(sceneProgram, "uColor"),
        glGetUniformLocation(sceneProgram, "uTexture"),
        glGetUniformLocation(sceneProgram, "uUseTexture"),
        glGetUniformLocation(sceneProgram, "uEyePos"),
        glGetUniformLocation(sceneProgram, "uClipY"),
        glGetUniformLocation(sceneProgram, "uUseClip"),
        glGetUniformLocation(sceneProgram, "uWaterHeight"),
        glGetUniformLocation(sceneProgram, "uUnderwater"),
        glGetUniformLocation(sceneProgram, "uFogColorAbove"),
        glGetUniformLocation(sceneProgram, "uFogColorBelow"),
        glGetUniformLocation(sceneProgram, "uFogStart"),
        glGetUniformLocation(sceneProgram, "uFogEnd"),
        glGetUniformLocation(sceneProgram, "uUnderFogDensity"),
        glGetUniformLocation(sceneProgram, "uLightVP"),
        glGetUniformLocation(sceneProgram, "uShadowMap"),
        glGetUniformLocation(sceneProgram, "uTime"),
    };

    // Water shader
    const std::string waterVsSource = readFile("shaders/water.vshader");
    const std::string waterFsSource = readFile("shaders/water.fshader");
    GLuint waterVs = compileShader(GL_VERTEX_SHADER, waterVsSource);
    GLuint waterFs = compileShader(GL_FRAGMENT_SHADER, waterFsSource);
    GLuint waterProgram = linkProgram(waterVs, waterFs);
    struct WaterUniforms {
        GLint viewProj;
        GLint model;
        GLint time;
        GLint move;
        GLint deepColor;
        GLint lightDir;
        GLint eyePos;
        GLint refl;
        GLint reflVP;
        GLint sceneTex;
        GLint sceneDepth;
        GLint nearZ;
        GLint farZ;
        GLint viewProjScene;
        GLint roughness;
        GLint fresnelBias;
        GLint fresnelScale;
        GLint foamColor;
        GLint foamIntensity;
        GLint normalMap;
        GLint normalScale;
        GLint reflDistort;
        GLint refrDistort;
        GLint dudvMap;
        GLint underwater;
        GLint lightVP;
        GLint shadowMap;
        GLint rippleCount;
        GLint ripples;
    } waterU{
        glGetUniformLocation(waterProgram, "uViewProj"),
        glGetUniformLocation(waterProgram, "uModel"),
        glGetUniformLocation(waterProgram, "uTime"),
        glGetUniformLocation(waterProgram, "uMove"),
        glGetUniformLocation(waterProgram, "uDeepColor"),
        glGetUniformLocation(waterProgram, "uLightDir"),
        glGetUniformLocation(waterProgram, "uEyePos"),
        glGetUniformLocation(waterProgram, "uReflectionTex"),
        glGetUniformLocation(waterProgram, "uReflectionVP"),
        glGetUniformLocation(waterProgram, "uSceneTex"),
        glGetUniformLocation(waterProgram, "uSceneDepth"),
        glGetUniformLocation(waterProgram, "uNear"),
        glGetUniformLocation(waterProgram, "uFar"),
        glGetUniformLocation(waterProgram, "uViewProjScene"),
        glGetUniformLocation(waterProgram, "uRoughness"),
        glGetUniformLocation(waterProgram, "uFresnelBias"),
        glGetUniformLocation(waterProgram, "uFresnelScale"),
        glGetUniformLocation(waterProgram, "uFoamColor"),
        glGetUniformLocation(waterProgram, "uFoamIntensity"),
        glGetUniformLocation(waterProgram, "uNormalMap"),
        glGetUniformLocation(waterProgram, "uNormalScale"),
        glGetUniformLocation(waterProgram, "uReflDistort"),
        glGetUniformLocation(waterProgram, "uRefrDistort"),
        glGetUniformLocation(waterProgram, "uDudvMap"),
        glGetUniformLocation(waterProgram, "uUnderwater"),
        glGetUniformLocation(waterProgram, "uLightVP"),
        glGetUniformLocation(waterProgram, "uShadowMap"),
        glGetUniformLocation(waterProgram, "uRippleCount"),
        glGetUniformLocation(waterProgram, "uRipples[0]"),
    };

    // Shadow-only shader
    const std::string shadowVsSource = readFile("shaders/shadow.vshader");
    const std::string shadowFsSource = readFile("shaders/shadow.fshader");
    GLuint shadowVs = compileShader(GL_VERTEX_SHADER, shadowVsSource);
    GLuint shadowFs = compileShader(GL_FRAGMENT_SHADER, shadowFsSource);
    GLuint shadowProgram = linkProgram(shadowVs, shadowFs);
    struct ShadowUniforms {
        GLint lightVP;
        GLint model;
    } shadowU{
        glGetUniformLocation(shadowProgram, "uLightVP"),
        glGetUniformLocation(shadowProgram, "uModel"),
    };

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

    struct BrightUniforms { GLint hdr; GLint threshold; } brightU{
        glGetUniformLocation(brightProgram, "uHDRColor"),
        glGetUniformLocation(brightProgram, "uThreshold"),
    };
    struct BlurUniforms { GLint image; GLint horizontal; GLint texelSize; } blurU{
        glGetUniformLocation(blurProgram, "uImage"),
        glGetUniformLocation(blurProgram, "uHorizontal"),
        glGetUniformLocation(blurProgram, "uTexelSize"),
    };
    struct TonemapUniforms { GLint hdr; GLint bloom; GLint exposure; GLint bloomStrength; GLint gamma; } toneU{
        glGetUniformLocation(tonemapProgram, "uHDRColor"),
        glGetUniformLocation(tonemapProgram, "uBloom"),
        glGetUniformLocation(tonemapProgram, "uExposure"),
        glGetUniformLocation(tonemapProgram, "uBloomStrength"),
        glGetUniformLocation(tonemapProgram, "uGamma"),
    };
    struct LightshaftUniforms { GLint depth; GLint sunPos; GLint decay; GLint density; GLint weight; GLint exposure; GLint underwater; } shaftU{
        glGetUniformLocation(lightshaftProgram, "uDepth"),
        glGetUniformLocation(lightshaftProgram, "uSunScreenPos"),
        glGetUniformLocation(lightshaftProgram, "uDecay"),
        glGetUniformLocation(lightshaftProgram, "uDensity"),
        glGetUniformLocation(lightshaftProgram, "uWeight"),
        glGetUniformLocation(lightshaftProgram, "uExposure"),
        glGetUniformLocation(lightshaftProgram, "uUnderwater"),
    };
    struct FxaaUniforms { GLint image; GLint texelSize; } fxaaU{
        glGetUniformLocation(fxaaProgram, "uImage"),
        glGetUniformLocation(fxaaProgram, "uTexelSize"),
    };

    // Geometry: ground plane, cubes, water plane
    const float halfSize = 10.0f;
    Mesh ground = makeGroundMesh(halfSize);
    Mesh cube   = makeCubeMesh();
    Mesh cube2  = makeCubeMesh();
    Mesh waterMesh = makeGroundMesh(halfSize); // big plane at water height
    Mesh boatMesh{};
    GLuint boatTexture = 0;
    Mesh fishMesh{};
    GLuint fishTexture = 0;
    Mesh chestMesh{};
    try {
        boatMesh = loadObjMesh("assets/models/SpeedBoat/10634_SpeedBoat_v01_LOD3.obj");
    } catch (const std::exception &e) {
        std::cerr << e.what() << " falling back to cube for boat" << std::endl;
        boatMesh = makeCubeMesh();
    }
    try {
        boatTexture = loadTexture2D("assets/models/SpeedBoat/10634_SpeedBoat_v01.jpg");
    } catch (const std::exception &e) {
        std::cerr << e.what() << " using flat color for boat\n";
    }
    try {
        fishMesh = loadObjMesh("assets/models/Fish/12265_Fish_v1_L2.obj");
    } catch (const std::exception &e) {
        std::cerr << e.what() << " falling back to cube for fish" << std::endl;
        fishMesh = makeCubeMesh();
    }
    try {
        fishTexture = loadTexture2D("assets/models/Fish/fish.jpg");
    } catch (const std::exception &e) {
        std::cerr << e.what() << " using flat color for fish\n";
    }
    try {
        chestMesh = loadObjMesh("assets/models/chest.obj");
    } catch (const std::exception &e) {
        std::cerr << e.what() << " falling back to cube for chest" << std::endl;
        chestMesh = makeCubeMesh();
    }

    Vec3 cubePos(0.0f, 0.5f, 0.0f);
    Vec3 cube2Pos(2.5f, 0.5f, -1.5f);
    float cubeVelY = 0.0f;
    float cube2VelY = 0.0f;
    // Cube interaction state
    bool leftMouseWasDown = false;
    float cubePressTime = 0.0f;
    int activeCubeIndex = -1;
    int controlledCubeIndex = 0;
    bool vWasDown = false;
    Boat boat;
    boat.pos = Vec3(-4.0f, 0.4f, 1.0f);
    boat.yawDeg = 20.0f;
    boat.speed = 0.0f;
    std::vector<Fish> fish;
    initFish(fish, 20, kWaterHeight);
    int fishCaught = 0;
    Rod rod;
    Chest chest;
    int prizes = 0;
    bool boatMode = false;
    bool prevBoatToggle = false;
    float boatViewYawOffset = 0.0f;
    float boatCamYawDeg = 0.0f;

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

    bool showMenu = false;
    bool prevEsc = false;

    // Audio
    Audio audio;
    const bool audioReady = audio.init();
    if (!audioReady) {
        std::cerr << "Audio init failed; continuing without sound\n";
    } else {
        // Streaming music (BGM)
        audio.loadMusic("assets/audio/relaxing zelda music + ocean waves.wav");
        audio.loadClip("boat", "assets/audio/Speed Boat Ride Ambience.wav", true);
        audio.loadClip("drop1", "assets/audio/water_drop1.wav");
        audio.loadClip("drop2", "assets/audio/water_drop2.wav");
        audio.loadClip("drop3", "assets/audio/water_drop3.wav");
        audio.loadClip("tiny_splash", "assets/audio/tiny_splash.wav");
        audio.loadClip("chest_spawn", "assets/audio/Magic WHOOSH.wav");
        audio.loadClip("chest_pickup", "assets/audio/Magical Twinkle Sound Effect (HD).wav");
        audio.loadClip("fish_catch", "assets/audio/success.wav");
        audio.loadClip("menu", "assets/audio/game_menu.wav");
        audio.loadClip("click", "assets/audio/mouse_click.wav");
        audio.loadClip("reel", "assets/audio/fish_reel.wav");
        audio.loadClip("hit_thud", "assets/audio/hit_thud.wav");
        audio.loadClip("underwater", "assets/audio/underwater_sounds.wav", true);
        audio.playMusic(-1, 48); // start BGM
    }
    const int kBoatChannel = 1;
    const int kReelChannel = 2;
    const int kUnderChannel = 3;
    const int kSplashChannel = 4;

    const double kChestInterval = 300.0; // 5 real minutes between spawns
    double nextChestTime = glfwGetTime() + kChestInterval;
    int splashIndex = 0;
    float bgmVolume = 0.38f; // normalized 0..1 (~48/128)
    bool bgmMuted = false;
    // BGM cue points (seconds) based on provided timestamps
    std::vector<double> bgmCues = {
        0.0,    248.0,  415.0,  599.0,  816.0, 1092.0, 1215.0, 1395.0, 1584.0,
        1775.0, 2121.0, 2198.0, 2301.0, 2363.0, 2949.0, 3110.0, 3247.0, 3425.0,
        3631.0, 3775.0, 3895.0, 4005.0, 4127.0, 4398.0, 4483.0, 4676.0, 4812.0,
        4898.0, 5193.0
    };
    int bgmCueIndex = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Toggle menu with ESC
        bool escNow = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        if (escNow && !prevEsc) {
            showMenu = !showMenu;
            g_mouseCaptured = !showMenu;
            g_firstMouse = true;
            if (audioReady && showMenu) audio.play("menu", 0, -1, 128);
        }
        prevEsc = escNow;

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;
        const float timef = static_cast<float>(now);

        const bool inputEnabled = !showMenu;

        // Update capture flag based on menu
        g_mouseCaptured = !showMenu;
        if (showMenu) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        // Boat mode toggle
        if (inputEnabled) {
        bool boatToggleNow = (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS);
        if (boatToggleNow && !prevBoatToggle) {
            boatMode = !boatMode;
            g_inBoatMode = boatMode;
            g_firstMouse = true; // avoid jump on next drag/boat mode switch
            if (boatMode) {
                    // Align boat heading to current view
                    boat.yawDeg = g_yawDeg;
                    boat.speed = 0.0f;
                    boatViewYawOffset = 0.0f;
                    const float boatYawVisualOffset = 90.0f; // model nose rotates +90 deg
                    boatCamYawDeg = boat.yawDeg + boatYawVisualOffset;
                }
            }
            prevBoatToggle = boatToggleNow;
        }

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
        if (inputEnabled) {
            if (!boatMode) {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += forward * (moveSpeed * dt);
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos += forward * (-moveSpeed * dt);
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos += right * (-moveSpeed * dt);
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += right * (moveSpeed * dt);
                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos += worldUp * (-moveSpeed * dt);
                if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += worldUp * (moveSpeed * dt);
            } else {
                // In boat mode, camera motion is handled after boat update.
            }
        }

        const float turnSpeed = 90.0f; // camera turn
        if (inputEnabled && !boatMode) {
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  g_yawDeg -= turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_yawDeg += turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    g_pitchDeg += turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  g_pitchDeg -= turnSpeed * dt;
        } else if (boatMode) {
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  boatViewYawOffset -= turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) boatViewYawOffset += turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    g_pitchDeg += turnSpeed * dt;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  g_pitchDeg -= turnSpeed * dt;
        }
        if (g_pitchDeg > 89.0f)  g_pitchDeg = 89.0f;
        if (g_pitchDeg < -89.0f) g_pitchDeg = -89.0f;

        if (inputEnabled) {
            // Toggle which cube is controlled (V key)
            bool vNow = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);
            if (vNow && !vWasDown) {
                controlledCubeIndex = 1 - controlledCubeIndex;
                std::cout << "Now controlling cube " << controlledCubeIndex << std::endl;
            }
            vWasDown = vNow;

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

                    splashIndex = 0;
                    spawnStone(cameraPos, forward, up, angleDownDeg, speed);
                    std::cout << "Throw: charge=" << g_throwCharge
                              << " angleDown=" << angleDownDeg
                              << " speed=" << speed << std::endl;
                    g_throwCharging = false;
                    g_throwCharge = 0.0f;
                }
            }

            // Cube mouse interaction
            int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            bool leftMouseDown = (mouseState == GLFW_PRESS);

            // On mouse press: start pushing the controlled cube
            if (leftMouseDown && !leftMouseWasDown) {
                activeCubeIndex = controlledCubeIndex; // 0 or 1
                cubePressTime = 0.0f;
            }

            // While holding mouse: push the selected cube DOWN via velocity
            if (leftMouseDown && activeCubeIndex != -1) {
                cubePressTime += dt;
                float &velY = (activeCubeIndex == 0) ? cubeVelY : cube2VelY;
                const float pushStrength = 20.0f;
                velY -= pushStrength * dt;
            }

            // On mouse release: give an upward impulse based on how long we held
            if (!leftMouseDown && leftMouseWasDown && activeCubeIndex != -1) {
                float &velY = (activeCubeIndex == 0) ? cubeVelY : cube2VelY;
                float t = std::min(cubePressTime, 2.0f); // cap charge time
                const float baseImpulse = 4.0f;
                const float extraImpulse = 6.0f * t;
                velY += baseImpulse + extraImpulse;
                activeCubeIndex = -1;
                cubePressTime = 0.0f;
            }

            leftMouseWasDown = leftMouseDown;

        // Launch red cube (rod) with R
        bool rodKeyNow = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
        if (rodKeyNow) {
            if (!rod.charging) startRodCharge(rod);
            updateRodCharge(rod, dt);
            if (audioReady && !audio.isChannelPlaying(kReelChannel)) {
                audio.play("reel", -1, kReelChannel, 96);
            }
        } else {
            if (rod.charging) {
                if (audioReady) audio.stopChannel(kReelChannel);
                releaseRod(rod, cameraPos, forward, right, up, kWaterHeight, timef);
            }
        }
        } else {
            // When menu is open, reset interaction states
            leftMouseWasDown = false;
            g_throwCharging = false;
            rod.charging = false;
            if (audioReady) audio.stopChannel(kReelChannel);
            activeCubeIndex = -1;
        }

       // Menu overlay
       if (showMenu) {
            MenuResult menuRes = drawEscMenu(fbWidth, audioReady, audio, g_mouseSensitivity,
                                             bgmVolume, bgmMuted, bgmCueIndex, bgmCues);
            if (menuRes.resume) {
                showMenu = false;
                g_mouseCaptured = true;
                g_firstMouse = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            if (menuRes.exit) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        // Boat steering
        if (boatMode && inputEnabled) {
            bool fwdKey = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
            bool backKey = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
            bool leftKey = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
            bool rightKey = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);

            // When steering, align hull to current camera view so W/S are always "forward/back" relative to what you see.
            if (fwdKey || backKey || leftKey || rightKey) {
                const float boatYawVisualOffset = 90.0f;
                float viewYawDeg = boatCamYawDeg + boatViewYawOffset - boatYawVisualOffset;
                boat.yawDeg = viewYawDeg;
                boatCamYawDeg = boat.yawDeg + boatYawVisualOffset;
                boatViewYawOffset = 0.0f;
                g_yawDeg = boatCamYawDeg;
            }

            updateBoat(boat, dt, fwdKey, backKey, leftKey, rightKey, kWaterHeight, timef);

            // Camera locked to boat bow (driver seat)
            auto shortestAngle = [](float deg) {
                while (deg > 180.0f) deg -= 360.0f;
                while (deg < -180.0f) deg += 360.0f;
                return deg;
            };

            // Smoothly ease the camera yaw toward the hull heading so the bow stays in view.
            const float boatYawVisualOffset = 90.0f; // model nose rotates +90 deg
            const float followRate = 6.0f;           // lerp rate per second
            float targetYaw = boat.yawDeg + boatYawVisualOffset;
            float yawDiff = shortestAngle(targetYaw - boatCamYawDeg);
            boatCamYawDeg += yawDiff * std::clamp(followRate * dt, 0.0f, 1.0f);

            Vec3 boatDir = boatForward(boat);

            const float seatHeight = 1.8f;
            const float seatBackDist = 0.01f;
            cameraPos = boat.pos - boatDir * seatBackDist + Vec3(0.0f, seatHeight, 0.0f);
            // Apply view yaw offset and pitch for look-around without moving seat
            float viewYawDeg = boatCamYawDeg + boatViewYawOffset;
            float viewYawRad = viewYawDeg * (kPi / 180.0f);
            float viewPitchRad = g_pitchDeg * (kPi / 180.0f);
            Vec3 viewDir(std::cos(viewYawRad) * std::cos(viewPitchRad),
                         std::sin(viewPitchRad),
                         std::sin(viewYawRad) * std::cos(viewPitchRad));
            Vec3 camTarget = cameraPos + normalize(viewDir) * 3.5f;
            forward = normalize(camTarget - cameraPos);
            right = normalize(cross(forward, worldUp));
            up = normalize(cross(right, forward));
            g_yawDeg = viewYawDeg;

            // Boat vs cube collisions handled via helper
            pushCubesWithBoat(boat, cubePos, cube2Pos);

            // Boat ambience volume based on speed
            if (audioReady) {
                float spd = std::fabs(boat.speed);
                if (spd > 0.1f) {
                    // Louder boat engine and a minimum floor so it's audible at low speeds
                    float norm = std::clamp(spd / 8.0f, 0.0f, 1.0f);
                    int vol = static_cast<int>((0.35f + 0.75f * norm) * 128.0f);
                    audio.setChannelVolume(kBoatChannel, vol);
                    if (!audio.isChannelPlaying(kBoatChannel)) {
                        audio.play("boat", -1, kBoatChannel, vol);
                    }
                } else {
                    audio.stopChannel(kBoatChannel);
                }
            }
        }
        if (!boatMode && audioReady) {
            audio.stopChannel(kBoatChannel);
        }

        // Allow going underwater; clamp only far below
        cameraPos.y = std::max(cameraPos.y, -10.0f);

        bool underwater = (cameraPos.y < kWaterHeight - 0.05f);

        // Underwater audio ducking + ambience
        if (audioReady) {
            const float bgmNorm = bgmMuted ? 0.0f : std::clamp(bgmVolume, 0.0f, 1.0f);
            const int baseVol = static_cast<int>(bgmNorm * 128.0f);
            if (underwater) {
                int underVol = static_cast<int>(baseVol * 0.25f); // ducked underwater
                audio.setMusicVolume(underVol);
                if (!audio.isChannelPlaying(kUnderChannel)) {
                    audio.play("underwater", -1, kUnderChannel, 72);
                } else {
                    audio.setChannelVolume(kUnderChannel, 72);
                }
            } else {
                audio.setMusicVolume(baseVol); // restore BGM
                audio.stopChannel(kUnderChannel);
            }
        }

        // Time-of-day sun direction (auto cycle) with a ~1-hour real-time full day
        const float kDayLengthSeconds = 3600.0f; // 24h game in 1h real
        float dayPhase = std::fmod(timef, kDayLengthSeconds) / kDayLengthSeconds; // 0..1
        const float sunTime = dayPhase * 2.0f * kPi;
        const float sunElevation = 0.6f;
        const float sunCosT = std::cos(sunTime);
        const float sunSinT = std::sin(sunTime);
        const float sunCosE = std::cos(sunElevation);
        const float sunSinE = std::sin(sunElevation);
        Vec3 sunDir(sunCosT * sunCosE,
                    sunSinE,
                    sunSinT * sunCosE);
        sunDir = normalize(sunDir);
        float sunHeightClamped = std::clamp(sunDir.y * 0.5f + 0.5f, 0.0f, 1.0f);

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

        // Time-of-day overlay (non-interactive, always visible)
        {
            ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoInputs;
            const float padding = 12.0f;
            // Convert sun cycle to a 24h clock based on sunTime period (2*pi) with accelerated time
            float dayHours = dayPhase * 24.0f;
            int hours = static_cast<int>(dayHours) % 24;
            if (hours < 0) hours += 24;
            int minutes = static_cast<int>((dayHours - hours) * 60.0f);

            ImVec2 pos(padding, padding);
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowBgAlpha(0.7f);
            ImGui::Begin("TimeHUD", nullptr, hudFlags);
            ImGui::SetWindowFontScale(1.1f);
            ImGui::Text("Time: %02d:%02d", hours, minutes);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::End();
        }

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
        updateStones(dt, timef, kWaterHeight, cubePos, cube2Pos, cubeVelY, cube2VelY, boat.pos, 1.5f);
        if (audioReady && !audio.isChannelPlaying(kSplashChannel)) {
            if (consumeTinySplash()) {
                audio.play("tiny_splash", 0, kSplashChannel, 128);
            } else if (consumeWaterSplash()) {
                if (splashIndex == 0) {
                    audio.play("drop1", 0, kSplashChannel, 128);
                } else if (splashIndex == 1) {
                    audio.play("drop2", 0, kSplashChannel, 128);
                } else {
                    audio.play("drop3", 0, kSplashChannel, 128);
                }
                splashIndex = std::min(splashIndex + 1, 2);
            }
        }
        if (audioReady && (consumeCubeHit() || consumeBoatHit())) {
            audio.play("hit_thud", 0, -1, 110);
        }
        updateRod(rod, dt, timef, kWaterHeight);
        pruneRipples(timef);

        int fishBefore = fishCaught;
        // Fish update
        updateFish(fish, dt, timef, kWaterHeight, boat, rod, cubePos, cube2Pos, cameraPos, underwater, fishCaught);
        if (audioReady && fishCaught > fishBefore) {
            audio.play("fish_catch", 0, -1, 128);
        }

        // Chest spawning / despawn
        double nowT = glfwGetTime();
        if (!chest.active && nowT > nextChestTime) {
            Vec3 center = boatMode ? boat.pos : cameraPos;
            spawnChestNear(center, chest, 12.0f, nowT, kGroundY);
            if (audioReady) audio.play("chest_spawn");
            nextChestTime = nowT + kChestInterval;
        }
        if (chestExpired(chest, nowT, 15.0)) {
            chest.active = false;
            nextChestTime = nowT + kChestInterval;
        }

        // Chest pickup by player (camera position)
        if (tryCollectChest(chest, cameraPos, 0.8f, prizes, nowT, nextChestTime, static_cast<float>(kChestInterval), static_cast<float>(kChestInterval))) {
            std::cout << "Chest collected! prizes=" << prizes << std::endl;
            if (audioReady) audio.play("chest_pickup");
        }

        {
            char title[160];
            float spd = std::fabs(boat.speed);
            std::snprintf(title, sizeof(title), "Water Demo | Mode: %s | Boat speed: %.2f | Fish: %d | Prizes: %d",
                          boatMode ? "Boat" : "Free", spd, fishCaught, prizes);
            glfwSetWindowTitle(window, title);
        }

        // Model matrices for this frame
        Mat4 modelCube = Mat4::translate(cubePos);
        Mat4 modelCube2 = Mat4::translate(cube2Pos);
        Mat4 modelChest = Mat4::identity();
        if (chest.active) {
            modelChest = Mat4::translate(chest.pos) *
                         Mat4::rotateY(timef * 0.5f) *
                         Mat4::scale(Vec3(0.25f, 0.25f, 0.25f));
        }
        // Scale down/imported meshes so they fit the scene/water plane
        constexpr float kBoatModelYawOffsetDeg = 180.0f; // align mesh nose with physics forward
        Mat4 modelBoat = Mat4::translate(boat.pos) *
                         Mat4::rotateY((boat.yawDeg + kBoatModelYawOffsetDeg) * (kPi / 180.0f)) *
                         Mat4::rotateX(-kPi * 0.5f) *
                         Mat4::scale(Vec3(0.016f, 0.016f, 0.016f));
        const float tileSize = halfSize * 2.0f;
        const int tileRadius = 3;
        float baseX = std::floor(cameraPos.x / tileSize) * tileSize;
        float baseZ = std::floor(cameraPos.z / tileSize) * tileSize;
        std::vector<Mat4> groundModels;
        groundModels.reserve((2 * tileRadius + 1) * (2 * tileRadius + 1));
        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;
                groundModels.push_back(Mat4::translate(Vec3(tileX, kGroundY, tileZ)));
            }
        }

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
        glUniformMatrix4fv(shadowU.lightVP, 1, GL_FALSE, lightVP.m.data());

        // Ground tiles
        glBindVertexArray(ground.vao);
        for (const auto &modelGroundTile : groundModels) {
            glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelGroundTile.m.data());
            glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
        }

        // Cube 1
        glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelCube.m.data());
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube 2
        glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelCube2.m.data());
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Boat
        glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelBoat.m.data());
        glBindVertexArray(boatMesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, boatMesh.vertexCount);

        // Fish
        for (const auto &f : fish) {
            if (!f.active) continue;
            Mat4 modelFish = Mat4::translate(f.pos) *
                             Mat4::rotateY((f.yawDeg + 180.0f) * (kPi / 180.0f)) *
                             Mat4::rotateX(-kPi * 0.5f) *
                             Mat4::scale(Vec3(0.03f, 0.03f, 0.03f));
            glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelFish.m.data());
            glBindVertexArray(fishMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, fishMesh.vertexCount);
        }

        // Rod shadow (small red cube)
        if (rod.active) {
            Mat4 modelRod = Mat4::translate(rod.pos) * Mat4::scale(Vec3(0.12f, 0.12f, 0.12f));
            glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelRod.m.data());
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Skipping stones
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelStone.m.data());
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Chest in shadow map
        if (chest.active) {
            glUniformMatrix4fv(shadowU.model, 1, GL_FALSE, modelChest.m.data());
            glBindVertexArray(chestMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, chestMesh.vertexCount);
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --------- Scene prepass (color/depth for refraction) ---------
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFb.fbo);
        glViewport(0, 0, sceneFb.width, sceneFb.height);
        glClearColor(0.08f, 0.1f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(sceneProgram);
        glUniform1i(sceneU.shadowMap, 5);
        glUniform1i(sceneU.texture, 0);
        glUniformMatrix4fv(sceneU.viewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform3f(sceneU.lightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(sceneU.eyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(sceneU.clipY, kWaterHeight);
        glUniform1i(sceneU.useClip, 0);
        glUniform1f(sceneU.waterHeight, kWaterHeight);
        glUniform1i(sceneU.underwater, underwater ? 1 : 0);
        glUniform3f(sceneU.fogColorAbove, 0.6f, 0.75f, 0.9f);
        glUniform3f(sceneU.fogColorBelow, 0.02f, 0.10f, 0.14f);
        glUniform1f(sceneU.fogStart, 20.0f);
        glUniform1f(sceneU.fogEnd,   90.0f);
        glUniform1f(sceneU.underFogDensity, 0.06f);
        glUniformMatrix4fv(sceneU.lightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1f(sceneU.time, timef);
        glUniform1i(sceneU.useTexture, 0);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Ground tiles
        glUniform3f(sceneU.color, 0.35f, 0.55f, 0.35f);
        glBindVertexArray(ground.vao);
        for (const auto &modelGroundTile : groundModels) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelGroundTile.m.data());
            glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
        }

        // Cube 1
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(sceneU.color, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube 2
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(sceneU.color, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Boat
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelBoat.m.data());
        {
            const bool boatHasTex = boatTexture != 0;
            glUniform1i(sceneU.useTexture, boatHasTex ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, boatTexture);
            if (boatHasTex) {
                glUniform3f(sceneU.color, 1.0f, 1.0f, 1.0f);
            } else {
                glUniform3f(sceneU.color, 0.65f, 0.35f, 0.25f);
            }
            glBindVertexArray(boatMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, boatMesh.vertexCount);
            glUniform1i(sceneU.useTexture, 0);
        }

        // Fish
        for (const auto &f : fish) {
            if (!f.active) continue;
            float rollRad = std::clamp(-f.yawVel * 0.005f, -0.4f, 0.4f); // bank with turn
            Mat4 modelFish = Mat4::translate(f.pos) *
                             Mat4::rotateY((f.yawDeg + 180.0f) * (kPi / 180.0f)) *
                             Mat4::rotateX(-kPi * 0.5f) *
                             Mat4::rotateZ(rollRad) *
                             Mat4::scale(Vec3(0.03f, 0.03f, 0.03f));
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelFish.m.data());
            glUniform1i(sceneU.useTexture, fishTexture ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fishTexture);
            if (fishTexture) {
                glUniform3f(sceneU.color, 1.0f, 1.0f, 1.0f);
            } else {
                glUniform3f(sceneU.color, 0.6f, 1.0f, 1.4f);
            }
            glBindVertexArray(fishMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, fishMesh.vertexCount);
        }
        glUniform1i(sceneU.useTexture, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Stones prepass
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(sceneU.color, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Chest prepass
        if (chest.active) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelChest.m.data());
            glUniform3f(sceneU.color, 0.6f, 0.4f, 0.15f);
            glBindVertexArray(chestMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, chestMesh.vertexCount);
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
        glUniform1i(sceneU.shadowMap, 5);
        glUniform1i(sceneU.texture, 0);
        glUniformMatrix4fv(sceneU.viewProj, 1, GL_FALSE, reflViewProj.m.data());
        glUniform3f(sceneU.lightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(sceneU.eyePos, reflPos.x, reflPos.y, reflPos.z);
        glUniform1f(sceneU.clipY, kWaterHeight);
        glUniform1i(sceneU.useClip, 1);
        glUniformMatrix4fv(sceneU.lightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1i(sceneU.underwater, 0);
        glUniform1f(sceneU.time, timef);
        glUniform1i(sceneU.useTexture, 0);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);

        // Ground tiles reflected
        glUniform3f(sceneU.color, 0.35f, 0.55f, 0.35f);
        glBindVertexArray(ground.vao);
        for (const auto &modelGroundTile : groundModels) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelGroundTile.m.data());
            glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
        }

        // Cube1
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(sceneU.color, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube2
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(sceneU.color, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Boat reflected
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelBoat.m.data());
        {
            const bool boatHasTex = boatTexture != 0;
            glUniform1i(sceneU.useTexture, boatHasTex ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, boatTexture);
            if (boatHasTex) {
                glUniform3f(sceneU.color, 1.0f, 1.0f, 1.0f);
            } else {
                glUniform3f(sceneU.color, 0.65f, 0.35f, 0.25f);
            }
            glBindVertexArray(boatMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, boatMesh.vertexCount);
            glUniform1i(sceneU.useTexture, 0);
        }

        // Fish
        for (const auto &f : fish) {
            if (!f.active) continue;
            Mat4 modelFish = Mat4::translate(f.pos) *
                             Mat4::rotateY((f.yawDeg + 180.0f) * (kPi / 180.0f)) *
                             Mat4::rotateX(-kPi * 0.5f) *
                             Mat4::scale(Vec3(0.03f, 0.03f, 0.03f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelFish.m.data());
            glUniform1i(sceneU.useTexture, fishTexture ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fishTexture);
            Vec3 fishBaseColor = fishTexture ? Vec3(1.0f, 1.0f, 1.0f) : Vec3(0.6f, 1.0f, 1.4f);
            glUniform3f(sceneU.color, fishBaseColor.x, fishBaseColor.y, fishBaseColor.z);
            glBindVertexArray(fishMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, fishMesh.vertexCount);
        }
        glUniform1i(sceneU.useTexture, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Stones reflected
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(sceneU.color, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Chest reflected
        if (chest.active) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelChest.m.data());
            glUniform3f(sceneU.color, 0.6f, 0.4f, 0.15f);
            glBindVertexArray(chestMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, chestMesh.vertexCount);

            // Glow column (reflective, does not cast shadow)
            Mat4 glowModel = Mat4::translate(chest.pos + Vec3(0.0f, 3.0f, 0.0f)) *
                             Mat4::scale(Vec3(0.55f, 6.0f, 0.55f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, glowModel.m.data());
            glUniform3f(sceneU.color, 1.0f, 0.9f, 0.4f);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
            glDisable(GL_BLEND);
            glEnable(GL_CULL_FACE);
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
        float sunHeight = sunHeightClamped;

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

        glUniform3f(skyU.top, topColor.x, topColor.y, topColor.z);
        glUniform3f(skyU.horizon, horizonColor.x, horizonColor.y, horizonColor.z);
        glUniform1i(skyU.underwater, underwater ? 1 : 0);
        glUniform1f(skyU.sunHeight, sunHeight);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // Scene geometry to HDR
        glUseProgram(sceneProgram);
        glUniform1i(sceneU.shadowMap, 5);
        glUniform1i(sceneU.texture, 0);
        glUniformMatrix4fv(sceneU.viewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform3f(sceneU.lightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(sceneU.eyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(sceneU.clipY, kWaterHeight);
        glUniform1i(sceneU.useClip, 0);
        glUniform1f(sceneU.waterHeight, kWaterHeight);
        glUniform1i(sceneU.underwater, underwater ? 1 : 0);
        glUniform3f(sceneU.fogColorAbove, 0.6f, 0.75f, 0.9f);
        glUniform3f(sceneU.fogColorBelow, 0.02f, 0.10f, 0.14f);
        glUniform1f(sceneU.fogStart, 20.0f);
        glUniform1f(sceneU.fogEnd,   90.0f);
        glUniform1f(sceneU.underFogDensity, 0.06f);
        glUniformMatrix4fv(sceneU.lightVP, 1, GL_FALSE, lightVP.m.data());
        glUniform1f(sceneU.time, timef);
        glUniform1i(sceneU.useTexture, 0);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Chest glow in screen space (non-interactive)
        if (chest.active) {
            Vec4 clip = viewProj * Vec4(chest.pos.x, chest.pos.y + 0.5f, chest.pos.z, 1.0f);
            if (clip.w > 0.0f) {
                float invW = 1.0f / clip.w;
                float ndcX = clip.x * invW;
                float ndcY = clip.y * invW;
                float sx = (ndcX * 0.5f + 0.5f) * static_cast<float>(fbWidth);
                float sy = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(fbHeight);
                ImDrawList* dl = ImGui::GetBackgroundDrawList();
                ImU32 outerCol = IM_COL32(255, 215, 140, 60);
                ImU32 innerCol = IM_COL32(255, 240, 180, 110);
                ImVec2 p0(sx - 12.0f, sy - 160.0f);
                ImVec2 p1(sx + 12.0f, sy);
                dl->AddRectFilled(p0, p1, outerCol, 6.0f);
                ImVec2 p2(sx - 6.0f, sy - 130.0f);
                ImVec2 p3(sx + 6.0f, sy);
                dl->AddRectFilled(p2, p3, innerCol, 4.0f);
            }
        }

        // Ground tiles
        glUniform3f(sceneU.color, 0.35f, 0.55f, 0.35f);
        glBindVertexArray(ground.vao);
        for (const auto &modelGroundTile : groundModels) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelGroundTile.m.data());
            glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);
        }

        // Cube1
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube.m.data());
        glUniform3f(sceneU.color, 0.85f, 0.3f, 0.2f);
        glBindVertexArray(cube.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        // Cube2
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelCube2.m.data());
        glUniform3f(sceneU.color, 0.2f, 0.4f, 0.85f);
        glBindVertexArray(cube2.vao);
        glDrawArrays(GL_TRIANGLES, 0, cube2.vertexCount);

        // Boat
        glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelBoat.m.data());
        {
            const bool boatHasTex = boatTexture != 0;
            glUniform1i(sceneU.useTexture, boatHasTex ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, boatTexture);
            if (boatHasTex) {
                glUniform3f(sceneU.color, 1.0f, 1.0f, 1.0f);
            } else {
                glUniform3f(sceneU.color, 0.65f, 0.35f, 0.25f);
            }
            glBindVertexArray(boatMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, boatMesh.vertexCount);
            glUniform1i(sceneU.useTexture, 0);
        }

        // Fish
        for (const auto &f : fish) {
            if (!f.active) continue;
            Mat4 modelFish = Mat4::translate(f.pos) *
                             Mat4::rotateY((f.yawDeg + 180.0f) * (kPi / 180.0f)) *
                             Mat4::rotateX(-kPi * 0.5f) *
                             Mat4::scale(Vec3(0.03f, 0.03f, 0.03f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelFish.m.data());
            glUniform1i(sceneU.useTexture, fishTexture ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fishTexture);
            glUniform3f(sceneU.color, 0.6f, 1.0f, 1.4f);
            glBindVertexArray(fishMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, fishMesh.vertexCount);
        }
        glUniform1i(sceneU.useTexture, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Skipping stones
        for (int i = 0; i < kMaxStones; ++i) {
            const Stone &s = g_stones[i];
            if (!s.active) continue;

            Mat4 modelStone = Mat4::translate(s.pos) * Mat4::scale(Vec3(0.25f, 0.05f, 0.25f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelStone.m.data());
            glUniform3f(sceneU.color, 0.65f, 0.65f, 0.7f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Rod in main HDR pass (small red cube)
        if (rod.active) {
            Mat4 modelRod = Mat4::translate(rod.pos) * Mat4::scale(Vec3(0.12f, 0.12f, 0.12f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelRod.m.data());
            glUniform3f(sceneU.color, 0.9f, 0.2f, 0.2f);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }

        // Chest
        if (chest.active) {
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, modelChest.m.data());
            glUniform3f(sceneU.color, 0.6f, 0.4f, 0.15f);
            glBindVertexArray(chestMesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, chestMesh.vertexCount);

            // Glow column visible above water
            Mat4 glowModel = Mat4::translate(chest.pos + Vec3(0.0f, 3.0f, 0.0f)) *
                             Mat4::scale(Vec3(0.55f, 6.0f, 0.55f));
            glUniformMatrix4fv(sceneU.model, 1, GL_FALSE, glowModel.m.data());
            glUniform3f(sceneU.color, 1.0f, 0.9f, 0.4f);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBindVertexArray(cube.vao);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
            glDisable(GL_BLEND);
            glEnable(GL_CULL_FACE);
        }

        // Water surface (tiled around the camera for "infinite" lake)
        glUseProgram(waterProgram);

        glUniformMatrix4fv(waterU.viewProj, 1, GL_FALSE, viewProj.m.data());
        glUniform1f(waterU.time, timef);
        glUniform1f(waterU.move, timef * 0.03f);
        Vec3 waterDeepDay(0.05f, 0.2f, 0.35f);
        Vec3 waterDeepNight(0.02f, 0.05f, 0.12f);
        float nightFactor = 1.0f - sunHeight;
        Vec3 waterDeep = Vec3(waterDeepDay.x * (1 - nightFactor) + waterDeepNight.x * nightFactor,
                              waterDeepDay.y * (1 - nightFactor) + waterDeepNight.y * nightFactor,
                              waterDeepDay.z * (1 - nightFactor) + waterDeepNight.z * nightFactor);
        glUniform3f(waterU.deepColor, waterDeep.x, waterDeep.y, waterDeep.z);
        glUniform3f(waterU.lightDir, sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(waterU.eyePos, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniformMatrix4fv(waterU.reflVP, 1, GL_FALSE, reflViewProj.m.data());
        glUniformMatrix4fv(waterU.viewProjScene, 1, GL_FALSE, viewProj.m.data());
        glUniform1f(waterU.nearZ, 0.1f);
        glUniform1f(waterU.farZ, 200.0f);
        glUniform1f(waterU.roughness, 0.25f);
        glUniform1f(waterU.fresnelBias, 0.04f);
        glUniform1f(waterU.fresnelScale, 0.85f);
        glUniform3f(waterU.foamColor, 0.8f, 0.85f, 0.9f);
        glUniform1f(waterU.foamIntensity, 0.15f);
        glUniform1f(waterU.normalScale, 0.5f);
        glUniform1f(waterU.reflDistort, 0.4f);
        glUniform1f(waterU.refrDistort, 0.25f);
        glUniform1i(waterU.underwater, underwater ? 1 : 0);
        glUniformMatrix4fv(waterU.lightVP, 1, GL_FALSE, lightVP.m.data());
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
            glUniform1i(waterU.rippleCount, rippleCount);
            if (rippleCount > 0 && waterU.ripples >= 0) {
                glUniform4fv(waterU.ripples, rippleCount, rippleBuf.data());
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionFb.colorTex);
        glUniform1i(waterU.refl, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneFb.colorTex);
        glUniform1i(waterU.sceneTex, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sceneFb.depthTex);
        glUniform1i(waterU.sceneDepth, 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, waterNormalTex);
        glUniform1i(waterU.normalMap, 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, waterDudvTex);
        glUniform1i(waterU.dudvMap, 4);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
        glUniform1i(waterU.shadowMap, 5);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(waterMesh.vao);

        for (int dz = -tileRadius; dz <= tileRadius; ++dz) {
            for (int dx = -tileRadius; dx <= tileRadius; ++dx) {
                float tileX = baseX + dx * tileSize;
                float tileZ = baseZ + dz * tileSize;

                Mat4 modelWater = Mat4::translate(Vec3(tileX, kWaterHeight, tileZ));
                glUniformMatrix4fv(waterU.model, 1, GL_FALSE, modelWater.m.data());
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
        glUniform1i(brightU.hdr, 0);
        glUniform1f(brightU.threshold, 1.2f);

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
        glUniform2f(shaftU.sunPos, sunScreen.x, sunScreen.y);
        glUniform1f(shaftU.decay, 0.95f);
        glUniform1f(shaftU.density, 0.9f);
        glUniform1f(shaftU.weight, 0.25f);
        glUniform1f(shaftU.exposure, 0.6f);
        glUniform1i(shaftU.underwater, underwater ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneFb.depthTex);
        glUniform1i(shaftU.depth, 0);
        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisable(GL_BLEND);

        // Blur ping-pong
        glUseProgram(blurProgram);
        glUniform2f(blurU.texelSize,
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

            glUniform1i(blurU.horizontal, horizontal ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            if (firstIteration) {
                glBindTexture(GL_TEXTURE_2D, bloomFb[0].colorTex);
                firstIteration = false;
            } else {
                glBindTexture(GL_TEXTURE_2D, sourceFb.colorTex);
            }
            glUniform1i(blurU.image, 0);

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
        glUniform1i(toneU.hdr, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lastBloomTex);
        glUniform1i(toneU.bloom, 1);

        glUniform1f(toneU.exposure, 1.0f);
        glUniform1f(toneU.bloomStrength, 0.8f);
        glUniform1f(toneU.gamma, 2.2f);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // FXAA from LDR buffer to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbWidth, fbHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(fxaaProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ldrFb.colorTex);
        glUniform1i(fxaaU.image, 0);
        glUniform2f(fxaaU.texelSize,
                    1.0f / fbWidth,
                    1.0f / fbHeight);

        glBindVertexArray(fsQuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // ImGui rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    destroyMesh(ground);
    destroyMesh(cube);
    destroyMesh(cube2);
    destroyMesh(waterMesh);
    destroyMesh(boatMesh);
    destroyMesh(fishMesh);
    destroyMesh(chestMesh);

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

    if (audioReady) audio.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
