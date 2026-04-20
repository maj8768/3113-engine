// Sample-based engine audio — four looping WAV samples crossfaded by RPM and
// throttle, pitch-shifted each frame to track engine speed.
//
// Sample layout (record what each file IS):
//   [0] on_low   — steady idle, throttle on
//   [1] on_high  — steady 6000 RPM, throttle on
//   [2] off_low  — deceleration 3000 → idle, throttle off
//   [3] off_high — deceleration 6000 → idle, throttle off
//
// Uses ma_device (raw callback) + ma_decoder alongside raylib's InitAudioDevice().
// MA_API static keeps miniaudio symbols file-internal so they don't clash with
// the copy raylib uses internally.

#define MA_API static
#define MINIAUDIO_IMPLEMENTATION
#include "../../raylib/src/external/miniaudio.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>

// ============================================================================
// Tunable constants — edit these to match your recordings
// ============================================================================

static const char* kPaths[6] = {
    "resources/sounds/car/engine_on_low.wav",   // [0] on_low
    "resources/sounds/car/engine_on_mid.wav",   // [1] on_mid
    "resources/sounds/car/engine_on_high.wav",  // [2] on_high
    "resources/sounds/car/engine_off_low.wav",  // [3] off_low
    "resources/sounds/car/engine_off_mid.wav",  // [4] off_mid
    "resources/sounds/car/engine_off_high.wav", // [5] off_high
};

// RPM at which each sample was recorded — pitch = 1.0 at this RPM.
//   on_low  : steady 1500 RPM
//   on_mid  : steady 3000 RPM
//   on_high : steady 6000 RPM
//   off_low : decel 2000 → 1500 RPM  (ref = start of sweep)
//   off_mid : decel 3000 → 2000 RPM  (ref = start of sweep)
//   off_high: decel 6000 → 2500 RPM  (ref = start of sweep)
static const float kRefRPM[6] = { 1500.f, 3000.f, 6000.f, 2000.f, 3000.f, 6000.f };

