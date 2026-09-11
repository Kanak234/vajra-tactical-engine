#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>

#include "Physics/Collision.h"

using namespace vajra;

namespace {

void check(bool condition, const std::string& what, int& failures) {
    std::printf("  [%s] %s\n", condition ? " OK " : "FAIL", what.c_str());
    if (!condition) ++failures;
}

} // namespace

void runCollisionTests(int& failures) {
    std::printf("\n=== Collision & Physics Tests ===\n");

    // 1. AABB Properties & Strict Overlap
    {
        AABB box1 = AABB::fromCentre({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        check(box1.min == glm::vec3(-1.0f, -1.0f, -1.0f), "AABB min calculation", failures);
        check(box1.max == glm::vec3(1.0f, 1.0f, 1.0f), "AABB max calculation", failures);
        check(box1.centre() == glm::vec3(0.0f, 0.0f, 0.0f), "AABB centre calculation", failures);
        check(box1.extents() == glm::vec3(1.0f, 1.0f, 1.0f), "AABB extents calculation", failures);

        AABB box2 = AABB::fromCentre({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
        check(box1.overlaps(box2), "Overlapping boxes detected", failures);

        // Touching boxes should NOT overlap (strict inequality)
        AABB touchingX = AABB::fromCentre({2.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        check(!box1.overlaps(touchingX), "Touching boxes do not overlap (strict inequality)", failures);

        AABB distant = AABB::fromCentre({10.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        check(!box1.overlaps(distant), "Distant boxes do not overlap", failures);
    }

    // 2. Ray vs AABB Intersection
    {
        AABB box = AABB::fromCentre({0.0f, 0.0f, 10.0f}, {1.0f, 1.0f, 1.0f});
        float dist = 0.0f;

        // Direct hit
        bool hit = rayAabb({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, box, 50.0f, dist);
        check(hit && std::abs(dist - 9.0f) < 0.001f, "Ray hit box front face at distance 9.0", failures);

        // Complete miss
        hit = rayAabb({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, box, 50.0f, dist);
        check(!hit, "Ray pointing away misses box", failures);

        // Max distance clipping
        hit = rayAabb({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, box, 5.0f, dist);
        check(!hit, "Ray with short maxDistance does not reach box", failures);

        // Ray origin inside box
        hit = rayAabb({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f}, box, 50.0f, dist);
        check(hit, "Ray starting inside box detects intersection", failures);
    }

    // 3. PhysicsWorld Raycasting & Line of Sight
    {
        PhysicsWorld world;
        world.addBox(AABB::fromCentre({0.0f, 0.0f, 5.0f}, {1.0f, 1.0f, 1.0f}));  // Box 0
        world.addBox(AABB::fromCentre({0.0f, 0.0f, 15.0f}, {1.0f, 1.0f, 1.0f})); // Box 1

        check(world.boxes().size() == 2, "PhysicsWorld contains 2 boxes", failures);

        RayHit rHit = world.raycast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 50.0f);
        check(rHit.hit, "World raycast hits nearest obstacle", failures);
        check(rHit.index == 0, "World raycast returns nearest box index 0", failures);
        check(std::abs(rHit.distance - 4.0f) < 0.001f, "Nearest hit distance is 4.0", failures);
        check(rHit.normal == glm::vec3(0.0f, 0.0f, -1.0f), "Surface normal opposes ray direction", failures);

        // Line of sight tests
        check(!world.lineOfSight({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}), "Line of sight blocked by Box 0", failures);
        check(world.lineOfSight({0.0f, 5.0f, 0.0f}, {0.0f, 5.0f, 10.0f}), "Line of sight clear above boxes", failures);
    }

    // 4. Character Movement & Collision Resolution
    {
        PhysicsWorld world;
        // Floor at y = 0
        world.addBox(AABB{{-20.0f, -2.0f, -20.0f}, {20.0f, 0.0f, 20.0f}});
        // Wall at x = 5
        world.addBox(AABB{{5.0f, 0.0f, -5.0f}, {6.0f, 4.0f, 5.0f}});

        bool grounded = false;
        glm::vec3 half{0.3f, 0.9f, 0.3f};

        // Fall onto floor
        glm::vec3 pos{0.0f, 1.5f, 0.0f};
        glm::vec3 moved = world.moveCharacter(pos, half, {0.0f, -2.0f, 0.0f}, grounded);
        check(grounded, "Character landing on floor becomes grounded", failures);
        check(moved.y >= 0.9f, "Character y position stopped by floor", failures);

        // Walk towards wall at x=5
        pos = {4.0f, 0.9f, 0.0f};
        moved = world.moveCharacter(pos, half, {2.0f, 0.0f, 0.0f}, grounded);
        check(moved.x < 5.0f, "Character stopped by wall at x=5", failures);
    }
}
