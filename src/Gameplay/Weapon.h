#pragma once
#include <glm/glm.hpp>
#include <string>

namespace vajra {

enum class WeaponId { SilencedRifle, Sidearm, Count };

struct WeaponStats {
    std::string name;
    float damage        = 45.0f;
    float fireInterval  = 0.9f;    // seconds between shots
    int   magazine      = 10;
    int   reserve       = 60;
    float reloadTime    = 2.4f;
    float spreadHip     = 3.2f;    // degrees
    float spreadAds     = 0.35f;
    float spreadCrouchMul = 0.55f;
    float recoilPitch   = 1.6f;
    float adsFov        = 28.0f;
    bool  suppressed    = true;
    float noiseRadius   = 12.0f;   // metres guards can hear the shot from
    float range         = 300.0f;
};

/// Hitscan weapon with magazines, reload timing, spread that responds to
/// stance and aim, and a noise radius that feeds the AI hearing system.
/// Suppressed fire is quiet but not silent — that trade-off is the game.
class Weapon {
public:
    Weapon() = default;
    explicit Weapon(WeaponId id);

    void update(float dt);

    /// Returns true if a round actually left the barrel this call.
    bool tryFire();
    void beginReload();
    void setAiming(bool aiming) { m_aiming = aiming; }

    /// Current cone half-angle in degrees, given stance and aim state.
    [[nodiscard]] float currentSpread(bool crouched) const;
    /// Applies random spread to a direction using the current cone.
    [[nodiscard]] glm::vec3 spreadDirection(const glm::vec3& forward, bool crouched) const;

    [[nodiscard]] const WeaponStats& stats() const { return m_stats; }
    [[nodiscard]] int   ammoInMag() const { return m_ammoInMag; }
    [[nodiscard]] int   reserve() const { return m_reserve; }
    [[nodiscard]] bool  reloading() const { return m_reloadTimer > 0.0f; }
    [[nodiscard]] bool  aiming() const { return m_aiming; }
    [[nodiscard]] float reloadProgress() const {
        return m_stats.reloadTime > 0.0f ? 1.0f - m_reloadTimer / m_stats.reloadTime : 1.0f;
    }
    [[nodiscard]] WeaponId id() const { return m_id; }

    static WeaponStats defaultStats(WeaponId id);

private:
    WeaponId    m_id = WeaponId::SilencedRifle;
    WeaponStats m_stats{};
    int   m_ammoInMag   = 0;
    int   m_reserve     = 0;
    float m_fireTimer   = 0.0f;
    float m_reloadTimer = 0.0f;
    bool  m_aiming      = false;
};

}  // namespace vajra
