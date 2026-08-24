#include "audio3d.h"

#include <cmath>

namespace {

vector3 gListenerPos = {0.f, 0.f, 0.f};
float   gListenerYaw = 0.f;   // camTarget.x
float   gListenerPitch = 0.f; // camTarget.y (unused for horizontal pan; kept for future)

// Fill volume (0..baseVolume) and pan (0 = left .. 1 = right, 0.5 = center) for a
// source at `src`, given the current listener. Distance uses a squared linear falloff
// to maxDist; pan uses the horizontal bearing against the camera's right vector.
void panVol(vector3 src, float baseVolume, float maxDist, float& outVol, float& outPan) {
    float dx = src.x - gListenerPos.x;
    float dy = src.y - gListenerPos.y;
    float dz = src.z - gListenerPos.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    float atten;
    if (maxDist <= 0.f) atten = 1.f;
    else if (dist >= maxDist) atten = 0.f;
    else { float x = 1.f - dist / maxDist; atten = x * x; }
    outVol = baseVolume * atten;

    // Horizontal camera basis: right = cross(forward, up=(0,1,0)) = (-sin yaw, 0, cos yaw),
    // matching the engine's right vector (see drawFireParticles in particles.cpp).
    // Project the FULL 3D direction (normalised by `dist`, not the horizontal span) onto
    // the right vector: a source mostly overhead/below or dead-ahead pans to centre, and
    // only genuine left/right displacement moves it off-centre. (Normalising by the
    // horizontal length instead made an overhead source, slightly offset sideways, pan
    // hard to one ear.)
    float rx = -sinf(gListenerYaw), rz = cosf(gListenerYaw);
    float pan = 0.5f;
    if (dist > 1e-4f) {
        float dot = (dx * rx + dz * rz) / dist; // -1 (left) .. +1 (right), reduced by elevation
        pan = 0.5f - 0.5f * dot;                // raylib pan: lower = right, so subtract
        if (pan < 0.f) pan = 0.f;
        if (pan > 1.f) pan = 1.f;
    }
    outPan = pan;
}

} // namespace

void setAudioListener(vector3 pos, float yaw, float pitch) {
    gListenerPos = pos;
    gListenerYaw = yaw;
    gListenerPitch = pitch;
}

void playPositional(Sound s, vector3 worldPos, float baseVolume, float maxDist) {
    float vol, pan;
    panVol(worldPos, baseVolume, maxDist, vol, pan);
    if (vol <= 0.001f) return; // inaudible from here: don't bother triggering it
    SetSoundVolume(s, vol);
    SetSoundPan(s, pan);
    PlaySound(s);
}

void updatePositional(Sound s, vector3 worldPos, float baseVolume, float maxDist) {
    float vol, pan;
    panVol(worldPos, baseVolume, maxDist, vol, pan);
    SetSoundVolume(s, vol);
    SetSoundPan(s, pan);
}
