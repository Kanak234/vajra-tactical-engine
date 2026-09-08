#include "Core/Application.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <nlohmann/json.hpp>

#include "Core/Log.h"
#include "Scene/Components.h"

namespace vajra {

namespace {

constexpr const char* kAssetDir =
#ifdef VAJRA_ASSET_DIR
    VAJRA_ASSET_DIR;
#else
    "assets/";
#endif

std::string asset(const std::string& rel) { return std::string(kAssetDir) + rel; }

const glm::vec4 kInk{0.85f, 0.88f, 0.86f, 1.0f};
const glm::vec4 kAmber{0.98f, 0.72f, 0.24f, 1.0f};
const glm::vec4 kRed{0.90f, 0.25f, 0.20f, 1.0f};
const glm::vec4 kGreen{0.42f, 0.85f, 0.45f, 1.0f};
const glm::vec4 kPanel{0.03f, 0.04f, 0.05f, 0.62f};
const glm::vec4 kDim{0.55f, 0.60f, 0.58f, 1.0f};

bool losCallback(const glm::vec3& from, const glm::vec3& to, void* userData) {
    return static_cast<const PhysicsWorld*>(userData)->lineOfSight(from, to);
}

std::string formatInt(int value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d", value);
    return buffer;
}

std::string formatTime(float seconds) {
    const int total = static_cast<int>(seconds);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", total / 60, total % 60);
    return buffer;
}

InstanceData boxInstance(const glm::vec3& centre, const glm::vec3& halfExtents,
                         const glm::vec3& albedo, float roughness, float emissive = 0.0f) {
    InstanceData instance;
    instance.model = glm::scale(glm::translate(glm::mat4{1.0f}, centre), halfExtents * 2.0f);
    instance.albedoRoughness = glm::vec4(albedo, roughness);
    instance.params = glm::vec4(emissive, 0.0f, 0.0f, 0.0f);
    return instance;
}

}  // namespace

// ---------------------------------------------------------------- lifecycle --

Application::Application()
    : m_window(WindowSpec{"VAJRA - The Quiet Station", 1600, 900, true, 4, 6}) {
    m_lit       = Shader{asset("shaders/lit.vert"),   asset("shaders/lit.frag")};
    m_depth     = Shader{asset("shaders/depth.vert"), asset("shaders/depth.frag")};
    m_hudShader = Shader{asset("shaders/hud.vert"),   asset("shaders/hud.frag")};

    m_cube = Mesh::makeCube(1.0f);
    m_quad = Mesh::makeQuad(1.0f);

    m_shadow.init(2048);
    m_post.init(m_window.width(), m_window.height(), asset("shaders/"));
    m_hud.init();
    m_audio.init();
    m_jobs.start();
    m_particles.init(4096);

    m_solidInstances.reserve(4096);
    m_allInstances.reserve(6144);
    m_particleInstances.reserve(4096);

    m_weapons.emplace_back(WeaponId::SilencedRifle);
    m_weapons.emplace_back(WeaponId::Sidearm);

    m_campaign.init(asset("levels/"));
    loadLevel(m_campaign.pathFor(0));
    openMenu();

    m_camera.setAspect(m_window.aspect());

    VJ_INFO("Renderer: HDR + SSAO + bloom + FXAA | %u worker threads", m_jobs.workerCount());
    VJ_INFO("WASD move | Shift sprint | Ctrl crouch | LMB fire | RMB aim | E interact");
    VJ_INFO("R reload | 1/2 weapons | H medkit | F3 debug | F6 save | F9 load");
}

Application::~Application() {
    m_jobs.stop();
    m_hud.shutdown();
    m_post.shutdown();
    m_shadow.shutdown();
    m_audio.shutdown();
}

void Application::loadLevel(const std::string& path) {
    m_level  = Level::loadFromFile(path);
    m_ground = Mesh::makePlane(m_level.groundSize * 2.0f, 8);
    // Procedural Himalayan backdrop, generated from a fixed seed so the
    // skyline is identical every run.
    m_mountains = Mesh::makeMountainRing(320.0f, 460.0f, 96, 55.0f, 165.0f, 20260730u);
    buildWorldFromLevel();
}

void Application::startMission(size_t index) {
    if (index >= m_campaign.count() || !m_campaign.isUnlocked(index)) return;
    m_currentMission = index;
    loadLevel(m_campaign.pathFor(index));
    m_state = GameState::Playing;
    Window::captureMouse(true);
}

void Application::openMenu() {
    m_state = GameState::Menu;
    m_menuCursor = m_currentMission;
    Window::captureMouse(false);
}

void Application::buildWorldFromLevel() {
    m_registry.clear();
    m_world.clear();

    for (const LevelBox& box : m_level.boxes) {
        if (box.collides)
            m_world.addBox(AABB::fromCentre(box.centre, box.halfExtents));

        const auto entity = m_registry.create();
        m_registry.emplace<Transform>(entity, Transform{
            box.centre, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, box.halfExtents * 2.0f});
        m_registry.emplace<MeshRenderer>(entity, MeshRenderer{
            &m_cube, box.albedo, 0.0f, box.roughness, true});
    }

    m_world.addBox(AABB{{-m_level.groundSize, -2.0f, -m_level.groundSize},
                        { m_level.groundSize,  0.0f,  m_level.groundSize}});

    const float cell = 1.0f;
    const int cells = static_cast<int>(m_level.groundSize / cell);
    m_nav.build(m_world,
                glm::vec3{-m_level.groundSize * 0.5f, 0.0f, -m_level.groundSize * 0.5f},
                cell, cells, cells, 0.42f, 1.7f);

    for (const LevelGuard& spawn : m_level.guards) {
        const auto entity = m_registry.create();
        m_registry.emplace<Transform>(entity, Transform{
            spawn.position, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, glm::vec3{0.7f, 1.75f, 0.7f}});
        m_registry.emplace<Health>(entity, Health{100.0f, 100.0f});
        m_registry.emplace<NavAgent>(entity);
        m_registry.emplace<ActorAnim>(entity, ActorAnim{0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                        spawn.position});

        Perception perception;
        perception.visionRange     = spawn.visionRange;
        perception.visionHalfAngle = spawn.visionHalfAngle;
        m_registry.emplace<Perception>(entity, perception);

        Combatant combatant;
        combatant.accuracy = spawn.accuracy;
        combatant.squadId  = spawn.squadId;
        m_registry.emplace<Combatant>(entity, combatant);

        if (spawn.isMachine) m_registry.emplace<Machine>(entity);

        PatrolRoute route;
        route.waypoints = spawn.patrol.empty()
            ? std::vector<glm::vec3>{spawn.position} : spawn.patrol;
        m_registry.emplace<PatrolRoute>(entity, route);
    }

    std::vector<ObjectiveState> objectives;
    for (const LevelObjective& spec : m_level.objectives) {
        objectives.push_back(ObjectiveState{spec.id, spec.label, spec.position,
                                            spec.radius, spec.requiresDestroy, false,
                                            spec.optional});
        if (spec.requiresDestroy) {
            const auto entity = m_registry.create();
            m_registry.emplace<Transform>(entity, Transform{
                spec.position + glm::vec3{0.0f, 0.75f, 0.0f},
                glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, glm::vec3{1.0f, 1.5f, 0.6f}});
            m_registry.emplace<MeshRenderer>(entity, MeshRenderer{
                &m_cube, glm::vec3{0.18f, 0.35f, 0.42f}, 0.2f, 0.4f, true});
            m_registry.emplace<Objective>(entity, Objective{
                spec.id, spec.label, spec.radius, true, false});
        }
    }

    m_mission.configure(m_level.title, objectives,
                        m_level.extractionPoint, m_level.extractionRadius);
    restart();
}

void Application::restart() {
    m_player.teleport(m_level.playerStart);
    m_playerHealth = 100.0f;
    m_medkits = 2;
    m_hurtFlash = 0.0f;
    m_tracers.clear();
    m_particles.clear();
    m_state = GameState::Playing;
    m_activeWeapon = 0;
    m_weapons.clear();
    m_weapons.emplace_back(WeaponId::SilencedRifle);
    m_weapons.emplace_back(WeaponId::Sidearm);
    m_guards.reset();
    m_mission.reset();

    auto view = m_registry.view<Health, Perception, Combatant, NavAgent, ActorAnim, Transform>();
    for (auto [entity, health, perception, combatant, agent, anim, transform] : view.each()) {
        health.current = health.max;
        perception.awareness = 0.0f;
        perception.hasLineOfSight = false;
        combatant.state = AlertState::Idle;
        combatant.stateTimer = 0.0f;
        combatant.fireCooldown = 0.0f;
        agent.path.clear();
        agent.hasGoal = false;
        anim.death = 0.0f;
        anim.walkSpeed = 0.0f;
        anim.lastPosition = transform.position;
    }

    // Restore spawn positions from the level definition.
    size_t index = 0;
    auto guards = m_registry.view<Transform, Combatant>();
    for (auto [entity, transform, combatant] : guards.each()) {
        if (index < m_level.guards.size()) transform.position = m_level.guards[index].position;
        ++index;
    }

    m_toast = "MISSION: " + m_level.title;
    m_toastTimer = 4.0f;
}

// ------------------------------------------------------------------- events --

void Application::pollEvents() {
    m_input.beginFrame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        m_input.handleEvent(e);
        if (e.type == SDL_QUIT) m_running = false;
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            m_window.onResize(e.window.data1, e.window.data2);
            m_camera.setAspect(m_window.aspect());
            m_post.resize(e.window.data1, e.window.data2);
        }
    }

    if (m_input.keyPressed(SDL_SCANCODE_ESCAPE)) {
        if (m_state == GameState::Playing) { m_state = GameState::Paused; Window::captureMouse(false); }
        else if (m_state == GameState::Paused) { m_state = GameState::Playing; Window::captureMouse(true); }
    }
    if (m_input.mousePressed(SDL_BUTTON_LEFT) && m_state == GameState::Paused) {
        m_state = GameState::Playing;
        Window::captureMouse(true);
    }
    if (m_input.keyPressed(SDL_SCANCODE_F5)) {
        m_lit.reload(); m_depth.reload(); m_hudShader.reload(); m_post.reloadShaders();
        m_toast = "SHADERS RELOADED"; m_toastTimer = 2.0f;
    }
    if (m_input.keyPressed(SDL_SCANCODE_F3)) m_showDebug = !m_showDebug;
    if (m_input.keyPressed(SDL_SCANCODE_F6)) saveGame();
    if (m_input.keyPressed(SDL_SCANCODE_F9)) loadGame();

    // Live graphics toggles — useful for both debugging and low-end machines.
    if (m_input.keyPressed(SDL_SCANCODE_F7)) {
        m_post.setEnabled(!m_post.ssaoEnabled(), m_post.bloomEnabled(), m_post.fxaaEnabled());
        m_toast = m_post.ssaoEnabled() ? "SSAO ON" : "SSAO OFF"; m_toastTimer = 1.5f;
    }
    if (m_input.keyPressed(SDL_SCANCODE_F8)) {
        m_post.setEnabled(m_post.ssaoEnabled(), !m_post.bloomEnabled(), m_post.fxaaEnabled());
        m_toast = m_post.bloomEnabled() ? "BLOOM ON" : "BLOOM OFF"; m_toastTimer = 1.5f;
    }
    if (m_input.keyPressed(SDL_SCANCODE_F10)) {
        m_post.setEnabled(m_post.ssaoEnabled(), m_post.bloomEnabled(), !m_post.fxaaEnabled());
        m_toast = m_post.fxaaEnabled() ? "FXAA ON" : "FXAA OFF"; m_toastTimer = 1.5f;
    }

    if (m_state == GameState::Menu) {
        if (m_input.keyPressed(SDL_SCANCODE_DOWN) || m_input.keyPressed(SDL_SCANCODE_S))
            m_menuCursor = (m_menuCursor + 1) % m_campaign.count();
        if (m_input.keyPressed(SDL_SCANCODE_UP) || m_input.keyPressed(SDL_SCANCODE_W))
            m_menuCursor = (m_menuCursor + m_campaign.count() - 1) % m_campaign.count();
        for (size_t i = 0; i < m_campaign.count() && i < 9; ++i)
            if (m_input.keyPressed(static_cast<SDL_Scancode>(SDL_SCANCODE_1 + i)))
                m_menuCursor = i;
        if (m_input.keyPressed(SDL_SCANCODE_RETURN) || m_input.keyPressed(SDL_SCANCODE_SPACE))
            startMission(m_menuCursor);
        return;
    }

    if (m_state == GameState::Success) {
        // Enter advances to the next mission once it is unlocked; M goes back
        // to the list so a player can replay for a ghost run.
        if (m_input.keyPressed(SDL_SCANCODE_RETURN)) {
            if (m_campaign.hasNextAfter(m_currentMission))
                startMission(m_campaign.nextAfter(m_currentMission));
            else
                openMenu();
        }
        if (m_input.keyPressed(SDL_SCANCODE_M)) openMenu();
        if (m_input.keyPressed(SDL_SCANCODE_R)) startMission(m_currentMission);
    } else if (m_state == GameState::Failed) {
        if (m_input.keyPressed(SDL_SCANCODE_RETURN)) startMission(m_currentMission);
        if (m_input.keyPressed(SDL_SCANCODE_M)) openMenu();
    }
    if (m_state == GameState::Paused && m_input.keyPressed(SDL_SCANCODE_M)) openMenu();
}

