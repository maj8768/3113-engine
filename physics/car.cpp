#include "../util.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "raylib.h"
#include "physics.h"
#include "sounds.h"
#include "string"

void processCar(meshedObject& car, player& player, float deltaTime, Music& engineSound) {
    const float accel        = 5.f;   // forward acceleration
    const float brakeAccel   = 5.5f;   // braking/reverse acceleration
    const float drag         = 0.25f;    // velocity drag coefficient when coasting
    const float turnRate     = 2.2f;   // max yaw radians per second at full steering lock
    const float steerInRate  = 3.f;    // how fast steering angle builds up (units/sec)
    const float steerOutRate = 5.f;    // how fast steering returns to center (units/sec)
    const float maxSpeed     = 20.f;   // reference speed for speed-sensitive steering

    // Persistent steering angle in [-1, 1]: builds toward input, decays toward 0
    static float steeringAngle = 0.f;

    // World-space forward direction
    vector3 forward = {0.f, 0.f, 0.f};
    applyRot(forward, car.pEntity.rot, 0.f, 0.f, -1.f);

    if (player.pState.forward > 0.f) {
        car.pEntity.magnitude = car.pEntity.magnitude + forward.fmult(accel * deltaTime);
    } else if (player.pState.brake > 0.f) {
        car.pEntity.magnitude = car.pEntity.magnitude + forward.fmult(-brakeAccel * deltaTime);
    } else {
        // Coast: apply drag proportional to current velocity
        car.pEntity.magnitude = car.pEntity.magnitude + car.pEntity.magnitude.fmult(-drag * deltaTime);
    }

    // Determine steering target from input
    float steerTarget = 0.f;
    if (player.pState.leftTurn > 0.f)  steerTarget =  1.f;
    if (player.pState.rightTurn > 0.f) steerTarget = -1.f;

    // Move steering angle linearly toward target ("equal parts")
    float rate = (steerTarget != 0.f) ? steerInRate : steerOutRate;
    if (steerTarget > steeringAngle)
        steeringAngle = fminf(steeringAngle + rate * deltaTime, steerTarget);
    else
        steeringAngle = fmaxf(steeringAngle - rate * deltaTime, steerTarget);

    // Speed-sensitive steering: less turn effect at very low speed
    float hSpeed = sqrtf(car.pEntity.magnitude.x * car.pEntity.magnitude.x +
                         car.pEntity.magnitude.z * car.pEntity.magnitude.z);
    float speedFactor = fminf(hSpeed / maxSpeed, 1.f);

    float dYaw = turnRate * steeringAngle * speedFactor * deltaTime;
    car.pEntity.rot.y += dYaw;

    // Rotate the velocity vector by the same yaw so momentum follows the new heading
    float cosA = cosf(dYaw);
    float sinA = sinf(dYaw);
    float vx = car.pEntity.magnitude.x * cosA + car.pEntity.magnitude.z * sinA;
    float vz = -car.pEntity.magnitude.x * sinA + car.pEntity.magnitude.z * cosA;
    car.pEntity.magnitude.x = vx;
    car.pEntity.magnitude.z = vz;

    // Reset each frame so input doesn't accumulate across frames
    player.pState.forward   = 0.f;
    player.pState.brake     = 0.f;
    player.pState.leftTurn  = 0.f;
    player.pState.rightTurn = 0.f;

    if (player.pState.inCar) {
        processCarEngine(engineSound, car, 50.f, deltaTime);
    } else {
        stopCarEngine(engineSound);
    }
}
