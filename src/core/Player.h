#pragma once

#include "Card.h"
#include "Deck.h"
#include <vector>

// Tracks all player state: health, mana, hand, and deck.
class Player {
public:
    explicit Player(int maxHealth = 100);

    // --- Health ---
    int  getHealth()    const;
    int  getMaxHealth() const;
    void takeDamage(int amount);  // Clamps to 0
    void heal(int amount);        // Clamps to maxHealth
    bool isDead()       const;

    // --- Mana ---
    int  getMana()            const;
    void setMana(int amount);
    void addMana(int amount);

    // --- Hand / Deck ---
    // Reference to the cards currently in hand
    std::vector<Card>& getHand();
    const std::vector<Card>& getHand() const;

    // Add a card directly to the hand (called by GameState after drawing)
    void drawCard(const Card& card);

    // Remove the card at handIndex from hand and return it (to play or discard)
    Card playCard(int handIndex);

    // Access the player's deck for drawing
    Deck& getDeck();

private:
    int               m_health;
    int               m_maxHealth;
    int               m_mana;
    std::vector<Card> m_hand;
    Deck              m_deck;
};
