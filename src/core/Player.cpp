#include "Player.h"

#include <algorithm>
#include <cassert>

Player::Player(int maxHealth)
    : m_health(maxHealth)
    , m_maxHealth(maxHealth)
    , m_mana(3)
{}

int  Player::getHealth()    const { return m_health;    }
int  Player::getMaxHealth() const { return m_maxHealth; }
bool Player::isDead()       const { return m_health <= 0; }

void Player::takeDamage(int amount) {
    m_health = std::max(0, m_health - amount);
}

void Player::heal(int amount) {
    m_health = std::min(m_maxHealth, m_health + amount);
}

int  Player::getMana()            const { return m_mana; }
void Player::setMana(int amount)        { m_mana = amount; }
void Player::addMana(int amount)        { m_mana += amount; }

std::vector<Card>& Player::getHand()             { return m_hand; }
const std::vector<Card>& Player::getHand() const  { return m_hand; }

void Player::drawCard(const Card& card) {
    m_hand.push_back(card);
}

Card Player::playCard(int handIndex) {
    assert(handIndex >= 0 && handIndex < static_cast<int>(m_hand.size())
           && "handIndex out of range");
    Card played = m_hand[handIndex];
    m_hand.erase(m_hand.begin() + handIndex);
    return played;
}

Deck& Player::getDeck() { return m_deck; }
