#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "Renderer/Mesh.h"

namespace vajra {

/// Procedural articulated figure. Without an artist there are no rigged
/// meshes to skin, so instead of a skeletal-animation system with nothing to
/// play, this builds a jointed body out of boxes and animates it directly:
/// a real walk cycle, aim pose, and death slump driven by parameters.
///
/// Every part is one more instance in the same batch, so a fully animated
/// guard costs nothing extra in draw calls.
struct ActorPose {
    glm::vec3 position{0.0f};   // feet
    float yaw = 0.0f;           // radians
    float walkPhase = 0.0f;     // advanced by distance travelled
    float walkSpeed = 0.0f;     // metres per second, drives stride amplitude
    float crouch = 0.0f;        // 0 standing, 1 crouched
    float aim = 0.0f;           // 0 rifle down, 1 shouldered
    float death = 0.0f;         // 0 alive, 1 collapsed
    glm::vec3 tint{0.28f, 0.31f, 0.27f};
    float roughness = 0.72f;
};

/// Appends the boxes making up one posed figure to the instance list.
void buildActorInstances(const ActorPose& pose, std::vector<InstanceData>& out);

/// Advances a walk phase given distance moved this frame.
float advanceWalkPhase(float phase, float speed, float dt);

}  // namespace vajra
