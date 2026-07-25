---
name: engine-guide
description: Architecture, build steps, and conventions for the 3113-engine (a raylib-based custom 3D game engine). Use when working anywhere in this repo — adding/removing world objects, touching rendering/shadows/physics/collision, the player controller, or the main loop in main.cpp.
---

# 3113-engine

A custom C++17 3D engine built on top of **raylib** (used mainly for the window,
input, audio, and low-level `rlgl` GL calls). Everything above that — the mesh
pipeline, world-to-screen shading, cascaded shadow maps, physics, collision, and
the player controller — is hand-written.

`main.cpp` is currently stripped to an **engine sandbox**: all gameplay is
disabled and only the testing platform spawns. See "Sandbox state" below.

## Build & run

- raylib is a **vendored** dependency in `vendor/raylib` (gitignored). If it's
  missing, run `sh setup-deps.sh` to fetch it for the current platform.
- Build with `make` (cross-platform: MinGW on Windows, X11 on Linux, frameworks
  on macOS). The Makefile prefers `vendor/raylib`, falling back to a system install.
- Run the result: `./TheGame` (`TheGame.exe` on Windows).
- Fallback build scripts: `sh build-mingw.sh` (Windows), `sh build.sh` (Linux),
  `sh build-mac.sh` (macOS).
- **Do not compile on the user's behalf — the user always compiles themselves.**

## Layout

| Path | Role |
|------|------|
| `main.cpp` | Entry point + engine loop: `initialise / update / render / shutdown`, shadow passes, object spawning, shader setup. |
| `util.h` / `util.cpp` | Core types + math: `vector3`, `mtx44`, `meshedObject`, `player`, `world`, `gameData`; matrix math, OBJ loading (`objToQuads`), `canInteract`. |
| `draw/` | GPU draw calls (`Draw3DGPU`, `Draw3DDepthGPU`, `DrawColliderGPU`) and the GUI (`gui.cpp`). |
| `physics/` | `processPhysics`, `buildWorld`, collision (`spherePlaneCollide`), `car.cpp`, engine-sound glue (`sounds.cpp`). |
| `game/` | Gameplay: `player.cpp` (controller: `movePlayer`, `moveLook`) and `game.cpp` (`levelLogic`: gas pump, drank, buying, menus). |
| `camera/` | Camera/view math. |
| `system/` | Raw keyboard (`getAsyncKeyStateWrapper`) and mouse (`RawMouseGetDelta`) input. |
| `music/` | `engine_synth.cpp` (procedural car engine) + `miniaudio.h`. |
| `resources/` | Models (`.obj` + collider `.obj`), textures, shaders (`w2s.*`, `depth.*`), sounds. |

## Core data structures (util.h)

- **`meshedObject`** — a world object: `mesh` (`triDomMesh`, an array of `tri`),
  `collider`/`cPlaneCount` (a `planeMtx` array), `texo` texture, `pEntity`
  (`physicsEntity` holding `location`/`rot`/`magnitude`), `scale`, `noCull`,
  `emissive` (`vector4`: rgb glow color + `.t` amount 0..1, fed to `uEmissive`;
  `{0,0,0,0}` = no glow; Draw3DGPU adds `uEmissive.rgb * uEmissive.a` to the lit color),
  `bloom` (`vector2`: x = brightness, y = length as a WORLD-space radius). An object
  with `emissive.t > 0` also acts as a physical point light (see bloom step) that
  lights nearby geometry within `bloom.y`.
- **`player`** — `camera`, `pEntity`, `pState` (`playerState`: `inCar`, `noClip`,
  `hasDrank`, `money`, `carFuel`, …), `collider`/`cPlaneCount`, `controls` (WASD).
