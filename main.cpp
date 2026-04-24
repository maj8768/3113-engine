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

#include "draw/draw.h"
#include "draw/gui.h"

#include "camera/camera.h"

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
static constexpr int SHADOW_MAP_NEAR = 2048;
static constexpr int SHADOW_MAP_FAR = 1024;
static constexpr float SHADOW_RANGE_NEAR = 80.0f;
static constexpr float SHADOW_RANGE_FAR = 300.0f;
static constexpr float CASCADE_SPLIT = 65.0f;
static constexpr float SHADOW_NEAR = 1.0f;
static constexpr float SUN_DIST = 100.0f;
static constexpr float SHADOW_FAR = SUN_DIST * 2.2f;
static constexpr float LIGHT_ORBIT_SPEED = 0.15f;
static constexpr float LIGHT_ORBIT_RADIUS_Y = 150.0f;
static constexpr float LIGHT_ORBIT_RADIUS_Z = 25.0f;
static constexpr float LIGHT_ORBIT_X_OFFSET = 55.0f;

static Vector3 lightPos = {250.f, 250.f, 250.f};
static Vector3 lightDir = {-0.5f, -0.707f, 0.5f};
static Vector4 lightColor = {1.0f, 0.72f, 0.2f, 1.0f};
static float ambient = 0.28f;

static meshedObject skysphere1;
static meshedObject cube;
static meshedObject testingplatforms;
static meshedObject testcar;
static meshedObject testcarwheel;

static meshedObject gasPump;
static meshedObject gasPumpNozzle;
static meshedObject gasPumpNozzleOff;
static meshedObject cashRegister;
static meshedObject drank;
static meshedObject buyBox;
static meshedObject dragan;
static meshedObject tree;
static meshedObject gasStation;
static meshedObject apt1;
static meshedObject apt2;
static meshedObject sketchyBuilding;
static meshedObject farm;
static meshedObject spawnDirt;
static meshedObject spawnBag;
static meshedObject spawnToilet;

static meshedObject uberGirl;
static meshedObject uberGuy;
static meshedObject uberWomanInCar;
static meshedObject uberManInCar;
static Texture2D uberGirlPortrait = {0};
static Texture2D uberGuyPortrait = {0};

static meshedObject fakeMesh;

static bool iamreal = false;
static int iamalsoreal = 1;

struct UberSpawn {vector3 start, dest; const char* startName; const char* destName; float reward;};

static const UberSpawn kUberL1[] = {
    {{32.6524f, 0.f, 16.0543f}, {-1591.f, 0.f, 77.2f}, "Gasazk Stationgrad","Chelyabinsk 1", 15.f},
        {{-1592.f, 0.f, -13.0587f}, {32.6524f, 0.f, 16.0543f}, "Chelyabinsk 2", "Gasazk Stationgrad",15.f},
    };
static const UberSpawn kUberL2[] = {
    {{1164.2f, 0.f, -31.06f}, {-1591.f, 0.f, 77.2f}, "Ufa", "Chelyabinsk 1", 20.f},
        {{32.6524f, 0.f, 16.0543f}, {2365.9f, 0.f, 70.2609f}, "Gasazk Stationgrad","Vladivostok", 15.f},
        {{-1592.f, 0.f, -13.0587f}, {32.6524f, 0.f, 16.0543f}, "Chelyabinsk 2", "Gasazk Stationgrad",15.f},
    };
static const UberSpawn kUberL3[] = {
    {{2365.9f, 0.f, 70.2609f}, {-1591.f, 0.f, 77.2f}, "Vladivostok", "Chelyabinsk 1", 20.f},
        {{32.6524f, 0.f, 16.0543f}, {1164.2f, 0.f, -31.06f}, "Gasazk Stationgrad","Ufa", 15.f},
        {{-1592.f, 0.f, -13.0587f}, {1164.2f, 0.f, -31.06f}, "Chelyabinsk 2", "Ufa", 20.f},
        {{2365.9f, 0.f, 70.2609f}, {32.6524f, 0.f, 16.0543f}, "Vladivostok", "Gasazk Stationgrad",15.f},
    };

