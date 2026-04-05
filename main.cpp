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

static Vector3 lightPos = { 0.f, 15.f, 0.f };
static Vector3 lightDir = { 0.0f, -1.f, 0.f };
static Vector4 lightColor = { 0.447, 0.816, 0.922, 1.0f };
static float ambient  = 0.05f;

static meshedObject ship;
static meshedObject control;
static meshedObject data;
static meshedObject platforms1;
static meshedObject platforms2;
static meshedObject platforms3;
static meshedObject chair1;
static meshedObject evilroomba2;
static meshedObject skysphere1;

static bool iamreal = false;
static int iamalsoreal = 1;

static Texture2D con1;
static Texture2D con2;
static Texture2D con3;

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
    .currentLevel = gameData::LEVEL3,
    .lives = 3
};

static Sound introWAV;
static Sound rocketWAV;
static Sound WarningWAV;
static Sound explosionWAV;
static Sound yayWAV;
static Sound bgmusicWAV;
static Sound bgMusicLevel1;
static Sound bgMusicLevel2;
static Sound bgMusicLevel3;
static Sound jump1;
static Sound jump2;
static Sound jump3;
static Sound jump4;
static Sound jump5;
static Sound jump6;
static Sound jump7;
static Sound chairSound;
static Sound chairScared;
static Sound deathSound;
static Sound level1win;
static Sound level2win;
static Sound level3win;
static Sound roombaDialog;
static Sound woosh;



// static camera cam = {
//     .camPos = { 5, 5, 5 },  // up and back
//     .camTarget = { 0, 0, 0 },
//     .up = {0, 1, 0},
//     .aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
//     .fov = 90.0f * M_PI / 180.0f
// };

// pyramidMtx pyramid2 = { pyramidPosX2, pyramidPosY2, 0, pyramidPosX2+size*0.5, pyramidPosY2 + size*std::sqrt(3) * 0.5, 0, pyramidPosX2+size, pyramidPosY2, 0, pyramidPosX2+size*0.5, pyramidPosY2, size };

// Global Variables
AppStatus gAppStatus   = RUNNING;

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

void temp(int d) {
    std::cout << d << std::endl;
}

void initializePlayer(player& player1) {
    // set up pc environment for player here as well
    // HideCursor();
//    SetMousePosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    DisableCursor();
    // SetMousePosition(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);

    player1.pEntity.location = {0.f,5.f,0.f};
    player1.camera.camPos = {0,5,0};
    player1.camera.camTarget = {M_PI/2.f,0,0};
    player1.camera.up = {0,1,0};
    player1.camera.aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    player1.camera.fov = 90.0f * M_PI / 180.0f;
    player1.controls = { 'W', 'A', 'S', 'D' };
    player1.canMove = true;
}

//void createSphere(sphere_& ball, int depth, float size, vector3 spawnpos) {
//    static spungonMtx ngonSpun;
//    static vector3 location = spawnpos;
//    static vector3 acceleration = {0.f,0.f,0.f};
//    static vector3 newForce;
//    static vector3 magnitude;
//    static vector3 applyAccel = {1.f,1.f,1.f}; // whether or not to apply acceleration (used to nicely stop when acceleration shouldn't affect possition)
//    ngonSpun.size = depth;
//    ngonSpun.mtxarr = new gonalMtx[depth];
//    for (int f = 0; f < depth; f++) {
//        ngonSpun.mtxarr[f].size = depth;
//        ngonSpun.mtxarr[f].mtx  = new vector3[depth];
//    }
//    ball.spungon_mtx = ngonSpun;
//    ball.location =location;
//    ball.size = size;
//    ball.newForce = newForce;
//    ball.magnitude = magnitude;
//    ball.acceleration = acceleration;
//    ball.applyAccel = applyAccel;
//}

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

