#pragma once

#include "../util.h" // vector3, mtx44, ToRaylibMatrix

// Additive-billboard fire particle system. Self-contained: owns a procedurally
// generated soft sprite and its own particle/emitter pools, and draws in its own
// rlgl pass (camera-facing quads, additive blend, depth-tested but not depth-
// writing). Lives outside the w2s mesh pipeline on purpose — see render().

void initParticles();                       // build soft sprite + clear pools
int  spawnFireEmitter(vector3 pos);         // continuous torch-style emitter; returns id (-1 if full)
void moveFireEmitter(int id, vector3 pos);  // relocate an emitter (e.g. attach to a moving object)
void setFireEmitterActive(int id, bool on); // pause/resume emission; live particles still finish
void updateParticles(float deltaTime);      // emit + integrate
void drawFireParticles(vector3 camPos, vector3 camTarget, vector3 camUp,
                       const mtx44& view, const mtx44& proj); // additive billboard pass
void shutdownParticles();                   // free sprite
