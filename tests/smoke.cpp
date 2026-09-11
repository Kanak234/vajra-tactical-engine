#include <cstdio>
#include <string>
#include <vector>

#include "AI/Navigation.h"
#include "Physics/Collision.h"
#include "Scene/Level.h"

#ifndef VAJRA_ASSET_DIR
#define VAJRA_ASSET_DIR "assets/"
#endif

using namespace vajra;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& what) {
    std::printf("  [%s] %s\n", condition ? " OK " : "FAIL", what.c_str());
    if (!condition) ++g_failures;
}

/// Builds the collision world exactly the way Application does, including the
/// ground plane, so the test exercises the real configuration.
void buildWorld(const Level& level, PhysicsWorld& world) {
    for (const auto& box : level.boxes)
        if (box.collides) world.addBox(AABB::fromCentre(box.centre, box.halfExtents));
    world.addBox(AABB{{-level.groundSize, -2.0f, -level.groundSize},
                      { level.groundSize,  0.0f,  level.groundSize}});
}

void validateLevel(const std::string& file) {
    std::printf("\n=== %s\n", file.c_str());

    bool ok = false;
    Level level = Level::loadFromFile(std::string(VAJRA_ASSET_DIR) + "levels/" + file, &ok);
    check(ok, "level file parses");
    if (!ok) return;

    check(!level.boxes.empty(), "has geometry");
    check(!level.guards.empty(), "has guards");
    check(!level.objectives.empty(), "has objectives");

    PhysicsWorld world;
    buildWorld(level, world);

    NavGrid nav;
    const float cell = 1.0f;
    const int cells = static_cast<int>(level.groundSize / cell);
    nav.build(world, {-level.groundSize * 0.5f, 0.0f, -level.groundSize * 0.5f},
              cell, cells, cells, 0.42f, 1.7f);

    // The player must not spawn inside geometry.
    {
        const glm::vec3 half{0.32f, 0.9f, 0.32f};
        const AABB body = AABB::fromCentre(level.playerStart + glm::vec3{0.0f, 0.9f, 0.0f}, half);
        bool stuck = false;
        for (const AABB& b : world.boxes())
            if (body.overlaps(b)) { stuck = true; break; }
        check(!stuck, "player start is not inside geometry");
    }

    std::vector<glm::vec3> path;

    // Every mandatory objective must be walkable-to from the spawn.
    for (const auto& objective : level.objectives) {
        if (objective.optional) continue;
        const bool reachable = nav.findPath(level.playerStart, objective.position, path);
        check(reachable, "objective reachable: " + objective.id);
    }

    // And the player must be able to get back out.
    check(nav.findPath(level.playerStart, level.extractionPoint, path),
          "extraction reachable from spawn");

    // Guards must not be walled into a box they can never leave.
    int stuckGuards = 0;
    int unreachablePatrols = 0;
    for (const auto& guard : level.guards) {
        const glm::vec3 half{0.42f, 0.85f, 0.42f};
        const AABB body = AABB::fromCentre(guard.position + glm::vec3{0.0f, 0.9f, 0.0f}, half);
        for (const AABB& b : world.boxes())
            if (body.overlaps(b)) { ++stuckGuards; break; }

        for (const auto& waypoint : guard.patrol)
            if (!nav.findPath(guard.position, waypoint, path)) { ++unreachablePatrols; break; }
    }
    check(stuckGuards == 0, "no guard spawns inside geometry (" +
                            std::to_string(stuckGuards) + " stuck)");
    check(unreachablePatrols == 0, "all patrol routes are walkable (" +
                                   std::to_string(unreachablePatrols) + " broken)");

    // Sanity: a ray straight down from above the spawn should hit the ground.
    const RayHit down = world.raycast(level.playerStart + glm::vec3{0, 12, 0}, {0, -1, 0}, 60.0f);
    check(down.hit, "ground raycast hits");
}

}  // namespace

void runCollisionTests(int& failures);
void runNavigationTests(int& failures);
void runGameplayTests(int& failures);
void runLevelTests(int& failures);

int main() {
    std::printf("========================================\n");
    std::printf("     VAJRA TACTICAL ENGINE TEST SUITE   \n");
    std::printf("========================================\n");

    for (const char* file : {"quiet_station.json", "uplink.json", "cold_storage.json"})
        validateLevel(file);

    runCollisionTests(g_failures);
    runNavigationTests(g_failures);
    runGameplayTests(g_failures);
    runLevelTests(g_failures);

    std::printf("\n========================================\n");
    std::printf("TOTAL TEST RESULT: %s (%d failures)\n", g_failures == 0 ? "ALL CHECKS PASSED" : "FAILURES DETECTED", g_failures);
    std::printf("========================================\n");
    return g_failures == 0 ? 0 : 1;
}
