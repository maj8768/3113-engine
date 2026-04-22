#include "../util.h"
#include "raylib.h"
#include "../physics/physics.h"
#include "../system/keyboard/keyboard.h"
#include "../draw/gui.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

static bool locked = false;
static bool interacting = true;
static bool interactingG = true;
static vector3 PumpStorage = {0.f, 0.f, 0.f};

void holdItem(player& player, meshedObject& item) {
    item.pEntity.location = player.camera.camPos;
    applyCamRot(item.pEntity.location, vector3{player.camera.camTarget.x, 0.f, 0.f}, 0.f, 0.f, 1.5f);
    // item.pEntity.rot.y = -player.camera.camTarget.x;
}

void buy(player& player, int buy_guy) {
    std::cout << "Buy menu open" << std::endl;
    guiBuyMenu(SCREEN_WIDTH, SCREEN_HEIGHT, player, buy_guy);
}

void checkBuyBox(player& player, float deltaTime, gameData& gData, meshedObject& buyBox) {
    std::cout << "buybox loc: " << buyBox.pEntity.location.x << ", " << buyBox.pEntity.location.y << ", " << buyBox.pEntity.location.z << std::endl;
     if (canInteract(player, buyBox.pEntity.location, 6.0f, GetScreenWidth(), GetScreenHeight(), 300.f, {0.f, 2.75f, 0.f})) {
        player.pState.canBuy = true;
        std::cout << "can buy" << std::endl;
    }
    else {
        std::cout << "cant buy" << std::endl;
        player.pState.canBuy = false;
    }
}

void checkDrank(player& player, float deltaTime, gameData& gData, meshedObject& drankEntity) {
    // std::cout << "drank loc: " << drankEntity.pEntity.location.x << ", " << drankEntity.pEntity.location.y << ", " << drankEntity.pEntity.location.z << std::endl;
     if (canInteract(player, drankEntity.pEntity.location, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f, {0.f, -1.f, 0.f})) {
        player.pState.canPickDrank = true;
    }
    else {
        player.pState.canPickDrank = false;
    }
}

void checkCar(player& player, float deltaTime, gameData& gData, meshedObject& carEntity) {
    if (canInteract(player, carEntity.pEntity.location, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f, {0.f, 1.5f, 0.f})) {
        player.pState.canCar = true;
    }
    else {
        player.pState.canCar = false;
    }
}

void checkGasPump(player& player, float deltaTime, gameData& gData, meshedObject& gasPump) {
    // std::cout << "gaspump loc " << gasPump.pEntity.location.x << ", " << gasPump.pEntity.location.y << ", " << gasPump.pEntity.location.z << std::endl;
    if (canInteract(player, gasPump.pEntity.location, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f, {0.f, 2.f, 0.f})) {
        player.pState.canGasPump = true;
    } 
    else {
        player.pState.canGasPump = false;
    }
}

// player, then all things you want to check interact with
void interact(player& player, float deltaTime, gameData& gData, meshedObject& car, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank) {
    // std::cout << "Car: " << player.pState.canCar << " GasPump: " << player.pState.canGasPump << "Has Gas Pump: " << player.pState.hasGasPump << std::endl;
    if (getAsyncKeyStateWrapper(KEY_G)) {
        if (player.pState.hasDrank && interactingG) {
            drank.pEntity.location = player.camera.camPos;
            player.pState.hasDrank = false;
            player.pState.canPickDrank = false;
            player.pState.canBuy = false;
            interactingG = false;
        }
    }
    else {
        interactingG = true;
    }
    if (getAsyncKeyStateWrapper(KEY_F)) {
        if (player.pState.canGasPump && interacting ) {
            if (!player.pState.hasGasPump && !player.pState.pumpingUp) {
                player.pState.hasGasPump = true;
                PumpStorage = gasPumpNozzle.pEntity.location;
                gasPumpNozzle.pEntity.location = {0.f, -20.f, 0.f};
                interacting = false;
            }
            else if (player.pState.hasGasPump) {
                player.pState.hasGasPump = false;
                player.pState.pumpingUp = false;
                gasPumpNozzle.pEntity.location = PumpStorage;
                gasPumpNozzleOff.pEntity.location = {0.f, -20.f, 0.f};
                interacting = false;
            }
        }
        else if (player.pState.hasGasPump && interacting) {
            if (player.pState.canCar) {
                resetMeshedLocation(gasPumpNozzleOff);
                locked = true;
                player.pState.hasGasPump = false;
                player.pState.pumpingUp = true;
                // resetMeshedLocation(gasPumpNozzleOff);
                // applyCamRot(gasPumpNozzleOff.pEntity.location, vector3{0.f, 0.f, 0.f}, 0.f, 0.f, 0.f);
                gasPumpNozzleOff.pEntity.location = car.pEntity.location + vector3{0.f, 5.5f, 0.f};
                interacting = false;
            }
        } 
        else if (interacting && player.pState.pumpingUp) {
            if (player.pState.canCar) {
                resetMeshedLocation(gasPumpNozzleOff);
                // gasPumpNozzle.pEntity.location = PumpStorage;
                player.pState.hasGasPump = true;
                player.pState.pumpingUp = false;
                interacting = false;
                locked = false;
            }
        }
        else if (interacting && player.pState.canPickDrank) {
            if (player.pState.hasDrank == false) {
                player.pState.hasDrank = true;
                interacting = false;
            }
        }
        else if (interacting && player.pState.hasDrank) {
            if (player.pState.canBuy || player.pState.buying) {
                player.pState.buying = !player.pState.buying;
                interacting = false;
            }
            else if (player.pState.canDrinkDrank) {

                interacting = false;
            }
        }
        else if (interacting && !player.pState.hasGasPump) {
            if (!player.pState.inCar && player.pState.canCar) {
                player.pState.inCar = !player.pState.inCar;
                interacting = false;
            }
            else if (player.pState.inCar) {
                player.pState.inCar = !player.pState.inCar;
                interacting = false;
                vector3 playerLoc = player.pEntity.location;
                applyRot(player.pEntity.location, car.pEntity.rot, -4.f, 0.001, -0.45f);

            }
        }
    }
    else {
        interacting = true;
    }
}

