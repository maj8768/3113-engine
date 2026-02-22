#include "raylib.h"
#include "draw.h"
#include "camera.h"
#include "physics.h"
#include <cmath>
#include <iostream>

/**

* Author: Maxim Jovanovic

* Assignment: Simple 2D Scene

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


// Global Constants
constexpr int SCREEN_WIDTH        = 400 * 1.5f,
              SCREEN_HEIGHT       = 300 * 1.5f,
              FPS                 = 60;

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


static pyramidMtx pyramid;
static planeMtx plane;
static planeMtx plane2;
static sphere_ ball;
static world world;

static camera cam = {
    .camPos = { x, h, z },
    .camTarget = { 0, h, 0 },
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

// Function Definitions
void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");

    Texture2D texo = LoadTexture(EDELGARD_FP);

    world.planeCount = 2;
    world.planes = new planeMtx[2];

    pyramid = {
        { // pyramid object-space coordinates
            x,     0.0f, z,        // v0 base
            x + s, 0.0f, z,        // v1 base
            x + s *0.5f,     0.0f, z + s,    // v2 base
            x + s*0.5f, h, z + s*0.5f  // v3 apex
        },
        WHITE, // ignored for now
        { // texture coordinates (absolute not additive)
            { 0.0f, 0.0f, 0.25f, 0.25f, 0.0f, 0.25f },
            { 0.25f, 0.0f, 0.5f, 0.0f, 0.25f, 0.25f },
            { 0.5f, 0.0f, 0.75f, 0.0f, 0.5f, 0.25f },
            { 0.75f, 0.0f, 1.0f, 0.0f, 0.75f, 0.25f },
        },
        texo // texture
    };

    plane = {
        { // plane object-space coordinates
            px,     ps, pz,        // v0 base
            px + ps, ps, pz,        // v1 base
            px + ps, 0.0f, pz + ps,    // v2 base
            px, 0.f, pz + ps  // v3 apex
        },
        WHITE, // ignored for now
        { // texture coordinates (absolute not additive)
                { 0.0f, 0.0f },
                { 0.25f, 0.0f },
                { 0.5f, 0.0f},
                { 0.75f, 0.0f },
        },
        texo // texture
    };

    plane2 = {
        { // plane object-space coordinates
            px2 + ps2,     ps2, pz2+2,           // v0 base
            px2, ps2, pz2+2,         // v1 base
            px2, 0,  pz2 - ps2+2,   // v2 base
            px2 + ps2,     0,  pz2 - ps2+2      // v3 apex
        },
        WHITE, // ignored for now
        { // texture coordinates (absolute not additive)
                    { 0.0f, 0.0f },
                    { 0.25f, 0.0f },
                    { 0.5f, 0.0f},
                    { 0.75f, 0.0f },
            },
            texo // texture
        };

    world.planes[0] = plane;
    world.planes[1] = plane2;

    createSphere(ball,
        16,
        0.25,
        {0,5,0},
        16
    );

    applyAcceleration({0.f,-9.80665f,0.f},ball); // gravity lol
    // applyForce({.25f,0.f,0.f},ball);

    SetTargetFPS(FPS);
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
}

static bool pRot = true;


float i = 1;
int g = 1;
float floating = 1;
    
void update() {

    auto ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;                   // step 3

    i += 60 * deltaTime;

    cam.camPos = { (float)(5 * cos(i * M_PI / 180.f)), 3, (float)(5 * sin(i * M_PI / 180.f)) }; // orbit x + z
    processPhysics(deltaTime, GetFPS(), ball, world);

    if (i > 361 || i < 0) {
        g *= -1;
        i = 0;
    }
}

void render()
{

    BeginDrawing();

    ClearBackground(RAYWHITE);

    // drawing world
    for (int draw = 0; draw < world.planeCount; draw++) {
        DrawPlaneFancy(world.planes[draw], cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK,true);
    }

    DrawSphere(ball,cam,SCREEN_WIDTH,SCREEN_HEIGHT);


    DrawFPS(10, 10);

    EndDrawing();
}

void shutdown() 
{ 
    CloseWindow(); // Close window and OpenGL context
    UnloadTexture(pyramid.texture);  // right here!
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
