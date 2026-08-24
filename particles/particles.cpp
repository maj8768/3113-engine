#include "particles.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstring>

namespace {

// ---- tunables -----------------------------------------------------------
const int   MAX_PARTICLES = 14000;
const int   MAX_EMITTERS  = 24;
const float EMIT_RATE     = 120.f;  // particles per second, per torch emitter
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

// ---- explosion tunables -------------------------------------------------
// The explosion is a ONE-SHOT batch: a ton of particles spawned at once that
// simply fade out (start fading at EXPL_FADE_START, gone by EXPL_LIFE). It does
// NOT emit continuously.
const int   FIRE_BATCH       = 340;   // fire particles spawned instantly per explosion
const int   SMOKE_BATCH      = 1400;  // smoke particles spawned instantly per explosion (dense billowing cloud)
const float SMOKE_SPREAD_MULT= 3.0f;  // smoke reaches ~3x the fireball radius (spread + speed vs fire)
const float EXPL_LIFE        = 2.0f;  // fire/corona life
const float EXPL_FADE_START  = 1.0f;  // hold full until here, then fade to 0 by EXPL_LIFE
const float SMOKE_EXTRA_LIFE = 0.3f;  // smoke cloud outlives the fireball by this much
const float EXPL_EXPAND_SPEED= 20.0f; // initial outward burst speed (× scale) — fast
const float EXPL_DRAG        = 5.0f;  // deceleration /s; terminal travel ~= speed/drag, kept < corona radius
const float EXPL_CORONA_RADIUS = 6.0f; // peak impact-corona radius (× scale)
const float EXPL_CORONA_BRIGHT = 3.0f; // peak impact-corona additive brightness
const float EXPL_FIRE_RATE   = 260.f; // (legacy) fire particles/sec for the emitter path
const float EXPL_SMOKE_RATE  = 45.f;  // (legacy) smoke particles/sec for the emitter path
const float EXPL_HOLD        = 1.0f;  // (legacy) seconds emitting at full rate
const float EXPL_TAPER       = 3.0f;  // (legacy) emission ramps to 0 here
const float SMOKE_RISE       = 1.3f;  // upward accel
const float SMOKE_DRAG       = 0.8f;  // lateral velocity damping /s
const float SMOKE_SIZE_START = 0.6f;  // (unused) legacy birth diameter
const float SMOKE_SIZE_END   = 3.6f;  // smoke puff diameter (× emitter scale); big soft cloud puffs
const float SMOKE_ALPHA      = 120.f; // (legacy) alpha ceiling; smoke is now solid via dissolve
const float SMOKE_DISSOLVE_BASE = 0.06f; // coverage floor for solid smoke (lower = larger disc)

// Shockwave shell: thin smoke ring emitted on the expanding sphere's surface (see
// spawnShockSmoke). Short-lived puffs that drift outward a touch so the shell reads as a
// fast-moving heat/smoke wall (the screen warp does the refraction on top).
const int   SHOCK_SMOKE_COUNT = 260;  // smoke puffs per shell emission (dense wall)
const float SHOCK_SMOKE_SPEED = 6.0f; // outward drift (units/s × scale)
const float SHOCK_SMOKE_LIFE  = 1.0f; // puff lifetime (seconds, × small jitter)
const float SHOCK_SMOKE_SIZE  = 8.0f; // puff diameter (× scale); billboard draws at half
const unsigned char SHOCK_SMOKE_ALPHA = 150; // fairly opaque so the shell reads solid

// Huge quick-dissipating smoke cloud (see spawnSmokeCloud): a big volume-filling puff that
// blooms at the impact and clears out fast. `scale` passed in is the cloud RADIUS (world units).
const int   CLOUD_BATCH  = 2600;  // puffs filling the cloud volume (dense)
const float CLOUD_LIFE   = 0.9f;  // short life -> dissipates quickly
const float CLOUD_HOLD   = 0.12f; // fraction of life held full before it starts clearing
const float CLOUD_EXPAND = 4.0f;  // outward drift speed (× radius) as it blooms
const float CLOUD_PUFF   = 7.0f;  // individual puff diameter (world units)
const unsigned char CLOUD_ALPHA = 140;

const float TAU = 6.2831853f;
const float DECAY_K = 3.0f; // exponential decay steepness (higher = holds full longer, sharper end)

enum { PK_FIRE = 0, PK_BURST = 1, PK_SMOKE = 2, PK_WISP = 3, PK_RAY = 4, PK_BEAM = 5 }; // particle kinds

// Impact rays: fire-coloured vertical beams that shoot up from a strike point and fade
// to nothing over their life. (maxSize = target height, fadeStart = beam half-width.)
const int   RAY_COUNT      = 14;    // beams per impact
const float RAY_LIFE       = 3.0f;  // brightest at spawn, gone by here
const float RAY_HEIGHT     = 34.f;  // target beam height (× scale)
const float RAY_WIDTH      = 1.1f;  // beam half-width (× scale)
const float RAY_SHOOT_TIME = 0.28f; // seconds to grow from 0 to full height (the "shoot")
const float RAY_SPREAD     = 2.2f;  // bases scattered this far around the impact (× scale)

// Darkblast beams (PK_BEAM): directed segments from A (pos) to B (vel), spawned instantly,
// held, then faded over the last quarter of life. A bundle of beams offset radially around
// the axis gives layered colour: near-white centre -> ultraviolet -> red at the outer edge.
const int   BEAM_COUNT  = 40;    // beams per blast (more blades -> rounder cylinder)
const float BEAM_LIFE   = 2.0f;  // full until 0.75*life (1.5s), then fade to 0 by 2.0s
const float BEAM_WIDTH  = 0.25f; // beam half-width (× scale)
const float BEAM_SPREAD = 1.0f;  // cylinder radius around the central axis (world units)

// Sizzle: each beam is subdivided along its length and each piece's brightness crackles
// (animated two-frequency noise), so the beam reads as sizzling energy.
const int   BEAM_SEGMENTS     = 8;     // subdivisions per beam
const float BEAM_SIZZLE_FREQ  = 5.0f;  // crackle cells per beam length
const float BEAM_SIZZLE_SPEED = 14.0f; // how fast the crackle animates/travels
const float BEAM_SIZZLE_DEPTH = 0.6f;  // 0..1 how deep the brightness dips

// Adjustable bloom halo for the vertical RAYS (an extra wider, dimmer additive layer).
// The darkblast BEAMS instead get a real glow via the corona pass (see darkblastGlow).
const float RAY_BLOOM   = 0.0f;  // sword rays: no bloom
const float BLOOM_ALPHA = 0.4f;  // halo brightness relative to the core
enum { EK_TORCH = 0, EK_EXPLOSION = 1, EK_CORE = 2 };  // emitter kinds

// Projectile "core": a tight, dense cluster of small fire particles that reads as
// the fireball body (the projectile mesh itself is not drawn).
const float CORE_RATE   = 750.f;  // particles/sec (dense fireball body)
const float CORE_RADIUS = 0.60f;  // spawn radius around the emitter (larger fireball)
const float CORE_LIFE   = 0.26f;  // short life -> stays clustered near the moving ball
const float CORE_SIZE   = 1.10f;  // per-particle size multiplier
const float CORE_JITTER = 0.7f;   // small random velocity

struct Particle {
    vector3 pos;
    vector3 vel;
    float   age;
    float   life;
    float   seed;    // per-particle turbulence phase
    bool    alive;
    int     kind;    // PK_FIRE / PK_BURST / PK_SMOKE
    float   maxSize; // FIRE: size multiplier; BURST/SMOKE: peak/end diameter
    float   fadeStart; // >0: hold full opacity until this age, then linear fade to 0 at life
    unsigned char cr, cg, cb, ca; // BURST/SMOKE color + intensity ceiling
};

struct Emitter {
    vector3 pos;
    bool    used;
    bool    active;
    float   accum;      // fractional fire-particle carry between frames
    float   smokeAccum; // fractional smoke-particle carry (explosion only)
    int     kind;       // EK_TORCH (infinite) / EK_EXPLOSION (timed taper)
    float   age;        // explosion clock
    float   hold;       // explosion: seconds at full emission rate
    float   taper;      // explosion: seconds until emission hits 0 (then released)
    float   scale;      // explosion: size/speed multiplier
};

// A transient, very bright point light spawned at an explosion. It follows the
// same fast-grow/slow-shrink envelope as the fireball flash and is read by the
// renderer (explosionLightCount/At) so it spills real light onto nearby geometry.
struct ExpLight {
    vector3 pos;
    float   age;
    float   life;
    float   peakRadius;       // point-light reach
    float   peakIntensity;    // point-light brightness
    float   peakCoronaRadius; // world-space corona (halo) radius at peak
    float   peakCoronaBright; // corona additive brightness at peak
    vector3 color;            // base (unit-ish) colour; scaled by envelope
    bool    used;
};

const int MAX_EXP_LIGHTS = 8;

// A persistent white glow at the wand tip while casting (position + 0..1 intensity
// set every frame from the cast logic; the envelope shaping is done by the caller).
struct WandGlow { bool active; vector3 pos; float intensity; };
const float WAND_LIGHT_INTENSITY = 8.0f; // white point-light peak (reasonably bright)
const float WAND_LIGHT_RADIUS    = 4.0f; // small reach: hits the wand, not the floor
const float WAND_CORONA_BRIGHT   = 0.30f; // small white corona peak brightness (very dim)
const float WAND_CORONA_RADIUS   = 1.6f; // small corona radius (× intensity)

Particle  gParticles[MAX_PARTICLES];
Emitter   gEmit[MAX_EMITTERS];
ExpLight  gExpLights[MAX_EXP_LIGHTS];
WandGlow  gWandGlow = { false, {0.f, 0.f, 0.f}, 0.f };
int       gCursor = 0;   // round-robin write cursor into the particle ring
float     gTime   = 0.f; // internal clock for turbulence
Texture2D gSoft   = {0}; // procedural soft round sprite
Texture2D gHard   = {0}; // procedural crisp round sprite (hard-edged twinkles)
Texture2D gStar   = {0}; // procedural 4-point star sprite (bright core + spikes)
Shader    gSmokeShader = {0}; // solid-smoke shader (alpha as coverage mask + depth write)
bool      gInit   = false;

// Point lights fed in from the renderer each frame (same set the scene uses). Smoke
// samples these so it isn't a flat dark blob inside a lit area.
const int PL_MAX = 32;
int   gPLCount = 0;
float gPLPos[3 * PL_MAX];
float gPLColor[3 * PL_MAX];
float gPLRadius[PL_MAX];
const float SMOKE_LIGHT_GAIN = 26.f; // maps incident light (~0..14) onto the 0..255 smoke colour

// Incident light at world position p from the registered point lights (same falloff as
// the mesh shader: (1 - d/r)^2). Fills lr/lg/lb (unbounded; caller scales/clamps).
void lightAt(const vector3& p, float& lr, float& lg, float& lb) {
    lr = lg = lb = 0.f;
    for (int i = 0; i < gPLCount; i++) {
        float dx = gPLPos[i*3+0] - p.x, dy = gPLPos[i*3+1] - p.y, dz = gPLPos[i*3+2] - p.z;
        float d = sqrtf(dx*dx + dy*dy + dz*dz);
        float r = gPLRadius[i];
        if (r <= 0.f) continue;
        float at = 1.f - d / r; if (at <= 0.f) continue; at *= at;
        lr += gPLColor[i*3+0] * at;
        lg += gPLColor[i*3+1] * at;
        lb += gPLColor[i*3+2] * at;
    }
}

inline float frand()  { return (float)GetRandomValue(0, 10000) / 10000.f; } // 0..1
inline float frand2() { return frand() * 2.f - 1.f; }                        // -1..1

// Envelope for the fireball flash / corona / light. Snaps up fast to full, drifts
// slowly down to ~90%, then drops fast to 0:
//   [0 .. GROW]  0 -> 1     (very quick growth)
//   [GROW .. HOLD] 1 -> FLOOR (slow fade to ~90% of size)
//   [HOLD .. 1]  FLOOR -> 0  (quick fade out)
inline float growDecay(float t) {
    const float GROW  = 0.05f; // full size by 5% of life
    const float HOLD  = 0.85f; // slow drift until 85% of life
    const float FLOOR = 0.90f; // size it slowly drifts down to
    if (t < GROW) return t / GROW;
    if (t < HOLD) return 1.f - (1.f - FLOOR) * (t - GROW) / (HOLD - GROW);
    // exponential decay of the last phase: holds near FLOOR then drops off at the end.
    float rem = 1.f - (t - HOLD) / (1.f - HOLD); // 1 -> 0
    return FLOOR * (1.f - expf(-DECAY_K * rem)) / (1.f - expf(-DECAY_K));
}

// Full opacity until fadeStart, then an EXPONENTIAL fade to 0 at life (absolute
// seconds). Concave, so it stays near full for most of the fade and only shrinks
// away near the very end, instead of linearly passing through every small value.
inline float holdFade(float age, float fadeStart, float life) {
    if (age <= fadeStart) return 1.f;
    if (age >= life) return 0.f;
    float rem = (life - age) / (life - fadeStart); // 1 -> 0
    return (1.f - expf(-DECAY_K * rem)) / (1.f - expf(-DECAY_K));
}

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
    p.kind = PK_FIRE;
    p.maxSize = 1.f; // torch fire uses the default SIZE_START..SIZE_END ramp
    p.fadeStart = 0.f; // torch fire uses the default fade-out curve
}

