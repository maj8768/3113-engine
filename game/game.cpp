#include "../util.h"
#include "raylib.h"
#include "../physics/physics.h"
#include "../system/keyboard/keyboard.h"
#include "../draw/gui.h"
#include "../draw/draw.h"
#include "game.h"
#include "item.h" // heldItemObject: spells and held lights gate on what is held
#include "attack_wizard.h"
#include "../particles/particles.h"
#include "../server/server_util.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdlib>

static meshedObject gDarkblastMesh = {}; // empty placeholder passed to fireDarkblast for now

Sound sndShopIntro = {0};
Sound sndShopBye = {0};
Sound sndShopThank = {0};
Sound sndGasIntro = {0};
Sound sndGasPurchase = {0};

void playMenuSound(Sound s) {
    static Sound last = {0};
    if (last.frameCount > 0) StopSound(last);
    PlaySound(s);
    last = s;
}

// Advance all live projectiles (fixed timestep). Firing is handled per-frame in
// spellsUpdate so the charge timeline uses real time.
void wizardCombatUpdate(player& player, float deltaTime, world& worldInstance) {
    updateWizardAttacks(deltaTime, worldInstance, player);
}

// Cast envelope 0..1: exponential gain to full, hold at full, exponential fade.
// Timeline: gain [0,0.10), hold [0.10,0.50) (0.4s), fade [0.50,0.80). The fireball
// launches at 0.50s (end of hold); see spellsUpdate.
static float wandChargeIntensity(float age) {
    const float GAIN = 0.50f, HOLD_END = 0.60f, FADE = 0.30f, K = 4.0f;
    if (age < GAIN)     { float u = age / GAIN;              return (1.f - expf(-K * u)) / (1.f - expf(-K)); }
    if (age < HOLD_END) return 1.f;
    float u = (age - HOLD_END) / FADE;
    if (u >= 1.f) return 0.f;
    return (expf(-K * u) - expf(-K)) / (1.f - expf(-K));
}

// The three spells that exist today, as plain cast functions so they can be
// registered as data. Each takes the held item's tip as its origin; the ones that
// aim from the player instead simply ignore it.
void castFireball(player& p, vector3 origin)    { fireWizardAttack(p, origin); }
void castHeavenSword(player& p, vector3 origin) { (void)origin; fireHeavenSword(p); }
void castDarkblast(player& p, vector3 origin)   { (void)origin; fireDarkblast(p, gDarkblastMesh); }

namespace {

const int MAX_SPELLS = 8;

struct spellSlot {
    int key;
    meshedObject* required; // item that must be held for this spell to fire
    float cooldown;         // configured, seconds
    spellCast cast;
    float cooldownLeft;     // runtime
};

spellSlot gSpells[MAX_SPELLS];
int gSpellCount = 0;

} // namespace

void registerSpell(int key, meshedObject* requiredItem, float cooldown, spellCast cast) {
    if (gSpellCount >= MAX_SPELLS || !cast) return;
    spellSlot& s = gSpells[gSpellCount++];
    s.key = key;
    s.required = requiredItem;
    s.cooldown = cooldown;
    s.cast = cast;
    s.cooldownLeft = 0.f;
}