void create3dObject(meshedObject& object, const char* path, const char* colliderPath, bool collider, shaderStore& shader, float scale, vector3 location) {
    triDomMesh mesh;
    object.scale = scale;
    object.pEntity.location = location;
    if (collider) objToQuads(colliderPath, object, scale);
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
            t.v[0] = { m->vertices[vi+0] + object.pEntity.location.x, m->vertices[vi+1] + object.pEntity.location.y, m->vertices[vi+2] + object.pEntity.location.z };
            t.v[1] = { m->vertices[vi+3] + object.pEntity.location.x, m->vertices[vi+4] + object.pEntity.location.y, m->vertices[vi+5] + object.pEntity.location.z };
            t.v[2] = { m->vertices[vi+6] + object.pEntity.location.x, m->vertices[vi+7] + object.pEntity.location.y, m->vertices[vi+8] + object.pEntity.location.z };
            t.n[0] = { m->normals[vi+0],  m->normals[vi+1],  m->normals[vi+2]  };
            t.n[1] = { m->normals[vi+3],  m->normals[vi+4],  m->normals[vi+5]  };
            t.n[2] = { m->normals[vi+6],  m->normals[vi+7],  m->normals[vi+8]  };
            t.t[0] = { m->texcoords[uv+0],  m->texcoords[uv+1]  };
            t.t[1] = { m->texcoords[uv+2],  m->texcoords[uv+3]  };
            t.t[2] = { m->texcoords[uv+4],  m->texcoords[uv+5]  };
            
            ot.v[0] = { m->vertices[vi+0], m->vertices[vi+1], m->vertices[vi+2] };
            ot.v[1] = { m->vertices[vi+3], m->vertices[vi+4], m->vertices[vi+5] };
            ot.v[2] = { m->vertices[vi+6], m->vertices[vi+7], m->vertices[vi+8] };
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
}

// Function Definitions
void initialise()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");
    InitAudioDevice();
    
    WarningWAV = LoadSound("resources/warning.wav");
    explosionWAV = LoadSound("resources/explosion.mp3");
    yayWAV = LoadSound("resources/yay.mp3");
    introWAV = LoadSound("resources/intro.wav");
    rocketWAV = LoadSound("resources/rocket.mp3");
    bgmusicWAV = LoadSound("resources/bgmusic.mp3");
    jump1 = LoadSound("resources/sounds/jumps/j1.mp3");
    jump2 = LoadSound("resources/sounds/jumps/j2.mp3");
    jump3 = LoadSound("resources/sounds/jumps/j3.mp3");
    jump4 = LoadSound("resources/sounds/jumps/j4.mp3");
    jump5 = LoadSound("resources/sounds/jumps/j5.mp3");
    jump6 = LoadSound("resources/sounds/jumps/j6.mp3");
    jump7 = LoadSound("resources/sounds/jumps/j7.mp3");
    chairSound = LoadSound("resources/sounds/chairdialog.mp3");
    chairScared = LoadSound("resources/sounds/chairscared.mp3");
    bgMusicLevel1 = LoadSound("resources/levels/level1/Chopin_-_The_Lost_Nocturne_KLICKAUD.mp3");
    bgMusicLevel2 = LoadSound("resources/levels/level2/Nocturne_Op9_No2_KLICKAUD.mp3");
    bgMusicLevel3 = LoadSound("resources/levels/level3/Fantaisie-Impromptu_in_C_Sharp_Minor_Op66_KLICKAUD.mp3");
    deathSound = LoadSound("resources/sounds/fall2.mp3");
    level1win = LoadSound("resources/levels/level1/win.mp3");
    level2win = LoadSound("resources/levels/level2/win.mp3");
    level3win = LoadSound("resources/levels/level3/win.mp3");
    roombaDialog = LoadSound("resources/sounds/roombadialog.mp3");
    woosh = LoadSound("resources/sounds/woosh.mp3");

    SetSoundVolume(bgMusicLevel1, 0.5f);
    SetSoundVolume(bgMusicLevel2, 0.5f);
    SetSoundVolume(bgMusicLevel3, 0.5f);

    // PlaySound(bgmusicWAV);
    SetSoundVolume(bgmusicWAV, 0.8f);
    
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


    createPlane(plane,0,{0,0,0},dimensions,texo,temp);
    createPlane(plane2,0,{0,1,0},dimensions2,texo,temp);
    initializePlayer(player1);
    SetTargetFPS(FPS);

    Shader w2s = LoadShader("resources/shaders/w2s.vs", "resources/shaders/w2s.fs");

    w2sShader.shader = w2s;
    w2sShader.lightDirLoc   = GetShaderLocation(w2sShader.shader, "uLightDir");
    w2sShader.colorLoc = GetShaderLocation(w2sShader.shader, "uColor");
    w2sShader.lightColorLoc = GetShaderLocation(w2sShader.shader, "uLightColor");
    w2sShader.ambientLoc    = GetShaderLocation(w2sShader.shader, "uAmbient");
    w2sShader.lightPosLoc = GetShaderLocation(w2sShader.shader, "uLightPos");
    w2sShader.normalLoc = GetShaderLocation(w2sShader.shader, "uNormal");
    w2sShader.texoLoc = GetShaderLocation(w2sShader.shader, "uTexo");
    w2sShader.fadeToLoc = GetShaderLocation(w2sShader.shader, "fadeTo");
    w2sShader.vpLoc = GetShaderLocation(w2sShader.shader, "uVP");
    
    con1 = LoadTexture("resources/panel1_1.png");
    con2 = LoadTexture("resources/panel1_2.png");
    con3 = LoadTexture("resources/panel1_3.png");

    // call creates before initializing the physics entity
    
    // create3dObject(ship, "resources/ship.obj", "resources/ship_collider.obj", true, w2sShader, 3.f, {0.f,0.f,0.f});
    // create3dObject(control, "resources/control.obj", "", false, w2sShader, 1.f,{0.f,0.25f,-3.8f});
    // create3dObject(data, "resources/data.obj", "", false, w2sShader, 1.2f,{-0.66f,0.f,2.33f});