// Explosion fire: pre-spread through a small sphere so the whole cloud EXISTS at
// once, with a gentle outward drift. Reuses the PK_FIRE colour ramp + integration.
void emitExplosionFire(const Emitter& e) {
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES;

    float a = frand() * TAU;
    float z = frand2();                 // -1..1 (full sphere, no upward bias)
    float r = sqrtf(1.f - z * z);
    vector3 dir = { r * cosf(a), z, r * sinf(a) }; // uniform direction over a sphere
    float rad = 0.9f * e.scale * powf(frand(), 0.3333f); // uniform through the sphere volume
    p.pos  = { e.pos.x + dir.x * rad, e.pos.y + dir.y * rad, e.pos.z + dir.z * rad };
    float spd = EXPL_EXPAND_SPEED * e.scale * (0.3f + frand() * 0.7f);
    p.vel  = { dir.x * spd, dir.y * spd, dir.z * spd }; // fast symmetric burst (drag bounds it)
    p.age  = 0.f;
    p.life = EXPL_LIFE;
    p.fadeStart = EXPL_FADE_START;
    p.seed = frand() * TAU;
    p.alive = true;
    p.kind = PK_FIRE;
    p.maxSize = (0.6f + frand() * 0.9f) * e.scale; // smaller, varied puffs
}

