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

static pyramidMtx pyramid;
static planeMtx plane;
static sphere_ ball;

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
            px,     0.0f, pz,        // v0 base
            px + ps, 0.0f, pz,        // v1 base
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

    createSphere(ball,
        16,
        0.25,
        {0,5,0},
        16
    );

    // gravity :/
    applyAcceleration({0.f,-9.80665f,0.f},ball); // apply gravity (normalized to 1 for some reason, idk)
    applyForce({.25f,0.f,0.f},ball);

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

    float ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;                   // step 3


    // if (g == -1) {
    //     DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RAYWHITE);
    // }
    // else {
    //     DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, DARKGRAY);
    // }

    i += 60 * deltaTime;

    cam.camPos = { (float)(5 * cos(1 * M_PI / 180.f)), 3, (float)(5 * sin(60 * M_PI / 180.f)) }; // orbit x + z
    processPhysics(deltaTime, GetFPS(), ball, plane);

    // height of pyramid follows sin ease
    // pyramid.m[0][1] += sin((i-1)/1000 * M_PI / 180.f) * g;
    // pyramid.m[1][1] += sin((i-1)/1000 * M_PI / 180.f) * g;
    // pyramid.m[2][1] += sin((i-1)/1000 * M_PI / 180.f) * g;
    // pyramid.m[3][1] += sin((i-1)/1000 * M_PI / 180.f) * g;
    // std::cout << "full camera position: " << cam.camPos.x << ", " << cam.camPos.y << ", " << cam.camPos.z << std::endl;
    // std::cout << i << std::endl;
    // std::cout << (int)abs(i) % 90 << std::endl;
    if (i > 361 || i < 0) {
        g *= -1;
        i = 0;
    }


//     if (IsKeyPressed(KEY_E)) {
//         // XYZRotatePyramidAboutSelf(pyramid, -25.0f, -25.0f, 0.0f);
//         XYZRotatePyramidAboutPoint(pyramid, pyramidPosX, pyramidPosY, 0.0f, centerX, centerY, 0.0f);
//         XYZRotatePyramidAboutPoint(pyramid2, pyramidPosX2, pyramidPosY2, 0.0f, centerX, centerY, 0.0f);
//     }
//     if (IsKeyPressed(KEY_R)) {
//         pRot = !pRot;
//     }

//     if (pRot) {
//         XYZRotatePyramidAboutSelf(pyramid, 1.f * deltaTime,1.f * deltaTime,1.f * deltaTime);
//         XYZRotatePyramidAboutSelf(pyramid2, 1.f * deltaTime,1.f * deltaTime,1.f * deltaTime);
//     }

//     // rotate pyramid to face mouse
//     // float angleX = atan2(relativeY, relativeX);

//     // ZRotateTriangleAboutSelf(triangle, -1.0f * deltaTime);
//     // ZRotateTriangleAboutPoint(triangle, 400, 400, 1.0f * deltaTime);
//     // XYZRotatePyramidAboutSelf(pyramid, -1.0f * deltaTime, -1.0f * deltaTime, -1.0f * deltaTime);
//     static float i = 0;
//     static float g = 0.01;
//     if (i < 4) {
//         // XYScaleTriangleAroundCenter(triangle, 1.0f + g);
//         i+= 1.f*deltaTime;
//         gScale.x *= 1.0f + g/10;
//         gScale.y *= 1.0f + g/10;
//         if (i > 3) {
//         }
//         else if (i > 2) {
//             textureArea = {
//                 // top-left corner
//                 (gTexture.width)*0.5,
//                 0,

//                 // bottom-right corner (of texture)
//                 (gTexture.width)/2,
//                 (gTexture.height)/2
//             };
//         }
//         else if (i > 1) {
//             textureArea = {
//                 // top-left corner
//                 0,
//                 (gTexture.height)/2,

//                 // bottom-right corner (of texture)
//                 (gTexture.width)/2,
//                 (gTexture.height)/2
//             };
//         }
//         else {
//             textureArea = {
//                 // top-left corner
//                 0,
//                 0,

//                 // bottom-right corner (of texture)
//                 static_cast<float>(gTexture.width)/2,
//                 static_cast<float>(gTexture.height)/2
//             };
//         }
//     } else {
//         i = 0;
//         g *= -1;
//     }
//     // std::cout <<  << std::endl;
}

void render()
{

    // vector3 pointWorld = { 10.0f, 2.0f, -25.0f };

    // DrawPyramidFancy(pyramid, cam, world, SCREEN_WIDTH, SCREEN_HEIGHT, RED);
    // vector2 screen;
    // bool ok = worldToScreen(pointWorld, world, view, proj, SCREEN_WIDTH, SCREEN_HEIGHT, screen);

    // if (ok) {
    //     std::cout << "Screen coordinates: (" << screen.x << ", " << screen.y << ")" << std::endl;
    // } else {
    //     std::cout << "Point is behind camera or outside the view" << std::endl;
    // }

    // float 
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawPlaneFancy(plane, cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    //DrawPyramidFancy(pyramid, cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    DrawSphere(ball,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
    // DrawPolyHedron(, 0.5, {0, 3, 0}, cam, SCREEN_WIDTH, SCREEN_HEIGHT);

    // DrawTriangleFancy(triangle, RED);

    // DrawPyramidFancy(pyramid2, RED);

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