//    create3dObject(cheese, "resources/cheese.obj", "", false, w2sShader, 100,{0.f,0.f,0.f});
    create3dObject(platforms1, "resources/levels/level1/level1.obj", "resources/levels/level1/colliders/level1collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    create3dObject(platforms2, "resources/levels/level2/level2.obj", "resources/levels/level2/colliders/level2collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    create3dObject(chair1, "resources/levels/level1/chair.obj", "resources/levels/level1/colliders/chaircollider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});
    create3dObject(skysphere1, "resources/levels/skysphere.obj", "", false, w2sShader, 25.f, {0.f,0.f,0.f});
    create3dObject(evilroomba2, "resources/levels/level2/evilroomba.obj", "", true, w2sShader, 1.f, {0.f,0.f,0.f});
    create3dObject(platforms3, "resources/levels/level3/level3.obj", "resources/levels/level3/colliders/level3collider.obj", true, w2sShader, 4.f, {0.f,0.f,0.f});

    /* planeMtx struct for reference:
     * struct planeMtx {
            float m[4][3];
            Color color;
            */
    
//    initializePhysicsEntity(cheese.pEntity, 12500.f); // all enities that need physics have to be initialized :/
    initializePhysicsEntity(player1.pEntity, 1.f); // even players need to be initialized because they have physics and im lazy
    initializePhysicsEntity(chair1.pEntity, 100.f);
    initializePhysicsEntity(evilroomba2.pEntity, 50.f);
    applyAcceleration({0.f,-20.5f,0.f}, player1.pEntity);
    
//    applyAcceleration({0.f,9.8f,0.f},cheese.pEntity);
//    applyForce({0.f,150.f,0.f},cheese.pEntity);

    // std::cout << ship.collider[0].m[0][0];


    gPreviousTicks = static_cast<float>(GetTime());
    // w2sShader = { w2s, SHADER_LOC_MATRIX_MVP, SHADER_LOC_COLOR_DIFFUSE, GetShaderLocation(w2s, "uLightDir") };

    gData.currentLevel = gameData::GAMESTART; // start on level 1 for testing purposes
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
}

static bool pRot = true;


float i = 0;
int g = 1;
float floating = 1;
int target = 0;

double previousTime = GetTime();
bool start = false;
int frameCount = 0;
bool warning = false;
bool success = false;
bool intro = false;
float musicVol = 0.2f;
vector2 md;;

void update() {
    double currentTime = GetTime();
    frameCount++;
    auto ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;
    
    // default player movement/look updating
    RawMouseGetDelta(md.x, md.y);
    moveLook(player1, deltaTime, md);

    int r = rand() % 7 + 1;
    Sound jumpSound = r == 1 ? jump1 : r == 2 ? jump2 : r == 3 ? jump3 : r == 4 ? jump4 : r == 5 ? jump5 : r == 6 ? jump6 : jump7; 
    
    movePlayer(gData, player1, false, deltaTime, 13, jumpSound, woosh);
    
    if (gData.currentLevel == gameData::LEVEL1) {
        meshedObject* worldObjects[] = { &platforms1, &chair1 }; 
        world worldInstance = buildWorld(worldObjects, 2);
        processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true); // last bool is for collision
        processPhysics(deltaTime, 0, chair1.pEntity, worldInstance, iamreal, iamalsoreal, false, false);
    }
    else if (gData.currentLevel == gameData::LEVEL2) {
        meshedObject* worldObjects[] = { &platforms2, &evilroomba2 }; 
        world worldInstance = buildWorld(worldObjects, 2);
        processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true); // last bool is for collision
        processPhysics(deltaTime, 0, evilroomba2.pEntity, worldInstance, iamreal, iamalsoreal, false, false);
    }
    else if(gData.currentLevel == gameData::LEVEL3) {
        meshedObject* worldObjects[] = { &platforms3 }; 
        world worldInstance = buildWorld(worldObjects, 1);
        processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true); // last bool is for collision
    }
    else if(gData.currentLevel == gameData::GAMESTART || gData.currentLevel == gameData::GAMEINFOMERCIAL) {
        getStartingInput(gData);
    }
    else if (gData.currentLevel == gameData::GAMEEND || gData.currentLevel == gameData::GAMEWIN) {
        getRestartInput(gData);
    }

    levelLogic(player1, deltaTime, gData, deathSound, level1win, level2win, level3win, bgMusicLevel1, bgMusicLevel2, bgMusicLevel3, chairSound, chairScared, roombaDialog, chair1, evilroomba2);



    //    processPhysics(deltaTime, 0, cheese.pEntity, worldInstance, iamreal, iamalsoreal, false, false);