- **`world`** — array of `worldSegment` (each = one object's collider). Built from
  a `meshedObject*[]` via `buildWorld`. Physics tests the player against this.
- **`gameData`** — global state incl. the `levels` enum (`LEVEL1..LEVEL4`,
  `GAMESTART`, `GAMEINFOMERCIAL`, `GAMEEND`, `GAMEWIN`, `TESTING_ENVIRONMENT`).

## Spawning a world object

Objects are global `static meshedObject`s in `main.cpp`. To add one, wire it
into **all** of these places (grep an existing object like `testingplatforms` to
find every site):

1. Declare the global `static meshedObject myObj;`
2. `initialise()`: `create3dObject(myObj, "model.obj", "collider.obj", true, w2sShader, scale, {x,y,z}, empty, false, 100, {0,0,0});`
   — pass `collider=true` + a collider `.obj` for collidable geometry.
3. If it moves under physics: `initializePhysicsEntity(myObj.pEntity, weight, COMPLEX);`
   and optionally `applyAcceleration({0,-20.5f,0}, myObj.pEntity);` for gravity.
4. `updateColliderLocation(myObj, player1, false);`
5. Add `&myObj` to `worldObjects[]` (and/or `secondaryObjects[]`) and bump the
   count passed to `buildWorld(...)`.
6. Render: add a `Draw3DGPU(myObj, ...)` line in `DrawSceneWithShadows`'s
   `drawScene` lambda, and a `Draw3DDepthGPU(myObj, ...)` in
   `RenderShadowMapPass`'s `drawCasters` if it should cast shadows.
7. `shutdown()`: `free(myObj.mesh.tris); free(myObj.mesh.trisO);` and
   `delete[] myObj.collider; delete[] myObj.colliderO;` if it has a collider.

## Rendering pipeline (per frame, in `render()`)

1. Build the light-space matrices (`BuildLightSpaceMatrix`) and render the scene
   depth into two cascaded shadow maps (`RenderShadowMapPass`, near + far).
2. Build the camera view-projection (`frameVP`) and draw the scene with the
   `w2s` shader (`DrawSceneWithShadows` → `Draw3DGPU` per object). The sun is a
   POINT light: `uLightPos` (world position `lightPos`) + `uLightRange` falloff,
   shaded per-pixel. Shader uniforms (light pos/range/color, ambient, extra point
   lights, fade, drunkenness, cascade split) are
   set here.
3. Emissive light + bloom (physical):
   - **Illumination**: emissive objects (`gEmitters[]` in main.cpp) are registered
     as point lights each frame in `DrawSceneWithShadows` — position = object
     location, colour = `emissive.rgb * emissive.a * bloom.x`, radius = `bloom.y`
     (WORLD units). The shader's existing `uPointPos/uPointColor/uPointRadius` loop
     lights nearby geometry, so an object's emission spills onto things near it,
     camera-independent.
   - **Glow/bloom**: drawn as analytic light CORONAS, not a screen blur. For each
     emitter, `render()` projects its world position + `bloom.y` (world radius) to a
     screen center + pixel radius, then `glow.fs` draws an additive radial falloff
     (`color * (1 - d/r)^2`). Peak brightness = colour (constant, camera-independent)
     and size is world-relative (perspective) — and it's perfectly smooth/circular
     because nothing is sampled. Colour = `emissive.rgb * emissive.a * bloom.x *
     bloomIntensity`. `glow.fs` is `#version 330` (raylib default vertex shader).
   - NOTE: the older screen-space blur (`emissiveRT` + `bloom.fs`, `uEmissiveOnly`/
     `uBloomFocal`/`uBloomMax` in w2s) is still loaded but UNUSED — the corona pass
     replaced it (a blur's brightness scales with the source's screen coverage, so it
     got brighter as the camera approached; the corona doesn't).
4. 2D GUI is drawn under an ortho projection (menus, timers, overlays).

Shaders live in `resources/shaders/`: `w2s.*` (main lit/shadowed pass) and
`depth.*` (shadow depth pass). Shader uniform locations are cached into
`w2sShader` (`shaderStore`) in `initialise()`.

## Physics & collision

- `initializePhysicsEntity` sets up a body; `applyAcceleration` adds a persistent
  accel (e.g. gravity `{0,-20.5,0}`).
- `processPhysics(dt, 0, entity, world, ...)` integrates and resolves collisions
  against the `world`'s colliders each frame.
- Colliders are `planeMtx` quads loaded from a separate collider `.obj`;
  `updateColliderLocation` moves them to follow the object.

## Input

- Keyboard: `getAsyncKeyStateWrapper(KEY_*)` (raw, edge-detect with a `canX` bool
  latch — see existing code for the pattern).
- Mouse look: `RawMouseGetDelta(dx, dy)` feeding `moveLook`.

## Conventions

- **Disable code with `/* ... */` block comments, never `#if 0` and never
  deletion.** Keep the original lines intact inside the comment so they can be
  restored. Do not leave explanatory comments around them.
- Objects, sounds, and shaders are global `static`s in `main.cpp`, created in
  `initialise()` and freed in `shutdown()`.
- Block comments do not nest — before wrapping a region, confirm it contains no
  `/*` or `*/`.

## Sandbox state (current)

`main.cpp` has all game logic block-commented out. Active: window/audio/shader
init, the player controller (`moveLook` + `movePlayer`), gravity + `processPhysics`
against a world containing only `testingplatforms`, and rendering of the platform.
Disabled (inside `/* */`): the uber system, gas-pump/drank/buying `levelLogic`,
level timers, fail/win transitions, all menus, background music, the car, and
every non-platform object. `gData.currentLevel` boots to `TESTING_ENVIRONMENT`.

To bring gameplay back, un-comment the `/* */` blocks in `main.cpp`
(`initialise`, `update`, `RenderShadowMapPass`, `DrawSceneWithShadows`, `render`)
and restore the original `worldObjects`/`secondaryObjects` lists.
