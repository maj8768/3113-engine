#include "raylib.h"
#include "../util.h"
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>

static std::thread         gEngineThread;
static std::atomic<bool>   gEngineRunning{false};
static std::atomic<float>  gTargetPitch{0.6f};
static std::atomic<bool>   gShouldPlay{false};
static Music*              gEngineMusic = nullptr;

static void engineAudioThread() {
    const float minPitch  = 0.6f;
    const float pitchLerp = 4.f;
    const int   intervalMs = 10;        // 100 Hz update rate
    const float dt         = intervalMs / 1000.f;

    float currentPitch = minPitch;
    float lastSetPitch = -1.f;
    bool  started      = false;

    while (gEngineRunning.load(std::memory_order_relaxed)) {
        if (gShouldPlay.load(std::memory_order_relaxed)) {
            if (!started) {
                PlayMusicStream(*gEngineMusic);
                started = true;
            }

            float target = gTargetPitch.load(std::memory_order_relaxed);
            currentPitch += (target - currentPitch) * pitchLerp * dt;

            if (fabsf(currentPitch - lastSetPitch) > 0.005f) {
                SetMusicPitch(*gEngineMusic, currentPitch);
                lastSetPitch = currentPitch;
            }

            UpdateMusicStream(*gEngineMusic);
        } else if (started) {
            StopMusicStream(*gEngineMusic);
            started      = false;
            currentPitch = minPitch;
            lastSetPitch = -1.f;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

void startCarEngineThread(Music& engineSound) {
    gEngineMusic = &engineSound;
    gEngineRunning = true;
    gEngineThread  = std::thread(engineAudioThread);
}

void stopCarEngineThread() {
    gEngineRunning = false;
    if (gEngineThread.joinable()) gEngineThread.join();
    gEngineMusic = nullptr;
}

// Called each frame from car.cpp — just writes shared state, no raylib calls
void processCarEngine(Music& engineSound, meshedObject& car, float maxSpeed, float deltaTime) {
    const float minPitch = 0.5f;
    const float maxPitch = 0.92f;

    float hSpeed = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x +
                         car.pEntity.magnitude.z * car.pEntity.magnitude.z);

    float target = minPitch + fminf(hSpeed / maxSpeed, 1.f) * (maxPitch - minPitch);
    gTargetPitch.store(target, std::memory_order_relaxed);
    gShouldPlay.store(true,   std::memory_order_relaxed);
}

void stopCarEngine(Music& engineSound) {
    gShouldPlay.store(false, std::memory_order_relaxed);
}
