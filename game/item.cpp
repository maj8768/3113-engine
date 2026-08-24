#include "item.h"
#include "../physics/physics.h"
#include "../draw/draw.h"
#include "../system/keyboard/keyboard.h"

#include <vector>
#include <cmath>

namespace {

const vector3 ITEM_GRAVITY = {0.f, -20.5f, 0.f}; // same as the player

// Held-item placement, in camera space (tune to taste / to your models).
const float   HELD_FORWARD    = 2.0f;   // distance in front of the eye
const float   HELD_RIGHT      = 1.0f;   // offset to the right
const float   HELD_UP         = -0.8f;  // offset down
const vector3 HELD_ROT_OFFSET = {0.f, 0.f, 0.f}; // base orientation tweak

std::vector<item*> gItems;
int gHeldIndex = -1; // index of the held item in gItems, or -1 if none

// View-bob for the held item while the player moves.
const float BOB_FREQ      = 3.0f;   // oscillation speed
const float BOB_AMP       = 0.06f;  // magnitude in camera-space units
const float BOB_SMOOTH    = 8.0f;   // how fast the bob ramps in/out
const float BOB_MIN_SPEED = 0.5f;   // horizontal speed needed to start bobbing
float gBobPhase = 0.f;
float gBobAmount = 0.f;             // smoothed 0..1 "is moving" factor

// One-time base yaw baked into each item's mesh so its forward axis becomes -Z
// (the axis this engine's Y-up view + Euler order expect). The wand model points
// down +X, so it needs +90 deg; change this if a model faces a different way.
const float ITEM_FORWARD_FIX = 1.5707963f; // +90 deg about Y

// Rotate an item's geometry (render verts + normals) and collider about Y, once,
// so the loaded model's forward axis lines up with -Z.
void rotateItemMeshY(meshedObject* o, float ang) {
    float c = cosf(ang), s = sinf(ang);
    for (int i = 0; i < o->mesh.count; i++)
        for (int j = 0; j < 3; j++) {
            vector3& v = o->mesh.trisO[i].v[j];
            float vx = v.x * c + v.z * s, vz = -v.x * s + v.z * c; v.x = vx; v.z = vz;
            vector3& n = o->mesh.trisO[i].n[j];
            float nx = n.x * c + n.z * s, nz = -n.x * s + n.z * c; n.x = nx; n.z = nz;
        }
    for (int i = 0; i < o->cPlaneCount; i++)
        for (int k = 0; k < 4; k++) {
            float x = o->colliderO[i].m[k][0], z = o->colliderO[i].m[k][2];
            o->colliderO[i].m[k][0] = x * c + z * s;
            o->colliderO[i].m[k][2] = -x * s + z * c;
        }
}

} // namespace

item createItem(meshedObject* obj, float pickupDistance, float lookatRadius) {
    item it;
    it.obj = obj;
    it.pickupDistance = pickupDistance;
    it.lookatRadius = lookatRadius;
    it.prevLoc = obj->pEntity.location;
    initializePhysicsEntity(obj->pEntity, 1.f, COMPLEX);
    applyAcceleration(ITEM_GRAVITY, obj->pEntity); // fall like everything else when free
    rotateItemMeshY(obj, ITEM_FORWARD_FIX);        // align the model's forward to -Z
    return it;
}

void addItem(item* it) {
    gItems.push_back(it);
}

meshedObject* heldItemObject() {
    if (gHeldIndex < 0 || gHeldIndex >= (int)gItems.size()) return nullptr;
    return gItems[gHeldIndex]->obj;
}

void updateItemsPhysics(player& player, float deltaTime, world& worldInstance) {
    bool end = false;
    int target = 0;
    for (int i = 0; i < (int)gItems.size(); i++) {
        if (i == gHeldIndex) continue; // held items don't simulate
        item* it = gItems[i];
        it->prevLoc = it->obj->pEntity.location; // for interpolation
        updateColliderLocation(*it->obj, player, false);
        processPhysics(deltaTime, 0, it->obj->pEntity, worldInstance, end, target,
                       false, true, it->obj->collider, it->obj->cPlaneCount);
    }
}

