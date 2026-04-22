#include "EnemyFactory.h"

#include "config/Defines.h"
#include "content/EnemySpriteConfig.h"
#include "core/Card.h"
#include "core/Enemy.h"
#include "gameplay/CardEffect.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>
#include <vector>

namespace {

std::optional<Card> parseEnemyCard(const nlohmann::json& entry, std::string& error) {
    const std::string id   = entry.value("id", "");
    const std::string name = entry.value("name", "");
    const std::string typeValue = entry.value("type", "");

    if (id.empty() || name.empty()) {
        error = "Enemy card is missing a required id or name";
        return std::nullopt;
    }

    auto cardType = cardTypeFromString(typeValue);
    if (!cardType) {
        error = "Enemy card '" + id + "' has an invalid type: " + typeValue;
        return std::nullopt;
    }

    std::vector<std::string> tags;
    if (entry.contains("tags") && entry["tags"].is_array()) {
        for (const auto& tag : entry["tags"]) {
            if (!tag.is_string()) {
                error = "Enemy card '" + id + "' has a non-string tag";
                return std::nullopt;
            }
            tags.push_back(tag.get<std::string>());
        }
    }

    std::vector<CardEffect> effects;
    if (!entry.contains("effects") || !entry["effects"].is_array()) {
        error = "Enemy card '" + id + "' is missing an effects array";
        return std::nullopt;
    }

    for (const auto& effectEntry : entry["effects"]) {
        const std::string effectTypeValue = effectEntry.value("type", "");
        auto effectType = effectTypeFromString(effectTypeValue);
        if (!effectType) {
            error = "Enemy card '" + id + "' has an invalid effect type: " + effectTypeValue;
            return std::nullopt;
        }

        const std::string targetValue = effectEntry.value("target", "opponent");
        auto effectTarget = effectTargetFromString(targetValue);
        if (!effectTarget) {
            error = "Enemy card '" + id + "' has an invalid effect target: " + targetValue;
            return std::nullopt;
        }

                effects.push_back(CardEffect{
                    *effectType,
                    *effectTarget,
                    effectEntry.value("amount", 0),
                    effectEntry.value("duration", CombatConfig::DefaultEffectDuration)
                });
            }

    return Card(
        id,
        name,
        entry.value("cost", 0),
        *cardType,
        CardTier::Common,
        std::move(tags),
        std::move(effects),
        entry.value("description", ""),
        entry.value("artPath", "")
    );
}

} // namespace

std::unique_ptr<Enemy> EnemyFactory::loadFromJSON(const std::string& filePath, std::string& error) {
    error.clear();
    std::ifstream file(filePath);
    if (!file.is_open()) {
        error = "Unable to open enemy definition: " + filePath;
        return nullptr;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::exception& ex) {
        error = "Failed to parse enemy definition '" + filePath + "': " + ex.what();
        return nullptr;
    }

    const std::string name = root.value("name", "");
    const int maxHealth = root.value("maxHealth", 0);
    const int goldReward = root.value("goldReward", 0);

    if (name.empty() || maxHealth <= 0) {
        error = "Enemy definition must provide a valid name and maxHealth";
        return nullptr;
    }

    Deck enemyDeck;
    if (!root.contains("cards") || !root["cards"].is_array()) {
        error = "Enemy definition '" + filePath + "' is missing a cards array";
        return nullptr;
    }

    for (const auto& cardEntry : root["cards"]) {
        auto card = parseEnemyCard(cardEntry, error);
        if (!card) {
            return nullptr;
        }
        enemyDeck.addCard(*card);
    }

    enemyDeck.shuffle();

    // --- Parse optional sprite section ---
    EnemySpriteConfig spriteConfig;
    if (root.contains("sprite") && root["sprite"].is_object()) {
        const auto& s = root["sprite"];
        spriteConfig.sheetPath   = s.value("sheet",       "");
        spriteConfig.frameWidth  = s.value("frameWidth",  80);
        spriteConfig.frameHeight = s.value("frameHeight", 80);

        auto parseClip = [&s](const char* key, AnimClip& clip) {
            if (!s.contains(key) || !s[key].is_object()) return;
            const auto& c = s[key];
            clip.startFrame      = c.value("startFrame",      clip.startFrame);
            clip.frameCount      = c.value("frameCount",      clip.frameCount);
            clip.frameDurationMs = c.value("frameDurationMs", clip.frameDurationMs);
            clip.looping         = c.value("looping",         clip.looping);
        };

        parseClip("idle",   spriteConfig.idle);
        parseClip("attack", spriteConfig.attack);
        parseClip("death",  spriteConfig.death);
    }

    return std::make_unique<Enemy>(name, maxHealth, std::move(enemyDeck),
                                   std::move(spriteConfig), goldReward);
}

std::unique_ptr<Enemy> EnemyFactory::loadById(const std::string& enemyId, std::string& error) {
    if (enemyId.empty()) {
        error = "Enemy id cannot be empty";
        return nullptr;
    }

    return loadFromJSON(std::string(AssetPaths::ENEMY_DIRECTORY) + "/" + enemyId + ".json", error);
}
