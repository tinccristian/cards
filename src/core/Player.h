#pragma once

#include "config/Defines.h"
#include "Card.h"
#include "Deck.h"
#include "gameplay/StatusCollection.h"
#include <optional>
#include <vector>

// Tracks all player state: health, mana, block, hand, and deck.
class Player {
public:
    explicit Player(int maxHealth = CombatConfig::PlayerMaxHealth,
                    int maxMana = CombatConfig::PlayerBaseMana);

    // --- Health ---
    int  getHealth()    const;
    int  getMaxHealth() const;
    bool isDead()       const;

    // Block absorbs damage first; overflow reduces health.
    int  takeDamage(int amount);
    int  heal(int amount); // Clamps to maxHealth, returns HP restored

    // --- Block (expires after the opposing side finishes its turn) ---
    int  getBlock()         const;
    void addBlock(int amount);
    void clearBlock();
    // Absorbs 'damage' with block. Returns any overflow not covered by block.
    int  reduceBlock(int damage);

    // --- Mana ---
    int  getMana()    const;
    int  getMaxMana() const;
    void startTurn();
    void gainMana(int amount);
    // Deduct 'amount' mana. Returns false (no change) if insufficient.
    bool useMana(int amount);

    // --- Hand / Deck ---
    std::vector<Card>&       getHand();
    const std::vector<Card>& getHand() const;

    // Draw one card from deck to hand. Reshuffles discard into deck if needed.
    // Returns false if both deck and discard are empty.
    bool drawCardFromDeck();

    // Add a card directly to hand (used when constructing the opening hand).
    void drawCard(const Card& card);

    // Attempt to play card at handIndex.
    // Deducts mana equal to card cost; removes card from hand and discards it.
    // Returns the played card, or nullopt if mana is insufficient.
    std::optional<Card> playCard(int handIndex);

    // Move all cards in hand to the discard pile, then clear hand.
    void discardHand();

    // Returns true if a card can be drawn (deck or discard non-empty).
    bool canDrawCard() const;

    // Add a card directly to the deck (used when building the starter deck).
    void addCardToDeck(const Card& card);
    void moveCardInHand(int fromIndex, int toIndex);

    void addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition);
    int  clearNegativeStatuses();
    int  getStatusMagnitude(StatusType type) const;

    Deck& getDeck();
    const Deck& getDeck() const;

private:
    int               m_health;
    int               m_maxHealth;
    int               m_block = 0;
    int               m_maxMana;
    int               m_currentMana;
    StatusCollection  m_statuses;
    std::vector<Card> m_hand;
    Deck              m_deck;
};