// Per-frame spell evaluation. Every registered spell is considered every frame and
// the held-item match is the only gate on firing, so giving an item a spell is one
// registerSpell call and nothing else — and an item no spell names (the hand) casts
// nothing without a check anywhere. On a valid cast: play a random magic sound, run
// the SAME charge FX regardless of spell — glow (dim point light) + sparkles at the
// item's tip through a ~0.6s charge — and launch the spell partway through. Only the
// spell and its cooldown differ. castOrigin follows the held item's position/rotation.
void spellsUpdate(player& player, float dt, vector3 castOrigin,
                  Sound m1, Sound m2, Sound m3) {
    static bool  charging   = false;
    static float chargeAge  = 0.f;
    static bool  fired      = false;
    static float sparkAccum = 0.f;
    static int   active     = -1; // index into gSpells of the cast in progress

    const float FIRE_AT    = 0.6f; // projectile launches 0.6s into the cast
    const float CHARGE_END = 0.9f; // glow fully faded by here (wandChargeIntensity)

    vector3 wandTip = castOrigin;
    meshedObject* held = heldItemObject();

    for (int i = 0; i < gSpellCount; i++)
        if (gSpells[i].cooldownLeft > 0.f) gSpells[i].cooldownLeft -= dt;

    for (int i = 0; i < gSpellCount; i++) {
        spellSlot& s = gSpells[i];

        // Poll every spell's key every frame BEFORE any gating, so the edge latches
        // stay current — otherwise a key held down through a cooldown would fire the
        // instant that cooldown expired.
        bool pressed = getKeyPressedOnce(s.key);

        if (charging || !pressed) continue;
        if (!s.required || held != s.required) continue; // wrong item held: not allowed to fire
        if (s.cooldownLeft > 0.f) continue;

        charging = true; chargeAge = 0.f; fired = false; sparkAccum = 0.f;
        active = i;
        s.cooldownLeft = s.cooldown;
        int r = GetRandomValue(0, 2);
        PlaySound(r == 0 ? m1 : r == 1 ? m2 : m3);
    }

    if (charging) {
        if (active < 0 || held != gSpells[active].required) { // dropped mid-cast: cancel
            charging = false;
            setWandGlow(false, wandTip, 0.f);
            return;
        }
        chargeAge += dt;

        // Point light: in front of the camera, centered and close in, so it lights the
        // FRONT of the wand; its small radius (WAND_LIGHT_RADIUS) keeps it off the floor.
        float I = wandChargeIntensity(chargeAge);
        float cyw = cosf(player.camera.camTarget.x), syw = sinf(player.camera.camTarget.x);
        float cpi = cosf(player.camera.camTarget.y), spi = sinf(player.camera.camTarget.y);
        vector3 F = normalize3({cpi * cyw, spi, cpi * syw});
        vector3 lightPos = player.camera.camPos + F.fmult(1.5f); // centered in front of the eye
        setWandGlow(true, lightPos, I);

        // ~15–30 small white / ultralight-blue motes twinkling AROUND the corona.
        const float WISP_RATE = 40.f; // per second; with ~0.6s life -> ~15-30 alive
        if (I > 0.08f) {
            sparkAccum += dt;
            while (sparkAccum >= 1.f / WISP_RATE) {
                sparkAccum -= 1.f / WISP_RATE;
                vector3 j = { wandTip.x + GetRandomValue(-40, 40) * 0.01f,
                              wandTip.y + GetRandomValue(-40, 40) * 0.01f,
                              wandTip.z + GetRandomValue(-40, 40) * 0.01f };
                Color c = GetRandomValue(0, 1) ? (Color){255, 255, 255, 255}
                                               : (Color){200, 225, 255, 255};
                spawnWisp(j, c);
            }
        } else sparkAccum = 0.f;

        // Launch the registered spell partway through the charge (same FX either way).
        if (!fired && chargeAge >= FIRE_AT) {
            fired = true;
            gSpells[active].cast(player, wandTip);
        }

        // End the cast once the glow has faded out.
        if (chargeAge >= CHARGE_END) {
            charging = false;
            setWandGlow(false, wandTip, 0.f);
        }
    }
}

