#include "../util.h"
#include "raylib.h"
#include "../physics/physics.h"
#include "../system/keyboard/keyboard.h"
#include "../draw/gui.h"
#include "game.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

Sound sndShopIntro = {0};
Sound sndShopBye = {0};
Sound sndShopThank = {0};
Sound sndGasIntro = {0};
Sound sndGasPurchase = {0};

void playMenuSound(Sound s) {
    static Sound last = {0};
    if (last.frameCount > 0) StopSound(last);
    PlaySound(s);
    last = s;
}

static bool locked = false;
static bool interacting = true;
static bool interactingG = true;

void resetLevelLogicState() {
    locked = false;
    interacting = true;
    interactingG = true;
}
static vector3 PumpStorage = {0.f, 0.f, 0.f};

static vector3 drinkoffset = {-79.9136f, -3.f, 25.2414f};
static vector3 gasPumpOffset = {49.3031f, 5.81f, -24.243f};

void holdItem(player& player, meshedObject& item) {
    item.pEntity.location = player.camera.camPos + drinkoffset;
    applyCamRot(item.pEntity.location, vector3{player.camera.camTarget.x, 0.f, 0.f}, 0.f, 0.f, 1.5);

}

void buy(player& player, int buy_guy) {
    std::cout << "Buy menu open" << std::endl;
    guiBuyMenu(SCREEN_WIDTH, SCREEN_HEIGHT, player, buy_guy);
}

void checkBuyBox(player& player, float deltaTime, gameData& gData, meshedObject& buyBox) {

    if (canInteract(player, buyBox.collider, buyBox.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 300.f)) {
        player.pState.canBuy = true;

    }
    else {

        player.pState.canBuy = false;
    }
}

void checkDrank(player& player, float deltaTime, gameData& gData, meshedObject& drankEntity) {

    if (canInteract(player, drankEntity.collider, drankEntity.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f)) {
        player.pState.canPickDrank = true;
    }
    else {
        player.pState.canPickDrank = false;
    }
}

void checkCar(player& player, float deltaTime, gameData& gData, meshedObject& carEntity) {
    if (canInteract(player, carEntity.collider, carEntity.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f)) {
        player.pState.canCar = true;
    }
    else {
        player.pState.canCar = false;
    }
}

void checkGasPump(player& player, float deltaTime, gameData& gData, meshedObject& gasPump) {

    if (canInteract(player, gasPump.collider, gasPump.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 500.f)) {
        player.pState.canGasPump = true;
    }
    else {
        player.pState.canGasPump = false;
    }
}

