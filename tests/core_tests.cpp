#include "config/Defines.h"
#include "content/EnemyFactory.h"
#include "core/CardDatabase.h"
#include "core/GameState.h"
#include "core/Player.h"
#include "gameplay/CardEffect.h"
#include "gameplay/StatusCollection.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

bool assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << "\n";
        return false;
    }
    return true;
}

bool testCardDatabaseLoadsEffects() {
    std::string error;
    if (!CardDatabase::loadCardsFromJSON(AssetPaths::PLAYER_CARD_LIBRARY, error)) {
        std::cerr << error << "\n";
        return false;
    }

    const auto quarantine = CardDatabase::findCard("quarantine");
    if (!assertTrue(quarantine.has_value(), "quarantine card should exist")) {
        return false;
    }

    return assertTrue(quarantine->hasEffect(EffectType::SkipEnemyTurn), "quarantine should skip the enemy turn");
}

bool testPlayerBonusManaStatus() {
    Player player(CombatConfig::PlayerMaxHealth, CombatConfig::PlayerBaseMana);
    player.addStatus(StatusType::BonusManaNextTurn, 2, 1, StatusDisposition::Positive);
    player.startTurn();
    if (!assertTrue(player.getMana() == CombatConfig::PlayerBaseMana + 2, "bonus mana should apply at turn start")) {
        return false;
    }

    player.startTurn();
    return assertTrue(player.getMana() == CombatConfig::PlayerBaseMana, "bonus mana should be consumed after one turn");
}

bool testGameStateResolvesSpecialCards() {
    std::string error;
    if (!CardDatabase::loadCardsFromJSON(AssetPaths::PLAYER_CARD_LIBRARY, error)) {
        std::cerr << error << "\n";
        return false;
    }

    GameState state;
    if (!state.startNewGame(error)) {
        std::cerr << error << "\n";
        return false;
    }

    auto& hand = state.getPlayer().getHand();
    hand.clear();

    const auto immuneBoost = CardDatabase::findCard("immune_boost");
    const auto quarantine  = CardDatabase::findCard("quarantine");
    if (!assertTrue(immuneBoost.has_value() && quarantine.has_value(), "special cards should load")) {
        return false;
    }

    hand.push_back(*immuneBoost);
    hand.push_back(*quarantine);

    if (!assertTrue(state.playCard(0).has_value(), "immune boost should play successfully")) {
        return false;
    }

    if (!assertTrue(state.playCard(0).has_value(), "quarantine should play successfully")) {
        return false;
    }

    state.endPlayerTurn();
    state.executeEnemyTurn();

    return assertTrue(state.getPlayer().getMana() == CombatConfig::PlayerBaseMana + 1,
        "immune boost should grant bonus mana after the skipped enemy turn");
}

bool testEnemyFactoryLoadsEnemy() {
    std::string error;
    auto enemy = EnemyFactory::loadFromJSON(AssetPaths::DEFAULT_ENEMY, error);
    if (!assertTrue(enemy != nullptr, "enemy should load from JSON")) {
        std::cerr << error << "\n";
        return false;
    }

    return assertTrue(enemy->getName() == "Fungus", "enemy should keep its configured name");
}

} // namespace

int main() {
    const bool ok =
        testCardDatabaseLoadsEffects() &&
        testPlayerBonusManaStatus() &&
        testGameStateResolvesSpecialCards() &&
        testEnemyFactoryLoadsEnemy();

    return ok ? 0 : 1;
}