// --------------------------------------------------------------- simulation --

float Application::highestAwareness() const {
    float highest = 0.0f;
    auto view = m_registry.view<const Perception, const Health>();
    for (auto [entity, perception, health] : view.each())
        if (health.alive()) highest = std::max(highest, perception.awareness);
    return highest;
}

void Application::firePlayerWeapon() {
    Weapon& weapon = activeWeapon();
    if (!weapon.tryFire()) return;

    const bool crouched = m_player.stance() == Stance::Crouched;
    const glm::vec3 origin = m_camera.position();
    const glm::vec3 dir = weapon.spreadDirection(m_camera.forward(), crouched);
    const float range = weapon.stats().range;

    const RayHit worldHit = m_world.raycast(origin, dir, range);
    float bestDistance = worldHit.hit ? worldHit.distance : range;
    entt::entity bestGuard = entt::null;

    auto view = m_registry.view<Transform, Health, Perception>();
    for (auto [entity, transform, health, perception] : view.each()) {
        if (!health.alive()) continue;
        const AABB body = AABB::fromCentre(transform.position + glm::vec3{0.0f, 0.9f, 0.0f},
                                           glm::vec3{0.38f, 0.9f, 0.38f});
        float distance = 0.0f;
        if (rayAabb(origin, dir, body, bestDistance, distance) && distance < bestDistance) {
            bestDistance = distance;
            bestGuard = entity;
        }
    }

    const glm::vec3 impact = origin + dir * bestDistance;
    m_tracers.push_back({origin + dir * 0.35f, impact, 0.06f, true});
    m_particles.emitMuzzleFlash(origin + m_camera.forward() * 0.4f -
                                m_camera.right() * 0.10f + glm::vec3{0.0f, -0.12f, 0.0f}, dir);

    if (bestGuard != entt::null) {
        auto& health = m_registry.get<Health>(bestGuard);
        health.current -= weapon.stats().damage;
        m_particles.emitBlood(impact);
        if (health.current <= 0.0f) {
            health.current = 0.0f;
            m_registry.get<Combatant>(bestGuard).state = AlertState::Idle;
            m_toast = "TARGET DOWN";
            m_toastTimer = 1.6f;
        }
        m_audio.playAt(Sfx::Impact, impact, m_camera.position(), m_camera.right(), 0.9f, 60.0f);
    } else {
        m_particles.emitImpact(impact, worldHit.hit ? worldHit.normal : -dir);
        m_audio.playAt(Sfx::Impact, impact, m_camera.position(), m_camera.right(), 0.55f, 45.0f);
    }

    m_audio.play(weapon.stats().suppressed ? Sfx::SuppressedShot : Sfx::LoudShot, 0.85f, 0.0f);
    m_camera.addRecoil(weapon.stats().recoilPitch, (std::rand() % 100 - 50) * 0.006f);
    m_perception.emitNoise(m_registry, m_camera.position(), weapon.stats().noiseRadius / 25.0f);
}

