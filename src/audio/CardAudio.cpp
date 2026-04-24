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
Sound g_cardPickedSound = {};
Sound g_rewardEnterSound = {};
Sound g_armorSound = {};
Sound g_coinPickedSound = {};
Sound g_enemyBuffSound = {};
Sound g_enemyDeathSound = {};
Sound g_playerDeathSound = {};
Sound g_gameOverSound = {};
Sound g_nextTurnSound = {};
Wave g_hoverWave = {};
std::vector<Sound> g_hoverVoices;
Wave g_damageWave = {};
std::vector<Sound> g_damageVoices;
Wave g_armorHitWave = {};
std::vector<Sound> g_armorHitVoices;
Wave g_enemyHurtWave = {};
std::vector<Sound> g_enemyHurtVoices;

bool loadVoicePool(const char* path, int voiceCount, Wave& wave, std::vector<Sound>& voices, std::string& warning, const char* label) {
    if (!FileExists(path)) {
        if (!warning.empty()) warning += " | ";
        warning += std::string("Missing ") + label + " sound: " + path;
        return false;
    }

    wave = LoadWave(path);
    if (wave.frameCount <= 0) {
        if (!warning.empty()) warning += " | ";
        warning += std::string("Could not decode ") + label + " sound: " + path;
        return false;
    }

    voices.clear();
    voices.reserve(voiceCount);
    for (int i = 0; i < voiceCount; ++i) {
        voices.push_back(LoadSoundFromWave(wave));
    }
    return !voices.empty();
}
} // namespace

