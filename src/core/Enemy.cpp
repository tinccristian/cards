#include "Enemy.h"
#include "config/Defines.h"
#include "Player.h"

#include <algorithm>
#include <utility>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Enemy::Enemy(std::string name, int health, Deck deck, EnemySpriteConfig spriteConfig, int goldReward)
    : m_name(std::move(name))
    , m_health(health)
    , m_maxHealth(health)
    , m_goldReward(goldReward)
    , m_spriteConfig(std::move(spriteConfig))
    , m_enemyDeck(std::move(deck))
{}

// ---------------------------------------------------------------------------
// Basic stats
// ---------------------------------------------------------------------------

const std::string& Enemy::getName()      const { return m_name;      }
int                Enemy::getHealth()    const { return m_health;    }
int                Enemy::getMaxHealth() const { return m_maxHealth; }
int                Enemy::getGoldReward() const { return m_goldReward; }
bool               Enemy::isDead()       const { return m_health <= 0; }

int Enemy::takeDamage(int amount) {
    // Block absorbs first
    if (m_enemyBlock >= amount) {
        m_enemyBlock -= amount;
        return 0;
    }
    amount -= m_enemyBlock;
    m_enemyBlock = 0;
    m_health = std::max(0, m_health - amount);
    return amount;
}

// ---------------------------------------------------------------------------
// Block
// ---------------------------------------------------------------------------

int  Enemy::getEnemyBlock()  const { return m_enemyBlock; }
void Enemy::addBlock(int amt)      { m_enemyBlock += amt; }
void Enemy::clearBlock()           { m_enemyBlock = 0; }

// ---------------------------------------------------------------------------
// Intent system
// ---------------------------------------------------------------------------

EnemyIntent Enemy::getIntent()        const { return m_intent;       }
int         Enemy::getIntentDamage()  const {
    return m_statuses.has(StatusType::SkipTurn) ? 0 : m_intentDamage;
}
int         Enemy::getIntentBlock()   const {
    return m_statuses.has(StatusType::SkipTurn) ? 0 : m_intentBlock;
}

const std::vector<Card>& Enemy::getPlayedCards() const { return m_playedCards; }

void Enemy::addCardToDeck(const Card& card) { m_enemyDeck.addCard(card); }

const EnemySpriteConfig& Enemy::getSpriteConfig() const { return m_spriteConfig; }

void Enemy::addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition) {
    m_statuses.add(type, magnitude, duration, disposition);
}

void Enemy::queueSkipTurn(int magnitude, int duration) {
    addStatus(StatusType::SkipTurn, magnitude, duration, StatusDisposition::Negative);
    discardPlannedCards();
    resetIntent();
}

int Enemy::getStatusMagnitude(StatusType type) const {
    return m_statuses.getMagnitude(type);
}

void Enemy::discardPlannedCards() {
    for (const auto& card : m_playedCards) {
        m_enemyDeck.discard(card);
    }
    m_playedCards.clear();
}

void Enemy::resetIntent() {
    m_intent = EnemyIntent::IDLE;
    m_intentDamage = 0;
    m_intentBlock = 0;
}

std::string Enemy::getIntentDescription() const {
    if (m_statuses.has(StatusType::SkipTurn)) {
        return "Unable to act this turn";
    }
    if (m_intentDamage > 0 && m_intentBlock > 0) {
        return "Intends to attack for " + std::to_string(m_intentDamage)
             + " and block " + std::to_string(m_intentBlock);
    }
    if (m_intentDamage > 0) {
        return "Intends to attack for " + std::to_string(m_intentDamage) + " damage";
    }
    if (m_intentBlock > 0) {
        return "Intends to block " + std::to_string(m_intentBlock);
    }
    return "Repositioning...";
}

void Enemy::decideIntent() {
    discardPlannedCards();
    resetIntent();

    // Draw the configured number of cards each turn.
    for (int i = 0; i < CombatConfig::EnemyCardsPerTurn; ++i) {
        if (m_enemyDeck.isEmpty()) m_enemyDeck.reshuffleDiscard();
        auto nextCard = m_enemyDeck.draw();
        if (nextCard.has_value()) {
            m_playedCards.push_back(*nextCard);
        }
    }

    // Calculate totals from planned cards
    bool hasAttack = false, hasDefend = false;
    for (const auto& c : m_playedCards) {
        if (c.getDamageAmount() > 0) {
            hasAttack = true;
            m_intentDamage += c.getDamageAmount();
        }
        if (c.getBlockAmount() > 0) {
            hasDefend = true;
            m_intentBlock += c.getBlockAmount();
        }
    }

    if      (hasAttack && !hasDefend) m_intent = EnemyIntent::ATTACK;
    else if (!hasAttack && hasDefend) m_intent = EnemyIntent::DEFEND;
    else if (hasAttack && hasDefend)  m_intent = EnemyIntent::MIXED;
    else                              m_intent = EnemyIntent::IDLE;
}

EnemyTurnResult Enemy::executeTurn(Player& player) {
    if (m_statuses.consume(StatusType::SkipTurn) > 0) {
        discardPlannedCards();
        resetIntent();
        return EnemyTurnResult{ true, 0, 0 };
    }

    EnemyTurnResult result;

    // Apply attack damage to player
    if (m_intentDamage > 0) {
        result.damageDealt = player.takeDamage(m_intentDamage);
    }

    // Apply block to self
    if (m_intentBlock > 0) {
        addBlock(m_intentBlock);
        result.blockGained = m_intentBlock;
    }

    discardPlannedCards();
    resetIntent();

    return result;
}
