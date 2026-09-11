# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-09-11

### Added
- **Comprehensive Unit & Integration Test Suite:**
  - `tests/test_collision.cpp`: AABB properties, ray-AABB intersections, PhysicsWorld line-of-sight and collision resolution.
  - `tests/test_navigation.cpp`: NavGrid cell coordinate mapping, walkability rasterization, and A* pathfinding.
  - `tests/test_gameplay.cpp`: Weapon ballistics, cadence throttling, ADS/crouch spread, ammo management, sidearm mechanics, and MissionSystem objective progressions.
  - `tests/test_level.cpp`: Level JSON loading resilience, geometry/guard/objective validation, and fallback room generation.
- **Production Hardening & Sanitizers:**
  - Added `-DENABLE_ASAN=ON` and `-DENABLE_UBSAN=ON` options in CMake with clean execution (0 errors, 0 undefined behaviors).
  - Added `-DENABLE_COVERAGE=ON` option with measured 98.54% line coverage across core engine logic.
  - Added headless modular target `vajra_core` decoupling game logic from SDL2/OpenGL display dependencies.
- **Security & Analysis:**
  - CodeQL workflow for continuous static C++ vulnerability analysis.
  - Dependabot workflow for automated GitHub Actions dependency updates.
  - Formal `SECURITY.md` policy defining threat models for untrusted level asset deserialization.
- **Containerization & Packaging:**
  - Multi-stage unprivileged `Dockerfile` for headless build, test validation, and execution.
  - CPack packaging configuration for distribution archives (`.tar.gz` and `.zip`).
  - Automated release workflow generating release archives with SHA-256 provenance checksums.
- **Hygiene & Safety:**
  - Added Apache 2.0 `LICENSE`.
  - Comprehensive `.gitignore` covering CMake caches, build artifacts, coverage profiling (`*.gcda`, `*.gcno`, `*.gcov`), and IDE settings.
  - Removed stray root files and stale Python bytecode.
