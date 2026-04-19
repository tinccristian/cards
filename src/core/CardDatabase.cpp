#include "CardDatabase.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

std::vector<Card>                    CardDatabase::s_cards;
std::unordered_map<std::string, int> CardDatabase::s_index;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Build a Card from a single JSON object. Returns an empty optional on error.
static std::optional<Card> parseEntry(const nlohmann::json& entry) {
    try {
        std::string id   = entry.value("id",   "");
        std::string name = entry.value("name", "");
        if (id.empty() || name.empty()) return std::nullopt;

        int  cost        = entry.value("cost",        0);
        int  power       = entry.value("power",       0);
        int  blockAmount = entry.value("blockAmount", 0);
        std::string type = entry.value("type",        "attack");
        std::string desc = entry.value("description", "");
        std::string art  = entry.value("artPath",     "");

        std::vector<std::string> tags;
        if (entry.contains("tags") && entry["tags"].is_array()) {
            for (const auto& t : entry["tags"])
                tags.push_back(t.get<std::string>());
        }

        return Card(id, name, cost, power, type, tags, desc, art, blockAmount);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[CardDatabase] Skipping malformed entry: " << e.what() << "\n";
        return std::nullopt;
    }
}

static nlohmann::json readJSON(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[CardDatabase] Cannot open: " << filePath << "\n";
        return {};
    }
    nlohmann::json root;
    try { file >> root; }
    catch (const nlohmann::json::exception& e) {
        std::cerr << "[CardDatabase] JSON parse error in " << filePath << ": " << e.what() << "\n";
        return {};
    }
    return root;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CardDatabase::loadCardsFromJSON(const std::string& filePath) {
    s_cards.clear();
    s_index.clear();

    nlohmann::json root = readJSON(filePath);
    if (!root.contains("cards") || !root["cards"].is_array()) return;

    for (const auto& entry : root["cards"]) {
        auto card = parseEntry(entry);
        if (!card) continue;
        int idx = static_cast<int>(s_cards.size());
        s_index[card->getId()] = idx;
        s_cards.push_back(std::move(*card));
        s_cards.back().loadArt(); // load texture now (InitWindow must already be called)
    }

    std::cout << "[CardDatabase] Loaded " << s_cards.size()
              << " player cards from " << filePath << "\n";
}

Card CardDatabase::getCard(const std::string& cardId) {
    auto it = s_index.find(cardId);
    assert(it != s_index.end() && "Card ID not found in database");
    return s_cards[it->second];
}

const std::vector<Card>& CardDatabase::getAllCards() {
    return s_cards;
}

Deck CardDatabase::loadEnemyDeckFromJSON(const std::string& filePath) {
    Deck deck;
    nlohmann::json root = readJSON(filePath);
    if (!root.contains("cards") || !root["cards"].is_array()) return deck;

    for (const auto& entry : root["cards"]) {
        auto card = parseEntry(entry);
        if (card) deck.addCard(*card);
    }

    deck.shuffle();
    std::cout << "[CardDatabase] Loaded enemy deck (" << deck.size()
              << " cards) from " << filePath << "\n";
    return deck;
}