void Application::tryInteract() {
    const glm::vec3 feet = m_player.feetPosition();
    auto view = m_registry.view<Transform, Objective>();
    for (auto [entity, transform, objective] : view.each()) {
        if (objective.complete) continue;
        if (glm::length2(feet - transform.position) > objective.radius * objective.radius) continue;

        objective.complete = true;
        m_mission.markDestroyed(objective.id);
        if (auto* renderer = m_registry.try_get<MeshRenderer>(entity))
            renderer->albedo = glm::vec3{0.12f, 0.13f, 0.12f};

        for (int i = 0; i < 18; ++i)
            m_particles.emitSpark(transform.position + glm::vec3{0.0f, 0.9f, 0.0f},
                                  glm::vec3{(std::rand() % 200 - 100) * 0.03f,
                                            (std::rand() % 100) * 0.04f,
                                            (std::rand() % 200 - 100) * 0.03f},
                                  0.3f + (std::rand() % 100) * 0.004f);

        m_audio.play(Sfx::ObjectiveComplete, 0.7f);
        m_toast = "OBJECTIVE COMPLETE";
        m_toastTimer = 2.5f;
        m_perception.emitNoise(m_registry, transform.position, 0.6f);
        return;
    }
}

void Application::simulate(float dt) {
    if (m_state != GameState::Playing) { m_time += dt; return; }
    ScopedTimer timer(m_profiler, Profiler::Simulate);

    m_time += dt;
    m_player.update(m_input, m_camera, m_world, dt);

    for (Weapon& weapon : m_weapons) weapon.update(dt);
    activeWeapon().setAiming(m_input.mouse(SDL_BUTTON_RIGHT));

    if (m_input.keyPressed(SDL_SCANCODE_1)) m_activeWeapon = 0;
    if (m_input.keyPressed(SDL_SCANCODE_2) && m_weapons.size() > 1) m_activeWeapon = 1;
    if (m_input.keyPressed(SDL_SCANCODE_R)) {
        activeWeapon().beginReload();
        m_audio.play(Sfx::Reload, 0.6f);
    }
    if (m_input.keyPressed(SDL_SCANCODE_E)) tryInteract();
    if (m_input.keyPressed(SDL_SCANCODE_H) && m_medkits > 0 && m_playerHealth < 100.0f) {
        --m_medkits;
        m_playerHealth = std::min(100.0f, m_playerHealth + 45.0f);
        m_toast = "MEDKIT USED";
        m_toastTimer = 1.8f;
    }
    if (m_input.mouse(SDL_BUTTON_LEFT)) firePlayerWeapon();

    const float targetFov = activeWeapon().aiming() ? activeWeapon().stats().adsFov : 70.0f;
    static float currentFov = 70.0f;
    currentFov += (targetFov - currentFov) * std::min(1.0f, dt * 12.0f);
    m_camera.setFov(currentFov);

    const float speed = m_player.speed();
    if (speed > 0.4f && m_player.grounded()) {
        m_footstepTimer -= dt * speed;
        if (m_footstepTimer <= 0.0f) {
            m_footstepTimer = 2.6f;
            m_audio.play(Sfx::Footstep, m_player.sprinting() ? 0.45f : 0.22f);
            m_particles.emitDust(m_player.feetPosition());
        }
    }

    {
        ScopedTimer aiTimer(m_profiler, Profiler::Ai);
        m_perception.tick(m_registry, dt, &losCallback,
                          const_cast<void*>(static_cast<const void*>(&m_world)));

        GuardSystem::PlayerRef ref;
        ref.eyePosition  = m_camera.position();
        ref.feetPosition = m_player.feetPosition();
        ref.health       = m_playerHealth;
        ref.noise        = m_player.noiseLevel();
        ref.alive        = m_playerHealth > 0.0f;

        GuardEvents events;
        events.onShotFired = [this](const glm::vec3& from, const glm::vec3& to) {
            m_tracers.push_back({from, to, 0.07f, false});
            m_particles.emitMuzzleFlash(from, glm::normalize(to - from));
            m_audio.playAt(Sfx::EnemyShot, from, m_camera.position(), m_camera.right(), 1.0f, 80.0f);
        };
        events.onPlayerHit = [this](float amount) {
            m_hurtFlash = std::min(1.0f, m_hurtFlash + amount / 40.0f);
            m_audio.play(Sfx::PlayerHurt, 0.7f);
        };
        events.onAlarmRaised = [this](const glm::vec3&) {
            m_audio.play(Sfx::Alarm, 0.5f);
            m_toast = "SECTOR ALERTED";
            m_toastTimer = 3.0f;
        };

        m_guards.tick(m_registry, m_world, m_nav, ref, dt, events);
        m_playerHealth = ref.health;
    }

    // ---- animation state ----------------------------------------------------
    auto actors = m_registry.view<Transform, ActorAnim, Health, Combatant, Perception>();
    for (auto [entity, transform, anim, health, combatant, perception] : actors.each()) {
        const float moved = glm::length(glm::vec3{transform.position.x - anim.lastPosition.x,
                                                  0.0f,
                                                  transform.position.z - anim.lastPosition.z});
        const float instantSpeed = dt > 0.0001f ? moved / dt : 0.0f;
        anim.walkSpeed += (instantSpeed - anim.walkSpeed) * std::min(1.0f, dt * 9.0f);
        anim.walkPhase = advanceWalkPhase(anim.walkPhase, anim.walkSpeed, dt);
        anim.lastPosition = transform.position;

        const float wantAim = (combatant.state == AlertState::Combat) ? 1.0f : 0.0f;
        anim.aim += (wantAim - anim.aim) * std::min(1.0f, dt * 7.0f);

        const float wantCrouch = (combatant.state == AlertState::Searching) ? 0.35f : 0.0f;
        anim.crouch += (wantCrouch - anim.crouch) * std::min(1.0f, dt * 4.0f);

        if (!health.alive()) anim.death += (1.0f - anim.death) * std::min(1.0f, dt * 6.0f);
    }

    m_mission.update(m_player.feetPosition(), m_playerHealth > 0.0f, dt);
    if (m_mission.status() == MissionStatus::Success) {
        m_state = GameState::Success;
        Window::captureMouse(false);
        m_audio.play(Sfx::ObjectiveComplete, 0.9f);
        m_campaign.recordCompletion(m_currentMission, m_mission.elapsed(),
                                    !m_guards.sectorAlerted());
    } else if (m_mission.status() == MissionStatus::Failed) {
        m_state = GameState::Failed;
        Window::captureMouse(false);
    }

    m_hurtFlash = std::max(0.0f, m_hurtFlash - dt * 1.6f);
    if (m_toastTimer > 0.0f) m_toastTimer -= dt;

    for (Tracer& tracer : m_tracers) tracer.life -= dt;
    m_tracers.erase(std::remove_if(m_tracers.begin(), m_tracers.end(),
                                   [](const Tracer& t) { return t.life <= 0.0f; }),
                    m_tracers.end());

    m_particles.update(dt, m_jobs);
}

