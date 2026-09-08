#include "Scene/Level.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Core/Log.h"

namespace vajra {

namespace {

using json = nlohmann::json;

glm::vec3 readVec3(const json& node, const glm::vec3& fallback) {
    if (!node.is_array() || node.size() < 3) return fallback;
    return glm::vec3{node[0].get<float>(), node[1].get<float>(), node[2].get<float>()};
}

}  // namespace

Level Level::fallback() {
    Level level;
    level.title = "FALLBACK BLOCKOUT";
    level.briefing = "LEVEL FILE MISSING - RUNNING BUILT-IN TEST ROOM";
    level.playerStart = {0.0f, 0.2f, 14.0f};

    for (int i = 0; i < 10; ++i) {
        LevelBox box;
        box.centre = {static_cast<float>((i % 5) * 6 - 12), 0.8f,
                      static_cast<float>((i / 5) * -9 - 4)};
        box.halfExtents = {1.2f, 0.8f, 1.2f};
        box.albedo = {0.42f, 0.40f, 0.36f};
        level.boxes.push_back(box);
    }

    LevelGuard guard;
    guard.position = {0.0f, 0.0f, -20.0f};
    guard.patrol = {{-8.0f, 0.0f, -20.0f}, {8.0f, 0.0f, -20.0f}};
    level.guards.push_back(guard);

    LevelObjective objective;
    objective.id = "relay";
    objective.label = "REACH THE RELAY";
    objective.position = {0.0f, 0.0f, -26.0f};
    level.objectives.push_back(objective);

    level.extractionPoint = {0.0f, 0.0f, 16.0f};
    return level;
}

Level Level::loadFromFile(const std::string& path, bool* outOk) {
    if (outOk) *outOk = false;

    std::ifstream in(path);
    if (!in) {
        VJ_WARN("Could not open level '%s' - using fallback", path.c_str());
        return fallback();
    }

    json root;
    try {
        in >> root;
    } catch (const std::exception& e) {
        VJ_ERROR("Level parse error in '%s': %s", path.c_str(), e.what());
        return fallback();
    }

    Level level;
    level.title    = root.value("title", std::string{"UNTITLED"});
    level.briefing = root.value("briefing", std::string{});
    level.groundSize = root.value("groundSize", 140.0f);

    if (root.contains("playerStart"))  level.playerStart  = readVec3(root["playerStart"], level.playerStart);
    if (root.contains("sunDirection")) level.sunDirection = readVec3(root["sunDirection"], level.sunDirection);
    if (root.contains("sunColour"))    level.sunColour    = readVec3(root["sunColour"], level.sunColour);
    if (root.contains("fogColour"))    level.fogColour    = readVec3(root["fogColour"], level.fogColour);

    for (const auto& node : root.value("boxes", json::array())) {
        LevelBox box;
        box.centre      = readVec3(node.value("centre", json::array()), box.centre);
        box.halfExtents = readVec3(node.value("halfExtents", json::array()), box.halfExtents);
        box.albedo      = readVec3(node.value("albedo", json::array()), box.albedo);
        box.roughness   = node.value("roughness", 0.8f);
        box.collides    = node.value("collides", true);
        level.boxes.push_back(box);
    }

    for (const auto& node : root.value("guards", json::array())) {
        LevelGuard guard;
        guard.position        = readVec3(node.value("position", json::array()), guard.position);
        guard.accuracy        = node.value("accuracy", 0.5f);
        guard.visionRange     = node.value("visionRange", 55.0f);
        guard.visionHalfAngle = node.value("visionHalfAngle", 55.0f);
        guard.squadId         = node.value("squad", 0);
        guard.isMachine       = node.value("machine", false);
        for (const auto& wp : node.value("patrol", json::array()))
            guard.patrol.push_back(readVec3(wp, glm::vec3{0.0f}));
        level.guards.push_back(guard);
    }

    for (const auto& node : root.value("objectives", json::array())) {
        LevelObjective objective;
        objective.id              = node.value("id", std::string{"obj"});
        objective.label           = node.value("label", std::string{"OBJECTIVE"});
        objective.position        = readVec3(node.value("position", json::array()), glm::vec3{0.0f});
        objective.radius          = node.value("radius", 3.0f);
        objective.requiresDestroy = node.value("destroy", false);
        objective.optional        = node.value("optional", false);
        level.objectives.push_back(objective);
    }

    if (root.contains("extraction")) {
        const auto& ex = root["extraction"];
        level.extractionPoint  = readVec3(ex.value("position", json::array()), glm::vec3{0.0f});
        level.extractionRadius = ex.value("radius", 4.0f);
    }

    VJ_INFO("Loaded level '%s': %zu boxes, %zu guards, %zu objectives",
            level.title.c_str(), level.boxes.size(), level.guards.size(),
            level.objectives.size());

    if (outOk) *outOk = true;
    return level;
}

}  // namespace vajra
