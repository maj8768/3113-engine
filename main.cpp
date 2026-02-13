#include "raylib.h"
#include "draw.h"
#include "camera.h"
#include <math.h>
#include <iostream>
#include <vector>


float p = 200;
float size = 15;

// Enums
enum AppStatus { TERMINATED, RUNNING };

constexpr char EDELGARD_FP[]  = "demon_days.png";

Texture2D gTexture;

// Global Constants
constexpr int SCREEN_WIDTH        = 800 * 1.5f,
              SCREEN_HEIGHT       = 600 * 1.5f,
              FPS                 = 60;

Vector2 gPosition = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
Vector2 gScale = { 250.0f, 250.0f };
float gAngle = 0.0f;


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

pyramidMtx pyramid = {
  x,     0.0f, z,        // v0 base
  x + s, 0.0f, z,        // v1 base
  x + s *0.5f,     0.0f, z + s,    // v2 base
  x + s*0.5f, h, z + s*0.5f  // v3 apex
};

static camera cam = {
    .camPos = { x + s*0.5f, h,      z + s*3.0f },
    .camTarget = { x + s*0.5f, h*0.5f, z + s*0.5f },
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

// Function Definitions
void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");

    gTexture = LoadTexture(EDELGARD_FP);

    SetTargetFPS(FPS);
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
}

static bool pRot = true;
static Rectangle textureArea = {
        // top-left corner
        0.0f, 0.0f,

        // bottom-right corner (of texture)
        static_cast<float>(gTexture.width),
        static_cast<float>(gTexture.height)
    };


float i = 0;
int g = 1;
    
void update() {

    float ticks = (float) GetTime();          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;                   // step 3

    std::cout << "update" << std::endl;

    i += 0.1 * g;

    cam.camPos    = { x + s*0.5f + i, h,      z + s*3.0f };
    std::cout << "full camera position: " << cam.camPos.x << ", " << cam.camPos.y << ", " << cam.camPos.z << std::endl;
    if (i > 10 || i < 0) {
        g *= -1;
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
//             textureArea = {
//                 // top-left corner
//                 (gTexture.width)/2,
//                 (gTexture.height)/2,

//                 // bottom-right corner (of texture)
//                 (gTexture.width)/2,
//                 (gTexture.height)/2
//             };
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

    DrawPyramidFancy(pyramid, cam, SCREEN_WIDTH, SCREEN_HEIGHT, RED);

    // DrawTriangleFancy(triangle, RED);

    // DrawPyramidFancy(pyramid2, RED);

    // Rectangle destinationArea = {
    //     gPosition.x,
    //     gPosition.y,
    //     static_cast<float>(gScale.x),
    //     static_cast<float>(gScale.y)
    // };

    // Vector2 originOffset = {
    //     static_cast<float>(gScale.x) / 2.0f,
    //     static_cast<float>(gScale.y) / 2.0f
    // };

    // DrawTexturePro(
    //     gTexture, 
    //     textureArea, 
    //     destinationArea, 
    //     originOffset, 
    //     gAngle, 
    //     WHITE
    // );

    DrawFPS(10, 10);

    EndDrawing();
}

void shutdown() 
{ 
    CloseWindow(); // Close window and OpenGL context
    UnloadTexture(gTexture);  // right here!
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
