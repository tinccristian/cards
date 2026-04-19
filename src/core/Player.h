#pragma once

#include "Card.h"
#include "Deck.h"
#include <optional>
#include <vector>

// Tracks all player state: health, mana, block, hand, and deck.
class Player {
public:
    explicit Player(int maxHealth = 100, int maxMana = 3);

    // --- Health ---
    int  getHealth()    const;
    int  getMaxHealth() const;
    bool isDead()       const;

    // Block absorbs damage first; overflow reduces health.
    void takeDamage(int amount);
    void heal(int amount); // Clamps to maxHealth

    // --- Block (persists across turns until consumed by damage) ---
    int  getBlock()         const;
    void addBlock(int amount);
    // Absorbs 'damage' with block. Returns any overflow not covered by block.
    int  reduceBlock(int damage);

    // --- Mana ---
    int  getMana()    const;
    int  getMaxMana() const;
    void resetMana(); // Set currentMana = maxMana
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

    Deck& getDeck();
    const Deck& getDeck() const;

private:
    int               m_health;
    int               m_maxHealth;
    int               m_block = 0;   // persistent until consumed
    int               m_maxMana;
    int               m_currentMana;
    std::vector<Card> m_hand;
    Deck              m_deck;
};
