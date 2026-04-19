#pragma once

#include "Card.h"
#include "Deck.h"
#include <string>
#include <vector>

class Player; // forward declaration

enum class EnemyIntent {
    ATTACK,
    DEFEND,
    MIXED,
    DEBUFF
};

// Represents a pathogen or health threat the player fights in combat.
class Enemy {
public:
    Enemy(std::string name, int health, int attack, Deck deck);

    // --- Basic stats ---
    const std::string& getName()      const;
    int                getHealth()    const;
    int                getMaxHealth() const;
    int                getAttack()    const;
    bool               isDead()       const;

    // Reduce health by amount. Enemy block absorbs damage first.
    void takeDamage(int amount);

    // --- Block ---
    int  getEnemyBlock() const;
    void addEnemyBlock(int amount);

    // --- Intent system ---
    EnemyIntent getIntent()       const;
    int         getIntentDamage() const; // Total attack damage planned
    int         getIntentBlock()  const; // Total block planned
    std::string getIntentDescription() const;

    // Draw 1-2 cards from the enemy deck and set the intent for this turn.
    void decideIntent();

    // Execute the declared intent against the player; returns damage dealt.
    int executeIntent(Player& player);

    // Cards the enemy has drawn for this turn's intent
    const std::vector<Card>& getPlayedCards() const;

    // Add a card directly to the enemy's deck.
    void addCardToDeck(const Card& card);

private:
    std::string       m_name;
    int               m_health;
    int               m_maxHealth;
    int               m_attack;
    int               m_enemyBlock  = 0;
    EnemyIntent       m_intent      = EnemyIntent::ATTACK;
    int               m_intentDamage = 0;
    int               m_intentBlock  = 0;
    Deck              m_enemyDeck;
    std::vector<Card> m_playedCards;

    static int randomInt(int lo, int hi);
};
