#include "attack_wizard.h"
#include "../physics/physics.h"
#include "../draw/draw.h"
#include "../particles/particles.h"
#include "../system/audio3d.h"

#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

// Defined in main.cpp (external linkage). Used ONCE at startup to build the shared
// sphere template; projectiles are cheap copies of it, not fresh loads.
void create3dObject(meshedObject& object, const char* path, const char* colliderPath,
                    bool collider, shaderStore& shader, float scale, vector3 location,
                    char text[100], bool renderText, float textRenderDistance,
                    vector3 relativeTextOffset);

namespace {

const float   ATTACK_SPEED    = 12.5f;              // initial launch speed (half; it accelerates)
// Accelerate along the travel direction so the fireball is going twice its initial
// speed after ACCEL_DIST units. From v^2 = v0^2 + 2 a d with v = 2 v0: a = 3 v0^2 / (2 d).
const float   ACCEL_DIST      = 5.0f;
const float   ATTACK_ACCEL    = 3.0f * ATTACK_SPEED * ATTACK_SPEED / (2.0f * ACCEL_DIST);
const vector3 ATTACK_GRAVITY  = {0.f, -20.5f, 0.f}; // identical to the player's gravity
const float   ATTACK_LIFETIME = 8.0f;               // seconds before the instance is unloaded
const float   ATTACK_DAMAGE   = 10.0f;
const float   ATTACK_SCALE    = 1.0f;               // sphere.obj scale (tune to taste)

// Heaven Sword: a mesh projectile that spawns high above the looked-at spot and falls.
const float   SWORD_SCALE        = 1.0f;            // heaven_sword.obj scale (tune to taste)
// The sword falls under its OWN (strong) gravity so it plummets fast. Spawn height is
// derived from that gravity and the target fall time so it still lands exactly as the
// heaven_calls announce ends: from rest, h = ½·g·t². Bump SWORD_GRAVITY_MAG to make it
// faster — the height auto-scales to keep the landing on the sound.
const float   SWORD_GRAVITY_MAG  = 120.0f;         // downward accel (units/s²); ~6× player gravity -> hard plummet
const vector3 SWORD_GRAVITY      = {0.f, -SWORD_GRAVITY_MAG, 0.f};
const float   SWORD_FALL_TIME    = 5.52f;          // heaven_calls.wav duration (land as it ends)
const float   SWORD_SPAWN_HEIGHT = 0.5f * SWORD_GRAVITY_MAG * SWORD_FALL_TIME * SWORD_FALL_TIME; // ≈ 914
const float   SWORD_AHEAD_DIST   = 40.f;           // when looking level/up: aim this far ahead at ground height
const float   SWORD_LIFETIME     = 12.0f;          // comfortably longer than the ~5.5s fall (falling phase)
const float   SWORD_DAMAGE       = 40.0f;
const float   SWORD_FACE_YAW_OFFSET = PI * 0.5f;   // face the player, then rotate 90° about vertical

// Landed phase: the sword sits, then fades out and despawns.
const float   SWORD_LAND_LIFETIME = 5.0f;          // total seconds embedded before despawn + free
const float   SWORD_FADE_START    = 3.5f;          // hold fully opaque until here, then fade to 0 by LAND_LIFETIME

// Impact: a smoke-only blast (2/3 the fireball radius) plus fire-coloured rays that shoot
// UP from the strike and emit an orangish-red light. No white lights / corona.
const float   SWORD_BLAST_SCALE = 2.5f * (2.0f / 3.0f); // fireball uses 2.5; sword = 2/3 of that
const float   SWORD_RAY_SCALE   = 1.5f;                 // size of the ray burst

// The rays emit an orangish-red light (fire): brightest at impact, gone by RAY_LIGHT_LIFE.
const float   RAY_LIGHT_LIFE      = 3.0f;              // matches the rays' fade
const vector3 RAY_LIGHT_COLOR     = {1.0f, 0.35f, 0.10f}; // orangish-red, like fire
const float   RAY_LIGHT_INTENSITY = 9.0f;
const float   RAY_LIGHT_RADIUS    = 45.0f;
const float   RAY_LIGHT_UP        = 4.0f;             // lift it off the ground so it isn't grazing

// Red light UNDER the sword while it falls: underlights the blade and casts a red warning
// glow on the ground it's plunging toward.
const vector3 FALL_LIGHT_COLOR     = {1.0f, 0.05f, 0.03f}; // red
const float   FALL_LIGHT_INTENSITY = 10.0f;
const float   FALL_LIGHT_RADIUS    = 45.0f;
const float   FALL_LIGHT_BELOW     = 8.0f;            // units below the falling sword

// Sword-impact SHOCKWAVE: an expanding spherical collider drawn as a smoke shell with a
// screen-space heat-warp (refraction) over it. The sphere grows fast from the strike point;
// when its surface sweeps past the player it prints "shockwave" (once). It expands to
// SHOCK_MAX_RADIUS, then fades out.
const float   SHOCK_SPEED       = 220.0f;             // radius growth (units/s) — expands quickly
const float   SHOCK_MAX_RADIUS  = 150.0f;             // fully expanded, then it fades
const float   SHOCK_FADE_TIME   = 0.35f;              // seconds to linger/fade after reaching max radius
const float   SHOCK_EMIT_STEP   = 3.5f;               // spawn a fresh smoke shell every this many units of growth (tight = dense wall)
const float   SWORD_CLOUD_SCALE = 22.0f;              // radius (world units) of the huge quick-dissipating impact smoke cloud

// Darkblast: a raycast beam attack. Beams spawn at the ray's hit point; a purple light
// rides above the player. Both hold, then fade over the last quarter of DARK_LIFE.
const float   DARK_RANGE       = 100.0f;  // raycast reach (or the nearer collider)
const float   DARK_BEAM_SCALE  = 1.5f;    // size of the beam burst
const float   DARK_ORIGIN_UP   = 3.5f;    // beams start this far above the player's feet (centre mass)
const float   DARK_LIFE        = 2.0f;    // full until DARK_HOLD, then fade to 0 by here
const float   DARK_HOLD        = 1.5f;    // instant/full until this age
const vector3 DARK_LIGHT_COLOR = {0.6f, 0.1f, 1.0f}; // purple
const float   DARK_LIGHT_INTENSITY = 4.0f;  // segment-light brightness (dim)
const float   DARK_LIGHT_RADIUS    = 18.0f; // light reach around the beam

// Visible glow (drawn via the renderer's real glow/corona pass, not a widened sprite):
// soft coronas strung along the beam. Fades with the beam.
const vector3 DARK_GLOW_COLOR   = {0.65f, 0.2f, 1.0f}; // bright purple
const float   DARK_GLOW_BRIGHT  = 2.2f;   // additive corona brightness
const float   DARK_GLOW_RADIUS  = 4.0f;   // per-corona world radius (hugs the beam)
const float   DARK_GLOW_SPACING = 1.0f;   // units between coronas along the beam

const float   HEAVEN_CALL_VOLUME  = 1.0f;   // announce sound plays loud at the strike spot
const float   HEAVEN_CALL_MAXDIST = 220.f;  // audible even when the strike is far off

// Positional audio (Option A): each shot carries a looping travel sound that follows
// the ball; a one-shot impact sound plays at the detonation point.
const float   TRAVEL_VOLUME   = 0.6f;   // travel loop base volume (at the source)
const float   TRAVEL_MAXDIST  = 80.f;   // travel loop fades to silence by here
const float   IMPACT_VOLUME   = 1.0f;   // impact base volume (at the source)
const float   IMPACT_MAXDIST  = 140.f;  // explosions carry farther than the travel hiss

std::vector<physicalAttack*> gAttacks;  // live projectiles, one heap instance each
meshedObject gTemplate;                 // sphere loaded ONCE (disk + GPU upload)
meshedObject gSwordTemplate;            // heaven_sword loaded ONCE (disk + GPU upload)
bool gTemplateLoaded = false;

Sound gTravelSound = {0};  // source sample; each shot plays its own alias of this
Sound gImpactSound = {0};  // fireball impact source sample (played only via the alias pool below)
const int IMPACT_POOL = 6;              // how many impacts can overlap before one cuts off
Sound gImpactPool[IMPACT_POOL] = {0};   // round-robin aliases so overlapping booms don't clip
int   gImpactCursor = 0;

Sound gSwordImpactSound = {0};             // heaven sword impact (darkblast)
Sound gSwordImpactPool[IMPACT_POOL] = {0}; // round-robin aliases for overlapping sword impacts
int   gSwordImpactCursor = 0;
Sound gHeavenCallSound = {0};              // announce played at the strike spot when a sword is cast
const int HEAVEN_POOL = 4;
Sound gHeavenCallPool[HEAVEN_POOL] = {0};
int   gHeavenCallCursor = 0;

Sound gDarkblastSound = {0};               // played at the beam's endpoint on a darkblast

bool  gSoundsLoaded = false;

world* gWorld = nullptr; // cached each frame (updateWizardAttacks) so fireDarkblast can raycast
// Purple darkblast beam light: a single SEGMENT light (start..end); the shader lights each
// fragment from the closest point on it, so the whole beam emits. Fades with the effect.
struct { bool active; float age; vector3 start; vector3 end; } gDarkLight =
    { false, 0.f, {0.f, 0.f, 0.f}, {0.f, 0.f, 0.f} };

// Sword-impact shockwave: an expanding spherical collider (origin + growing radius) drawn as a
// smoke shell with a screen-space heat-warp over it. hitPlayer latches so the "shockwave" contact
// print fires once; emittedTo throttles shell spawns as the sphere sweeps outward; fadeAge times
// the linger after full size.
struct {
    bool    active;
    vector3 origin;
    float   radius;
    float   emittedTo;
    float   fadeAge;
    bool    hitPlayer;
} gShock = { false, {0.f, 0.f, 0.f}, 0.f, 0.f, 0.f, false };

// Spawn a smoke shell on the sphere of `radius` around `origin`, giving the shockwave its
// expanding smoke wall. The screen-space heat-warp pass (main.cpp) refracts over it.
void spawnShockShell(vector3 origin, float radius) {
    spawnShockSmoke(origin, radius, 1.0f);
}

// Build a projectile by copying the shared template — malloc/memcpy only, no disk
// read, no OBJ parse, no GPU upload. The mesh geometry (trisO), collider, and the
// scratch tris buffer are per-instance so instances never stomp each other; the
// GPU texture is SHARED (its id is just copied), so it is not unloaded per shot.
physicalAttack* instantiateFromTemplate(const meshedObject& tmpl) {
    physicalAttack* a = new physicalAttack();
    a->obj = tmpl; // shallow copy: scalars, texo id, emissive/bloom, and pointers

    int triCount = tmpl.mesh.count;
    a->obj.mesh.trisO = (tri*)malloc(triCount * sizeof(tri));
    a->obj.mesh.tris  = (tri*)malloc(triCount * sizeof(tri));
    memcpy(a->obj.mesh.trisO, tmpl.mesh.trisO, triCount * sizeof(tri));
    memcpy(a->obj.mesh.tris,  tmpl.mesh.tris,  triCount * sizeof(tri));

    int planeCount = tmpl.cPlaneCount;
    a->obj.collider  = new planeMtx[planeCount];
    a->obj.colliderO = new planeMtx[planeCount];
    memcpy(a->obj.collider,  tmpl.collider,  planeCount * sizeof(planeMtx));
    memcpy(a->obj.colliderO, tmpl.colliderO, planeCount * sizeof(planeMtx));

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

// Yaw the sword so its front (local +Z -> world (sin yaw, 0, cos yaw)) points at the
// player. Called each tick while falling, so it lands facing the player.
void faceSwordAtPlayer(physicalAttack* a, const player& p) {
    const vector3& L = a->obj.pEntity.location;
    float dx = p.pEntity.location.x - L.x;
    float dz = p.pEntity.location.z - L.z;
    if (dx * dx + dz * dz > 1e-6f)
        a->obj.pEntity.rot.y = atan2f(dx, dz) + SWORD_FACE_YAW_OFFSET;
}

// Landed-sword opacity/brightness 0..1: full until SWORD_FADE_START, then linear to 0
// at SWORD_LAND_LIFETIME. Shared by the mesh fade, the impact light, and the corona.
float landedFade(float age) {
    if (age <= SWORD_FADE_START) return 1.f;
    if (age >= SWORD_LAND_LIFETIME) return 0.f;
    return 1.f - (age - SWORD_FADE_START) / (SWORD_LAND_LIFETIME - SWORD_FADE_START);
}

// Darkblast 0..1: full until DARK_HOLD, then linear to 0 at DARK_LIFE (beams + light).
float darkFade(float age) {
    if (age <= DARK_HOLD) return 1.f;
    if (age >= DARK_LIFE) return 0.f;
    return 1.f - (age - DARK_HOLD) / (DARK_LIFE - DARK_HOLD);
}

} // namespace

void initWizardAttacks(shaderStore& shader) {
    if (gTemplateLoaded) return;
    char empty[100] = "";
    // The one and only load of sphere.obj + its collider (disk + GPU upload).
    create3dObject(gTemplate, "resources/levels/sphere.obj",
                   "resources/levels/sphere_collider.obj", true, shader,
                   ATTACK_SCALE, {0.f, 0.f, 0.f}, empty, false, 100, {0.f, 0.f, 0.f});
    gTemplate.emissive = {1.f, 0.45f, 0.12f, 0.30f}; // orange fireball
    gTemplate.bloom = {5.0f, 2.5f};                   // small corona on the projectile (y = world radius); the big growth/decay glow is at impact

    // Heaven sword: loaded ONCE, same as the sphere. Its own mesh IS drawn (drawMesh),
    // so no emissive/corona is set — it's a solid falling blade, not a glowing orb.
    create3dObject(gSwordTemplate, "resources/levels/objects/heaven_sword.obj",
                   "resources/levels/objects/colliders/heaven_sword_collider.obj", true, shader,
                   SWORD_SCALE, {0.f, 0.f, 0.f}, empty, false, 100, {0.f, 0.f, 0.f});
    gTemplateLoaded = true;

    // Projectile audio: a sustained hiss that follows the ball, and an impact hit.
    // (InitAudioDevice has already run by the time initWizardAttacks is called.)
    gTravelSound = LoadSound("resources/sounds/magic/wind-howl-01.wav");
    gImpactSound = LoadSound("resources/sounds/magic/fireball.wav");
    for (int i = 0; i < IMPACT_POOL; i++) gImpactPool[i] = LoadSoundAlias(gImpactSound);

    // Heaven sword audio: rock-breaking on impact, plus an announce at the strike spot.
    gSwordImpactSound = LoadSound("resources/sounds/magic/rock_breaking.wav");
    for (int i = 0; i < IMPACT_POOL; i++) gSwordImpactPool[i] = LoadSoundAlias(gSwordImpactSound);
    gHeavenCallSound = LoadSound("resources/sounds/magic/heaven_calls.wav");
    for (int i = 0; i < HEAVEN_POOL; i++) gHeavenCallPool[i] = LoadSoundAlias(gHeavenCallSound);
    gDarkblastSound = LoadSound("resources/sounds/magic/darkblast.wav");

    gSoundsLoaded = true;
}

void fireWizardAttack(player& player, vector3 origin) {
    if (!gTemplateLoaded) return;

    physicalAttack* a = instantiateFromTemplate(gTemplate);
    a->drawMesh = false; // fireball body is the particle core, not the sphere mesh
    a->isSword  = false;

    initializePhysicsEntity(a->obj.pEntity, 1.f, COMPLEX);
    /* applyAcceleration(ATTACK_GRAVITY, a->obj.pEntity); // fireball: no gravity, flies straight */

    // Launch from `origin` (the wand tip), along the look direction, at the fixed
    // speed. camTarget stores yaw (.x) and pitch (.y) angles, NOT a world point —
    // build the same forward vector the view matrix uses (see viewMtx44) so the ball
    // goes exactly where the crosshair points.
    float cy = cosf(player.camera.camTarget.x), sy = sinf(player.camera.camTarget.x);
    float cp = cosf(player.camera.camTarget.y), sp = sinf(player.camera.camTarget.y);
    vector3 dir = normalize3({cp * cy, sp, cp * sy});
    a->obj.pEntity.location  = origin;
    a->obj.pEntity.magnitude = dir.fmult(ATTACK_SPEED);
    applyAcceleration(dir.fmult(ATTACK_ACCEL), a->obj.pEntity); // speeds up along its travel dir
    a->prevLoc = origin; // seed interpolation so the first frame doesn't lerp from origin
    updateColliderLocation(a->obj, player, false); // position the collider at the spawn

    a->lifetime = ATTACK_LIFETIME;
    a->damage   = ATTACK_DAMAGE;
    a->emitterId = spawnCoreEmitter(origin); // tight fire cluster = the (invisible-mesh) fireball body

    // Own alias so this shot's travel sound pans independently of other projectiles.
    a->hasTravelSound = false;
    if (gSoundsLoaded) {
        a->travelSound = LoadSoundAlias(gTravelSound);
        a->hasTravelSound = true;
        updatePositional(a->travelSound, origin, TRAVEL_VOLUME, TRAVEL_MAXDIST);
        PlaySound(a->travelSound);
    }

    gAttacks.push_back(a);
}

void fireHeavenSword(player& player) {
    if (!gTemplateLoaded) return;

    // Find the spot the player is looking at, on a horizontal plane at foot height.
    float cy = cosf(player.camera.camTarget.x), sy = sinf(player.camera.camTarget.x);
    float cp = cosf(player.camera.camTarget.y), sp = sinf(player.camera.camTarget.y);
    vector3 dir = normalize3({cp * cy, sp, cp * sy});
    vector3 eye = player.camera.camPos;
    float groundY = player.pEntity.location.y; // player's feet ≈ ground level

    vector3 target;
    if (dir.y < -0.05f) {                       // looking down: ray meets the ground plane
        float t = (groundY - eye.y) / dir.y;    // t > 0 (both numerator and dir.y negative)
        target = { eye.x + dir.x * t, groundY, eye.z + dir.z * t };
    } else {                                    // looking level/up: aim a fixed distance ahead
        vector3 flat = normalize3({dir.x, 0.f, dir.z});
        target = { eye.x + flat.x * SWORD_AHEAD_DIST, groundY, eye.z + flat.z * SWORD_AHEAD_DIST };
    }

    // Spawn high above the target and let gravity drop it straight down onto that spot.
    vector3 origin = { target.x, target.y + SWORD_SPAWN_HEIGHT, target.z };

    physicalAttack* a = instantiateFromTemplate(gSwordTemplate);
    a->drawMesh  = true; // the sword's own mesh is rendered
    a->isSword   = true;
    a->landed    = false;
    a->landedAge = 0.f;

    // Announce: play the heaven call LOUDLY at the strike spot as the sword is cast.
    if (gSoundsLoaded) {
        setAudioListener(player.camera.camPos, player.camera.camTarget.x, player.camera.camTarget.y);
        playPositional(gHeavenCallPool[gHeavenCallCursor], target, HEAVEN_CALL_VOLUME, HEAVEN_CALL_MAXDIST);
        gHeavenCallCursor = (gHeavenCallCursor + 1) % HEAVEN_POOL;
    }

    initializePhysicsEntity(a->obj.pEntity, 1.f, COMPLEX);
    applyAcceleration(SWORD_GRAVITY, a->obj.pEntity); // strong gravity: plummets fast
    a->obj.pEntity.location  = origin;
    a->obj.pEntity.magnitude = {0.f, 0.f, 0.f};        // no launch velocity: pure vertical drop
    a->prevLoc = origin;
    faceSwordAtPlayer(a, player); // face the player from the start (kept updated as it falls)
    updateColliderLocation(a->obj, player, false);

    a->lifetime = SWORD_LIFETIME;
    a->damage   = SWORD_DAMAGE;
    a->emitterId = -1; // no fire trail on the sword

    // Falling-blade travel sound, panned to the sword as it drops.
    a->hasTravelSound = false;
    if (gSoundsLoaded) {
        a->travelSound = LoadSoundAlias(gTravelSound);
        a->hasTravelSound = true;
        updatePositional(a->travelSound, origin, TRAVEL_VOLUME, TRAVEL_MAXDIST);
        PlaySound(a->travelSound);
    }

    gAttacks.push_back(a);
}

void fireDarkblast(player& player, meshedObject& mesh) {
    (void)mesh; // empty for now — placeholder for a future beam mesh

    // Raycast forward from the eye: reach DARK_RANGE units, or stop at the nearest collider.
    float cy = cosf(player.camera.camTarget.x), sy = sinf(player.camera.camTarget.x);
    float cp = cosf(player.camera.camTarget.y), sp = sinf(player.camera.camTarget.y);
    vector3 dir = normalize3({cp * cy, sp, cp * sy});
    vector3 origin = player.camera.camPos;
    float dist = gWorld ? raycastWorld(origin, dir, DARK_RANGE, *gWorld) : DARK_RANGE;
    vector3 hit = origin + dir.fmult(dist);

    // Beams are drawn from the caster's centre mass to the hit point.
    vector3 centerMass = { player.pEntity.location.x,
                           player.pEntity.location.y + DARK_ORIGIN_UP,
                           player.pEntity.location.z };
    spawnDarkblast(centerMass, hit, DARK_BEAM_SCALE);

    // darkblast.wav positioned at the middle of the beam.
    if (gSoundsLoaded) {
        vector3 mid = (centerMass + hit).fmult(0.5f);
        setAudioListener(player.camera.camPos, player.camera.camTarget.x, player.camera.camTarget.y);
        playPositional(gDarkblastSound, mid, IMPACT_VOLUME, IMPACT_MAXDIST);
    }

    // Purple segment light spanning the whole beam (start -> hit).
    gDarkLight.active = true;
    gDarkLight.age = 0.f;
    gDarkLight.start = centerMass;
    gDarkLight.end = hit;
}

// Purple darkblast beam light: a single SEGMENT light (the whole beam emits). count is
// 0 or 1. *At fills start[3]/end[3]/color[3]/radius for the active beam.
int darkblastBeamCount() { return gDarkLight.active ? 1 : 0; }

bool darkblastBeamAt(int i, float* start, float* end, float* color, float* radius) {
    if (!gDarkLight.active || i != 0) return false;
    float k = DARK_LIGHT_INTENSITY * darkFade(gDarkLight.age);
    start[0] = gDarkLight.start.x; start[1] = gDarkLight.start.y; start[2] = gDarkLight.start.z;
    end[0]   = gDarkLight.end.x;   end[1]   = gDarkLight.end.y;   end[2]   = gDarkLight.end.z;
    color[0] = DARK_LIGHT_COLOR.x * k;
    color[1] = DARK_LIGHT_COLOR.y * k;
    color[2] = DARK_LIGHT_COLOR.z * k;
    *radius = DARK_LIGHT_RADIUS;
    return true;
}

// Visible darkblast glow for the renderer's soft-corona pass: the beam segment plus a glow
// colour, per-corona radius, and the spacing to string coronas along it. False when inactive.
bool darkblastGlow(float* start, float* end, float* color, float* radius, float* spacing) {
    if (!gDarkLight.active) return false;
    float k = DARK_GLOW_BRIGHT * darkFade(gDarkLight.age);
    start[0] = gDarkLight.start.x; start[1] = gDarkLight.start.y; start[2] = gDarkLight.start.z;
    end[0]   = gDarkLight.end.x;   end[1]   = gDarkLight.end.y;   end[2]   = gDarkLight.end.z;
    color[0] = DARK_GLOW_COLOR.x * k;
    color[1] = DARK_GLOW_COLOR.y * k;
    color[2] = DARK_GLOW_COLOR.z * k;
    *radius  = DARK_GLOW_RADIUS;
    *spacing = DARK_GLOW_SPACING;
    return true;
}

// Active sword-impact shockwave sphere(s), fed to the renderer's screen-space heat-warp pass.
// count is 0 or 1; *At fills origin[3] (world centre), *radius (world), *strength (0..1 envelope,
// full while expanding, ramping to 0 over the post-expansion fade).
int shockwaveCount() { return gShock.active ? 1 : 0; }
bool shockwaveAt(int i, float* origin, float* radius, float* strength) {
    if (!gShock.active || i != 0) return false;
    origin[0] = gShock.origin.x; origin[1] = gShock.origin.y; origin[2] = gShock.origin.z;
    *radius = gShock.radius;
    float s = (gShock.radius >= SHOCK_MAX_RADIUS) ? (1.f - gShock.fadeAge / SHOCK_FADE_TIME) : 1.f;
    *strength = s < 0.f ? 0.f : (s > 1.f ? 1.f : s);
    return true;
}

void updateWizardAttacks(float deltaTime, world& worldInstance, player& player) {
    // Listener = the player's ears, refreshed each frame so pan/attenuation track the
    // camera. camTarget stores yaw (.x) / pitch (.y) angles.
    setAudioListener(player.camera.camPos, player.camera.camTarget.x, player.camera.camTarget.y);

    gWorld = &worldInstance; // cache for fireDarkblast's raycast

    // Age the purple darkblast beam light.
    if (gDarkLight.active) {
        gDarkLight.age += deltaTime;
        if (gDarkLight.age >= DARK_LIFE) gDarkLight.active = false;
    }

    // Advance the sword-impact shockwave: grow the spherical collider, drive the screen warp,
    // and print once when the surface sweeps past the player.
    if (gShock.active) {
        if (gShock.radius < SHOCK_MAX_RADIUS) {
            gShock.radius += SHOCK_SPEED * deltaTime;
            if (gShock.radius > SHOCK_MAX_RADIUS) gShock.radius = SHOCK_MAX_RADIUS;
            /*
            while (gShock.emittedTo + SHOCK_EMIT_STEP <= gShock.radius) {
                gShock.emittedTo += SHOCK_EMIT_STEP;
                spawnShockShell(gShock.origin, gShock.emittedTo);
            }
            */
            if (!gShock.hitPlayer &&
                gShock.radius >= (player.pEntity.location - gShock.origin).mag()) {
                gShock.hitPlayer = true;
                std::cout << "shockwave" << std::endl;
            }
        } else {
            gShock.fadeAge += deltaTime;
            if (gShock.fadeAge >= SHOCK_FADE_TIME) gShock.active = false;
        }
    }

    bool end = false;
    int target = 0;
    for (size_t i = 0; i < gAttacks.size();) {
        physicalAttack* a = gAttacks[i];
        if (a->landed) {
            // Frozen in the ground: age it, then despawn + free once fully faded. The
            // impact rays + their light are emitted from the particle system / swordLightAt.
            a->landedAge += deltaTime;
            if (a->landedAge >= SWORD_LAND_LIFETIME) {
                releaseEmitter(a->emitterId);
                freeAttack(a);
                gAttacks.erase(gAttacks.begin() + i);
            } else i++;
            continue;
        }

        a->prevLoc = a->obj.pEntity.location; // remember this tick's start for interpolation
        updateColliderLocation(a->obj, player, false);
        bool hit = false;
        vector3 hitPoint = {0.f, 0.f, 0.f};
        processPhysics(deltaTime, 0, a->obj.pEntity, worldInstance, end, target,
                       false, true, a->obj.collider, a->obj.cPlaneCount, &hit, &hitPoint);
        moveFireEmitter(a->emitterId, a->obj.pEntity.location); // trail follows the ball
        if (a->isSword) faceSwordAtPlayer(a, player); // keep facing the player as it falls

        // Travel sound rides the ball: re-pan/attenuate, and re-trigger to loop (Sound
        // has no loop flag, so replay it whenever the last instance has finished).
        if (a->hasTravelSound) {
            updatePositional(a->travelSound, a->obj.pEntity.location, TRAVEL_VOLUME, TRAVEL_MAXDIST);
            if (!IsSoundPlaying(a->travelSound)) PlaySound(a->travelSound);
        }

        a->lifetime -= deltaTime;

        // Heaven sword strikes the ground: play the impact, then FREEZE and KEEP it —
        // it stays embedded, facing the player, and is never freed by this update.
        if (a->isSword && hit) {
            vector3 boom = hitPoint;
            if (a->hasTravelSound) { StopSound(a->travelSound); UnloadSoundAlias(a->travelSound); a->hasTravelSound = false; }
            spawnSmokeBlast(boom, SWORD_BLAST_SCALE);  // smoke-only blast at 2/3 the fireball radius
            spawnSmokeCloud(boom, SWORD_CLOUD_SCALE);   // huge cloud that blooms and clears fast
            spawnImpactRays(boom, SWORD_RAY_SCALE);    // fire-coloured rays shooting up from the strike
            gShock = { true, boom, 0.f, 0.f, 0.f, false }; // expanding sphere shockwave from the strike
            if (gSoundsLoaded) {
                playPositional(gSwordImpactPool[gSwordImpactCursor], boom, IMPACT_VOLUME, IMPACT_MAXDIST);
                gSwordImpactCursor = (gSwordImpactCursor + 1) % IMPACT_POOL;
            }
            // Freeze in place, start the landed clock. The impact rays + their orange-red
            // light are emitted (particles + swordLightAt) while landedAge < RAY_LIGHT_LIFE.
            a->landed = true;
            a->landedAge = 0.f;
            a->obj.pEntity.magnitude    = {0.f, 0.f, 0.f}; // freeze: no residual velocity / accel
            a->obj.pEntity.applyAccel   = {0.f, 0.f, 0.f};
            a->obj.pEntity.acceleration = {0.f, 0.f, 0.f};
            a->prevLoc = a->obj.pEntity.location;          // so render interpolation doesn't drift
            i++;
            continue;
        }

        if (hit || a->lifetime <= 0.f) {
            vector3 boom = hit ? hitPoint : a->obj.pEntity.location;
            if (a->hasTravelSound) { StopSound(a->travelSound); UnloadSoundAlias(a->travelSound); a->hasTravelSound = false; }

            // Sword that expired mid-air (fell into the void, never hit): remove quietly.
            if (!a->isSword) {
                // Fireball detonation: full flash + smoke + fire + transient light.
                spawnExplosion(boom, 2.5f);
                if (gSoundsLoaded) { // one-shot at the impact point, round-robin so overlaps don't clip
                    playPositional(gImpactPool[gImpactCursor], boom, IMPACT_VOLUME, IMPACT_MAXDIST);
                    gImpactCursor = (gImpactCursor + 1) % IMPACT_POOL;
                }
            }

            releaseEmitter(a->emitterId); // stop the trail (its live particles finish on their own)
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
        // Fireballs are drawn as particles (mesh stays invisible); the heaven sword
        // renders its actual mesh.
        if (a->drawMesh) {
            // A landed sword fades out over its final phase; draw it alpha-blended with
            // the vertex-colour alpha the shader now honours. Falling/opaque: alpha 255.
            float av = a->landed ? landedFade(a->landedAge) * 255.f : 255.f;
            bool  blend = av < 255.f;
            if (blend) BeginBlendMode(BLEND_ALPHA);
            Draw3DGPU(a->obj, cam, shader, {255.f, 255.f, 255.f, av}, vp, shadowTex, receiveShadows, shadowTexFar);
            if (blend) EndBlendMode();
        }
        a->obj.pEntity.location = real; // restore the true physics state for the next tick
    }
}

void drawWizardAttacksDepth(Shader depthShader, int modelLoc) {
    for (physicalAttack* a : gAttacks)
        Draw3DDepthGPU(a->obj, depthShader, modelLoc);
}

int wizardBallCount() { return (int)gAttacks.size(); }
meshedObject* wizardBallAt(int i) { return &gAttacks[i]->obj; }

// One light per sword: while FALLING, a red light under the blade; once LANDED, the
// orangish-red ray light (brightest at impact, gone by RAY_LIGHT_LIFE). A landed sword
// past that emits nothing.
static bool swordHasLight(const physicalAttack* a) {
    return a->isSword && (!a->landed || a->landedAge < RAY_LIGHT_LIFE);
}

int swordLightCount() {
    int n = 0;
    for (physicalAttack* a : gAttacks) if (swordHasLight(a)) n++;
    return n;
}

bool swordLightAt(int i, float* pos, float* color, float* radius) {
    int n = 0;
    for (physicalAttack* a : gAttacks) {
        if (!swordHasLight(a)) continue;
        if (n == i) {
            const vector3& L = a->obj.pEntity.location;
            if (!a->landed) {                             // RED light under the falling sword
                pos[0] = L.x; pos[1] = L.y - FALL_LIGHT_BELOW; pos[2] = L.z;
                color[0] = FALL_LIGHT_COLOR.x * FALL_LIGHT_INTENSITY;
                color[1] = FALL_LIGHT_COLOR.y * FALL_LIGHT_INTENSITY;
                color[2] = FALL_LIGHT_COLOR.z * FALL_LIGHT_INTENSITY;
                *radius = FALL_LIGHT_RADIUS;
            } else {                                      // orangish-red ray light at the impact
                float f = 1.f - a->landedAge / RAY_LIGHT_LIFE; f *= f; // brightest at impact -> 0 at 3s
                float k = RAY_LIGHT_INTENSITY * f;
                pos[0] = L.x; pos[1] = L.y + RAY_LIGHT_UP; pos[2] = L.z;
                color[0] = RAY_LIGHT_COLOR.x * k;
                color[1] = RAY_LIGHT_COLOR.y * k;
                color[2] = RAY_LIGHT_COLOR.z * k;
                *radius = RAY_LIGHT_RADIUS;
            }
            return true;
        }
        n++;
    }
    return false;
}

void shutdownWizardAttacks() {
    for (physicalAttack* a : gAttacks) {
        if (a->hasTravelSound) { StopSound(a->travelSound); UnloadSoundAlias(a->travelSound); a->hasTravelSound = false; }
        releaseEmitter(a->emitterId);
        freeAttack(a);
    }
    gAttacks.clear();

    if (gSoundsLoaded) {
        for (int i = 0; i < IMPACT_POOL; i++) UnloadSoundAlias(gImpactPool[i]);
        for (int i = 0; i < IMPACT_POOL; i++) UnloadSoundAlias(gSwordImpactPool[i]);
        for (int i = 0; i < HEAVEN_POOL; i++) UnloadSoundAlias(gHeavenCallPool[i]);
        UnloadSound(gTravelSound);
        UnloadSound(gImpactSound);
        UnloadSound(gSwordImpactSound);
        UnloadSound(gHeavenCallSound);
        UnloadSound(gDarkblastSound);
        gSoundsLoaded = false;
    }

    if (gTemplateLoaded) {
        if (gTemplate.texo.id > 1) UnloadTexture(gTemplate.texo);
        free(gTemplate.mesh.tris);
        free(gTemplate.mesh.trisO);
        delete[] gTemplate.collider;
        delete[] gTemplate.colliderO;

        if (gSwordTemplate.texo.id > 1) UnloadTexture(gSwordTemplate.texo);
        free(gSwordTemplate.mesh.tris);
        free(gSwordTemplate.mesh.trisO);
        delete[] gSwordTemplate.collider;
        delete[] gSwordTemplate.colliderO;

        gTemplateLoaded = false;
    }
}
