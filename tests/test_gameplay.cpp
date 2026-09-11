#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

#include "Gameplay/Weapon.h"
#include "Gameplay/MissionSystem.h"

using namespace vajra;

namespace {

void check(bool condition, const std::string& what, int& failures) {
    std::printf("  [%s] %s\n", condition ? " OK " : "FAIL", what.c_str());
    if (!condition) ++failures;
}

} // namespace

void runGameplayTests(int& failures) {
    std::printf("\n=== Weapon & Gameplay System Tests ===\n");

    // 1. Weapon Mechanics
    {
        Weapon rifle(WeaponId::SilencedRifle);
        check(rifle.stats().name == "SR-9 KAVACH", "Silenced rifle named SR-9 KAVACH", failures);
        check(rifle.stats().suppressed, "Rifle is suppressed", failures);
        check(rifle.ammoInMag() == 8, "Initial magazine has 8 rounds", failures);
        check(rifle.reserve() == 48, "Initial reserve has 48 rounds", failures);

        // Firing
        bool fired = rifle.tryFire();
        check(fired, "First shot fires successfully", failures);
        check(rifle.ammoInMag() == 7, "Magazine decrements to 7", failures);

        // Immediate next shot should be blocked by fireInterval
        bool secondShot = rifle.tryFire();
        check(!secondShot, "Immediate second shot throttled by cadence", failures);

        // Advance past fire interval
        rifle.update(rifle.stats().fireInterval + 0.1f);
        check(rifle.tryFire(), "Shot allowed after interval elapsed", failures);
        check(rifle.ammoInMag() == 6, "Magazine decrements to 6", failures);

        // Spread reduction with aim & crouch
        float hipSpread = rifle.currentSpread(false);
        float crouchSpread = rifle.currentSpread(true);
        rifle.setAiming(true);
        float adsSpread = rifle.currentSpread(false);
        check(crouchSpread < hipSpread, "Crouch reduces spread angle", failures);
        check(adsSpread < hipSpread, "Aiming down sights reduces spread angle", failures);

        // Reload cycle
        rifle.beginReload();
        check(rifle.reloading(), "Reload in progress", failures);
        rifle.update(rifle.stats().reloadTime + 0.1f);
        check(!rifle.reloading(), "Reload completed", failures);
        check(rifle.ammoInMag() == 8, "Magazine replenished to full capacity", failures);
        check(rifle.reserve() == 46, "Reserve properly decremented by 2 bullets", failures);

        // Spread direction computation
        glm::vec3 fwd{0.0f, 0.0f, 1.0f};
        glm::vec3 dir = rifle.spreadDirection(fwd, false);
        check(glm::length(dir) > 0.99f && glm::length(dir) < 1.01f, "Spread direction is normalized", failures);
        check(glm::dot(dir, fwd) > 0.8f, "Spread direction generally aligns with forward", failures);

        // Sidearm & empty-magazine firing
        Weapon sidearm(WeaponId::Sidearm);
        check(sidearm.stats().name == "VP-4 SIDEARM", "Sidearm named VP-4 SIDEARM", failures);
        check(!sidearm.stats().suppressed, "Sidearm is unsuppressed", failures);
        check(sidearm.ammoInMag() == 15, "Sidearm has 15 rounds", failures);

        // Exhaust ammo to test empty magazine reload trigger
        while (sidearm.ammoInMag() > 0) {
            sidearm.tryFire();
            sidearm.update(sidearm.stats().fireInterval + 0.01f);
        }
        check(sidearm.ammoInMag() == 0, "Sidearm magazine empty", failures);
        bool firedEmpty = sidearm.tryFire();
        check(!firedEmpty, "Firing empty weapon fails", failures);
        check(sidearm.reloading(), "Firing empty weapon triggers reload", failures);
    }

    // 2. MissionSystem Progression
    {
        MissionSystem mission;
        std::vector<ObjectiveState> objs = {
            {"intel", "Recover data terminal", {10.0f, 0.0f, 0.0f}, 2.0f, false, false, false},
            {"antenna", "Sabotage radar uplink", {20.0f, 0.0f, 0.0f}, 2.0f, true, false, false},
            {"bonus", "Optional audio log", {30.0f, 0.0f, 0.0f}, 2.0f, false, false, true}
        };
        mission.configure("Operation Vajra", objs, {0.0f, 0.0f, 0.0f}, 3.0f);

        check(mission.status() == MissionStatus::InProgress, "Mission starts InProgress", failures);
        check(!mission.extractionActive(), "Extraction inactive at start", failures);
        check(mission.currentObjective() != nullptr, "Current objective available", failures);
        check(mission.currentObjective()->id == "intel", "First objective is intel", failures);

        // Stand in proximity of first objective
        mission.update({10.5f, 0.0f, 0.0f}, true, 0.1f);
        check(mission.objectives()[0].complete, "Proximity objective completes in zone", failures);
        check(mission.currentObjective()->id == "antenna", "Current objective advances to antenna", failures);
        check(!mission.extractionActive(), "Extraction still locked (antenna incomplete)", failures);

        // Destroy antenna
        mission.markDestroyed("antenna");
        mission.update({10.5f, 0.0f, 0.0f}, true, 0.1f);
        check(mission.objectives()[1].complete, "Destroy objective completes", failures);
        check(mission.extractionActive(), "Extraction unlocks after all mandatory objectives complete", failures);

        // Walk to extraction point (0, 0, 0)
        mission.update({0.5f, 0.0f, 0.0f}, true, 0.1f);
        check(mission.status() == MissionStatus::Success, "Reaching extraction triggers MissionStatus::Success", failures);

        // Player death fails mission
        mission.reset();
        check(mission.status() == MissionStatus::InProgress, "Reset restarts mission to InProgress", failures);
        mission.update({0.0f, 0.0f, 0.0f}, false, 0.1f);
        check(mission.status() == MissionStatus::Failed, "Player death triggers MissionStatus::Failed", failures);
    }
}