//    applyForce({0.f,gData.thrustY,0.f}, cheese.pEntity);
    
//    updateEntityLocation(cheese);
//    updateEntityLocation(player1);
    
    // old game logic
    
//    if (!gData.gameEnded && gData.gameStarted == true && gData.infommercial == true) {
//        if (!intro) {
//            SetSoundVolume(bgmusicWAV, musicVol);
//            PlaySound(introWAV);
//            intro = true;
//        }                  // step 3
//        
//        i += deltaTime * g;
//        
//        if (i >= 1 || i <= -1) g *= -1;
//        
//        lightPos.y = 4.5f + 1.f * sinf(i);
//        
//        //    std::cout << i << std::endl;
//        
//        if (i > .5) {
//            control.texo = con1;
//        }
//        else if (i > -0.5) {
//            control.texo = con2;
//        }
//        else control.texo = con3;
//        
//        int fuelTarget = 0;
//        if (cheese.pEntity.location.y > -100.f) {
////            PauseSound(bgmusicWAV);
//            if (fabs(cheese.pEntity.magnitude.y) < 50) {
//                cheese.pEntity.magnitude.y = 10; // simulating a parachute :)
//            }
//        }
//        if (cheese.pEntity.location.y > -0.1) {
//            musicVol = musicVol + (0.25f - ((musicVol > 0.25) ? 0.25 : musicVol) * deltaTime * .20);
//            SetSoundVolume(bgmusicWAV, musicVol);
//            if (fabs(cheese.pEntity.magnitude.y) < 15) {
//                success = true;
//                PauseSound(rocketWAV);
//                PlaySound(yayWAV);
//            }
//            else {
//                PauseSound(rocketWAV);
//                PlaySound(explosionWAV);
//                SetSoundVolume(bgmusicWAV, 0.2f);
//            }
//            cheese.pEntity.location.y = -0.1;
//            cheese.pEntity.magnitude = {0.f,0.f,0.f};
//            gData.gameEnded = true;
//        }
//        else {
//            cheese.pEntity.acceleration = {0.f,9.8f,0.f};
//        }
//        float deltaAlt = (gData.alt - fabs(cheese.pEntity.location.y));
//        float deltaFuel = gData.fuel - gData.oldFuel;
//        float deltaThrustY = gData.thrustY - gData.oldThrustY;
//        gData.alt = fabs(cheese.pEntity.location.y);
//        
//        //    std::cout << "bruh" << deltaAlt/8000.f << std::endl;
//        if (start) {
//            moveUVs(data.mesh, fuel, 4, deltaFuel/700.f);
//            moveUVs(data.mesh, alt, 4, deltaAlt/15500.f);
//            //    std::cout << deltaAlt/10000.f * 0.8f << std::endl;
//            moveUVs(data.mesh, thrust, 4, deltaThrustY/2200000.f);
//        }
//        else {
//            start = true;
//        }
//        
//        if (gData.alt < 1200 && warning == false) {
////            musicVol = musicVol + (0.8 - musicVol) * deltaTime;
//            PlaySound(WarningWAV);
//            lightColor = {0.92f, 0.35f, 0.35f, 1};
//            warning = true;
//        }
//        if (gData.alt < 950.f && gData.alt > 5.f) {
//            musicVol = musicVol + (1.0f - musicVol) * deltaTime * .20;
//            SetSoundVolume(bgmusicWAV, musicVol);
//            lightColor = { 0.447, 0.816, 0.922, 1.0f };
//        }
//        
//        
////        std::cout << gData.alt << ", " << cheese.pEntity.magnitude.y << std::endl;
//        
//        // std::cout << i << std::endl;
//        vector2 md;
//        RawMouseGetDelta(md.x, md.y);
//        moveLook(player1, deltaTime, md);
//        movePlayer(gData, player1, false, deltaTime, 10, rocketWAV);
//        // void processPhysics(float deltaTime, int frameRate, player& player, world& world, bool& end, int& target)
//        processPhysics(deltaTime, 0, player1.pEntity, worldInstance, iamreal, iamalsoreal, false, true);
//        processPhysics(deltaTime, 0, cheese.pEntity, worldInstance, iamreal, iamalsoreal, false, false);
//        applyForce({0.f,gData.thrustY,0.f}, cheese.pEntity);
//        
//        updateEntityLocation(cheese);
//    }
//    else if (!(gData.gameStarted) || !(gData.infommercial)) {
//    //    std::cout << gData.gameStarted << std::endl;
//        getStartingInput(gData);
//    }

    // std::cout << "dx: " << md.x << " dy: " << md.y << std::endl;

