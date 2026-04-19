#include "CardEffect.h"

std::optional<CardType> cardTypeFromString(const std::string& value) {
    if (value == "attack") return CardType::Attack;
    if (value == "skill")  return CardType::Skill;
    return std::nullopt;
}

const char* toString(CardType type) {
    switch (type) {
    case CardType::Attack: return "Attack";
    case CardType::Skill:  return "Skill";
    case CardType::Unknown: return "Unknown";
    }
    return "Unknown";
}

std::optional<EffectType> effectTypeFromString(const std::string& value) {
    if (value == "damage")              return EffectType::Damage;
    if (value == "block")               return EffectType::Block;
    if (value == "heal")                return EffectType::Heal;
    if (value == "cleanse_debuffs")     return EffectType::CleanseDebuffs;
    if (value == "gain_mana_next_turn") return EffectType::GainManaNextTurn;
    if (value == "skip_enemy_turn")     return EffectType::SkipEnemyTurn;
    if (value == "draw_cards")          return EffectType::DrawCards;
    return std::nullopt;
}

const char* toString(EffectType type) {
    switch (type) {
    case EffectType::Damage:           return "Damage";
    case EffectType::Block:            return "Block";
    case EffectType::Heal:             return "Heal";
    case EffectType::CleanseDebuffs:   return "Cleanse Debuffs";
    case EffectType::GainManaNextTurn: return "Gain Mana Next Turn";
    case EffectType::SkipEnemyTurn:    return "Skip Enemy Turn";
    case EffectType::DrawCards:        return "Draw Cards";
    case EffectType::Unknown:          return "Unknown";
    }
    return "Unknown";
}

std::optional<EffectTarget> effectTargetFromString(const std::string& value) {
    if (value == "self")     return EffectTarget::Self;
    if (value == "opponent") return EffectTarget::Opponent;
    return std::nullopt;
}

const char* toString(EffectTarget target) {
    switch (target) {
    case EffectTarget::Self:     return "Self";
    case EffectTarget::Opponent: return "Opponent";
    }
    return "Opponent";
}
