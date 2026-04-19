#include "Deck.h"

#include <algorithm>
#include <random>

Deck::Deck() = default;

void Deck::addCard(const Card& card) {
    m_drawPile.push_back(card);
}

std::optional<Card> Deck::draw() {
    if (m_drawPile.empty()) {
        return std::nullopt;
    }

    Card top = m_drawPile.back();
    m_drawPile.pop_back();
    return top;
}

bool Deck::isEmpty() const {
    return m_drawPile.empty();
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(m_drawPile.begin(), m_drawPile.end(), rng);
}

int Deck::size() const {
    return static_cast<int>(m_drawPile.size());
}

const std::vector<Card>& Deck::getDiscard() const {
    return m_discardPile;
}

void Deck::reshuffleDiscard() {
    for (auto& card : m_discardPile) {
        m_drawPile.push_back(card);
    }
    m_discardPile.clear();
    shuffle();
}

void Deck::discard(const Card& card) {
    m_discardPile.push_back(card);
}

int                      Deck::getDrawPileSize()     const { return static_cast<int>(m_drawPile.size());    }
int                      Deck::getDiscardPileSize()  const { return static_cast<int>(m_discardPile.size()); }
const std::vector<Card>& Deck::getDrawPileCards()    const { return m_drawPile;    }
const std::vector<Card>& Deck::getDiscardPileCards() const { return m_discardPile; }
