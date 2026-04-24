#include "../util.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "raylib.h"
#include "physics.h"
#include "sounds.h"
#include "../music/engine_synth.h"
#include "string"

void processCar(meshedObject& car, meshedObject& wheel, player& player, float deltaTime, Music& engineSound) {

    static const float gearRpmRise[4] = {0.1875f, 0.125f, 0.0625f, 0.04175f};
    static const float gearRpmFall[4] = {0.80f, 0.50f, 0.30f, 0.16f};
    static const float gearTopSpeeds[4] = {10.f, 20.f, 35.f, 65.f};

    const float reverseAccel = 4.0f;
    const float brakeAccel = 18.f;
    const float drag = 0.25f;
    const float turnRate = 2.2f;
    const float steerInRate = 3.f;
    const float steerOutRate = 5.f;
    const int numGears = 4;
    const float reverseTopSpeed = 8.f;

    static float steeringAngle = 0.f;
    static int gear = 0;
    static int pendingGear = 0;
    static bool shiftPending = false;
    static bool shiftIsUp = false;
    static float shiftTimer = 0.f;
    const float shiftDelay = 0.45f;

    vector3 forward = {0.f, 0.f, 0.f};
    applyRot(forward, car.pEntity.rot, 0.f, 0.f, -1.f);
    if (!shiftPending) {
        int targetGear = gear;
        if (player.pState.shiftUp && gear < numGears) {
            if (gear >= 1) targetGear = gear + 1;
            else if (gear == 0) targetGear = 1;
            else if (gear == -1) targetGear = 0;
        }
        if (player.pState.shiftDown && gear > 0) {
            targetGear = gear - 1;
        }
        if (player.pState.reverseDown) {
            if (gear == 0) targetGear = -1;
            else if (gear == -1) targetGear = 0;
        }
        if (targetGear != gear) {
            pendingGear = targetGear;
            shiftPending = true;
            shiftIsUp = (targetGear > gear);
            shiftTimer = 0.f;
        }
    }
    player.pState.shiftUp = false;
    player.pState.shiftDown = false;
    player.pState.reverseDown = false;

    if (shiftPending) {
        shiftTimer += deltaTime;
        if (shiftTimer >= shiftDelay) {
            gear = pendingGear;
            shiftPending = false;
        }
    }

    float hSpeedPre = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x + car.pEntity.magnitude.z * car.pEntity.magnitude.z);

    bool cutThrottle = shiftPending && !shiftIsUp;
    float throttleInput = (player.pState.forward > 0.f && !cutThrottle) ? 1.f : 0.f;

    static float rpm = 0.f;
    static int lastGear = 0;

    bool gearChanged = (gear != lastGear);
    lastGear = gear;

    if (gear == 0) {
        if (throttleInput > 0.f) rpm = fminf(rpm + 0.6875f * deltaTime, 1.f);
        else rpm = fmaxf(rpm - 1.0f * deltaTime, 0.f);
        player.pState.carFuel = fmaxf(player.pState.carFuel - deltaTime * 0.5f * rpm, 0.f);

    }
    else if (gear == -1) {
        rpm = fmaxf(0.f, fminf(hSpeedPre / reverseTopSpeed, 1.f));
        player.pState.carFuel = fmaxf(player.pState.carFuel - deltaTime * 0.5f * rpm, 0.f);
    }
    else {
        static const float upshiftRetain[4] = {1.f, 0.40f, 0.55f, 0.65f};

        if (gearChanged) {
            if (shiftIsUp && gear >= 2 && gear <= 4) {
                rpm = rpm * upshiftRetain[gear - 1];
            } else {

                rpm = fmaxf(0.f, fminf(hSpeedPre / gearTopSpeeds[gear - 1], 1.f));
            }
        }
        else if (player.pState.brake > 0.f) {
            float brakeRpmRate = brakeAccel / gearTopSpeeds[gear - 1];
            rpm = fmaxf(rpm - brakeRpmRate * deltaTime, 0.f);
        }
        else if (throttleInput > 0.f && player.pState.carFuel > 0.f) {
            rpm = fminf(rpm + gearRpmRise[gear - 1] * deltaTime, 1.f);
            player.pState.carFuel = fmaxf(player.pState.carFuel - deltaTime * 0.5f * rpm, 0.f);
        }
        else {
            rpm = fmaxf(rpm - gearRpmFall[gear - 1] * deltaTime, 0.f);
        }
    }

    if (gearChanged && gear >= 0) {
        if (shiftIsUp) engineSynth_upshift(rpm);
        else engineSynth_downshift(rpm);
    }

    if (gear == 0) {
        car.pEntity.magnitude = car.pEntity.magnitude + car.pEntity.magnitude.fmult(-drag * deltaTime);
    }
    else if (gear == -1) {
        float backSpeed = -(forward.x * car.pEntity.magnitude.x + forward.z * car.pEntity.magnitude.z);
        if (player.pState.forward > 0.f) {
            if (backSpeed < reverseTopSpeed)
            car.pEntity.magnitude = car.pEntity.magnitude + forward.fmult(-reverseAccel * deltaTime);
        }
        else if (player.pState.brake > 0.f) {
            float spd = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x + car.pEntity.magnitude.z * car.pEntity.magnitude.z);
            if (spd > 0.f) {
                float decel = fminf(brakeAccel * deltaTime, spd);
                car.pEntity.magnitude.x *= (spd - decel) / spd;
                car.pEntity.magnitude.z *= (spd - decel) / spd;
            }
        }
        else {
            car.pEntity.magnitude = car.pEntity.magnitude + car.pEntity.magnitude.fmult(-drag * deltaTime);
        }
    }
    else {
        float driveSpeed = rpm * gearTopSpeeds[gear - 1];
        float fwdLen = forward.mag();
        if (fwdLen > 1e-5f) {
            vector3 fwdUnit = forward.fmult(1.f / fwdLen);
            float curFwdSpeed = dot3(car.pEntity.magnitude, fwdUnit);
            car.pEntity.magnitude.x += fwdUnit.x * (driveSpeed - curFwdSpeed);
            car.pEntity.magnitude.z += fwdUnit.z * (driveSpeed - curFwdSpeed);

            vector3 fwdVel = fwdUnit.fmult(dot3(car.pEntity.magnitude, fwdUnit));
            float latDamp = powf(0.01f, deltaTime);
            car.pEntity.magnitude.x = fwdVel.x + (car.pEntity.magnitude.x - fwdVel.x) * latDamp;
            car.pEntity.magnitude.z = fwdVel.z + (car.pEntity.magnitude.z - fwdVel.z) * latDamp;
        }
    }

    float hMag = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x +
        car.pEntity.magnitude.z * car.pEntity.magnitude.z);
    if (hMag < 0.01f) {
        car.pEntity.magnitude.x = 0.f;
        car.pEntity.magnitude.z = 0.f;
    }

    float steerTarget = 0.f;
    if (player.pState.leftTurn > 0.f) steerTarget = 1.f;
    if (player.pState.rightTurn > 0.f) steerTarget = -1.f;

    float rate = (steerTarget != 0.f) ? steerInRate : steerOutRate;
    if (steerTarget > steeringAngle) {
        steeringAngle = fminf(steeringAngle + rate * deltaTime, steerTarget);
    }
    else {
        steeringAngle = fmaxf(steeringAngle - rate * deltaTime, steerTarget);
    }
    float hSpeed = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x + car.pEntity.magnitude.z * car.pEntity.magnitude.z);
    float dYaw = turnRate * steeringAngle * fminf(hSpeed / 15.f, 1.f) * deltaTime;
    car.pEntity.rot.y += dYaw;

    vector3 wheelLocalOffset = {0.f, 0.f, 0.f};
    applyRot(wheelLocalOffset, car.pEntity.rot, -1.4f, 3.2f, -2.3f);
    wheel.pEntity.location.x = car.pEntity.location.x + wheelLocalOffset.x;
    wheel.pEntity.location.y = car.pEntity.location.y + wheelLocalOffset.y;
    wheel.pEntity.location.z = car.pEntity.location.z + wheelLocalOffset.z;

    static tri* wheelOriginalTris = nullptr;
    if (!wheelOriginalTris) {
        wheelOriginalTris = (tri*)malloc(sizeof(tri) * wheel.mesh.count);
        memcpy(wheelOriginalTris, wheel.mesh.trisO, sizeof(tri) * wheel.mesh.count);
    }
    memcpy(wheel.mesh.trisO, wheelOriginalTris, sizeof(tri) * wheel.mesh.count);

    float wheelAngle = steeringAngle * 1.5f;
    if (fabsf(wheelAngle) > 0.0001f) {
        vector3 axle = {0.f, 0.f, 1.f};
        float cosW = cosf(wheelAngle), sinW = sinf(wheelAngle), omcW = 1.f - cosW;
        for (int i = 0; i < wheel.mesh.count; i++) {
            for (int j = 0; j < 3; j++) {
                vector3& v = wheel.mesh.trisO[i].v[j];
                float dot = axle.x*v.x + axle.y*v.y + axle.z*v.z;
                float cx = axle.y*v.z - axle.z*v.y;
                float cy = axle.z*v.x - axle.x*v.z;
                float cz = axle.x*v.y - axle.y*v.x;
                v.x = v.x*cosW + cx*sinW + axle.x*dot*omcW;
                v.y = v.y*cosW + cy*sinW + axle.y*dot*omcW;
                v.z = v.z*cosW + cz*sinW + axle.z*dot*omcW;
            }
        }
    }
    wheel.pEntity.rot.x = car.pEntity.rot.x;
    wheel.pEntity.rot.y = car.pEntity.rot.y;
    wheel.pEntity.rot.z = car.pEntity.rot.z;

    float cosA = cosf(dYaw), sinA = sinf(dYaw);
    float vx = car.pEntity.magnitude.x * cosA + car.pEntity.magnitude.z * sinA;
    float vz = -car.pEntity.magnitude.x * sinA + car.pEntity.magnitude.z * cosA;
    car.pEntity.magnitude.x = vx;
    car.pEntity.magnitude.z = vz;

    player.pState.forward = 0.f;
    player.pState.brake = 0.f;
    player.pState.leftTurn = 0.f;
    player.pState.rightTurn = 0.f;

    if (player.pState.inCar) {
        engineSynth_setRPM(rpm);
        engineSynth_setThrottle(throttleInput);
        engineSynth_setVolume(0.1f);
        engineSynth_setGear(gear);
    }
    else {
        engineSynth_setThrottle(0.f);
        stopCarEngine(engineSound);
    }
    engineSynth_update();

    float absRPM = 1500.f + rpm * (6000.f - 1500.f);

}