// ---------------------------------------------------------------- rendering --

void Application::buildInstanceLists() {
    m_solidInstances.clear();
    m_allInstances.clear();
    m_particleInstances.clear();
    m_culledCount = 0;

    m_frustum.extract(m_camera.projection() * m_camera.view());

    // Static geometry and objective devices, frustum-culled.
    auto statics = m_registry.view<Transform, MeshRenderer>();
    for (auto [entity, transform, renderer] : statics.each()) {
        const glm::vec3 half = transform.scale * 0.5f;
        if (!m_frustum.intersects(transform.position, half)) { ++m_culledCount; continue; }

        float emissive = 0.0f;
        glm::vec3 albedo = renderer.albedo;
        if (const auto* objective = m_registry.try_get<Objective>(entity)) {
            // Pulse the unfinished objectives so they read at a distance.
            emissive = objective->complete
                ? 0.0f
                : 0.35f + 0.25f * std::sin(m_time * 3.0f);
        }
        m_solidInstances.push_back(boxInstance(transform.position, half, albedo,
                                               renderer.roughness, emissive));
    }

    // Guards as fully articulated procedural actors.
    auto actors = m_registry.view<Transform, ActorAnim, Health, Combatant>();
    for (auto [entity, transform, anim, health, combatant] : actors.each()) {
        if (!m_frustum.intersects(transform.position + glm::vec3{0.0f, 1.0f, 0.0f},
                                  glm::vec3{1.0f, 1.2f, 1.0f})) { ++m_culledCount; continue; }

        ActorPose pose;
        pose.position = transform.position;
        pose.yaw = glm::eulerAngles(transform.rotation).y;
        pose.walkPhase = anim.walkPhase;
        pose.walkSpeed = anim.walkSpeed;
        pose.crouch = anim.crouch;
        pose.aim = anim.aim;
        pose.death = anim.death;

        // Alert state tints the uniform: a development readability aid that
        // also happens to look like different unit types.
        if (health.alive()) {
            switch (combatant.state) {
                case AlertState::Suspicious: pose.tint = {0.46f, 0.42f, 0.20f}; break;
                case AlertState::Searching:  pose.tint = {0.52f, 0.36f, 0.16f}; break;
                case AlertState::Combat:     pose.tint = {0.58f, 0.19f, 0.15f}; break;
                default:                     pose.tint = {0.28f, 0.31f, 0.27f}; break;
            }
        } else {
            pose.tint = {0.19f, 0.16f, 0.16f};
        }
        buildActorInstances(pose, m_solidInstances);
    }

    // Everything above casts shadows; effects below do not.
    m_allInstances = m_solidInstances;

    if (m_mission.extractionActive()) {
        const float pulse = 0.55f + 0.35f * std::sin(m_time * 4.0f);
        m_allInstances.push_back(boxInstance(
            m_mission.extractionPoint() + glm::vec3{0.0f, 0.04f, 0.0f},
            glm::vec3{m_level.extractionRadius, 0.04f, m_level.extractionRadius},
            glm::vec3{0.15f, 0.85f, 0.40f}, 0.3f, pulse));
    }

    for (const Tracer& tracer : m_tracers) {
        const glm::vec3 delta = tracer.to - tracer.from;
        const float length = glm::length(delta);
        if (length < 0.01f) continue;

        const glm::vec3 dir = delta / length;
        const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3{1, 0, 0} : glm::vec3{0, 1, 0};
        const glm::vec3 right = glm::normalize(glm::cross(dir, up));

        glm::mat4 basis{1.0f};
        basis[0] = glm::vec4(right, 0.0f);
        basis[1] = glm::vec4(glm::cross(right, dir), 0.0f);
        basis[2] = glm::vec4(dir, 0.0f);

        glm::mat4 model = glm::translate(glm::mat4{1.0f}, (tracer.from + tracer.to) * 0.5f) * basis;
        model = glm::scale(model, glm::vec3{0.02f, 0.02f, length});

        InstanceData instance;
        instance.model = model;
        instance.albedoRoughness = tracer.friendly
            ? glm::vec4{1.0f, 0.85f, 0.5f, 1.0f} : glm::vec4{1.0f, 0.45f, 0.25f, 1.0f};
        instance.params = glm::vec4{2.6f, 0.0f, 0.0f, 0.0f};
        m_allInstances.push_back(instance);
    }

    m_particles.buildInstances(m_camera.right(),
                               glm::normalize(glm::cross(m_camera.right(), m_camera.forward())),
                               m_particleInstances);
}

