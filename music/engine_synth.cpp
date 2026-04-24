

#define MA_API static
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* kPaths[6] = {
    "resources/sounds/car/engine_on_low.wav",
        "resources/sounds/car/engine_on_mid.wav",
        "resources/sounds/car/engine_on_high.wav",
        "resources/sounds/car/engine_off_low.wav",
        "resources/sounds/car/engine_off_mid.wav",
        "resources/sounds/car/engine_off_high.wav",
    };

static const float kRefRPM[6] = {1500.f, 3000.f, 6000.f, 2000.f, 3000.f, 6000.f};

static const float kVolTrim[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

static const float kPitchFactorUp[6] = {0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f};
static const float kPitchFactorDown[6] = {0.2f, 0.4f, 0.2f, 0.2f, 0.2f, 0.2f};

static const float kOffMidClampRPM = 5500.f;

static const float kOnMidLowRPM = 1500.f;
static const float kOnMidHighRPM = 4000.f;

static const float kRPM_LowStart = 2000.f;
static const float kRPM_LowEnd = 2500.f;
static const float kRPM_HighStart = 4500.f;
static const float kRPM_HighEnd = 5500.f;

static const float kIdleRPM = 1500.f;
static const float kRedlineRPM = 6000.f;

static const float kSampleRate = 44100.f;

static std::atomic<float> gAtomRPM {0.f};
static std::atomic<float> gAtomThr {0.f};
static std::atomic<float> gAtomVol {0.f};
static std::atomic<int> gAtomGear {0};

static std::atomic<bool> gDownshiftSeekPending {false};

static float gRPM = kIdleRPM;
static float gThr = 0.f;
static float gVol = 0.f;

struct SampleSlot {
    float* buf = nullptr;
    ma_uint64 frames = 0;
    float gain = 0.f;
    float pitch = 1.f;
    float resPos = 0.f;
    bool loaded = false;
    std::atomic<float> pendingSeekPos {-1.f};
};
static SampleSlot gSlots[6];

static ma_device gDevice;
static bool gInited = false;

static float gRefRPM[6] = {1500.f, 3000.f, 6000.f, 2000.f, 3000.f, 6000.f};

static float clamp01(float v) {return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);}

static float ratio(float v, float lo, float hi) {
    return clamp01((v - lo) / (hi - lo));
}

static void eqpower(float t, float* a, float* b) {
    float angle = clamp01(t) * 1.5707963f;
    *a = cosf(angle);
    *b = sinf(angle);
}

static void audio_cb(ma_device*, void* pOut, const void*, ma_uint32 frameCount)
{
    float* out = (float*)pOut;
    memset(out, 0, sizeof(float) * frameCount * 2);

    for (int s = 0; s < 6; s++) {
        SampleSlot& slot = gSlots[s];
        if (!slot.loaded || slot.frames == 0 || slot.gain < 0.0001f) continue;

        float seekPos = slot.pendingSeekPos.exchange(-1.f, std::memory_order_relaxed);
        if (seekPos >= 0.f) slot.resPos = seekPos;

        const float gain = slot.gain;
        const float pitch = slot.pitch < 0.05f ? 0.05f : slot.pitch;

        float rp = gSlots[s].resPos;

        for (ma_uint32 f = 0; f < frameCount; f++) {

            while (rp >= (float)slot.frames) rp -= (float)slot.frames;
            while (rp < 0.f) rp += (float)slot.frames;

            ma_uint64 i0 = (ma_uint64)rp;
            ma_uint64 i1 = (i0 + 1) % slot.frames;
            float frac = rp - (float)i0;

            float L = slot.buf[i0*2] + (slot.buf[i1*2] - slot.buf[i0*2]) * frac;
            float R = slot.buf[i0*2 + 1] + (slot.buf[i1*2 + 1] - slot.buf[i0*2 + 1]) * frac;

            out[f*2] += L * gain;
            out[f*2 + 1] += R * gain;

            rp += pitch;
        }

        gSlots[s].resPos = rp;
    }

    for (ma_uint32 i = 0; i < frameCount * 2; i++) {
        if (out[i] > 0.95f) out[i] = 0.95f;
        else if (out[i] < -0.95f) out[i] = -0.95f;
    }
}

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

    gSlots[idx].buf = buf;
    gSlots[idx].frames = total;
    gSlots[idx].resPos = 0.f;
    gSlots[idx].gain = 0.f;
    gSlots[idx].pitch = 1.f;
    gSlots[idx].loaded = true;
    fprintf(stderr, "[EngineAudio] Slot %d OK  '%s'  (%llu frames)\n",
        idx, path, (unsigned long long)total);
    return true;
}

void engineSynth_start() {
    if (gInited) return;

    for (int i = 0; i < 6; i++) loadSlot(i, kPaths[i]);

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = (ma_uint32)kSampleRate;
    cfg.dataCallback = audio_cb;

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
        gSlots[i].buf = nullptr;
        gSlots[i].frames = 0;
        gSlots[i].gain = 0.f;
        gSlots[i].pitch = 1.f;
        gSlots[i].resPos = 0.f;
        gSlots[i].loaded = false;
        gSlots[i].pendingSeekPos.store(-1.f, std::memory_order_relaxed);
    }
    gRPM = kIdleRPM;
    gThr = 0.f;
    gVol = 0.f;
    gInited = false;
}