/*
// --- Wand: a holdable item -------------------------------------------------
static bool gWandHeld = false;

// Held-item placement in camera space + pickup range. Tune to the wand model.
static const float WAND_FORWARD      = 2.0f;   // distance in front of the eye
static const float WAND_RIGHT        = 1.0f;   // offset to the right
static const float WAND_UP           = -0.8f;  // offset down
static const float WAND_PICKUP_RANGE = 8.0f;   // must be this close to pick up
// Base orientation tweak. Signs/values depend on the model's default facing;
// adjust so the wand points where you look.
static const vector3 WAND_ROT_OFFSET = {0.f, 0.f, 0.f};

// Fixed-step: gravity + collision, but ONLY while the wand is free (not held).
void updateWandPhysics(player& player, float deltaTime, world& worldInstance, meshedObject& wand) {
    if (gWandHeld) return;
    bool end = false;
    int target = 0;
    updateColliderLocation(wand, player, false);
    processPhysics(deltaTime, 0, wand.pEntity, worldInstance, end, target,
                   false, true, wand.collider, wand.cPlaneCount);
}

// Per-frame: F toggles pick up (when near) / drop. While held, the wand is placed
// in front of the POV and rotated with the camera. Call AFTER the camera position
// is finalised so it tracks the smoothed view.
void updateWandHold(player& player, meshedObject& wand) {
    if (getKeyPressedOnce(KEY_F)) {
        if (gWandHeld) {
            gWandHeld = false;                      // drop: resume gravity from rest
            wand.pEntity.magnitude = {0.f, 0.f, 0.f};
            wand.pEntity.collidingY = false;
        } else {
            float dx = wand.pEntity.location.x - player.camera.camPos.x;
            float dy = wand.pEntity.location.y - player.camera.camPos.y;
            float dz = wand.pEntity.location.z - player.camera.camPos.z;
            if (dx*dx + dy*dy + dz*dz <= WAND_PICKUP_RANGE * WAND_PICKUP_RANGE) {
                gWandHeld = true;
                wand.pEntity.magnitude = {0.f, 0.f, 0.f};
            }
        }
    }

    if (!gWandHeld) return;

    // Camera basis from yaw (camTarget.x) / pitch (camTarget.y) — same forward the
    // view matrix builds (see viewMtx44).
    float cyw = cosf(player.camera.camTarget.x), syw = sinf(player.camera.camTarget.x);
    float cpi = cosf(player.camera.camTarget.y), spi = sinf(player.camera.camTarget.y);
    vector3 F = normalize3({cpi * cyw, spi, cpi * syw});
    vector3 R = normalize3(cross3(F, player.camera.up));
    vector3 U = cross3(R, F);

    wand.pEntity.location = player.camera.camPos
                          + F.fmult(WAND_FORWARD)
                          + R.fmult(WAND_RIGHT)
                          + U.fmult(WAND_UP);
    wand.pEntity.rot = { player.camera.camTarget.y + WAND_ROT_OFFSET.x,
                        -player.camera.camTarget.x + WAND_ROT_OFFSET.y,
                         WAND_ROT_OFFSET.z };
}
*/

static bool locked = false;
static bool interacting = true;
static bool interactingG = true;

void resetLevelLogicState() {
    locked = false;
    interacting = true;
    interactingG = true;
}
static vector3 PumpStorage = {0.f, 0.f, 0.f};

static vector3 drinkoffset = {-79.9136f, -3.f, 25.2414f};
static vector3 gasPumpOffset = {49.3031f, 5.81f, -24.243f};

void holdItem(player& player, meshedObject& item) {
    item.pEntity.location = player.camera.camPos + drinkoffset;
    applyCamRot(item.pEntity.location, vector3{player.camera.camTarget.x, 0.f, 0.f}, 0.f, 0.f, 1.5);

}

void buy(player& player, int buy_guy) {
    std::cout << "Buy menu open" << std::endl;
    guiBuyMenu(SCREEN_WIDTH, SCREEN_HEIGHT, player, buy_guy);
}

void checkBuyBox(player& player, float deltaTime, gameData& gData, meshedObject& buyBox) {

    if (canInteract(player, buyBox.collider, buyBox.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 300.f)) {
        player.pState.canBuy = true;

    }
    else {

        player.pState.canBuy = false;
    }
}

void checkDrank(player& player, float deltaTime, gameData& gData, meshedObject& drankEntity) {

    if (canInteract(player, drankEntity.collider, drankEntity.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f)) {
        player.pState.canPickDrank = true;
    }
    else {
        player.pState.canPickDrank = false;
    }
}

void checkCar(player& player, float deltaTime, gameData& gData, meshedObject& carEntity) {
    if (canInteract(player, carEntity.collider, carEntity.cPlaneCount, 6.0f, GetScreenWidth(), GetScreenHeight(), 125.f)) {
        player.pState.canCar = true;
    }
    else {
        player.pState.canCar = false;
    }
}

void checkGasPump(player& player, float deltaTime, gameData& gData, meshedObject& gasPump) {

    if (canInteract(player, gasPump.collider, gasPump.cPlaneCount, 8.0f, GetScreenWidth(), GetScreenHeight(), 500.f)) {
        player.pState.canGasPump = true;
    }
    else {
        player.pState.canGasPump = false;
    }
}

