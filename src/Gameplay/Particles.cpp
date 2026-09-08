#include "Gameplay/Particles.h"

#include <algorithm>
#include <cmath>

#include "Core/JobSystem.h"

namespace vajra {

void ParticleSystem::init(size_t capacity) {
    m_particles.assign(capacity, Particle{});
    m_next = 0;
    m_alive = 0;
}

void ParticleSystem::clear() {
    for (Particle& p : m_particles) p.life = 0.0f;
    m_alive = 0;
}

float ParticleSystem::random() {
    m_rng = m_rng * 1664525u + 1013904223u;
    return static_cast<float>((m_rng >> 8) & 0xFFFFFF) / 16777215.0f;
}

glm::vec3 ParticleSystem::randomCone(const glm::vec3& axis, float spreadDegrees) {
    const glm::vec3 a = glm::normalize(axis);
    glm::vec3 up = std::abs(a.y) > 0.95f ? glm::vec3{1, 0, 0} : glm::vec3{0, 1, 0};
    const glm::vec3 right = glm::normalize(glm::cross(a, up));
    const glm::vec3 realUp = glm::cross(right, a);

    const float angle = glm::radians(spreadDegrees) * std::sqrt(random());
    const float roll = random() * 6.2831853f;
    return glm::normalize(a + (right * std::cos(roll) + realUp * std::sin(roll)) *
                                  std::tan(angle));
}

ParticleSystem::Particle& ParticleSystem::allocate() {
    // Ring allocation: always O(1), never allocates, oldest dies first when
    // the pool is saturated.
    Particle& p = m_particles[m_next];
    m_next = (m_next + 1) % m_particles.size();
    return p;
}

void ParticleSystem::emitMuzzleFlash(const glm::vec3& position, const glm::vec3& direction) {
    // One bright core flash...
    Particle& core = allocate();
    core.position = position + direction * 0.25f;
    core.velocity = direction * 1.5f;
    core.colour   = glm::vec4{1.0f, 0.82f, 0.42f, 1.0f};
    core.life = core.maxLife = 0.055f;
    core.size = 0.28f;
    core.sizeGrow = 1.6f;
    core.drag = 6.0f;
    core.emissive = 4.0f;

    // ...plus a spray of sparks that sell the direction of fire.
    for (int i = 0; i < 7; ++i) {
        Particle& spark = allocate();
        spark.position = position + direction * 0.2f;
        spark.velocity = randomCone(direction, 22.0f) * (5.0f + random() * 7.0f);
        spark.colour = glm::vec4{1.0f, 0.65f + random() * 0.25f, 0.25f, 1.0f};
        spark.life = spark.maxLife = 0.10f + random() * 0.12f;
        spark.size = 0.022f;
        spark.sizeGrow = -0.01f;
        spark.drag = 3.0f;
        spark.gravity = -6.0f;
        spark.emissive = 3.0f;
    }
}

void ParticleSystem::emitImpact(const glm::vec3& position, const glm::vec3& normal) {
    for (int i = 0; i < 10; ++i) {
        Particle& p = allocate();
        p.position = position + normal * 0.02f;
        p.velocity = randomCone(normal, 62.0f) * (2.0f + random() * 5.0f);
        p.colour = glm::vec4{0.85f, 0.80f, 0.72f, 1.0f};
        p.life = p.maxLife = 0.25f + random() * 0.35f;
        p.size = 0.018f + random() * 0.02f;
        p.sizeGrow = 0.0f;
        p.drag = 2.2f;
        p.gravity = -11.0f;
        p.emissive = 0.15f;
    }
    // Dust puff at the point of impact.
    Particle& puff = allocate();
    puff.position = position + normal * 0.05f;
    puff.velocity = normal * 0.6f;
    puff.colour = glm::vec4{0.55f, 0.53f, 0.48f, 0.55f};
    puff.life = puff.maxLife = 0.55f;
    puff.size = 0.14f;
    puff.sizeGrow = 0.9f;
    puff.drag = 3.5f;
    puff.emissive = 0.0f;
}

void ParticleSystem::emitBlood(const glm::vec3& position) {
    for (int i = 0; i < 14; ++i) {
        Particle& p = allocate();
        p.position = position;
        p.velocity = glm::vec3{(random() - 0.5f) * 4.0f, random() * 3.0f,
                               (random() - 0.5f) * 4.0f};
        p.colour = glm::vec4{0.45f, 0.05f, 0.05f, 0.9f};
        p.life = p.maxLife = 0.4f + random() * 0.4f;
        p.size = 0.03f + random() * 0.03f;
        p.drag = 1.6f;
        p.gravity = -14.0f;
        p.emissive = 0.0f;
    }
}

void ParticleSystem::emitDust(const glm::vec3& position) {
    Particle& p = allocate();
    p.position = position + glm::vec3{(random() - 0.5f) * 0.3f, 0.02f, (random() - 0.5f) * 0.3f};
    p.velocity = glm::vec3{(random() - 0.5f) * 0.4f, 0.25f + random() * 0.3f,
                           (random() - 0.5f) * 0.4f};
    p.colour = glm::vec4{0.52f, 0.50f, 0.46f, 0.30f};
    p.life = p.maxLife = 0.6f + random() * 0.4f;
    p.size = 0.08f;
    p.sizeGrow = 0.5f;
    p.drag = 2.8f;
    p.emissive = 0.0f;
}

void ParticleSystem::emitSpark(const glm::vec3& position, const glm::vec3& velocity, float life) {
    Particle& p = allocate();
    p.position = position;
    p.velocity = velocity;
    p.colour = glm::vec4{1.0f, 0.75f, 0.35f, 1.0f};
    p.life = p.maxLife = life;
    p.size = 0.02f;
    p.drag = 1.5f;
    p.gravity = -9.0f;
    p.emissive = 3.0f;
}

void ParticleSystem::update(float dt, JobSystem& jobs) {
    const size_t count = m_particles.size();

    jobs.parallelFor(count, 512, [this, dt](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            Particle& p = m_particles[i];
            if (p.life <= 0.0f) continue;

            p.life -= dt;
            if (p.life <= 0.0f) { p.life = 0.0f; continue; }

            p.velocity.y += p.gravity * dt;
            p.velocity -= p.velocity * std::min(1.0f, p.drag * dt);
            p.position += p.velocity * dt;
            p.size = std::max(0.001f, p.size + p.sizeGrow * dt);

            // Cheap floor collision so sparks skitter instead of sinking.
            if (p.position.y < 0.02f && p.velocity.y < 0.0f) {
                p.position.y = 0.02f;
                p.velocity.y *= -0.32f;
                p.velocity.x *= 0.6f;
                p.velocity.z *= 0.6f;
            }
        }
    });

