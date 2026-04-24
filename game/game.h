#pragma once
#include "raylib.h"

void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, meshedObject& buyBox, Sound swallow);
void resetLevelLogicState();
void playMenuSound(Sound s);

extern int gGasMenuSelection;
extern Sound sndShopIntro;
extern Sound sndShopBye;
extern Sound sndShopThank;
extern Sound sndGasIntro;
extern Sound sndGasPurchase;