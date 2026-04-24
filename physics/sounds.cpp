#include "sounds.h"
#include "../music/engine_synth.h"
#include "../util.h"
#include "raylib.h"
#include <cmath>

void startCarEngineThread(Music& engineSound) {
    engineSynth_start();
}

void stopCarEngineThread() {
    engineSynth_stop();
}

void processCarEngine(Music& engineSound, meshedObject& car, float maxSpeed, float deltaTime) {
    float hSpeed = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x +
        car.pEntity.magnitude.z * car.pEntity.magnitude.z);
    engineSynth_setRPM(fminf(hSpeed / maxSpeed, 1.f));
    engineSynth_setVolume(1.f);
}

void stopCarEngine(Music& engineSound) {

    engineSynth_setVolume(0.f);
}
