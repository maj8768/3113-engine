#pragma once
#include "raylib.h"
#include "../util.h" // player, camera, shaderStore, meshedObject, mtx44, vector3
#include "../server/server_util.h" // playerPacket, NET_MAX_PLAYERS

void levelLogic(player& player, float deltaTime,gameData& gData, meshedObject& testcar, meshedObject& gasPump, meshedObject& gasPumpNozzle, meshedObject& gasPumpNozzleOff, meshedObject& drank, meshedObject& buyBox, Sound swallow);
void wizardCombatUpdate(player& player, float deltaTime, world& worldInstance);
// --- spells ------------------------------------------------------------------
// Every registered spell is always live. Each frame it checks whether the item it
// requires is the one currently held, and only a match is allowed to fire. An item
// that no spell requires (the hand, today) simply never matches, so holding it
// casts nothing without needing a check anywhere else.
typedef void (*spellCast)(player& p, vector3 origin);

// Register at startup, once the item meshes exist. requiredItem == nullptr means
// the spell can never fire.
void registerSpell(int key, meshedObject* requiredItem, float cooldown, spellCast cast);

// Per-frame: evaluates every registered spell. castOrigin is where the charge FX
// and projectiles come from — the held item's tip.
void spellsUpdate(player& player, float dt, vector3 castOrigin,
                  Sound m1, Sound m2, Sound m3);

// The three spells that exist today. All of them require the wand.
void castFireball(player& p, vector3 origin);
void castHeavenSword(player& p, vector3 origin);
void castDarkblast(player& p, vector3 origin);

// --- held-item lights ---------------------------------------------------------
// A light an item emits just by being held: always on while it is the held item,
// with no cast and no envelope, unlike the wand's charge glow.
void registerHeldLight(meshedObject* item, vector3 color, float intensity, float radius);

// Resolve this frame's held light. Call after the camera is finalised.
void heldLightUpdate(player& player);

// Feeds the renderer's point-light pass, same shape as wandGlowLightAt.
bool heldItemLightAt(float* pos, float* color, float* radius);
/*
void updateWandPhysics(player& player, float deltaTime, world& worldInstance, meshedObject& wand);
void updateWandHold(player& player, meshedObject& wand);
*/
void resetLevelLogicState();
void playMenuSound(Sound s);

// --- networked players -------------------------------------------------------
// A pool of stand-in bodies for everyone else in the session. Their positions come
// straight off the server snapshot: set FLAT, with no physics entity, no gravity,
// no collision and no render interpolation. The network struct is the only thing
// that moves them.

// Build the pool. Call once from initialise(), after the shader is set up.
// count is clamped to NET_MAX_PLAYERS.
void spawnPlayers(int count, shaderStore& shader);

// Player movement handler. Takes a flat array of position packets and places the
// spawned bodies from it — nothing more. It does not know or care whether this
// instance is hosting or joined; both roles hand it the same array. Slots beyond
// count go inactive and stop drawing. Call once per frame from update().
void processPlayerPackets(const playerPacket* packets, int count);

// How many bodies are currently being drawn.
int spawnedPlayerCount();

void drawSpawnedPlayers(const camera& cam, shaderStore& shader, const mtx44* vp,
                        const Texture2D* shadowTex, bool receiveShadows,
                        const Texture2D* shadowTexFar);
void drawSpawnedPlayersDepth(Shader depthShader, int modelLoc);

// Frees the pool's meshes. Call from shutdown().
void shutdownSpawnedPlayers();

// Defined in main.cpp, where all meshedObjects are created and owned.
void create3dObject(meshedObject& object, const char* path, const char* colliderPath,
                    bool collider, shaderStore& shader, float scale, vector3 location,
                    char text[100], bool renderText, float textRenderDistance,
                    vector3 relativeTextOffset);

extern int gGasMenuSelection;
extern Sound sndShopIntro;
extern Sound sndShopBye;
extern Sound sndShopThank;
extern Sound sndGasIntro;
extern Sound sndGasPurchase;