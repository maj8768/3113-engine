#include "particles.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstring>

namespace {

// ---- tunables -----------------------------------------------------------
const int   MAX_PARTICLES = 2048;
const int   MAX_EMITTERS  = 16;
const float EMIT_RATE     = 120.f;  // particles per second, per emitter
const float LIFE_MIN      = 0.50f;
const float LIFE_MAX      = 0.85f;
const float RISE_SPEED    = 4.0f;   // initial upward velocity
const float SPREAD        = 1.0f;   // lateral velocity jitter
const float SRC_RADIUS    = 0.30f;  // emitter mouth radius (world units)
const float BUOYANCY      = 2.0f;   // upward accel (fire speeds up as it heats)
const float SWIRL         = 2.0f;   // turbulence sway amount
const float SIZE_START    = 0.30f;  // billboard diameter at birth
const float SIZE_END      = 0.90f;  // billboard diameter at death
const float BASE_ALPHA    = 200.f;  // additive contribution ceiling

const float TAU = 6.2831853f;

struct Particle {
    vector3 pos;
    vector3 vel;
    float   age;
    float   life;
    float   seed; // per-particle turbulence phase
    bool    alive;
};

struct Emitter {
    vector3 pos;
    bool    used;
    bool    active;
    float   accum; // fractional-particle carry between frames
};

Particle  gParticles[MAX_PARTICLES];
Emitter   gEmit[MAX_EMITTERS];
int       gCursor = 0;   // round-robin write cursor into the particle ring
float     gTime   = 0.f; // internal clock for turbulence
Texture2D gSoft   = {0}; // procedural soft round sprite
bool      gInit   = false;

inline float frand()  { return (float)GetRandomValue(0, 10000) / 10000.f; } // 0..1
inline float frand2() { return frand() * 2.f - 1.f; }                        // -1..1

void emitOne(const Emitter& e) {
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES; // overwrite oldest slot

    float a  = frand() * TAU;
    float rr = SRC_RADIUS * sqrtf(frand());  // uniform over the mouth disc
    p.pos  = { e.pos.x + cosf(a) * rr, e.pos.y, e.pos.z + sinf(a) * rr };
    p.vel  = { frand2() * SPREAD, RISE_SPEED + frand() * 2.0f, frand2() * SPREAD };
    p.age  = 0.f;
    p.life = LIFE_MIN + frand() * (LIFE_MAX - LIFE_MIN);
    p.seed = frand() * TAU;
    p.alive = true;
}

// Fire gradient: hot white core -> yellow -> orange -> deep red as t (0..1) rises.
void fireColor(float t, unsigned char& r, unsigned char& g, unsigned char& b) {
    if (t < 0.25f) {                 // white -> yellow
        float k = t / 0.25f;
        r = 255; g = (unsigned char)(255 - k * 40.f); b = (unsigned char)(210 - k * 150.f);
    } else if (t < 0.60f) {          // yellow -> orange
        float k = (t - 0.25f) / 0.35f;
        r = 255; g = (unsigned char)(215 - k * 110.f); b = (unsigned char)(60 - k * 45.f);
    } else {                         // orange -> deep red
        float k = (t - 0.60f) / 0.40f;
        r = (unsigned char)(255 - k * 70.f); g = (unsigned char)(105 - k * 80.f); b = 15;
    }
}

} // namespace

void initParticles() {
    if (gInit) return;
    memset(gParticles, 0, sizeof(gParticles));
    memset(gEmit, 0, sizeof(gEmit));
    gCursor = 0;
    gTime = 0.f;

    // 64x64 soft round sprite: rgb = white, alpha = smooth quadratic falloff.
    // Additive blend uses (SRC_ALPHA, ONE), so the alpha ramp shapes the glow.
    const int S = 64;
    Color* px = (Color*)MemAlloc(S * S * sizeof(Color));
    float c = (S - 1) * 0.5f;
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            float dx = (x - c) / c, dy = (y - c) / c;
            float d = sqrtf(dx * dx + dy * dy); // 0 center .. 1 edge
            float a = 1.f - d;
            if (a < 0.f) a = 0.f;
            a *= a; // soften the shoulder
            px[y * S + x] = (Color){ 255, 255, 255, (unsigned char)(a * 255.f) };
        }
    }
    Image img = { px, S, S, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    gSoft = LoadTextureFromImage(img);
    SetTextureFilter(gSoft, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img); // frees px

    gInit = true;
}

int spawnFireEmitter(vector3 pos) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
        if (!gEmit[i].used) {
            gEmit[i].used = true;
            gEmit[i].active = true;
            gEmit[i].pos = pos;
            gEmit[i].accum = 0.f;
            return i;
        }
    }
    return -1;
}

