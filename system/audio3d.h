#pragma once

#include "raylib.h"
#include "../util.h" // vector3

// Option-A positional audio: stereo pan from bearing + distance attenuation, layered
// on top of raylib's non-spatial Sound API (SetSoundPan/SetSoundVolume). Not true 3D
// HRTF — a cheap "which side, how far" cue that works with existing Sound objects.
//
// Set the listener once per frame from the player camera, then play/update sounds
// with a world position.

// Listener = the player's ears. pos is the camera world position; yaw/pitch are the
// camera angles (camTarget.x / .y). Call once per frame before any positional play.
void setAudioListener(vector3 pos, float yaw, float pitch);

// One-shot: pan + attenuate by worldPos, then PlaySound. baseVolume is the volume at
// the source (0..1); it fades to 0 by maxDist. Skips playback if inaudible.
void playPositional(Sound s, vector3 worldPos, float baseVolume, float maxDist);

// Re-pan/attenuate an already-playing sound (e.g. a looping travel sound attached to
// a moving projectile). Only updates pan + volume; does NOT (re)trigger playback.
void updatePositional(Sound s, vector3 worldPos, float baseVolume, float maxDist);
