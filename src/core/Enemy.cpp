#include "Enemy.h"

#include <algorithm>
#include <utility>

Enemy::Enemy(std::string name, int health, int attack)
    : m_name(std::move(name))
    , m_health(health)
    , m_maxHealth(health)
    , m_attack(attack)
{}

const std::string& Enemy::getName()      const { return m_name;      }
int                Enemy::getHealth()    const { return m_health;    }
int                Enemy::getMaxHealth() const { return m_maxHealth; }
int                Enemy::getAttack()    const { return m_attack;    }
bool               Enemy::isDead()       const { return m_health <= 0; }

void Enemy::takeDamage(int amount) {
    m_health = std::max(0, m_health - amount);
}
