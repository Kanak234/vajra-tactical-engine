#include "AI/Navigation.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

#include "Physics/Collision.h"

namespace vajra {

void NavGrid::build(const PhysicsWorld& world, const glm::vec3& origin,
                    float cellSize, int cellsX, int cellsZ,
                    float agentRadius, float agentHeight) {
    m_origin   = origin;
    m_cellSize = cellSize;
    m_cellsX   = cellsX;
    m_cellsZ   = cellsZ;
    m_walkable.assign(static_cast<size_t>(cellsX * cellsZ), 1);

    // A cell is blocked if an agent-sized box placed there would intersect
    // level geometry. Inflating by agentRadius is what keeps guards from
    // clipping walls when they cut corners.
    const glm::vec3 half{agentRadius, agentHeight * 0.5f, agentRadius};

    for (int z = 0; z < cellsZ; ++z) {
        for (int x = 0; x < cellsX; ++x) {
            // Lift the probe a few centimetres so resting exactly on the floor
            // plane can never register as a collision.
            const glm::vec3 centre = cellToWorld(x, z) +
                                     glm::vec3{0.0f, agentHeight * 0.5f + 0.05f, 0.0f};
            const AABB probe = AABB::fromCentre(centre, half);
            bool blocked = false;
            for (const AABB& b : world.boxes()) {
                if (probe.overlaps(b)) { blocked = true; break; }
            }
            m_walkable[static_cast<size_t>(z * cellsX + x)] = blocked ? 0 : 1;
        }
    }
}

bool NavGrid::walkable(int x, int z) const {
    if (x < 0 || z < 0 || x >= m_cellsX || z >= m_cellsZ) return false;
    return m_walkable[static_cast<size_t>(z * m_cellsX + x)] != 0;
}

glm::vec3 NavGrid::cellToWorld(int x, int z) const {
    return m_origin + glm::vec3{(static_cast<float>(x) + 0.5f) * m_cellSize,
                                0.0f,
                                (static_cast<float>(z) + 0.5f) * m_cellSize};
}

bool NavGrid::worldToCell(const glm::vec3& p, int& outX, int& outZ) const {
    const glm::vec3 local = p - m_origin;
    outX = static_cast<int>(std::floor(local.x / m_cellSize));
    outZ = static_cast<int>(std::floor(local.z / m_cellSize));
    return outX >= 0 && outZ >= 0 && outX < m_cellsX && outZ < m_cellsZ;
}

bool NavGrid::findPath(const glm::vec3& start, const glm::vec3& goal,
                       std::vector<glm::vec3>& outPath) const {
    outPath.clear();

    int sx, sz, gx, gz;
    if (!worldToCell(start, sx, sz) || !worldToCell(goal, gx, gz)) return false;

    // Snap the goal to the nearest walkable cell — the player often stands on
    // a crate, which is a blocked cell.
    if (!walkable(gx, gz)) {
        bool found = false;
        for (int r = 1; r <= 6 && !found; ++r) {
            for (int dz = -r; dz <= r && !found; ++dz) {
                for (int dx = -r; dx <= r && !found; ++dx) {
                    if (walkable(gx + dx, gz + dz)) { gx += dx; gz += dz; found = true; }
                }
            }
        }
        if (!found) return false;
    }
    if (!walkable(sx, sz)) return false;

    struct Node { int index; float f; };
    struct Compare {
        bool operator()(const Node& a, const Node& b) const { return a.f > b.f; }
    };

    const int total = m_cellsX * m_cellsZ;
    std::vector<float> gScore(static_cast<size_t>(total), std::numeric_limits<float>::infinity());
    std::vector<int>   cameFrom(static_cast<size_t>(total), -1);
    std::vector<uint8_t> closed(static_cast<size_t>(total), 0);

    auto idx  = [this](int x, int z) { return z * m_cellsX + x; };
    auto heur = [&](int x, int z) {
        const float dx = static_cast<float>(std::abs(x - gx));
        const float dz = static_cast<float>(std::abs(z - gz));
        // Octile distance: correct admissible heuristic for 8-way movement.
        return (dx + dz) + (std::sqrt(2.0f) - 2.0f) * std::min(dx, dz);
    };

    std::priority_queue<Node, std::vector<Node>, Compare> open;
    const int startIndex = idx(sx, sz);
    gScore[static_cast<size_t>(startIndex)] = 0.0f;
    open.push({startIndex, heur(sx, sz)});

    const int goalIndex = idx(gx, gz);
    bool reached = false;

    while (!open.empty()) {
        const Node current = open.top();
        open.pop();
        if (closed[static_cast<size_t>(current.index)]) continue;
        closed[static_cast<size_t>(current.index)] = 1;

        if (current.index == goalIndex) { reached = true; break; }

        const int cx = current.index % m_cellsX;
        const int cz = current.index / m_cellsX;

        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dz == 0) continue;
                const int nx = cx + dx;
                const int nz = cz + dz;
                if (!walkable(nx, nz)) continue;
                // Do not cut diagonally through a corner.
                if (dx != 0 && dz != 0 && (!walkable(cx + dx, cz) || !walkable(cx, cz + dz)))
                    continue;

                const int   ni   = idx(nx, nz);
                const float step = (dx != 0 && dz != 0) ? std::sqrt(2.0f) : 1.0f;
                const float tentative = gScore[static_cast<size_t>(current.index)] + step;

                if (tentative < gScore[static_cast<size_t>(ni)]) {
                    gScore[static_cast<size_t>(ni)]   = tentative;
                    cameFrom[static_cast<size_t>(ni)] = current.index;
                    open.push({ni, tentative + heur(nx, nz)});
                }
            }
        }
    }

    if (!reached) return false;

    for (int at = goalIndex; at != -1; at = cameFrom[static_cast<size_t>(at)])
        outPath.push_back(cellToWorld(at % m_cellsX, at / m_cellsX));
    std::reverse(outPath.begin(), outPath.end());

    stringPull(outPath);
    return true;
}

void NavGrid::stringPull(std::vector<glm::vec3>& path) const {
    if (path.size() < 3) return;

    std::vector<glm::vec3> result;
    result.push_back(path.front());

    size_t anchor = 0;
    for (size_t probe = 2; probe < path.size(); ++probe) {
        // Walk the straight line between anchor and probe; if every sampled
        // cell is walkable we can skip everything in between.
        const glm::vec3 a = path[anchor];
        const glm::vec3 b = path[probe];
        const float dist  = glm::length(b - a);
        const int steps   = std::max(2, static_cast<int>(dist / (m_cellSize * 0.5f)));

        bool clear = true;
        for (int s = 1; s < steps && clear; ++s) {
            const glm::vec3 p = a + (b - a) * (static_cast<float>(s) / static_cast<float>(steps));
            int px, pz;
            if (!worldToCell(p, px, pz) || !walkable(px, pz)) clear = false;
        }

        if (!clear) {
            result.push_back(path[probe - 1]);
            anchor = probe - 1;
        }
    }
    result.push_back(path.back());
    path.swap(result);
}

}  // namespace vajra
