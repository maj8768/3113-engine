#include "attack_wizard.h"
#include "../physics/physics.h"
#include "../draw/draw.h"

#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>

// Defined in main.cpp (external linkage). Used ONCE at startup to build the shared
// sphere template; projectiles are cheap copies of it, not fresh loads.
void create3dObject(meshedObject& object, const char* path, const char* colliderPath,
                    bool collider, shaderStore& shader, float scale, vector3 location,
                    char text[100], bool renderText, float textRenderDistance,
                    vector3 relativeTextOffset);

namespace {

const float   ATTACK_SPEED    = 25.0f;               // launch speed
const vector3 ATTACK_GRAVITY  = {0.f, -20.5f, 0.f}; // identical to the player's gravity
const float   ATTACK_LIFETIME = 8.0f;               // seconds before the instance is unloaded
const float   ATTACK_DAMAGE   = 10.0f;
const float   ATTACK_SCALE    = 1.0f;               // sphere.obj scale (tune to taste)

std::vector<physicalAttack*> gAttacks;  // live projectiles, one heap instance each
meshedObject gTemplate;                 // sphere loaded ONCE (disk + GPU upload)
bool gTemplateLoaded = false;

// Build a projectile by copying the shared template — malloc/memcpy only, no disk
// read, no OBJ parse, no GPU upload. The mesh geometry (trisO), collider, and the
// scratch tris buffer are per-instance so instances never stomp each other; the
// GPU texture is SHARED (its id is just copied), so it is not unloaded per shot.
physicalAttack* instantiateFromTemplate() {
    physicalAttack* a = new physicalAttack();
    a->obj = gTemplate; // shallow copy: scalars, texo id, emissive/bloom, and pointers

    int triCount = gTemplate.mesh.count;
    a->obj.mesh.trisO = (tri*)malloc(triCount * sizeof(tri));
    a->obj.mesh.tris  = (tri*)malloc(triCount * sizeof(tri));
    memcpy(a->obj.mesh.trisO, gTemplate.mesh.trisO, triCount * sizeof(tri));
    memcpy(a->obj.mesh.tris,  gTemplate.mesh.tris,  triCount * sizeof(tri));

    int planeCount = gTemplate.cPlaneCount;
    a->obj.collider  = new planeMtx[planeCount];
    a->obj.colliderO = new planeMtx[planeCount];
    memcpy(a->obj.collider,  gTemplate.collider,  planeCount * sizeof(planeMtx));
    memcpy(a->obj.colliderO, gTemplate.colliderO, planeCount * sizeof(planeMtx));

    return a;
}

void freeAttack(physicalAttack* a) {
    // Per-instance copies: tris via malloc, collider via new[]. The texture is
    // shared with the template, so it is NOT unloaded here (only at shutdown).
    free(a->obj.mesh.tris);
    free(a->obj.mesh.trisO);
    delete[] a->obj.collider;
    delete[] a->obj.colliderO;
    delete a;
}

} // namespace

void initWizardAttacks(shaderStore& shader) {
    if (gTemplateLoaded) return;
    char empty[100] = "";
    // The one and only load of sphere.obj + its collider (disk + GPU upload).
    create3dObject(gTemplate, "resources/levels/sphere.obj",
                   "resources/levels/sphere_collider.obj", true, shader,
                   ATTACK_SCALE, {0.f, 0.f, 0.f}, empty, false, 100, {0.f, 0.f, 0.f});
    gTemplateLoaded = true;
}

void fireWizardAttack(player& player) {
    if (!gTemplateLoaded) return;

    physicalAttack* a = instantiateFromTemplate();

    initializePhysicsEntity(a->obj.pEntity, 1.f, COMPLEX);
    applyAcceleration(ATTACK_GRAVITY, a->obj.pEntity); // same accel the player has

    // Launch from eye level, along the look direction, at the fixed speed. camTarget
    // stores yaw (.x) and pitch (.y) angles, NOT a world point — build the same
    // forward vector the view matrix uses (see viewMtx44) so the ball goes exactly
    // where the crosshair points.
    vector3 eye = player.camera.camPos;
    float cy = cosf(player.camera.camTarget.x), sy = sinf(player.camera.camTarget.x);
    float cp = cosf(player.camera.camTarget.y), sp = sinf(player.camera.camTarget.y);
    vector3 dir = normalize3({cp * cy, sp, cp * sy});
    a->obj.pEntity.location  = eye;
    a->obj.pEntity.magnitude = dir.fmult(ATTACK_SPEED);
    a->prevLoc = eye; // seed interpolation so the first frame doesn't lerp from origin
    updateColliderLocation(a->obj, player, false); // position the collider at the spawn

    a->lifetime = ATTACK_LIFETIME;
    a->damage   = ATTACK_DAMAGE;
    gAttacks.push_back(a);
}

void updateWizardAttacks(float deltaTime, world& worldInstance, player& player) {
    bool end = false;
    int target = 0;
    for (size_t i = 0; i < gAttacks.size();) {
        physicalAttack* a = gAttacks[i];
        a->prevLoc = a->obj.pEntity.location; // remember this tick's start for interpolation
        updateColliderLocation(a->obj, player, false);
        processPhysics(deltaTime, 0, a->obj.pEntity, worldInstance, end, target,
                       false, true, a->obj.collider, a->obj.cPlaneCount);
        a->lifetime -= deltaTime;
        if (a->lifetime <= 0.f) {
            freeAttack(a);
            gAttacks.erase(gAttacks.begin() + i);
        } else {
            i++;
        }
    }
}

void drawWizardAttacks(const camera& cam, shaderStore& shader, float alpha, const mtx44* vp,
                       const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar) {
    for (physicalAttack* a : gAttacks) {
        // Render BETWEEN the last two physics ticks: draw at lerp(prev, current, alpha),
        // matching the camera's interpolation so motion is smooth at any framerate.
        vector3 real = a->obj.pEntity.location;
        a->obj.pEntity.location = a->prevLoc + (real - a->prevLoc).fmult(alpha);
        Draw3DGPU(a->obj, cam, shader, {255, 255, 255, 255}, vp, shadowTex, receiveShadows, shadowTexFar);
        a->obj.pEntity.location = real; // restore the true physics state for the next tick
    }
}

void drawWizardAttacksDepth(Shader depthShader, int modelLoc) {
    for (physicalAttack* a : gAttacks)
        Draw3DDepthGPU(a->obj, depthShader, modelLoc);
}

void shutdownWizardAttacks() {
    for (physicalAttack* a : gAttacks) freeAttack(a);
    gAttacks.clear();

    if (gTemplateLoaded) {
        if (gTemplate.texo.id > 1) UnloadTexture(gTemplate.texo);
        free(gTemplate.mesh.tris);
        free(gTemplate.mesh.trisO);
        delete[] gTemplate.collider;
        delete[] gTemplate.colliderO;
        gTemplateLoaded = false;
    }
}
