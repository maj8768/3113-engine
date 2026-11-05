#include "sounds.h"

#include <cmath>

namespace {
Music* gEngineStream = nullptr;
}

// Legacy entry-point names retained for callers. Audio uses raylib's existing
// device and streaming API; there is no separate synth or audio thread.
void startCarEngineThread(Music& engineSound) {
    if (!IsMusicValid(engineSound)) return;
    if (gEngineStream && gEngineStream != &engineSound) stopCarEngineThread();
    gEngineStream = &engineSound;
    engineSound.looping = true;
    if (!IsMusicStreamPlaying(engineSound)) PlayMusicStream(engineSound);
}

void stopCarEngineThread() {
    if (gEngineStream && IsMusicValid(*gEngineStream)) StopMusicStream(*gEngineStream);
    gEngineStream = nullptr;
}

void processCarEngine(Music& engineSound, meshedObject& car, float maxSpeed, float) {
    if (!IsMusicValid(engineSound)) return;
    startCarEngineThread(engineSound);
    float speed = sqrtf(car.pEntity.magnitude.x*car.pEntity.magnitude.x +
                        car.pEntity.magnitude.z*car.pEntity.magnitude.z);
    float normalized = maxSpeed > 0.f ? fminf(speed/maxSpeed, 1.f) : 0.f;
    SetMusicPitch(engineSound, 0.7f + normalized*0.6f);
    SetMusicVolume(engineSound, 0.1f);
    UpdateMusicStream(engineSound);
}

void stopCarEngine(Music& engineSound) {
    if (IsMusicValid(engineSound)) StopMusicStream(engineSound);
    if (gEngineStream == &engineSound) gEngineStream = nullptr;
}
