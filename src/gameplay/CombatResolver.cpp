#include "CombatResolver.h"

#include "core/Card.h"
#include "core/Enemy.h"
#include "core/Player.h"
#include "gameplay/CardEffect.h"
#include "gameplay/StatusCollection.h"

#include <sstream>
#include <vector>

bool CardResolutionSummary::hasGameplayEffect() const {
    return damageDealt > 0 || blockGained > 0 || healingDone > 0 || cardsDrawn > 0
        || manaGained > 0 || bonusManaGranted > 0 || debuffsCleared > 0
        || nextCardFreeGranted || enemyTurnSkipped;
}

namespace CombatResolver {

CardResolutionSummary applyPlayerCard(const Card& card, Player& player, Enemy& enemy) {
    CardResolutionSummary summary;

    for (const auto& effect : card.getEffects()) {
        switch (effect.type) {
        case EffectType::Damage:
            if (effect.target == EffectTarget::Opponent) {
                summary.damageAttempted += effect.amount;
                const DamageBreakdown breakdown = enemy.takeDamageDetailed(effect.amount);
                summary.enemyDamageEvents.push_back(breakdown);
                summary.damageDealt += breakdown.health;
            } else {
                summary.damageAttempted += effect.amount;
                const DamageBreakdown breakdown = player.takeDamageDetailed(effect.amount);
                summary.playerDamageEvents.push_back(breakdown);
                summary.damageDealt += breakdown.health;
            }
            break;

        case EffectType::Block:
            if (effect.target == EffectTarget::Self) {
                player.addBlock(effect.amount);
                summary.blockGained += effect.amount;
            } else {
                enemy.addBlock(effect.amount);
            }
            break;

        case EffectType::Heal:
            if (effect.target == EffectTarget::Self) {
                summary.healingDone += player.heal(effect.amount);
            }
            break;

        case EffectType::CleanseDebuffs:
            if (effect.target == EffectTarget::Self) {
                summary.debuffsCleared += player.clearNegativeStatuses();
            }
            break;

        case EffectType::GainMana:
            if (effect.target == EffectTarget::Self) {
                player.gainMana(effect.amount);
                summary.manaGained += effect.amount;
            }
            break;

        case EffectType::GainManaNextTurn:
            if (effect.target == EffectTarget::Self) {
                player.addStatus(StatusType::BonusManaNextTurn, effect.amount, effect.duration, StatusDisposition::Positive);
                summary.bonusManaGranted += effect.amount;
            }
            break;

        case EffectType::SkipEnemyTurn:
            if (effect.target == EffectTarget::Opponent) {
                enemy.queueSkipTurn(effect.amount, effect.duration);
                summary.enemyTurnSkipped = true;
            }
            break;

        case EffectType::DrawCards:
            if (effect.target == EffectTarget::Self) {
                for (int count = 0; count < effect.amount; ++count) {
                    if (player.drawCardFromDeck()) {
                        ++summary.cardsDrawn;
                    }
                }
            }
            break;

        case EffectType::NextCardFree:
            if (effect.target == EffectTarget::Self) {
                player.addStatus(StatusType::NextCardFree, 1, 1, StatusDisposition::Positive);
                summary.nextCardFreeGranted = true;
            }
            break;

        case EffectType::ApplyStatus:
        case EffectType::ModifyMaxHealthPercent:
        case EffectType::DamagePerCounter:
        case EffectType::Unknown:
            break;
        }
    }

    return summary;
}

std::string buildPlayerActionText(const Card& card, const CardResolutionSummary& summary) {
    if (!summary.hasGameplayEffect()) {
        return "You played " + card.getDisplayName() + ".";
    }

    std::vector<std::string> fragments;

    if (summary.damageDealt > 0) {
        fragments.push_back("dealt " + std::to_string(summary.damageDealt) + " damage");
    }
    if (summary.blockGained > 0) {
        fragments.push_back("gained " + std::to_string(summary.blockGained) + " block");
    }
    if (summary.healingDone > 0) {
        fragments.push_back("healed " + std::to_string(summary.healingDone) + " HP");
    }
    if (summary.cardsDrawn > 0) {
        fragments.push_back("drew " + std::to_string(summary.cardsDrawn) + " card");
        if (summary.cardsDrawn > 1) {
            fragments.back() += "s";
        }
    }
    if (summary.manaGained > 0) {
        fragments.push_back("gained +" + std::to_string(summary.manaGained) + " mana");
    }
    if (summary.bonusManaGranted > 0) {
        fragments.push_back("banked +" + std::to_string(summary.bonusManaGranted) + " mana for next turn");
    }
    if (summary.nextCardFreeGranted) {
        fragments.push_back("made the next card cost 0");
    }
    if (summary.debuffsCleared > 0) {
        fragments.push_back("cleansed " + std::to_string(summary.debuffsCleared) + " debuff");
        if (summary.debuffsCleared > 1) {
            fragments.back() += "s";
        }
    }
    if (summary.enemyTurnSkipped) {
        fragments.push_back("stopped the enemy from acting next turn");
    }

    std::ostringstream builder;
    builder << "You played " << card.getDisplayName() << ": ";

    for (size_t index = 0; index < fragments.size(); ++index) {
        if (index > 0) {
            builder << (index + 1 == fragments.size() ? " and " : ", ");
        }
        builder << fragments[index];
    }

    builder << ".";
    return builder.str();
}

} // namespace CombatResolver
