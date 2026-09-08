#pragma once
#include <SDL2/SDL.h>
#include <array>
#include <glm/glm.hpp>

namespace vajra {

enum class Sfx {
    SuppressedShot,
    LoudShot,
    EnemyShot,
    Impact,
    Footstep,
    Alarm,
    ObjectiveComplete,
    PlayerHurt,
    Reload,
    Count
};

/// Procedural audio engine. Every sound is synthesised at runtime — noise
/// bursts with envelopes for gunfire, filtered thumps for footsteps, tones for
/// UI. That means zero audio assets to ship, and the whole mix is tunable by
/// changing numbers instead of re-recording samples.
class AudioEngine {
public:
    bool init();
    void shutdown();

    /// Distance-attenuated one-shot. Pass the listener position each frame.
    void play(Sfx sfx, float volume = 1.0f, float pan = 0.0f);
    void playAt(Sfx sfx, const glm::vec3& source, const glm::vec3& listener,
                const glm::vec3& listenerRight, float volume = 1.0f, float falloff = 45.0f);

    void setMasterVolume(float v) { m_master = v; }
    [[nodiscard]] bool ready() const { return m_device != 0; }

private:
    struct Voice {
        bool  active   = false;
        Sfx   sfx      = Sfx::Impact;
        float phase    = 0.0f;   // seconds into the sound
        float duration = 0.0f;
        float volume   = 1.0f;
        float pan      = 0.0f;
        float seed     = 0.0f;
    };

    static void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);
    void mix(float* buffer, int frames);
    static float sample(const Voice& voice, float t, unsigned& rngState);

    SDL_AudioDeviceID m_device = 0;
    SDL_AudioSpec     m_spec{};
    static constexpr int kMaxVoices = 24;
    std::array<Voice, kMaxVoices> m_voices{};
    float m_master = 0.55f;
    unsigned m_rng = 22222u;
};

}  // namespace vajra
