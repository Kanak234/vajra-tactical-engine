#include "Gameplay/Weapon.h"

#include <cmath>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>

namespace vajra {

namespace {
float randUnit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}
}  // namespace

WeaponStats Weapon::defaultStats(WeaponId id) {
    WeaponStats s;
    switch (id) {
        case WeaponId::SilencedRifle:
            s.name = "SR-9 KAVACH";
            s.damage = 85.0f;          // one shot to the body at close range
            s.fireInterval = 1.05f;    // bolt-action cadence, forces commitment
            s.magazine = 8;
            s.reserve  = 48;
            s.reloadTime = 2.6f;
            s.spreadHip = 4.5f;
            s.spreadAds = 0.15f;
            s.recoilPitch = 2.1f;
            s.adsFov = 22.0f;
            s.suppressed = true;
            s.noiseRadius = 14.0f;
            s.range = 400.0f;
            break;
        case WeaponId::Sidearm:
        default:
            s.name = "VP-4 SIDEARM";
            s.damage = 34.0f;
            s.fireInterval = 0.22f;
            s.magazine = 15;
            s.reserve  = 75;
            s.reloadTime = 1.7f;
            s.spreadHip = 2.6f;
            s.spreadAds = 0.9f;
            s.recoilPitch = 0.9f;
            s.adsFov = 45.0f;
            s.suppressed = false;
            s.noiseRadius = 55.0f;     // unsuppressed: the whole sector hears it
            s.range = 120.0f;
            break;
    }
    return s;
}

Weapon::Weapon(WeaponId id) : m_id(id), m_stats(defaultStats(id)) {
    m_ammoInMag = m_stats.magazine;
    m_reserve   = m_stats.reserve;
}

void Weapon::update(float dt) {
    if (m_fireTimer > 0.0f) m_fireTimer -= dt;
    if (m_reloadTimer > 0.0f) {
        m_reloadTimer -= dt;
        if (m_reloadTimer <= 0.0f) {
            m_reloadTimer = 0.0f;
            const int needed = m_stats.magazine - m_ammoInMag;
            const int taken  = (needed < m_reserve) ? needed : m_reserve;
            m_ammoInMag += taken;
            m_reserve   -= taken;
        }
    }
}

bool Weapon::tryFire() {
    if (reloading() || m_fireTimer > 0.0f) return false;
    if (m_ammoInMag <= 0) {
        beginReload();
        return false;
    }
    --m_ammoInMag;
    m_fireTimer = m_stats.fireInterval;
    return true;
}

void Weapon::beginReload() {
    if (reloading() || m_reserve <= 0 || m_ammoInMag >= m_stats.magazine) return;
    m_reloadTimer = m_stats.reloadTime;
}

float Weapon::currentSpread(bool crouched) const {
    float spread = m_aiming ? m_stats.spreadAds : m_stats.spreadHip;
    if (crouched) spread *= m_stats.spreadCrouchMul;
    return spread;
}

glm::vec3 Weapon::spreadDirection(const glm::vec3& forward, bool crouched) const {
    const float coneDeg = currentSpread(crouched);
    if (coneDeg <= 0.0001f) return glm::normalize(forward);

    // Uniform sample inside the cone, then rotate the forward vector.
    const float angle = glm::radians(coneDeg) * std::sqrt(randUnit());
    const float roll  = randUnit() * 6.2831853f;

    glm::vec3 f = glm::normalize(forward);
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    if (std::abs(glm::dot(f, up)) > 0.99f) up = glm::vec3{1.0f, 0.0f, 0.0f};
    const glm::vec3 right = glm::normalize(glm::cross(f, up));
    const glm::vec3 realUp = glm::cross(right, f);

    const glm::vec3 offset = (right * std::cos(roll) + realUp * std::sin(roll)) * std::tan(angle);
    return glm::normalize(f + offset);
}

}  // namespace vajra
