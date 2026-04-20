#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 410
#else
    // PLATFORM_ANDROID, PLATFORM_WEB <= from web
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

/**
 * world to screen from shader-- source below:
 * 
 */

/**

/**
* Author: Maxim Jovanovic
* Assignment: Rise of the AI
* Date due: 2026-04-04, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

/* 3 objects are:
 * 1. face 1 of pyramid
 * 2. face 2 of pyramid
 * 3. face 3 of pyramid
 * 4. HM: face 4 of pyramid
 *
 * The camera orbits the pyramid/plane.
 * The camera is still in screen space but constantly rotating in object space
 * The faces are technically constantly scaling in screen space, but remain stationary in object space
 * The entire pyramid translates along the y-axis in object space and because the camera has no-
 * vertical movement it also translates along the y-axis in screen space!
 *
 * The background shifts from gray to white as the camera completes a full orbit!
 *
 */

int thrust[] = { 249, 250, 119, 120 };
int alt[] = { 99, 101, 227, 229};
int fuel[] = { 47, 49, 173, 174 };


float p = 200;
float size = 15;

// Enums
enum AppStatus { TERMINATED, RUNNING };

constexpr char EDELGARD_FP[]  = "test2.png";

vector2 gPosition = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
vector2 gScale = { 250.0f, 250.0f };
// float gAngle = 0.0f;

static float gPreviousTicks = 0.0f;

// float relativeRotationX = 360.0f / centerX;
// float relativeRotationY = 360.0f / centerY;

float x = 0.0f;      // pyramidPosX
float z = 0.0f;      // pyramidPosZ
float s = 2.0f;      // base size
float h = 2.0f;      // height

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

static int depthLightSpaceLoc = -1;

static RenderTexture2D shadowMapRT = { 0 };
static constexpr int SHADOW_MAP_SIZE = 2048;
static constexpr float SHADOW_RANGE = 60.0f;
static constexpr float SHADOW_NEAR = 1.0f;
static constexpr float SHADOW_FAR = 180.0f;
static constexpr float LIGHT_ORBIT_SPEED = 0.15f;
static constexpr float LIGHT_ORBIT_RADIUS_Y = 150.0f;
static constexpr float LIGHT_ORBIT_RADIUS_Z = 25.0f;
static constexpr float LIGHT_ORBIT_X_OFFSET = 55.0f;

static Vector3 lightPos = { 250.f, 1000.f, 0.f };
static Vector3 lightDir = { -0.10f, -0.99f, -0.05f };
static Vector4 lightColor = { 0.447, 0.816, 0.922, 1.0f };
static float ambient  = 0.55f; // 0.05

// other prep
static meshedObject skysphere1;
static meshedObject cube;
static meshedObject testingplatforms;
static meshedObject testcar;
static meshedObject testcarwheel;

// map prep
static meshedObject gasPump;
static meshedObject gasPumpNozzle;
static meshedObject gasPumpNozzleOff;

static meshedObject fakeMesh; // fake and isnt real

static bool iamreal = false;
static int iamalsoreal = 1;

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

static Music carEngine;

AppStatus gAppStatus = RUNNING;


// Global Variables

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();
static mtx44 BuildLightSpaceMatrix();
static void RenderShadowMapPass(const mtx44& lightSpace);
static void DrawSceneWithShadows(const mtx44& frameVP, const mtx44& lightSpace);

