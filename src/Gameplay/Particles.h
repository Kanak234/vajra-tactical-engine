#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "Renderer/Mesh.h"

namespace vajra {

class JobSystem;

/// Fixed-capacity CPU particle system. The pool is allocated once at startup
/// and never grows — emitting past capacity recycles the oldest particle
/// instead of allocating. That is the memory pooling the roadmap asked for,
/// in the one place the engine actually churns objects.
class ParticleSystem {
public:
    void init(size_t capacity = 4096);
    void clear();

    void emitMuzzleFlash(const glm::vec3& position, const glm::vec3& direction);
    void emitImpact(const glm::vec3& position, const glm::vec3& normal);
    void emitBlood(const glm::vec3& position);
    void emitDust(const glm::vec3& position);
    void emitSpark(const glm::vec3& position, const glm::vec3& velocity, float life);

    /// Simulation is split across the job system: it is trivially parallel and
    /// the only per-frame workload big enough to be worth threading.
    void update(float dt, JobSystem& jobs);

    /// Builds camera-facing billboard instances for rendering.
    void buildInstances(const glm::vec3& cameraRight, const glm::vec3& cameraUp,
                        std::vector<InstanceData>& out) const;

    [[nodiscard]] size_t aliveCount() const { return m_alive; }
    [[nodiscard]] size_t capacity() const { return m_particles.size(); }

private:
    struct Particle {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec4 colour{1.0f};
        float life = 0.0f;
        float maxLife = 1.0f;
        float size = 0.1f;
        float sizeGrow = 0.0f;
        float drag = 1.0f;
        float gravity = 0.0f;
        float emissive = 1.0f;
    };

    Particle& allocate();

    std::vector<Particle> m_particles;
    size_t m_next = 0;
    size_t m_alive = 0;
    unsigned m_rng = 1337u;

    float random();
    glm::vec3 randomCone(const glm::vec3& axis, float spreadDegrees);
};

}  // namespace vajra
