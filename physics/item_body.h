#pragma once

#include "../util.h"

// A uniform box fitted once to the item's collision mesh. Gravity acts on one
// center of mass; contact impulses provide the torque. No render vertices move.
struct itemBody {
    vector3 localCenter;
    vector3 halfSize;
    vector3 inverseInertia;
    float inverseMass = 1.f;
    float radius = 0.f;
    Quaternion orientation = {0.f, 0.f, 0.f, 1.f};
    Quaternion previousOrientation = {0.f, 0.f, 0.f, 1.f};
    vector3 angularVelocity;
    float quietTime = 0.f;
    bool sleeping = false;
    int supportSegment = -1;
    int supportPlane = -1;
    vector3 supportVertices[4];
};

void initializeItemBody(itemBody& body, const meshedObject& object);
// Synchronize a held/teleported pose and clear all motion and sleep state.
void resetItemBody(itemBody& body, physicsEntity& entity);
// Release with exactly 0.25 world units/second along direction (no inherited speed).
void releaseItemBody(itemBody& body, physicsEntity& entity, vector3 direction);
void stepItemBody(itemBody& body, physicsEntity& entity, float dt, const world& scene);
// Shortest-path quaternion interpolation, converted to the renderer's Rz*Ry*Rx Euler order.
vector3 itemBodyRenderRotation(const itemBody& body, float alpha);