void Application::render() {
    const glm::vec3 sun = glm::normalize(m_level.sunDirection);
    const int w = m_window.width();
    const int h = m_window.height();

    buildInstanceLists();

    // ---- 1. shadow depth pass ----------------------------------------------
    {
        ScopedTimer timer(m_profiler, Profiler::Shadow);
        if (m_shadow.valid() && m_depth.valid()) {
            m_cube.uploadInstances(m_solidInstances);
            m_shadow.beginDepthPass(sun, m_player.feetPosition(), 48.0f);
            m_depth.bind();
            m_depth.set("uLightSpace", m_shadow.lightSpaceMatrix());
            m_cube.drawInstances();
            m_ground.drawSingle(InstanceData{});
            m_shadow.endDepthPass(w, h);
        }
    }

    const bool usePost = m_post.valid();
    m_cube.uploadInstances(m_allInstances);

    // ---- 2. depth + view-normal prepass -------------------------------------
    if (usePost) {
        ScopedTimer timer(m_profiler, Profiler::Prepass);
        m_post.beginPrepass();
        Shader& prepass = m_post.prepassShader();
        if (prepass.valid()) {
            prepass.bind();
            prepass.set("uView", m_camera.view());
            prepass.set("uProjection", m_camera.projection());
            m_cube.drawInstances();
            m_ground.drawSingle(InstanceData{});
        }
        m_post.endPrepass();
    }

    // ---- 3. SSAO -------------------------------------------------------------
    if (usePost) {
        ScopedTimer timer(m_profiler, Profiler::Ssao);
        m_post.runSsao(m_camera.projection(), m_camera.view(), 0.55f, 1.15f);
    }

    // ---- 4. forward scene into HDR ------------------------------------------
    {
        ScopedTimer timer(m_profiler, Profiler::Scene);
        if (usePost) m_post.beginScene();
        else         glViewport(0, 0, w, h);

        glClearColor(m_level.fogColour.r * 2.0f, m_level.fogColour.g * 2.0f,
                     m_level.fogColour.b * 2.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | (usePost ? GL_DEPTH_BUFFER_BIT : GL_DEPTH_BUFFER_BIT));

        if (m_lit.valid()) {
            m_lit.bind();
            m_lit.set("uView", m_camera.view());
            m_lit.set("uProjection", m_camera.projection());
            m_lit.set("uCameraPos", m_camera.position());
            m_lit.set("uLightDir", sun);
            m_lit.set("uLightColour", m_level.sunColour);
            m_lit.set("uFogColour", m_level.fogColour);
            m_lit.set("uLightSpace", m_shadow.lightSpaceMatrix());
            m_lit.set("uScreenSize", glm::vec2(static_cast<float>(w), static_cast<float>(h)));
            m_lit.set("uShadowMap", 0);
            m_lit.set("uAmbientOcclusion", 1);
            m_lit.set("uUseAo", (usePost && m_post.ssaoEnabled()) ? 1 : 0);

            m_shadow.bindTexture(GL_TEXTURE0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_post.ssaoTexture());

            // Backdrop first, with culling off so we see the inside of the ring.
            glDisable(GL_CULL_FACE);
            m_mountains.drawSingle(boxInstance(glm::vec3{0.0f, -6.0f, 0.0f},
                                               glm::vec3{0.5f}, glm::vec3{0.30f, 0.33f, 0.40f},
                                               0.95f));
            glEnable(GL_CULL_FACE);

            m_ground.drawSingle(boxInstance(glm::vec3{0.0f}, glm::vec3{0.5f},
                                            glm::vec3{0.17f, 0.19f, 0.17f}, 0.96f));
            m_cube.drawInstances();

            // Particles: additive, no depth write, drawn last.
            if (!m_particleInstances.empty()) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);
                m_quad.uploadInstances(m_particleInstances);
                m_quad.drawInstances();
                glEnable(GL_CULL_FACE);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        }
        if (usePost) m_post.endScene();
    }

    // ---- 5. bloom, tonemap, FXAA --------------------------------------------
    {
        ScopedTimer timer(m_profiler, Profiler::Post);
        if (usePost) {
            // Aiming down sights darkens the periphery slightly.
            const float vignette = 0.35f + activeWeapon().aiming() * 0.55f;
            m_post.resolveToScreen(w, h, m_exposure, 0.55f, vignette);
        }
    }

    // ---- 6. HUD --------------------------------------------------------------
    {
        ScopedTimer timer(m_profiler, Profiler::Hud);
        drawHud();
    }

    m_window.swap();
}

