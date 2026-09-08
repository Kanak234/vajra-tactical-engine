#include "Gameplay/Campaign.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Core/Log.h"

namespace vajra {

namespace {
constexpr const char* kProgressFile = "vajra_campaign.json";
}

void Campaign::init(const std::string& levelDirectory) {
    m_levelDirectory = levelDirectory;
    m_missions = {
        {"quiet_station.json", "THE QUIET STATION",
         "ACT I - INFILTRATE RESEARCH POST 4", true,  false, false, 0.0f},
        {"uplink.json",        "UPLINK",
         "ACT I - CUT THE RELAY CHAIN",        false, false, false, 0.0f},
        {"cold_storage.json",  "COLD STORAGE",
         "ACT III - THE ROBOTICS LAB",         false, false, false, 0.0f},
    };
    load();
    m_missions[0].unlocked = true;   // the first is always available
}

std::string Campaign::pathFor(size_t index) const {
    if (index >= m_missions.size()) return {};
    return m_levelDirectory + m_missions[index].file;
}

int Campaign::completedCount() const {
    int count = 0;
    for (const auto& mission : m_missions)
        if (mission.completed) ++count;
    return count;
}

void Campaign::recordCompletion(size_t index, float time, bool ghosted) {
    if (index >= m_missions.size()) return;

    CampaignMission& mission = m_missions[index];
    mission.completed = true;
    // Ghost status is sticky: earning it once keeps it, even if a later
    // replay goes loud.
    mission.ghosted = mission.ghosted || ghosted;
    if (mission.bestTime <= 0.0f || time < mission.bestTime) mission.bestTime = time;

    if (index + 1 < m_missions.size()) m_missions[index + 1].unlocked = true;
    save();
}

void Campaign::save() const {
    nlohmann::json root;
    nlohmann::json list = nlohmann::json::array();
    for (const auto& mission : m_missions) {
        list.push_back({{"file", mission.file},
                        {"unlocked", mission.unlocked},
                        {"completed", mission.completed},
                        {"ghosted", mission.ghosted},
                        {"bestTime", mission.bestTime}});
    }
    root["missions"] = list;

    std::ofstream out(kProgressFile);
    if (out) out << root.dump(2);
}

void Campaign::load() {
    std::ifstream in(kProgressFile);
    if (!in) return;

    nlohmann::json root;
    try {
        in >> root;
    } catch (const std::exception& e) {
        VJ_WARN("Campaign progress unreadable (%s) - starting fresh", e.what());
        return;
    }

    for (const auto& node : root.value("missions", nlohmann::json::array())) {
        const std::string file = node.value("file", std::string{});
        for (auto& mission : m_missions) {
            if (mission.file != file) continue;
            mission.unlocked  = node.value("unlocked", mission.unlocked);
            mission.completed = node.value("completed", false);
            mission.ghosted   = node.value("ghosted", false);
            mission.bestTime  = node.value("bestTime", 0.0f);
        }
    }
}

}  // namespace vajra
