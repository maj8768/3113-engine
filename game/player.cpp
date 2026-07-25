#include "../util.h"
#include "../draw/gui.h"
#include <cmath>
#include <iostream>
#include "raylib.h"
#include "../physics/physics.h"
#include "../system/keyboard/keyboard.h"
#include "game.h"

static bool canEnter = true;
static bool canR = true;
static bool canThrust = false;
static bool canDebug = true;
static bool canEnterCar = true;
static bool canBuyMenu = true;
static bool canArrow = true;

void haltPlayerLerp(player& player, bool swappedNormals, float deltaTime) {
    player.pEntity.magnitude.x = player.pEntity.magnitude.x * (1 - deltaTime * 10.f);
    player.pEntity.magnitude.z = player.pEntity.magnitude.z * (1 - deltaTime * 10.f);

    if (abs(player.pEntity.magnitude.x) < 0.02f) {
        player.pEntity.magnitude.x = 0;
    }
    if (abs(player.pEntity.magnitude.z) < 0.02f) {
        player.pEntity.magnitude.z = 0;
    }
}

void getRestartInput(gameData& gData) {

    if(getAsyncKeyStateWrapper('R')) {

        if ((gData.currentLevel == gameData::GAMEEND || gData.currentLevel == gameData::GAMEWIN) && canR) {
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
                    .currentLevel = gameData::LEVEL1,
                    .lives = 3
            };

            canR = false;
        }
    }
    else canR = true;
}

void getEscapeMenuInput(gameData& gData) {
    static bool canR = false;
    if (!getAsyncKeyStateWrapper('R')) {canR = true; return;}
    if (!canR) return;
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
    canR = false;
}

void getStartingInput(gameData& gData) {
    if(getAsyncKeyStateWrapper(KEY_ENTER)) {
        if (gData.currentLevel == gameData::GAMESTART && canEnter) {
            gData.currentLevel = gameData::GAMEINFOMERCIAL;
            canEnter = false;
        }
    }
    else canEnter = true;
}

void moveCar() {

}

