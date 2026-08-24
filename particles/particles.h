#pragma once

#include "../util.h" // vector3, mtx44, ToRaylibMatrix

// Additive-billboard fire particle system. Self-contained: owns a procedurally
// generated soft sprite and its own particle/emitter pools, and draws in its own
// rlgl pass (camera-facing quads, additive blend, depth-tested but not depth-
// writing). Lives outside the w2s mesh pipeline on purpose — see render().

void initParticles();                       // build soft sprite + clear pools
// Feed the frame's point lights (world pos[3*n], color[3*n], radius[n]) so smoke is lit
// by them instead of reading as a flat dark blob. Call once per frame before drawFireParticles.
void setParticleLights(int count, const float* pos, const float* color, const float* radius);
int  spawnBurst(vector3 pos, float maxSize, float life, Color color); // one-shot glowing puff: grows fast, shrinks slow
void spawnWisp(vector3 pos, Color color, float sizeScale = 1.f); // twinkling additive mote (magic wisp); sizeScale enlarges it
void spawnExplosion(vector3 pos, float scale); // fireball flash + smoke + tapering fire/smoke emitter + bright transient light
void spawnSmokeBlast(vector3 pos, float scale); // only the one-shot smoke batch (no fire/corona/light)
void spawnImpactRays(vector3 pos, float scale); // fire-coloured vertical rays that shoot up and fade over 3s
void spawnShockSmoke(vector3 origin, float radius, float scale); // one smoke shell on the sphere of `radius` (called as a shockwave expands)
void spawnSmokeCloud(vector3 pos, float scale); // huge volume-filling smoke cloud (radius `scale`) that dissipates quickly
void spawnDarkblast(vector3 from, vector3 to, float scale); // directed beams from->to (white->UV->red), hold 1.5s then fade to 2s
float darkblastSizzle(float u, float phase); // beam sizzle brightness (0..1) at position u; the corona/glow pass uses it too so the glow crackles in lockstep
void spawnPointFlash(vector3 pos, vector3 color, float radius, float intensity, float life); // scripted transient point light, no corona
int  spawnFireEmitter(vector3 pos);         // continuous torch-style emitter; returns id (-1 if full)
int  spawnCoreEmitter(vector3 pos);         // tight dense fire cluster (projectile body); returns id (-1 if full)
void moveFireEmitter(int id, vector3 pos);  // relocate an emitter (e.g. attach to a moving object)
void setFireEmitterActive(int id, bool on); // pause/resume emission; live particles still finish
void releaseEmitter(int id);                // free an emitter slot (live particles still finish)
void updateParticles(float deltaTime);      // emit + integrate
void drawFireParticles(vector3 camPos, vector3 camTarget, vector3 camUp,
                       const mtx44& view, const mtx44& proj); // smoke (alpha) + fire/burst (additive) passes
void shutdownParticles();                   // free sprite

// Transient explosion point lights, read by the renderer so they spill real light
// onto nearby geometry. explosionLightAt fills pos[3]/color[3]/radius for a live
// slot (colour already scaled by the grow/decay envelope) and returns false if dead.
int  explosionLightCount();
bool explosionLightAt(int i, float* pos, float* color, float* radius);
// Transient impact corona (halo): radius + brightness both follow the grow/decay
// envelope. Fed into the renderer's soft-corona pass. False for a dead slot.
bool explosionCoronaAt(int i, float* pos, float* color, float* radius);

// Persistent white wand-tip glow (small corona + dim point light), set every frame
// while casting. intensity01 is the 0..1 envelope (exponential gain/fade); pos is
// the wand tip. The *At accessors feed the renderer's point-light + corona passes.
void setWandGlow(bool active, vector3 pos, float intensity01);
bool wandGlowLightAt(float* pos, float* color, float* radius);
bool wandGlowCoronaAt(float* pos, float* color, float* radius);