void Application::drawMenu(float w, float h, float s) {
    m_hud.rect(0, 0, w, h, glm::vec4{0.02f, 0.03f, 0.04f, 0.93f});

    auto centred = [&](const std::string& t, float y, float scale, const glm::vec4& c) {
        m_hud.text(t, w * 0.5f - Hud::textWidth(t, scale) * 0.5f, y, scale, c);
    };

    centred("VAJRA", h * 0.10f, s * 3.4f, kInk);
    centred("BUREAU OF STRATEGIC TECHNOLOGIES - OPERATION FILE",
            h * 0.10f + s * 22.0f, s * 0.8f, kAmber);

    const auto& missions = m_campaign.missions();
    const float listTop = h * 0.30f;
    const float rowH = std::max(46.0f, h * 0.085f);
    const float left = w * 0.5f - 250.0f;

    for (size_t i = 0; i < missions.size(); ++i) {
        const CampaignMission& mission = missions[i];
        const float y = listTop + static_cast<float>(i) * rowH;
        const bool selected = (i == m_menuCursor);

        if (selected)
            m_hud.rect(left - 16.0f, y - 10.0f, 532.0f, rowH - 8.0f,
                       glm::vec4{0.14f, 0.13f, 0.08f, 0.85f});

        const glm::vec4 titleColour = !mission.unlocked ? glm::vec4{0.32f, 0.32f, 0.32f, 1.0f}
                                    : selected ? kAmber : kInk;

        m_hud.text(formatInt(static_cast<int>(i) + 1) + ".", left - 4.0f, y, s * 1.1f, titleColour);
        m_hud.text(mission.unlocked ? mission.title : std::string("- LOCKED -"),
                   left + 30.0f, y, s * 1.1f, titleColour);
        m_hud.text(mission.unlocked ? mission.subtitle : std::string("COMPLETE THE PREVIOUS MISSION"),
                   left + 30.0f, y + 14.0f * s * 0.55f, s * 0.62f, kDim);

        if (mission.completed) {
            const std::string record = "BEST " + formatTime(mission.bestTime);
            m_hud.text(record, left + 400.0f, y, s * 0.7f, kGreen);
            if (mission.ghosted)
                m_hud.text("GHOST", left + 400.0f, y + 12.0f * s * 0.6f, s * 0.7f, kGreen);
        }
    }

    const int done = m_campaign.completedCount();
    centred(formatInt(done) + " / " + formatInt(static_cast<int>(m_campaign.count())) +
            " MISSIONS COMPLETE", h * 0.80f, s * 0.85f, kDim);
    centred("UP / DOWN SELECT     ENTER DEPLOY", h * 0.87f, s * 0.9f, kAmber);
}

