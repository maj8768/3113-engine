#pragma once

void engineSynth_start();
void engineSynth_stop();
void engineSynth_setRPM(float normalized);
void engineSynth_setThrottle(float throttle);
void engineSynth_setVolume(float vol);
void engineSynth_setGear(int gear);
void engineSynth_update();

void engineSynth_upshift(float normalizedRPM);
void engineSynth_downshift(float normalizedRPM);

void engineSynth_setAtLimiter(bool atLimiter);
void engineSynth_setPopsEnabled(bool enabled);
