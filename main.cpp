#include "raylib.h"
#include "util.h"
#include "physics.h"
#include "player.h"

#include "draw.h"
#include "camera.h"
#include "gui.h"


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

static worldsStore worldObj;
static world playerWorld;
static world worldInstance;

static player player1;
static player player2;

static pyramidMtx pyramid;
static planeMtx plane;
static planeMtx plane2;
static sphere_ ball1;
static sphere_ ball2;
static sphere_ ball3;


static bool ballsPicked = false;
static bool gameStarted = false;
static bool gameEnded = false;
static int balls = 1;
static int playerCount = 2;

static camera cam = {
    .camPos = { x, 0, z },
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

    worldInstance.planeCount = 6;
    worldInstance.planes = new planeMtx*[6];
    playerWorld.planeCount = 2;
    playerWorld.planes = new planeMtx*[2];

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

    // Box centered at (0,0,0)
    // X length = 20  => x in [-10, +10]
    // Z width  = 10  => z in [-5,  +5]
    // Y height = 10  => y in [-5,  +5]

    const float hx = 10.0f; // half X
    const float hz = 5.0f;  // half Z
    const float hy = 5.0f;  // half Y

    // Reuse UVs for all faces
    float uvs[4][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    // +Y (top), outward normal +Y
    worldInstance.planes[0] = new planeMtx{
        {
            -hx, +hy, -hz,   // v0
            +hx, +hy, -hz,   // v1
            +hx, +hy, +hz,   // v2
            -hx, +hy, +hz    // v3
        },
        WHITE,
        texo,
        0,
        standardCollide
    };

    // -Y (bottom), outward normal -Y
    worldInstance.planes[1] = new planeMtx{
        {
            -hx, -hy, +hz,   // v0
            +hx, -hy, +hz,   // v1
            +hx, -hy, -hz,   // v2
            -hx, -hy, -hz    // v3
        },
        WHITE,
        texo,
        0,
        standardCollide
    };

    // +X (right), outward normal +X
    worldInstance.planes[2] = new planeMtx{
        {
            +hx, -hy, -hz,   // v0
            +hx, -hy, +hz,   // v1
            +hx, +hy, +hz,   // v2
            +hx, +hy, -hz    // v3
        },
        WHITE,
        texo,
        2,
        killPlayer
    };

    // -X (left), outward normal -X
    worldInstance.planes[3] = new planeMtx{
        {
            -hx, -hy, +hz,   // v0
            -hx, -hy, -hz,   // v1
            -hx, +hy, -hz,   // v2
            -hx, +hy, +hz    // v3
        },
        WHITE,
        texo,
        1,
        killPlayer
    };

    // +Z (front), outward normal +Z
    worldInstance.planes[4] = new planeMtx{
        {
            +hx, -hy, +hz,   // v0
            -hx, -hy, +hz,   // v1
            -hx, +hy, +hz,   // v2
            +hx, +hy, +hz    // v3
        },
        WHITE,
        texo,
        0,
        standardCollide
    };

    // -Z (back), outward normal -Z
    worldInstance.planes[5] = new planeMtx{
        {
            -hx, -hy, -hz,   // v0
            +hx, -hy, -hz,   // v1
            +hx, +hy, -hz,   // v2
            -hx, +hy, -hz    // v3
        },
        WHITE,
        texo,
        0,
        standardCollide
    };


    createSphere(ball1,
        16,
        0.25,
        {0,0,0},
        16
    );
    createSphere(ball2,
        16,
        0.25,
        {0,0,0},
        16
    );
    createSphere(ball3,
        16,
        0.25,
        {0,0,0},
        16
    );



    player1.model = new planeMtx{
            -hx, -hy/4.f, +hz/4.f,   // v0
            -hx, -hy/4.f, -hz/4.f,   // v1
            -hx, +hy/4.f, -hz/4.f,   // v2
            -hx, +hy/4.f, +hz/4.f    // v3
    };
    player1.model->id = 3;
    player1.model->action = paddleHit;
    player1.location = {-hx,hy,hz};
    player1.bounding = {
            -hx, -hy, +hz,   // v0
            -hx, -hy, -hz,   // v1
            -hx, +hy, -hz,   // v2
            -hx, +hy, +hz    // v3
    };
    player1.controls = {'W','A','S','D'};
    player1.hits = 0;

    player2.model = new planeMtx{
            +hx, +hy/4.f, +hz/4.f,   // v0
            +hx, +hy/4.f, -hz/4.f,   // v1
            +hx, -hy/4.f, -hz/4.f,   // v2
            +hx, -hy/4.f, +hz/4.f    // v3
    };
    player2.model->id = 3;
    player2.model->action = paddleHit;
    player2.location = {-hx,hy,hz};
    player2.bounding = {
            +hx, -hy, +hz,   // v0
            +hx, -hy, -hz,   // v1
            +hx, +hy, -hz,   // v2
            +hx, +hy, +hz    // v3
    };
    player2.controls = {KEY_UP,KEY_RIGHT,KEY_DOWN,KEY_LEFT}; // swapped because sides swapped
    player2.hits = 0;

    playerWorld.planes[0] = player1.model;
    playerWorld.planes[1] = player2.model;

    worldObj.worldInstance = worldInstance;
    worldObj.playerInstance = playerWorld;


    // applyAcceleration({0.f,-9.80665f,0.f},ball); // gravity lol
    srand(time(NULL));
    float ran1 = (float)(rand() % 100) / 100.f;
    float ran2 = (float)(rand() % 100) / 100.f;
    float ran3 = (float)(rand() % 100) / 100.f;
    float mran1 = 1;
    float mran2 = 1;
    float mran3 = 1;
    if (ran1 > 0.5) {
        mran1 = -1;
    }
    if (ran2 > 0.5) {
        mran2 = -1;
    }
    if (ran3 < 0.5) {
        mran3 = -1;
    }
    std::cout << ran1 << ", " << ran2 << ", " << ran3 << std::endl;
    applyForce({(15*ran1 < 5) ? 5 : 15*ran1,15*(1-ran1)*mran1,0.f},ball1);
    applyForce({(15*ran1 < 4) ? 4 : 10*ran2*mran2,10*(1-ran2)*mran2,0.f},ball2);
    applyForce({(15*ran1 < 3) ? 3 : 5*ran3*mran3,5*(1-ran3),0.f},ball3);

    SetTargetFPS(FPS);
}

void processInput() 
{
    if (WindowShouldClose()) gAppStatus = TERMINATED;
}

static bool pRot = true;


float i = 90;
int g = 1;
float floating = 1;
int target = 0;
    
void update() {
    
        auto ticks = static_cast<float>(GetTime());          // step 1
        float deltaTime = ticks - gPreviousTicks; // step 2
        gPreviousTicks = ticks;                   // step 3

        i += 2 * deltaTime;

        cam.camPos = { (float)(1 * cos(90 * M_PI / 180.f)), 0, (float)(15 * sin(90 * M_PI / 180.f)) }; // orbit x + z
        playerWorld.planes[0] = player1.model;
        playerWorld.planes[1] = player2.model;
        // int target = 0;
    if (gameStarted && !gameEnded) {
        // std::cout << playerWorld.planes[0]->m[0][1] << std::endl;
        if (balls == 1){ 
            processPhysics(deltaTime, GetFPS(), ball1, worldObj, gameEnded,target);
            // std::cout << gameEnded << std::endl;
        }
        else if (balls == 2) {
            processPhysics(deltaTime, GetFPS(), ball1, worldObj, gameEnded,target);
            processPhysics(deltaTime, GetFPS(), ball2, worldObj, gameEnded,target);
        }
        else if (balls == 3) {
            processPhysics(deltaTime, GetFPS(), ball1, worldObj, gameEnded,target);
            processPhysics(deltaTime, GetFPS(), ball2, worldObj, gameEnded,target);
            processPhysics(deltaTime, GetFPS(), ball3, worldObj, gameEnded,target);
        }
        // processPhysics(deltaTime, GetFPS(), ball, playerWorld, true);
        if (balls == 1) {
            cam.camTarget = {ball1.location.x/5.f, cam.camTarget.y, cam.camTarget.z};
        }

        if (i > 361 || i < 0) {
            g *= -1;
            i = 0;
        }

        if (playerCount == 2) {
            movePlayer(player2, true);
        }
        else {
            playerFollowBall(player2, ball1, true);
        }
        movePlayer(player1, false);
        // std::cout << target << std::endl;
    }
    else if (gameEnded) {
        // std::cout << "fin " << target << std::endl;
            // std::cout << "2sadness: " << sadness << std::endl;
            guiDrawEndPopup(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 100, LIGHTGRAY, target);
    }
    else {
        if (IsKeyPressed(KEY_ENTER) && ballsPicked) {
            gameStarted = true;
        }
        else if (IsKeyPressed(KEY_T) && ballsPicked && balls == 1) {
            playerCount = 1;
            gameStarted = true;
        }
        else if (IsKeyPressed(KEY_ENTER)) {
            ballsPicked = true;
        }
        if (IsKeyPressed(KEY_ONE)) {
            balls = 1;
        }
        else if (IsKeyPressed(KEY_TWO)) {
            balls = 2;
        }
        else if (IsKeyPressed(KEY_THREE)) {
            balls = 3;
        }
    }
}

void render()
{

    BeginDrawing();

    ClearBackground(RAYWHITE);
    if (ballsPicked) {
        if (!gameStarted) {
            guiDrawStartPopup(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 100, LIGHTGRAY);
        }
        for (int draww = 0; draww < worldObj.worldInstance.planeCount; draww++) {
                DrawPlaneFancy(*worldObj.worldInstance.planes[draww], cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK, true);
        }
        for (int drawp = 0; drawp < worldObj.playerInstance.planeCount; drawp++) {
                DrawPlaneFancy(*worldObj.playerInstance.planes[drawp], cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK, true);
        }

        // drawing ball(s)
        if (balls == 1) {
            DrawSphere(ball1,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
        }
        else if (balls == 2) {
            DrawSphere(ball1,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
            DrawSphere(ball2,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
        }
        else if (balls == 3) {
            DrawSphere(ball1,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
            DrawSphere(ball2,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
            DrawSphere(ball3,cam,SCREEN_WIDTH,SCREEN_HEIGHT);
        }

        // // drawing players
        // DrawPlaneFancy(*player1.model, cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK, true);
        // DrawPlaneFancy(*player2.model, cam, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK, true);
    }
    else {
        guiDrawStartMenu(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 100, LIGHTGRAY, balls);
    }

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
