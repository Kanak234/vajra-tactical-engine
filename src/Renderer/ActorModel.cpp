#include "Renderer/ActorModel.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace vajra {

namespace {

/// Appends one oriented box, expressed in the actor's local space.
void part(const glm::mat4& actorToWorld, const glm::vec3& localCentre,
          const glm::vec3& halfExtents, const glm::vec3& colour, float roughness,
          std::vector<InstanceData>& out, float pitch = 0.0f, float roll = 0.0f) {
    glm::mat4 model = glm::translate(actorToWorld, localCentre);
    if (pitch != 0.0f) model = glm::rotate(model, pitch, glm::vec3{1.0f, 0.0f, 0.0f});
    if (roll  != 0.0f) model = glm::rotate(model, roll,  glm::vec3{0.0f, 0.0f, 1.0f});
    model = glm::scale(model, halfExtents * 2.0f);

    InstanceData instance;
    instance.model = model;
    instance.albedoRoughness = glm::vec4(colour, roughness);
    instance.params = glm::vec4(0.0f);
    out.push_back(instance);
}

/// Rotates a limb about its top end rather than its centre, which is what
/// makes a swinging arm look hinged at the shoulder instead of floating.
void limb(const glm::mat4& actorToWorld, const glm::vec3& pivot, float length,
          float thickness, float pitch, const glm::vec3& colour, float roughness,
          std::vector<InstanceData>& out) {
    glm::mat4 model = glm::translate(actorToWorld, pivot);
    model = glm::rotate(model, pitch, glm::vec3{1.0f, 0.0f, 0.0f});
    model = glm::translate(model, glm::vec3{0.0f, -length * 0.5f, 0.0f});
    model = glm::scale(model, glm::vec3{thickness, length, thickness});

    InstanceData instance;
    instance.model = model;
    instance.albedoRoughness = glm::vec4(colour, roughness);
    instance.params = glm::vec4(0.0f);
    out.push_back(instance);
}

}  // namespace

float advanceWalkPhase(float phase, float speed, float dt) {
    // Stride frequency scales with speed so the feet do not skate.
    const float frequency = 1.15f + speed * 0.55f;
    phase += dt * frequency * 6.2831853f * (speed > 0.05f ? 1.0f : 0.0f);
    if (phase > 6.2831853f) phase -= 6.2831853f;
    return phase;
}

void buildActorInstances(const ActorPose& pose, std::vector<InstanceData>& out) {
    const glm::vec3 skin{0.62f, 0.48f, 0.38f};
    const glm::vec3 gear = pose.tint;
    const glm::vec3 dark = gear * 0.55f;
    const glm::vec3 metal{0.13f, 0.13f, 0.14f};

    const float crouchDrop = pose.crouch * 0.38f;
    const float deathDrop  = pose.death * 0.95f;

    glm::mat4 actor = glm::translate(glm::mat4{1.0f}, pose.position);
    actor = glm::rotate(actor, pose.yaw, glm::vec3{0.0f, 1.0f, 0.0f});

    // A dead actor rotates onto its side and drops to the floor.
    if (pose.death > 0.001f) {
        actor = glm::translate(actor, glm::vec3{0.0f, 0.30f * pose.death, 0.0f});
        actor = glm::rotate(actor, pose.death * 1.5708f, glm::vec3{1.0f, 0.0f, 0.0f});
    }

    // Gait: stride amplitude grows with speed, and the body bobs at twice the
    // stride frequency, which is what real walking does.
    const float stride = std::min(0.85f, pose.walkSpeed * 0.22f);
    const float swing  = std::sin(pose.walkPhase) * stride;
    const float bob    = std::abs(std::cos(pose.walkPhase)) * stride * 0.055f;
    const float lean   = pose.walkSpeed * 0.028f + pose.crouch * 0.30f;

    const float hipY   = 0.92f - crouchDrop - deathDrop + bob;
    const float chestY = 1.32f - crouchDrop * 1.05f - deathDrop + bob;
    const float headY  = 1.62f - crouchDrop * 1.15f - deathDrop + bob;

    // Legs, hinged at the hips.
    limb(actor, {-0.11f, hipY, 0.0f}, 0.86f - crouchDrop * 0.5f, 0.17f,  swing, dark, 0.8f, out);
    limb(actor, { 0.11f, hipY, 0.0f}, 0.86f - crouchDrop * 0.5f, 0.17f, -swing, dark, 0.8f, out);

    // Pelvis and torso. The torso leans forward when moving or crouching.
    part(actor, {0.0f, hipY + 0.06f, 0.0f}, {0.17f, 0.10f, 0.12f}, dark, 0.8f, out);
    part(actor, {0.0f, (hipY + chestY) * 0.5f + 0.08f, lean * 0.10f},
         {0.19f, 0.24f, 0.13f}, gear, pose.roughness, out, -lean);

    // Chest rig — a lighter block reads as webbing and breaks the silhouette.
    part(actor, {0.0f, chestY - 0.02f, 0.13f}, {0.15f, 0.11f, 0.04f},
         gear * 1.25f, 0.65f, out, -lean);

    // Head and helmet.
    part(actor, {0.0f, headY, 0.02f}, {0.085f, 0.10f, 0.09f}, skin, 0.6f, out);
    part(actor, {0.0f, headY + 0.08f, 0.01f}, {0.105f, 0.055f, 0.11f}, dark, 0.5f, out);

    // Arms. When aiming, both come up and forward to hold the weapon; when
    // not, the outer arm swings counter to the legs.
    const float aimPitch = -1.15f * pose.aim;
    const float leftArm  = aimPitch + (1.0f - pose.aim) * -swing;
    const float rightArm = aimPitch + (1.0f - pose.aim) *  swing;

    limb(actor, {-0.26f, chestY + 0.10f, 0.0f}, 0.60f, 0.115f, leftArm,  gear, 0.75f, out);
    limb(actor, { 0.26f, chestY + 0.10f, 0.0f}, 0.60f, 0.115f, rightArm, gear, 0.75f, out);

    // Rifle: slung across the chest at rest, shouldered when aiming.
    const float weaponY = chestY + 0.02f - pose.aim * 0.06f;
    const float weaponZ = 0.16f + pose.aim * 0.30f;
    const float weaponRoll = (1.0f - pose.aim) * 0.5f;
    part(actor, {0.10f, weaponY, weaponZ}, {0.035f, 0.045f, 0.34f}, metal, 0.42f,
         out, 0.0f, weaponRoll);
    part(actor, {0.10f, weaponY - 0.09f, weaponZ + 0.06f}, {0.028f, 0.075f, 0.05f},
         metal, 0.42f, out, 0.0f, weaponRoll);
}

}  // namespace vajra