static RenderTexture2D LoadShadowMapRenderTexture(int width, int height) {
    RenderTexture2D target = { 0 };

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

static mtx44 BuildLightSpaceMatrix() {
    // Centre the shadow frustum on the player/car so shadows follow as it moves.
    vector3 sceneCenter = player1.pEntity.location;
    vector3 lightPos3 = {
        sceneCenter.x + 15.f,
        sceneCenter.y + 15.f,
        sceneCenter.z + 15.f
    };
    lightPos = { lightPos3.x, lightPos3.y, lightPos3.z };

    vector3 lightDirVec = sceneCenter - lightPos3;
    if (len3(lightDirVec) <= eps) lightDirVec = { 0.f, -1.f, 0.f };
    lightDirVec = normalize3(lightDirVec);
    lightDir = { lightDirVec.x, lightDirVec.y, lightDirVec.z };

    mtx44 lightView = lookAtMtx44(lightPos3, sceneCenter, {0.f, 1.f, 0.f});
    mtx44 lightProj = orthoMtx44(-SHADOW_RANGE, SHADOW_RANGE, -SHADOW_RANGE, SHADOW_RANGE, SHADOW_NEAR, SHADOW_FAR);
    return mmult4(lightProj, lightView);
}


void initializePlayer(player& player1) {
    // set up pc environment for player here as well
    DisableCursor();

    player1.pEntity.location = {0.f,5.f,0.f};
    player1.camera.camPos = {0,5,0};
    player1.camera.camTarget = {M_PI/2.f,0,0};
    player1.camera.up = {0,1,0};
    player1.camera.aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    player1.camera.fov = 90.0f * M_PI / 180.0f;
    player1.controls = { 'W', 'A', 'S', 'D' };
    player1.canMove = true;
    player1.pState = { false, false, 0.f, 0.f, 100.f };

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

void create3dObject(meshedObject& object, const char* path, const char* colliderPath, bool collider, shaderStore& shader, float scale, vector3 location, char text[100], bool renderText, float textRenderDistance, vector3 relativeTextOffset ) {
    memcpy(object.text, text, 100);
    object.renderText = renderText;
    object.textRenderDistance = textRenderDistance;
    object.relativeTextOffset = relativeTextOffset;
    triDomMesh mesh;
    object.scale = scale;
    object.pEntity.location = location;
    if (collider) objToQuads(colliderPath, object, scale, player1, false);
    Model model = LoadModel(path);
    int totalTriangles = 0;
    for (int i = 0; i < model.meshCount; i++) {
        totalTriangles += model.meshes[i].triangleCount;
    }

    mesh.tris  = (tri*)malloc(totalTriangles * sizeof(tri));
    mesh.trisO = (tri*)malloc(totalTriangles * sizeof(tri));
    mesh.count = 0;
    for (int j = 0; j < 5; j++) {  // just first 5 tris
    int uv = j * 6;
}
    for (int i = 0; i < model.meshCount; i++) {
        Mesh* m = &model.meshes[i];
        Texture2D tex = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].texture;
        // Texture2D tex = model.materials[model.meshMaterialId[i]].maps[MAP_DIFFUSE].texture;
        object.texo = tex;
        for (int j = 0; j < m->triangleCount; j++) {
            tri t;
            tri ot;
            int vi = j * 9; // splits by 3 verts and then by 3 coords
            int uv = j * 6; // split by 3verts and then by 2uv coords
            t.v[0] = { m->vertices[vi+0] * scale + object.pEntity.location.x, m->vertices[vi+1] * scale + object.pEntity.location.y, m->vertices[vi+2] * scale + object.pEntity.location.z };
            t.v[1] = { m->vertices[vi+3] * scale + object.pEntity.location.x, m->vertices[vi+4] * scale + object.pEntity.location.y, m->vertices[vi+5] * scale + object.pEntity.location.z };
            t.v[2] = { m->vertices[vi+6] * scale + object.pEntity.location.x, m->vertices[vi+7] * scale + object.pEntity.location.y, m->vertices[vi+8] * scale + object.pEntity.location.z };
            t.n[0] = { m->normals[vi+0],  m->normals[vi+1],  m->normals[vi+2]  };
            t.n[1] = { m->normals[vi+3],  m->normals[vi+4],  m->normals[vi+5]  };
            t.n[2] = { m->normals[vi+6],  m->normals[vi+7],  m->normals[vi+8]  };
            t.t[0] = { m->texcoords[uv+0],  m->texcoords[uv+1]  };
            t.t[1] = { m->texcoords[uv+2],  m->texcoords[uv+3]  };
            t.t[2] = { m->texcoords[uv+4],  m->texcoords[uv+5]  };

            ot.v[0] = { m->vertices[vi+0] * scale, m->vertices[vi+1] * scale, m->vertices[vi+2] * scale };
            ot.v[1] = { m->vertices[vi+3] * scale, m->vertices[vi+4] * scale, m->vertices[vi+5] * scale };
            ot.v[2] = { m->vertices[vi+6] * scale, m->vertices[vi+7] * scale, m->vertices[vi+8] * scale };
            ot.n[0] = { m->normals[vi+0],  m->normals[vi+1],  m->normals[vi+2]  };
            ot.n[1] = { m->normals[vi+3],  m->normals[vi+4],  m->normals[vi+5]  };
            ot.n[2] = { m->normals[vi+6],  m->normals[vi+7],  m->normals[vi+8]  };
            ot.t[0] = { m->texcoords[uv+0],  m->texcoords[uv+1]  };
            ot.t[1] = { m->texcoords[uv+2],  m->texcoords[uv+3]  };
            ot.t[2] = { m->texcoords[uv+4],  m->texcoords[uv+5]  };
            mesh.tris[mesh.count] = t;
            mesh.trisO[mesh.count] = ot;
            mesh.count++;
        }
    }
    // std::cout << "v count: " << mesh.count << std::endl;
    object.mesh = mesh;
    UnloadModel(model);
}

// Function Definitions
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
    startCarEngineThread(carEngine);

    
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


    // createPlane(plane,0,{0,0,0},dimensions,texo,temp);
    // createPlane(plane2,0,{0,1,0},dimensions2,texo,temp);
    initializePlayer(player1);
    SetTargetFPS(FPS);

    Shader w2s = LoadShader("resources/shaders/w2s.vs", "resources/shaders/w2s.fs");

    w2sShader.shader = w2s;
    w2sShader.lightDirLoc   = GetShaderLocation(w2sShader.shader, "uLightDir");
    w2sShader.colorLoc = GetShaderLocation(w2sShader.shader, "uColor");
    w2sShader.lightColorLoc = GetShaderLocation(w2sShader.shader, "uLightColor");
    w2sShader.ambientLoc    = GetShaderLocation(w2sShader.shader, "uAmbient");
    w2sShader.normalLoc = GetShaderLocation(w2sShader.shader, "uNormal");
    w2sShader.texoLoc = GetShaderLocation(w2sShader.shader, "uTexo");
    w2sShader.fadeToLoc = GetShaderLocation(w2sShader.shader, "fadeTo");
    w2sShader.vpLoc = GetShaderLocation(w2sShader.shader, "uVP");
    w2sShader.lightSpaceMatrixLoc = GetShaderLocation(w2sShader.shader, "uLightSpaceMatrix");
    w2sShader.shadowMapLoc = GetShaderLocation(w2sShader.shader, "uShadowMap");
    w2sShader.shadowsEnabledLoc = GetShaderLocation(w2sShader.shader, "uShadowsEnabled");

    depthShader = LoadShader("resources/shaders/depth.vs", "resources/shaders/depth.fs");
    depthLightSpaceLoc = GetShaderLocation(depthShader, "uLightSpaceMatrix");

    shadowMapRT = LoadShadowMapRenderTexture(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

    // call creates before initializing the physics entity
    
    // create3dObject(ship, "resources/ship.obj", "resources/ship_collider.obj", true, w2sShader, 3.f, {0.f,0.f,0.f});
    // create3dObject(control, "resources/control.obj", "", false, w2sShader, 1.f,{0.f,0.25f,-3.8f});
    // create3dObject(data, "resources/data.obj", "", false, w2sShader, 1.2f,{-0.66f,0.f,2.33f});
//    create3dObject(cheese, "resources/cheese.obj", "", false, w2sShader, 100,{0.f,0.f,0.f});
    // create3dObject(platforms1, "resources/levels/level1/level1.obj", "resources/levels/level1/colliders/level1collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    // create3dObject(platforms2, "resources/levels/level2/level2.obj", "resources/levels/level2/colliders/level2collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    // create3dObject(chair1, "resources/levels/level1/chair.obj", "resources/levels/level1/colliders/chaircollider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    char empty[100] = "";
    char cubeText[100] = "I am a cube";
    create3dObject(skysphere1, "resources/levels/skysphere.obj", "", false, w2sShader, 25.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    // create3dObject(evilroomba2, "resources/levels/level2/evilroomba.obj", "", true, w2sShader, 1.f, {0.f,0.f,0.f});
    // create3dObject(platforms3, "resources/levels/level3/level3.obj", "resources/levels/level3/colliders/level3collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    create3dObject(testingplatforms, "resources/levels/testing/testplatform.obj", "resources/levels/testing/colliders/testplatformcollider.obj", true, w2sShader, 1.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(cube, "resources/levels/testing/cube.obj", "", false, w2sShader, 1.f, {7.f,5.f,3.f}, cubeText, true, 1.5f, {0,-1.5,0});
    create3dObject(testcar, "resources/levels/testing/testcar.obj", "resources/levels/testing/colliders/testcarcollider.obj", true, w2sShader, 4.f, {0.f,5.f,15.f}, empty, false, 100, {0,0,0});
    create3dObject(testcarwheel, "resources/levels/testing/testcarwheel.obj", "", false, w2sShader, 4.f, {0.f,0.f,0.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPump, "resources/levels/testing/gas_pump/pump/gas_pump.obj", "resources/levels/testing/gas_pump/colliders/pumpcollider.obj", true, w2sShader, 1.f, {10.f,0.f,10.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPumpNozzle, "resources/levels/testing/gas_pump/grab/grab_onpump.obj", "", false, w2sShader, 1.f, {10.f,0.f,10.f}, empty, false, 100, {0,0,0});
    create3dObject(gasPumpNozzleOff, "resources/levels/testing/gas_pump/grab/grab_offpump.obj", "", false, w2sShader, 1.f, {10.f,-20.f,10.f}, empty, false, 100, {0,0,0});
    /* planeMtx struct for reference:
     * struct planeMtx {
            float m[4][3];
            Color color;
            */
    
//    initializePhysicsEntity(cheese.pEntity, 12500.f); // all enities that need physics have to be initialized :/
    initializePhysicsEntity(player1.pEntity, 1.f, COMPLEX); // even players need to be initialized because they have physics and im lazy
    initializePhysicsEntity(testcar.pEntity, 1000.f, COMPLEX);

    initializePhysicsEntity(testcarwheel.pEntity, 100.f, COMPLEX);

    // testcar.pEntity.rot = {0.f, 180.f, 0.f};
    updateColliderLocation(fakeMesh,player1,true);
    updateColliderLocation(testcar,player1,false);

    // initializePhysicsEntity(chair1.pEntity, 100.f);
    // initializePhysicsEntity(evilroomba2.pEntity, 50.f);
    applyAcceleration({0.f,-20.5f,0.f}, player1.pEntity);

    applyAcceleration({0.f,-20.5f,0.f},testcar.pEntity);
//    applyForce({0.f,150.f,0.f},cheese.pEntity);

    // std::cout << ship.collider[0].m[0][0];


    gPreviousTicks = static_cast<float>(GetTime());

}

// void processInput()
// {
//     if (WindowShouldClose()) gAppStatus = TERMINATED;
// }

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


void update() {

    auto ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;

    // default player movement/look updating
    RawMouseGetDelta(md.x, md.y);
    moveLook(player1, deltaTime, md);

    int r = rand() % 7 + 1;
    Sound jumpSound = r == 1 ? jump1 : r == 2 ? jump2 : r == 3 ? jump3 : r == 4 ? jump4 : r == 5 ? jump5 : r == 6 ? jump6 : jump7; 
    
    movePlayer(gData, player1, testcar, false, deltaTime, 13, jumpSound);
    // lightPos.y = player1.pEntity.location.y + 10.f;
    // lightPos.x = player1.pEntity.location.x;
    // lightPos.z = player1.pEntity.location.z;
    g++;
    cube.pEntity.location.x = 1.f + sinf(g * 0.00025) * 5.f;
    cube.pEntity.location.z = -1.f + sinf(g * 0.00025) * 5.f;
    // lightPos.x = player1.pEntity.location.x;
    // lightPos.z = player1.pEntity.location.z;
    // lightPos.y = player1.pEntity.location.y + 9.f;
    updateEntityLocation(cube);
// meshedObject* worldObjects[] = { &testingplatforms };
//             world worldInstance = buildWorld(worldObjects, 1);
//             processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true); // last bool is for collision
            // break;
    switch (gData.currentLevel) {
        case gameData::TESTING_ENVIRONMENT: {
            updateColliderLocation(fakeMesh,player1,true);
            meshedObject* worldObjects[] = { &testingplatforms, &testcar, &gasPump };
            meshedObject* secondaryObjects[] = { &testingplatforms, &gasPump };
            world worldInstance = buildWorld(worldObjects, 3, player1);
            if (!player1.pState.inCar) {
                processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, player1.collider, player1.cPlaneCount); // last bool is for collision
            }
            world secondaryInstance = buildWorld(secondaryObjects, 2, player1);
            processPhysics(deltaTime, 0, testcar.pEntity, secondaryInstance, iamreal, iamalsoreal, false, true, testcar.collider, testcar.cPlaneCount); // last bool is for collision
            updateEntityLocation(testcar);
            testcarwheel.pEntity = testcar.pEntity;
            // updateEntityLocation(testcarwheel);
            processCar(testcar, testcarwheel, player1, deltaTime, carEngine);
            // std::cout << "car rot: " << testcar.pEntity.rot.y << std::endl;
            applyRot(testcarwheel.pEntity.location, testcar.pEntity.rot, 0.f,0.f,0.f);
            updateEntityLocation(testcarwheel);
            updateEntityLocation(gasPumpNozzle);
            updateEntityLocation(gasPumpNozzleOff);
            // updateColliderLocation(testcar,player1,false);

            // Re-sync camera to car's post-physics position so mesh and camera match
            if (player1.pState.inCar) {
                player1.pEntity.location = testcar.pEntity.location;
                vector3 camPos = player1.pEntity.location;
                applyRot(camPos, testcar.pEntity.rot, -1.5f, 4.25, -0.45);
                player1.camera.camPos = camPos;
            }

            break;
        }
        case gameData::LEVEL1: {
            meshedObject* worldObjects[] = {  };
            world worldInstance = buildWorld(worldObjects, 0, player1);
            processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, nullptr, 0); // last bool is for collision
            break;
        }
        case gameData::LEVEL2: {
            meshedObject* worldObjects[] = {  };
            world worldInstance = buildWorld(worldObjects, 0, player1);
            processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, nullptr, 0); // last bool is for collision
            break;
        }
        case gameData::LEVEL3: {
            meshedObject* worldObjects[] = {  };
            world worldInstance = buildWorld(worldObjects, 0, player1);
            processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true, nullptr, 0); // last bool is for collision
            break;
        }
        case gameData::GAMESTART:
        case gameData::GAMEINFOMERCIAL:
            getStartingInput(gData);
            break;
        case gameData::GAMEEND:
        case gameData::GAMEWIN:
            getRestartInput(gData);
            break;
    }

    static double lastPrintTime = 0.0;
    double now = GetTime();
    if (now - lastPrintTime >= 0.25) {
        std::cout << "FPS: " << GetFPS() << std::endl;
        lastPrintTime = now;
    }
    levelLogic(player1, deltaTime, gData, testcar, gasPump, gasPumpNozzle, gasPumpNozzleOff);
    // levelLogic(player1, deltaTime, gData, deathSound, level1win, level2win, level3win, bgMusicLevel1, bgMusicLevel2, bgMusicLevel3, chairSound, chairScared, roombaDialog, chair1, evilroomba2);
}

