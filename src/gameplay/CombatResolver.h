#pragma once

#include <string>

class Card;
class Enemy;
class Player;

struct CardResolutionSummary {
    int damageAttempted    = 0;
    int damageDealt       = 0;
    int blockGained       = 0;
    int healingDone       = 0;
    int cardsDrawn        = 0;
    int manaGained        = 0;
    int bonusManaGranted  = 0;
    int debuffsCleared    = 0;
    bool enemyTurnSkipped = false;

    [[nodiscard]] bool hasGameplayEffect() const;
};

namespace CombatResolver {
CardResolutionSummary applyPlayerCard(const Card& card, Player& player, Enemy& enemy);
std::string           buildPlayerActionText(const Card& card, const CardResolutionSummary& summary);
}
