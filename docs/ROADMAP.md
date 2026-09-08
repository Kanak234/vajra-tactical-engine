# VAJRA — Technical Roadmap

Realistic sequencing for a solo developer. Each phase ends in something you can
run, not just something that compiles.

---

## Phase 0 — Foundation ✅ DONE (this drop)

- [x] CMake project, C++20, ASan/UBSan on Debug
- [x] SDL2 window + OpenGL 4.6 core context, GL debug callback
- [x] GLAD 4.6 loader vendored in `ThirdParty/glad`
- [x] Fixed-timestep loop with interpolation alpha
- [x] Frame-coherent input snapshot (`key` vs `keyPressed`)
- [x] FPS camera: mouselook, walk/sprint/crouch, eye-height blend
- [x] Shader wrapper with **hot reload on F5** and cached uniform locations
- [x] RAII Mesh with cube/plane primitives
- [x] EnTT registry + component set
- [x] Cook-Torrance GGX single-light shader
- [x] Awareness-based perception system with state machine

**Verified:** compiles and links clean under `g++ -std=c++20 -Wall -Wextra`.

---

## Phase 1 — It feels like a game ✅ DONE

- [x] AABB physics world: raycasts, line-of-sight, swept character resolution
- [x] Movement **substepping** so fast motion cannot tunnel through walls
- [x] Character controller: gravity, ground detection, crouch with headroom check,
      acceleration and friction, stance-based speed
- [x] Hitscan weapons: magazines, reload timing, spread by stance and aim,
      recoil, ADS field-of-view zoom, per-weapon noise radius
- [x] Cascade-free directional **shadow map** with 3x3 PCF and slope-scaled bias
- [x] Distance fog
- [x] Batched **HUD** with a built-in 3x5 bitmap font — no font asset, one draw call
- [x] Procedural **audio engine** — gunfire, footsteps, alarms and UI all
      synthesised at runtime, stereo panned and distance attenuated

## Phase 2 — Stealth actually works ✅ DONE

- [x] Grid **navmesh** build from level geometry with agent-radius inflation
- [x] **A\*** with octile heuristic, corner-cut prevention, and line-of-sight
      string pulling
- [x] Guard behaviour: patrol → suspicious → search → combat, driven by the
      awareness meter
- [x] Guards path to your **last known position**, not your actual one
- [x] **Squad radio net** — one guard in contact alerts everyone within 120m
- [x] Muzzle-level line-of-sight test, so guards will not shoot through the
      cover their heads happen to clear
- [x] Directional miss scatter, range-based accuracy falloff, reaction delay
- [x] Noise propagation from footsteps, sprinting, gunfire and sabotage

## Phase 3 — Content pipeline ✅ MOSTLY DONE

- [x] **JSON level format** — geometry, guards, patrol routes, objectives,
      lighting and extraction all data-driven. New mission = new file, no rebuild
- [x] Mission system: objectives, unlock-gated extraction, success and failure
- [x] Save / load to `vajra_save.json`
- [x] Headless **test suite** (`ctest`) covering physics, navigation and loading
- [ ] Lua scripting for triggers and radio dialogue
- [ ] Blender export path for real geometry

## Phase 4 — Presentation ✅ DONE

- [x] **HDR pipeline** — the whole scene renders to RGBA16F; nothing clips to
      white until the final tonemap
- [x] **ACES filmic tonemapping** (Narkowicz fit) with manual sRGB encode
- [x] **Bloom** — soft-knee bright pass, separable 9-tap gaussian collapsed to
      5 fetches, three ping-pong iterations at half resolution
- [x] **SSAO** — 24-sample hemisphere kernel, view-space position reconstructed
      from depth (no position buffer), noise-rotated basis, range check to kill
      halos, 4x4 box blur matched to the noise tile
- [x] **FXAA** anti-aliasing with an early-out on flat areas
- [x] Depth + view-normal **prepass**, which also gives the main pass early-Z
- [x] **GPU instancing** — the entire scene is one draw call for boxes and one
      for particles, regardless of object count
- [x] **Frustum culling** (Gribb-Hartmann plane extraction, box-vs-plane test)
- [x] **Particle system** — muzzle flashes, impact debris, dust, sparks, blood;
      fixed-capacity pool, camera-facing billboards, additive blending
- [x] **Procedural character animation** — articulated figures with a real walk
      cycle, speed-scaled stride, body bob, forward lean, aim pose and death
      slump. No rigged model needed, and every limb is one more instance in the
      same batch, so it costs zero extra draw calls
- [x] Procedural **mountain backdrop** generated from layered hash noise
- [x] Distance fog, screen vignette that tightens when aiming

## Phase 5 — Ship it ✅ DONE

- [x] **Job system** — thread pool with a parallel-for, used for particle
      simulation (the one workload that genuinely benefits)
- [x] **Memory pooling** — the particle pool allocates once at startup and
      recycles via ring allocation; emitting never allocates
- [x] **Profiler** — per-pass CPU timings with rolling averages, live on the
      F3 overlay: frame, sim, AI, shadow, prepass, SSAO, scene, post, HUD
- [x] Runtime quality toggles (F7 SSAO, F8 bloom, F10 FXAA) for low-end machines
- [x] CMake `install()` rules
- [x] **AppImage** build script
- [x] **Flatpak** manifest (sandboxed, no network permission)
- [x] `.desktop` entry and a prefix-configurable install script
- [x] Headless test suite runnable in CI without a display

## What is left, honestly

Every system in the original brief is now implemented. What remains is not
code — it is **content and craft**:

| Remaining | Why it is not code |
|---|---|
| Real 3D models and textures | Needs an artist, or asset store purchases |
| Skeletal animation from mocap | Needs rigged models to skin; the procedural rig covers the gap |
| Voice acting | Needs actors |
| More missions | Needs level design time — but each is just a JSON file |
| Networking | Needs servers and a reason to exist |

## The three things to do next

1. **Play it for an hour and tune.** Guard vision cones, awareness gain rate,
   weapon damage. This is worth more than any new feature.
2. **Build a second mission.** It costs one JSON file. Proving the pipeline
   works twice is what turns this from a demo into an engine.
3. **Assimp + textures.** When you are ready for real art, this is the hook —
   and the instancing path already supports it.

Resist adding new systems. The engine is done. The game is what is left.
