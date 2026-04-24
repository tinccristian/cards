#include "CardDatabase.h"

#include "config/Defines.h"
#include "gameplay/CardEffect.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

std::vector<Card>                    CardDatabase::s_cards;
std::unordered_map<std::string, int> CardDatabase::s_index;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::optional<Card> parseEntry(const nlohmann::json& entry, std::string& error) {
    try {
        const std::string id   = entry.value("id", "");
        const std::string name = entry.value("name", "");
        if (id.empty() || name.empty()) {
            error = "Card entry is missing a required id or name";
            return std::nullopt;
        }

        const std::string typeValue = entry.value("type", "");
        auto cardType = cardTypeFromString(typeValue);
        if (!cardType) {
            error = "Card '" + id + "' has an invalid type: " + typeValue;
            return std::nullopt;
        }

        const std::string tierValue = entry.value("tier", "");
        auto cardTier = cardTierFromString(tierValue);
        if (!cardTier) {
            error = "Card '" + id + "' has an invalid tier: " + tierValue;
            return std::nullopt;
        }

        std::vector<std::string> tags;
        if (entry.contains("tags") && entry["tags"].is_array()) {
            for (const auto& tag : entry["tags"]) {
                if (!tag.is_string()) {
                    error = "Card '" + id + "' has a non-string tag";
                    return std::nullopt;
                }
                tags.push_back(tag.get<std::string>());
            }
        }

        std::vector<CardEffect> effects;
        if (entry.contains("effects") && entry["effects"].is_array()) {
            for (const auto& effectEntry : entry["effects"]) {
                const std::string effectTypeValue = effectEntry.value("type", "");
                auto effectType = effectTypeFromString(effectTypeValue);
                if (!effectType) {
                    error = "Card '" + id + "' has an invalid effect type: " + effectTypeValue;
                    return std::nullopt;
                }

                const std::string targetValue = effectEntry.value("target", "opponent");
                auto effectTarget = effectTargetFromString(targetValue);
                if (!effectTarget) {
                    error = "Card '" + id + "' has an invalid effect target: " + targetValue;
                    return std::nullopt;
                }

                effects.push_back(CardEffect{
                    *effectType,
                    *effectTarget,
                    effectEntry.value("amount", 0),
                    effectEntry.value("duration", CombatConfig::DefaultEffectDuration),
                    effectEntry.value("key", "")
                });
            }
        } else {
            const int power = entry.value("power", 0);
            const int blockAmount = entry.value("blockAmount", 0);
            if (power > 0) {
                effects.push_back(CardEffect{ EffectType::Damage, EffectTarget::Opponent, power, CombatConfig::DefaultEffectDuration, "" });
            }
            if (blockAmount > 0) {
                effects.push_back(CardEffect{ EffectType::Block, EffectTarget::Self, blockAmount, CombatConfig::DefaultEffectDuration, "" });
            }
        }

        return Card(
            id,
            name,
            entry.value("cost", 0),
            *cardType,
            *cardTier,
            std::move(tags),
            std::move(effects),
            entry.value("description", ""),
            entry.value("artPath", ""),
            entry.value("displayName", ""),
            entry.value("displayDescription", ""),
            entry.value("obscured", false),
            entry.value("hideFooterStats", false)
        );
    } catch (const nlohmann::json::exception& ex) {
        error = ex.what();
        return std::nullopt;
    }
}

static nlohmann::json readJSON(const std::string& filePath, std::string& error) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        error = "Cannot open JSON file: " + filePath;
        return {};
    }
    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::exception& ex) {
        error = "JSON parse error in " + filePath + ": " + ex.what();
        return {};
    }
    return root;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool CardDatabase::loadCardsFromJSON(const std::string& filePath, std::string& error) {
    error.clear();
    s_cards.clear();
    s_index.clear();
    return appendCardsFromJSON(filePath, error);
}

bool CardDatabase::appendCardsFromJSON(const std::string& filePath, std::string& error) {
    error.clear();
    nlohmann::json root = readJSON(filePath, error);
    if (root.is_null()) {
        return false;
    }

    if (!root.contains("cards") || !root["cards"].is_array()) {
        error = "Card library is missing a cards array: " + filePath;
        return false;
    }

    for (const auto& entry : root["cards"]) {
        auto card = parseEntry(entry, error);
        if (!card) {
            return false;
        }

        if (s_index.find(card->getId()) != s_index.end()) {
            error = "Duplicate card id found in card library: " + card->getId();
            return false;
        }

        int idx = static_cast<int>(s_cards.size());
        s_index[card->getId()] = idx;
        s_cards.push_back(std::move(*card));
    }

    if (s_cards.empty()) {
        error = "Card library is empty: " + filePath;
        return false;
    }

    std::cout << "[CardDatabase] Loaded " << s_cards.size()
              << " card definitions from " << filePath << "\n";
    return true;
}

std::vector<std::string> CardDatabase::loadDeckConfigFromJSON(const std::string& filePath, std::string& error) {
    error.clear();
    std::vector<std::string> ids;
    nlohmann::json root = readJSON(filePath, error);
    if (root.is_null()) {
        return ids;
    }

    if (!root.contains("starter_deck") || !root["starter_deck"].is_array()) {
        error = "No starter_deck array found in " + filePath;
        return ids;
    }

    for (const auto& entry : root["starter_deck"]) {
        if (!entry.is_string()) {
            error = "starter_deck contains a non-string entry in " + filePath;
            return {};
        }
        ids.push_back(entry.get<std::string>());
    }

    std::cout << "[CardDatabase] Loaded deck config with " << ids.size()
              << " cards from " << filePath << "\n";
    return ids;
}

std::optional<Card> CardDatabase::findCard(const std::string& cardId) {
    auto it = s_index.find(cardId);
    if (it == s_index.end()) {
        return std::nullopt;
    }
    return s_cards[it->second];
}

const std::vector<Card>& CardDatabase::getAllCards() {
    return s_cards;
}

std::vector<Card> CardDatabase::getCardsWithTag(const std::string& tag) {
    std::vector<Card> results;
    for (const Card& card : s_cards) {
        const auto& tags = card.getTags();
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            results.push_back(card);
        }
    }
    return results;
}
