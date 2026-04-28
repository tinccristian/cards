#include "CombatResolver.h"

#include "core/Card.h"
#include "core/Enemy.h"
#include "core/Player.h"
#include "gameplay/CardEffect.h"
#include "gameplay/StatusCollection.h"

#include <cstdlib>
#include <optional>
#include <sstream>
#include <vector>

bool CardResolutionSummary::hasGameplayEffect() const {
    return damageDealt > 0 || blockGained > 0 || healingDone > 0 || cardsDrawn > 0
        || manaGained > 0 || bonusManaGranted > 0 || debuffsCleared > 0
        || nextCardFreeGranted || enemyTurnSkipped || copyNextCardGranted;
}

namespace {

std::optional<StatusType> statusTypeFromKey(const std::string& key) {
    if (key == "poison") {
        return StatusType::Poison;
    }
    return std::nullopt;
}

StatusDisposition statusDispositionFor(StatusType type, EffectTarget target) {
    if (type == StatusType::Poison) {
        return StatusDisposition::Negative;
    }
    return target == EffectTarget::Self ? StatusDisposition::Positive : StatusDisposition::Negative;
}

} // namespace

namespace CombatResolver {

CardResolutionSummary applyPlayerCard(const Card& card, Player& player, Enemy& enemy) {
    CardResolutionSummary summary;

    // Shared helper: fire all DamageOnDraw triggers (organ passive + temporary status).
    auto fireDamageOnDraw = [&]() {
        for (const Card& organ : player.getActiveOrgans()) {
            for (const CardEffect& oe : organ.getEffects()) {
                if (oe.type == EffectType::DamageOnDraw) {
                    const DamageBreakdown bd = enemy.takeDamageDetailed(oe.amount);
                    summary.enemyDamageEvents.push_back(bd);
                    summary.damageDealt += bd.health;
                    summary.damageAttempted += oe.amount;
                }
            }
        }
        const int statusDmg = player.getStatusMagnitude(StatusType::DamageOnDraw);
        if (statusDmg > 0) {
            const DamageBreakdown bd = enemy.takeDamageDetailed(statusDmg);
            summary.enemyDamageEvents.push_back(bd);
            summary.damageDealt += bd.health;
            summary.damageAttempted += statusDmg;
        }
    };

    for (const auto& effect : card.getEffects()) {
        if (effect.chance < 100 && (std::rand() % 100 + 1) > effect.chance) {
            continue;
        }
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
                const int drawnBefore = summary.cardsDrawn;
                for (int count = 0; count < effect.amount; ++count) {
                    if (player.drawCardFromDeck()) {
                        ++summary.cardsDrawn;
                        fireDamageOnDraw();
                    }
                }
                // Synthetic Lung: 1 bonus card per draw event (not per card drawn).
                const bool anyDrawn = summary.cardsDrawn > drawnBefore;
                if (anyDrawn) {
                    for (const Card& organ : player.getActiveOrgans()) {
                        for (const CardEffect& oe : organ.getEffects()) {
                            if (oe.type == EffectType::BonusDrawOnDraw) {
                                if (player.drawCardFromDeck()) {
                                    ++summary.cardsDrawn;
                                    fireDamageOnDraw();
                                }
                            }
                        }
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

        case EffectType::DamageIfPoisoned:
            if (effect.target == EffectTarget::Opponent) {
                if (enemy.getStatusMagnitude(StatusType::Poison) > 0) {
                    summary.damageAttempted += effect.amount;
                    const DamageBreakdown breakdown = enemy.takeDamageDetailed(effect.amount);
                    summary.enemyDamageEvents.push_back(breakdown);
                    summary.damageDealt += breakdown.health;
                }
            }
            break;

        case EffectType::DamagePerHandCard: {
            const int total = effect.amount * static_cast<int>(player.getHand().size());
            if (effect.target == EffectTarget::Opponent && total > 0) {
                summary.damageAttempted += total;
                const DamageBreakdown breakdown = enemy.takeDamageDetailed(total);
                summary.enemyDamageEvents.push_back(breakdown);
                summary.damageDealt += breakdown.health;
            }
            break;
        }

        case EffectType::DamageOnDrawThisTurn:
            if (effect.target == EffectTarget::Self) {
                player.addStatus(StatusType::DamageOnDraw, effect.amount, 1, StatusDisposition::Positive);
            }
            break;

        case EffectType::CopyHandCard:
            if (effect.target == EffectTarget::Self) {
                player.addStatus(StatusType::CopyNextCard, 1, 1, StatusDisposition::Positive);
                summary.copyNextCardGranted = true;
            }
            break;

        case EffectType::HealOnTurnStart:
        case EffectType::BonusDrawOnDraw:
        case EffectType::DamageOnDraw:
            // Organ passives — resolved at draw / turn-start time.
            break;

        case EffectType::PeekAndSelect:
            // Handled by GameState before the resolver runs.
            break;

        case EffectType::ApplyStatus:
            if (const auto statusType = statusTypeFromKey(effect.key); statusType.has_value()) {
                const StatusDisposition disposition = statusDispositionFor(*statusType, effect.target);
                if (effect.target == EffectTarget::Self) {
                    player.addStatus(*statusType, effect.amount, effect.duration, disposition);
                } else {
                    enemy.addStatus(*statusType, effect.amount, effect.duration, disposition);
                }
            }
            break;

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
    if (summary.copyNextCardGranted) {
        fragments.push_back("will copy the next card played into the discard pile");
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