void interact(player& player, float deltaTime, gameData& gData, meshedObject& car, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, Sound swallow) {

    if (getAsyncKeyStateWrapper(KEY_G)) {
        if (player.pState.hasDrank && interactingG) {
            drank.pEntity.location = {0.f, 0.f, 0.f};
            drank.pEntity.magnitude = {0.f, 0.f, 0.f};
            drank.pEntity.collidingY = false;
            player.pState.hasDrank = false;
            player.pState.canPickDrank = false;
            player.pState.canBuy = false;
            interactingG = false;
        }
    }
    else {
        interactingG = true;
    }
    if (getAsyncKeyStateWrapper(KEY_F) && !player.pState.uberMenuOpen) {
        if (player.pState.canGasPump && interacting) {
            if (!player.pState.hasGasPump && !player.pState.pumpingUp && !player.pState.gasMenuOpen) {
                player.pState.gasMenuOpen = true;
                player.canMove = false;
                PumpStorage = gasPumpNozzle.pEntity.location;
                playMenuSound(sndGasIntro);
                interacting = false;
            } else if (player.pState.hasGasPump) {
                player.pState.hasGasPump = false;
                player.pState.pumpingUp = false;
                gasPumpNozzle.pEntity.location = PumpStorage;
                gasPumpNozzleOff.pEntity.location = {0.f, -20.f, 0.f};
                interacting = false;
            }
        }
        else if (player.pState.hasGasPump && interacting) {
            if (player.pState.canCar) {
                resetMeshedLocation(gasPumpNozzleOff);
                locked = true;
                player.pState.hasGasPump = false;
                player.pState.pumpingUp = true;

                gasPumpNozzleOff.pEntity.location = car.pEntity.location + vector3{0.f, 5.5f, 0.f};
                interacting = false;
            }
        }
        else if (interacting && player.pState.canPickDrank && !player.pState.hasDrank) {
            player.pState.hasDrank = true;
            interacting = false;
        }
        else if (interacting && player.pState.hasDrank) {
            if (player.pState.canBuy || player.pState.buying) {
                player.canMove = !player.canMove;
                player.pState.buying = !player.pState.buying;
                if (player.pState.buying) playMenuSound(sndShopIntro);
                interacting = false;
            }
            else if (player.pState.canDrinkDrank) {
                player.pState.canDrinkDrank = false;
                player.pState.drunkenness += 0.2f;
                PlaySound(swallow);
                player.pState.hasDrank = false;
                drank.pEntity.location = {0.f, 0.f, 0.f};
                gData.dranksConsumed++;
                gData.drankTimer = 170.0f;
                interacting = false;
            }
        }
        else if (interacting && player.pState.pumpingUp) {
            if (player.pState.canCar) {
                resetMeshedLocation(gasPumpNozzleOff);

                player.pState.hasGasPump = true;
                player.pState.pumpingUp = false;
                interacting = false;
                locked = false;
            }
        }
        else if (interacting && !player.pState.hasGasPump) {
            if (!player.pState.inCar && player.pState.canCar) {
                player.pState.inCar = !player.pState.inCar;
                interacting = false;
            }
            else if (player.pState.inCar) {
                player.pState.inCar = !player.pState.inCar;
                interacting = false;
                vector3 playerLoc = player.pEntity.location;
                applyRot(player.pEntity.location, car.pEntity.rot, -4.f, 0.001, -0.45f);

            }
        }
    }
    else {
        interacting = true;
    }
}

static constexpr int kNozzleAnchorTriA = 22;
static constexpr int kNozzleAnchorTriB = 120;

