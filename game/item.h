#pragma once

#include "raylib.h"
#include "../util.h" // player, world, camera, shaderStore, meshedObject, mtx44, vector3

// A holdable world item. The meshedObject is owned elsewhere (e.g. a global in
// main.cpp, created via create3dObject); the item only references it. Every item
// can be held, but only ONE across the whole game at a time. F picks up the item
// you are aiming at (within range) or drops the one you hold.
struct item {
    meshedObject* obj;     // the item's mesh + collider (not owned by the item)
    float pickupDistance;  // how close the player must be, world units (canInteract maxDist)
    float lookatRadius;    // aim tolerance in screen pixels (canInteract radiusPixels)
    vector3 prevLoc;       // previous physics-tick location, for render interpolation
};

// Build an item around an existing meshedObject and give it gravity. Call after
// create3dObject, then register it with addItem.
item createItem(meshedObject* obj, float pickupDistance, float lookatRadius);

// Register an item so the handler manages it (physics, pickup/drop, drawing).
void addItem(item* it);

// The meshedObject currently held, or nullptr if nothing is held. Compare against
// a specific object (e.g. the wand) to gate actions on holding that item.
meshedObject* heldItemObject();

// Fixed timestep: gravity + collision for every item that isn't currently held.
void updateItemsPhysics(player& player, float deltaTime, world& worldInstance);

// Per-frame: F picks up the aimed item (only when in range and nothing is held) or
// drops the held one. While held, the item sits in front of the POV and rotates
// with the camera. Call AFTER the camera position is finalised for the frame.
void handleItemInput(player& player);

// Render all items: main lit pass (free items interpolated by alpha) + shadow depth.
void drawItems(const camera& cam, shaderStore& shader, float alpha, const mtx44* vp,
               const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar);
void drawItemsDepth(Shader depthShader, int modelLoc);
