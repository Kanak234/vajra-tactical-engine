# VAJRA

[![CI](https://github.com/Kanak234/vajra-tactical-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/Kanak234/vajra-tactical-engine/actions/workflows/ci.yml)
[![CodeQL](https://github.com/Kanak234/vajra-tactical-engine/actions/workflows/codeql.yml/badge.svg)](https://github.com/Kanak234/vajra-tactical-engine/actions/workflows/codeql.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

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
| Packaging | CPack (TGZ/ZIP), AppImage script, Flatpak manifest, .desktop entry, install script |
| Physics | AABB world, raycasts, swept character movement with substepping |
| Character | Gravity, crouch with headroom test, sprint, stance-based speed |
| Weapons | Two hitscan weapons, magazines, reload, spread, ADS zoom, recoil |
| AI | Awareness meter, patrol/suspicious/search/combat, A* pathing, squad radio |
| Audio | Fully procedural — gunfire, footsteps, alarms, all synthesised at runtime |
| HUD | Batched 2D overlay with a built-in bitmap font, one draw call |
| Content | JSON levels — new mission is a new file, no recompile |
| Persistence | Save / load |
| Tests | 103 test assertions across 5 suites (98.54% measured core line coverage) |

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

## Testing & Quality

Vajra includes a comprehensive test suite covering physics collision, A* navigation, weapon ballistics, mission state transitions, and level deserialization robustness.

```bash
# Standard test run
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target vajra_tests -j$(nproc)
./build/vajra_tests
# or via CTest:
ctest --test-dir build --output-on-failure

# Memory safety & bounds verification (ASan + UBSan)
cmake -B build_san -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build_san --target vajra_tests -j$(nproc)
./build_san/vajra_tests

# Code coverage profiling
cmake -B build_cov -DENABLE_COVERAGE=ON
cmake --build build_cov -j$(nproc)
./build_cov/vajra_tests
```

## Security

Security vulnerabilities and threat models are defined in [SECURITY.md](SECURITY.md). All pull requests are verified against AddressSanitizer, UndefinedBehaviorSanitizer, and GitHub CodeQL static analysis.


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
