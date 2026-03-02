#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 430
#else
    // PLATFORM_ANDROID, PLATFORM_WEB <= from web
    #define GLSL_VERSION 100
#endif

#include "raylib.h"
#include "rlgl.h"
#include "util.h"
#include "physics.h"
#include "player.h"

#include "draw.h"
#include "camera.h"
#include "gui.h"


#include <cmath>
#include <iostream>

/**
 * world to screen from shader-- source below:
 * 
 */

/**

* Author: Maxim Jovanovic

* Assignment: Pong Clone

* Date due: 02/14/2026

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



float p = 200;
float size = 15;

// Enums
enum AppStatus { TERMINATED, RUNNING };

constexpr char EDELGARD_FP[]  = "test2.png";

Vector2 gPosition = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
Vector2 gScale = { 250.0f, 250.0f };
// float gAngle = 0.0f;


Vector2 lastMousePos = {
    .x = 0,
    .y = 0
};

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

static world worldInstance;

static player player1;

static planeMtx plane;
static planeMtx plane2;

static shaderStore w2sShader;

static camera cam = {
    .camPos = { 5, 5, 5 },  // up and back
    .camTarget = { 0, 0, 0 },
    .up = {0, 1, 0},
    .aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
    .fov = 90.0f * M_PI / 180.0f
};

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

void createSphere(sphere_& ball, int depth, float size, vector3 spawnpos, int maxAccelForces) {
    static spungonMtx ngonSpun;
    static vector3 location = spawnpos;
    static vector3* accelForces = new vector3[maxAccelForces];
    static vector3 newForce;
    static vector3 magnitude;
    static vector3 applyAccel = {1,1,1}; // whether or not to apply acceleration (used to nicely stop when acceleration shouldn't affect possition)
    ngonSpun.size = depth;
    ngonSpun.mtxarr = new gonalMtx[depth];
    for (int f = 0; f < depth; f++) {
        ngonSpun.mtxarr[f].size = depth;
        ngonSpun.mtxarr[f].mtx  = new vector3[depth];
    }
    ball.spungon_mtx = ngonSpun;
    ball.location =location;
    ball.size = size;
    ball.newForce = newForce;
    ball.magnitude = magnitude;
    ball.accelForces = accelForces;
    ball.maxAccelForces = maxAccelForces;
    ball.accelForcesCount = 0;
    ball.applyAccel = applyAccel;
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

// Function Definitions
void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");

    Texture2D texo = LoadTexture(EDELGARD_FP);

    static float dimensions[4][3] = {
        {-2,0,-2},
        {-2,0,2},
        {2,0,2},
        {2,0,-2}
    };

    static float dimensions2[4][3] = {
        {-2,-2,-2},
        {-2,-2,2},
        {2,-2,2},
        {2,-2,-2}
    };


    createPlane(plane,0,{0,0,0},dimensions,texo,temp);
    createPlane(plane2,0,{0,0,0},dimensions2,texo,temp);
    SetTargetFPS(FPS);

    Shader w2s = LoadShader("resources/shaders/w2s.vs", "resources/shaders/w2s.fs");

    w2sShader.shader = w2s;
    w2sShader.lightDirLoc   = GetShaderLocation(w2sShader.shader, "uLightDir");
    w2sShader.colorLoc = GetShaderLocation(w2sShader.shader, "uColor");
    w2sShader.lightColorLoc = GetShaderLocation(w2sShader.shader, "uLightColor");
    w2sShader.ambientLoc    = GetShaderLocation(w2sShader.shader, "uAmbient");
    w2sShader.lightPosLoc = GetShaderLocation(w2sShader.shader, "uLightPos");

    // w2sShader = { w2s, SHADER_LOC_MATRIX_MVP, SHADER_LOC_COLOR_DIFFUSE, GetShaderLocation(w2s, "uLightDir") };
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
    
void update() {
    auto ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;                   // step 3

    i += deltaTime;

    // std::cout << i << std::endl;

    cam.camPos = { (float)(3 * cos(90 * M_PI / 180.f)), i, (float)(3 * sin(90 * M_PI / 180.f)) }; // orbit x + z
    // int target = 0;
}

void render()
{
    BeginDrawing();

    rlClearColor(0, 0, 0, 255);

    // changing to 3d setup and rlgl

    rlClearScreenBuffers();
    rlEnableDepthTest();
    rlEnableDepthMask();

    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    DrawPlaneGPU(plane, cam, w2sShader, {255,0,0,255});
    // DrawPlaneGPU(plane2, cam, w2sShader, {0,255,0,255});

    // back to 2d raylib

    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    DrawFPS(10, 10);
    EndDrawing();
}

void shutdown() 
{ 
    CloseWindow(); // Close window and OpenGL context
    UnloadShader(w2sShader.shader);
    // UnloadTexture(pyramid.texture);  // right here!
}

int main(void)
{
    std::cout << "Hello, World!" << std::endl;
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}