// this bs (MAKE SURE TO DUAL RENDER, SHADOWS AND SCENE BOTH HAVE TO BE RENDERED) Load into shadow map then draw the map and shadows.

static void RenderShadowMapPass(const mtx44& lightSpace) {
    if (shadowMapRT.id == 0) return;

    rlViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    rlEnableFramebuffer(shadowMapRT.id);
    rlClearScreenBuffers();

    BeginShaderMode(depthShader);
    SetShaderValueMatrix(depthShader, depthLightSpaceLoc, ToRaylibMatrix(lightSpace));

    // Solid closed objects: front-face cull so back-face depth is stored.
    // Back-face depth > front-face depth so the comparison passes without bias,
    // eliminating peter panning on these objects entirely.
    rlEnableBackfaceCulling();
    // rlSetCullFace(RL_CULL_FACE_FRONT);
    Draw3DDepthGPU(cube);
    Draw3DDepthGPU(testcar);
    Draw3DDepthGPU(testcarwheel);
    Draw3DDepthGPU(gasPump);
    Draw3DDepthGPU(gasPumpNozzle);
    Draw3DDepthGPU(gasPumpNozzleOff);

    // Flat/open geometry: use normal back-face culling so the only face is rendered.
    // rlSetCullFace(RL_CULL_FACE_BACK);
    Draw3DDepthGPU(testingplatforms);
    EndShaderMode();

    rlDisableFramebuffer();
    rlViewport(0, 0, GetRenderWidth(), GetRenderHeight());
}

