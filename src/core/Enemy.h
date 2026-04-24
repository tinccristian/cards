#pragma once

#include "Card.h"
#include "Deck.h"
#include "content/EnemySpriteConfig.h"
#include "gameplay/StatusCollection.h"
#include <unordered_map>
#include <string>
#include <vector>

class Player; // forward declaration

enum class EnemyIntent {
    ATTACK,
    DEFEND,
    MIXED,
    IDLE
};

struct EnemyTurnResult {
    bool skipped      = false;
    int  damageAttempted = 0;
    int  damageDealt  = 0;
    int  turnStartDamageTaken = 0;
    int  hitCount = 0;
    int  blockGained  = 0;
    int  maxHealthGained = 0;
    std::string actionText;
};

enum class IntentIconType {
    Attack,
    Block,
    Buff,
    Debuff
};

struct IntentIndicator {
    IntentIconType type = IntentIconType::Attack;
    std::string    label;
    std::string    tooltip;
};

// Represents a pathogen or health threat the player fights in combat.
class Enemy {
public:
    Enemy(std::string name, int health, Deck deck,
          EnemySpriteConfig spriteConfig = {},
          int goldReward = 0);

    // --- Basic stats ---
    const std::string& getName()      const;
    int                getHealth()    const;
    int                getMaxHealth() const;
    int                getGoldReward() const;
    bool               isDead()       const;

    // Reduce health by amount. Enemy block absorbs damage first.
    int takeDamage(int amount);

    // --- Block ---
    int  getEnemyBlock() const;
    void addBlock(int amount);
    void clearBlock();

    // --- Intent system ---
    EnemyIntent getIntent()       const;
    int         getIntentDamage() const; // Total attack damage planned
    int         getIntentBlock()  const; // Total block planned
    std::string getIntentDescription() const;
    const std::vector<IntentIndicator>& getIntentIndicators() const;

    // Draw the configured number of cards from the enemy deck and set the intent for this turn.
    void decideIntent();

    // Execute the declared intent against the player; returns damage dealt.
    EnemyTurnResult executeTurn(Player& player);

    // Cards the enemy has drawn for this turn's intent
    const std::vector<Card>& getPlayedCards() const;

    // Add a card directly to the enemy's deck.
    void addCardToDeck(const Card& card);

    // Sprite sheet configuration for rendering (empty → no sprite).
    const EnemySpriteConfig& getSpriteConfig() const;

    void addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition);
    void queueSkipTurn(int magnitude, int duration);
    int  getStatusMagnitude(StatusType type) const;
    int  getCounterValue(const std::string& key) const;

private:
    void discardPlannedCards();
    void resetIntent();
    int  applyMaxHealthGrowthPercent(int percent, bool trackCounter, const std::string& counterKey);

    std::string       m_name;
    int               m_health;
    int               m_maxHealth;
    int               m_goldReward = 0;
    int               m_enemyBlock  = 0;
    EnemyIntent       m_intent      = EnemyIntent::IDLE;
    int               m_intentDamage = 0;
    int               m_intentBlock  = 0;
    std::vector<IntentIndicator> m_intentIndicators;
    StatusCollection  m_statuses;
    EnemySpriteConfig m_spriteConfig;
    Deck              m_enemyDeck;
    std::vector<Card> m_playedCards;
    // Generic enemy-side counters let content scale future moves without
    // adding one-off member variables for every new mechanic.
    std::unordered_map<std::string, int> m_counters;
};
