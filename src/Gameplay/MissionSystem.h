#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace vajra {

enum class MissionStatus { InProgress, Success, Failed };

struct ObjectiveState {
    std::string id;
    std::string label;
    glm::vec3   position{0.0f};
    float       radius = 3.0f;
    bool        requiresDestroy = false;
    bool        complete = false;
    bool        optional = false;
};

/// Tracks objectives, unlocks extraction once the mandatory ones are done, and
/// decides success or failure. Objectives are data, so a new mission is a new
/// JSON file rather than new code.
class MissionSystem {
public:
    void configure(std::string title, std::vector<ObjectiveState> objectives,
                   const glm::vec3& extractionPoint, float extractionRadius);

    void update(const glm::vec3& playerPosition, bool playerAlive, float dt);

    /// Called when the player destroys an objective device.
    void markDestroyed(const std::string& id);

    [[nodiscard]] MissionStatus status() const { return m_status; }
    [[nodiscard]] const std::string& title() const { return m_title; }
    [[nodiscard]] const std::vector<ObjectiveState>& objectives() const { return m_objectives; }
    [[nodiscard]] bool extractionActive() const { return m_extractionActive; }
    [[nodiscard]] const glm::vec3& extractionPoint() const { return m_extractionPoint; }
    [[nodiscard]] float elapsed() const { return m_elapsed; }

    /// The next incomplete objective — what the HUD shows.
    [[nodiscard]] const ObjectiveState* currentObjective() const;

    void reset();

private:
    std::string m_title;
    std::vector<ObjectiveState> m_objectives;
    glm::vec3 m_extractionPoint{0.0f};
    float m_extractionRadius = 4.0f;
    bool  m_extractionActive = false;
    MissionStatus m_status = MissionStatus::InProgress;
    float m_elapsed = 0.0f;
};

}  // namespace vajra
