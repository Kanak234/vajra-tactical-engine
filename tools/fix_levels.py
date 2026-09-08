#!/usr/bin/env python3
"""Level placement fixer.

Mirrors the C++ NavGrid walkability test exactly, then snaps any guard spawn or
patrol waypoint that ended up inside geometry to the nearest free position.

Hand-placing dozens of guards across several levels guarantees a few end up in
a wall. Catching that at author time beats discovering it when a guard stands
frozen inside a crate mid-playtest.

Usage:  python3 tools/fix_levels.py assets/levels/*.json
"""
import json
import math
import sys

AGENT_RADIUS = 0.42
AGENT_HEIGHT = 1.70
PROBE_LIFT   = 0.05          # matches NavGrid::build


def overlaps(a_min, a_max, b_min, b_max):
    """Strict inequality, matching AABB::overlaps in Collision.h — touching is
    not overlapping, otherwise standing on the floor counts as a collision."""
    return all(a_min[i] < b_max[i] and a_max[i] > b_min[i] for i in range(3))


def build_colliders(level):
    boxes = []
    for b in level.get("boxes", []):
        if not b.get("collides", True):
            continue
        c, h = b["centre"], b["halfExtents"]
        boxes.append(([c[i] - h[i] for i in range(3)],
                      [c[i] + h[i] for i in range(3)]))
    return boxes


def blocked(x, z, boxes):
    cy = AGENT_HEIGHT * 0.5 + PROBE_LIFT
    half = (AGENT_RADIUS, AGENT_HEIGHT * 0.5, AGENT_RADIUS)
    a_min = (x - half[0], cy - half[1], z - half[2])
    a_max = (x + half[0], cy + half[1], z + half[2])
    return any(overlaps(a_min, a_max, lo, hi) for lo, hi in boxes)


def nearest_free(x, z, boxes, max_radius=14.0, step=0.5):
    """Spiral outward for the closest unblocked point."""
    if not blocked(x, z, boxes):
        return x, z, 0.0

    r = step
    while r <= max_radius:
        samples = max(8, int(2 * math.pi * r / step))
        best = None
        for i in range(samples):
            angle = 2 * math.pi * i / samples
            nx, nz = x + math.cos(angle) * r, z + math.sin(angle) * r
            if not blocked(nx, nz, boxes):
                d = math.hypot(nx - x, nz - z)
                if best is None or d < best[2]:
                    best = (nx, nz, d)
        if best:
            return best
        r += step
    return None


def build_grid(level, boxes, cell=1.0):
    """Occupancy grid matching NavGrid::build, plus flood-fill helpers."""
    size = level.get("groundSize", 140.0)
    origin = -size * 0.5
    n = int(size / cell)
    free = [[not blocked(origin + (x + 0.5) * cell, origin + (z + 0.5) * cell, boxes)
             for x in range(n)] for z in range(n)]
    return free, origin, cell, n


def to_cell(x, z, origin, cell, n):
    cx = int((x - origin) / cell)
    cz = int((z - origin) / cell)
    if 0 <= cx < n and 0 <= cz < n:
        return cx, cz
    return None


def flood(free, start, n):
    """All cells reachable from start with 8-way movement (no corner cutting)."""
    if start is None or not free[start[1]][start[0]]:
        return set()
    seen = {start}
    stack = [start]
    while stack:
        x, z = stack.pop()
        for dx in (-1, 0, 1):
            for dz in (-1, 0, 1):
                if dx == 0 and dz == 0:
                    continue
                nx, nz = x + dx, z + dz
                if not (0 <= nx < n and 0 <= nz < n):
                    continue
                if not free[nz][nx] or (nx, nz) in seen:
                    continue
                if dx and dz and not (free[z][nx] and free[nz][x]):
                    continue          # do not squeeze through a diagonal gap
                seen.add((nx, nz))
                stack.append((nx, nz))
    return seen


def largest_region(free, n):
    best, seen_all = set(), set()
    for z in range(n):
        for x in range(n):
            if not free[z][x] or (x, z) in seen_all:
                continue
            region = flood(free, (x, z), n)
            seen_all |= region
            if len(region) > len(best):
                best = region
    return best


def nearest_in_region(x, z, region, origin, cell):
    """Closest cell centre belonging to a given connected region."""
    if not region:
        return None
    best, best_d = None, None
    for cx, cz in region:
        wx = origin + (cx + 0.5) * cell
        wz = origin + (cz + 0.5) * cell
        d = (wx - x) ** 2 + (wz - z) ** 2
        if best_d is None or d < best_d:
            best, best_d = (wx, wz), d
    return best


def fix(path):
    with open(path) as f:
        level = json.load(f)

    boxes = build_colliders(level)
    moved_spawns = 0
    moved_waypoints = 0
    failed = []

    free, origin, cell, n = build_grid(level, boxes)
    main = largest_region(free, n)

    for guard in level.get("guards", []):
        px, py, pz = guard["position"]

        # A guard sealed in a closet is a bug, not a design. Pull any spawn
        # outside the main walkable region back into it.
        cellpos = to_cell(px, pz, origin, cell, n)
        if cellpos is None or cellpos not in main:
            target = nearest_in_region(px, pz, main, origin, cell)
            if target is None:
                failed.append(f"guard spawn ({px:.1f}, {pz:.1f})")
            else:
                guard["position"] = [round(target[0], 2), py, round(target[1], 2)]
                px, pz = target
                moved_spawns += 1

        # Waypoints must be reachable *from that guard*, not merely unblocked.
        region = flood(free, to_cell(px, pz, origin, cell, n), n)
        fixed_route = []
        for wx, wy, wz in guard.get("patrol", []):
            wc = to_cell(wx, wz, origin, cell, n)
            if wc is not None and wc in region:
                fixed_route.append([round(wx, 2), wy, round(wz, 2)])
                continue
            target = nearest_in_region(wx, wz, region, origin, cell)
            if target is None:
                failed.append(f"waypoint ({wx:.1f}, {wz:.1f})")
                fixed_route.append([wx, wy, wz])
            else:
                moved_waypoints += 1
                fixed_route.append([round(target[0], 2), wy, round(target[1], 2)])
        if fixed_route:
            guard["patrol"] = fixed_route

    # Objectives you must stand next to also need clear floor around them.
    for objective in level.get("objectives", []):
        ox, oy, oz = objective["position"]
        if blocked(ox, oz, boxes):
            result = nearest_free(ox, oz, boxes)
            if result:
                objective["position"] = [round(result[0], 2), oy, round(result[1], 2)]
                print(f"    note: nudged objective '{objective['id']}' clear of geometry")

    with open(path, "w") as f:
        json.dump(level, f, indent=1)

    print(f"  {path}: moved {moved_spawns} spawns, {moved_waypoints} waypoints"
          + (f", {len(failed)} UNFIXABLE" if failed else ""))
    for item in failed:
        print(f"    !! could not place {item}")
    return len(failed)


if __name__ == "__main__":
    targets = sys.argv[1:]
    if not targets:
        print(__doc__)
        sys.exit(2)
    print("fixing levels:")
    sys.exit(1 if sum(fix(t) for t in targets) else 0)
