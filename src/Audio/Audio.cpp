#include "Audio/Audio.h"

#include <algorithm>
#include <cmath>

#include "Core/Log.h"

namespace vajra {

namespace {

float fastRand(unsigned& state) {
    state = state * 1664525u + 1013904223u;
    return (static_cast<float>((state >> 9) & 0x7FFFFF) / 4194304.0f) - 1.0f;
}

float durationOf(Sfx sfx) {
    switch (sfx) {
        case Sfx::SuppressedShot:    return 0.16f;
        case Sfx::LoudShot:          return 0.42f;
        case Sfx::EnemyShot:         return 0.34f;
        case Sfx::Impact:            return 0.10f;
        case Sfx::Footstep:          return 0.09f;
        case Sfx::Alarm:             return 1.10f;
        case Sfx::ObjectiveComplete: return 0.55f;
        case Sfx::PlayerHurt:        return 0.28f;
        case Sfx::Reload:            return 0.14f;
        default:                     return 0.2f;
    }
}

}  // namespace

float AudioEngine::sample(const Voice& voice, float t, unsigned& rngState) {
    const float d = voice.duration;
    const float n = t / d;               // normalised progress 0..1
    if (n >= 1.0f) return 0.0f;

    switch (voice.sfx) {
        case Sfx::SuppressedShot: {
            // Short filtered noise puff with a low thump underneath.
            const float env = std::exp(-n * 14.0f);
            const float noise = fastRand(rngState) * 0.5f;
            const float thump = std::sin(6.2831853f * 130.0f * t) * std::exp(-n * 22.0f);
            return (noise * 0.55f + thump * 0.45f) * env;
        }
        case Sfx::LoudShot:
        case Sfx::EnemyShot: {
            // Sharp crack, then a decaying tail.
            const float crack = std::exp(-n * 45.0f);
            const float tail  = std::exp(-n * 5.0f);
            const float noise = fastRand(rngState);
            const float body  = std::sin(6.2831853f * 85.0f * t) * std::exp(-n * 12.0f);
            return noise * (crack * 0.8f + tail * 0.25f) + body * 0.4f;
        }
        case Sfx::Impact: {
            const float env = std::exp(-n * 30.0f);
            return fastRand(rngState) * env * 0.7f;
        }
        case Sfx::Footstep: {
            const float env = std::exp(-n * 26.0f);
            const float noise = fastRand(rngState) * 0.4f;
            const float low = std::sin(6.2831853f * 95.0f * t);
            return (noise + low * 0.6f) * env * 0.5f;
        }
        case Sfx::Alarm: {
            // Two-tone warble.
            const float warble = (std::fmod(t, 0.5f) < 0.25f) ? 620.0f : 460.0f;
            const float env = std::min(1.0f, n * 8.0f) * std::exp(-n * 1.4f);
            return std::sin(6.2831853f * warble * t) * env * 0.5f;
        }
        case Sfx::ObjectiveComplete: {
            // Rising two-note confirmation.
            const float freq = (n < 0.45f) ? 523.25f : 783.99f;
            const float env = std::exp(-n * 3.2f) * std::min(1.0f, n * 20.0f);
            return (std::sin(6.2831853f * freq * t) * 0.6f +
                    std::sin(6.2831853f * freq * 2.0f * t) * 0.2f) * env;
        }
        case Sfx::PlayerHurt: {
            const float env = std::exp(-n * 9.0f);
            const float freq = 180.0f * (1.0f - n * 0.5f);
            return (std::sin(6.2831853f * freq * t) * 0.6f +
                    fastRand(rngState) * 0.35f) * env;
        }
        case Sfx::Reload: {
            const float env = std::exp(-n * 34.0f);
            return (fastRand(rngState) * 0.3f +
                    std::sin(6.2831853f * 1400.0f * t) * 0.3f) * env;
        }
        default:
            return 0.0f;
    }
}

void AudioEngine::mix(float* buffer, int frames) {
    const float inverseRate = 1.0f / static_cast<float>(m_spec.freq);

    for (int i = 0; i < frames * 2; ++i) buffer[i] = 0.0f;

    for (Voice& voice : m_voices) {
        if (!voice.active) continue;

        unsigned rng = static_cast<unsigned>(voice.seed * 4294967295.0f) | 1u;
        // Advance the noise generator to where this voice already is, so
        // restarting the callback does not restart the noise.
        rng ^= static_cast<unsigned>(voice.phase * 100000.0f);

        const float leftGain  = std::sqrt(std::max(0.0f, 0.5f * (1.0f - voice.pan)));
        const float rightGain = std::sqrt(std::max(0.0f, 0.5f * (1.0f + voice.pan)));

        for (int f = 0; f < frames; ++f) {
            const float t = voice.phase + static_cast<float>(f) * inverseRate;
            if (t >= voice.duration) { voice.active = false; break; }
            const float s = sample(voice, t, rng) * voice.volume * m_master;
            buffer[f * 2 + 0] += s * leftGain * 1.41421356f;
            buffer[f * 2 + 1] += s * rightGain * 1.41421356f;
        }
        voice.phase += static_cast<float>(frames) * inverseRate;
        if (voice.phase >= voice.duration) voice.active = false;
    }

    // Soft clip so overlapping gunfire never hard-clips into a click.
    for (int i = 0; i < frames * 2; ++i)
        buffer[i] = std::tanh(buffer[i] * 1.15f);
}

void SDLCALL AudioEngine::audioCallback(void* userdata, Uint8* stream, int len) {
    auto* engine = static_cast<AudioEngine*>(userdata);
    engine->mix(reinterpret_cast<float*>(stream), len / static_cast<int>(sizeof(float) * 2));
}

bool AudioEngine::init() {
    SDL_AudioSpec want{};
    want.freq     = 48000;
    want.format   = AUDIO_F32SYS;
    want.channels = 2;
    want.samples  = 512;
    want.callback = &AudioEngine::audioCallback;
    want.userdata = this;

    m_device = SDL_OpenAudioDevice(nullptr, 0, &want, &m_spec, 0);
    if (m_device == 0) {
        VJ_WARN("Audio unavailable (%s) - running silent", SDL_GetError());
        return false;
    }
    SDL_PauseAudioDevice(m_device, 0);
    VJ_INFO("Audio: %d Hz, %d ch, %d frame buffer", m_spec.freq, m_spec.channels, m_spec.samples);
    return true;
}

void AudioEngine::shutdown() {
    if (m_device) {
        SDL_CloseAudioDevice(m_device);
        m_device = 0;
    }
}

void AudioEngine::play(Sfx sfx, float volume, float pan) {
    if (!m_device || volume <= 0.001f) return;

    SDL_LockAudioDevice(m_device);
    for (Voice& voice : m_voices) {
        if (voice.active) continue;
        voice.active   = true;
        voice.sfx      = sfx;
        voice.phase    = 0.0f;
        voice.duration = durationOf(sfx);
        voice.volume   = std::clamp(volume, 0.0f, 1.5f);
        voice.pan      = std::clamp(pan, -1.0f, 1.0f);
        m_rng = m_rng * 1664525u + 1013904223u;
        voice.seed = static_cast<float>(m_rng & 0xFFFFFF) / 16777215.0f;
        break;
    }
    SDL_UnlockAudioDevice(m_device);
}

void AudioEngine::playAt(Sfx sfx, const glm::vec3& source, const glm::vec3& listener,
                         const glm::vec3& listenerRight, float volume, float falloff) {
    const glm::vec3 delta = source - listener;
    const float distance = glm::length(delta);
    if (distance > falloff) return;

    // Inverse-ish falloff: audible far away but drops off fast up close.
    const float attenuation = 1.0f - (distance / falloff);
    const float gain = volume * attenuation * attenuation;

    float pan = 0.0f;
    if (distance > 0.01f) pan = glm::dot(delta / distance, glm::normalize(listenerRight));

    play(sfx, gain, pan);
}

}  // namespace vajra