// Per-slot volume trim
static const float kVolTrim[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// How aggressively pitch follows RPM, above and below the reference RPM.
// Formula: pitch_ratio = 2 ^ ( (currentRPM - refRPM) * factor / 1200 )
// kPitchFactorUp  — used when RPM >= refRPM (controls how high pitch can go)
// kPitchFactorDown — used when RPM <  refRPM (controls how low pitch starts)
// on_mid (slot 1) has a steeper down-slope so it sounds lower at idle/low RPM
// without affecting how high it pitches at the top.
static const float kPitchFactorUp[6]   = { 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f };
static const float kPitchFactorDown[6] = { 0.2f, 0.4f, 0.2f, 0.2f, 0.2f, 0.2f };

// off_mid seek mapping:
//   RPM >= kOffMidClampRPM  →  frame 0  (start of file)
//   RPM == kOffMidClampRPM/2 →  50% of file  (e.g. 2750 RPM = 50%)
//   RPM == 0                →  100% of file  (end)
//   Formula: seekFrac = clamp01( (kOffMidClampRPM - rpm) / kOffMidClampRPM )
static const float kOffMidClampRPM = 5500.f;

// RPM range of on_mid recording — used to seek on upshift so the RPM drop is audible.
// Beginning of file = engine at kOnMidLowRPM, end = kOnMidHighRPM (or wherever it revs to).
static const float kOnMidLowRPM  = 1500.f;  // RPM at the very start of the on_mid file
static const float kOnMidHighRPM = 4000.f;  // RPM at the very end of the on_mid file

// 3-way RPM crossfade band:
//   Below kRPM_LowEnd        → low only
//   kRPM_LowStart..kRPM_LowEnd   → low fades out, mid fades in
//   kRPM_LowEnd..kRPM_HighStart  → mid only
//   kRPM_HighStart..kRPM_HighEnd → mid fades out, high fades in
//   Above kRPM_HighEnd        → high only
static const float kRPM_LowStart  = 2000.f;  // low begins fading out here
static const float kRPM_LowEnd    = 2500.f;  // low is fully gone, mid dominant
static const float kRPM_HighStart = 4500.f;  // mid begins fading out here
static const float kRPM_HighEnd   = 5500.f;  // high is fully dominant here

// Absolute RPM values that normalized input 0.0 and 1.0 map to.
static const float kIdleRPM    = 1500.f;
static const float kRedlineRPM = 6000.f;

static const float kSampleRate = 44100.f;

// ============================================================================
// Internal state
// ============================================================================

// Game-thread setters write here; update() reads them.
static std::atomic<float> gAtomRPM  { 0.f };
static std::atomic<float> gAtomThr  { 0.f };
static std::atomic<float> gAtomVol  { 0.f };
static std::atomic<int>   gAtomGear { 0 };

// Set to true by engineSynth_downshift so update() re-allows the off_mid seek
// even if gOff was already high (continuous downshifts while engine-braking).
static std::atomic<bool> gDownshiftSeekPending { false };


// Smoothed working values used by audio_cb (written only by update()).
static float gRPM = kIdleRPM;
static float gThr = 0.f;
static float gVol = 0.f;

// Per-slot decoded PCM + playback state.
struct SampleSlot {
    float*    buf    = nullptr;  // interleaved stereo f32
    ma_uint64 frames = 0;        // total frames
    float     gain   = 0.f;      // crossfade gain  (written by update, read by cb)
    float     pitch  = 1.f;      // playback ratio  (written by update, read by cb)
    float     resPos = 0.f;      // resampler read head (fractional frame index)
    bool      loaded = false;
    std::atomic<float> pendingSeekPos { -1.f };  // >=0: seek to this frame on next cb tick; -1=none
};
static SampleSlot gSlots[6];

static ma_device gDevice;
static bool      gInited = false;

// Dynamic pitch reference per slot — starts equal to kRefRPM but is updated
// whenever a slot is seeked so pitch=1.0 right after a seek, drifting only slightly
// as RPM changes between seeks.
static float gRefRPM[6] = { 1500.f, 3000.f, 6000.f, 2000.f, 3000.f, 6000.f };

// ============================================================================
// Helpers
// ============================================================================

static float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

// Map v into [lo,hi], return 0..1.
static float ratio(float v, float lo, float hi) {
    return clamp01((v - lo) / (hi - lo));
}

// Equal-power (cos/sin) crossfade.
//   t = 0  →  *a = 1,  *b = 0   (a is fully audible, b is silent)
//   t = 1  →  *a = 0,  *b = 1   (a is silent, b is fully audible)
static void eqpower(float t, float* a, float* b) {
    float angle = clamp01(t) * 1.5707963f;  // map to [0 .. π/2]
    *a = cosf(angle);   // cos(0)=1  cos(π/2)=0
    *b = sinf(angle);   // sin(0)=0  sin(π/2)=1
}

// ============================================================================
// Audio callback — runs on the audio thread at ~44100 Hz
// ============================================================================
static void audio_cb(ma_device* /*dev*/, void* pOut, const void* /*pIn*/, ma_uint32 frameCount)
{
    float* out = (float*)pOut;
    memset(out, 0, sizeof(float) * frameCount * 2);

    for (int s = 0; s < 6; s++) {
        SampleSlot& slot = gSlots[s];
        if (!slot.loaded || slot.frames == 0 || slot.gain < 0.0001f) continue;

        // Seek requested by update() — jump to the computed frame position.
        float seekPos = slot.pendingSeekPos.exchange(-1.f, std::memory_order_relaxed);
        if (seekPos >= 0.f) slot.resPos = seekPos;

        const float gain  = slot.gain;
        const float pitch = slot.pitch < 0.05f ? 0.05f : slot.pitch;

        // Local copy of fractional read head so we don't race with update().
        float rp = gSlots[s].resPos;

        for (ma_uint32 f = 0; f < frameCount; f++) {
            // Wrap into buffer
            while (rp >= (float)slot.frames) rp -= (float)slot.frames;
            while (rp <  0.f)               rp += (float)slot.frames;

            ma_uint64 i0   = (ma_uint64)rp;
            ma_uint64 i1   = (i0 + 1) % slot.frames;
            float     frac = rp - (float)i0;

            float L = slot.buf[i0*2]     + (slot.buf[i1*2]     - slot.buf[i0*2])     * frac;
            float R = slot.buf[i0*2 + 1] + (slot.buf[i1*2 + 1] - slot.buf[i0*2 + 1]) * frac;

            out[f*2]     += L * gain;
            out[f*2 + 1] += R * gain;

            rp += pitch;
        }

        gSlots[s].resPos = rp;
    }


    // Hard limiter
    for (ma_uint32 i = 0; i < frameCount * 2; i++) {
        if      (out[i] >  0.95f) out[i] =  0.95f;
        else if (out[i] < -0.95f) out[i] = -0.95f;
    }
}

// ============================================================================
// Load one WAV slot (decoded to f32 stereo at kSampleRate)
// ============================================================================
static bool loadSlot(int idx, const char* path)
{
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 2, (ma_uint32)kSampleRate);
    ma_decoder dec;

    if (ma_decoder_init_file(path, &cfg, &dec) != MA_SUCCESS) {
        fprintf(stderr, "[EngineAudio] Cannot open slot %d: '%s'\n", idx, path);
        return false;
    }

    ma_uint64 frameCount = 0;
    ma_decoder_get_length_in_pcm_frames(&dec, &frameCount);

    float* buf = nullptr;
    ma_uint64 total = 0;

    if (frameCount > 0) {
        buf = (float*)malloc(frameCount * 2 * sizeof(float));
        ma_decoder_read_pcm_frames(&dec, buf, frameCount, &total);
    } else {
        // Streaming format — drain in chunks
        const ma_uint64 kChunk = 4096;
        float tmp[kChunk * 2];
        ma_uint64 got = 0;
        do {
            ma_decoder_read_pcm_frames(&dec, tmp, kChunk, &got);
            buf = (float*)realloc(buf, (total + got) * 2 * sizeof(float));
            memcpy(buf + total * 2, tmp, got * 2 * sizeof(float));
            total += got;
        } while (got == kChunk);
    }

    ma_decoder_uninit(&dec);

    if (total == 0) {
        fprintf(stderr, "[EngineAudio] Slot %d decoded 0 frames from '%s'\n", idx, path);
        free(buf);
        return false;
    }

    gSlots[idx].buf    = buf;
    gSlots[idx].frames = total;
    gSlots[idx].resPos = 0.f;
    gSlots[idx].gain   = 0.f;
    gSlots[idx].pitch  = 1.f;
    gSlots[idx].loaded = true;
    fprintf(stderr, "[EngineAudio] Slot %d OK  '%s'  (%llu frames)\n",
            idx, path, (unsigned long long)total);
    return true;
}