struct UberAnnounce {
    float timer = -1.f;
    float delay = 0.f;
    char text[96] = {};
};
static UberAnnounce uberAnnounce;

static Sound sndUberIntroGuy = {0};
static Sound sndUberIntroGirl = {0};
static Sound sndUberExitGuy[3] = {};
static Sound sndUberExitGirl[3] = {};

static void triggerUberAnnounce(const char* txt, float delay = 0.f) {
    strncpy(uberAnnounce.text, txt, 95);
    uberAnnounce.text[95] = '\0';
    uberAnnounce.delay = delay;
    uberAnnounce.timer = (delay <= 0.f) ? 0.f : -1.f;
}

static const char* kMaleNames[] = {
    "Aleksandr Ivanov","Dmitri Petrov","Ivan Sokolov","Nikolai Smirnov",
        "Mikhail Volkov","Sergei Kuznetsov","Andrei Popov","Alexei Morozov",
        "Viktor Orlov","Pavel Lebedev","Yuri Fedorov","Vladimir Mikhailov",
        "Oleg Romanov","Roman Kozlov","Maxim Novikov"
};
static const char* kFemaleNames[] = {
    "Anastasia Ivanova","Ekaterina Petrova","Maria Sokolova","Olga Smirnova",
        "Tatiana Volkova","Natalia Kuznetsova","Irina Popova","Svetlana Morozova",
        "Yelena Orlova","Daria Lebedeva","Alina Fedorova","Ksenia Mikhailova",
        "Vera Romanova","Polina Kozlova","Marina Novikova"
};
static constexpr int kNameCount = 15;

enum UberPhase {UBER_INACTIVE, UBER_WAITING, UBER_MENU, UBER_RIDING, UBER_ARRIVED, UBER_DONE};

struct UberSys {
    UberPhase phase = UBER_INACTIVE;
    int spawnIdx = 0;
    int totalSpawns = 0;
    const UberSpawn* spawns = nullptr;
    float waitTimer = 15.f;
    bool isFemale = false;
    int nameIdx = 0;
    int menuSel = 0;
    bool menuCanNav = true;
    bool menuCanAct = false;
    bool arrivedConfirmed = false;
    float deliveredTimer = -1.f;
    bool deliveredFemale = false;
};
static UberSys uberSys;

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
        .currentLevel = gameData::GAMESTART,
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
static void initUberForLevel();
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
    vector3 sunDir = normalize3({lightDir.x, lightDir.y, lightDir.z});
    if (len3(sunDir) <= eps) sunDir = {0.f, -1.f, 0.f};
    vector3 lightPos3 = {
        sceneCenter.x - sunDir.x * SUN_DIST,
            sceneCenter.y - sunDir.y * SUN_DIST,
            sceneCenter.z - sunDir.z * SUN_DIST
    };
    lightPos = {lightPos3.x, lightPos3.y, lightPos3.z};
    mtx44 lightView = lookAtMtx44(lightPos3, sceneCenter, {0.f, 1.f, 0.f});
    mtx44 lightProj = orthoMtx44(-range, range, -range, range, SHADOW_NEAR, SHADOW_FAR);
    return mmult4(lightProj, lightView);
}