void deformGasPump(meshedObject& gasPump, meshedObject& gasPumpNozzleOff) {
    if (kNozzleAnchorTriA >= gasPumpNozzleOff.mesh.count || kNozzleAnchorTriB >= gasPumpNozzleOff.mesh.count) return;

    struct AffectedVert {int tri, vert; vector3 orig;};
    static AffectedVert affected[64];
    static int affectedCount = 0;
    static bool initialized = false;

    if (!initialized) {
        vector3 anchors[6];
        int ac = 0;
        for (int j = 0; j < 3; j++) {
            anchors[ac++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriA].v[j];
            anchors[ac++] = gasPumpNozzleOff.mesh.trisO[kNozzleAnchorTriB].v[j];
        }
        const float kEps = 1e-4f;
        for (int i = 0; i < gasPumpNozzleOff.mesh.count && affectedCount < 64; i++) {
            for (int j = 0; j < 3 && affectedCount < 64; j++) {
                const vector3& v = gasPumpNozzleOff.mesh.trisO[i].v[j];
                for (int k = 0; k < ac; k++) {
                    if (fabsf(v.x - anchors[k].x) < kEps &&
                        fabsf(v.y - anchors[k].y) < kEps &&
                        fabsf(v.z - anchors[k].z) < kEps) {
                        affected[affectedCount++] = {i, j, v};
                        break;
                    }
                }
            }
        }
        initialized = true;
    }

    vector3 target = gasPumpOffset - gasPumpNozzleOff.pEntity.location;
    vector3 rot = gasPumpNozzleOff.pEntity.rot;

    float cz = cosf(rot.z), sz = sinf(rot.z);
    float cy = cosf(rot.y), sy = sinf(rot.y);
    float cx = cosf(rot.x), sx = sinf(rot.x);
    {float x = target.x*cz + target.y*sz; float y = -target.x*sz + target.y*cz; target.x=x; target.y=y;}
    {float x = target.x*cy - target.z*sy; float z = target.x*sy + target.z*cy; target.x=x; target.z=z;}
    {float y = target.y*cx + target.z*sx; float z = -target.y*sx + target.z*cx; target.y=y; target.z=z;}
    vector3 centroid = {0, 0, 0};
    for (int i = 0; i < affectedCount; i++) {
        centroid.x += affected[i].orig.x;
        centroid.y += affected[i].orig.y;
        centroid.z += affected[i].orig.z;
    }
    if (affectedCount > 0) {
        centroid.x /= affectedCount;
        centroid.y /= affectedCount;
        centroid.z /= affectedCount;
    }

    for (int i = 0; i < affectedCount; i++) {
        float relX = affected[i].orig.x - centroid.x;
        float relZ = affected[i].orig.z - centroid.z;
        gasPumpNozzleOff.mesh.trisO[affected[i].tri].v[affected[i].vert] = {
            target.x + relX,
                target.y,
                target.z + relZ
        };
    }
}

void gasPumpLogic(player& player, float deltaTime, gameData& gData, meshedObject& gasPump, meshedObject& car, meshedObject& gasPumpNozzleOff) {
    if (player.pState.hasGasPump) {
        gasPumpNozzleOff.pEntity.location = player.pEntity.location + vector3{0.f, 0.f, 0.f};
        applyCamRot(gasPumpNozzleOff.pEntity.location, vector3{player.camera.camTarget.x, 0.f, 0.f}, -3.f, -1.f, 2.5f);
        gasPumpNozzleOff.pEntity.rot.y = -player.camera.camTarget.x;
        deformGasPump(gasPump, gasPumpNozzleOff);
    }
    if (player.pState.pumpingUp) {
        gasPumpNozzleOff.pEntity.location = car.pEntity.location;
        applyRot(gasPumpNozzleOff.pEntity.location, car.pEntity.rot, 8.0f, -1.9f, 4.8f);
        gasPumpNozzleOff.pEntity.rot.y = car.pEntity.rot.y - M_PI/2.f;
        deformGasPump(gasPump, gasPumpNozzleOff);
    }
}

int gGasMenuSelection = 0;

