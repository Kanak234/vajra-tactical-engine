#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace vajra {

struct LevelBox {
    glm::vec3 centre{0.0f};
    glm::vec3 halfExtents{1.0f};
    glm::vec3 albedo{0.5f};
    float     roughness = 0.8f;
    bool      collides = true;
};

struct LevelGuard {
    glm::vec3 position{0.0f};
    std::vector<glm::vec3> patrol;
    float accuracy = 0.5f;
    float visionRange = 55.0f;
    float visionHalfAngle = 55.0f;
    int   squadId = 0;
    bool  isMachine = false;   // drone/turret: autonomous, not on the radio net
};

struct LevelObjective {
    std::string id;
    std::string label;
    glm::vec3   position{0.0f};
    float       radius = 3.0f;
    bool        requiresDestroy = false;
    bool        optional = false;
};

/// Everything a mission is made of, loaded from JSON. Adding a mission means
/// adding a file under assets/levels — no recompile.
struct Level {
    std::string title = "UNTITLED";
    std::string briefing;
    glm::vec3   playerStart{0.0f, 0.2f, 0.0f};
    glm::vec3   sunDirection{-0.35f, -1.0f, -0.28f};
    glm::vec3   sunColour{1.0f, 0.94f, 0.84f};
    glm::vec3   fogColour{0.07f, 0.09f, 0.12f};
    float       groundSize = 140.0f;

    std::vector<LevelBox>       boxes;
    std::vector<LevelGuard>     guards;
    std::vector<LevelObjective> objectives;

    glm::vec3 extractionPoint{0.0f};
    float     extractionRadius = 4.0f;

    /// Loads from disk. On failure, returns a small built-in fallback level so
    /// the game always starts rather than dying on a missing file.
    static Level loadFromFile(const std::string& path, bool* outOk = nullptr);
    static Level fallback();
};

}  // namespace vajra