// Projectile core fire: a small, short-lived, low-velocity particle spawned in a
// tight sphere around the emitter, so the moving cluster looks like a fireball body.
void emitCore(const Emitter& e) {
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES;

    float a = frand() * TAU;
    float z = frand2();
    float r = sqrtf(1.f - z * z);
    float rad = CORE_RADIUS * powf(frand(), 0.3333f); // uniform through a tight sphere
    p.pos  = { e.pos.x + r * cosf(a) * rad, e.pos.y + z * rad, e.pos.z + r * sinf(a) * rad };
    p.vel  = { frand2() * CORE_JITTER, frand2() * CORE_JITTER, frand2() * CORE_JITTER };
    p.age  = 0.f;
    p.life = CORE_LIFE;
    p.seed = frand() * TAU;
    p.alive = true;
    p.kind = PK_FIRE;
    p.maxSize = CORE_SIZE * (0.65f + frand() * 0.7f); // per-particle size variety
    p.fadeStart = 0.f;
}

// Explosion smoke: pre-spread grey haze that rises, expands, and fades on the
// same 0.8..1.2s window as the fire.
void emitExplosionSmoke(const Emitter& e) {
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES;

    float a = frand() * TAU;
    float z = frand2();                 // -1..1 (full sphere, no upward bias)
    float r = sqrtf(1.f - z * z);
    vector3 dir = { r * cosf(a), z, r * sinf(a) }; // uniform direction over a sphere
    float rad = 0.9f * e.scale * SMOKE_SPREAD_MULT * powf(frand(), 0.3333f); // uniform through the (4x) sphere volume
    p.pos  = { e.pos.x + dir.x * rad, e.pos.y + dir.y * rad, e.pos.z + dir.z * rad };
    float spd = EXPL_EXPAND_SPEED * e.scale * (0.3f + frand() * 0.7f) * SMOKE_SPREAD_MULT; // travels ~4x the fireball radius (shared drag)
    p.vel  = { dir.x * spd, dir.y * spd, dir.z * spd }; // fast symmetric burst (drag bounds it)
    p.age  = 0.f;
    p.life = EXPL_LIFE + SMOKE_EXTRA_LIFE; // persists 0.3s longer than the fireball
    p.fadeStart = EXPL_FADE_START;
    p.seed = frand() * TAU;
    p.alive = true;
    p.kind = PK_SMOKE;
    p.maxSize = SMOKE_SIZE_END * e.scale;
    if (frand() < 0.85f) {                 // mostly black sooty smoke (alpha-blended -> darkens scene)
        unsigned char g = (unsigned char)(6 + GetRandomValue(0, 14));
        p.cr = g; p.cg = g; p.cb = g;
        p.ca = (unsigned char)(SMOKE_ALPHA + 20.f);
    } else {                               // lighter grey smoke
        unsigned char g = (unsigned char)(38 + GetRandomValue(0, 26));
        p.cr = g; p.cg = g; p.cb = (unsigned char)(g + 6);
        p.ca = (unsigned char)SMOKE_ALPHA;
    }
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

// Darkblast sizzle brightness multiplier (0..1) at normalised position u along a beam,
// with an optional per-beam phase. Shared by the beam draw and the corona/glow pass so the
// glow crackles in lockstep with the beam. Two animated sines at different rates -> crackle.
float darkblastSizzle(float u, float phase) {
    float m = 0.5f + 0.5f * sinf(u * BEAM_SIZZLE_FREQ * TAU + gTime * BEAM_SIZZLE_SPEED + phase * 6.3f);
    m *= 0.5f + 0.5f * sinf(u * BEAM_SIZZLE_FREQ * 2.7f * TAU - gTime * BEAM_SIZZLE_SPEED * 0.7f + phase * 2.1f);
    return 1.f - BEAM_SIZZLE_DEPTH * (1.f - m);
}

// Emit one shell of smoke puffs on the sphere of `radius` around `origin` (random points
// over the sphere), each drifting slightly outward. Called repeatedly as a shockwave sphere
// expands, so the accumulated shells read as a fast smoke wall sweeping outward.
void spawnShockSmoke(vector3 origin, float radius, float scale) {
    for (int k = 0; k < SHOCK_SMOKE_COUNT; k++) {
        float a = frand() * TAU;
        float z = frand2();                 // -1..1 (uniform over the full sphere)
        float r = sqrtf(1.f - z * z);
        vector3 dir = { r * cosf(a), z, r * sinf(a) };
        Particle& p = gParticles[gCursor];
        gCursor = (gCursor + 1) % MAX_PARTICLES;
        p.pos = { origin.x + dir.x * radius, origin.y + dir.y * radius, origin.z + dir.z * radius };
        float spd = SHOCK_SMOKE_SPEED * scale * (0.5f + frand() * 0.7f);
        p.vel = { dir.x * spd, dir.y * spd, dir.z * spd };
        p.age = 0.f;
        p.life = SHOCK_SMOKE_LIFE * (0.8f + frand() * 0.4f);
        p.fadeStart = 0.f;                  // auto fade-in then out (see PK_SMOKE draw)
        p.seed = frand() * TAU;
        p.alive = true;
        p.kind = PK_SMOKE;
        p.maxSize = SHOCK_SMOKE_SIZE * scale;
        unsigned char g = (unsigned char)(30 + GetRandomValue(0, 40));
        p.cr = g; p.cg = g; p.cb = (unsigned char)(g + 4);
        p.ca = SHOCK_SMOKE_ALPHA;
    }
}

// A huge smoke cloud that blooms at `pos` (filling a sphere of world radius `scale`) and
// dissipates quickly. Puffs are seeded uniformly through the volume with a gentle outward
// drift, and fade out over most of their short life (holdFade), so the cloud clears fast.
void spawnSmokeCloud(vector3 pos, float scale) {
    for (int k = 0; k < CLOUD_BATCH; k++) {
        float a = frand() * TAU;
        float z = frand2();                     // -1..1 (uniform over the sphere)
        float r = sqrtf(1.f - z * z);
        vector3 dir = { r * cosf(a), z, r * sinf(a) };
        float rad = scale * powf(frand(), 0.3333f); // uniform through the sphere volume
        Particle& p = gParticles[gCursor];
        gCursor = (gCursor + 1) % MAX_PARTICLES;
        p.pos = { pos.x + dir.x * rad, pos.y + dir.y * rad, pos.z + dir.z * rad };
        float spd = CLOUD_EXPAND * scale * (0.2f + frand() * 0.8f);
        p.vel = { dir.x * spd, dir.y * spd, dir.z * spd };
        p.age = 0.f;
        p.life = CLOUD_LIFE * (0.7f + frand() * 0.6f);
        p.fadeStart = p.life * CLOUD_HOLD;      // brief hold, then clears over the rest of its life
        p.seed = frand() * TAU;
        p.alive = true;
        p.kind = PK_SMOKE;
        p.maxSize = CLOUD_PUFF;
        if (frand() < 0.85f) {                  // mostly dark sooty smoke
            unsigned char g = (unsigned char)(6 + GetRandomValue(0, 14));
            p.cr = g; p.cg = g; p.cb = g;
            p.ca = (unsigned char)(CLOUD_ALPHA + 20);
        } else {                                // lighter grey
            unsigned char g = (unsigned char)(38 + GetRandomValue(0, 26));
            p.cr = g; p.cg = g; p.cb = (unsigned char)(g + 6);
            p.ca = CLOUD_ALPHA;
        }
    }
}

void setParticleLights(int count, const float* pos, const float* color, const float* radius) {
    if (count < 0) count = 0;
    if (count > PL_MAX) count = PL_MAX; // ignore lights past our cap
    gPLCount = count;
    if (count > 0) {
        memcpy(gPLPos,   pos,   sizeof(float) * 3 * count);
        memcpy(gPLColor, color, sizeof(float) * 3 * count);
        memcpy(gPLRadius, radius, sizeof(float) * count);
    }
}

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

    /* gSmokeShader = LoadShader(0, "resources/shaders/smoke.fs"); */

    // Hard sprite: a crisp disc (solid core, tiny AA rim) for sharp-edged twinkles.
    Color* pxh = (Color*)MemAlloc(S * S * sizeof(Color));
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            float dx = (x - c) / c, dy = (y - c) / c;
            float d = sqrtf(dx * dx + dy * dy);
            float a = (d < 0.80f) ? 1.f : (d < 0.95f) ? (1.f - (d - 0.80f) / 0.15f) : 0.f;
            pxh[y * S + x] = (Color){ 255, 255, 255, (unsigned char)(a * 255.f) };
        }
    }
    Image imgh = { pxh, S, S, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    gHard = LoadTextureFromImage(imgh);
    SetTextureFilter(gHard, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imgh);

    // Star sprite: a tight bright core plus four thin diffraction spikes along the
    // axes (a classic twinkle). Alpha = core + spikes, each tapering to the rim.
    Color* pxs = (Color*)MemAlloc(S * S * sizeof(Color));
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            float dx = (x - c) / c, dy = (y - c) / c;
            float d = sqrtf(dx * dx + dy * dy);
            float r = d > 1.f ? 1.f : d;
            float ax = fabsf(dx), ay = fabsf(dy);
            float taper = 1.f - r;                 // 1 center .. 0 rim
            float core = taper * taper * taper;    // small, intense round core
            float sh = fmaxf(0.f, 1.f - ay / 0.045f); sh *= sh * taper * taper; // horizontal ray
            float sv = fmaxf(0.f, 1.f - ax / 0.045f); sv *= sv * taper * taper; // vertical ray
            float a = core + 0.9f * (sh + sv);
            if (a > 1.f) a = 1.f;
            pxs[y * S + x] = (Color){ 255, 255, 255, (unsigned char)(a * 255.f) };
        }
    }
    Image imgs = { pxs, S, S, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    gStar = LoadTextureFromImage(imgs);
    SetTextureFilter(gStar, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imgs);

    gInit = true;
}