static void gasMenuLogic(player& player, meshedObject& gasPumpNozzle) {
    int& selection = gGasMenuSelection;
    static bool canNav = true;
    static bool canAction = false;

    if (!player.pState.gasMenuOpen) {
        selection = 0;
        canAction = false;
        return;
    }

    if (!getAsyncKeyStateWrapper(KEY_F))
    canAction = true;

    bool full = player.pState.carFuel >= 100.f;
    bool broke = player.pState.money <= 0.0f;
    int opts = (full || broke) ? 1 : 2;

    if (getAsyncKeyStateWrapper(KEY_UP)) {
        if (canNav) {selection = (selection - 1 + opts) % opts; canNav = false;}
    } else if (getAsyncKeyStateWrapper(KEY_DOWN)) {
        if (canNav) {selection = (selection + 1) % opts; canNav = false;}
    } else {
        canNav = true;
    }

    if (getAsyncKeyStateWrapper(KEY_F)) {
        if (canAction) {
            player.pState.gasMenuOpen = false;
            player.canMove = true;
            canAction = false;
        }
        return;
    }

    if (!getAsyncKeyStateWrapper(KEY_ENTER))
    canAction = true;

    if (!canAction) return;

    if (getAsyncKeyStateWrapper(KEY_ENTER)) {
        bool wantBuy = !full && !broke && selection == 0;
        bool wantClose = !wantBuy;
        if (wantBuy) {
            float gallonsNeeded = (100.f - player.pState.carFuel) / 4.f;
            float maxAfford = std::min(gallonsNeeded, player.pState.money / 2.5f);
            float cost = maxAfford * 2.5f;
            if (maxAfford > 0.0f) {
                player.pState.money -= cost;
                player.pState.carFuel += maxAfford * 4.0f;
                if (player.pState.carFuel > 100.f) player.pState.carFuel = 100.f;
                player.pState.hasGasPump = true;
                gasPumpNozzle.pEntity.location = {0.f, -20.f, 0.f};
                playMenuSound(sndGasPurchase);
            }
        }
        if (wantBuy || wantClose) {
            player.pState.gasMenuOpen = false;
            player.canMove = true;
            canAction = false;
        }
    }

}

// --- held-item lights ---------------------------------------------------------

namespace {

const int MAX_HELD_LIGHTS = 8;

// Same standoff the wand's charge light uses: centred just in front of the eye, so
// it lights the front of whatever is being held rather than the floor.
const float HELD_LIGHT_FORWARD = 1.5f;

struct heldLightDef {
    meshedObject* item;
    vector3 color;
    float intensity;
    float radius;
};

heldLightDef gHeldLights[MAX_HELD_LIGHTS];
int gHeldLightCount = 0;

// Resolved once a frame for the renderer to read.
bool    gHeldLightActive = false;
vector3 gHeldLightPos;
vector3 gHeldLightColor;
float   gHeldLightRadius = 0.f;

} // namespace

void registerHeldLight(meshedObject* item, vector3 color, float intensity, float radius) {
    if (gHeldLightCount >= MAX_HELD_LIGHTS || !item) return;
    heldLightDef& l = gHeldLights[gHeldLightCount++];
    l.item = item;
    l.color = color;
    l.intensity = intensity;
    l.radius = radius;
}

void heldLightUpdate(player& player) {
    gHeldLightActive = false;

    meshedObject* held = heldItemObject();
    if (!held) return;

    for (int i = 0; i < gHeldLightCount; i++) {
        if (gHeldLights[i].item != held) continue;

        float cyw = cosf(player.camera.camTarget.x), syw = sinf(player.camera.camTarget.x);
        float cpi = cosf(player.camera.camTarget.y), spi = sinf(player.camera.camTarget.y);
        vector3 F = normalize3({cpi * cyw, spi, cpi * syw});

        gHeldLightPos    = player.camera.camPos + F.fmult(HELD_LIGHT_FORWARD);
        gHeldLightColor  = gHeldLights[i].color.fmult(gHeldLights[i].intensity);
        gHeldLightRadius = gHeldLights[i].radius;
        gHeldLightActive = true;
        return;
    }
}

bool heldItemLightAt(float* pos, float* color, float* radius) {
    if (!gHeldLightActive) return false;
    pos[0] = gHeldLightPos.x;   pos[1] = gHeldLightPos.y;   pos[2] = gHeldLightPos.z;
    color[0] = gHeldLightColor.x; color[1] = gHeldLightColor.y; color[2] = gHeldLightColor.z;
    *radius = gHeldLightRadius;
    return true;
}


// --- networked players -------------------------------------------------------

namespace {

const char* PLAYER_MODEL = "resources/levels/objects/player_standin.obj";
const float PLAYER_SCALE = 1.0f;

// Parked far below the world until a snapshot places them, so a slot that has
// been built but not yet filled is never visible at the origin.
const vector3 PLAYER_PARKED = {0.f, -10000.f, 0.f};

struct spawnedPlayer {
    meshedObject obj;
    uint32_t playerId;
    bool built;   // mesh loaded
    bool active;  // has a position this frame, so it draws
};

spawnedPlayer gPlayers[NET_MAX_PLAYERS];
int gPlayerPoolSize = 0;
int gActivePlayers = 0;

} // namespace