void Application::drawHud() {
    const float w = static_cast<float>(m_window.width());
    const float h = static_cast<float>(m_window.height());
    const float s = std::max(2.0f, std::round(h / 300.0f));

    m_hud.begin(m_window.width(), m_window.height());

    if (m_state == GameState::Menu) {
        drawMenu(w, h, s);
        m_hud.end(m_hudShader);
        return;
    }

    if (m_hurtFlash > 0.01f)
        m_hud.rect(0, 0, w, h, glm::vec4{0.7f, 0.05f, 0.05f, m_hurtFlash * 0.35f});

    if (m_state == GameState::Playing) {
        const bool crouched = m_player.stance() == Stance::Crouched;
        const float spread = activeWeapon().currentSpread(crouched);
        m_hud.crosshair(w * 0.5f, h * 0.5f, 3.0f + spread * 2.6f, 6.0f,
                        glm::vec4{0.9f, 0.95f, 0.9f, 0.75f});

        const float margin = 24.0f;
        m_hud.text("HEALTH", margin, h - margin - 34.0f * s / 3.0f, s * 0.8f, kInk);
        m_hud.bar(margin, h - margin - 18.0f, 220.0f, 10.0f, m_playerHealth / 100.0f,
                  m_playerHealth > 35.0f ? kGreen : kRed, glm::vec4{0.1f, 0.1f, 0.1f, 0.7f});

        const Weapon& weapon = m_weapons[m_activeWeapon];
        const std::string ammo = formatInt(weapon.ammoInMag()) + " / " + formatInt(weapon.reserve());
        m_hud.text(ammo, w - margin - Hud::textWidth(ammo, s * 1.4f), h - margin - 22.0f,
                   s * 1.4f, kInk);
        m_hud.text(weapon.stats().name,
                   w - margin - Hud::textWidth(weapon.stats().name, s * 0.75f),
                   h - margin - 40.0f, s * 0.75f, kDim);
        if (weapon.reloading())
            m_hud.bar(w - margin - 140.0f, h - margin - 4.0f, 140.0f, 5.0f,
                      weapon.reloadProgress(), kAmber, glm::vec4{0.1f, 0.1f, 0.1f, 0.7f});
        if (!weapon.stats().suppressed)
            m_hud.text("LOUD", w - margin - Hud::textWidth("LOUD", s * 0.75f),
                       h - margin - 54.0f, s * 0.75f, kRed);

        m_hud.text("MEDKITS " + formatInt(m_medkits) + "  [H]", margin, h - margin + 2.0f,
                   s * 0.7f, kDim);

        const float awareness = highestAwareness();
        if (awareness > 0.02f) {
            const float meterW = 120.0f;
            const float mx = w * 0.5f - meterW * 0.5f;
            const float my = h * 0.5f - 60.0f;
            const glm::vec4 colour = awareness >= 0.99f ? kRed
                                   : awareness > 0.45f ? kAmber
                                   : glm::vec4{0.85f, 0.85f, 0.5f, 1.0f};
            m_hud.bar(mx, my, meterW, 6.0f, awareness, colour, glm::vec4{0.05f, 0.05f, 0.05f, 0.6f});
            const char* label = awareness >= 0.99f ? "DETECTED"
                              : awareness > 0.45f ? "SEARCHING" : "SUSPICIOUS";
            m_hud.text(label, w * 0.5f - Hud::textWidth(label, s * 0.7f) * 0.5f,
                       my - 12.0f, s * 0.7f, colour);
        }

        if (m_guards.sectorAlerted())
            m_hud.text("SECTOR ALERTED", w * 0.5f - Hud::textWidth("SECTOR ALERTED", s) * 0.5f,
                       24.0f, s, kRed);

        const ObjectiveState* current = m_mission.currentObjective();
        const std::string objectiveText = m_mission.extractionActive()
            ? std::string("EXTRACT - MOVE TO THE LZ")
            : (current ? current->label : std::string("STAND BY"));

        m_hud.rect(margin - 6.0f, margin - 6.0f,
                   Hud::textWidth(objectiveText, s * 0.9f) + 12.0f, 26.0f, kPanel);
        m_hud.text("OBJECTIVE", margin, margin, s * 0.65f, kAmber);
        m_hud.text(objectiveText, margin, margin + 12.0f, s * 0.9f, kInk);

        const glm::vec3 target = m_mission.extractionActive()
            ? m_mission.extractionPoint()
            : (current ? current->position : m_player.feetPosition());
        m_hud.text(formatInt(static_cast<int>(glm::length(target - m_player.feetPosition()))) + "M",
                   margin, margin + 26.0f, s * 0.7f, kDim);

        const std::string clock = formatTime(m_mission.elapsed());
        m_hud.text(clock, w - margin - Hud::textWidth(clock, s * 0.8f), margin, s * 0.8f, kDim);

        const char* stance = crouched ? "CROUCHED"
                           : m_player.sprinting() ? "SPRINTING - LOUD" : "STANDING";
        m_hud.text(stance, w * 0.5f - Hud::textWidth(stance, s * 0.7f) * 0.5f, h - 30.0f,
                   s * 0.7f, m_player.sprinting() ? kAmber : kDim);
    }

    if (m_toastTimer > 0.0f) {
        const float alpha = std::min(1.0f, m_toastTimer);
        m_hud.text(m_toast, w * 0.5f - Hud::textWidth(m_toast, s) * 0.5f, h * 0.35f, s,
                   glm::vec4{kAmber.r, kAmber.g, kAmber.b, alpha});
    }

    auto centred = [&](const std::string& t, float y, float scale, const glm::vec4& c) {
        m_hud.text(t, w * 0.5f - Hud::textWidth(t, scale) * 0.5f, y, scale, c);
    };

    if (m_state == GameState::Paused) {
        m_hud.rect(0, 0, w, h, glm::vec4{0.0f, 0.0f, 0.0f, 0.6f});
        centred("PAUSED", h * 0.42f, s * 2.2f, kInk);
        centred("CLICK OR ESC TO RESUME", h * 0.52f, s * 0.9f, kAmber);
        centred("M - ABANDON TO MISSION LIST", h * 0.58f, s * 0.8f, kDim);
    } else if (m_state == GameState::Success) {
        m_hud.rect(0, 0, w, h, glm::vec4{0.02f, 0.06f, 0.03f, 0.72f});
        centred("MISSION COMPLETE", h * 0.36f, s * 2.2f, kGreen);
        centred(m_level.title, h * 0.46f, s * 1.0f, kInk);
        centred("TIME " + formatTime(m_mission.elapsed()), h * 0.53f, s * 0.9f, kInk);
        centred(m_guards.sectorAlerted() ? "DETECTED - NO GHOST BONUS" : "UNDETECTED - GHOST",
                h * 0.59f, s * 0.9f, m_guards.sectorAlerted() ? kAmber : kGreen);
        const bool more = m_campaign.hasNextAfter(m_currentMission);
        centred(more ? "ENTER - NEXT MISSION" : "ENTER - RETURN TO FILE",
                h * 0.70f, s * 0.9f, kAmber);
        centred("R REPLAY     M MISSION LIST", h * 0.76f, s * 0.8f, kDim);
        if (!more && m_campaign.completedCount() == static_cast<int>(m_campaign.count()))
            centred("ALL MISSIONS COMPLETE - OPERATION VAJRA CLOSED",
                    h * 0.64f, s * 0.9f, kGreen);
    } else if (m_state == GameState::Failed) {
        m_hud.rect(0, 0, w, h, glm::vec4{0.10f, 0.01f, 0.01f, 0.72f});
        centred("MISSION FAILED", h * 0.40f, s * 2.2f, kRed);
        centred("ENTER - RETRY", h * 0.55f, s * 0.9f, kAmber);
        centred("M - MISSION LIST", h * 0.61f, s * 0.8f, kDim);
    }

    if (m_showDebug) {
        float y = h * 0.30f;
        const float x = 24.0f;
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "FPS %d", static_cast<int>(m_fps));
        m_hud.text(buffer, x, y, s * 0.75f, kGreen); y += 11.0f * s * 0.75f;

        for (int i = 0; i < Profiler::SlotCount; ++i) {
            const auto slot = static_cast<Profiler::Slot>(i);
            std::snprintf(buffer, sizeof(buffer), "%-8s %5.2F MS",
                          Profiler::name(slot), m_profiler.milliseconds(slot));
            m_hud.text(buffer, x, y, s * 0.7f, kDim);
            y += 10.0f * s * 0.7f;
        }
        std::snprintf(buffer, sizeof(buffer), "DRAWN %zu  CULLED %zu",
                      m_allInstances.size(), m_culledCount);
        m_hud.text(buffer, x, y, s * 0.7f, kGreen); y += 10.0f * s * 0.7f;
        std::snprintf(buffer, sizeof(buffer), "PARTICLES %zu / %zu",
                      m_particles.aliveCount(), m_particles.capacity());
        m_hud.text(buffer, x, y, s * 0.7f, kGreen); y += 10.0f * s * 0.7f;
        std::snprintf(buffer, sizeof(buffer), "THREADS %u  GUARDS %d",
                      m_jobs.workerCount(), m_guards.guardsAlive());
        m_hud.text(buffer, x, y, s * 0.7f, kGreen); y += 12.0f * s * 0.7f;
        m_hud.text("F7 SSAO  F8 BLOOM  F10 FXAA", x, y, s * 0.65f, kAmber);
    }

    m_hud.end(m_hudShader);
}

