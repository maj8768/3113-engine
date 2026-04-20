#pragma once
#include "raylib.h"
#include "../util.h"

void startCarEngineThread(Music& engineSound);
void stopCarEngineThread();
void processCarEngine(Music& engineSound, meshedObject& car, float maxSpeed, float deltaTime);
void stopCarEngine(Music& engineSound);