int spawnBurst(vector3 pos, float maxSize, float life, Color color) {
    if (!gInit) return -1;
    int idx = gCursor;
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES; // shares the ring with fire particles
    p.pos  = pos;
    p.vel  = {0.f, 0.f, 0.f};
    p.age  = 0.f;
    p.life = life > 0.f ? life : 0.6f;
    p.seed = 0.f;
    p.alive = true;
    p.kind = PK_BURST;
    p.maxSize = maxSize;
    p.fadeStart = 0.f; // burst uses the growDecay envelope, not the hold-fade
    p.cr = color.r; p.cg = color.g; p.cb = color.b; p.ca = color.a;
    return idx;
}

void spawnWisp(vector3 pos, Color color, float sizeScale) {
    if (!gInit) return;
    Particle& p = gParticles[gCursor];
    gCursor = (gCursor + 1) % MAX_PARTICLES;
    p.pos  = pos;
    p.vel  = { frand2() * 0.5f, frand() * 0.5f, frand2() * 0.5f }; // slight drift (stays around the corona)
    p.age  = 0.f;
    p.life = 0.4f + frand() * 0.4f;
    p.seed = frand() * TAU;
    p.alive = true;
    p.kind = PK_WISP;
    p.maxSize = (0.18f + frand() * 0.14f) * sizeScale; // star motes (× caller scale)
    p.fadeStart = 0.f;
    p.cr = color.r; p.cg = color.g; p.cb = color.b; p.ca = color.a;
}

void spawnExplosion(vector3 pos, float scale) {
    if (!gInit) return;

    // The impact glow corona: a fast-growing, slow-shrinking orange flash (~2s).
    spawnBurst(pos, 3.5f * scale, 2.0f, (Color){ 255, 140, 40, 255 });

    // A ONE-SHOT batch: a ton of fire + smoke spawned instantly, pre-spread through
    // a sphere so they exist immediately, then fade out (no continuous emission).
    Emitter batch = {};
    batch.pos = pos;
    batch.scale = scale;
    for (int k = 0; k < FIRE_BATCH; k++)  emitExplosionFire(batch);
    for (int k = 0; k < SMOKE_BATCH; k++) emitExplosionSmoke(batch);

    /*
    for (int i = 0; i < MAX_EMITTERS; i++) {
        if (!gEmit[i].used) {
            gEmit[i] = {};
            gEmit[i].used = true;
            gEmit[i].active = true;
            gEmit[i].pos = pos;
            gEmit[i].kind = EK_EXPLOSION;
            gEmit[i].hold = EXPL_HOLD;
            gEmit[i].taper = EXPL_TAPER;
            gEmit[i].scale = scale;
            break;
        }
    }
    */

    // A very bright transient point light, following the same grow/decay envelope.
    for (int i = 0; i < MAX_EXP_LIGHTS; i++) {
        if (!gExpLights[i].used) {
            gExpLights[i].used = true;
            gExpLights[i].pos = pos;
            gExpLights[i].age = 0.f;
            gExpLights[i].life = 2.0f;                 // matches the fireball flash
            gExpLights[i].peakRadius = 16.0f * scale;  // how far the light reaches
            gExpLights[i].peakIntensity = 14.0f;       // very bright at peak
            gExpLights[i].peakCoronaRadius = EXPL_CORONA_RADIUS * scale;
            gExpLights[i].peakCoronaBright = EXPL_CORONA_BRIGHT;
            gExpLights[i].color = { 1.0f, 0.45f, 0.12f }; // warm orange
            break;
        }
    }
}

