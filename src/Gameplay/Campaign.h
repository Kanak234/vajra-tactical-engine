#pragma once
#include <string>
#include <vector>

namespace vajra {

/// One entry in the campaign. Progress is per-mission so a player can replay
/// an earlier one to chase a ghost run without losing later unlocks.
struct CampaignMission {
    std::string file;        // path relative to the levels directory
    std::string title;
    std::string subtitle;
    bool  unlocked  = false;
    bool  completed = false;
    bool  ghosted   = false;   // finished without ever being fully detected
    float bestTime  = 0.0f;    // seconds, 0 = never completed
};

/// Mission list, unlock order, and persistent records. Deliberately separate
/// from MissionSystem: that tracks objectives *inside* a mission, this tracks
/// progress *across* them.
class Campaign {
public:
    void init(const std::string& levelDirectory);

    /// Records a completed run and unlocks the next mission.
    void recordCompletion(size_t index, float time, bool ghosted);

    void load();
    void save() const;

    [[nodiscard]] const std::vector<CampaignMission>& missions() const { return m_missions; }
    [[nodiscard]] size_t count() const { return m_missions.size(); }
    [[nodiscard]] std::string pathFor(size_t index) const;
    [[nodiscard]] bool isUnlocked(size_t index) const {
        return index < m_missions.size() && m_missions[index].unlocked;
    }
    /// Index of the next mission after `index`, or the same index if it is last.
    [[nodiscard]] size_t nextAfter(size_t index) const {
        return (index + 1 < m_missions.size()) ? index + 1 : index;
    }
    [[nodiscard]] bool hasNextAfter(size_t index) const { return index + 1 < m_missions.size(); }
    [[nodiscard]] int completedCount() const;

private:
    std::vector<CampaignMission> m_missions;
    std::string m_levelDirectory;
};

}  // namespace vajra