//if (currentTime - previousTime >= .1) {
//   // Calculate FPS
//   double fps = (double)frameCount / (currentTime - previousTime);
//
//   // Display the FPS (e.g., in the window title)
////   std::cout << "[" << fps << " FPS]" << std::endl;
//   // glfwSetWindowTitle(pWindow, ss.str().c_str()); // Replace pWindow with your GLFWwindow pointer
//
//   // Reset the counter and time
//   frameCount = 0;
//   previousTime = currentTime;
//}
}

void render()
{
    ClearBackground(BLACK);

    if (/*gData.gameStarted == true && gData.gameEnded == false && gData.infommercial*/ true) { // true for now for stateless testing
    
        rlEnableDepthTest();
        rlEnableDepthMask();

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        BeginShaderMode(w2sShader.shader);
        SetShaderValue(w2sShader.shader, w2sShader.lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(w2sShader.shader, w2sShader.lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(w2sShader.shader, w2sShader.lightColorLoc, &lightColor, SHADER_UNIFORM_VEC4);
        SetShaderValue(w2sShader.shader, w2sShader.ambientLoc, &ambient, SHADER_UNIFORM_FLOAT);
        SetShaderValue(w2sShader.shader, w2sShader.fadeToLoc, &gData.fadeTo, SHADER_UNIFORM_FLOAT);

        // Draw3DGPU(ship, player1.camera, w2sShader, {255, 0, 0, 255}, ship.scale);
        // Draw3DGPU(control, player1.camera, w2sShader, {255, 0, 0, 255}, control.scale);
        // Draw3DGPU(data, player1.camera, w2sShader, {255, 0, 0, 255}, data.scale);
        switch(gData.currentLevel) {
            case gameData::LEVEL1:
                Draw3DGPU(skysphere1, player1.camera, w2sShader, {255, 0, 0, 255}, skysphere1.scale);
                Draw3DGPU(platforms1, player1.camera, w2sShader, {255, 0, 0, 255}, platforms1.scale);
                Draw3DGPU(chair1, player1.camera, w2sShader, {255, 0, 0, 255}, chair1.scale);
                break;
            case gameData::LEVEL2:
                Draw3DGPU(skysphere1, player1.camera, w2sShader, {255, 0, 0, 255}, skysphere1.scale);
                Draw3DGPU(platforms2, player1.camera, w2sShader, {255, 0, 0, 255}, platforms2.scale);
                Draw3DGPU(evilroomba2, player1.camera, w2sShader, {255, 0, 0, 255}, evilroomba2.scale);
                break;
            case gameData::LEVEL3:
                Draw3DGPU(skysphere1, player1.camera, w2sShader, {255, 0, 0, 255}, skysphere1.scale);
                Draw3DGPU(platforms3, player1.camera, w2sShader, {255, 0, 0, 255}, platforms3.scale);
                break;
            default:
                break;
        }
//        Draw3DGPU(cheese, player1.camera, w2sShader, {255, 0, 0, 255}, cheese.scale);
        EndShaderMode();

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
//        guiDrawHUD(15, SCREEN_HEIGHT - 50, GREEN, gData, cheese);

    }
//    else if (gData.gameEnded == true) {
//        if (success) {
//            guiDrawSuccess(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
//        }
//        else {
//            guiDrawFailure(SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
//        }
//    }
//    else if (gData.gameStarted == false && gData.infommercial == false) {
//        guiDrawStartMenu( SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE, 1);
//    }
//    else {
//        guiDrawStartPopup( SCREEN_WIDTH/2.f-175.f,  SCREEN_HEIGHT/2.f-100.f,  350,  200, WHITE);
//    }
//    DrawPlaneGPU(plane, player1.camera, w2sShader, {1.f,0.f,0.f,1.f}, 25.f);

//    DrawFPS(10, 10);
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
    UnloadSound(WarningWAV);     // Unload sound data
    UnloadSound(introWAV);     // Unload sound data
    UnloadSound(rocketWAV);     // Unload sound data
    UnloadSound(explosionWAV);     // Unload sound data
    UnloadSound(yayWAV);     // Unload sound data
    UnloadSound(bgmusicWAV);     // Unload sound data
    UnloadSound(jump1);
    UnloadSound(jump2);
    UnloadSound(jump3);
    UnloadSound(jump4);
    UnloadSound(jump5);
    UnloadSound(jump6);
    UnloadSound(jump7);
    UnloadSound(chairSound);
    UnloadSound(chairScared);
    UnloadSound(deathSound);
    UnloadSound(level1win);
    UnloadSound(level2win);
    UnloadSound(level3win);
    UnloadSound(roombaDialog);
    UnloadSound(woosh);

    UnloadTexture(con1);
    UnloadTexture(con2);
    UnloadTexture(con3);

    CloseAudioDevice();
    CloseWindow(); // Close window and OpenGL context
    UnloadShader(w2sShader.shader);
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
