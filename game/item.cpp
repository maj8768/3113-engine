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
    o->mesh.gpuDirty = true;
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

vector3 itemRenderLocation(const item& it, float alpha) {
    if (it.state == itemState::player) return it.obj->pEntity.location;
    return it.prevLoc + (it.obj->pEntity.location - it.prevLoc).fmult(alpha);
}

} // namespace

item createItem(meshedObject* obj, float pickupDistance, float lookatRadius) {
    item it{};
    it.uuid = generateItemUuid();
    it.obj = obj;
    it.pickupDistance = pickupDistance;
    it.lookatRadius = lookatRadius;
    it.prevLoc = obj->pEntity.location;
    initializePhysicsEntity(obj->pEntity, 1.f, COMPLEX);
    applyAcceleration(ITEM_GRAVITY, obj->pEntity); // fall like everything else when free
    rotateItemMeshY(obj, ITEM_FORWARD_FIX);        // align the model's forward to -Z
    initializeItemBody(it.body, *obj);
    return it;
}

void updateItemsPhysics(player& player, float deltaTime, world& worldInstance) {
    for (item* it : registeredItems()) {
        if (it->state != itemState::ground) continue; // equipped AND stowed items don't simulate
        it->prevLoc = it->obj->pEntity.location; // for interpolation
        bool wasSleeping = it->body.sleeping;
        stepItemBody(it->body, it->obj->pEntity, deltaTime, worldInstance);
        if (!wasSleeping || !it->body.sleeping) updateColliderLocation(*it->obj, player, false);
    }
}

void handleItemInput(player& player, bool actionsEnabled) {
    bool canAct = actionsEnabled && !player.pState.buying && !player.pState.gasMenuOpen
               && !player.pState.uberMenuOpen && !player.pState.inCar;
    // Poll even while blocked so a key held through a menu does not act on close.
    for (int i = 0; i < playerInventory::SLOT_COUNT; ++i) {
        bool pressed = getKeyPressedOnce(KEY_ONE + i);
        if (pressed && canAct) selectInventorySlot(player, i+1);
    }
    bool interact = getKeyPressedOnce(KEY_F);

    // One current-frame hold/drop pose, including bob. A newly selected item must
    // be dropped here, rather than at its stale position from when it was stowed.
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

    vector3 location = player.camera.camPos + F.fmult(HELD_FORWARD)
                     + R.fmult(HELD_RIGHT + hbob) + U.fmult(HELD_UP + vbob);
    // With the model re-aligned to -Z forward, this is the clean Ry(yaw)*Rx(pitch)
    // look: pitch on rot.x, yaw on rot.y (atan2 keeps it full-range and continuous,
    // no snap past 90 deg), roll left at 0. Flip camPitch's sign if it tilts the
    // wrong way. Verified correct at all yaw/pitch angles.
    float camYaw   = player.camera.camTarget.x;
    float camPitch = player.camera.camTarget.y;
    vector3 rotation = {camPitch + HELD_ROT_OFFSET.x,
                       atan2f(-cosf(camYaw), -sinf(camYaw)) + HELD_ROT_OFFSET.y,
                       HELD_ROT_OFFSET.z};

    if (interact && canAct) {
        item* target = nullptr;
        float nearest = INFINITY;
        for (item* candidate : registeredItems()) {
            if (candidate->state != itemState::ground) continue;
            if (!canInteract(player, candidate->obj->collider, candidate->obj->cPlaneCount,
                             candidate->pickupDistance, (float)GetScreenWidth(),
                             (float)GetScreenHeight(), candidate->lookatRadius)) continue;
            vector3 d = candidate->obj->pEntity.location - player.camera.camPos;
            float distance = dot3(d, d);
            if (distance < nearest) { target = candidate; nearest = distance; }
        }
        item* dropped = selectedItem(player);
        if (target) {
            if (!pickUpItem(player, target->uuid, location, rotation, F)) dropped = nullptr;
        } else dropped = dropSelectedItem(player, location, rotation, F);
        if (dropped) updateColliderLocation(*dropped->obj, player, false);
    }

    item* it = selectedItem(player);
    if (!it) return;
    it->obj->pEntity.location = location;
    it->obj->pEntity.rot = rotation;
    updateColliderLocation(*it->obj, player, false);
}

vector3 heldItemTip(const player& player) {
    item* it = selectedItem(player);
    if (!it) return player.camera.camPos;
    // The wand extends along local +Y. Its collider top is within a few cm of
    // the mesh tip. Transform this point with the actual held pose (including bob).
    vector3 tip = it->body.localCenter + vector3{0.f, it->body.halfSize.y, 0.f};
    mtx44 model = buildModelMatrix(it->obj->pEntity.location, it->obj->pEntity.rot, {});
    vector4 p = modmmult(model, {tip.x, tip.y, tip.z, 1.f});
    return {p.x, p.y, p.z};
}

void drawItems(const player& player, shaderStore& shader, float alpha, const mtx44* vp,
               const TextureCubemap* shadowTex, bool receiveShadows) {
    item* equipped = selectedItem(player);
    for (item* it : registeredItems()) {
        if (it->state == itemState::player && it != equipped) continue;
        vector3 real = it->obj->pEntity.location;
        vector3 realRot = it->obj->pEntity.rot;
        it->obj->pEntity.location = itemRenderLocation(*it, alpha);
        if (it->state == itemState::ground) it->obj->pEntity.rot = itemBodyRenderRotation(it->body, alpha);
        Draw3DGPU(*it->obj, player.camera, shader, {255, 255, 255, 255}, vp, shadowTex, receiveShadows);
        it->obj->pEntity.location = real;
        it->obj->pEntity.rot = realRot;
    }
}

void drawItemsDepth(const player& player, Shader depthShader, int modelLoc, float alpha) {
    item* equipped = selectedItem(player);
    for (item* it : registeredItems()) {
        if (it->state == itemState::player && it != equipped) continue;
        vector3 real = it->obj->pEntity.location;
        vector3 realRot = it->obj->pEntity.rot;
        it->obj->pEntity.location = itemRenderLocation(*it, alpha);
        if (it->state == itemState::ground) it->obj->pEntity.rot = itemBodyRenderRotation(it->body, alpha);
        Draw3DDepthGPU(*it->obj, depthShader, modelLoc);
        it->obj->pEntity.location = real;
        it->obj->pEntity.rot = realRot;
    }
}