static void DrawSceneWithShadows(const mtx44& frameVP, const mtx44& lightSpace) {
    BeginShaderMode(w2sShader.shader);
    SetShaderValue(w2sShader.shader, w2sShader.lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(w2sShader.shader, w2sShader.lightColorLoc, &lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(w2sShader.shader, w2sShader.ambientLoc, &ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(w2sShader.shader, w2sShader.fadeToLoc, &gData.fadeTo, SHADER_UNIFORM_FLOAT);
    SetShaderValueMatrix(w2sShader.shader, w2sShader.lightSpaceMatrixLoc, ToRaylibMatrix(lightSpace));
    const bool hasShadowMap = shadowMapRT.depth.id != 0;
    const Texture2D* shadowTex = hasShadowMap ? &shadowMapRT.depth : nullptr;

    switch(gData.currentLevel) {
        case gameData::TESTING_ENVIRONMENT:
            Draw3DGPU(skysphere1, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, false);
            Draw3DGPU(testingplatforms, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(testcar, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(testcarwheel, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(cube, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(gasPump, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(gasPumpNozzle, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);
            Draw3DGPU(gasPumpNozzleOff, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, hasShadowMap);

            DrawColliderGPU(testcar.cPlaneCount, testcar.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
            DrawColliderGPU(player1.cPlaneCount, player1.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
            DrawColliderGPU(gasPump.cPlaneCount, gasPump.collider, player1.camera, w2sShader, {0, 255, 0, 255}, &frameVP);
            
            DrawPlaneNormalsGPU(testcar.cPlaneCount, testcar.collider, player1.camera, w2sShader, {255, 0, 0, 255}, 1.0f, &frameVP);

            break;
        case gameData::LEVEL1:
        case gameData::LEVEL2:
        case gameData::LEVEL3:
            Draw3DGPU(skysphere1, player1.camera, w2sShader, {255, 0, 0, 255}, &frameVP, shadowTex, false);
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

    if (/*gData.gameStarted == true && gData.gameEnded == false && gData.infommercial*/ true) { // true for now for stateless testing

        rlEnableDepthTest();
        rlEnableDepthMask();

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        mtx44 lightSpace = BuildLightSpaceMatrix();
        RenderShadowMapPass(lightSpace);

        // Compute VP once per frame and share across all Draw3DGPU calls
        mtx44 frameView = viewMtx44(player1.camera.camPos, player1.camera.camTarget, player1.camera.up);
        mtx44 frameProj = projMtx44(player1.camera.fov, player1.camera.aspect, 0.1f, 1000000000000000000.0f);
        mtx44 frameVP   = mmult4(frameProj, frameView);

        DrawSceneWithShadows(frameVP, lightSpace);

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlDisableDepthTest();

        switch(gData.currentLevel) {
            case gameData::GAMEEND:
                guiDrawFailure(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
                break;
            case gameData::GAMEWIN:
                guiDrawSuccess(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
                break;
            case gameData::GAMESTART:
                guiDrawStartMenu(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE, 1);
                break;
            case gameData::GAMEINFOMERCIAL:
                guiDrawStartPopup(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
                break;
            default:
                break;
        }
    }
}

void shutdown() 
{
    /*
     static Sound introWAV;
     static Sound rocketWAV;
     static Sound WarningWAV;
     static Sound explosionWAV;
     static Sound yayWAV;
     static Sound bgmusicWAV;
     */
    UnloadSound(jump1);
    UnloadSound(jump2);
    UnloadSound(jump3);
    UnloadSound(jump4);
    UnloadSound(jump5);
    UnloadSound(jump6);
    UnloadSound(jump7);
    stopCarEngineThread();
    UnloadMusicStream(carEngine);


    UnloadShader(w2sShader.shader);
    UnloadShader(depthShader);
    if (shadowMapRT.texture.id != 0) rlUnloadTexture(shadowMapRT.texture.id);
    if (shadowMapRT.depth.id != 0) rlUnloadTexture(shadowMapRT.depth.id);
    if (shadowMapRT.id != 0) rlUnloadFramebuffer(shadowMapRT.id);

    // Free CPU mesh data for all loaded objects
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


    CloseAudioDevice();
    CloseWindow(); // Close window and OpenGL context
    // UnloadTexture(pyramid.texture);  // right here!
}


int main(void)
{
    std::cout << "Hello, World!" << std::endl;
    initialise();
    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // just to make sure everything is loaded before the game starts, not really necessary but it looks nicer this way
    while (gAppStatus == RUNNING)
    {
        update();
        BeginDrawing();
        render();
        EndDrawing();
        #ifdef _WIN32
            // SwapScreenBuffer();
            pumpMessages();
        #endif
    }

    RawMouseShutdown();
    shutdown();
    return 0;
}
