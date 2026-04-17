#include "../util.h"
#include "raylib.h"
#include "../physics/physics.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

void checkCar(player& player, float deltaTime, gameData& gData, meshedObject& carEntity) {
    // float dist = player.pEntity.location.dist(carEntity.pEntity.location);
    float dist = player.pEntity.location.dist(carEntity.pEntity.location);
    if (dist < 6.0f) { // Adjust the threshold as needed
        player.pState.canCar = true;
    } else {
        player.pState.canCar = false;
    }
}

void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar) {
    // std::cout << "current plane: " << player.pEntity.groundPlane << std::endl;
    checkCar(player, deltaTime, gData, testcar);
}
