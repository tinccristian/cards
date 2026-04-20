#include "CardAudio.h"

#include "config/Defines.h"
#include "raylib.h"

#include <algorithm>
#include <array>
#include <vector>

namespace {
float clampPercent(float value) {
    return std::clamp(value, 0.0f, 100.0f);
}

constexpr std::array<float, 6> HoverPitchCycle = {
    0.943874f, // -1 semitone
    1.000000f, // root
    1.059463f, // +1 semitone
    1.122462f, // +2 semitones
    1.059463f,
    1.000000f
};

Sound g_shuffleSound = {};
Wave g_hoverWave = {};
std::vector<Sound> g_hoverVoices;
} // namespace

bool CardAudio::initialize(std::string& warning) {
    warning.clear();

    if (FileExists(AudioPaths::CARD_HOVER_SOUND)) {
        g_hoverWave = LoadWave(AudioPaths::CARD_HOVER_SOUND);
        if (g_hoverWave.frameCount > 0) {
            g_hoverVoices.clear();
            g_hoverVoices.reserve(AudioConfig::CardHoverVoiceCount);
            for (int voiceIndex = 0; voiceIndex < AudioConfig::CardHoverVoiceCount; ++voiceIndex) {
                g_hoverVoices.push_back(LoadSoundFromWave(g_hoverWave));
            }
            m_hoverLoaded = !g_hoverVoices.empty();
        } else {
            warning += std::string("Could not decode hover sound: ") + AudioPaths::CARD_HOVER_SOUND;
        }
    } else {
        warning += std::string("Missing hover sound: ") + AudioPaths::CARD_HOVER_SOUND;
    }

    if (FileExists(AudioPaths::CARD_SHUFFLE_SOUND)) {
        g_shuffleSound = LoadSound(AudioPaths::CARD_SHUFFLE_SOUND);
        m_shuffleLoaded = true;
    } else {
        if (!warning.empty()) {
            warning += " | ";
        }
        warning += std::string("Missing shuffle sound: ") + AudioPaths::CARD_SHUFFLE_SOUND;
    }

    applyConfiguredVolumes();
    return m_hoverLoaded && m_shuffleLoaded;
}

void CardAudio::shutdown() {
    if (m_hoverLoaded) {
        for (Sound& hoverVoice : g_hoverVoices) {
            UnloadSound(hoverVoice);
        }
        g_hoverVoices.clear();
        UnloadWave(g_hoverWave);
        g_hoverWave = {};
        m_hoverLoaded = false;
    }

    if (m_shuffleLoaded) {
        UnloadSound(g_shuffleSound);
        g_shuffleSound = {};
        m_shuffleLoaded = false;
    }

    m_hoverPitchIndex = 0;
    m_hoverVoiceIndex = 0;
}

void CardAudio::playHover() {
    if (!m_hoverLoaded) {
        return;
    }

    applyConfiguredVolumes();
    Sound& hoverVoice = g_hoverVoices[static_cast<std::size_t>(m_hoverVoiceIndex)];
    SetSoundPitch(hoverVoice, HoverPitchCycle[static_cast<std::size_t>(m_hoverPitchIndex)]);
    m_hoverPitchIndex = (m_hoverPitchIndex + 1) % static_cast<int>(HoverPitchCycle.size());
    m_hoverVoiceIndex = (m_hoverVoiceIndex + 1) % static_cast<int>(g_hoverVoices.size());
    PlaySound(hoverVoice);
}

void CardAudio::playShuffle() {
    if (!m_shuffleLoaded) {
        return;
    }

    applyConfiguredVolumes();
    PlaySound(g_shuffleSound);
}

void CardAudio::applyConfiguredVolumes() const {
    if (m_hoverLoaded) {
        const float hoverVolume = clampPercent(AudioConfig::CardHoverVolume) / 100.0f;
        for (Sound& hoverVoice : g_hoverVoices) {
            SetSoundVolume(hoverVoice, hoverVolume);
        }
    }

    if (m_shuffleLoaded) {
        SetSoundVolume(g_shuffleSound, clampPercent(AudioConfig::CardShuffleVolume) / 100.0f);
    }
}
