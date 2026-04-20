#pragma once

// Sample-based engine audio (markeasting/engine-audio architecture).
// Four looping samples crossfaded by RPM + throttle, pitch-shifted per frame.
//
// Usage per frame:
//   engineSynth_setRPM(normalised);    // 0=idle, 1=redline
//   engineSynth_setThrottle(0 or 1);   // 0=lift, 1=WOT
//   engineSynth_setVolume(0..1);
//   engineSynth_update();              // apply gains + pitch — call last

void engineSynth_start();
void engineSynth_stop();
void engineSynth_setRPM(float normalized);
void engineSynth_setThrottle(float throttle);
void engineSynth_setVolume(float vol);
void engineSynth_setGear(int gear);
void engineSynth_update();              // call once per frame after the setters above

void engineSynth_upshift(float normalizedRPM);    // seek on_mid to post-upshift (lower) RPM
void engineSynth_downshift(float normalizedRPM);  // seek on_mid to post-downshift (higher) RPM

void engineSynth_setAtLimiter(bool atLimiter);
void engineSynth_setPopsEnabled(bool enabled);