void interact(player& player, float deltaTime, gameData& gData, meshedObject& car, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, Sound swallow) {

    if (getAsyncKeyStateWrapper(KEY_G)) {
        if (player.pState.hasDrank && interactingG) {
            drank.pEntity.location = {0.f, 0.f, 0.f};
            drank.pEntity.magnitude = {0.f, 0.f, 0.f};
            drank.pEntity.collidingY = false;
            player.pState.hasDrank = false;
            player.pState.canPickDrank = false;
            player.pState.canBuy = false;
            interactingG = false;
        }
    }
    else {
        interactingG = true;
    }
    if (getAsyncKeyStateWrapper(KEY_F) && !player.pState.uberMenuOpen) {
        if (player.pState.canGasPump && interacting) {
            if (!player.pState.hasGasPump && !player.pState.pumpingUp && !player.pState.gasMenuOpen) {
                player.pState.gasMenuOpen = true;
                player.canMove = false;
                PumpStorage = gasPumpNozzle.pEntity.location;
                playMenuSound(sndGasIntro);
                interacting = false;
            } else if (player.pState.hasGasPump) {
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

                gasPumpNozzleOff.pEntity.location = car.pEntity.location + vector3{0.f, 5.5f, 0.f};
                interacting = false;
            }
        }
        else if (interacting && player.pState.canPickDrank && !player.pState.hasDrank) {
            player.pState.hasDrank = true;
            interacting = false;
        }
        else if (interacting && player.pState.hasDrank) {
            if (player.pState.canBuy || player.pState.buying) {
                player.canMove = !player.canMove;
                player.pState.buying = !player.pState.buying;
                if (player.pState.buying) playMenuSound(sndShopIntro);
                interacting = false;
            }
            else if (player.pState.canDrinkDrank) {
                player.pState.canDrinkDrank = false;
                player.pState.drunkenness += 0.2f;
                PlaySound(swallow);
                player.pState.hasDrank = false;
                drank.pEntity.location = {0.f, 0.f, 0.f};
                gData.dranksConsumed++;
                gData.drankTimer = 170.0f;
                interacting = false;
            }
        }
        else if (interacting && player.pState.pumpingUp) {
            if (player.pState.canCar) {
                resetMeshedLocation(gasPumpNozzleOff);

                player.pState.hasGasPump = true;
                player.pState.pumpingUp = false;
                interacting = false;
                locked = false;
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

static constexpr int kNozzleAnchorTriA = 22;
static constexpr int kNozzleAnchorTriB = 120;

void deformGasPump(meshedObject& gasPump, meshedObject& gasPumpNozzleOff) {
    if (kNozzleAnchorTriA >= gasPumpNozzleOff.mesh.count || kNozzleAnchorTriB >= gasPumpNozzleOff.mesh.count) return;

    struct AffectedVert {int tri, vert; vector3 orig;};
    static AffectedVert affected[64];
    static int affectedCount = 0;
    static bool initialized = false;

    if (!initialized) {
        vector3 anchors[6];
        int ac = 0;
        for (int j = 0; j < 3; j++) {
            anchors[ac++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriA].v[j];
            anchors[ac++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriB].v[j];
        }
        const float kEps = 1e-4f;
        for (int i = 0; i < gasPumpNozzleOff.mesh.count && affectedCount < 64; i++) {
            for (int j = 0; j < 3 && affectedCount < 64; j++) {
                const vector3& v = gasPumpNozzleOff.mesh.trisO[i].v[j];
                for (int k = 0; k < ac; k++) {
                    if (fabsf(v.x - anchors[k].x) < kEps &&
                        fabsf(v.y - anchors[k].y) < kEps &&
                        fabsf(v.z - anchors[k].z) < kEps) {
                        affected[affectedCount++] = {i, j, v};
                        break;
                    }
                }
            }
        }
        initialized = true;
    }

    vector3 target = gasPumpOffset - gasPumpNozzleOff.pEntity.location;
    vector3 rot = gasPumpNozzleOff.pEntity.rot;

    float cz = cosf(rot.z), sz = sinf(rot.z);
    float cy = cosf(rot.y), sy = sinf(rot.y);
    float cx = cosf(rot.x), sx = sinf(rot.x);
    {float x = target.x*cz + target.y*sz; float y = -target.x*sz + target.y*cz; target.x=x; target.y=y;}
    {float x = target.x*cy - target.z*sy; float z = target.x*sy + target.z*cy; target.x=x; target.z=z;}
    {float y = target.y*cx + target.z*sx; float z = -target.y*sx + target.z*cx; target.y=y; target.z=z;}
    vector3 centroid = {0, 0, 0};
    for (int i = 0; i < affectedCount; i++) {
        centroid.x += affected[i].orig.x;
        centroid.y += affected[i].orig.y;
        centroid.z += affected[i].orig.z;
    }
    if (affectedCount > 0) {
        centroid.x /= affectedCount;
        centroid.y /= affectedCount;
        centroid.z /= affectedCount;
    }

    for (int i = 0; i < affectedCount; i++) {
        float relX = affected[i].orig.x - centroid.x;
        float relZ = affected[i].orig.z - centroid.z;
        gasPumpNozzleOff.mesh.trisO[affected[i].tri].v[affected[i].vert] = {
            target.x + relX,
                target.y,
                target.z + relZ
        };
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

int gGasMenuSelection = 0;

static void gasMenuLogic(player& player, meshedObject& gasPumpNozzle) {
    int& selection = gGasMenuSelection;
    static bool canNav = true;
    static bool canAction = false;

    if (!player.pState.gasMenuOpen) {
        selection = 0;
        canAction = false;
        return;
    }

    if (!getAsyncKeyStateWrapper(KEY_F))
    canAction = true;

    bool full = player.pState.carFuel >= 100.f;
    bool broke = player.pState.money <= 0.0f;
    int opts = (full || broke) ? 1 : 2;

    if (getAsyncKeyStateWrapper(KEY_UP)) {
        if (canNav) {selection = (selection - 1 + opts) % opts; canNav = false;}
    } else if (getAsyncKeyStateWrapper(KEY_DOWN)) {
        if (canNav) {selection = (selection + 1) % opts; canNav = false;}
    } else {
        canNav = true;
    }

    if (getAsyncKeyStateWrapper(KEY_F)) {
        if (canAction) {
            player.pState.gasMenuOpen = false;
            player.canMove = true;
            canAction = false;
        }
        return;
    }

    if (!getAsyncKeyStateWrapper(KEY_ENTER))
    canAction = true;

    if (!canAction) return;

    if (getAsyncKeyStateWrapper(KEY_ENTER)) {
        bool wantBuy = !full && !broke && selection == 0;
        bool wantClose = !wantBuy;
        if (wantBuy) {
            float gallonsNeeded = (100.f - player.pState.carFuel) / 4.f;
            float maxAfford = std::min(gallonsNeeded, player.pState.money / 2.5f);
            float cost = maxAfford * 2.5f;
            if (maxAfford > 0.0f) {
                player.pState.money -= cost;
                player.pState.carFuel += maxAfford * 4.0f;
                if (player.pState.carFuel > 100.f) player.pState.carFuel = 100.f;
                player.pState.hasGasPump = true;
                gasPumpNozzle.pEntity.location = {0.f, -20.f, 0.f};
                playMenuSound(sndGasPurchase);
            }
        }
        if (wantBuy || wantClose) {
            player.pState.gasMenuOpen = false;
            player.canMove = true;
            canAction = false;
        }
    }

}

void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, meshedObject& buyBox, Sound swallow) {
    gasPumpLogic(player, deltaTime, gData, gasPump, testcar, gasPumpNozzleOff);
    checkCar(player, deltaTime, gData, testcar);
    checkGasPump(player, deltaTime, gData, gasPump);
    checkDrank(player, deltaTime, gData, drank);
    checkBuyBox(player, deltaTime, gData, buyBox);
    if (player.pState.hasDrank) holdItem(player, drank);
    if (player.pState.buying) buy(player, 1);
    interact(player, deltaTime, gData, testcar, gasPump, gasPumpNozzle, gasPumpNozzleOff, drank, swallow);
    gasMenuLogic(player, gasPumpNozzle);

}
