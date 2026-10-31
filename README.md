
# 3113-engine

  

Project 3 - Finished:

  

Short demo in case you get really lost:
[![Watch the video](https://img.youtube.com/vi/HRSTtutvLXU/maxresdefault.jpg)](https://youtu.be/HRSTtutvLXU)


  

I can't begin to explain what I'm doing in this. Half of it is almost object oriented and the other half is just all over the place.

New Features:
=
- More Refined Game Logic Builder
- Better Entities (movable collision boxes)
- Better Physics (The Y is Real)
- Y Collision
- Jumping!
- The rest of the new stuff is basically just art :)

Dependencies:
=

Dependencies live in ``vendor/``, which is gitignored, so after cloning run:

``sh setup-deps.sh`` - raylib

``sh setup-steamworks.sh`` - Steamworks SDK (optional)

Steamworks is picked up automatically by every build script when
``vendor/steamworks`` exists, and the matching ``steam_api`` runtime gets copied
next to the executable. Wrap Steam code in ``#ifdef STEAMWORKS_AVAILABLE`` so
builds without the SDK still work. Headers include as ``<steam/steam_api.h>``.

Building/Running:
=

  

Build with ``make``
Run with ``./TheGame``

  

If nothing else works you can try with g++ stuffs

``sh build-mac.sh`` - macos

  

``sh buil-mingwd.sh`` - windows mingw

Smoke rendering and profiling:
=
Smoke uses a half-resolution off-screen target by default, with scene-depth occlusion,
soft intersections, and depth-aware upsampling. Fire, sparks, rays, and beams render
at full resolution. The particle count is the same in both smoke modes.

Build and profile on macOS:

```sh
ENGINE_PROFILE=1 sh build-mac.sh
```

After closing the game, compare the full-resolution smoke path with the same executable:

```sh
ENGINE_PROFILE=1 ENGINE_SMOKE_HALF_RES=0 ./therewillnotbeafoldernamedthis
```

Use the same camera angle and spell for both runs. The profiler reports the actual
half-resolution smoke target dimensions; `0x0` means that path was inactive during
the sample window. On Windows, use `sh build-mingw.sh` and the `.exe` executable.

The captured scene is presented with a full-screen textured draw. To compare the
framebuffer-blit backend using the same build:

```sh
ENGINE_PROFILE=1 ENGINE_SCENE_COPY=blit ./therewillnotbeafoldernamedthis
```

`warp/copy` is a CPU submission measurement and can include waits for earlier GPU
work; it is not a direct measurement of GPU copy time.
