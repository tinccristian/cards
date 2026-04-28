#include "Player.h"

#include "gameplay/CardEffect.h"
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
    return takeDamageDetailed(amount).health;
}

DamageBreakdown Player::takeDamageDetailed(int amount) {
    DamageBreakdown result;
    result.blocked = std::min(std::max(0, amount), m_block);
    int overflow = reduceBlock(amount);
    result.health = std::max(0, std::min(m_health, overflow));
    m_health = std::max(0, m_health - overflow);
    return result;
}

int Player::loseHealth(int amount) {
    const int applied = std::max(0, std::min(m_health, amount));
    m_health -= applied;
    return applied;
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
int  Player::getGold()    const { return m_gold;        }

PlayerTurnStartResult Player::startTurn() {
    PlayerTurnStartResult result;
    m_currentMana = m_maxMana + m_statuses.consume(StatusType::BonusManaNextTurn);
    m_statuses.remove(StatusType::DamageOnDraw);
    for (const Card& organ : m_activeOrgans) {
        for (const CardEffect& effect : organ.getEffects()) {
            if (effect.type == EffectType::HealOnTurnStart) {
                result.organHealingDone += heal(effect.amount);
            }
        }
    }
    return result;
}

DamageBreakdown Player::tickPoison() {
    const int poisonDamage = m_statuses.tick(StatusType::Poison);
    if (poisonDamage > 0) {
        return takeDamageDetailed(poisonDamage);
    }
    return {};
}

void Player::gainMana(int amount) {
    if (amount <= 0) {
        return;
    }
    m_currentMana += amount;
}

void Player::addGold(int amount) {
    if (amount <= 0) {
        return;
    }
    m_gold += amount;
}

bool Player::spendGold(int amount) {
    if (amount <= 0 || m_gold < amount) {
        return false;
    }
    m_gold -= amount;
    return true;
}

bool Player::useMana(int amount) {
    if (m_currentMana < amount) return false;
    m_currentMana -= amount;
    return true;
}

int Player::getEffectiveCost(const Card& card) const {
    if (card.isFreeThisTurn()) {
        return 0;
    }
    if (m_statuses.has(StatusType::NextCardFree)) {
        return 0;
    }
    return card.getCost();
}

// --- Hand / Deck ---

std::vector<Card>&       Player::getHand()       { return m_hand; }
const std::vector<Card>& Player::getHand() const  { return m_hand; }

bool Player::drawCardRaw() {
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

bool Player::drawCardFromDeck() {
    return drawCardRaw();
}

void Player::drawCard(const Card& card) {
    m_hand.push_back(card);
}

void Player::drawCardFreeThisTurn(const Card& card) {
    Card freeCard = card;
    freeCard.setFreeThisTurn(true);
    m_hand.push_back(freeCard);
}

std::optional<Card> Player::playCard(int handIndex) {
    if (handIndex < 0 || handIndex >= static_cast<int>(m_hand.size()))
        return std::nullopt;
    const bool cardIsFreeThisTurn = m_hand[handIndex].isFreeThisTurn();
    const bool hadNextCardFree = !cardIsFreeThisTurn && m_statuses.has(StatusType::NextCardFree);
    const int effectiveCost = getEffectiveCost(m_hand[handIndex]);
    if (!useMana(effectiveCost)) return std::nullopt;
    if (hadNextCardFree) {
        m_statuses.remove(StatusType::NextCardFree);
    }
    Card played = m_hand[handIndex];
    played.setFreeThisTurn(false);
    m_hand.erase(m_hand.begin() + handIndex);
    if (played.getType() == CardType::Organ) {
        m_activeOrgans.push_back(played);
    }
    // Non-organ cards are discarded by the caller AFTER effects resolve,
    // so that a card which draws cannot reshuffle itself back into the deck.
    return played;
}

void Player::discardHand() {
    for (auto card : m_hand) {
        card.setFreeThisTurn(false);
        m_deck.discard(card);
    }
    m_hand.clear();
}

bool Player::canDrawCard() const {
    return !m_deck.isEmpty() || !m_deck.getDiscard().empty();
}

void Player::addCardToDeck(const Card& card) {
    m_ownedCards.push_back(card);
}

bool Player::removeOwnedCardAt(int index) {
    if (index < 0 || index >= static_cast<int>(m_ownedCards.size())) {
        return false;
    }
    m_ownedCards.erase(m_ownedCards.begin() + index);
    return true;
}

bool Player::replaceOwnedCards(const std::vector<int>& indices, const std::vector<Card>& replacements) {
    if (indices.empty() || indices.size() != replacements.size()) {
        return false;
    }

    std::vector<int> sortedIndices = indices;
    std::sort(sortedIndices.begin(), sortedIndices.end());
    for (int index : sortedIndices) {
        if (index < 0 || index >= static_cast<int>(m_ownedCards.size())) {
            return false;
        }
    }

    for (std::size_t i = 0; i < sortedIndices.size(); ++i) {
        m_ownedCards[sortedIndices[i]] = replacements[i];
    }
    return true;
}

void Player::rebuildCombatDeck() {
    m_block = 0;
    m_currentMana = m_maxMana;
    m_statuses = StatusCollection{};
    m_hand.clear();
    m_deck = Deck{};
    m_activeOrgans.clear();

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

const std::vector<Card>& Player::getOwnedCards() const {
    return m_ownedCards;
}

void Player::addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition) {
    m_statuses.add(type, magnitude, duration, disposition);
}

void Player::removeStatus(StatusType type) {
    m_statuses.remove(type);
}

int Player::clearNegativeStatuses() {
    return m_statuses.clearNegative();
}

int Player::getStatusMagnitude(StatusType type) const {
    return m_statuses.getMagnitude(type);
}

const StatusCollection& Player::getStatuses() const {
    return m_statuses;
}

Deck&       Player::getDeck()       { return m_deck; }
const Deck& Player::getDeck() const { return m_deck; }

// --- Organ system ---

void Player::activateOrgan(const Card& card) {
    m_activeOrgans.push_back(card);
}

bool Player::hasOrgan(const std::string& id) const {
    for (const Card& organ : m_activeOrgans) {
        if (organ.getId() == id) return true;
    }
    return false;
}

void Player::clearOrgans() {
    m_activeOrgans.clear();
}

const std::vector<Card>& Player::getActiveOrgans() const {
    return m_activeOrgans;
}