// ============================================================================
// Public API
// ============================================================================

void engineSynth_start() {
    if (gInited) return;

    for (int i = 0; i < 6; i++) loadSlot(i, kPaths[i]);

    ma_device_config cfg  = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = (ma_uint32)kSampleRate;
    cfg.dataCallback      = audio_cb;

    if (ma_device_init(NULL, &cfg, &gDevice) != MA_SUCCESS) {
        fprintf(stderr, "[EngineAudio] ma_device_init failed\n");
        return;
    }
    if (ma_device_start(&gDevice) != MA_SUCCESS) {
        fprintf(stderr, "[EngineAudio] ma_device_start failed\n");
        return;
    }
    gInited = true;
    fprintf(stderr, "[EngineAudio] Started\n");
}

void engineSynth_stop() {
    if (!gInited) return;
    ma_device_stop(&gDevice);
    ma_device_uninit(&gDevice);
    for (int i = 0; i < 6; i++) {
        free(gSlots[i].buf);
        gSlots[i].buf    = nullptr;
        gSlots[i].frames = 0;
        gSlots[i].gain   = 0.f;
        gSlots[i].pitch  = 1.f;
        gSlots[i].resPos = 0.f;
        gSlots[i].loaded = false;
        gSlots[i].pendingSeekPos.store(-1.f, std::memory_order_relaxed);
    }
    gRPM = kIdleRPM;
    gThr = 0.f;
    gVol = 0.f;
    gInited = false;
}

// normalized: 0.0 = idle (kIdleRPM), 1.0 = redline (kRedlineRPM)
void engineSynth_setRPM(float normalized) {
    gAtomRPM.store(clamp01(normalized), std::memory_order_relaxed);
}

