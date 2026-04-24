#pragma once

#include <string>

class CardAudio {
public:
    CardAudio() = default;

    bool initialize(std::string& warning);
    void shutdown();

    void playHover();
    void playShuffle();
    void playCardPicked();
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

private:
    void applyConfiguredVolumes() const;

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
    int  m_hoverPitchIndex = 0;
    int  m_hoverVoiceIndex = 0;
};
