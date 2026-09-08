# VAJRA

A playable tactical infiltration game and its engine, written from scratch in
modern C++20 on Linux. No Unreal, no Unity, no Godot.

A lone Indian special-operations officer infiltrates an abandoned Himalayan
research station seized by a stateless arms syndicate. Inspired by the pacing
and patience of Project IGI.

## Status: complete and playable start to finish

Every phase of the technical roadmap is implemented, and a **three-mission
campaign** with persistent progression ships with it.

| Mission | Setting | Hook |
|---|---|---|
| THE QUIET STATION | Daylight compound | Open ground, room to retreat |
| UPLINK | Night ridge corridor | Short sightlines both ways, one drone |
| COLD STORAGE | Indoor robotics lab | Deaf machines with 360° vision and no radio |

Every mission is validated automatically by `ctest`: objectives reachable, no
guard stuck in a wall, every patrol route walkable.

| System | State |
|---|---|
| Renderer | OpenGL 4.6, HDR, Cook-Torrance PBR, shadow maps + PCF, SSAO, bloom, ACES tonemap, FXAA, fog |
| Performance | GPU instancing (whole scene in 1 draw call), frustum culling, depth prepass, thread pool |
| Animation | Procedural articulated actors — walk cycle, aim pose, death slump |
| Particles | Pooled muzzle flashes, impact debris, dust, sparks, blood |
| Tooling | Per-pass CPU profiler on F3, live quality toggles, shader hot-reload |
| Packaging | AppImage script, Flatpak manifest, .desktop entry, install script |
| Physics | AABB world, raycasts, swept character movement with substepping |
| Character | Gravity, crouch with headroom test, sprint, stance-based speed |
| Weapons | Two hitscan weapons, magazines, reload, spread, ADS zoom, recoil |
| AI | Awareness meter, patrol/suspicious/search/combat, A* pathing, squad radio |
| Audio | Fully procedural — gunfire, footsteps, alarms, all synthesised at runtime |
| HUD | Batched 2D overlay with a built-in bitmap font, one draw call |
| Content | JSON levels — new mission is a new file, no recompile |
| Persistence | Save / load |
| Tests | Headless `ctest` suite for physics, navigation and level loading |

## Quick start

```bash
sudo apt install build-essential cmake ninja-build git \
                 libsdl2-dev libgl1-mesa-dev
cmake --preset release
cmake --build --preset release -j$(nproc)
./build/vajra
```

**In CLion:** File → Open → select this folder → pick the **Release** preset →
choose **Vajra** in the run dropdown → press Run. Nothing else to configure;
asset paths are compiled in as absolute paths.

## Documentation

- **[docs/CONTROLS.md](docs/CONTROLS.md)** — controls, the mission, how to win
- **[docs/GAME_DESIGN.md](docs/GAME_DESIGN.md)** — setting, protagonist, systems
- **[docs/ROADMAP.md](docs/ROADMAP.md)** — what is done and what to build next
- **[docs/BUILD.md](docs/BUILD.md)** — dependencies and CLion setup

## Layout

```
Vajra/
├── CMakeLists.txt
├── src/
│   ├── Core/        Application, Window, Timer, Log
│   ├── Renderer/    Shader, Mesh, Camera, ShadowMap, Hud
│   ├── Physics/     AABB collision, raycasts, character sweep
│   ├── AI/          Perception, GuardSystem, Navigation (A*)
│   ├── Gameplay/    CharacterController, Weapon, MissionSystem
│   ├── Audio/       Procedural synthesis engine
│   ├── Scene/       ECS components, JSON level loader
│   └── Input/
├── assets/
│   ├── shaders/     lit, depth, hud
│   └── levels/      quiet_station.json, uplink.json, cold_storage.json
├── packaging/       AppImage, Flatpak, .desktop, install script
├── tools/           fix_levels.py — level placement validator/repair tool
├── tests/           headless smoke test
├── ThirdParty/glad/ vendored GL 4.6 loader
└── docs/
```