// Just the SMOKE half of an explosion: the instant one-shot smoke batch at `scale`,
// no fire / corona / light. Used by the heaven sword's impact.
void spawnSmokeBlast(vector3 pos, float scale) {
    if (!gInit) return;
    Emitter batch = {};
    batch.pos = pos;
    batch.scale = scale;
    for (int k = 0; k < SMOKE_BATCH; k++) emitExplosionSmoke(batch);
}

// A burst of fire-coloured vertical rays that shoot up from `pos` and fade out by RAY_LIFE.
void spawnImpactRays(vector3 pos, float scale) {
    if (!gInit) return;
    for (int k = 0; k < RAY_COUNT; k++) {
        Particle& p = gParticles[gCursor];
        gCursor = (gCursor + 1) % MAX_PARTICLES;
        float a  = frand() * TAU;
        float rr = RAY_SPREAD * scale * sqrtf(frand());  // scattered around the impact
        p.pos  = { pos.x + cosf(a) * rr, pos.y, pos.z + sinf(a) * rr };
        p.vel  = { 0.f, 0.f, 0.f };
        p.age  = 0.f;
        p.life = RAY_LIFE;
        p.seed = frand() * TAU;
        p.alive = true;
        p.kind = PK_RAY;
        p.maxSize   = RAY_HEIGHT * scale * (0.6f + 0.8f * frand()); // per-ray height
        p.fadeStart = RAY_WIDTH  * scale * (0.7f + 0.6f * frand()); // per-ray half-width
        // Orangish-red, drawn from the hot end of the fire palette.
        p.cr = 255;
        p.cg = (unsigned char)(50 + GetRandomValue(0, 70));
        p.cb = (unsigned char)(10 + GetRandomValue(0, 20));
        p.ca = 200;
    }
}

// Radial darkblast colour: near-white at the centre (f=0), ultraviolet in the mid ring,
// red at the outer layers (f=1).
static void darkblastColor(float f, unsigned char& r, unsigned char& g, unsigned char& b) {
    // stops: white (235,225,255) -> ultraviolet (150,40,255) -> red (255,30,15)
    if (f < 0.5f) {
        float k = f / 0.5f;
        r = (unsigned char)(235 + (150 - 235) * k);
        g = (unsigned char)(225 + (40  - 225) * k);
        b = 255;
    } else {
        float k = (f - 0.5f) / 0.5f;
        r = (unsigned char)(150 + (255 - 150) * k);
        g = (unsigned char)(40  + (30  - 40)  * k);
        b = (unsigned char)(255 + (15  - 255) * k);
    }
}

// A bundle of darkblast beams drawn FROM `from` (the caster's centre mass) TO `to` (the
// raycast hit). Beams are offset radially around the axis: white centre -> UV -> red edge.
void spawnDarkblast(vector3 from, vector3 to, float scale) {
    if (!gInit) return;
    vector3 axis = to - from;
    float len = axis.mag();
    vector3 adir = len > 1e-4f ? axis.fmult(1.f / len) : vector3{0.f, 0.f, 1.f};
    // A stable basis perpendicular to the beam axis, for the radial offsets.
    vector3 ref = fabsf(adir.y) < 0.9f ? vector3{0.f, 1.f, 0.f} : vector3{1.f, 0.f, 0.f};
    vector3 perp1 = normalize3(cross3(adir, ref));
    vector3 perp2 = cross3(adir, perp1);
    for (int k = 0; k < BEAM_COUNT; k++) {
        Particle& p = gParticles[gCursor];
        gCursor = (gCursor + 1) % MAX_PARTICLES;
        float a  = TAU * ((float)k + frand()) / (float)BEAM_COUNT; // even angular spread -> round cylinder
        float f  = sqrtf(frand());               // 0..1 radial fraction (uniform over the disc)
        float rr = BEAM_SPREAD * f;              // filled cross-section (white centre -> red edge)
        vector3 off = (perp1.fmult(cosf(a)) + perp2.fmult(sinf(a))).fmult(rr);
        p.pos  = from + off;                     // A: start (near centre mass)
        p.vel  = to + off;                       // B: end (near the hit point)
        p.age  = 0.f;
        p.life = BEAM_LIFE;
        p.seed = frand() * TAU;
        p.alive = true;
        p.kind = PK_BEAM;
        p.fadeStart = BEAM_WIDTH * scale * (0.7f + 0.6f * frand()); // half-width
        p.maxSize = a; // angular position around the axis (draw spins the quad tangent to it)
        darkblastColor(f, p.cr, p.cg, p.cb);
        p.ca = 210;
    }
}

// A scripted transient point light (grow/decay envelope, no corona) at an arbitrary
// position — for effects that place their own lighting (e.g. the sword impact light).
void spawnPointFlash(vector3 pos, vector3 color, float radius, float intensity, float life) {
    if (!gInit) return;
    for (int i = 0; i < MAX_EXP_LIGHTS; i++) {
        if (!gExpLights[i].used) {
            gExpLights[i].used = true;
            gExpLights[i].pos = pos;
            gExpLights[i].age = 0.f;
            gExpLights[i].life = life;
            gExpLights[i].peakRadius = radius;
            gExpLights[i].peakIntensity = intensity;
            gExpLights[i].peakCoronaRadius = 0.f; // no halo: pure illumination
            gExpLights[i].peakCoronaBright = 0.f;
            gExpLights[i].color = color;
            break;
        }
    }
}

int spawnFireEmitter(vector3 pos) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
        if (!gEmit[i].used) {
            gEmit[i] = {};
            gEmit[i].used = true;
            gEmit[i].active = true;
            gEmit[i].pos = pos;
            gEmit[i].kind = EK_TORCH;
            return i;
        }
    }
    return -1;
}