bool CardAudio::initialize(std::string& warning) {
    warning.clear();

    const auto appendWarning = [&warning](const std::string& message) {
        if (!warning.empty()) {
            warning += " | ";
        }
        warning += message;
    };

    const auto loadOneShot = [&appendWarning](const char* path, Sound& sound, bool& loaded, const char* label) {
        loaded = false;
        if (!FileExists(path)) {
            appendWarning(std::string("Missing ") + label + " sound: " + path);
            return;
        }

        sound = LoadSound(path);
        loaded = sound.frameCount > 0;
        if (!loaded) {
            appendWarning(std::string("Could not decode ") + label + " sound: " + path);
        }
    };

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
            appendWarning(std::string("Could not decode hover sound: ") + AudioPaths::CARD_HOVER_SOUND);
        }
    } else {
        appendWarning(std::string("Missing hover sound: ") + AudioPaths::CARD_HOVER_SOUND);
    }

    if (FileExists(AudioPaths::CARD_SHUFFLE_SOUND)) {
        g_shuffleSound = LoadSound(AudioPaths::CARD_SHUFFLE_SOUND);
        m_shuffleLoaded = true;
    } else {
        appendWarning(std::string("Missing shuffle sound: ") + AudioPaths::CARD_SHUFFLE_SOUND);
    }

    loadOneShot(AudioPaths::CARD_PICKED_SOUND, g_cardPickedSound, m_cardPickedLoaded, "card picked");
    loadOneShot(AudioPaths::BONUS_SOUND, g_rewardEnterSound, m_rewardEnterLoaded, "reward enter");
    loadOneShot(AudioPaths::ARMOR_SOUND, g_armorSound, m_armorLoaded, "armor");
    m_armorHitLoaded = loadVoicePool(AudioPaths::ARMOR_HIT_SOUND, 6, g_armorHitWave, g_armorHitVoices, warning, "armor hit");
    m_damageLoaded = loadVoicePool(AudioPaths::DAMAGE_SOUND, 6, g_damageWave, g_damageVoices, warning, "damage");
    loadOneShot(AudioPaths::COIN_PICKED_SOUND, g_coinPickedSound, m_coinPickedLoaded, "coin picked");
    loadOneShot(AudioPaths::ENEMY_BUFF_SOUND, g_enemyBuffSound, m_enemyBuffLoaded, "enemy buff");
    m_enemyHurtLoaded = loadVoicePool(AudioPaths::ENEMY_HURT_SOUND, 6, g_enemyHurtWave, g_enemyHurtVoices, warning, "enemy hurt");
    loadOneShot(AudioPaths::ENEMY_DEATH_SOUND, g_enemyDeathSound, m_enemyDeathLoaded, "enemy death");
    loadOneShot(AudioPaths::PLAYER_DEATH_SOUND, g_playerDeathSound, m_playerDeathLoaded, "player death");
    loadOneShot(AudioPaths::GAME_OVER_SOUND, g_gameOverSound, m_gameOverLoaded, "game over");
    loadOneShot(AudioPaths::NEXT_TURN_SOUND, g_nextTurnSound, m_nextTurnLoaded, "next turn");

    applyConfiguredVolumes();
    return m_hoverLoaded && m_shuffleLoaded && m_cardPickedLoaded && m_rewardEnterLoaded
        && m_armorLoaded && m_armorHitLoaded && m_damageLoaded
        && m_coinPickedLoaded && m_enemyBuffLoaded && m_enemyHurtLoaded
        && m_enemyDeathLoaded && m_playerDeathLoaded && m_gameOverLoaded
        && m_nextTurnLoaded;
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
    if (m_cardPickedLoaded) {
        UnloadSound(g_cardPickedSound);
        g_cardPickedSound = {};
        m_cardPickedLoaded = false;
    }
    if (m_rewardEnterLoaded) {
        UnloadSound(g_rewardEnterSound);
        g_rewardEnterSound = {};
        m_rewardEnterLoaded = false;
    }
    if (m_armorLoaded) {
        UnloadSound(g_armorSound);
        g_armorSound = {};
        m_armorLoaded = false;
    }
    if (m_armorHitLoaded) {
        for (Sound& voice : g_armorHitVoices) {
            UnloadSound(voice);
        }
        g_armorHitVoices.clear();
        UnloadWave(g_armorHitWave);
        g_armorHitWave = {};
        m_armorHitLoaded = false;
    }
    if (m_damageLoaded) {
        for (Sound& voice : g_damageVoices) {
            UnloadSound(voice);
        }
        g_damageVoices.clear();
        UnloadWave(g_damageWave);
        g_damageWave = {};
        m_damageLoaded = false;
    }
    if (m_coinPickedLoaded) {
        UnloadSound(g_coinPickedSound);
        g_coinPickedSound = {};
        m_coinPickedLoaded = false;
    }
    if (m_enemyBuffLoaded) {
        UnloadSound(g_enemyBuffSound);
        g_enemyBuffSound = {};
        m_enemyBuffLoaded = false;
    }
    if (m_enemyHurtLoaded) {
        for (Sound& voice : g_enemyHurtVoices) {
            UnloadSound(voice);
        }
        g_enemyHurtVoices.clear();
        UnloadWave(g_enemyHurtWave);
        g_enemyHurtWave = {};
        m_enemyHurtLoaded = false;
    }
    if (m_enemyDeathLoaded) {
        UnloadSound(g_enemyDeathSound);
        g_enemyDeathSound = {};
        m_enemyDeathLoaded = false;
    }
    if (m_playerDeathLoaded) {
        UnloadSound(g_playerDeathSound);
        g_playerDeathSound = {};
        m_playerDeathLoaded = false;
    }
    if (m_gameOverLoaded) {
        UnloadSound(g_gameOverSound);
        g_gameOverSound = {};
        m_gameOverLoaded = false;
    }
    if (m_nextTurnLoaded) {
        UnloadSound(g_nextTurnSound);
        g_nextTurnSound = {};
        m_nextTurnLoaded = false;
    }

    m_hoverPitchIndex = 0;
    m_hoverVoiceIndex = 0;
    m_damageVoiceIndex = 0;
    m_armorHitVoiceIndex = 0;
    m_enemyHurtVoiceIndex = 0;
    m_scheduledSounds.clear();
}

