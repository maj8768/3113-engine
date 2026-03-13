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

#include "draw/draw.h"
#include "draw/gui.h"

#include "camera/camera.h"

#include "system/keyboard/keyboard.h"
#include "system/mouse/mouse.h"


#include <cmath>
#include <iostream>
#include <thread>

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

static world worldInstance;

static player player1;

static planeMtx plane;
static planeMtx plane2;

static shaderStore w2sShader;

static Vector3 lightPos = { 0.f, 4.f,0.f };
static Vector3 lightDir = { 0.0f, -1.f, 0.f };
static Vector4 lightColor = { 0.447, 0.816, 0.922, 1.0f };
static float ambient  = 0.05f;

static triDomMesh mesh;

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

    player1.location = {0,0,0};
    player1.camera.camPos = {0,5,0};
    player1.camera.camTarget = {M_PI/2.f,0,0};
    player1.camera.up = {0,1,0};
    player1.camera.aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    player1.camera.fov = 90.0f * M_PI / 180.0f;
    player1.controls = { 'W', 'A', 'S', 'D' };
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

void create3dObject(triDomMesh& mesh, const char* path, shaderStore& shader) {
    Model model = LoadModel(path);
    mesh.count = 0;
    mesh.tris  = (tri*)malloc(model.meshes[0].triangleCount * sizeof(tri));
    for (int j = 0; j < 5; j++) {  // just first 5 tris
    int uv = j * 6;
}
    for (int i = 0; i < model.meshCount; i++) {
        Mesh* m = &model.meshes[i];
        Texture2D tex = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].texture;
        shader.texo = tex;
        // Texture2D tex = model.materials[model.meshMaterialId[i]].maps[MAP_DIFFUSE].texture;
        for (int j = 0; j < m->triangleCount; j++) {
            tri t;
            int vi = j * 9; // splits by 3 verts and then by 3 coords
            int uv = j * 6; // split by 3verts and then by 2uv coords
            t.v[0] = { m->vertices[vi+0], m->vertices[vi+1], m->vertices[vi+2] };
            t.v[1] = { m->vertices[vi+3], m->vertices[vi+4], m->vertices[vi+5] };
            t.v[2] = { m->vertices[vi+6], m->vertices[vi+7], m->vertices[vi+8] };
            t.n[0] = { m->normals[vi+0],  m->normals[vi+1],  m->normals[vi+2]  };
            t.n[1] = { m->normals[vi+3],  m->normals[vi+4],  m->normals[vi+5]  };
            t.n[2] = { m->normals[vi+6],  m->normals[vi+7],  m->normals[vi+8]  };
            t.t[0] = { m->texcoords[uv+0],  m->texcoords[uv+1]  };
            t.t[1] = { m->texcoords[uv+2],  m->texcoords[uv+3]  };
            t.t[2] = { m->texcoords[uv+4],  m->texcoords[uv+5]  };
            mesh.tris[mesh.count++] = t;
        }
    }
    // std::cout << "v count: " << mesh.count << std::endl;
}

// Function Definitions
void initialise()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello raylib!");

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
    w2sShader.vpLoc = GetShaderLocation(w2sShader.shader, "uVP");
    

    create3dObject(mesh, "resources/ship.obj", w2sShader);

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

double previousTime = GetTime();
int frameCount = 0;

void update() {
    double currentTime = GetTime();
    frameCount++;
    auto ticks = static_cast<float>(GetTime());          // step 1
    float deltaTime = ticks - gPreviousTicks; // step 2
    gPreviousTicks = ticks;                   // step 3

    i += deltaTime;

    lightPos.y = 4.5f + 1.f * sinf(i);

    // std::cout << i << std::endl;
    vector2 md;
    RawMouseGetDelta(md.x, md.y);
    moveLook(player1, deltaTime, md);
    movePlayer(player1, false, deltaTime);

    // std::cout << "dx: " << md.x << " dy: " << md.y << std::endl;

if (currentTime - previousTime >= .1) {
    // Calculate FPS
    double fps = (double)frameCount / (currentTime - previousTime);

    // Display the FPS (e.g., in the window title)
    std::cout << "[" << fps << " FPS]" << std::endl;
    // glfwSetWindowTitle(pWindow, ss.str().c_str()); // Replace pWindow with your GLFWwindow pointer

    // Reset the counter and time
    frameCount = 0;
    previousTime = currentTime;
}
}

void render()
{
    ClearBackground(BLACK);

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

    Draw3DGPU(mesh, player1.camera, w2sShader, {255, 0, 0, 255}, 3.f);

    EndShaderMode();

    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();

    DrawFPS(10, 10);
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
        // PollInputEvents();
        // processInput();
        #ifdef _WIN32
            pumpMessages();
        #endif
        update();
        BeginDrawing();
        render();
        EndDrawing();
        SwapScreenBuffer();
    }

    RawMouseShutdown();
    shutdown();
    return 0;
}