// throttle: 0.0 = fully lifted (off samples), 1.0 = wide open (on samples)
void engineSynth_setThrottle(float throttle) {
    gAtomThr.store(clamp01(throttle), std::memory_order_relaxed);
}

void engineSynth_setVolume(float vol) {
    gAtomVol.store(clamp01(vol), std::memory_order_relaxed);
}

void engineSynth_setGear(int gear) {
    gAtomGear.store(gear, std::memory_order_relaxed);
}

// Call once per game frame, after the setters above.
void engineSynth_update() {
    if (!gInited) return;

    // Convert normalized RPM → absolute RPM
    const int   gear      = gAtomGear.load(std::memory_order_relaxed);
    const float targetRPM = kIdleRPM + gAtomRPM.load(std::memory_order_relaxed)
                                       * (kRedlineRPM - kIdleRPM);
    const float targetThr = gAtomThr.load(std::memory_order_relaxed);
    const float targetVol = gAtomVol.load(std::memory_order_relaxed);

    // Smooth everything.  RPM rises faster than it falls (turbo holds revs).
    const float dt = 1.f / 60.f;
    gRPM += (targetRPM > gRPM ? 3.f : 2.f) * dt * (targetRPM - gRPM);
    gThr += 10.f * dt * (targetThr - gThr);
    gVol += 12.f * dt * (targetVol - gVol);

    // ON: 2-way RPM blend (low → mid)
    float t1 = ratio(gRPM, kRPM_LowStart, kRPM_LowEnd);
    const float k = 1.5707963f;
    float onLow = cosf(t1 * k);
    float onMid = sinf(t1 * k);

    // Throttle blend — idleBias forces on_low at idle so off_mid doesn't
    // play when just sitting still; crossfades off_mid → on_low as RPM drops.
    float idleBias = 1.f - ratio(gRPM, kIdleRPM, kIdleRPM + 500.f);
    float thrBlend = gThr > idleBias ? gThr : idleBias;
    float gOff, gOn;
    eqpower(thrBlend, &gOff, &gOn);

    // Seek off_mid to the frame matching current RPM on:
    //   a) initial lift-off (gOff crosses 0.01 from below), or
    //   b) any downshift while already off-throttle (gDownshiftSeekPending flag).
    // Both cases: beginning of file = kOffMidHighRPM, end of file = kOffMidLowRPM.
    // On lift-off: seek off_mid to the frame matching current RPM (initial lift-off only).
    // On downshift: engineSynth_downshift() already stored the seek directly using the
    // accurate rpm from car.cpp; we just reset prevGOff so the next real lift-off re-arms.
    static float prevGOff = 0.f;
    bool downshiftJustFired = gDownshiftSeekPending.exchange(false, std::memory_order_relaxed);
    if (downshiftJustFired) {
        prevGOff = gOff;  // re-arm: don't treat next frame as a fresh lift-off edge
    }
    if (gOff > 0.01f && prevGOff <= 0.01f && gSlots[3].loaded) {
        float frac = clamp01((kOffMidClampRPM - gRPM) / kOffMidClampRPM);
        float seekFrame = frac * (float)gSlots[3].frames;
        if (seekFrame >= (float)gSlots[3].frames) seekFrame = (float)gSlots[3].frames - 1.f;
        gSlots[3].pendingSeekPos.store(seekFrame, std::memory_order_relaxed);
        gRefRPM[3] = gRPM;
        fprintf(stderr, "[EngineAudio] lift-off seek: RPM=%.0f  frac=%.3f  frame=%.0f / %llu (%.1f%%)\n",
                gRPM, frac, seekFrame, (unsigned long long)gSlots[3].frames,
                100.f * frac);
    }
    prevGOff = gOff;

    // ---- Compute per-slot gains ---------------------------------------------
    // [0]=on_low  [1]=on_mid  [2]=on_high(off)  [3]=off_low  [4]=off_mid(off)  [5]=off_high(off)
    const float gain[6] = {
        gOn  * onLow * kVolTrim[0] * gVol,
        gOn  * onMid * kVolTrim[1] * gVol,
        0.f,
        gOff         * kVolTrim[3] * gVol,
        0.f,
        0.f,
    };

    // ---- Pitch + commit -----------------------------------------------------
    static float varPhase = 0.f;
    const float varFreq = 3.f;  // Hz
    varPhase += varFreq * (1.f / 60.f) * 6.2831853f;
    if (varPhase > 6.2831853f) varPhase -= 6.2831853f;
    float varAmt = ratio(gRPM, 5900.f, kRedlineRPM);
    float varDip = (sinf(varPhase) * 0.5f + 0.5f) * varAmt;

    float onMidRPM;
    onMidRPM = gRPM - varDip * 500.f;
    // Per-slot pitch trim multiplier applied on top of the RPM-based pitch.
    static const float kPitchTrim[6] = { 1.f, 1.f, 1.f, 1.3f, 1.f, 1.f };

    for (int i = 0; i < 6; i++) {
        if (!gSlots[i].loaded) continue;
        gSlots[i].gain  = gain[i];
        float rpm = (i == 1) ? onMidRPM : gRPM;
        float pf  = (rpm >= gRefRPM[i]) ? kPitchFactorUp[i] : kPitchFactorDown[i];
        gSlots[i].pitch = powf(2.f, (rpm - gRefRPM[i]) * pf / 1200.f) * kPitchTrim[i];
    }
}

