#pragma once

#include "Card.h"
#include <vector>

// Manages a collection of cards split into draw pile, hand, and discard pile.
class Deck {
public:
    Deck();

    // Add a card to the draw pile
    void addCard(const Card& card);

    // Draw the top card from the draw pile into the draw pile iterator;
    // returns the card drawn. Asserts that the draw pile is not empty.
    Card draw();

    // Returns true when the draw pile is empty
    bool isEmpty() const;

    // Shuffle the draw pile into a random order
    void shuffle();

    // Number of cards remaining in the draw pile
    int size() const;

    // Access the discard pile (cards that have been played)
    const std::vector<Card>& getDiscard() const;

    // Move all discard pile cards back to draw pile and shuffle
    void reshuffleDiscard();

    // Add a card to the discard pile (called after a card is played)
    void discard(const Card& card);

private:
    std::vector<Card> m_drawPile;
    std::vector<Card> m_discardPile;
};