    m_alive = 0;
    for (const Particle& p : m_particles)
        if (p.life > 0.0f) ++m_alive;
}

void ParticleSystem::buildInstances(const glm::vec3& cameraRight, const glm::vec3& cameraUp,
                                    std::vector<InstanceData>& out) const {
    for (const Particle& p : m_particles) {
        if (p.life <= 0.0f) continue;

        const float t = p.life / std::max(p.maxLife, 0.0001f);
        const float fade = t * t;   // quadratic fade reads as a sharper falloff

        // Billboard: build the basis directly from camera axes rather than
        // computing a lookAt per particle.
        glm::mat4 model{1.0f};
        model[0] = glm::vec4(cameraRight * p.size, 0.0f);
        model[1] = glm::vec4(cameraUp * p.size, 0.0f);
        model[2] = glm::vec4(glm::cross(cameraRight, cameraUp) * p.size, 0.0f);
        model[3] = glm::vec4(p.position, 1.0f);

        InstanceData instance;
        instance.model = model;
        instance.albedoRoughness = glm::vec4(p.colour.r, p.colour.g, p.colour.b, 1.0f);
        instance.params = glm::vec4(p.emissive * fade * p.colour.a, 0.0f, 0.0f, 0.0f);
        out.push_back(instance);
    }
}

}  // namespace vajra
