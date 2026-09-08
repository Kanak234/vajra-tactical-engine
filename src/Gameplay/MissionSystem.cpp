#include "Gameplay/MissionSystem.h"

#include <glm/gtx/norm.hpp>
#include <utility>

namespace vajra {

void MissionSystem::configure(std::string title, std::vector<ObjectiveState> objectives,
                              const glm::vec3& extractionPoint, float extractionRadius) {
    m_title            = std::move(title);
    m_objectives       = std::move(objectives);
    m_extractionPoint  = extractionPoint;
    m_extractionRadius = extractionRadius;
    reset();
}

void MissionSystem::reset() {
    for (auto& o : m_objectives) o.complete = false;
    m_extractionActive = false;
    m_status  = MissionStatus::InProgress;
    m_elapsed = 0.0f;
}

void MissionSystem::markDestroyed(const std::string& id) {
    for (auto& o : m_objectives)
        if (o.id == id) o.complete = true;
}

const ObjectiveState* MissionSystem::currentObjective() const {
    for (const auto& o : m_objectives)
        if (!o.complete && !o.optional) return &o;
    return nullptr;
}

void MissionSystem::update(const glm::vec3& playerPosition, bool playerAlive, float dt) {
    if (m_status != MissionStatus::InProgress) return;

    m_elapsed += dt;

    if (!playerAlive) {
        m_status = MissionStatus::Failed;
        return;
    }

    // Proximity objectives complete by standing in the zone.
    for (auto& o : m_objectives) {
        if (o.complete || o.requiresDestroy) continue;
        if (glm::length2(playerPosition - o.position) <= o.radius * o.radius)
            o.complete = true;
    }

    // Extraction unlocks only once every mandatory objective is done.
    bool allDone = true;
    for (const auto& o : m_objectives)
        if (!o.optional && !o.complete) { allDone = false; break; }

    m_extractionActive = allDone;

    if (m_extractionActive &&
        glm::length2(playerPosition - m_extractionPoint) <= m_extractionRadius * m_extractionRadius) {
        m_status = MissionStatus::Success;
    }
}

}  // namespace vajra
