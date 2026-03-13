#include "../util.h"
#include "../draw/gui.h"
#include <cmath>
#include <iostream>
#include "raylib.h"
#include "../system/keyboard/keyboard.h"

void movePlayer(player& player1, bool swappedNormals, float deltaTime) {

    float xR = cos(player1.camera.camTarget.x);
    float zR = sin(player1.camera.camTarget.x);
    
    if (getAsyncKeyStateWrapper(player1.controls.x)) {
        player1.camera.camPos.x += 10 * xR * deltaTime;
        player1.camera.camPos.z += 10 * zR * deltaTime;
    }
    if (getAsyncKeyStateWrapper(player1.controls.y)) {
        player1.camera.camPos.z -= 10 * xR * deltaTime;
        player1.camera.camPos.x += 10 * zR * deltaTime;
    }
    if (getAsyncKeyStateWrapper(player1.controls.z)) {
        player1.camera.camPos.x -= 10 * xR * deltaTime;
        player1.camera.camPos.z -= 10 * zR * deltaTime;
    }
    if (getAsyncKeyStateWrapper(player1.controls.t)) {
        player1.camera.camPos.z += 10 * xR * deltaTime;
        player1.camera.camPos.x -= 10 * zR * deltaTime;
    }
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