void movePlayer(gameData& gData, player& player, bool swappedNormals, float deltaTime, float maxSpeed, Sound js) {
    float drunkScale = player.pState.drunkenness * 2.f;
    static float cachedRands[8] = {};
    static double lastRandTime = 0.0;
    double now = GetTime();
    if (now - lastRandTime >= 0.2) {
        for (int i = 0; i < 8; i++)
        cachedRands[i] = ((float)rand() / (float)RAND_MAX - 0.5f) * drunkScale;
        lastRandTime = now;
    }
    int randIdx = 0;
    auto dRand = [&]() {return cachedRands[randIdx++ % 8];};
    if (getAsyncKeyStateWrapper(KEY_P)) {
        if (canDebug) {
            debugMode = !debugMode;
            canDebug = false;

        }
    } else {
        canDebug = true;
    }
    static bool canNoClip = true;
    if (getAsyncKeyStateWrapper(KEY_N)) {
        if (canNoClip) {
            player.pState.noClip = !player.pState.noClip;
            player.pEntity.magnitude = {0.f, 0.f, 0.f};
            canNoClip = false;

        }
    } else {
        canNoClip = true;
    }
    if (player.pState.noClip) {
        std::cout << "player loc: " << player.pEntity.location.x << ", " << player.pEntity.location.y << ", " << player.pEntity.location.z << std::endl;
        const float ncSpeed = 15.f;
        if (getAsyncKeyStateWrapper(KEY_SPACE))
        player.pEntity.location.y += ncSpeed * deltaTime;
        if (getAsyncKeyStateWrapper(KEY_LEFT_CONTROL))
        player.pEntity.location.y -= ncSpeed * deltaTime;
        player.pEntity.magnitude = {0.f, 0.f, 0.f};
        player.pEntity.jumping = false;

        float xR = cos(player.camera.camTarget.x);
        float zR = sin(player.camera.camTarget.x);
        float xS = -sin(player.camera.camTarget.x);
        float zS = cos(player.camera.camTarget.x);
        float moveX = 0.f, moveZ = 0.f;
        if (getAsyncKeyStateWrapper(player.controls.x)) {moveX += xR; moveZ += zR;}
        if (getAsyncKeyStateWrapper(player.controls.y)) {moveX -= xS; moveZ -= zS;}
        if (getAsyncKeyStateWrapper(player.controls.z)) {moveX -= xR; moveZ -= zR;}
        if (getAsyncKeyStateWrapper(player.controls.t)) {moveX += xS; moveZ += zS;}
        float len = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (len > 0.f) {
            float spd = ncSpeed * (getAsyncKeyStateWrapper(KEY_LEFT_SHIFT) ? 2.f : 1.f);
            player.pEntity.location.x += (moveX / len) * spd * deltaTime;
            player.pEntity.location.z += (moveZ / len) * spd * deltaTime;
        }
        player.camera.camPos.x = player.pEntity.location.x;
        player.camera.camPos.y = player.pEntity.location.y + 5.f;
        player.camera.camPos.z = player.pEntity.location.z;
        return;
    }
    else {
        if (player.pEntity.collidingY && player.pEntity.magnitude.y <= 0.f) {
            player.pEntity.jumping = false;
            player.pEntity.magnitude.y = 0.f;
        }

        if (player.canMove == false) {
            haltPlayerLerp(player, swappedNormals, deltaTime);
        }
        else {
            static float lastGroundY = 0.f;
            if (player.pEntity.collidingY) lastGroundY = player.pEntity.location.y;
            bool nearGround = player.pEntity.collidingY || fabsf(player.pEntity.location.y - lastGroundY) < 0.5f;

            bool holdKeys = false;

            float xR = cos(player.camera.camTarget.x);
            float zR = sin(player.camera.camTarget.x);

            float xS = -sin(player.camera.camTarget.x);
            float zS = cos(player.camera.camTarget.x);

            float moveX = 0.0f;
            float moveZ = 0.0f;

            if (getAsyncKeyStateWrapper(player.controls.x)) {
                moveX += xR + dRand();
                moveZ += zR + dRand();
            }
            if (getAsyncKeyStateWrapper(player.controls.y)) {
                moveX -= xS + dRand();
                moveZ -= zS + dRand();
            }
            if (getAsyncKeyStateWrapper(player.controls.z)) {
                moveX -= xR + dRand();
                moveZ -= zR + dRand();
            }
            if (getAsyncKeyStateWrapper(player.controls.t)) {
                moveX += xS + dRand();
                moveZ += zS + dRand();
            }
            if (getAsyncKeyStateWrapper(KEY_LEFT_SHIFT)) {
                moveX *= 2.f;
                moveZ *= 2.f;
            }
            if (getAsyncKeyStateWrapper(KEY_SPACE)) {
                if (nearGround && !player.pEntity.jumping) {
                    player.pEntity.magnitude.y = 10.f;
                    player.pEntity.jumping = true;
                    PlaySound(js);
                }
            }

            float len = std::sqrt(moveX * moveX + moveZ * moveZ);
            if (len > 0.0f) {

                moveX /= len;
                moveZ /= len;
                player.pEntity.magnitude.x += 50.f * moveX * deltaTime;
                player.pEntity.magnitude.z += 50.f * moveZ * deltaTime;

                holdKeys = true;
            }

            if (holdKeys == false) {
                haltPlayerLerp(player, swappedNormals, deltaTime);
            }
            float speed = std::sqrt(player.pEntity.magnitude.x * player.pEntity.magnitude.x +
                player.pEntity.magnitude.z * player.pEntity.magnitude.z);

            player.pEntity.velocity = speed;

            if (speed > maxSpeed) {
                float scale = maxSpeed / speed;
                player.pEntity.magnitude.x *= scale;
                player.pEntity.magnitude.z *= scale;
            }

        }

        player.camera.camPos.z = player.pEntity.location.z;
        player.camera.camPos.x = player.pEntity.location.x;
        player.camera.camPos.y = player.pEntity.location.y+7;
    }
}

void moveLook(player& player1, float deltaTime, vector2 md) {

    player1.camera.camTarget.x += md.x * .0025;
    if (player1.camera.camTarget.x > M_PI) {
        player1.camera.camTarget.x = -M_PI + eps;
    }
    else if (player1.camera.camTarget.x < -M_PI) {
        player1.camera.camTarget.x = M_PI - eps;
    }

    if (player1.camera.camTarget.y < M_PI/2.f && player1.camera.camTarget.y > -M_PI/2.f) {
        player1.camera.camTarget.y -= md.y * .0025;
    }
    if (player1.camera.camTarget.y > M_PI/2.f) {
        player1.camera.camTarget.y = M_PI/2.f - eps;
    }
    else if (player1.camera.camTarget.y < -M_PI/2.f) {
        player1.camera.camTarget.y = -M_PI/2.f + eps;
    }
}

void paddleHit(int id) {
    std::cout << "a player has hit the paddle" << std::endl;
}

void killPlayer(int id) {
    std::cout << "a player has died in battle" << std::endl;

}

void standardCollide(int id) {
    std::cout << "this is a standard collision" << std::endl;
}

