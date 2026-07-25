#pragma once

#include "raylib.h"
#include "../util.h" // player, world, camera, shaderStore, meshedObject, mtx44, vector3

// A wizard's physical (projectile) attack: a live sphere flying under gravity.
// Each shot allocates its OWN sphere instance, which is freed (unloaded) when its
// lifetime expires. Instances never overwrite one another.
struct physicalAttack {
    meshedObject obj;   // the sphere instance (mesh + collider), built per shot
    float lifetime;     // seconds remaining before the instance is unloaded
    float damage;
    vector3 prevLoc;    // location at the previous physics tick, for render interpolation
};

// A future hitscan attack (e.g. lightning): an instantaneous ray.
struct hitscanAttack {
    vector3 start;
    vector3 end;
    float damage;
    float lifetime;
};

// Remember the shader used to build projectile instances (call once at startup).
void initWizardAttacks(shaderStore& shader);

// Allocate + launch a NEW sphere from the player's eye, along the look direction,
// at the fixed launch speed, under the same gravity the player uses.
void fireWizardAttack(player& player);

// Advance every live projectile (physics + lifetime); frees any that expired.
void updateWizardAttacks(float deltaTime, world& worldInstance, player& player);

// Render all live projectiles: main lit pass and shadow depth pass.
void drawWizardAttacks(const camera& cam, shaderStore& shader, float alpha, const mtx44* vp,
                       const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar);
void drawWizardAttacksDepth(Shader depthShader, int modelLoc);

// Free all remaining live projectiles (call at shutdown).
void shutdownWizardAttacks();
