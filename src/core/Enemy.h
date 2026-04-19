#pragma once

#include <string>

// Represents a pathogen or health threat the player fights in combat.
class Enemy {
public:
    Enemy(std::string name, int health, int attack);

    const std::string& getName()      const;
    int                getHealth()    const;
    int                getMaxHealth() const;
    int                getAttack()    const;
    bool               isDead()       const;

    // Reduce health by amount, clamped to 0
    void takeDamage(int amount);

private:
    std::string m_name;
    int         m_health;
    int         m_maxHealth;
    int         m_attack;
};