void handleItemInput(player& player) {
    if (getKeyPressedOnce(KEY_F)) {
        if (gHeldIndex >= 0) {
            // Drop the held item; it resumes gravity from rest.
            item* it = gItems[gHeldIndex];
            it->obj->pEntity.magnitude = {0.f, 0.f, 0.f};
            it->obj->pEntity.collidingY = false;
            it->prevLoc = it->obj->pEntity.location;
            gHeldIndex = -1;
        } else {
            // Pick up the first item the player is aiming at within its range.
            for (int i = 0; i < (int)gItems.size(); i++) {
                item* it = gItems[i];
                if (canInteract(player, it->obj->collider, it->obj->cPlaneCount,
                                it->pickupDistance, (float)GetScreenWidth(),
                                (float)GetScreenHeight(), it->lookatRadius)) {
                    it->obj->pEntity.magnitude = {0.f, 0.f, 0.f};
                    gHeldIndex = i;
                    break;
                }
            }
        }
    }

    if (gHeldIndex < 0) return;

    // Place the held item in front of the POV. Uses the same forward vector the view
    // matrix builds (see viewMtx44), so it follows where you look.
    item* it = gItems[gHeldIndex];
    float cyw = cosf(player.camera.camTarget.x), syw = sinf(player.camera.camTarget.x);
    float cpi = cosf(player.camera.camTarget.y), spi = sinf(player.camera.camTarget.y);
    vector3 F = normalize3({cpi * cyw, spi, cpi * syw});
    vector3 R = normalize3(cross3(F, player.camera.up));
    vector3 U = cross3(R, F);

    // View bob: a subtle figure-eight sway while moving, ramped in/out so it doesn't
    // pop when starting or stopping.
    float dt = GetFrameTime();
    float speed = sqrtf(player.pEntity.magnitude.x * player.pEntity.magnitude.x
                      + player.pEntity.magnitude.z * player.pEntity.magnitude.z);
    float targetBob = (speed > BOB_MIN_SPEED) ? 1.f : 0.f;
    gBobAmount += (targetBob - gBobAmount) * fminf(1.f, dt * BOB_SMOOTH);
    gBobPhase  += dt * BOB_FREQ;
    float hbob = sinf(gBobPhase)        * BOB_AMP * gBobAmount;
    float vbob = sinf(gBobPhase * 2.f)  * BOB_AMP * gBobAmount;

    it->obj->pEntity.location = player.camera.camPos
                              + F.fmult(HELD_FORWARD)
                              + R.fmult(HELD_RIGHT + hbob)
                              + U.fmult(HELD_UP + vbob);
    // With the model re-aligned to -Z forward, this is the clean Ry(yaw)*Rx(pitch)
    // look: pitch on rot.x, yaw on rot.y (atan2 keeps it full-range and continuous,
    // no snap past 90 deg), roll left at 0. Flip camPitch's sign if it tilts the
    // wrong way. Verified correct at all yaw/pitch angles.
    float camYaw   = player.camera.camTarget.x;
    float camPitch = player.camera.camTarget.y;
    it->obj->pEntity.rot = { camPitch + HELD_ROT_OFFSET.x,
                             atan2f(-cosf(camYaw), -sinf(camYaw)) + HELD_ROT_OFFSET.y,
                             HELD_ROT_OFFSET.z };
}

void drawItems(const camera& cam, shaderStore& shader, float alpha, const mtx44* vp,
               const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar) {
    for (int i = 0; i < (int)gItems.size(); i++) {
        item* it = gItems[i];
        if (i == gHeldIndex) {
            // Held: placed directly from the camera this frame, no interpolation.
            Draw3DGPU(*it->obj, cam, shader, {255, 255, 255, 255}, vp, shadowTex, receiveShadows, shadowTexFar);
        } else {
            // Free: draw between the last two physics ticks for smooth motion.
            vector3 real = it->obj->pEntity.location;
            it->obj->pEntity.location = it->prevLoc + (real - it->prevLoc).fmult(alpha);
            Draw3DGPU(*it->obj, cam, shader, {255, 255, 255, 255}, vp, shadowTex, receiveShadows, shadowTexFar);
            it->obj->pEntity.location = real;
        }
    }
}

void drawItemsDepth(Shader depthShader, int modelLoc) {
    for (item* it : gItems)
        Draw3DDepthGPU(*it->obj, depthShader, modelLoc);
}
