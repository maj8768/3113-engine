#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 410
#else

#define GLSL_VERSION 100
#endif

#include "raylib.h"
#include "rlgl.h"

#include "util.h"

#include "physics/physics.h"
#include "physics/car.h"
#include "physics/sounds.h"

#include "game/player.h"
#include "game/game.h"
#include "game/attack_wizard.h"
#include "game/item.h"

#include "draw/draw.h"
#include "draw/gui.h"

#include "camera/camera.h"

#include "particles/particles.h"

#include "system/keyboard/keyboard.h"
#include "system/mouse/mouse.h"

#include <cmath>
#include <iostream>
#include <thread>
#include <cstring>

int thrust[] = {249, 250, 119, 120};
int alt[] = {99, 101, 227, 229};
int fuel[] = {47, 49, 173, 174};

float p = 200;
float size = 15;

enum AppStatus {TERMINATED, RUNNING};

constexpr char EDELGARD_FP[] = "test2.png";

vector2 gPosition = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
vector2 gScale = {250.0f, 250.0f};

static float gPreviousTicks = 0.0f;

float x = 0.0f;
float z = 0.0f;
float s = 2.0f;
float h = 2.0f;

float px = -2.f;
float pz = -2.f;
float ps = 4.f;
float ph = 0.f;

float px2 = -2.f;
float pz2 = 2.f;
float ps2 = 4.f;
float ph2 = 0.f;

static player player1;

static planeMtx plane;
static planeMtx plane2;

static shaderStore w2sShader;
static Shader depthShader;
static Texture2D gasPumpPortrait = {0};

static int depthLightSpaceLoc = -1;
static int depthModelLoc = -1;

static RenderTexture2D shadowMapRT = {0};
static RenderTexture2D shadowMapRTFar = {0};

// --- Emissive bloom ---------------------------------------------------------
// The scene is rendered a second time in "emissive only" mode into emissiveRT
// (half res), blurred by bloomShader, and added additively over the scene.
static RenderTexture2D emissiveRT = {0};
static Shader bloomShader = {0};
static int bloomResLoc = -1;
static int bloomIntensityLoc = -1;
static int bloomSpreadLoc = -1;
static int gEmissiveOnly = 0;          // toggles the w2s shader's emissive-only output
static float bloomIntensity = 0.3f;    // master corona strength: peak = emissive*amount*bloom.x*this (tunable)

// --- Physical light corona (bloom) ------------------------------------------
// Emissive objects are drawn as analytic radial glows (glow.fs) — constant peak
// brightness, world-relative size. Replaces the camera-dependent screen blur.
static Shader glowShader = {0};
static int glowCountLoc = -1, glowCenterLoc = -1, glowRadiusLoc = -1, glowColorLoc = -1;
static int glowDepthArrLoc = -1, glowSceneDepthLoc = -1, glowScreenLoc = -1;
static int glowProjALoc = -1, glowProjBLoc = -1, glowBiasLoc = -1, glowOccRadiusLoc = -1;

static Shader glowBBShader = {0};
static int glowBBColorLoc = -1;
static int glowBBSceneDepthLoc = -1;
static int glowBBScreenLoc = -1;
static int glowBBProjALoc = -1;
static int glowBBProjBLoc = -1;
static int glowBBFadeLoc = -1;
static RenderTexture2D sceneDepthRT = {0}; // camera-view depth for the soft glow
static Shader depthViewShader = {0};       // DEBUG: amplified depth visualiser
static float bloomSpread = 32.0f;      // GLOBAL max blur radius in texels (the halo cap).
                                       // A world-space bloom length projects to more texels
                                       // the closer the camera is; it's clamped to this.
static constexpr int SHADOW_MAP_NEAR = 2048;
static constexpr int SHADOW_MAP_FAR = 1024;
static constexpr float SHADOW_RANGE_NEAR = 80.0f;
static constexpr float SHADOW_RANGE_FAR = 300.0f;
static constexpr float CASCADE_SPLIT = 65.0f;
static constexpr float SHADOW_NEAR = 1.0f;
static constexpr float SHADOW_FAR = 50 * 2.2f;
static constexpr float LIGHT_ORBIT_SPEED = 0.15f;
static constexpr float LIGHT_ORBIT_RADIUS_Y = 150.0f;
static constexpr float LIGHT_ORBIT_RADIUS_Z = 25.0f;
static constexpr float LIGHT_ORBIT_X_OFFSET = 55.0f;

static vector3 lightPos = {0.f, 15.f, 0.f};
static vector3 lightDir = {-0.5f, -0.707f, 0.5f};
static vector4 lightColor = {1.0f, 1.0f, 1.0f, 1.0f};
static float ambient = 0.05f;

// POINT-LIGHT SUN: the sun now radiates from the world position `lightPos` above.
// lightRange is the distance at which its brightness falls to zero (tunable).
static float lightRange = 30.f;

static meshedObject skysphere1;
static meshedObject cube;
static meshedObject testingplatforms;
static meshedObject drank;
static meshedObject wand;
static item wandItem;

// do not delete

static meshedObject fakeMesh;

static bool iamreal = false;
static int iamalsoreal = 1;

// continue normal code

gameData gData = {
    .fadeTo = 1.0f,
        .isDying = false,
        .gameStarted = false,
        .gameEnded = false,
        .infommercial = false,
        .bgMusicLevel1 = false,
        .bgMusicLevel2 = false,
        .bgMusicLevel3 = false,
        .hasAudioLevel1 = false,
        .hasAudioLevel2 = false,
        .hasAudioLevel3 = false,
        .hasChairScared = false,
        .currentLevel = gameData::TESTING_ENVIRONMENT,
        .lives = 3
};

static Sound jump1;
static Sound jump2;
static Sound jump3;
static Sound jump4;
static Sound jump5;
static Sound jump6;
static Sound jump7;

static Sound swallow;

static Music carEngine;

AppStatus gAppStatus = RUNNING;

void initialise();
void processInput();
void update();
void render();
void shutdown();
static mtx44 BuildLightSpaceMatrix();
static void RenderShadowMapPass(const mtx44& lightSpace);
static void DrawSceneWithShadows(const mtx44& frameVP, const mtx44& lightSpace);
void progressGame();
void failGame();

enum TransitionPhase {TRANS_NONE, TRANS_FADE_OUT, TRANS_FADE_IN};
static TransitionPhase transPhase = TRANS_NONE;
static gameData::levels transTarget = gameData::GAMESTART;
static bool transIsFail = false;
static float fadeOverlayAlpha = 0.0f;
static constexpr float FADE_OUT_SPEED = 480.0f;
static constexpr float FADE_IN_SPEED = 55.0f;

static Music bgMusic1 = {0};
static Music bgMusic2 = {0};
static Music bgMusic3 = {0};
static Music* currentBgMusic = nullptr;

