#include "util.h"
#include "gui.h"
#include <iostream>

void movePlayer(player& player1, bool swappedNormals) {
    if (player1.model->m[3][1] <= player1.bounding.m[3][1] - (swappedNormals ? (player1.model->m[0][1] - player1.model->m[3][1] + 0.25)  : 0.25)) {
        if (IsKeyDown(player1.controls.x)) {
            player1.model->m[0][1] += .25;
            player1.model->m[1][1] += .25;
            player1.model->m[2][1] += .25;
            player1.model->m[3][1] += .25;
        }
    }
    if (player1.model->m[0][2] <= player1.bounding.m[0][2] - 0.25) {
        if (IsKeyDown(player1.controls.y)) {
            player1.model->m[0][2] += .25;
            player1.model->m[1][2] += .25;
            player1.model->m[2][2] += .25;
            player1.model->m[3][2] += .25;
        }
    }
    if (player1.model->m[0][1] >= player1.bounding.m[0][1] - (swappedNormals ? (player1.model->m[3][1] - player1.model->m[0][1] -0.25)  : -0.25)) {
        if (IsKeyDown(player1.controls.z)) {
            player1.model->m[0][1] -= .25;
            player1.model->m[1][1] -= .25;
            player1.model->m[2][1] -= .25;
            player1.model->m[3][1] -= .25;
        }
    }
    if (player1.model->m[2][2] >= player1.bounding.m[2][2] + 0.25) {
        if (IsKeyDown(player1.controls.t)) {
            player1.model->m[0][2] -= .25;
            player1.model->m[1][2] -= .25;
            player1.model->m[2][2] -= .25;
            player1.model->m[3][2] -= .25;
        }
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