#include "Enemy.h"
#include "Player.h"

#include <algorithm>
#include <random>
#include <utility>

// ---------------------------------------------------------------------------
// RNG
// ---------------------------------------------------------------------------

int Enemy::randomInt(int lo, int hi) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Enemy::Enemy(std::string name, int health, int attack, Deck deck)
    : m_name(std::move(name))
    , m_health(health)
    , m_maxHealth(health)
    , m_attack(attack)
    , m_enemyDeck(std::move(deck))
{}

// ---------------------------------------------------------------------------
// Basic stats
// ---------------------------------------------------------------------------

const std::string& Enemy::getName()      const { return m_name;      }
int                Enemy::getHealth()    const { return m_health;    }
int                Enemy::getMaxHealth() const { return m_maxHealth; }
int                Enemy::getAttack()    const { return m_attack;    }
bool               Enemy::isDead()       const { return m_health <= 0; }

void Enemy::takeDamage(int amount) {
    // Block absorbs first
    if (m_enemyBlock >= amount) {
        m_enemyBlock -= amount;
        return;
    }
    amount -= m_enemyBlock;
    m_enemyBlock = 0;
    m_health = std::max(0, m_health - amount);
}

// ---------------------------------------------------------------------------
// Block
// ---------------------------------------------------------------------------

int  Enemy::getEnemyBlock()        const { return m_enemyBlock; }
void Enemy::addEnemyBlock(int amt)       { m_enemyBlock += amt; }

// ---------------------------------------------------------------------------
// Intent system
// ---------------------------------------------------------------------------

EnemyIntent Enemy::getIntent()        const { return m_intent;       }
int         Enemy::getIntentDamage()  const { return m_intentDamage; }
int         Enemy::getIntentBlock()   const { return m_intentBlock;  }

const std::vector<Card>& Enemy::getPlayedCards() const { return m_playedCards; }

std::string Enemy::getIntentDescription() const {
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
    return "Watching you...";
}

void Enemy::decideIntent() {
    // Do NOT reset m_enemyBlock here — block persists until consumed by damage.
    m_intentDamage = 0;
    m_intentBlock  = 0;

    // Draw 1 or 2 cards (equal chance)
    int numCards = randomInt(1, 2);
    for (int i = 0; i < numCards; ++i) {
        if (m_enemyDeck.isEmpty()) m_enemyDeck.reshuffleDiscard();
        if (!m_enemyDeck.isEmpty()) {
            m_playedCards.push_back(m_enemyDeck.draw());
        }
    }

    // Calculate totals from planned cards
    bool hasAttack = false, hasDefend = false;
    for (const auto& c : m_playedCards) {
        if (c.getType() == "attack") {
            hasAttack = true;
            m_intentDamage += c.getPower();
        } else {
            hasDefend = true;
            m_intentBlock += c.getBlockAmount();
        }
    }

    if      (hasAttack && !hasDefend) m_intent = EnemyIntent::ATTACK;
    else if (!hasAttack && hasDefend) m_intent = EnemyIntent::DEFEND;
    else if (hasAttack && hasDefend)  m_intent = EnemyIntent::MIXED;
    else                              m_intent = EnemyIntent::DEFEND;
}

int Enemy::executeIntent(Player& player) {
    // Apply attack damage to player
    if (m_intentDamage > 0) player.takeDamage(m_intentDamage);

    // Apply block to self
    if (m_intentBlock > 0) m_enemyBlock += m_intentBlock;

    // Send played cards to discard
    for (const auto& c : m_playedCards) m_enemyDeck.discard(c);
    m_playedCards.clear();

    return m_intentDamage;
}
