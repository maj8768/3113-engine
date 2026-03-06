#include "util.h"
#include "gui.h"
#include <iostream>

void movePlayer(player& player1, bool swappedNormals) {
    // std::cout << "moving player" << std::endl;
    // if (player1.model->m[3][1] <= player1.bounding.m[3][1] - (swappedNormals ? (player1.model->m[0][1] - player1.model->m[3][1] + 0.25)  : 0.25)) {
        if (IsKeyDown(player1.controls.x)) {
            // std::cout << "moving forward" << std::endl;
            // std::cout << "player cam pos: " << player1.camera.camPos.x << ", " << player1.camera.camPos.y << ", " << player1.camera.camPos.z << std::endl;
            player1.camera.camPos.x += .25;
            // player1.camera.camTarget.x += .25;
            // std::cout << "player cam pos: " << player1.camera.camPos.x << ", " << player1.camera.camPos.y << ", " << player1.camera.camPos.z << std::endl;
        }
    // }
    // if (player1.model->m[0][2] <= player1.bounding.m[0][2] - 0.25) {
        if (IsKeyDown(player1.controls.y)) {
            player1.camera.camPos.z -= .25;
            // player1.camera.camTarget.z -= .25;
        }
    // }
    // if (player1.model->m[0][1] >= player1.bounding.m[0][1] - (swappedNormals ? (player1.model->m[3][1] - player1.model->m[0][1] -0.25)  : -0.25)) {
        if (IsKeyDown(player1.controls.z)) {
            player1.camera.camPos.x -= .25;
            // player1.camera.camTarget.x -= .25;
        }
    // }
    // if (player1.model->m[2][2] >= player1.bounding.m[2][2] + 0.25) {
        if (IsKeyDown(player1.controls.t)) {
            player1.camera.camPos.z += .25;
            // player1.camera.camTarget.z += .25;
        }
    // }
}

void moveLook(player& player1, float xoffset, float yoffset) {

    // std::cout << "moving look" << std::endl;
    // std::cout << "xoffset: " << xoffset << ", yoffset: " << yoffset << std::endl;
    // std::cout << "player cam target before: " << player1.camera.camTarget.x << ", " << player1.camera.camTarget.y << ", " << player1.camera.camTarget.z << std::endl;
    player1.camera.camTarget.x += xoffset * 0.25f;
    player1.camera.camTarget.y += yoffset * 0.25f;
    // std::cout << "player cam target after: " << player1.camera.camTarget.x << ", " << player1.camera.camTarget.y << ", " << player1.camera.camTarget.z << std::endl;
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
