#include "util.h"
#include "gui.h"
#include <cmath>
#include <iostream>

void movePlayer(player& player1, bool swappedNormals) {
    float xR = cos(player1.camera.camTarget.x);
    float zR = sin(player1.camera.camTarget.x);
    
    if (IsKeyDown(player1.controls.x)) {
        player1.camera.camPos.x += .25 * xR;
        player1.camera.camPos.z += .25 * zR;
    }
    if (IsKeyDown(player1.controls.y)) {
        player1.camera.camPos.z -= .25 * xR;
        player1.camera.camPos.x += .25 * zR;
    }
    if (IsKeyDown(player1.controls.z)) {
        player1.camera.camPos.x -= .25 * xR;
        player1.camera.camPos.z -= .25 * zR;
    }
    if (IsKeyDown(player1.controls.t)) {
        player1.camera.camPos.z += .25 * xR;
        player1.camera.camPos.x -= .25 * zR;
    }
}

void moveLook(player& player1, float xDelta, float yDelta) {
    
    player1.camera.camTarget.x += xDelta * 0.01;
    
    if (player1.camera.camTarget.x > M_PI) {
        player1.camera.camTarget.x = -M_PI + eps;
    }
    else if (player1.camera.camTarget.x < -M_PI) {
        player1.camera.camTarget.x = M_PI - eps;
    }
    if (player1.camera.camTarget.y < M_PI/2.f && player1.camera.camTarget.y > -M_PI/2.f) {
        player1.camera.camTarget.y -= yDelta * 0.01;
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