int spawnCoreEmitter(vector3 pos) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
        if (!gEmit[i].used) {
            gEmit[i] = {};
            gEmit[i].used = true;
            gEmit[i].active = true;
            gEmit[i].pos = pos;
            gEmit[i].kind = EK_CORE;
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

void releaseEmitter(int id) {
    if (id < 0 || id >= MAX_EMITTERS) return;
    gEmit[id].used = false;   // frees the slot; particles it already spawned finish on their own
    gEmit[id].active = false;
}

int explosionLightCount() { return MAX_EXP_LIGHTS; }

// Fill the current (enveloped) state of explosion light i. Returns false for a
// dead slot so the renderer can skip it.
bool explosionLightAt(int i, float* pos, float* color, float* radius) {
    if (i < 0 || i >= MAX_EXP_LIGHTS || !gExpLights[i].used) return false;
    const ExpLight& L = gExpLights[i];
    float env = growDecay(L.age / L.life);
    float k = env * L.peakIntensity;
    pos[0] = L.pos.x; pos[1] = L.pos.y; pos[2] = L.pos.z;
    color[0] = L.color.x * k; color[1] = L.color.y * k; color[2] = L.color.z * k;
    *radius = L.peakRadius; // reach stays constant; brightness is what pulses
    return true;
}

// The impact corona (halo): both its radius and brightness follow the grow/decay
// envelope, so it flashes large then shrinks away. Read by the renderer's corona pass.
bool explosionCoronaAt(int i, float* pos, float* color, float* radius) {
    if (i < 0 || i >= MAX_EXP_LIGHTS || !gExpLights[i].used) return false;
    const ExpLight& L = gExpLights[i];
    float env = growDecay(L.age / L.life);
    float k = env * L.peakCoronaBright;
    pos[0] = L.pos.x; pos[1] = L.pos.y; pos[2] = L.pos.z;
    color[0] = L.color.x * k; color[1] = L.color.y * k; color[2] = L.color.z * k;
    *radius = L.peakCoronaRadius * env; // radius grows then shrinks
    return true;
}

void setWandGlow(bool active, vector3 pos, float intensity01) {
    gWandGlow.active = active;
    gWandGlow.pos = pos;
    gWandGlow.intensity = intensity01;
}

bool wandGlowLightAt(float* pos, float* color, float* radius) {
    if (!gWandGlow.active || gWandGlow.intensity <= 0.001f) return false;
    float k = gWandGlow.intensity * WAND_LIGHT_INTENSITY;
    pos[0] = gWandGlow.pos.x; pos[1] = gWandGlow.pos.y; pos[2] = gWandGlow.pos.z;
    color[0] = k; color[1] = k; color[2] = k; // white
    *radius = WAND_LIGHT_RADIUS;
    return true;
}

bool wandGlowCoronaAt(float* pos, float* color, float* radius) {
    if (!gWandGlow.active || gWandGlow.intensity <= 0.001f) return false;
    float k = gWandGlow.intensity * WAND_CORONA_BRIGHT;
    pos[0] = gWandGlow.pos.x; pos[1] = gWandGlow.pos.y; pos[2] = gWandGlow.pos.z;
    color[0] = k; color[1] = k; color[2] = k; // white
    *radius = WAND_CORONA_RADIUS * gWandGlow.intensity; // small, scales with the envelope
    return true;
}

void updateParticles(float dt) {
    if (dt <= 0.f) return;
    gTime += dt;

    // Emit from every emitter (fractional carry keeps the rate frame-independent).
    for (int i = 0; i < MAX_EMITTERS; i++) {
        Emitter& e = gEmit[i];
        if (!e.used) continue;

        if (e.kind == EK_EXPLOSION) {
            e.age += dt;
            if (e.age >= e.taper) { e.used = false; e.active = false; continue; }
            // full rate until hold, then linearly ramp to 0 by taper.
            float rf = (e.age < e.hold) ? 1.f : 1.f - (e.age - e.hold) / (e.taper - e.hold);
            if (rf < 0.f) rf = 0.f;
            e.accum += EXPL_FIRE_RATE * rf * dt;
            int nf = (int)e.accum; e.accum -= (float)nf;
            for (int k = 0; k < nf; k++) emitExplosionFire(e);
            e.smokeAccum += EXPL_SMOKE_RATE * rf * dt;
            int ns = (int)e.smokeAccum; e.smokeAccum -= (float)ns;
            for (int k = 0; k < ns; k++) emitExplosionSmoke(e);
        } else if (e.kind == EK_CORE) {
            if (!e.active) continue;
            e.accum += CORE_RATE * dt;
            int n = (int)e.accum; e.accum -= (float)n;
            for (int k = 0; k < n; k++) emitCore(e);
        } else { // EK_TORCH
            if (!e.active) continue;
            e.accum += EMIT_RATE * dt;
            int n = (int)e.accum; e.accum -= (float)n;
            for (int k = 0; k < n; k++) emitOne(e);
        }
    }

    // Age the transient explosion point lights.
    for (int i = 0; i < MAX_EXP_LIGHTS; i++) {
        if (!gExpLights[i].used) continue;
        gExpLights[i].age += dt;
        if (gExpLights[i].age >= gExpLights[i].life) gExpLights[i].used = false;
    }

    // Integrate each particle according to its kind.
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = gParticles[i];
        if (!p.alive) continue;
        p.age += dt;
        if (p.age >= p.life) { p.alive = false; continue; }

        if (p.kind == PK_BURST) continue;            // one-shot puff: stays put (envelope in draw)
        if (p.kind == PK_RAY || p.kind == PK_BEAM) continue; // static beams: animated in draw

        if (p.kind == PK_WISP) {                      // slight drift/curl, stays near the corona
            float swx = sinf(gTime * 4.0f + p.seed) * 0.3f;
            float swz = cosf(gTime * 3.5f + p.seed) * 0.3f;
            p.pos.x += (p.vel.x + swx) * dt;
            p.pos.y += p.vel.y * dt;
            p.pos.z += (p.vel.z + swz) * dt;
            p.vel.y *= (1.f - 0.6f * dt);
            continue;
        }

        if (p.kind == PK_SMOKE) {                    // fast outward burst, damps symmetrically -> bounded sphere
            float d = 1.f - EXPL_DRAG * dt; if (d < 0.f) d = 0.f;
            p.vel.x *= d; p.vel.y *= d; p.vel.z *= d;
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.pos.z += p.vel.z * dt;
            continue;
        }

        // PK_FIRE: buoyancy + swirl that grows with age. Explosion fire (fadeStart>0)
        // skips buoyancy and uses gentler swirl so it stays a round, uniformly
        // expanding ball instead of rising into a mushroom.
        bool expl = (p.fadeStart > 0.f);
        if (!expl) p.vel.y += BUOYANCY * dt; // torch/core fire rises
        else { float d = 1.f - EXPL_DRAG * dt; if (d < 0.f) d = 0.f; p.vel.x *= d; p.vel.y *= d; p.vel.z *= d; } // explosion fire: fast burst, then drag stops it within the corona
        float sw = expl ? (SWIRL * 0.3f) : SWIRL;
        float swx = sinf(gTime * 3.0f + p.seed) * sw * p.age;
        float swz = cosf(gTime * 2.6f + p.seed) * sw * p.age;
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

    // One camera-facing quad, sized by half-extent sz, at world position pos.
    auto emitQuad = [&](const vector3& pos, float sz, unsigned char r, unsigned char g,
                        unsigned char b, unsigned char a) {
        vector3 rx = { right.x * sz, right.y * sz, right.z * sz };
        vector3 uy = { up.x * sz, up.y * sz, up.z * sz };
        vector3 tl = { pos.x - rx.x + uy.x, pos.y - rx.y + uy.y, pos.z - rx.z + uy.z };
        vector3 bl = { pos.x - rx.x - uy.x, pos.y - rx.y - uy.y, pos.z - rx.z - uy.z };
        vector3 br = { pos.x + rx.x - uy.x, pos.y + rx.y - uy.y, pos.z + rx.z - uy.z };
        vector3 tr = { pos.x + rx.x + uy.x, pos.y + rx.y + uy.y, pos.z + rx.z + uy.z };
        rlColor4ub(r, g, b, a);
        rlTexCoord2f(0.f, 0.f); rlVertex3f(tl.x, tl.y, tl.z);
        rlTexCoord2f(0.f, 1.f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.f, 1.f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.f, 0.f); rlVertex3f(tr.x, tr.y, tr.z);
    };

    // Same billboard quad, but the in-plane basis is rotated by ang (radians) so a
    // star sprite can spin in screen space as it twinkles.
    auto emitQuadRot = [&](const vector3& pos, float sz, float ang, unsigned char r,
                           unsigned char g, unsigned char b, unsigned char a) {
        float cs = cosf(ang), sn = sinf(ang);
        vector3 rr = { right.x * cs + up.x * sn, right.y * cs + up.y * sn, right.z * cs + up.z * sn };
        vector3 uu = { up.x * cs - right.x * sn, up.y * cs - right.y * sn, up.z * cs - right.z * sn };
        vector3 rx = { rr.x * sz, rr.y * sz, rr.z * sz };
        vector3 uy = { uu.x * sz, uu.y * sz, uu.z * sz };
        vector3 tl = { pos.x - rx.x + uy.x, pos.y - rx.y + uy.y, pos.z - rx.z + uy.z };
        vector3 bl = { pos.x - rx.x - uy.x, pos.y - rx.y - uy.y, pos.z - rx.z - uy.z };
        vector3 br = { pos.x + rx.x - uy.x, pos.y + rx.y - uy.y, pos.z + rx.z - uy.z };
        vector3 tr = { pos.x + rx.x + uy.x, pos.y + rx.y + uy.y, pos.z + rx.z + uy.z };
        rlColor4ub(r, g, b, a);
        rlTexCoord2f(0.f, 0.f); rlVertex3f(tl.x, tl.y, tl.z);
        rlTexCoord2f(0.f, 1.f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.f, 1.f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.f, 0.f); rlVertex3f(tr.x, tr.y, tr.z);
    };

    // --- Pass 1: SMOKE (simple alpha-blended dark cloud, drawn before the fire).
    BeginBlendMode(BLEND_ALPHA);
    rlSetTexture(gSoft.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind != PK_SMOKE) continue;
        float t = p.age / p.life;
        float sz = p.maxSize * 0.5f;
        float fade;
        if (p.fadeStart > 0.f) fade = holdFade(p.age, p.fadeStart, p.life);
        else { float fin = t < 0.15f ? (t / 0.15f) : 1.f; fade = fin * (1.f - t); }
        float a = fade * (float)p.ca;
        if (a < 1.f) continue;
        // Light the smoke by the frame's point lights so it blends with lit ground/objects
        // instead of reading as a flat dark blob.
        float lr, lg, lb; lightAt(p.pos, lr, lg, lb);
        int rr = p.cr + (int)(lr * SMOKE_LIGHT_GAIN); if (rr > 255) rr = 255;
        int gg = p.cg + (int)(lg * SMOKE_LIGHT_GAIN); if (gg > 255) gg = 255;
        int bb = p.cb + (int)(lb * SMOKE_LIGHT_GAIN); if (bb > 255) bb = 255;
        emitQuad(p.pos, sz, (unsigned char)rr, (unsigned char)gg, (unsigned char)bb, (unsigned char)a);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    EndBlendMode();

    // --- Pass 2: FIRE + BURST (soft additive glow).
    BeginBlendMode(BLEND_ADDITIVE);
    rlSetTexture(gSoft.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind == PK_SMOKE || p.kind == PK_WISP || p.kind == PK_RAY || p.kind == PK_BEAM) continue;
        float t = p.age / p.life;

        float sz;
        unsigned char r, g, b, a;
        if (p.kind == PK_BURST) {
            // Fast attack to full size, then a slow eased shrink. Alpha tracks the
            // same envelope so it flashes bright and fades as it shrinks.
            float env = growDecay(t);
            sz = p.maxSize * env * 0.5f;                 // half-extent
            r = p.cr; g = p.cg; b = p.cb;
            a = (unsigned char)(env * (float)p.ca);
        } else { // PK_FIRE
            sz = (SIZE_START + (SIZE_END - SIZE_START) * t) * 0.5f * p.maxSize;
            // Per-particle heat bias: some embers run cooler (redder) or hotter
            // (whiter) at the same age, so the fire isn't a uniform colour wash.
            float heat = (p.seed * (1.f / TAU) - 0.5f) * 0.35f;
            float ct = t + heat; ct = ct < 0.f ? 0.f : (ct > 1.f ? 1.f : ct);
            fireColor(ct, r, g, b);
            float fade;
            if (p.fadeStart > 0.f) fade = holdFade(p.age, p.fadeStart, p.life);
            else { float fin = t < 0.1f ? (t / 0.1f) : 1.f; fade = fin * (1.f - t) * (1.f - t); }
            // Fast per-particle flicker so the flames shimmer instead of sitting still.
            float flick = 0.80f + 0.20f * sinf(gTime * 26.f + p.seed * 6.3f);
            a = (unsigned char)(fade * flick * BASE_ALPHA);
        }
        if (a == 0) continue;
        emitQuad(p.pos, sz, r, g, b, a);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();

    // --- Pass 3: WISPS (star sprite -> spinning, scintillating twinkles).
    /*
    rlSetTexture(gHard.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind != PK_WISP) continue;
        float t = p.age / p.life;
        float grow = t < 0.3f ? (t / 0.3f) : 1.f;        // quick grow-in
        float sz = p.maxSize * 0.5f * (0.5f + 0.5f * grow);
        float fin  = t < 0.2f ? (t / 0.2f) : 1.f;
        float fade = fin * (1.f - t) * (1.f - t);
        float flick = 0.6f + 0.4f * sinf(gTime * 30.f + p.seed * 8.f); // bright twinkle
        unsigned char a = (unsigned char)(fade * flick * (float)p.ca);
        if (a == 0) continue;
        emitQuad(p.pos, sz, p.cr, p.cg, p.cb, a);
    }
    rlEnd();
    */
    rlSetTexture(gStar.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind != PK_WISP) continue;
        float t = p.age / p.life;
        float grow = t < 0.3f ? (t / 0.3f) : 1.f;        // quick grow-in
        float fin  = t < 0.2f ? (t / 0.2f) : 1.f;
        float fade = fin * (1.f - t) * (1.f - t);
        // Scintillation: sits dim, flashes bright (squared sine), per-particle phase.
        float tw = 0.5f + 0.5f * sinf(gTime * 16.f + p.seed * 8.f);
        tw *= tw;
        float flick = 0.55f + 0.90f * tw;                // brighter floor + overdrive peak (additive clamps)
        float sz = p.maxSize * 0.5f * (0.6f + 0.4f * grow) * (1.0f + 0.45f * tw); // bigger, flares larger when bright
        float ang = p.seed + gTime * 1.6f;               // slow spin so the spikes turn
        unsigned char a = (unsigned char)(fade * flick * (float)p.ca);
        if (a == 0) continue;
        emitQuadRot(p.pos, sz, ang, p.cr, p.cg, p.cb, a);
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();

    // --- Pass 4: RAYS (vertical fire beams shooting up from a sword impact). Cylindrical
    // billboards: stand along world +Y, width faces the camera; bright at the base, fading
    // to transparent at the top (per-vertex alpha), fading out over life.
    vector3 rayRight = normalize3(cross3(fwd, { 0.f, 1.f, 0.f })); // horizontal, faces camera
    rlSetTexture(gSoft.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind != PK_RAY) continue;
        float t = p.age / p.life;
        float shoot = p.age < RAY_SHOOT_TIME ? (p.age / RAY_SHOOT_TIME) : 1.f; // grow up fast
        float h = p.maxSize * shoot;                    // current height
        float w = p.fadeStart;                          // half-width (stored per ray)
        float bright = (1.f - t) * (1.f - t);           // brightest at spawn, 0 at life
        float flick = 0.8f + 0.2f * sinf(gTime * 20.f + p.seed * 6.3f);
        float af = bright * flick * (float)p.ca;
        if (af < 1.f) continue;
        unsigned char ab = (unsigned char)(af > 255.f ? 255.f : af);
        // Core, plus an optional wider/dimmer bloom halo (adjustable via RAY_BLOOM).
        int layers = RAY_BLOOM > 0.f ? 2 : 1;
        for (int L = 0; L < layers; L++) {
            float lw = (L == 0) ? w : w * (1.f + RAY_BLOOM);
            unsigned char base = (L == 0) ? ab : (unsigned char)(ab * BLOOM_ALPHA);
            if (base == 0) continue;
            vector3 rx = { rayRight.x * lw, rayRight.y * lw, rayRight.z * lw };
            vector3 bl = { p.pos.x - rx.x, p.pos.y,     p.pos.z - rx.z };
            vector3 br = { p.pos.x + rx.x, p.pos.y,     p.pos.z + rx.z };
            vector3 tr = { p.pos.x + rx.x, p.pos.y + h, p.pos.z + rx.z };
            vector3 tl = { p.pos.x - rx.x, p.pos.y + h, p.pos.z - rx.z };
            rlColor4ub(p.cr, p.cg, p.cb, base);
            rlTexCoord2f(0.f, 0.5f); rlVertex3f(bl.x, bl.y, bl.z);
            rlTexCoord2f(1.f, 0.5f); rlVertex3f(br.x, br.y, br.z);
            rlColor4ub(p.cr, p.cg, p.cb, 0);
            rlTexCoord2f(1.f, 0.5f); rlVertex3f(tr.x, tr.y, tr.z);
            rlTexCoord2f(0.f, 0.5f); rlVertex3f(tl.x, tl.y, tl.z);
        }
    }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();

    // --- Pass 5: DARKBLAST BEAMS (directed segments A=pos -> B=vel). Each quad is spun
    // around the central axis (its width runs TANGENT to the bundle circle at the beam's
    // stored angle), so the beams together form a CYLINDER. Subdivided along the length so
    // each piece's brightness can crackle (the sizzle). The soft glow is a separate pass.
    auto emitBeamQuad = [&](const vector3& A, const vector3& B, const vector3& wdir, float w,
                            unsigned char r, unsigned char g, unsigned char b, unsigned char al) {
        vector3 rx = wdir.fmult(w);
        vector3 a0 = A - rx, a1 = A + rx, b0 = B - rx, b1 = B + rx;
        rlColor4ub(r, g, b, al);
        rlTexCoord2f(0.f, 0.5f); rlVertex3f(a0.x, a0.y, a0.z);
        rlTexCoord2f(1.f, 0.5f); rlVertex3f(a1.x, a1.y, a1.z);
        rlTexCoord2f(1.f, 0.5f); rlVertex3f(b1.x, b1.y, b1.z);
        rlTexCoord2f(0.f, 0.5f); rlVertex3f(b0.x, b0.y, b0.z);
    };
    rlSetTexture(gSoft.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = gParticles[i];
        if (!p.alive || p.kind != PK_BEAM) continue;
        float hold = 0.75f * p.life;
        float bright = p.age <= hold ? 1.f : (p.age >= p.life ? 0.f : 1.f - (p.age - hold) / (p.life - hold));
        float baseA = bright * (float)p.ca;
        if (baseA < 1.f) continue;
        vector3 A = p.pos, B = p.vel;
        vector3 axis = B - A;
        float len = axis.mag(); if (len < 1e-4f) continue;
        vector3 adir = axis.fmult(1.f / len);
        // Rebuild the same perpendicular basis spawnDarkblast used, then take the tangent
        // at this beam's stored angle so the quad lies on the cylinder wall.
        vector3 ref = fabsf(adir.y) < 0.9f ? vector3{0.f,1.f,0.f} : vector3{1.f,0.f,0.f};
        vector3 perp1 = normalize3(cross3(adir, ref));
        vector3 perp2 = cross3(adir, perp1);
        float ang = p.maxSize;
        vector3 wdir = perp1.fmult(-sinf(ang)) + perp2.fmult(cosf(ang)); // tangent (unit) -> cylinder wall
        // Subdivide along the length; each piece's brightness crackles (sizzle).
        for (int s = 0; s < BEAM_SEGMENTS; s++) {
            float u0 = (float)s / (float)BEAM_SEGMENTS;
            float u1 = (float)(s + 1) / (float)BEAM_SEGMENTS;
            float um = 0.5f * (u0 + u1);
            float sa = baseA * darkblastSizzle(um, p.seed);
            unsigned char ab = (unsigned char)(sa > 255.f ? 255.f : (sa < 0.f ? 0.f : sa));
            if (ab == 0) continue;
            emitBeamQuad(A + axis.fmult(u0), A + axis.fmult(u1), wdir, p.fadeStart, p.cr, p.cg, p.cb, ab);
        }
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
    if (gHard.id) UnloadTexture(gHard);
    gHard = {0};
    if (gStar.id) UnloadTexture(gStar);
    gStar = {0};
    /* if (gSmokeShader.id) UnloadShader(gSmokeShader); gSmokeShader = {0}; */
    gInit = false;
}
