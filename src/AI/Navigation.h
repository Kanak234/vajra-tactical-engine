#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace vajra {

class PhysicsWorld;

/// Uniform-grid navigation with A* and a line-of-sight string pull. A navmesh
/// is nicer for organic terrain, but for a station built from boxes a grid is
/// accurate, rebuildable in milliseconds, and easy to visualise while debugging.
class NavGrid {
public:
    void build(const PhysicsWorld& world, const glm::vec3& origin,
               float cellSize, int cellsX, int cellsZ, float agentRadius,
               float agentHeight);

    /// A* from start to goal. Returns false when no route exists.
    [[nodiscard]] bool findPath(const glm::vec3& start, const glm::vec3& goal,
                                std::vector<glm::vec3>& outPath) const;

    [[nodiscard]] bool walkable(int x, int z) const;
    [[nodiscard]] glm::vec3 cellToWorld(int x, int z) const;
    [[nodiscard]] bool worldToCell(const glm::vec3& p, int& outX, int& outZ) const;

    [[nodiscard]] int cellsX() const { return m_cellsX; }
    [[nodiscard]] int cellsZ() const { return m_cellsZ; }

private:
    /// Removes redundant waypoints when a straight line is unobstructed.
    void stringPull(std::vector<glm::vec3>& path) const;

    std::vector<uint8_t> m_walkable;
    glm::vec3 m_origin{0.0f};
    float m_cellSize = 1.0f;
    int   m_cellsX = 0;
    int   m_cellsZ = 0;
};

}  // namespace vajra
