#pragma once

#include "Card.h"

#include <functional>
#include <optional>
#include <vector>

// Manages a collection of cards split into draw pile, hand, and discard pile.
class Deck {
public:
    using ShuffleCallback = std::function<void()>;

    Deck();

    static void setShuffleCallback(ShuffleCallback callback);

    // Add a card to the draw pile
    void addCard(const Card& card);

    // Draw the top card from the draw pile.
    // Returns empty when the draw pile has no remaining cards.
    std::optional<Card> draw();

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

    // Draw up to n cards from the top without reshuffling. Caller owns the result.
    std::vector<Card> peekTop(int n);

    // Place a card on top of the draw pile (used to return peeked cards).
    void putOnTop(const Card& card);

    // --- Pile inspection ---
    int                      getDrawPileSize()    const;
    int                      getDiscardPileSize() const;
    const std::vector<Card>& getDrawPileCards()   const; // same as iterating m_drawPile
    const std::vector<Card>& getDiscardPileCards() const;

private:
    static ShuffleCallback s_shuffleCallback;

    std::vector<Card> m_drawPile;
    std::vector<Card> m_discardPile;
};