void spawnPlayers(int count, shaderStore& shader) {
    if (count < 0) count = 0;
    if (count > (int)NET_MAX_PLAYERS) count = (int)NET_MAX_PLAYERS;

    char empty[100] = {0};

    for (int i = 0; i < count; i++) {
        if (gPlayers[i].built) continue;

        // collider = false: these bodies are display-only. Giving them colliders
        // would put them in the world the local player collides against, and
        // remote positions arrive as teleports, which would shove the player.
        create3dObject(gPlayers[i].obj, PLAYER_MODEL, nullptr, false, shader,
                       PLAYER_SCALE, PLAYER_PARKED, empty, false, 0.f, {0, 0, 0});

        gPlayers[i].playerId = 0;
        gPlayers[i].built = true;
        gPlayers[i].active = false;
    }

    gPlayerPoolSize = count;
    std::cout << "spawned " << count << " player slots" << std::endl;
}

void processPlayerPackets(const playerPacket* packets, int count) {
    int n = count;
    if (n < 0) n = 0;
    if (n > gPlayerPoolSize) n = gPlayerPoolSize;

    for (int i = 0; i < n; i++) {
        // Set flat. No physics entity, no gravity, no collision response and no
        // interpolation between ticks — the packet IS the position.
        gPlayers[i].obj.pEntity.location = {packets[i].x, packets[i].y, packets[i].z};
        gPlayers[i].playerId = (uint32_t)packets[i].playerId;
        gPlayers[i].active = true;
    }

    for (int i = n; i < gPlayerPoolSize; i++) {
        gPlayers[i].active = false;
        gPlayers[i].obj.pEntity.location = PLAYER_PARKED;
    }

    gActivePlayers = n;
}

int spawnedPlayerCount() {
    return gActivePlayers;
}

void drawSpawnedPlayers(const camera& cam, shaderStore& shader, const mtx44* vp,
                        const Texture2D* shadowTex, bool receiveShadows,
                        const Texture2D* shadowTexFar) {
    for (int i = 0; i < gPlayerPoolSize; i++) {
        if (!gPlayers[i].active || !gPlayers[i].built) continue;
        Draw3DGPU(gPlayers[i].obj, cam, shader, {255, 0, 0, 255}, vp,
                  shadowTex, receiveShadows, shadowTexFar);
    }
}

void drawSpawnedPlayersDepth(Shader depthShader, int modelLoc) {
    for (int i = 0; i < gPlayerPoolSize; i++) {
        if (!gPlayers[i].active || !gPlayers[i].built) continue;
        Draw3DDepthGPU(gPlayers[i].obj, depthShader, modelLoc);
    }
}

void shutdownSpawnedPlayers() {
    for (int i = 0; i < (int)NET_MAX_PLAYERS; i++) {
        if (!gPlayers[i].built) continue;
        free(gPlayers[i].obj.mesh.tris);
        free(gPlayers[i].obj.mesh.trisO);
        gPlayers[i].obj.mesh.tris = nullptr;
        gPlayers[i].obj.mesh.trisO = nullptr;
        gPlayers[i].built = false;
        gPlayers[i].active = false;
    }
    gPlayerPoolSize = 0;
    gActivePlayers = 0;
}


void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, meshedObject& buyBox, Sound swallow) {
    gasPumpLogic(player, deltaTime, gData, gasPump, testcar, gasPumpNozzleOff);
    checkCar(player, deltaTime, gData, testcar);
    checkGasPump(player, deltaTime, gData, gasPump);
    checkDrank(player, deltaTime, gData, drank);
    checkBuyBox(player, deltaTime, gData, buyBox);
    if (player.pState.hasDrank) holdItem(player, drank);
    if (player.pState.buying) buy(player, 1);
    interact(player, deltaTime, gData, testcar, gasPump, gasPumpNozzle, gasPumpNozzleOff, drank, swallow);
    gasMenuLogic(player, gasPumpNozzle);

}
