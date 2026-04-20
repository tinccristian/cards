#pragma once

#include <optional>
#include <string>

enum class CardType {
    Attack,
    Skill,
    Unknown
};

enum class EffectType {
    Damage,
    Block,
    Heal,
    CleanseDebuffs,
    GainMana,
    GainManaNextTurn,
    SkipEnemyTurn,
    DrawCards,
    Unknown
};

enum class EffectTarget {
    Self,
    Opponent
};

struct CardEffect {
    EffectType   type     = EffectType::Unknown;
    EffectTarget target   = EffectTarget::Opponent;
    int          amount   = 0;
    int          duration = 0;
};

std::optional<CardType> cardTypeFromString(const std::string& value);
const char*             toString(CardType type);

std::optional<EffectType> effectTypeFromString(const std::string& value);
const char*               toString(EffectType type);

std::optional<EffectTarget> effectTargetFromString(const std::string& value);
const char*                 toString(EffectTarget target);
