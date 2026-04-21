#pragma once

#include "Card.h"
#include "Deck.h"
#include "content/EnemySpriteConfig.h"
#include "gameplay/StatusCollection.h"
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
    int  damageDealt  = 0;
    int  blockGained  = 0;
};

// Represents a pathogen or health threat the player fights in combat.
class Enemy {
public:
    Enemy(std::string name, int health, Deck deck,
          EnemySpriteConfig spriteConfig = {});

    // --- Basic stats ---
    const std::string& getName()      const;
    int                getHealth()    const;
    int                getMaxHealth() const;
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

private:
    void discardPlannedCards();
    void resetIntent();

    std::string       m_name;
    int               m_health;
    int               m_maxHealth;
    int               m_enemyBlock  = 0;
    EnemyIntent       m_intent      = EnemyIntent::IDLE;
    int               m_intentDamage = 0;
    int               m_intentBlock  = 0;
    StatusCollection  m_statuses;
    EnemySpriteConfig m_spriteConfig;
    Deck              m_enemyDeck;
    std::vector<Card> m_playedCards;
};
