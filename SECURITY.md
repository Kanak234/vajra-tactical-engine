# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 0.1.x   | :white_check_mark: |
| < 0.1.0 | :x:                |

## Threat Model & Scope

`vajra-tactical-engine` is a C++20 tactical infiltration game engine. Its security perimeter covers:
- **Asset & Level Deserialization:** Robust parsing of JSON level files, geometry definitions, patrol waypoints, and objectives. Untrusted or malformed JSON payloads must fail safely without buffer overflows, use-after-free, or memory corruption.
- **Memory Safety & Lifetime:** Clean memory ownership semantics across game entities, AABB trees, nav grids, and weapon states. The engine enforces AddressSanitizer and UndefinedBehaviorSanitizer cleanliness in CI.
- **Bounds & Raycast Integrity:** Strict numerical clipping on collision extents and raycasts against invalid IEEE-754 floats (NaN/Inf) or out-of-bound grid cells.
- **Resource Limits:** Protection against denial-of-service through absurdly large level dimensions, cycle loops in pathfinding, or unbounded weapon fire rates.

## Reporting a Vulnerability

If you discover a potential security vulnerability in Vajra Tactical Engine:
1. Do not open a public issue.
2. Email security vulnerability reports directly to the maintainer: `kanakprabhakar2004@gmail.com`.
3. Include detailed steps to reproduce the issue, sample exploit inputs (e.g. malformed level JSON), and system environment details.
4. Vulnerability reports are acknowledged within 48 hours, and patches are coordinated through security advisories.