// -------------------------------------------------------------- persistence --

void Application::saveGame() {
    nlohmann::json root;
    root["level"]   = m_level.title;
    root["health"]  = m_playerHealth;
    root["medkits"] = m_medkits;
    root["elapsed"] = m_mission.elapsed();
    root["position"] = {m_player.feetPosition().x, m_player.feetPosition().y,
                        m_player.feetPosition().z};
    root["alerted"] = m_guards.sectorAlerted();

    nlohmann::json objectives = nlohmann::json::array();
    for (const auto& o : m_mission.objectives())
        objectives.push_back({{"id", o.id}, {"complete", o.complete}});
    root["objectives"] = objectives;

    std::ofstream out("vajra_save.json");
    if (!out) { m_toast = "SAVE FAILED"; m_toastTimer = 2.0f; return; }
    out << root.dump(2);
    m_toast = "GAME SAVED";
    m_toastTimer = 2.0f;
}

void Application::loadGame() {
    std::ifstream in("vajra_save.json");
    if (!in) { m_toast = "NO SAVE FOUND"; m_toastTimer = 2.0f; return; }

    nlohmann::json root;
    try { in >> root; }
    catch (const std::exception&) { m_toast = "SAVE CORRUPT"; m_toastTimer = 2.0f; return; }

    restart();
    m_playerHealth = root.value("health", 100.0f);
    m_medkits = root.value("medkits", 2);
    if (root.contains("position") && root["position"].is_array() && root["position"].size() >= 3)
        m_player.teleport(glm::vec3{root["position"][0].get<float>(),
                                    root["position"][1].get<float>(),
                                    root["position"][2].get<float>()});
    for (const auto& node : root.value("objectives", nlohmann::json::array()))
        if (node.value("complete", false)) m_mission.markDestroyed(node.value("id", std::string{}));

    m_state = GameState::Playing;
    Window::captureMouse(true);
    m_toast = "GAME LOADED";
    m_toastTimer = 2.0f;
}

// -------------------------------------------------------------------- frame --

void Application::run() {
    while (m_running) {
        m_profiler.begin(Profiler::Frame);
        const double delta = m_clock.beginFrame();
        m_fps = m_fps * 0.9f + static_cast<float>(1.0 / std::max(delta, 0.0001)) * 0.1f;

        pollEvents();
        if (m_state == GameState::Playing)
            m_camera.updateLook(m_input, static_cast<float>(delta));

        while (m_clock.consumeStep())
            simulate(static_cast<float>(m_clock.fixedStep()));

        render();
        m_profiler.end(Profiler::Frame);
    }
}

}  // namespace vajra