// bruh idc
static constexpr int kNozzleAnchorTriA = 44;
static constexpr int kNozzleAnchorTriB = 45;

void deformGasPump(meshedObject& gasPump, meshedObject& gasPumpNozzleOff) {
    if (kNozzleAnchorTriA < gasPumpNozzleOff.mesh.count && kNozzleAnchorTriB < gasPumpNozzleOff.mesh.count) {
        vector3 anchorVerts[6];
        int anchorCount = 0;
        for (int j = 0; j < 3; j++) {
            anchorVerts[anchorCount++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriA].v[j];
            anchorVerts[anchorCount++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriB].v[j];
        }
        const float kEps = 1e-4f;
        for (int i = 0; i < gasPumpNozzleOff.mesh.count; i++) {
            for (int j = 0; j < 3; j++) {
                const vector3& lv = gasPumpNozzleOff.mesh.trisO[i].v[j];
                for (int k = 0; k < anchorCount; k++) {
                    if (fabsf(lv.x - anchorVerts[k].x) < kEps &&
                        fabsf(lv.y - anchorVerts[k].y) < kEps &&
                        fabsf(lv.z - anchorVerts[k].z) < kEps) {
                        gasPumpNozzleOff.mesh.tris[i].v[j] = lv + gasPump.pEntity.location;
                        break;
                    }
                }
            }
        }
    }
}

void gasPumpLogic(player& player, float deltaTime, gameData& gData, meshedObject& gasPump, meshedObject& car, meshedObject& gasPumpNozzleOff) {
    if (player.pState.hasGasPump) {
        gasPumpNozzleOff.pEntity.location = player.pEntity.location + vector3{0.f, 0.f, 0.f};
        applyCamRot(gasPumpNozzleOff.pEntity.location, vector3{player.camera.camTarget.x, 0.f, 0.f}, -3.f, -1.f, 2.5f);
        gasPumpNozzleOff.pEntity.rot.y = -player.camera.camTarget.x;
        deformGasPump(gasPump, gasPumpNozzleOff);
    }
    if (player.pState.pumpingUp) {
        gasPumpNozzleOff.pEntity.location = car.pEntity.location;
        applyRot(gasPumpNozzleOff.pEntity.location, car.pEntity.rot, 8.0f, -1.9f, 4.8f);
        gasPumpNozzleOff.pEntity.rot.y = car.pEntity.rot.y - M_PI/2.f;
        deformGasPump(gasPump, gasPumpNozzleOff);
    }
}

void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, meshedObject& buyBox) {
    // std::cout << "current plane: " << player.pEntity.groundPlane << std::endl;
    gasPumpLogic(player, deltaTime, gData, gasPump, testcar, gasPumpNozzleOff);
    checkCar(player, deltaTime, gData, testcar);
    checkGasPump(player, deltaTime, gData, gasPump);
    checkDrank(player, deltaTime, gData, drank);
    checkBuyBox(player, deltaTime, gData, buyBox);
    if (player.pState.hasDrank) holdItem(player, drank);
    if (player.pState.buying) buy(player,1);
    interact(player, deltaTime, gData, testcar, gasPump, gasPumpNozzle, gasPumpNozzleOff, drank);
    // std::cout << "drank: " << player.pState.canPickDrank << ", hasDrank: " << player.pState.hasDrank << std::endl;
    std::cout << "\033[2J\033[H"; 
}
