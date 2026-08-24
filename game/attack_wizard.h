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
    int emitterId;      // trailing fire emitter that follows the ball (-1 if none)
    Sound travelSound;  // per-shot looping travel sound (alias); pan/volume follow the ball
    bool  hasTravelSound;
    bool  drawMesh;     // true: render the object's own mesh (heaven sword); false: mesh is invisible (fireball is drawn as particles)
    bool  isSword;      // true: heaven sword (darkblast impact, smoke-only blast, guide light); false: fireball
    bool  landed;       // sword only: true once it has struck the ground — frozen in place, fading out
    float landedAge;    // sword only: seconds since it landed (drives mesh fade + impact rays/light, then despawn)
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

// Allocate + launch a NEW sphere from `origin` (e.g. the wand tip), along the look
// direction, at the fixed launch speed (accelerates along its heading).
void fireWizardAttack(player& player, vector3 origin);

// Heaven Sword: spawn a sword high above the point the player is looking at (1000
// units up), with gravity enabled so it plummets straight down onto that spot.
void fireHeavenSword(player& player);

// Darkblast: raycast forward (50 units or to the nearest collider) and spawn a burst of
// vertical beams (white centre -> ultraviolet -> red) at the hit, plus a purple point
// light above the player. `mesh` is an unused placeholder for now. Beams/light last 1.5s
// then fade out by 2s.
void fireDarkblast(player& player, meshedObject& mesh);
// Purple darkblast SEGMENT light: the whole beam emits (the shader lights from the closest
// point on start..end). count is 0 or 1; *At fills start[3]/end[3]/color[3]/radius.
int  darkblastBeamCount();
bool darkblastBeamAt(int i, float* start, float* end, float* color, float* radius);
// Visible darkblast glow for the renderer's corona pass: beam segment + glow colour +
// per-corona radius + spacing to string coronas along it. False when inactive.
bool darkblastGlow(float* start, float* end, float* color, float* radius, float* spacing);

// Sword-impact shockwave sphere(s), for the renderer's screen-space heat-warp pass. count is
// 0 or 1; *At fills origin[3] (world centre), *radius (world), *strength (0..1 envelope).
int  shockwaveCount();
bool shockwaveAt(int i, float* origin, float* radius, float* strength);

// Advance every live projectile (physics + lifetime); frees any that expired.
void updateWizardAttacks(float deltaTime, world& worldInstance, player& player);

// Render all live projectiles: main lit pass and shadow depth pass.
void drawWizardAttacks(const camera& cam, shaderStore& shader, float alpha, const mtx44* vp,
                       const Texture2D* shadowTex, bool receiveShadows, const Texture2D* shadowTexFar);
void drawWizardAttacksDepth(Shader depthShader, int modelLoc);

// Live projectiles, exposed so the corona/glow pass can emit from each ball.
int wizardBallCount();
meshedObject* wizardBallAt(int i);

// Guide lights: one per live heaven sword, floating a fixed distance BELOW the falling
// blade (maintained until impact). Fed into the renderer's point-light pass, like the
// wand glow / explosion lights. *At fills pos[3]/color[3]/radius; false past the end.
int  swordLightCount();
bool swordLightAt(int i, float* pos, float* color, float* radius);

// Free all remaining live projectiles (call at shutdown).
void shutdownWizardAttacks();
