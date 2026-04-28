#pragma once

#include <optional>
#include <string>

enum class CardType {
    Attack,
    Skill,
    Organ,
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
    NextCardFree,
    ApplyStatus,
    ModifyMaxHealthPercent,
    DamagePerCounter,
    DamageIfPoisoned,    // deals bonus damage when enemy is poisoned
    DamagePerHandCard,   // deals amount * hand_size damage
    HealOnTurnStart,     // organ passive: heals amount HP at player turn start
    BonusDrawOnDraw,     // organ passive: draws amount extra cards per draw event
    DamageOnDraw,         // organ passive: deal amount damage to enemy each time player draws
    DamageOnDrawThisTurn, // skill: grant a status that deals amount damage per draw until end of turn
    PeekAndSelect,        // reveal top amount cards; player picks 1 to add to hand free this turn; rest go back
    CopyHandCard,         // copy a random card from the player's current hand into the discard pile
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
    // Optional identifier used by data-driven effects such as
    // `apply_status` ("poison") or counter-based scaling ("spore_growth").
    std::string  key;
    // Percent chance (1–100) this effect fires. 100 = always.
    int          chance   = 100;
};

std::optional<CardType> cardTypeFromString(const std::string& value);
const char*             toString(CardType type);

std::optional<EffectType> effectTypeFromString(const std::string& value);
const char*               toString(EffectType type);

std::optional<EffectTarget> effectTargetFromString(const std::string& value);
const char*                 toString(EffectTarget target);
