#pragma once

#include <string>
#include <vector>

class CardAudio {
public:
    CardAudio() = default;

    bool initialize(std::string& warning);
    void shutdown();
    void update(float dt);

    void playHover();
    void playShuffle();
    void playCardPicked();
    void playCardEffect(const std::string& path);
    void playRewardEnter();
    void playArmor();
    void playArmorHit();
    void playDamage();
    void playCoinPicked();
    void playEnemyBuff();
    void playEnemyHurt();
    void playEnemyDeath();
    void playPlayerDeath();
    void playGameOver();
    void playNextTurn();
    void playNoahEvent();
    void scheduleDamage(float delaySecs = 0.0f);
    void scheduleArmorHit(float delaySecs = 0.0f);
    void scheduleEnemyHurt(float delaySecs = 0.0f);

private:
    void applyConfiguredVolumes() const;

    enum class ScheduledSoundType {
        Damage,
        ArmorHit,
        EnemyHurt
    };

    struct ScheduledSound {
        ScheduledSoundType type;
        float delaySecs;
    };

    bool m_hoverLoaded   = false;
    bool m_shuffleLoaded = false;
    bool m_cardPickedLoaded = false;
    bool m_rewardEnterLoaded = false;
    bool m_armorLoaded = false;
    bool m_armorHitLoaded = false;
    bool m_damageLoaded = false;
    bool m_coinPickedLoaded = false;
    bool m_enemyBuffLoaded = false;
    bool m_enemyHurtLoaded = false;
    bool m_enemyDeathLoaded = false;
    bool m_playerDeathLoaded = false;
    bool m_gameOverLoaded = false;
    bool m_nextTurnLoaded = false;
    bool m_noahEventLoaded = false;
    int  m_hoverPitchIndex = 0;
    int  m_hoverVoiceIndex = 0;
    int  m_damageVoiceIndex = 0;
    int  m_armorHitVoiceIndex = 0;
    int  m_enemyHurtVoiceIndex = 0;
    std::vector<ScheduledSound> m_scheduledSounds;
};
