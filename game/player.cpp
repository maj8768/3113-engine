#include "../util.h"
#include "../draw/gui.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "raylib.h"
#include "../system/keyboard/keyboard.h"

static bool canEnter = true;

void haltPlayerLerp(player& player, bool swappedNormals, float deltaTime) {
    player.pEntity.magnitude.x = player.pEntity.magnitude.x * (1 - deltaTime * 10.f);
    player.pEntity.magnitude.y = player.pEntity.magnitude.y * (1 - deltaTime * 10.f);
    player.pEntity.magnitude.z = player.pEntity.magnitude.z * (1 - deltaTime * 10.f);
    
    if (abs(player.pEntity.magnitude.x) < 0.02f) {
        player.pEntity.magnitude.x = 0;
    }
    if (abs(player.pEntity.magnitude.y) < 0.02f) {
        player.pEntity.magnitude.y = 0;
    }
    if (abs(player.pEntity.magnitude.z) < 0.02f) {
        player.pEntity.magnitude.z = 0;
    }
}

void getStartingInput(gameData& gData) {
//    std::cout << "a" << std::endl;
    if(getAsyncKeyStateWrapper(KEY_ENTER)) {
//        std::cout << "f" << std::endl;
        if (canEnter) {
            if (gData.gameStarted == false) {
                gData.gameStarted = true;
                canEnter = false;
            }
            else if (gData.gameStarted == true && gData.infommercial == false) {
                gData.infommercial = true;
                canEnter = false;
            }
        }
    }
    else {
        canEnter = true;
    }
}

void movePlayer(gameData& gData, player& player, bool swappedNormals, float deltaTime, float maxSpeed) {

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
        if (gData.fuel > 0 && gData.thrustY > -38.0) {
            gData.oldThrustY = gData.thrustY;
            gData.thrustY -= 0.1;
            gData.propThrustY += 0.1;
            gData.oldFuel = gData.fuel;
            gData.fuel -= 0.01;
        }
        else if (gData.fuel > 0) {
            gData.oldThrustY = gData.thrustY;
            gData.oldFuel = gData.fuel;
            gData.fuel -= 0.01;
        }
        else {
            gData.oldFuel = gData.fuel;
            if (gData.thrustY < gData.propThrustY*2) {
                gData.oldThrustY = gData.thrustY;
                gData.thrustY += 0.1;
                gData.propThrustY -= 0.1;
            }
            else {
                gData.thrustY = 0;
                gData.oldThrustY = 0;
                gData.propThrustY = 0;
            }
        }
    }
    else {
        gData.oldFuel = gData.fuel;
        if (gData.thrustY < gData.propThrustY*2) {
            gData.oldThrustY = gData.thrustY;
            gData.thrustY += 0.1;
            gData.propThrustY -= 0.1;
        }
        else {
            gData.thrustY = 0;
            gData.oldThrustY = 0;
            gData.propThrustY = 0;
        }
    }

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0f) {
        // std::cout << "he" << std::endl;
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

    if (speed > maxSpeed) {
        float scale = maxSpeed / speed;
        player.pEntity.magnitude.x *= scale;
        player.pEntity.magnitude.z *= scale;
    }
    
//    fmin(fmax(player.magnitude.x, -0.5), 0.5);
//    fmin(fmax(player.magnitude.y, -0.5), 0.5);
//    fmin(fmax(player.magnitude.z, -0.5), 0.5);
    
    // snapping camera to players location
    player.camera.camPos.z = player.pEntity.location.z;
    player.camera.camPos.x = player.pEntity.location.x;
    player.camera.camPos.y = player.pEntity.location.y+5; // head level
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
