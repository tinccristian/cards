#pragma once

#include "Card.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Static registry of player card definitions.
// Call loadCardsFromJSON() once at startup, then use findCard() / getAllCards().
class CardDatabase {
public:
    // Parse player card definitions from a JSON file into the static registry.
    static bool loadCardsFromJSON(const std::string& filePath, std::string& error);

    // Return a player card by its unique ID.
    static std::optional<Card> findCard(const std::string& cardId);

    // Return every registered player card.
    static const std::vector<Card>& getAllCards();

    // Parse a deck config JSON file and return the ordered list of card IDs.
    // Each ID may appear multiple times to indicate duplicates.
    static std::vector<std::string> loadDeckConfigFromJSON(const std::string& filePath, std::string& error);

private:
    static std::vector<Card>                    s_cards;
    static std::unordered_map<std::string, int> s_index; // id -> index in s_cards
};
