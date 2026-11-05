#pragma once

#include "raylib.h"
#include "../util.h" // player, world, camera, shaderStore, meshedObject, mtx44, vector3
#include "../physics/item_body.h"

enum class itemState { ground, player };

// The meshedObject is owned elsewhere; an item only references it. Ground items
// simulate/render normally. Player items occupy exactly one UUID inventory slot;
// only the selected slot's item is visible/equipped, the others are stowed.
struct item {
    itemUuid uuid;
    itemState state = itemState::ground;
    player* owner = nullptr;
    meshedObject* obj;     // the item's mesh + collider (not owned by the item)
    float pickupDistance;  // how close the player must be, world units (canInteract maxDist)
    float lookatRadius;    // aim tolerance in screen pixels (canInteract radiusPixels)
    vector3 prevLoc;       // previous physics-tick location, for render interpolation
    itemBody body;        // cheap rigid box, angular motion, and sleep state
};

// Build an item around an existing meshedObject and give it rigid-body gravity. Call after
// create3dObject, then register it with addItem.
item createItem(meshedObject* obj, float pickupDistance, float lookatRadius);

// Registry/ownership operations also work without a window or input device.
// Registration rejects nil/duplicate IDs. Clear before destroying item/player objects.
bool addItem(item* it);
void clearItems();
const std::vector<item*>& registeredItems();
item* findItem(itemUuid id);
item* inventoryItem(const player& player, int slot); // public slot numbers are 1–6
item* selectedItem(const player& player);
bool selectInventorySlot(player& player, int slot);
item* dropSelectedItem(player& player, vector3 location, vector3 rotation, vector3 direction);
bool pickUpItem(player& player, itemUuid id, vector3 heldLocation, vector3 heldRotation, vector3 dropDirection);

// The meshedObject currently held, or nullptr if nothing is held. Compare against
// a specific object (e.g. the wand) to gate actions on holding that item.
meshedObject* heldItemObject(const player& player);
// Single world-space origin for the charge sparkles, fireball, and beam.
vector3 heldItemTip(const player& player);

// Fixed timestep: gravity, contact torque, friction, and sleep for free items.
void updateItemsPhysics(player& player, float deltaTime, world& worldInstance);

// 1–6 select slots. F takes an aimed ground item into the selected slot, dropping
// its previous contents; with no target, F drops the selected item. Other slots stay stored.
// Call AFTER the camera is finalised. Polls edges even when actions are disabled.
void handleItemInput(player& player, bool actionsEnabled = true);

// Render all items: main lit pass (free items interpolated by alpha) + shadow depth.
void drawItems(const player& player, shaderStore& shader, float alpha, const mtx44* vp,
               const TextureCubemap* shadowTex, bool receiveShadows);
void drawItemsDepth(const player& player, Shader depthShader, int modelLoc, float alpha);
