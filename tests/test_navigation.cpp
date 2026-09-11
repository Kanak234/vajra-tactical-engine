#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

#include "AI/Navigation.h"
#include "Physics/Collision.h"

using namespace vajra;

namespace {

void check(bool condition, const std::string& what, int& failures) {
    std::printf("  [%s] %s\n", condition ? " OK " : "FAIL", what.c_str());
    if (!condition) ++failures;
}

} // namespace

void runNavigationTests(int& failures) {
    std::printf("\n=== Navigation & Pathfinding Tests ===\n");

    PhysicsWorld world;
    // Ground plane
    world.addBox(AABB{{-20.0f, -2.0f, -20.0f}, {20.0f, 0.0f, 20.0f}});
    // Wall from x=-5 to x=5 at z=0, height 3
    world.addBox(AABB{{-5.0f, 0.0f, -0.5f}, {5.0f, 3.0f, 0.5f}});

    NavGrid nav;
    nav.build(world, {-20.0f, 0.0f, -20.0f}, 1.0f, 40, 40, 0.4f, 1.8f);

    check(nav.cellsX() == 40, "NavGrid cellsX is 40", failures);
    check(nav.cellsZ() == 40, "NavGrid cellsZ is 40", failures);

    // Coordinate conversions
    int cx = -1, cz = -1;
    bool inBounds = nav.worldToCell({0.0f, 0.0f, 0.0f}, cx, cz);
    check(inBounds && cx == 20 && cz == 20, "Origin maps to cell (20, 20)", failures);

    glm::vec3 worldPos = nav.cellToWorld(20, 20);
    check(std::abs(worldPos.x - 0.5f) < 0.001f && std::abs(worldPos.z - 0.5f) < 0.001f,
          "Cell (20, 20) center maps to world (+0.5, +0.5)", failures);

    // Walkability
    check(!nav.walkable(20, 20), "Cell (20, 20) under wall is not walkable", failures);
    check(nav.walkable(10, 10), "Open cell (10, 10) is walkable", failures);

    // Pathfinding around wall: from (0, 0, -5) to (0, 0, 5)
    std::vector<glm::vec3> path;
    bool found = nav.findPath({0.0f, 0.0f, -5.0f}, {0.0f, 0.0f, 5.0f}, path);
    check(found, "Found path around obstructing wall", failures);
    check(path.size() >= 2, "Path contains waypoints", failures);

    // Start == Goal edge case
    path.clear();
    found = nav.findPath({2.0f, 0.0f, -5.0f}, {2.0f, 0.0f, -5.0f}, path);
    check(found, "Path with start == goal returns true", failures);

    // Completely enclosed target (impossible path)
    PhysicsWorld walledWorld;
    walledWorld.addBox(AABB{{-20.0f, -2.0f, -20.0f}, {20.0f, 0.0f, 20.0f}});
    // Box encircling (0,0)
    walledWorld.addBox(AABB{{-2.0f, 0.0f, -2.0f}, {2.0f, 3.0f, -1.0f}});
    walledWorld.addBox(AABB{{-2.0f, 0.0f, 1.0f}, {2.0f, 3.0f, 2.0f}});
    walledWorld.addBox(AABB{{-2.0f, 0.0f, -2.0f}, {-1.0f, 3.0f, 2.0f}});
    walledWorld.addBox(AABB{{1.0f, 0.0f, -2.0f}, {2.0f, 3.0f, 2.0f}});

    NavGrid trappedNav;
    trappedNav.build(walledWorld, {-10.0f, 0.0f, -10.0f}, 1.0f, 20, 20, 0.4f, 1.8f);
    path.clear();
    found = trappedNav.findPath({-8.0f, 0.0f, -8.0f}, {0.0f, 0.0f, 0.0f}, path);
    check(!found, "Enclosed target returns false for findPath", failures);
}