static RenderTexture2D LoadShadowMapRenderTexture(int width, int height) {
    RenderTexture2D target = {0};

    target.id = rlLoadFramebuffer();
    if (target.id <= 0) return target;

    rlEnableFramebuffer(target.id);

    target.texture.id = rlLoadTexture(nullptr, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    target.texture.mipmaps = 1;

    target.depth.id = rlLoadTextureDepth(width, height, false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = PIXELFORMAT_UNCOMPRESSED_R32;
    target.depth.mipmaps = 1;

    rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(target.id)) {
        std::cout << "Shadow map framebuffer incomplete!" << std::endl;
    }

    rlDisableFramebuffer();

    SetTextureFilter(target.depth, TEXTURE_FILTER_POINT);
    SetTextureWrap(target.depth, TEXTURE_WRAP_CLAMP);

    return target;
}

static mtx44 BuildLightSpaceMatrix(float range) {
    vector3 sceneCenter = player1.pEntity.location;
    // vector3 sunDir = normalize3({lightDir.x, lightDir.y, lightDir.z});
    // if (len3(sunDir) <= eps) sunDir = {0.f, -1.f, 0.f};
    // vector3 lightPos3 = {
    //     sceneCenter.x - sunDir.x * SUN_DIST,
    //         sceneCenter.y - sunDir.y * SUN_DIST,
    //         sceneCenter.z - sunDir.z * SUN_DIST
    // };
    // lightPos = {lightPos3.x, lightPos3.y, lightPos3.z};
    mtx44 lightView = lookAtMtx44(lightPos, sceneCenter, {0.f, 1.f, 0.f});
    mtx44 lightProj = orthoMtx44(-range, range, -range, range, SHADOW_NEAR, SHADOW_FAR);
    return mmult4(lightProj, lightView);
}

void initializePlayer(player& player1) {

    DisableCursor();

    player1.pEntity.location = {0.f, 10.0f, 0.f};
    player1.camera.camPos = {0.f, 10.0f, 0.f};
    player1.camera.camTarget = {M_PI/2.f,0,0};
    player1.camera.up = {0,1,0};
    player1.camera.aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    player1.camera.fov = 90.0f * M_PI / 180.0f;
    player1.controls = {'W', 'A', 'S', 'D'};
    player1.canMove = true;
    player1.pState = {false, false, 0.f, 0.f, 100.f};

    // Box collider (playercollider_box.obj) — avoids the sideways-weave the pointed
    // collider caused. Swap back to "playercollider.obj" to restore the old shape.
    objToQuads("resources/player/collider/playercollider_box.obj", fakeMesh, 1.0f, player1, true);
}

void createPlane(planeMtx& plane, int id, vector3 location, float dimensions[4][3], Texture2D texture, void (*action)(int)) {
    plane.id = id;
    plane.texture = texture;
    plane.action = action;
    plane.m[0][0] = location.x + dimensions[0][0], plane.m[0][1] = location.y + dimensions[0][1], plane.m[0][2] = location.z + dimensions[0][2];
    plane.m[1][0] = location.x + dimensions[1][0], plane.m[1][1] = location.y + dimensions[1][1], plane.m[1][2] = location.z + dimensions[1][2];
    plane.m[2][0] = location.x + dimensions[2][0], plane.m[2][1] = location.y + dimensions[2][1], plane.m[2][2] = location.z + dimensions[2][2];
    plane.m[3][0] = location.x + dimensions[3][0], plane.m[3][1] = location.y + dimensions[3][1], plane.m[3][2] = location.z + dimensions[3][2];
    plane.color = BLACK;
}

void create3dObject(meshedObject& object, const char* path, const char* colliderPath, bool collider, shaderStore& shader, float scale, vector3 location, char text[100], bool renderText, float textRenderDistance, vector3 relativeTextOffset) {
    memcpy(object.text, text, 100);
    object.renderText = renderText;
    object.textRenderDistance = textRenderDistance;
    object.relativeTextOffset = relativeTextOffset;
    triDomMesh mesh = {nullptr, nullptr, 0};
    object.scale = scale;
    object.pEntity.location = location;
    if (collider) objToQuads(colliderPath, object, scale, player1, false);

    std::cout << "loading: " << path << std::endl;
    Model model = LoadModel(path);
    std::cout << "meshes: " << model.meshCount << std::endl;

    int totalTriangles = 0;
    if (model.meshes) {
        for (int i = 0; i < model.meshCount; i++)
        totalTriangles += model.meshes[i].triangleCount;
    }
    std::cout << "triangles: " << totalTriangles << std::endl;

    if (totalTriangles <= 0) {
        std::cout << "warning: no triangles in " << path << "\n";
        object.mesh = mesh;
        UnloadModel(model);
        return;
    }

    mesh.tris = (tri*)malloc(totalTriangles * sizeof(tri));
    mesh.trisO = (tri*)malloc(totalTriangles * sizeof(tri));
    mesh.count = 0;

    if (!mesh.tris || !mesh.trisO) {
        std::cout << "error: malloc failed for " << path << "\n";
        free(mesh.tris); free(mesh.trisO);
        UnloadModel(model);
        return;
    }

    for (int i = 0; i < model.meshCount; i++) {
        Mesh* m = &model.meshes[i];
        if (!m->vertices || m->triangleCount <= 0) continue;
        if (model.materials && model.meshMaterial && model.materialCount > 0) {
            int matIdx = model.meshMaterial[i];
            if (matIdx >= 0 && matIdx < model.materialCount)
            object.texo = model.materials[matIdx].maps[MATERIAL_MAP_DIFFUSE].texture;
        }
        bool indexed = (m->indices != nullptr);
        for (int j = 0; j < m->triangleCount; j++) {
            tri t;
            tri ot;
            int a, b, c;
            if (indexed) {
                a = m->indices[j*3+0];
                b = m->indices[j*3+1];
                c = m->indices[j*3+2];
            } else {
                a = j*3+0; b = j*3+1; c = j*3+2;
            }
            if (a >= m->vertexCount || b >= m->vertexCount || c >= m->vertexCount) continue;
            t.v[0] = {m->vertices[a*3+0]*scale+location.x, m->vertices[a*3+1]*scale+location.y, m->vertices[a*3+2]*scale+location.z};
            t.v[1] = {m->vertices[b*3+0]*scale+location.x, m->vertices[b*3+1]*scale+location.y, m->vertices[b*3+2]*scale+location.z};
            t.v[2] = {m->vertices[c*3+0]*scale+location.x, m->vertices[c*3+1]*scale+location.y, m->vertices[c*3+2]*scale+location.z};
            if (m->normals) {
                t.n[0] = {m->normals[a*3+0], m->normals[a*3+1], m->normals[a*3+2]};
                t.n[1] = {m->normals[b*3+0], m->normals[b*3+1], m->normals[b*3+2]};
                t.n[2] = {m->normals[c*3+0], m->normals[c*3+1], m->normals[c*3+2]};
            } else {
                t.n[0] = t.n[1] = t.n[2] = {0.f, 1.f, 0.f};
            }
            if (m->texcoords) {
                t.t[0] = {m->texcoords[a*2+0], m->texcoords[a*2+1]};
                t.t[1] = {m->texcoords[b*2+0], m->texcoords[b*2+1]};
                t.t[2] = {m->texcoords[c*2+0], m->texcoords[c*2+1]};
            } else {
                t.t[0] = t.t[1] = t.t[2] = {0.f, 0.f};
            }
            ot.v[0] = {m->vertices[a*3+0]*scale, m->vertices[a*3+1]*scale, m->vertices[a*3+2]*scale};
            ot.v[1] = {m->vertices[b*3+0]*scale, m->vertices[b*3+1]*scale, m->vertices[b*3+2]*scale};
            ot.v[2] = {m->vertices[c*3+0]*scale, m->vertices[c*3+1]*scale, m->vertices[c*3+2]*scale};
            ot.n[0] = t.n[0]; ot.n[1] = t.n[1]; ot.n[2] = t.n[2];
            ot.t[0] = t.t[0]; ot.t[1] = t.t[1]; ot.t[2] = t.t[2];
            mesh.tris[mesh.count] = t;
            mesh.trisO[mesh.count] = ot;
            mesh.count++;
        }
    }
    std::cout << "loaded: " << path << " (" << mesh.count << " tris)\n";
    object.mesh = mesh;
    UnloadModel(model);
}

static meshedObject* worldObjects[] = {&testingplatforms};
static meshedObject* secondaryObjects[] = {&testingplatforms};
/*
static meshedObject* worldObjects[] = {&uberGirl, &uberGuy, &testingplatforms, &testcar, &gasPump, &cashRegister, &dragan, &gasStation, &apt1, &apt2, &sketchyBuilding, &farm, &spawnToilet};
static meshedObject* secondaryObjects[] = {&uberGirl, &uberGuy, &testingplatforms, &gasPump, &gasStation, &apt1, &apt2, &sketchyBuilding, &farm, &spawnToilet};
*/
static world worldInstance = {nullptr, 0};
static world secondaryInstance = {nullptr, 0};

// Objects that emit physical light (point lights). Each one with emissive.t > 0
// lights nearby geometry within its bloom length (bloom.y, world-space radius).
static constexpr int MAX_POINT_LIGHTS = 8; // must match #define in w2s.fs
static meshedObject* gEmitters[] = {&wand};

static int gTorchEmitterId = -1;

void initialise()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");
    InitAudioDevice();

    jump1 = LoadSound("resources/sounds/jumps/j1.mp3");
    jump2 = LoadSound("resources/sounds/jumps/j2.mp3");
    jump3 = LoadSound("resources/sounds/jumps/j3.mp3");
    jump4 = LoadSound("resources/sounds/jumps/j4.mp3");
    jump5 = LoadSound("resources/sounds/jumps/j5.mp3");
    jump6 = LoadSound("resources/sounds/jumps/j6.mp3");
    jump7 = LoadSound("resources/sounds/jumps/j7.mp3");

    bgMusic1 = LoadMusicStream("resources/sounds/map/bg1.mp3");
    bgMusic2 = LoadMusicStream("resources/sounds/map/bg2.mp3");
    bgMusic3 = LoadMusicStream("resources/sounds/map/bg3.mp3");
    SetMusicVolume(bgMusic1, 0.2f);
    SetMusicVolume(bgMusic2, 0.2f);
    SetMusicVolume(bgMusic3, 0.2f);

    void* handle = GetWindowHandle();
    if (!RawMouseInitFromHWND(handle))
    {
        CloseWindow();
        std::cout << "tragic failure" << std::endl;
        exit(1);
    }
    else {
        std::cout << "mouse initialized successfully" << std::endl;
    }

    Texture2D texo = LoadTexture(EDELGARD_FP);

    static float dimensions[4][3] = {
            {-2,0,-2},
            {-2,0,2},
            {2,0,2},
            {2,0,-2}
    };

    static float dimensions2[4][3] = {
        {-2,0,-2},
            {-2,0,2},
            {2,0,2},
            {2,0,-2}
    };

    initializePlayer(player1);
    SetTargetFPS(FPS);

    Shader w2s = LoadShader("resources/shaders/w2s.vs", "resources/shaders/w2s.fs");

    w2sShader.shader = w2s;
    w2sShader.lightDirLoc = GetShaderLocation(w2sShader.shader, "uLightDir");
    w2sShader.lightPosLoc = GetShaderLocation(w2sShader.shader, "uLightPos");
    w2sShader.lightRangeLoc = GetShaderLocation(w2sShader.shader, "uLightRange");
    w2sShader.emissiveLoc = GetShaderLocation(w2sShader.shader, "uEmissive");
    w2sShader.colorLoc = GetShaderLocation(w2sShader.shader, "uColor");
    w2sShader.lightColorLoc = GetShaderLocation(w2sShader.shader, "uLightColor");
    w2sShader.ambientLoc = GetShaderLocation(w2sShader.shader, "uAmbient");
    w2sShader.normalLoc = GetShaderLocation(w2sShader.shader, "uNormal");
    w2sShader.texoLoc = GetShaderLocation(w2sShader.shader, "uTexo");
    w2sShader.fadeToLoc = GetShaderLocation(w2sShader.shader, "fadeTo");
    w2sShader.vpLoc = GetShaderLocation(w2sShader.shader, "uVP");
    w2sShader.lightSpaceMatrixLoc = GetShaderLocation(w2sShader.shader, "uLightSpaceMatrix");
    w2sShader.shadowMapLoc = GetShaderLocation(w2sShader.shader, "uShadowMap");
    w2sShader.shadowsEnabledLoc = GetShaderLocation(w2sShader.shader, "uShadowsEnabled");
    w2sShader.timeLoc = GetShaderLocation(w2sShader.shader, "uTime");
    w2sShader.drunkennessLoc = GetShaderLocation(w2sShader.shader, "uDrunkenness");
    w2sShader.camPosLoc = GetShaderLocation(w2sShader.shader, "uCamPos");

    w2sShader.shadowMapFarLoc = GetShaderLocation(w2sShader.shader, "uShadowMapFar");
    w2sShader.lightSpaceMatrixFarLoc = GetShaderLocation(w2sShader.shader, "uLightSpaceMatrixFar");
    w2sShader.cascadeSplitLoc = GetShaderLocation(w2sShader.shader, "uCascadeSplit");

    depthShader = LoadShader("resources/shaders/depth.vs", "resources/shaders/depth.fs");
    /* gasPumpPortrait = LoadTexture("resources/levels/testing/gaspfp.PNG"); */
    depthLightSpaceLoc = GetShaderLocation(depthShader, "uLightSpaceMatrix");
    depthModelLoc = GetShaderLocation(depthShader, "uModel");
    w2sShader.modelLoc = GetShaderLocation(w2sShader.shader, "uModel");
    w2sShader.pointPosLoc = GetShaderLocation(w2sShader.shader, "uPointPos");
    w2sShader.pointColorLoc = GetShaderLocation(w2sShader.shader, "uPointColor");
    w2sShader.pointRadiusLoc= GetShaderLocation(w2sShader.shader, "uPointRadius");
    w2sShader.pointCountLoc = GetShaderLocation(w2sShader.shader, "uPointCount");
    w2sShader.drankUrgencyLoc = GetShaderLocation(w2sShader.shader, "uDrankUrgency");
    w2sShader.resolutionLoc   = GetShaderLocation(w2sShader.shader, "uResolution");

    shadowMapRT = LoadShadowMapRenderTexture(SHADOW_MAP_NEAR, SHADOW_MAP_NEAR);
    shadowMapRTFar = LoadShadowMapRenderTexture(SHADOW_MAP_FAR, SHADOW_MAP_FAR);

    // Emissive bloom: half-res buffer + blur shader.
    emissiveRT = LoadRenderTexture(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    SetTextureFilter(emissiveRT.texture, TEXTURE_FILTER_BILINEAR);
    bloomShader = LoadShader(0, "resources/shaders/bloom.fs");
    bloomResLoc = GetShaderLocation(bloomShader, "uResolution");
    bloomIntensityLoc = GetShaderLocation(bloomShader, "uIntensity");
    bloomSpreadLoc = GetShaderLocation(bloomShader, "uSpread");
    glowShader = LoadShader(0, "resources/shaders/glow.fs");
    glowCountLoc  = GetShaderLocation(glowShader, "uGlowCount");
    glowCenterLoc = GetShaderLocation(glowShader, "uGlowCenter");
    glowRadiusLoc = GetShaderLocation(glowShader, "uGlowRadius");
    glowColorLoc  = GetShaderLocation(glowShader, "uGlowColor");
    glowDepthArrLoc  = GetShaderLocation(glowShader, "uGlowDepth");
    glowSceneDepthLoc = GetShaderLocation(glowShader, "uSceneDepth");
    glowScreenLoc    = GetShaderLocation(glowShader, "uScreenSize");
    glowProjALoc     = GetShaderLocation(glowShader, "uProjA");
    glowProjBLoc     = GetShaderLocation(glowShader, "uProjB");
    glowBiasLoc      = GetShaderLocation(glowShader, "uDepthBias");
    glowOccRadiusLoc = GetShaderLocation(glowShader, "uOccludeRadius");

    glowBBShader = LoadShader(0, "resources/shaders/glowbb.fs");
    glowBBColorLoc = GetShaderLocation(glowBBShader, "uGlowColor");
    glowBBSceneDepthLoc = GetShaderLocation(glowBBShader, "uSceneDepth");
    glowBBScreenLoc = GetShaderLocation(glowBBShader, "uScreenSize");
    glowBBProjALoc = GetShaderLocation(glowBBShader, "uProjA");
    glowBBProjBLoc = GetShaderLocation(glowBBShader, "uProjB");
    glowBBFadeLoc = GetShaderLocation(glowBBShader, "uFadeDist");
    sceneDepthRT = LoadShadowMapRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    depthViewShader = LoadShader(0, "resources/shaders/depthview.fs");
    w2sShader.emissiveOnlyLoc = GetShaderLocation(w2sShader.shader, "uEmissiveOnly");
    w2sShader.bloomParamsLoc = GetShaderLocation(w2sShader.shader, "uBloom");
    w2sShader.bloomFocalLoc = GetShaderLocation(w2sShader.shader, "uBloomFocal");
    w2sShader.bloomMaxLoc = GetShaderLocation(w2sShader.shader, "uBloomMax");

    char empty[100] = "";

    create3dObject(testingplatforms, "resources/levels/testing/testplatform.obj", "resources/levels/testing/colliders/testplatformcollider.obj", true, w2sShader, 5.f /* scale */, {0.f,0.f,0.f} /* location */, empty, false, 100, {0,0,0});
    testingplatforms.noCull = true;

    /*
    create3dObject(drank, "resources/levels/testing/items/drank/drank.obj", "", false, w2sShader, 1.f, {-40.f,2.f,0.f}, empty, false, 100, {0,0,0});
    drank.emissive = {0.f, 1.f, 0.f, 0.15f};
    drank.bloom = {5.0f, 10.0f};
    */

    create3dObject(wand, "resources/levels/objects/wand.obj", "resources/levels/objects/colliders/wand_collider.obj", true, w2sShader, 1.f /* scale */, {0.f,2.f,5.f} /* location */, empty, false, 100, {0,0,0});
    wand.emissive = {0.f, 1.f, 0.f, 0.15f}; // glow moved here from drank (green; recolor freely)
    wand.bloom = {5.0f, 10.0f};             // bloom: x = brightness, y = world radius
    wandItem = createItem(&wand, 8.0f /* pickup distance */, 60.0f /* lookat radius (px) */);
    addItem(&wandItem);
    updateColliderLocation(wand, player1, false);

    initializePhysicsEntity(player1.pEntity, 1.f, COMPLEX);

    updateColliderLocation(fakeMesh, player1, true);
    updateColliderLocation(testingplatforms, player1, false);

    buildWorld(worldInstance, worldObjects, 1);
    buildWorld(secondaryInstance, secondaryObjects, 1);

    applyAcceleration({0.f,-20.5f,0.f}, player1.pEntity);

    initParticles();
    gTorchEmitterId = spawnFireEmitter({0.f, 5.f, 0.f});

    initWizardAttacks(w2sShader);

    gPreviousTicks = static_cast<float>(GetTime());
}

static bool pRot = true;

float i = 0;
int g = 1;
float floating = 1;
int target = 0;

bool start = false;
bool warning = false;
bool success = false;
bool intro = false;
float musicVol = 0.2f;
vector2 md;

static void resetLevelState() {
    float currentMoney = player1.pState.money;
    float currentGas = player1.pState.carFuel;
    player1.pEntity.location = {42.6027f, 10.0f, 131.896f};
    player1.camera.camPos = player1.pEntity.location;
    player1.camera.camTarget = {M_PI/2.f, 0.f, 0.f};
    player1.pEntity.magnitude = {0.f, 0.f, 0.f};
    player1.pEntity.jumping = false;
    player1.canMove = true;
    player1.pState = {};
    player1.pState.money = currentMoney;
    player1.pState.carFuel = currentGas;
    drank.pEntity.location = {0.f, 0.f, 0.f};

    updateColliderLocation(fakeMesh, player1, true);
    updateColliderLocation(drank, player1, false);

    resetLevelLogicState();
    bs = {START, ONE, 2};

    gData.fadeTo = 1.0f;
    gData.drankTimer = 170.0f;
    gData.dranksConsumed = 0;
    gData.escapeMenuOpen = false;
    switch (gData.currentLevel) {
        case gameData::LEVEL1: gData.levelTimer = 300.0f; break;
        case gameData::LEVEL2: gData.levelTimer = 450.0f; break;
        case gameData::LEVEL3: gData.levelTimer = 600.0f; break;
        default: gData.levelTimer = 0.0f; break;
    }
}

void progressGame() {
    transIsFail = false;
    transPhase = TRANS_FADE_OUT;
    switch (gData.currentLevel) {
        case gameData::GAMEINFOMERCIAL: transTarget = gameData::LEVEL1; break;
        case gameData::LEVEL1: transTarget = gameData::LEVEL2; break;
        case gameData::LEVEL2: transTarget = gameData::LEVEL3; break;
        case gameData::LEVEL3: transTarget = gameData::GAMEWIN; break;
        default: transTarget = gameData::LEVEL1; break;
    }
}

void failGame() {
    transTarget = gameData::GAMEEND;
    transIsFail = true;
    transPhase = TRANS_FADE_OUT;
}

static const float PHYSICS_DT = 1.0f / 60.0f; // fixed physics timestep (tunable)
static float gPhysicsAccumulator = 0.f;
static vector3 gPrevPlayerLoc, gCurrPlayerLoc; // player location at the last two physics ticks
static bool gCamInterpInit = false;

void update() {

    auto ticks = static_cast<float>(GetTime());
    float frameTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;
    if (frameTime > 0.25f) frameTime = 0.25f; // clamp after a stall (avoid the spiral of death)

    if (!gCamInterpInit) {
        gPrevPlayerLoc = gCurrPlayerLoc = player1.pEntity.location;
        gCamInterpInit = true;
    }

    // Mouse look stays per-frame so the view is as smooth as the framerate allows.
    RawMouseGetDelta(md.x, md.y);
    moveLook(player1, frameTime, md);

    // Physics runs at a FIXED timestep, decoupled from the framerate: bank the real
    // frame time and consume it in constant PHYSICS_DT steps. Deterministic and
    // framerate-independent. Discrete input (firing) is edge-detected inside the
    // step, so it still triggers exactly once per key press.
    gPhysicsAccumulator += frameTime;
    while (gPhysicsAccumulator >= PHYSICS_DT) {
        gPrevPlayerLoc = gCurrPlayerLoc; // remember the previous tick for render interpolation

        int r = rand() % 7 + 1;
        Sound jumpSound = r == 1 ? jump1 : r == 2 ? jump2 : r == 3 ? jump3 : r == 4 ? jump4 : r == 5 ? jump5 : r == 6 ? jump6 : jump7;
        movePlayer(gData, player1, false, PHYSICS_DT, 13, jumpSound);

        updateColliderLocation(fakeMesh, player1, true);
        if (!player1.pState.noClip)
            processPhysics(PHYSICS_DT, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, player1.collider, player1.cPlaneCount);

        wizardCombatUpdate(player1, PHYSICS_DT, worldInstance);
        updateItemsPhysics(player1, PHYSICS_DT, worldInstance);

        gCurrPlayerLoc = player1.pEntity.location;
        gPhysicsAccumulator -= PHYSICS_DT;
    }

    // Render interpolation: draw the camera position BETWEEN the last two physics
    // ticks (alpha = leftover time / step), so motion is smooth at any framerate
    // even though the sim advances in discrete PHYSICS_DT ticks. Look angles
    // (camTarget) are mouse-driven per-frame and need no interpolation. Costs ~one
    // step (PHYSICS_DT) of positional latency in exchange for smoothness.
    if (!player1.pState.noClip && !player1.pState.inCar) {
        float alpha = gPhysicsAccumulator / PHYSICS_DT;
        vector3 camLoc = gPrevPlayerLoc + (gCurrPlayerLoc - gPrevPlayerLoc).fmult(alpha);
        player1.camera.camPos.x = camLoc.x;
        player1.camera.camPos.y = camLoc.y + 7.0f;
        player1.camera.camPos.z = camLoc.z;
    }

    // Held item tracks the finalised (interpolated) camera, so update it here.
    handleItemInput(player1);

    {
        static double lastPrintTime = 0.0;
        double now = GetTime();
        if (now - lastPrintTime >= 0.25) { std::cout << "FPS: " << GetFPS() << "\n"; lastPrintTime = now; }
    }

    updateParticles(frameTime);

    return;

    /*
    skysphere1.pEntity.location = player1.pEntity.location;

    if (transPhase == TRANS_FADE_OUT) {
        fadeOverlayAlpha = std::min(255.0f, fadeOverlayAlpha + FADE_OUT_SPEED * deltaTime);
        if (fadeOverlayAlpha >= 255.0f) {
            gData.currentLevel = transTarget;
            if (!transIsFail) resetLevelState();
            transPhase = TRANS_FADE_IN;
            if (transTarget == gameData::GAMEEND || transTarget == gameData::GAMEWIN) {
                if (currentBgMusic) {StopMusicStream(*currentBgMusic); currentBgMusic = nullptr;}
                StopMusicStream(carEngine);
            }
        }
    } else if (transPhase == TRANS_FADE_IN) {
        fadeOverlayAlpha = std::max(0.0f, fadeOverlayAlpha - FADE_IN_SPEED * deltaTime);
        if (fadeOverlayAlpha <= 0.0f) transPhase = TRANS_NONE;
    }

    bool inGameState = (gData.currentLevel == gameData::LEVEL1 ||
    gData.currentLevel == gameData::LEVEL2 ||
    gData.currentLevel == gameData::LEVEL3 ||
    gData.currentLevel == gameData::LEVEL4 ||
    gData.currentLevel == gameData::TESTING_ENVIRONMENT);

    if (inGameState) {
        RawMouseGetDelta(md.x, md.y);
        moveLook(player1, deltaTime, md);

        int r = rand() % 7 + 1;
        Sound jumpSound = r == 1 ? jump1 : r == 2 ? jump2 : r == 3 ? jump3 : r == 4 ? jump4 : r == 5 ? jump5 : r == 6 ? jump6 : jump7;
        movePlayer(gData, player1, testcar, false, deltaTime, 13, jumpSound);
    }

    g++;
    cube.pEntity.location.x = 1.f + sinf(g * 0.00025) * 5.f;
    cube.pEntity.location.z = -1.f + sinf(g * 0.00025) * 5.f;

    auto runLevelUpdate = [&]() {
        updateUber(deltaTime);
        levelLogic(player1, deltaTime, gData, testcar, gasPump, gasPumpNozzle, gasPumpNozzleOff, drank, buyBox, swallow);

        updateColliderLocation(fakeMesh, player1, true);
        updateColliderLocation(testcar, player1, false);

        if (!player1.pState.inCar && !player1.pState.noClip)
        processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, player1.collider, player1.cPlaneCount);

        processPhysics(deltaTime, 0, testcar.pEntity, secondaryInstance, iamreal, iamalsoreal, false, true, testcar.collider, testcar.cPlaneCount);
        testcarwheel.pEntity = testcar.pEntity;
        processCar(testcar, testcarwheel, player1, deltaTime, carEngine);
        applyRot(testcarwheel.pEntity.location, testcar.pEntity.rot, 0.f, 0.f, 0.f);

        if (uberSys.phase == UBER_RIDING) {
            meshedObject& inCar = uberSys.isFemale ? uberWomanInCar : uberManInCar;
            inCar.pEntity.location = testcar.pEntity.location;
            inCar.pEntity.rot = testcar.pEntity.rot;
        }

        if (!player1.pState.hasDrank)
        processPhysics(deltaTime, 0, drank.pEntity, worldInstance, iamreal, iamalsoreal, false, true, drank.collider, drank.cPlaneCount);

        updateColliderLocation(drank, player1, false);
        updateColliderLocation(buyBox, player1, false);

        if (player1.pState.inCar) {
            player1.pEntity.location = testcar.pEntity.location;
            vector3 camPos = player1.pEntity.location;
            applyRot(camPos, testcar.pEntity.rot, -1.5f, 4.25, -0.45);
            player1.camera.camPos = camPos;
        }
    };

    switch (gData.currentLevel) {
        case gameData::LEVEL1:
            case gameData::LEVEL2:
            case gameData::LEVEL3:
            case gameData::LEVEL4:
            case gameData::TESTING_ENVIRONMENT: {

            static bool canEsc = false;
            if (!getAsyncKeyStateWrapper(KEY_ESCAPE)) canEsc = true;
            if (canEsc && getAsyncKeyStateWrapper(KEY_ESCAPE)) {
                gData.escapeMenuOpen = !gData.escapeMenuOpen;
                canEsc = false;
            }

            if (gData.escapeMenuOpen) {
                getEscapeMenuInput(gData);
                break;
            }

            runLevelUpdate();

            if (gData.currentLevel == gameData::LEVEL1 ||
                gData.currentLevel == gameData::LEVEL2 ||
                gData.currentLevel == gameData::LEVEL3) {

                gData.levelTimer = std::max(0.0f, gData.levelTimer - deltaTime);
                if (gData.levelTimer <= 0.0f && transPhase == TRANS_NONE)
                failGame();

                gData.drankTimer = std::max(0.0f, gData.drankTimer - deltaTime);
                if (gData.drankTimer <= 0.0f && transPhase == TRANS_NONE)
                failGame();

                int required = (gData.currentLevel == gameData::LEVEL1) ? 2
                : (gData.currentLevel == gameData::LEVEL2) ? 3 : 4;
                if (gData.dranksConsumed >= required && transPhase == TRANS_NONE) {
                    static bool canExitF = false;
                    if (!getAsyncKeyStateWrapper(KEY_F)) canExitF = true;
                    if (canExitF && getAsyncKeyStateWrapper(KEY_F)) {
                        if (canInteract(player1, spawnBag.collider, spawnBag.cPlaneCount,
                            6.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 99999.f)) {
                            progressGame();
                            canExitF = false;
                        }
                    }
                }
            }
            break;
        }

        case gameData::GAMESTART:
            getStartingInput(gData);
        break;

        case gameData::GAMEINFOMERCIAL: {
            static bool canEnterInfo = false;
            if (!getAsyncKeyStateWrapper(KEY_ENTER)) canEnterInfo = true;
            if (canEnterInfo && getAsyncKeyStateWrapper(KEY_ENTER)) {
                progressGame();
                canEnterInfo = false;
            }
            break;
        }

        case gameData::GAMEEND: {
            static bool canEnterEnd = false;
            if (!getAsyncKeyStateWrapper(KEY_ENTER)) canEnterEnd = true;
            if (canEnterEnd && getAsyncKeyStateWrapper(KEY_ENTER)) {
                gData = {
                    .fadeTo = 1.0f,
                        .isDying = false,
                        .gameStarted = false,
                        .gameEnded = false,
                        .infommercial = false,
                        .bgMusicLevel1 = false,
                        .bgMusicLevel2 = false,
                        .bgMusicLevel3 = false,
                        .hasAudioLevel1 = false,
                        .hasAudioLevel2 = false,
                        .hasAudioLevel3 = false,
                        .hasChairScared = false,
                        .currentLevel = gameData::GAMESTART,
                        .lives = 3
                };
                resetLevelState();
                canEnterEnd = false;
            }
            break;
        }

        case gameData::GAMEWIN:
            getRestartInput(gData);
        break;
    }

    {
        Music* target = nullptr;
        if (gData.currentLevel == gameData::LEVEL1) target = &bgMusic1;
        else if (gData.currentLevel == gameData::LEVEL2) target = &bgMusic2;
        else if (gData.currentLevel == gameData::LEVEL3) target = &bgMusic3;

        if (target != currentBgMusic) {
            if (currentBgMusic) StopMusicStream(*currentBgMusic);
            if (target) PlayMusicStream(*target);
            currentBgMusic = target;
        }
        if (currentBgMusic) UpdateMusicStream(*currentBgMusic);
    }

    static double lastPrintTime = 0.0;
    double now = GetTime();
    if (now - lastPrintTime >= 0.25) {
        std::cout << "FPS: " << GetFPS() << "\n";
        lastPrintTime = now;
    }
    */
}

static void RenderShadowMapPass(const mtx44& nearLSM, const mtx44& farLSM) {
    if (shadowMapRT.id == 0) return;

    auto drawCasters = [&]() {
        rlEnableBackfaceCulling();

        Draw3DDepthGPU(testingplatforms, depthShader, depthModelLoc);
        /*
        Draw3DDepthGPU(drank, depthShader, depthModelLoc);
        */
        drawItemsDepth(depthShader, depthModelLoc);
        /*
        drawWizardAttacksDepth(depthShader, depthModelLoc);
        */
        /*
        Draw3DDepthGPU(testcar, depthShader, depthModelLoc);
        Draw3DDepthGPU(testcarwheel, depthShader, depthModelLoc);
        Draw3DDepthGPU(gasPump, depthShader, depthModelLoc);
        Draw3DDepthGPU(gasPumpNozzle, depthShader, depthModelLoc);
        Draw3DDepthGPU(gasPumpNozzleOff, depthShader, depthModelLoc);
        Draw3DDepthGPU(cashRegister, depthShader, depthModelLoc);
        Draw3DDepthGPU(dragan, depthShader, depthModelLoc);
        Draw3DDepthGPU(tree, depthShader, depthModelLoc);
        Draw3DDepthGPU(drank, depthShader, depthModelLoc);
        Draw3DDepthGPU(gasStation, depthShader, depthModelLoc);
        Draw3DDepthGPU(apt1, depthShader, depthModelLoc);
        Draw3DDepthGPU(apt2, depthShader, depthModelLoc);
        Draw3DDepthGPU(sketchyBuilding, depthShader, depthModelLoc);
        Draw3DDepthGPU(farm, depthShader, depthModelLoc);
        Draw3DDepthGPU(spawnDirt, depthShader, depthModelLoc);
        Draw3DDepthGPU(spawnBag, depthShader, depthModelLoc);
        Draw3DDepthGPU(spawnToilet, depthShader, depthModelLoc);
        Draw3DDepthGPU(uberGirl, depthShader, depthModelLoc);
        Draw3DDepthGPU(uberGuy, depthShader, depthModelLoc);
        Draw3DDepthGPU(uberWomanInCar, depthShader, depthModelLoc);
        Draw3DDepthGPU(uberManInCar, depthShader, depthModelLoc);
        */
    };

    rlViewport(0, 0, SHADOW_MAP_NEAR, SHADOW_MAP_NEAR);
    rlEnableFramebuffer(shadowMapRT.id);
    rlClearScreenBuffers();
    BeginShaderMode(depthShader);
    SetShaderValueMatrix(depthShader, depthLightSpaceLoc, ToRaylibMatrix(nearLSM));
    drawCasters();
    EndShaderMode();
    rlDisableFramebuffer();

    if (shadowMapRTFar.id != 0) {
        rlViewport(0, 0, SHADOW_MAP_FAR, SHADOW_MAP_FAR);
        rlEnableFramebuffer(shadowMapRTFar.id);
        rlClearScreenBuffers();
        BeginShaderMode(depthShader);
        SetShaderValueMatrix(depthShader, depthLightSpaceLoc, ToRaylibMatrix(farLSM));
        drawCasters();
        EndShaderMode();
        rlDisableFramebuffer();
    }

    rlViewport(0, 0, GetRenderWidth(), GetRenderHeight());
}

static void DrawSceneWithShadows(const mtx44& frameVP, const mtx44& nearLSM, const mtx44& farLSM) {
    float t = (float)GetTime();
    BeginShaderMode(w2sShader.shader);
    SetShaderValue(w2sShader.shader, w2sShader.lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(w2sShader.shader, w2sShader.lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(w2sShader.shader, w2sShader.lightRangeLoc, &lightRange, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.emissiveOnlyLoc, &gEmissiveOnly, SHADER_UNIFORM_INT);
    SetShaderValue(w2sShader.shader, w2sShader.lightColorLoc, &lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(w2sShader.shader, w2sShader.ambientLoc, &ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.fadeToLoc, &gData.fadeTo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.timeLoc, &t, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.drunkennessLoc, &player1.pState.drunkenness, SHADER_UNIFORM_FLOAT);
    float camPosArr[3] = {player1.camera.camPos.x, player1.camera.camPos.y, player1.camera.camPos.z};
    SetShaderValue(w2sShader.shader, w2sShader.camPosLoc, camPosArr, SHADER_UNIFORM_VEC3);
    // Emissive objects act as PHYSICAL POINT LIGHTS: each lights nearby geometry
    // within its bloom length (bloom.y = world-space radius). Colour = emissive
    // colour * amount * bloom brightness. This is what makes an object's emission
    // "spill" onto other objects it's close to (independent of the camera).
    float pointPos[3 * MAX_POINT_LIGHTS];
    float pointColor[3 * MAX_POINT_LIGHTS];
    float pointRadius[MAX_POINT_LIGHTS];
    int pointCount = 0;
    for (int e = 0; e < (int)(sizeof(gEmitters) / sizeof(gEmitters[0])) && pointCount < MAX_POINT_LIGHTS; e++) {
        meshedObject* o = gEmitters[e];
        if (o->emissive.t <= 0.f) continue;
        pointPos[pointCount * 3 + 0] = o->pEntity.location.x;
        pointPos[pointCount * 3 + 1] = o->pEntity.location.y;
        pointPos[pointCount * 3 + 2] = o->pEntity.location.z;
        pointColor[pointCount * 3 + 0] = o->emissive.x * o->emissive.t * o->bloom.x;
        pointColor[pointCount * 3 + 1] = o->emissive.y * o->emissive.t * o->bloom.x;
        pointColor[pointCount * 3 + 2] = o->emissive.z * o->emissive.t * o->bloom.x;
        pointRadius[pointCount] = o->bloom.y; // physical light radius = bloom length (world units)
        pointCount++;
    }
    if (pointCount > 0) {
        SetShaderValueV(w2sShader.shader, w2sShader.pointPosLoc, pointPos, SHADER_UNIFORM_VEC3, pointCount);
        SetShaderValueV(w2sShader.shader, w2sShader.pointColorLoc, pointColor, SHADER_UNIFORM_VEC3, pointCount);
        SetShaderValueV(w2sShader.shader, w2sShader.pointRadiusLoc, pointRadius, SHADER_UNIFORM_FLOAT, pointCount);
    }
    SetShaderValue(w2sShader.shader, w2sShader.pointCountLoc, &pointCount, SHADER_UNIFORM_INT);

    // Bloom-length projection: convert a WORLD-space bloom length to screen texels
    // in the emissive shader. focal = (emissiveBufferHeight/2) / tan(fovY/2).
    float bloomFocal = (SCREEN_HEIGHT / 4.0f) / tanf(player1.camera.fov * 0.5f);
    SetShaderValue(w2sShader.shader, w2sShader.bloomFocalLoc, &bloomFocal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.bloomMaxLoc, &bloomSpread, SHADER_UNIFORM_FLOAT);

    float drankUrgency = 0.0f;
    if (gData.currentLevel == gameData::LEVEL1 ||
        gData.currentLevel == gameData::LEVEL2 ||
        gData.currentLevel == gameData::LEVEL3) {
        float dt = gData.drankTimer;
        drankUrgency = (dt > 1.0f) ? ((170.0f - dt) / 169.0f * 0.5f)
        : 1.1f;
    }
    SetShaderValue(w2sShader.shader, w2sShader.drankUrgencyLoc, &drankUrgency, SHADER_UNIFORM_FLOAT);
    float resArr[2] = { (float)GetRenderWidth(), (float)GetRenderHeight() };
    SetShaderValue(w2sShader.shader, w2sShader.resolutionLoc, resArr, SHADER_UNIFORM_VEC2);

    SetShaderValueMatrix(w2sShader.shader, w2sShader.lightSpaceMatrixLoc, ToRaylibMatrix(nearLSM));
    SetShaderValueMatrix(w2sShader.shader, w2sShader.lightSpaceMatrixFarLoc, ToRaylibMatrix(farLSM));
    float cascadeSplit = CASCADE_SPLIT;
    SetShaderValue(w2sShader.shader, w2sShader.cascadeSplitLoc, &cascadeSplit, SHADER_UNIFORM_FLOAT);
    const bool hasShadowMap = shadowMapRT.depth.id != 0;
    const Texture2D* shadowTex = hasShadowMap ? &shadowMapRT.depth : nullptr;
    const Texture2D* shadowTexFar = (shadowMapRTFar.depth.id != 0) ? &shadowMapRTFar.depth : nullptr;

    auto drawScene = [&]() {
        Draw3DGPU(testingplatforms, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        /*
        Draw3DGPU(drank, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        */
        float itemAlpha = gPhysicsAccumulator / PHYSICS_DT;
        drawItems(player1.camera, w2sShader, itemAlpha, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        drawWizardAttacks(player1.camera, w2sShader, itemAlpha, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        /*
        Draw3DGPU(skysphere1, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, false);
        Draw3DGPU(testcar, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(testcarwheel, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(gasPump, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(gasPumpNozzle, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(gasPumpNozzleOff, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(cashRegister, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(drank, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(dragan, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(tree, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(gasStation, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(apt1, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(apt2, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(sketchyBuilding, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(farm, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(spawnDirt, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(spawnBag, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(spawnToilet, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(uberGirl, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(uberGuy, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(uberWomanInCar, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        Draw3DGPU(uberManInCar, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
        */
    };

    drawScene();
    /*
    switch(gData.currentLevel) {
        case gameData::LEVEL1:
            case gameData::LEVEL2:
            case gameData::LEVEL3:
            case gameData::LEVEL4:
            case gameData::TESTING_ENVIRONMENT:
            drawScene();
        DrawColliderGPU(testcar.cPlaneCount, testcar.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(player1.cPlaneCount, player1.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(gasPump.cPlaneCount, gasPump.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(cashRegister.cPlaneCount, cashRegister.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(drank.cPlaneCount, drank.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(dragan.cPlaneCount, dragan.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(gasStation.cPlaneCount, gasStation.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(testingplatforms.cPlaneCount, testingplatforms.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(apt1.cPlaneCount, apt1.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(apt2.cPlaneCount, apt2.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(sketchyBuilding.cPlaneCount, sketchyBuilding.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawColliderGPU(farm.cPlaneCount, farm.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
        DrawPlaneNormalsGPU(testcar.cPlaneCount, testcar.collider, player1.camera, w2sShader, {255, 0, 0, 255}, 1.0f, &frameVP);
        break;
        default:
            break;
    }
    */

    EndShaderMode();
}

void render()
{
    ClearBackground(BLACK);
    rlClearScreenBuffers();

    if (true) {

        rlEnableDepthTest();
        rlEnableDepthMask();

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        mtx44 nearLSM = BuildLightSpaceMatrix(SHADOW_RANGE_NEAR);
        mtx44 farLSM = BuildLightSpaceMatrix(SHADOW_RANGE_FAR);
        RenderShadowMapPass(nearLSM, farLSM);

        // Match the projection's aspect to the ACTUAL framebuffer. If the window is
        // not exactly SCREEN_WIDTH:SCREEN_HEIGHT (borders, DPI scaling, resize), a
        // stale aspect stretches the view non-uniformly, which rotates diagonal
        // directions (fine on the axes, "slightly off" at diagonals).
        int rw = GetRenderWidth(), rh = GetRenderHeight();
        if (rw > 0 && rh > 0) player1.camera.aspect = (float)rw / (float)rh;
        mtx44 frameView = viewMtx44(player1.camera.camPos, player1.camera.camTarget, player1.camera.up);
        float zNear = 0.1f, zFar = 1000000000000000000.0f;
        mtx44 frameProj = projMtx44(player1.camera.fov, player1.camera.aspect, zNear, zFar);
        mtx44 frameVP = mmult4(frameProj, frameView);

        // Keep the depth RT matched to the ACTUAL framebuffer size/aspect. If it's a
        // different size (DPI/borders/resize), the glow samples it at a mismatched
        // aspect and the occlusion projection misaligns with the on-screen geometry.
        if (rw > 0 && rh > 0 && (sceneDepthRT.texture.width != rw || sceneDepthRT.texture.height != rh)) {
            if (sceneDepthRT.texture.id != 0) rlUnloadTexture(sceneDepthRT.texture.id);
            if (sceneDepthRT.depth.id != 0) rlUnloadTexture(sceneDepthRT.depth.id);
            if (sceneDepthRT.id != 0) rlUnloadFramebuffer(sceneDepthRT.id);
            sceneDepthRT = LoadShadowMapRenderTexture(rw, rh);
        }

        // --- CAMERA-VIEW SCENE DEPTH (for the glow occlusion depth check) ---
        // Opaque geometry depth from the camera's POV, so the glow pass can hide the
        // parts of each corona that sit behind scene geometry. depthShader transforms
        // by uLightSpaceMatrix*uModel, so feeding it frameVP renders camera-space depth.
        if (sceneDepthRT.id != 0) {
            rlEnableFramebuffer(sceneDepthRT.id);
            rlViewport(0, 0, rw, rh);
            rlClearScreenBuffers();
            rlEnableDepthTest();
            rlEnableDepthMask();
            BeginShaderMode(depthShader);
            SetShaderValueMatrix(depthShader, depthLightSpaceLoc, ToRaylibMatrix(frameVP));
            Draw3DDepthGPU(testingplatforms, depthShader, depthModelLoc);
            // Only OCCLUDERS go here — NOT the emitters (a light must not occlude its
            // own corona) and NOT held items. The single-sample test at each light's
            // centre then sees only geometry genuinely in front of the light.
            EndShaderMode();
            rlDisableFramebuffer();
            rlViewport(0, 0, GetRenderWidth(), GetRenderHeight());
        }

        // --- MAIN SCENE to the screen ---
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
        rlEnableDepthTest();
        DrawSceneWithShadows(frameVP, nearLSM, farLSM);

        // --- FIRE PARTICLES ---
        // Additive camera-facing billboards, drawn after the opaque scene so the
        // depth buffer occludes them, and before the screen-space glow coronas.
        drawFireParticles(player1.camera.camPos, player1.camera.camTarget,
                          player1.camera.up, frameView, frameProj);

        // --- PHYSICAL LIGHT CORONAS (bloom), spatial soft-particle billboards ---
        // Each emitter's glow is a camera-facing additive quad at its WORLD
        // position, sized to the world-space bloom radius, drawn with depth test ON
        // (depth WRITE off). The scene depth buffer occludes it per-pixel, so walls
        // block the glow instead of it bleeding through them, while the halo still
        // peeks around wall edges into open space. Peak brightness = colour (constant
        // with distance) and size is world-relative, same as the old screen pass.
        {
            // camTarget holds yaw (.x) / pitch (.y) ANGLES, not a world point — build the
            // camera forward the same way viewMtx44 does, or the billboard mis-orients.
            float cyw = cosf(player1.camera.camTarget.x), syw = sinf(player1.camera.camTarget.x);
            float cpi = cosf(player1.camera.camTarget.y), spi = sinf(player1.camera.camTarget.y);
            vector3 fwd = normalize3({cpi * cyw, spi, cpi * syw});
            vector3 rgt = normalize3(cross3(fwd, player1.camera.up));
            vector3 upv = cross3(rgt, fwd); // orthonormal -> already unit length

            rlDrawRenderBatchActive();
            rlSetMatrixProjection(ToRaylibMatrix(frameProj));
            rlSetMatrixModelview(ToRaylibMatrix(frameView));
            rlDisableBackfaceCulling();
            rlDisableDepthTest(); // occlusion is handled softly in the shader
            rlDisableDepthMask();
            BeginBlendMode(BLEND_ADDITIVE);
            BeginShaderMode(glowBBShader);
            SetShaderValueTexture(glowBBShader, glowBBSceneDepthLoc, sceneDepthRT.depth);
            float gScr[2] = { (float)GetRenderWidth(), (float)GetRenderHeight() };
            SetShaderValue(glowBBShader, glowBBScreenLoc, gScr, SHADER_UNIFORM_VEC2);
            float gProjA = (zFar + zNear) / (zNear - zFar);
            float gProjB = (2.0f * zFar * zNear) / (zNear - zFar);
            SetShaderValue(glowBBShader, glowBBProjALoc, &gProjA, SHADER_UNIFORM_FLOAT);
            SetShaderValue(glowBBShader, glowBBProjBLoc, &gProjB, SHADER_UNIFORM_FLOAT);
            float gFade = 4.0f; // world-space soft-fade distance (tunable)
            SetShaderValue(glowBBShader, glowBBFadeLoc, &gFade, SHADER_UNIFORM_FLOAT);
            for (int e = 0; e < (int)(sizeof(gEmitters) / sizeof(gEmitters[0])); e++) {
                meshedObject* o = gEmitters[e];
                if (o->emissive.t <= 0.f) continue;
                vector3 p = o->pEntity.location;
                float rad = o->bloom.y; // world-space radius
                float col[3] = {
                    o->emissive.x * o->emissive.t * o->bloom.x * bloomIntensity,
                    o->emissive.y * o->emissive.t * o->bloom.x * bloomIntensity,
                    o->emissive.z * o->emissive.t * o->bloom.x * bloomIntensity
                };
                SetShaderValue(glowBBShader, glowBBColorLoc, col, SHADER_UNIFORM_VEC3);
                vector3 rx = { rgt.x * rad, rgt.y * rad, rgt.z * rad };
                vector3 uy = { upv.x * rad, upv.y * rad, upv.z * rad };
                vector3 tl = { p.x - rx.x + uy.x, p.y - rx.y + uy.y, p.z - rx.z + uy.z };
                vector3 bl = { p.x - rx.x - uy.x, p.y - rx.y - uy.y, p.z - rx.z - uy.z };
                vector3 br = { p.x + rx.x - uy.x, p.y + rx.y - uy.y, p.z + rx.z - uy.z };
                vector3 tr = { p.x + rx.x + uy.x, p.y + rx.y + uy.y, p.z + rx.z + uy.z };
                rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlTexCoord2f(0.f, 0.f); rlVertex3f(tl.x, tl.y, tl.z);
                    rlTexCoord2f(0.f, 1.f); rlVertex3f(bl.x, bl.y, bl.z);
                    rlTexCoord2f(1.f, 1.f); rlVertex3f(br.x, br.y, br.z);
                    rlTexCoord2f(1.f, 0.f); rlVertex3f(tr.x, tr.y, tr.z);
                rlEnd();
                rlDrawRenderBatchActive(); // flush with this emitter's uGlowColor
            }
            EndShaderMode();
            EndBlendMode();
            rlEnableDepthTest();
            rlEnableDepthMask();
            rlEnableBackfaceCulling();
        }

        /*
        // --- PHYSICAL LIGHT CORONAS (bloom) ---
        // Project each emissive object's world position + world-space bloom radius
        // to the screen, then draw analytic additive radial glows (glow.fs). Peak
        // brightness is constant (= colour) and size scales with perspective, so the
        // glow is camera-independent in brightness and world-relative in size — and
        // perfectly smooth/circular since nothing is sampled/blurred.
        {
            float gCenter[2 * MAX_POINT_LIGHTS];
            float gRadius[MAX_POINT_LIGHTS];
            float gColor[3 * MAX_POINT_LIGHTS];
            float gDepth[MAX_POINT_LIGHTS];
            int gCount = 0;
            float focalFull = (rh / 2.0f) / tanf(player1.camera.fov * 0.5f);
            vector3 camPos = player1.camera.camPos;
            for (int e = 0; e < (int)(sizeof(gEmitters) / sizeof(gEmitters[0])) && gCount < MAX_POINT_LIGHTS; e++) {
                meshedObject* o = gEmitters[e];
                if (o->emissive.t <= 0.f) continue;
                vector3 p = o->pEntity.location;
                vector4 clip = modmmult(frameVP, {p.x, p.y, p.z, 1.0f});
                if (clip.t <= 0.001f) continue; // behind the camera
                float dx = camPos.x - p.x, dy = camPos.y - p.y, dz = camPos.z - p.z;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist < 0.001f) dist = 0.001f;
                gCenter[gCount*2+0] = (clip.x / clip.t * 0.5f + 0.5f) * rw;
                gCenter[gCount*2+1] = (clip.y / clip.t * 0.5f + 0.5f) * rh; // GL bottom-left (matches gl_FragCoord)
                gRadius[gCount] = o->bloom.y * focalFull / dist;                        // world radius -> screen px
                gColor[gCount*3+0] = o->emissive.x * o->emissive.t * o->bloom.x * bloomIntensity;
                gColor[gCount*3+1] = o->emissive.y * o->emissive.t * o->bloom.x * bloomIntensity;
                gColor[gCount*3+2] = o->emissive.z * o->emissive.t * o->bloom.x * bloomIntensity;
                gDepth[gCount] = (clip.z / clip.t) * 0.5f + 0.5f; // window depth [0,1] for the depth check
                gCount++;
            }
            if (gCount > 0) {
                rlDisableDepthTest();
                rlMatrixMode(RL_PROJECTION);
                rlLoadIdentity();
                rlOrtho(0, rw, rh, 0, -1, 1);
                rlMatrixMode(RL_MODELVIEW);
                rlLoadIdentity();

                SetShaderValue (glowShader, glowCountLoc,  &gCount,  SHADER_UNIFORM_INT);
                SetShaderValueV(glowShader, glowCenterLoc, gCenter,  SHADER_UNIFORM_VEC2,  gCount);
                SetShaderValueV(glowShader, glowRadiusLoc, gRadius,  SHADER_UNIFORM_FLOAT, gCount);
                SetShaderValueV(glowShader, glowColorLoc,  gColor,   SHADER_UNIFORM_VEC3,  gCount);
                SetShaderValueV(glowShader, glowDepthArrLoc, gDepth,  SHADER_UNIFORM_FLOAT, gCount);
                float gScr[2] = { (float)rw, (float)rh };
                SetShaderValue(glowShader, glowScreenLoc, gScr, SHADER_UNIFORM_VEC2);
                float gProjA = (zFar + zNear) / (zNear - zFar);
                float gProjB = (2.0f * zFar * zNear) / (zNear - zFar);
                SetShaderValue(glowShader, glowProjALoc, &gProjA, SHADER_UNIFORM_FLOAT);
                SetShaderValue(glowShader, glowProjBLoc, &gProjB, SHADER_UNIFORM_FLOAT);
                float gBias = 2.0f; // world-units slack on the occlusion comparison
                SetShaderValue(glowShader, glowBiasLoc, &gBias, SHADER_UNIFORM_FLOAT);

                BeginBlendMode(BLEND_ADDITIVE);
                BeginShaderMode(glowShader);
                // Bind the scene depth AFTER activating the shader, then draw the
                // fullscreen quad with immediate mode. NOT DrawRectangle: its internal
                // rlSetTexture triggers a batch flush that drops the uSceneDepth sampler
                // binding, so the glow ends up sampling a stale unit (the shadow map).
                SetShaderValueTexture(glowShader, glowSceneDepthLoc, sceneDepthRT.depth);
                rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlVertex2f(0.f, 0.f);
                    rlVertex2f(0.f, (float)rh);
                    rlVertex2f((float)rw, (float)rh);
                    rlVertex2f((float)rw, 0.f);
                rlEnd();
                rlDrawRenderBatchActive();
                EndShaderMode();
                EndBlendMode();
            }
        }
        */

        // DEBUG: full camera-view depth in the top-right, oriented like your view
        // (negative source height un-flips the render texture), amplified so geometry
        // shows (raw depth sits near 1.0). Remove when done.
        if (sceneDepthRT.depth.id != 0) {
            rlDrawRenderBatchActive();
            rlDisableDepthTest();
            rlMatrixMode(RL_PROJECTION); rlLoadIdentity();
            rlOrtho(0, rw, rh, 0, -1, 1);
            rlMatrixMode(RL_MODELVIEW); rlLoadIdentity();
            BeginShaderMode(depthViewShader);
            DrawTexturePro(sceneDepthRT.depth,
                (Rectangle){ 0, 0, (float)sceneDepthRT.depth.width, -(float)sceneDepthRT.depth.height },
                (Rectangle){ (float)rw - 410.0f, 10.0f, 400.0f, 225.0f },
                (Vector2){ 0, 0 }, 0.0f, WHITE);
            EndShaderMode();
        }

        /*
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlDisableDepthTest();
        switch(gData.currentLevel) {
            case gameData::GAMESTART:
                guiDrawStartMenu(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE, 1);
            break;
            case gameData::GAMEINFOMERCIAL:
                guiDrawStartPopup(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
            break;
            case gameData::GAMEEND:
                guiDrawFailure(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
            break;
            case gameData::GAMEWIN:
                guiDrawSuccess(SCREEN_WIDTH/2.f-175.f, SCREEN_HEIGHT/2.f-100.f, 350, 200, WHITE);
            break;
            default:
                if (player1.pState.buying) guiBuyMenu(SCREEN_WIDTH, SCREEN_HEIGHT, player1, 1);
            if (player1.pState.gasMenuOpen) guiGasMenu(SCREEN_WIDTH, SCREEN_HEIGHT, player1, gGasMenuSelection, gasPumpPortrait);
            if (uberSys.phase == UBER_MENU) {
                const char* pName = uberSys.isFemale ? kFemaleNames[uberSys.nameIdx] : kMaleNames[uberSys.nameIdx];
                Texture2D& port = uberSys.isFemale ? uberGirlPortrait : uberGuyPortrait;
                float reward = uberSys.spawns[uberSys.spawnIdx].reward;
                guiUberPickup(SCREEN_WIDTH, SCREEN_HEIGHT, pName, uberSys.spawns[uberSys.spawnIdx].destName, uberSys.menuSel, port, reward);
            }
            if (uberSys.phase == UBER_ARRIVED && !uberSys.arrivedConfirmed) {
                float reward = (uberSys.spawnIdx > 0 && uberSys.spawns)
                ? uberSys.spawns[uberSys.spawnIdx - 1].reward : 15.f;
                guiUberArrived(SCREEN_WIDTH, SCREEN_HEIGHT, reward);
            }
            break;
        }

        if (gData.currentLevel == gameData::LEVEL1 ||
            gData.currentLevel == gameData::LEVEL2 ||
            gData.currentLevel == gameData::LEVEL3) {
            int totalSecs = (int)gData.levelTimer;
            int mins = totalSecs / 60;
            int secs = totalSecs % 60;
            char timerBuf[16];
            snprintf(timerBuf, sizeof(timerBuf), "%d:%02d", mins, secs);
            int tw = MeasureText(timerBuf, 32);
            int tx = (SCREEN_WIDTH - tw) / 2;

            DrawText(timerBuf, tx + 2, 10, 32, GRAY);
            DrawText(timerBuf, tx - 2, 10, 32, GRAY);
            DrawText(timerBuf, tx, 10 + 2, 32, GRAY);
            DrawText(timerBuf, tx, 10 - 2, 32, GRAY);
            DrawText(timerBuf, tx, 10, 32, BLACK);
        }

        if (gData.escapeMenuOpen)
        guiDrawEscapeMenu();

        if (uberAnnounce.timer >= 0.f) {
            float t = uberAnnounce.timer;
            float alpha = (t < 1.f) ? (t * 255.f) : (t < 4.f) ? 255.f : ((5.f - t) * 255.f);
            alpha = std::max(0.f, std::min(255.f, alpha));
            int fontSize = 36;
            int tw = MeasureText(uberAnnounce.text, fontSize);

            DrawText(uberAnnounce.text, (SCREEN_WIDTH-tw)/2+2, SCREEN_HEIGHT/2+2, fontSize, {0,0,0,(unsigned char)(int)alpha});
            DrawText(uberAnnounce.text, (SCREEN_WIDTH-tw)/2, SCREEN_HEIGHT/2, fontSize, {255,255,255,(unsigned char)(int)alpha});
        }

        if (fadeOverlayAlpha > 0.0f)
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
            {0, 0, 0, (unsigned char)(int)fadeOverlayAlpha});
        */
    }
}

void shutdown()
{
    shutdownParticles();
    shutdownWizardAttacks();

    UnloadSound(jump1);
    UnloadSound(jump2);
    UnloadSound(jump3);
    UnloadSound(jump4);
    UnloadSound(jump5);
    UnloadSound(jump6);
    UnloadSound(jump7);
    stopCarEngineThread();
    UnloadMusicStream(carEngine);
    UnloadMusicStream(bgMusic1);
    UnloadMusicStream(bgMusic2);
    UnloadMusicStream(bgMusic3);
    UnloadSound(sndShopIntro);
    UnloadSound(sndShopBye);
    UnloadSound(sndShopThank);
    UnloadSound(sndGasIntro);
    UnloadSound(sndGasPurchase);

    UnloadShader(w2sShader.shader);
    UnloadShader(depthShader);
    if (shadowMapRT.texture.id != 0) rlUnloadTexture(shadowMapRT.texture.id);
    if (shadowMapRT.depth.id != 0) rlUnloadTexture(shadowMapRT.depth.id);
    if (shadowMapRT.id != 0) rlUnloadFramebuffer(shadowMapRT.id);
    if (shadowMapRTFar.texture.id != 0) rlUnloadTexture(shadowMapRTFar.texture.id);
    if (shadowMapRTFar.depth.id != 0) rlUnloadTexture(shadowMapRTFar.depth.id);
    if (shadowMapRTFar.id != 0) rlUnloadFramebuffer(shadowMapRTFar.id);
    if (sceneDepthRT.texture.id != 0) rlUnloadTexture(sceneDepthRT.texture.id);
    if (sceneDepthRT.depth.id != 0) rlUnloadTexture(sceneDepthRT.depth.id);
    if (sceneDepthRT.id != 0) rlUnloadFramebuffer(sceneDepthRT.id);

    if (emissiveRT.id != 0) UnloadRenderTexture(emissiveRT);
    if (bloomShader.id != 0) UnloadShader(bloomShader);
    if (glowShader.id != 0) UnloadShader(glowShader);
    if (glowBBShader.id != 0) UnloadShader(glowBBShader);

    free(skysphere1.mesh.tris);
    free(skysphere1.mesh.trisO);
    free(testingplatforms.mesh.tris);
    free(testingplatforms.mesh.trisO);
    delete[] testingplatforms.collider;
    delete[] testingplatforms.colliderO;
    free(wand.mesh.tris);
    free(wand.mesh.trisO);
    delete[] wand.collider;
    delete[] wand.colliderO;
    free(cube.mesh.tris);
    free(cube.mesh.trisO);

    CloseAudioDevice();
    CloseWindow();

}

int main(void)
{
    std::cout << "Hello, World!" << std::endl;
    initialise();

    while (gAppStatus == RUNNING)
    {
        update();
        BeginDrawing();
        render();
        EndDrawing();
#ifdef _WIN32

        pumpMessages();
#endif
    }

    RawMouseShutdown();
    shutdown();
    return 0;
}
