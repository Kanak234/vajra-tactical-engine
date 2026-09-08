#pragma once
#include <entt/entt.hpp>
#include <string>
#include <vector>

#include "AI/GuardSystem.h"
#include "AI/Navigation.h"
#include "AI/PerceptionSystem.h"
#include "Audio/Audio.h"
#include "Core/JobSystem.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Gameplay/Campaign.h"
#include "Gameplay/CharacterController.h"
#include "Gameplay/MissionSystem.h"
#include "Gameplay/Particles.h"
#include "Gameplay/Weapon.h"
#include "Input/Input.h"
#include "Physics/Collision.h"
#include "Renderer/ActorModel.h"
#include "Renderer/Camera.h"
#include "Renderer/Frustum.h"
#include "Renderer/Hud.h"
#include "Renderer/Mesh.h"
#include "Renderer/PostProcess.h"
#include "Renderer/Shader.h"
#include "Renderer/ShadowMap.h"
#include "Scene/Level.h"

namespace vajra {

enum class GameState { Menu, Playing, Paused, Success, Failed };

class Application {
public:
    Application();
    ~Application();
    void run();

private:
    void loadLevel(const std::string& path);
    void startMission(size_t index);
    void openMenu();
    void drawMenu(float w, float h, float s);
    void buildWorldFromLevel();
    void restart();

    void pollEvents();
    void simulate(float dt);
    void render();
    void buildInstanceLists();
    void drawHud();

    void firePlayerWeapon();
    void tryInteract();
    void saveGame();
    void loadGame();

    [[nodiscard]] Weapon& activeWeapon() { return m_weapons[m_activeWeapon]; }
    [[nodiscard]] float highestAwareness() const;

    struct Tracer {
        glm::vec3 from{0.0f};
        glm::vec3 to{0.0f};
        float life = 0.0f;
        bool  friendly = true;
    };

    bool m_running = true;

    Window    m_window;
    Input     m_input;
    Camera    m_camera;
    StepClock m_clock{1.0 / 60.0};
    Profiler  m_profiler;
    JobSystem m_jobs;

    entt::registry m_registry;
    PhysicsWorld   m_world;
    NavGrid        m_nav;
    Level          m_level;
    Frustum        m_frustum;

    CharacterController m_player;
    float m_playerHealth = 100.0f;
    int   m_medkits = 2;
    float m_hurtFlash = 0.0f;
    float m_footstepTimer = 0.0f;

    std::vector<Weapon> m_weapons;
    size_t m_activeWeapon = 0;

    PerceptionSystem m_perception;
    GuardSystem      m_guards;
    MissionSystem    m_mission;
    Campaign         m_campaign;
    size_t           m_currentMission = 0;
    size_t           m_menuCursor = 0;
    ParticleSystem   m_particles;

    Shader    m_lit;
    Shader    m_depth;
    Shader    m_hudShader;
    Mesh      m_cube, m_ground, m_quad, m_mountains;
    ShadowMap m_shadow;
    PostProcess m_post;
    Hud       m_hud;
    AudioEngine m_audio;

    // Rebuilt every frame; reserved once so rendering never allocates.
    std::vector<InstanceData> m_solidInstances;
    std::vector<InstanceData> m_allInstances;
    std::vector<InstanceData> m_particleInstances;
    size_t m_culledCount = 0;

    std::vector<Tracer> m_tracers;
    GameState m_state = GameState::Playing;
    std::string m_toast;
    float m_toastTimer = 0.0f;
    bool  m_showDebug = false;
    float m_fps = 0.0f;
    float m_exposure = 1.0f;
    float m_time = 0.0f;
};

}  // namespace vajra
