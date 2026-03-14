#include "../util.h"
#include "../draw/gui.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "raylib.h"
#include "../system/keyboard/keyboard.h"

void haltPlayerLerp(player& player, bool swappedNormals, float deltaTime) {
    player.magnitude.x = player.magnitude.x * (1 - deltaTime * 10.f);
    player.magnitude.y = player.magnitude.y * (1 - deltaTime * 10.f);
    player.magnitude.z = player.magnitude.z * (1 - deltaTime * 10.f);
    
    if (abs(player.magnitude.x) < 0.02f) {
        player.magnitude.x = 0;
    }
    if (abs(player.magnitude.y) < 0.02f) {
        player.magnitude.y = 0;
    }
    if (abs(player.magnitude.z) < 0.02f) {
        player.magnitude.z = 0;
    }
}

void movePlayer(player& player, bool swappedNormals, float deltaTime, float maxSpeed) {

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

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0f) {
        // std::cout << "he" << std::endl;
        moveX /= len;
        moveZ /= len;
        player.magnitude.x += 50.f * moveX * deltaTime;
        player.magnitude.z += 50.f * moveZ * deltaTime;
        holdKeys = true;
    }

    if (holdKeys == false) {
        haltPlayerLerp(player, swappedNormals, deltaTime);
    }
    float speed = std::sqrt(player.magnitude.x * player.magnitude.x +
                            player.magnitude.z * player.magnitude.z);

    if (speed > maxSpeed) {
        float scale = maxSpeed / speed;
        player.magnitude.x *= scale;
        player.magnitude.z *= scale;
    }
    
//    fmin(fmax(player.magnitude.x, -0.5), 0.5);
//    fmin(fmax(player.magnitude.y, -0.5), 0.5);
//    fmin(fmax(player.magnitude.z, -0.5), 0.5);
    
    // snapping camera to players location
    player.camera.camPos.z = player.location.z;
    player.camera.camPos.x = player.location.x;
    player.camera.camPos.y = player.location.y+5; // head level
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
