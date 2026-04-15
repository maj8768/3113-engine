#include "../util.h"
#include "../draw/gui.h"
#include <cmath>
#include <iostream>
#include "raylib.h"
#include "../physics/physics.h"
#include "../system/keyboard/keyboard.h"

static bool canEnter = true;
static bool canR = true;
static bool canThrust = false;
static bool canDebug = true;

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
//    std::cout << "a" << std::endl;
    if(getAsyncKeyStateWrapper('R')) {
    //    std::cout << "f" << std::endl;
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

void getStartingInput(gameData& gData) {
//    std::cout << "a" << std::endl;
    if(getAsyncKeyStateWrapper(KEY_ENTER)) {
    //    std::cout << "f" << std::endl;
        if (gData.currentLevel == gameData::GAMESTART && canEnter) {
            gData.currentLevel = gameData::GAMEINFOMERCIAL;
            canEnter = false;
        }
        else if (gData.currentLevel == gameData::GAMEINFOMERCIAL && canEnter) {
            gData.currentLevel = gameData::LEVEL1;
        }
    }
    else canEnter = true;
}

void moveCar() {

}

void changeCarState(player& player) {
    if (player.pState.inCar == false) {
        std::cout << "player has entered the car" << std::endl;
    }
    else {
        std::cout << "player has exited the car" << std::endl;
    }
}

void movePlayer(gameData& gData, player& player, bool swappedNormals, float deltaTime, float maxSpeed, Sound js) {
    // always run ground reset regardless of canMove
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
        float zS =  cos(player.camera.camTarget.x);

        float moveX = 0.0f;
        float moveZ = 0.0f;


        if (getAsyncKeyStateWrapper(player.controls.x)) {
            moveX += xR;
            moveZ += zR;
        }
        if (getAsyncKeyStateWrapper(player.controls.y)) {
            moveX -= xS;
            moveZ -= zS;
        }
        if (getAsyncKeyStateWrapper(player.controls.z)) {
            moveX -= xR;
            moveZ -= zR;
        }
        if (getAsyncKeyStateWrapper(player.controls.t)) {
            moveX += xS;
            moveZ += zS;
        }
        if (getAsyncKeyStateWrapper(KEY_SPACE)) {
            if (nearGround && !player.pEntity.jumping) {
                player.pEntity.magnitude.y = 10.f;
                player.pEntity.jumping = true;
                PlaySound(js);
            }
        }
        if (getAsyncKeyStateWrapper(KEY_P)) {
            if (canDebug) {
                debugMode = !debugMode;
                canDebug = false;
                std::cout << "debug mode: " << (debugMode ? "on" : "off") << std::endl;
            }
        } else {
            canDebug = true;
        }

        float len = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (len > 0.0f/* || player.pEntity.jumping == false*/) {
            // std::cout << "he" << std::endl;
            moveX /= len;
            moveZ /= len;
            player.pEntity.magnitude.x += 50.f * moveX * deltaTime;
            player.pEntity.magnitude.z += 50.f * moveZ * deltaTime;
            
            // std::cout << "velocity: " << player.pEntity.velocity << std::endl;
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
        
    //    fmin(fmax(player.magnitude.x, -0.5), 0.5);
    //    fmin(fmax(player.magnitude.y, -0.5), 0.5);
    //    fmin(fmax(player.magnitude.z, -0.5), 0.5);
    }
        // snapping camera to players location
        player.camera.camPos.z = player.pEntity.location.z;
        player.camera.camPos.x = player.pEntity.location.x;
        player.camera.camPos.y = player.pEntity.location.y+5; // head level (apparently 5 is to high)
}

void moveLook(player& player1, float deltaTime, vector2 md) {
    
    // lol no deltaTime because mdelta is abs value

    // x look
    player1.camera.camTarget.x += md.x * .0025;
    if (player1.camera.camTarget.x > M_PI) {
        player1.camera.camTarget.x = -M_PI + eps;
    }
    else if (player1.camera.camTarget.x < -M_PI) {
        player1.camera.camTarget.x = M_PI - eps;
    }

    // y look
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
    // sadness = id;
    // std::cout << "player: " << player << " has died in battle" << std::endl;
}

void standardCollide(int id) {
    std::cout << "this is a standard collision" << std::endl;
}


