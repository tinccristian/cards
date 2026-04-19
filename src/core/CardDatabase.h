#pragma once

#include "Card.h"
#include "Deck.h"
#include <string>
#include <unordered_map>
#include <vector>

// Static registry of player card definitions.
// Call loadCardsFromJSON() once at startup, then use getCard() / getAllCards().
// Enemy decks are loaded on-demand via loadEnemyDeckFromJSON() which returns a Deck.
class CardDatabase {
public:
    // Parse player card definitions from a JSON file into the static registry.
    static void loadCardsFromJSON(const std::string& filePath);

    // Return a player card by its unique ID. Asserts if the ID is not found.
    static Card getCard(const std::string& cardId);

    // Return every registered player card.
    static const std::vector<Card>& getAllCards();

    // Parse an enemy card JSON file and return a shuffled Deck ready for use.
    // These cards are NOT added to the player card registry.
    static Deck loadEnemyDeckFromJSON(const std::string& filePath);

    // Parse a deck config JSON file and return the ordered list of card IDs.
    // Each ID may appear multiple times to indicate duplicates.
    static std::vector<std::string> loadDeckConfigFromJSON(const std::string& filePath);

private:
    static std::vector<Card>                    s_cards;
    static std::unordered_map<std::string, int> s_index; // id -> index in s_cards
};