void initializePlayer(player& player1) {

    DisableCursor();

    player1.pEntity.location = {42.6027f, 10.0f, 131.896f};
    player1.camera.camPos = {42.6027f, 10.0f, 131.896f};
    player1.camera.camTarget = {M_PI/2.f,0,0};
    player1.camera.up = {0,1,0};
    player1.camera.aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    player1.camera.fov = 90.0f * M_PI / 180.0f;
    player1.controls = {'W', 'A', 'S', 'D'};
    player1.canMove = true;
    player1.pState = {false, false, 0.f, 0.f, 100.f};

    objToQuads("resources/player/collider/playercollider.obj", fakeMesh, 1.0f, player1, true);
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

static meshedObject* worldObjects[] = {&uberGirl, &uberGuy, &testingplatforms, &testcar, &gasPump, &cashRegister, &dragan, &gasStation, &apt1, &apt2, &sketchyBuilding, &farm, &spawnToilet};
static meshedObject* secondaryObjects[] = {&uberGirl, &uberGuy, &testingplatforms, &gasPump, &gasStation, &apt1, &apt2, &sketchyBuilding, &farm, &spawnToilet};
static world worldInstance = {nullptr, 0};
static world secondaryInstance = {nullptr, 0};

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
    carEngine = LoadMusicStream("resources/sounds/car/carEngine.ogg");
    swallow = LoadSound("resources/sounds/player/swallow.mp3");
    startCarEngineThread(carEngine);

    bgMusic1 = LoadMusicStream("resources/sounds/map/bg1.mp3");
    bgMusic2 = LoadMusicStream("resources/sounds/map/bg2.mp3");
    bgMusic3 = LoadMusicStream("resources/sounds/map/bg3.mp3");
    SetMusicVolume(bgMusic1, 0.2f);
    SetMusicVolume(bgMusic2, 0.2f);
    SetMusicVolume(bgMusic3, 0.2f);

    sndShopIntro = LoadSound("resources/sounds/map/intro.mp3");
    sndShopBye = LoadSound("resources/sounds/map/bye.mp3");
    sndShopThank = LoadSound("resources/sounds/map/thank.mp3");
    sndGasIntro = LoadSound("resources/sounds/map/gas_intro.mp3");
    sndGasPurchase = LoadSound("resources/sounds/map/gas_purchase.mp3");

    sndUberIntroGuy = LoadSound("resources/sounds/map/uber_intro_guy.mp3");
    sndUberIntroGirl = LoadSound("resources/sounds/map/uber_intro_girl.mp3");
    sndUberExitGuy[0] = LoadSound("resources/sounds/map/uber_exit1_guy.mp3");
    sndUberExitGuy[1] = LoadSound("resources/sounds/map/uber_exit2_guy.mp3");
    sndUberExitGuy[2] = LoadSound("resources/sounds/map/uber_exit3_guy.mp3");
    sndUberExitGirl[0] = LoadSound("resources/sounds/map/uber_exit1_girl.mp3");
    sndUberExitGirl[1] = LoadSound("resources/sounds/map/uber_exit2_girl.mp3");
    sndUberExitGirl[2] = LoadSound("resources/sounds/map/uber_exit3_girl.mp3");

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
    gasPumpPortrait = LoadTexture("resources/levels/testing/gaspfp.PNG");
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

    char empty[100] = "";
    char cubeText[100] = "I am a cube";
    create3dObject(skysphere1, "resources/levels/skysphere.obj", "", false, w2sShader, 25.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    skysphere1.noCull = true;

    create3dObject(testingplatforms, "resources/levels/testing/testplatform.obj", "resources/levels/testing/colliders/testplatformcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    testingplatforms.noCull = true;

    create3dObject(testcar, "resources/levels/testing/testcar.obj", "resources/levels/testing/colliders/testcarcollider.obj", true, w2sShader, 4.f, {0.f,5.f,15.f}, empty, false, 100, {0,0,0});
    create3dObject(testcarwheel, "resources/levels/testing/testcarwheel.obj", "", false, w2sShader, 4.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPump, "resources/levels/testing/gas_pump/pump/gas_pump.obj", "resources/levels/testing/gas_pump/colliders/pumpcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPumpNozzle, "resources/levels/testing/gas_pump/grab/grab_onpump.obj", "", false, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPumpNozzleOff, "resources/levels/testing/gas_pump/grab/grab_offpump.obj", "", false, w2sShader, 1.f, {0.f,-20.f,0.f}, empty, false, 100, {0,0,0});
    gasPumpNozzleOff.texo = LoadTexture("resources/levels/testing/gas_pump/Fuel_pump.png");
    create3dObject(cashRegister, "resources/levels/testing/register/cashregister.obj", "resources/levels/testing/register/colliders/cashregistercollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(drank, "resources/levels/testing/items/drank/drank.obj", "resources/levels/testing/items/drank/colliders/drankcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(buyBox, "resources/levels/testing/register/buybox.obj", "resources/levels/testing/register/colliders/buyboxcolliders.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(dragan, "resources/levels/testing/register/dragan.obj", "resources/levels/testing/register/colliders/dragancollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});

    create3dObject(gasStation, "resources/levels/testing/gasStation.obj", "resources/levels/testing/colliders/gasstationcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(apt1, "resources/levels/testing/structures/apt1.obj", "resources/levels/testing/structures/colliders/apt1collider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(apt2, "resources/levels/testing/structures/apt2.obj", "resources/levels/testing/structures/colliders/apt2collider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(sketchyBuilding, "resources/levels/testing/structures/sketchy.obj", "resources/levels/testing/structures/colliders/sketchycollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(farm, "resources/levels/testing/structures/farm.obj", "resources/levels/testing/structures/colliders/farmcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(spawnDirt, "resources/levels/testing/structures/spawndirt.obj", "", false, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(spawnBag, "resources/levels/testing/structures/spawnbag.obj", "resources/levels/testing/structures/colliders/spawnbagcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(spawnToilet, "resources/levels/testing/structures/spawntoilet.obj", "resources/levels/testing/structures/colliders/spawntoiletcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(uberGirl, "resources/levels/testing/guys/woman.obj", "resources/levels/testing/guys/colliders/collider.obj", false, w2sShader, 1.f, {0.f,-1000.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(uberGuy, "resources/levels/testing/guys/guy.obj", "resources/levels/testing/guys/colliders/collider.obj", false, w2sShader, 1.f, {0.f,-1000.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(uberWomanInCar,"resources/levels/testing/guys/womanincar.obj","", false, w2sShader, 1.f, {0.f,-1000.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(uberManInCar, "resources/levels/testing/guys/guyincar.obj", "", false, w2sShader, 1.f, {0.f,-1000.f,0.f}, empty, false, 100, {0,0,0});
    uberGirlPortrait = LoadTexture("resources/levels/testing/guys/ubergirl.PNG");
    uberGuyPortrait = LoadTexture("resources/levels/testing/guys/uberguy.PNG");

    initializePhysicsEntity(player1.pEntity, 1.f, COMPLEX);
    initializePhysicsEntity(testcar.pEntity, 1000.f, COMPLEX);
    initializePhysicsEntity(drank.pEntity, 1.f, COMPLEX);

    initializePhysicsEntity(testcarwheel.pEntity, 100.f, COMPLEX);

    updateColliderLocation(fakeMesh,player1,true);
    updateColliderLocation(testcar,player1,false);
    updateColliderLocation(drank,player1,false);
    updateColliderLocation(buyBox,player1,false);
    updateColliderLocation(dragan,player1,false);

    updateColliderLocation(testingplatforms, player1, false);
    updateColliderLocation(gasPump, player1, false);
    updateColliderLocation(cashRegister, player1, false);
    updateColliderLocation(gasStation, player1, false);

    buildWorld(secondaryInstance, secondaryObjects, 10);
    buildWorld(worldInstance, worldObjects, 13);

    applyAcceleration({0.f,-20.5f,0.f}, player1.pEntity);

    applyAcceleration({0.f,-20.5f,0.f},testcar.pEntity);
    applyAcceleration({0.f,-20.5f,0.f},drank.pEntity);

    gPreviousTicks = static_cast<float>(GetTime());

    initUberForLevel();
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
vector2 md;;

static void spawnNextUber() {
    if (uberSys.spawnIdx >= uberSys.totalSpawns) {
        uberSys.phase = UBER_DONE;
        uberGirl.pEntity.location = {0.f, -1000.f, 0.f};
        uberGuy.pEntity.location = {0.f, -1000.f, 0.f};
        return;
    }

    uberSys.nameIdx = rand() % kNameCount;
    uberSys.phase = UBER_WAITING;
    uberSys.menuSel = 0;
    uberSys.menuCanNav = true;
    uberSys.menuCanAct = false;
    uberSys.arrivedConfirmed = false;
    vector3 spawnPos = uberSys.spawns[uberSys.spawnIdx].start;
    if (uberSys.isFemale) {
        uberGirl.pEntity.location = spawnPos;

        if (uberSys.deliveredTimer < 0.f || uberSys.deliveredFemale)
        uberGuy.pEntity.location = {0.f, -1000.f, 0.f};
    } else {
        uberGuy.pEntity.location = spawnPos;
        if (uberSys.deliveredTimer < 0.f || !uberSys.deliveredFemale)
        uberGirl.pEntity.location = {0.f, -1000.f, 0.f};
    }
    if (uberSys.spawnIdx == 0) {
        char buf[96];
        snprintf(buf, sizeof(buf), "New Uber: %s", uberSys.spawns[0].startName);
        triggerUberAnnounce(buf, 2.0f);
    }
    player1.pState.uberMenuOpen = false;
}

static void initUberForLevel() {
    uberSys = UberSys{};
    uberGirl.pEntity.location = {0.f, -1000.f, 0.f};
    uberGuy.pEntity.location = {0.f, -1000.f, 0.f};
    uberWomanInCar.pEntity.location = {0.f, -1000.f, 0.f};
    uberManInCar.pEntity.location = {0.f, -1000.f, 0.f};
    switch (gData.currentLevel) {
        case gameData::LEVEL1:
            case gameData::TESTING_ENVIRONMENT: uberSys.spawns = kUberL1; uberSys.totalSpawns = 2; break;
        case gameData::LEVEL2: uberSys.spawns = kUberL2; uberSys.totalSpawns = 3; break;
        case gameData::LEVEL3: uberSys.spawns = kUberL3; uberSys.totalSpawns = 4; break;
        default: uberSys.phase = UBER_DONE; return;
    }
    uberSys.isFemale = (rand() % 2) == 0;
    spawnNextUber();
}

static void updateUber(float deltaTime) {

    if (!uberSys.spawns) {
        if (gData.currentLevel == gameData::LEVEL1 ||
            gData.currentLevel == gameData::LEVEL2 ||
            gData.currentLevel == gameData::LEVEL3 ||
            gData.currentLevel == gameData::TESTING_ENVIRONMENT)
        initUberForLevel();
    }
    if (uberSys.phase == UBER_DONE || uberSys.phase == UBER_INACTIVE || !uberSys.spawns) return;

    if (uberSys.deliveredTimer >= 0.f) {
        uberSys.deliveredTimer -= deltaTime;
        if (uberSys.deliveredTimer < 0.f) {
            if (uberSys.deliveredFemale) uberGirl.pEntity.location = {0.f, -1000.f, 0.f};
            else uberGuy.pEntity.location = {0.f, -1000.f, 0.f};
        }
    }

    meshedObject& active = uberSys.isFemale ? uberGirl : uberGuy;
    const UberSpawn& cur = uberSys.spawns[uberSys.spawnIdx];

    auto dist3 = [](vector3 a, vector3 b) {
        float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
        return sqrtf(dx*dx+dy*dy+dz*dz);
    };

    static bool canF = false;
    if (!getAsyncKeyStateWrapper(KEY_F)) canF = true;

    switch (uberSys.phase) {
        case UBER_WAITING: {
            if (player1.pState.inCar) {
                float d = dist3(testcar.pEntity.location, cur.start);
                if (d <= 30.f) {
                    player1.pState.uberMenuOpen = true;
                    if (canF && getAsyncKeyStateWrapper(KEY_F)) {
                        uberSys.phase = UBER_MENU;
                        uberSys.menuCanAct = false;
                        canF = false;
                    }
                } else {
                    player1.pState.uberMenuOpen = false;
                }
            } else {
                player1.pState.uberMenuOpen = false;
            }
            break;
        }
        case UBER_MENU: {
            player1.pState.uberMenuOpen = true;
            static bool canNav = true;
            if (getAsyncKeyStateWrapper(KEY_UP)) {
                if (canNav) {uberSys.menuSel = 0; canNav = false;}
            } else if (getAsyncKeyStateWrapper(KEY_DOWN)) {
                if (canNav) {uberSys.menuSel = 1; canNav = false;}
            } else {canNav = true;}

            if (!getAsyncKeyStateWrapper(KEY_ENTER)) uberSys.menuCanAct = true;
            if (uberSys.menuCanAct && getAsyncKeyStateWrapper(KEY_ENTER)) {
                if (uberSys.menuSel == 0) {
                    uberSys.phase = UBER_RIDING;
                    active.pEntity.location = {0.f, -1000.f, 0.f};
                    PlaySound(uberSys.isFemale ? sndUberIntroGirl : sndUberIntroGuy);
                } else {
                    uberSys.phase = UBER_WAITING;
                }
                player1.pState.uberMenuOpen = false;
                uberSys.menuCanAct = false;
            }
            break;
        }
        case UBER_RIDING: {
            float d = dist3(testcar.pEntity.location, cur.dest);
            if (d <= 50.f) {
                player1.pState.uberMenuOpen = true;
                if (canF && getAsyncKeyStateWrapper(KEY_F)) {
                    meshedObject& inCar = uberSys.isFemale ? uberWomanInCar : uberManInCar;
                    uberSys.phase = UBER_ARRIVED;
                    inCar.pEntity.location = {0.f, -1000.f, 0.f};
                    active.pEntity.location = cur.dest;
                    player1.pState.money += cur.reward;
                    if (cur.reward >= 20.f)
                        gData.levelTimer += 35.0f;
                    uberSys.arrivedConfirmed = false;
                    PlaySound((uberSys.isFemale ? sndUberExitGirl : sndUberExitGuy)[rand() % 3]);
                    canF = false;
                }
            } else {
                player1.pState.uberMenuOpen = false;
            }
            break;
        }
        case UBER_ARRIVED: {
            if (!uberSys.arrivedConfirmed) {
                player1.pState.uberMenuOpen = true;
                static bool canEnterArrive = false;
                if (!getAsyncKeyStateWrapper(KEY_ENTER)) canEnterArrive = true;
                if (canEnterArrive && getAsyncKeyStateWrapper(KEY_ENTER)) {
                    uberSys.arrivedConfirmed = true;
                    player1.pState.uberMenuOpen = false;

                    uberSys.deliveredFemale = uberSys.isFemale;
                    uberSys.deliveredTimer = 15.f;
                    canEnterArrive = false;

                    uberSys.spawnIdx++;
                    if (uberSys.spawnIdx < uberSys.totalSpawns) {
                        uberSys.isFemale = !uberSys.deliveredFemale;
                        spawnNextUber();
                        char buf[96];
                        snprintf(buf, sizeof(buf), "New Uber: %s", uberSys.spawns[uberSys.spawnIdx].startName);
                        triggerUberAnnounce(buf, 0.f);
                    } else {
                        uberSys.phase = UBER_DONE;
                    }
                }
            }
            break;
        }
        default: break;
    }

    if (uberAnnounce.delay > 0.f) {
        uberAnnounce.delay -= deltaTime;
        if (uberAnnounce.delay <= 0.f) {
            uberAnnounce.timer = 0.f;
        }
    } else if (uberAnnounce.timer >= 0.f) {
        uberAnnounce.timer += deltaTime;
        if (uberAnnounce.timer >= 5.f) {
            uberAnnounce.timer = -1.f;
        }
    }
}

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

    testcar.pEntity.location = {0.f, 5.f, 15.f};
    testcar.pEntity.magnitude = {0.f, 0.f, 0.f};
    drank.pEntity.location = {0.f, 0.f, 0.f};

    updateColliderLocation(fakeMesh, player1, true);
    updateColliderLocation(testcar, player1, false);
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
    initUberForLevel();
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

void update() {

    skysphere1.pEntity.location = player1.pEntity.location;

    auto ticks = static_cast<float>(GetTime());
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;
    if (deltaTime > 0.05f) deltaTime = 0.05f;

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
}

static void RenderShadowMapPass(const mtx44& nearLSM, const mtx44& farLSM) {
    if (shadowMapRT.id == 0) return;

    auto drawCasters = [&]() {
        rlEnableBackfaceCulling();

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
        Draw3DDepthGPU(testingplatforms, depthShader, depthModelLoc);
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
    SetShaderValue(w2sShader.shader, w2sShader.lightColorLoc, &lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(w2sShader.shader, w2sShader.ambientLoc, &ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.fadeToLoc, &gData.fadeTo, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.timeLoc, &t, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.drunkennessLoc, &player1.pState.drunkenness, SHADER_UNIFORM_FLOAT);
    float camPosArr[3] = {player1.camera.camPos.x, player1.camera.camPos.y, player1.camera.camPos.z};
    SetShaderValue(w2sShader.shader, w2sShader.camPosLoc, camPosArr, SHADER_UNIFORM_VEC3);
    static float pointPos[] = {83.0f, 6.75f, -26.23f, 83.0f, 6.75f, -40.23f};
    static float pointColor[] = {1.0f, 0.9f, 0.7f, 1.0f, 0.9f, 0.7f};
    static float pointRadius[] = {20.0f, 20.0f};
    static int pointCount = 2;
    SetShaderValueV(w2sShader.shader, w2sShader.pointPosLoc, pointPos, SHADER_UNIFORM_VEC3, pointCount);
    SetShaderValueV(w2sShader.shader, w2sShader.pointColorLoc, pointColor, SHADER_UNIFORM_VEC3, pointCount);
    SetShaderValueV(w2sShader.shader, w2sShader.pointRadiusLoc, pointRadius, SHADER_UNIFORM_FLOAT, pointCount);
    SetShaderValue (w2sShader.shader, w2sShader.pointCountLoc, &pointCount, SHADER_UNIFORM_INT);

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
        Draw3DGPU(skysphere1, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, false);
        Draw3DGPU(testingplatforms, player1.camera, w2sShader, {255,0,0,255}, &frameVP, shadowTex, hasShadowMap, shadowTexFar);
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
    };

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

        mtx44 frameView = viewMtx44(player1.camera.camPos, player1.camera.camTarget, player1.camera.up);
        mtx44 frameProj = projMtx44(player1.camera.fov, player1.camera.aspect, 0.1f, 1000000000000000000.0f);
        mtx44 frameVP = mmult4(frameProj, frameView);

        DrawSceneWithShadows(frameVP, nearLSM, farLSM);

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
    }
}

void shutdown()
{

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
    UnloadSound(sndUberIntroGuy);
    UnloadSound(sndUberIntroGirl);
    for (int i = 0; i < 3; i++) {UnloadSound(sndUberExitGuy[i]); UnloadSound(sndUberExitGirl[i]);}

    UnloadShader(w2sShader.shader);
    UnloadShader(depthShader);
    if (shadowMapRT.texture.id != 0) rlUnloadTexture(shadowMapRT.texture.id);
    if (shadowMapRT.depth.id != 0) rlUnloadTexture(shadowMapRT.depth.id);
    if (shadowMapRT.id != 0) rlUnloadFramebuffer(shadowMapRT.id);
    if (shadowMapRTFar.texture.id != 0) rlUnloadTexture(shadowMapRTFar.texture.id);
    if (shadowMapRTFar.depth.id != 0) rlUnloadTexture(shadowMapRTFar.depth.id);
    if (shadowMapRTFar.id != 0) rlUnloadFramebuffer(shadowMapRTFar.id);

    free(skysphere1.mesh.tris);
    free(skysphere1.mesh.trisO);
    free(testingplatforms.mesh.tris);
    free(testingplatforms.mesh.trisO);
    delete[] testingplatforms.collider;
    delete[] testingplatforms.colliderO;
    free(cube.mesh.tris);
    free(cube.mesh.trisO);
    free(testcar.mesh.tris);
    free(testcar.mesh.trisO);
    free(testcarwheel.mesh.tris);
    free(testcarwheel.mesh.trisO);
    free(tree.mesh.tris);
    free(tree.mesh.trisO);

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