void moveFireEmitter(int id, vector3 pos) {
    if (id < 0 || id >= MAX_EMITTERS) return;
    gEmit[id].pos = pos;
}

void setFireEmitterActive(int id, bool on) {
    if (id < 0 || id >= MAX_EMITTERS) return;
    gEmit[id].active = on;
}

void updateParticles(float dt) {
    if (dt <= 0.f) return;
    gTime += dt;

    // Emit from every active emitter (fractional carry keeps the rate frame-independent).
    for (int i = 0; i < MAX_EMITTERS; i++) {
        Emitter& e = gEmit[i];
        if (!e.used || !e.active) continue;
        e.accum += EMIT_RATE * dt;
        int n = (int)e.accum;
        e.accum -= (float)n;
        for (int k = 0; k < n; k++) emitOne(e);
    }

    // Integrate: buoyancy + a bit of swirl that grows as the particle ages.
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = gParticles[i];
        if (!p.alive) continue;
        p.age += dt;
        if (p.age >= p.life) { p.alive = false; continue; }
        p.vel.y += BUOYANCY * dt;
        float swx = sinf(gTime * 3.0f + p.seed) * SWIRL * p.age;
        float swz = cosf(gTime * 2.6f + p.seed) * SWIRL * p.age;
        p.pos.x += (p.vel.x + swx) * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += (p.vel.z + swz) * dt;
    }
}

void drawFireParticles(vector3 camPos, vector3 camTarget, vector3 camUp,
                       const mtx44& view, const mtx44& proj) {
    if (!gInit) return;

    // Camera basis in world space, for billboarding.
    // camTarget holds yaw (.x) / pitch (.y) ANGLES, not a world point — build the
    // camera forward the same way viewMtx44 does, or the billboards mis-orient.
    float cyw = cosf(camTarget.x), syw = sinf(camTarget.x);
    float cpi = cosf(camTarget.y), spi = sinf(camTarget.y);
    vector3 fwd = normalize3({ cpi * cyw, spi, cpi * syw });
    vector3 right = normalize3(cross3(fwd, camUp));
    vector3 up = cross3(right, fwd); // orthonormal -> already unit length

    rlDrawRenderBatchActive();                 // flush the opaque scene batch first
    rlSetMatrixProjection(ToRaylibMatrix(proj));
    rlSetMatrixModelview(ToRaylibMatrix(view)); // draw in the same VP as the scene

    rlDisableBackfaceCulling();
    rlEnableDepthTest();
    rlDisableDepthMask();                       // occluded by the world, but not by each other

    BeginBlendMode(BLEND_ADDITIVE);
    rlSetTexture(gSoft.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive) continue;
        float t = p.age / p.life;

        float sz = (SIZE_START + (SIZE_END - SIZE_START) * t) * 0.5f; // half-extent
        unsigned char r, g, b;
        fireColor(t, r, g, b);
        float fin  = t < 0.1f ? (t / 0.1f) : 1.f;   // quick fade-in
        float fade = fin * (1.f - t) * (1.f - t);   // fade-out over life
        unsigned char a = (unsigned char)(fade * BASE_ALPHA);
        if (a == 0) continue;

        vector3 rx = { right.x * sz, right.y * sz, right.z * sz };
        vector3 uy = { up.x * sz, up.y * sz, up.z * sz };
        vector3 tl = { p.pos.x - rx.x + uy.x, p.pos.y - rx.y + uy.y, p.pos.z - rx.z + uy.z };
        vector3 bl = { p.pos.x - rx.x - uy.x, p.pos.y - rx.y - uy.y, p.pos.z - rx.z - uy.z };
        vector3 br = { p.pos.x + rx.x - uy.x, p.pos.y + rx.y - uy.y, p.pos.z + rx.z - uy.z };
        vector3 tr = { p.pos.x + rx.x + uy.x, p.pos.y + rx.y + uy.y, p.pos.z + rx.z + uy.z };

        rlColor4ub(r, g, b, a);
        rlTexCoord2f(0.f, 0.f); rlVertex3f(tl.x, tl.y, tl.z);
        rlTexCoord2f(0.f, 1.f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.f, 1.f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.f, 0.f); rlVertex3f(tr.x, tr.y, tr.z);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();                  // flush our quads under the set matrices

    EndBlendMode();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void shutdownParticles() {
    if (!gInit) return;
    UnloadTexture(gSoft);
    gSoft = {0};
    gInit = false;
}