void CardAudio::update(float dt) {
    for (ScheduledSound& sound : m_scheduledSounds) {
        sound.delaySecs -= dt;
    }

    auto it = m_scheduledSounds.begin();
    while (it != m_scheduledSounds.end()) {
        if (it->delaySecs <= 0.0f) {
            switch (it->type) {
            case ScheduledSoundType::Damage:
                playDamage();
                break;
            case ScheduledSoundType::ArmorHit:
                playArmorHit();
                break;
            case ScheduledSoundType::EnemyHurt:
                playEnemyHurt();
                break;
            }
            it = m_scheduledSounds.erase(it);
        } else {
            ++it;
        }
    }
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

void CardAudio::playCardPicked() {
    if (!m_cardPickedLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_cardPickedSound);
}

void CardAudio::playRewardEnter() {
    if (!m_rewardEnterLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_rewardEnterSound);
}

void CardAudio::playArmor() {
    if (!m_armorLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_armorSound);
}

void CardAudio::playArmorHit() {
    if (!m_armorHitLoaded) {
        return;
    }
    applyConfiguredVolumes();
    Sound& voice = g_armorHitVoices[static_cast<std::size_t>(m_armorHitVoiceIndex)];
    m_armorHitVoiceIndex = (m_armorHitVoiceIndex + 1) % static_cast<int>(g_armorHitVoices.size());
    PlaySound(voice);
}

void CardAudio::playDamage() {
    if (!m_damageLoaded) {
        return;
    }
    applyConfiguredVolumes();
    Sound& voice = g_damageVoices[static_cast<std::size_t>(m_damageVoiceIndex)];
    m_damageVoiceIndex = (m_damageVoiceIndex + 1) % static_cast<int>(g_damageVoices.size());
    PlaySound(voice);
}

void CardAudio::playCoinPicked() {
    if (!m_coinPickedLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_coinPickedSound);
}

void CardAudio::playEnemyBuff() {
    if (!m_enemyBuffLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_enemyBuffSound);
}

void CardAudio::playEnemyHurt() {
    if (!m_enemyHurtLoaded) {
        return;
    }
    applyConfiguredVolumes();
    Sound& voice = g_enemyHurtVoices[static_cast<std::size_t>(m_enemyHurtVoiceIndex)];
    m_enemyHurtVoiceIndex = (m_enemyHurtVoiceIndex + 1) % static_cast<int>(g_enemyHurtVoices.size());
    PlaySound(voice);
}

void CardAudio::playEnemyDeath() {
    if (!m_enemyDeathLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_enemyDeathSound);
}

void CardAudio::playPlayerDeath() {
    if (!m_playerDeathLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_playerDeathSound);
}

void CardAudio::playGameOver() {
    if (!m_gameOverLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_gameOverSound);
}

void CardAudio::playNextTurn() {
    if (!m_nextTurnLoaded) {
        return;
    }
    applyConfiguredVolumes();
    PlaySound(g_nextTurnSound);
}

void CardAudio::scheduleDamage(float delaySecs) {
    m_scheduledSounds.push_back({ ScheduledSoundType::Damage, delaySecs });
}

void CardAudio::scheduleArmorHit(float delaySecs) {
    m_scheduledSounds.push_back({ ScheduledSoundType::ArmorHit, delaySecs });
}

void CardAudio::scheduleEnemyHurt(float delaySecs) {
    m_scheduledSounds.push_back({ ScheduledSoundType::EnemyHurt, delaySecs });
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
    if (m_cardPickedLoaded) {
        SetSoundVolume(g_cardPickedSound, clampPercent(AudioConfig::CardPickedVolume) / 100.0f);
    }
    if (m_rewardEnterLoaded) {
        SetSoundVolume(g_rewardEnterSound, clampPercent(AudioConfig::RewardEnterVolume) / 100.0f);
    }
    if (m_armorLoaded) {
        SetSoundVolume(g_armorSound, clampPercent(AudioConfig::ArmorVolume) / 100.0f);
    }
    if (m_armorHitLoaded) {
        const float volume = clampPercent(AudioConfig::ArmorHitVolume) / 100.0f;
        for (Sound& voice : g_armorHitVoices) {
            SetSoundVolume(voice, volume);
        }
    }
    if (m_damageLoaded) {
        const float volume = clampPercent(AudioConfig::DamageVolume) / 100.0f;
        for (Sound& voice : g_damageVoices) {
            SetSoundVolume(voice, volume);
        }
    }
    if (m_coinPickedLoaded) {
        SetSoundVolume(g_coinPickedSound, clampPercent(AudioConfig::CoinPickedVolume) / 100.0f);
    }
    if (m_enemyBuffLoaded) {
        SetSoundVolume(g_enemyBuffSound, clampPercent(AudioConfig::EnemyBuffVolume) / 100.0f);
    }
    if (m_enemyHurtLoaded) {
        const float volume = clampPercent(AudioConfig::EnemyHurtVolume) / 100.0f;
        for (Sound& voice : g_enemyHurtVoices) {
            SetSoundVolume(voice, volume);
        }
    }
    if (m_enemyDeathLoaded) {
        SetSoundVolume(g_enemyDeathSound, clampPercent(AudioConfig::EnemyDeathVolume) / 100.0f);
    }
    if (m_playerDeathLoaded) {
        SetSoundVolume(g_playerDeathSound, clampPercent(AudioConfig::PlayerDeathVolume) / 100.0f);
    }
    if (m_gameOverLoaded) {
        SetSoundVolume(g_gameOverSound, clampPercent(AudioConfig::GameOverVolume) / 100.0f);
    }
    if (m_nextTurnLoaded) {
        SetSoundVolume(g_nextTurnSound, clampPercent(AudioConfig::NextTurnVolume) / 100.0f);
    }
}