// Call when an upshift engages. Seeks on_mid to the frame matching the new
// (lower) RPM so the rev-drop is reflected in the sample position.
void engineSynth_upshift(float normalizedRPM) {
    if (!gInited || !gSlots[1].loaded) return;
    float rpm = kIdleRPM + clamp01(normalizedRPM) * (kRedlineRPM - kIdleRPM);
    float t = ratio(rpm, kOnMidLowRPM, kOnMidHighRPM);  // 0=low, 1=high
    float seekFrame = t * (float)gSlots[1].frames;
    if (seekFrame < 0.f) seekFrame = 0.f;
    if (seekFrame >= (float)gSlots[1].frames) seekFrame = (float)gSlots[1].frames - 1.f;
    gSlots[1].pendingSeekPos.store(seekFrame, std::memory_order_relaxed);
}

// Call when a downshift engages. Seeks on_mid and off_mid to frames matching
// the new (higher) RPM so pitch shift alone doesn't play the wrong part of the sample.
void engineSynth_downshift(float normalizedRPM) {
    if (!gInited) return;
    float rpm = kIdleRPM + clamp01(normalizedRPM) * (kRedlineRPM - kIdleRPM);

    // on_mid: beginning=low RPM, end=high RPM → seek forward for higher RPM
    if (gSlots[1].loaded) {
        float t = ratio(rpm, kOnMidLowRPM, kOnMidHighRPM);
        float f = t * (float)gSlots[1].frames;
        if (f < 0.f) f = 0.f;
        if (f >= (float)gSlots[1].frames) f = (float)gSlots[1].frames - 1.f;
        gSlots[1].pendingSeekPos.store(f, std::memory_order_relaxed);
    }

    // off_mid seek: RPM>=5500 → frame 0; 2750 → 50%; linear.
    if (gSlots[4].loaded) {
        float frac = clamp01((kOffMidClampRPM - rpm) / kOffMidClampRPM);
        float f = frac * (float)gSlots[4].frames;
        if (f >= (float)gSlots[4].frames) f = (float)gSlots[4].frames - 1.f;
        gSlots[4].pendingSeekPos.store(f, std::memory_order_relaxed);
        gRefRPM[4] = rpm;
        fprintf(stderr, "[EngineAudio] downshift seek: RPM=%.0f  frac=%.3f  frame=%.0f / %llu (%.1f%%)\n",
                rpm, frac, f, (unsigned long long)gSlots[4].frames,
                100.f * frac);
    }

    // Tell update() to bypass the prevGOff transition guard so the lift-off seek
    // path also re-arms — otherwise continuous downshifts while engine-braking
    // (gOff already > 0.01) would never re-seek via the update() path.
    gDownshiftSeekPending.store(true, std::memory_order_relaxed);
}

void engineSynth_setAtLimiter(bool) {}
void engineSynth_setPopsEnabled(bool) {}
