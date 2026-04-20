#include "Player.h"

#include <algorithm>

Player::Player(int maxHealth, int maxMana)
    : m_health(maxHealth)
    , m_maxHealth(maxHealth)
    , m_maxMana(maxMana)
    , m_currentMana(maxMana)
{}

// --- Health ---

int  Player::getHealth()    const { return m_health;    }
int  Player::getMaxHealth() const { return m_maxHealth; }
bool Player::isDead()       const { return m_health <= 0; }

int Player::takeDamage(int amount) {
    int overflow = reduceBlock(amount);
    m_health = std::max(0, m_health - overflow);
    return overflow;
}

int Player::heal(int amount) {
    const int previousHealth = m_health;
    m_health = std::min(m_maxHealth, m_health + amount);
    return m_health - previousHealth;
}

// --- Block ---

int Player::getBlock() const { return m_block; }

void Player::addBlock(int amount) {
    m_block += amount;
}

void Player::clearBlock() {
    m_block = 0;
}

int Player::reduceBlock(int damage) {
    if (m_block >= damage) {
        m_block -= damage;
        return 0;
    }
    int overflow = damage - m_block;
    m_block = 0;
    return overflow;
}

// --- Mana ---

int  Player::getMana()    const { return m_currentMana; }
int  Player::getMaxMana() const { return m_maxMana;     }

void Player::startTurn() {
    m_currentMana = m_maxMana + m_statuses.consume(StatusType::BonusManaNextTurn);
}

void Player::gainMana(int amount) {
    if (amount <= 0) {
        return;
    }
    m_currentMana += amount;
}

bool Player::useMana(int amount) {
    if (m_currentMana < amount) return false;
    m_currentMana -= amount;
    return true;
}

// --- Hand / Deck ---

std::vector<Card>&       Player::getHand()       { return m_hand; }
const std::vector<Card>& Player::getHand() const  { return m_hand; }

bool Player::drawCardFromDeck() {
    if (m_deck.isEmpty()) {
        if (m_deck.getDiscard().empty()) return false;
        m_deck.reshuffleDiscard();
    }
    auto drawnCard = m_deck.draw();
    if (drawnCard.has_value()) {
        m_hand.push_back(*drawnCard);
        return true;
    }
    return false;
}

void Player::drawCard(const Card& card) {
    m_hand.push_back(card);
}

std::optional<Card> Player::playCard(int handIndex) {
    if (handIndex < 0 || handIndex >= static_cast<int>(m_hand.size()))
        return std::nullopt;
    if (!useMana(m_hand[handIndex].getCost())) return std::nullopt;
    Card played = m_hand[handIndex];
    m_hand.erase(m_hand.begin() + handIndex);
    m_deck.discard(played);
    return played;
}

void Player::discardHand() {
    for (const auto& card : m_hand)
        m_deck.discard(card);
    m_hand.clear();
}

bool Player::canDrawCard() const {
    return !m_deck.isEmpty() || !m_deck.getDiscard().empty();
}

void Player::addCardToDeck(const Card& card) {
    m_ownedCards.push_back(card);
}

void Player::rebuildCombatDeck() {
    m_block = 0;
    m_currentMana = m_maxMana;
    m_statuses = StatusCollection{};
    m_hand.clear();
    m_deck = Deck{};

    for (const auto& card : m_ownedCards) {
        m_deck.addCard(card);
    }

    m_deck.shuffle();
}

void Player::moveCardInHand(int fromIndex, int toIndex) {
    if (fromIndex < 0 || toIndex < 0) {
        return;
    }
    if (fromIndex >= static_cast<int>(m_hand.size()) || toIndex >= static_cast<int>(m_hand.size())) {
        return;
    }
    if (fromIndex == toIndex) {
        return;
    }

    Card movedCard = m_hand[fromIndex];
    m_hand.erase(m_hand.begin() + fromIndex);
    m_hand.insert(m_hand.begin() + toIndex, movedCard);
}

void Player::addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition) {
    m_statuses.add(type, magnitude, duration, disposition);
}

int Player::clearNegativeStatuses() {
    return m_statuses.clearNegative();
}

int Player::getStatusMagnitude(StatusType type) const {
    return m_statuses.getMagnitude(type);
}

Deck&       Player::getDeck()       { return m_deck; }
const Deck& Player::getDeck() const { return m_deck; }