void engineSynth_setRPM(float normalized) {
    gAtomRPM.store(clamp01(normalized), std::memory_order_relaxed);
}

void engineSynth_setThrottle(float throttle) {
    gAtomThr.store(clamp01(throttle), std::memory_order_relaxed);
}

void engineSynth_setVolume(float vol) {
    gAtomVol.store(clamp01(vol), std::memory_order_relaxed);
}

void engineSynth_setGear(int gear) {
    gAtomGear.store(gear, std::memory_order_relaxed);
}

void engineSynth_update() {
    if (!gInited) return;

    const int gear = gAtomGear.load(std::memory_order_relaxed);
    const float targetRPM = kIdleRPM + gAtomRPM.load(std::memory_order_relaxed)
    * (kRedlineRPM - kIdleRPM);
    const float targetThr = gAtomThr.load(std::memory_order_relaxed);
    const float targetVol = gAtomVol.load(std::memory_order_relaxed);

    const float dt = 1.f / 60.f;
    gRPM += (targetRPM > gRPM ? 3.f : 2.f) * dt * (targetRPM - gRPM);
    gThr += 10.f * dt * (targetThr - gThr);
    gVol += 12.f * dt * (targetVol - gVol);

    float t1 = ratio(gRPM, kRPM_LowStart, kRPM_LowEnd);
    const float k = 1.5707963f;
    float onLow = cosf(t1 * k);
    float onMid = sinf(t1 * k);

    float idleBias = 1.f - ratio(gRPM, kIdleRPM, kIdleRPM + 500.f);
    float thrBlend = gThr > idleBias ? gThr : idleBias;
    float gOff, gOn;
    eqpower(thrBlend, &gOff, &gOn);

    static float prevGOff = 0.f;
    bool downshiftJustFired = gDownshiftSeekPending.exchange(false, std::memory_order_relaxed);
    if (downshiftJustFired) {
        prevGOff = gOff;
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

    const float gain[6] = {
        gOn * onLow * kVolTrim[0] * gVol,
            gOn * onMid * kVolTrim[1] * gVol,
            0.f,
            gOff * kVolTrim[3] * gVol,
            0.f,
            0.f,
        };

    static float varPhase = 0.f;
    const float varFreq = 3.f;
    varPhase += varFreq * (1.f / 60.f) * 6.2831853f;
    if (varPhase > 6.2831853f) varPhase -= 6.2831853f;
    float varAmt = ratio(gRPM, 5900.f, kRedlineRPM);
    float varDip = (sinf(varPhase) * 0.5f + 0.5f) * varAmt;

    float onMidRPM;
    onMidRPM = gRPM - varDip * 500.f;

    static const float kPitchTrim[6] = {1.f, 1.f, 1.f, 1.3f, 1.f, 1.f};

    for (int i = 0; i < 6; i++) {
        if (!gSlots[i].loaded) continue;
        gSlots[i].gain = gain[i];
        float rpm = (i == 1) ? onMidRPM : gRPM;
        float pf = (rpm >= gRefRPM[i]) ? kPitchFactorUp[i] : kPitchFactorDown[i];
        gSlots[i].pitch = powf(2.f, (rpm - gRefRPM[i]) * pf / 1200.f) * kPitchTrim[i];
    }
}

void engineSynth_upshift(float normalizedRPM) {
    if (!gInited || !gSlots[1].loaded) return;
    float rpm = kIdleRPM + clamp01(normalizedRPM) * (kRedlineRPM - kIdleRPM);
    float t = ratio(rpm, kOnMidLowRPM, kOnMidHighRPM);
    float seekFrame = t * (float)gSlots[1].frames;
    if (seekFrame < 0.f) seekFrame = 0.f;
    if (seekFrame >= (float)gSlots[1].frames) seekFrame = (float)gSlots[1].frames - 1.f;
    gSlots[1].pendingSeekPos.store(seekFrame, std::memory_order_relaxed);
}

void engineSynth_downshift(float normalizedRPM) {
    if (!gInited) return;
    float rpm = kIdleRPM + clamp01(normalizedRPM) * (kRedlineRPM - kIdleRPM);

    if (gSlots[1].loaded) {
        float t = ratio(rpm, kOnMidLowRPM, kOnMidHighRPM);
        float f = t * (float)gSlots[1].frames;
        if (f < 0.f) f = 0.f;
        if (f >= (float)gSlots[1].frames) f = (float)gSlots[1].frames - 1.f;
        gSlots[1].pendingSeekPos.store(f, std::memory_order_relaxed);
    }

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

    gDownshiftSeekPending.store(true, std::memory_order_relaxed);
}

void engineSynth_setAtLimiter(bool) {}
void engineSynth_setPopsEnabled(bool) {}
