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
    if (value == "gain_mana")           return EffectType::GainMana;
    if (value == "gain_mana_next_turn") return EffectType::GainManaNextTurn;
    if (value == "skip_enemy_turn")     return EffectType::SkipEnemyTurn;
    if (value == "draw_cards")          return EffectType::DrawCards;
    if (value == "next_card_free")      return EffectType::NextCardFree;
    if (value == "apply_status")        return EffectType::ApplyStatus;
    if (value == "modify_max_health_percent") return EffectType::ModifyMaxHealthPercent;
    if (value == "damage_per_counter")  return EffectType::DamagePerCounter;
    return std::nullopt;
}

const char* toString(EffectType type) {
    switch (type) {
    case EffectType::Damage:           return "Damage";
    case EffectType::Block:            return "Block";
    case EffectType::Heal:             return "Heal";
    case EffectType::CleanseDebuffs:   return "Cleanse Debuffs";
    case EffectType::GainMana:         return "Gain Mana";
    case EffectType::GainManaNextTurn: return "Gain Mana Next Turn";
    case EffectType::SkipEnemyTurn:    return "Skip Enemy Turn";
    case EffectType::DrawCards:        return "Draw Cards";
    case EffectType::NextCardFree:     return "Next Card Free";
    case EffectType::ApplyStatus:      return "Apply Status";
    case EffectType::ModifyMaxHealthPercent: return "Modify Max Health Percent";
    case EffectType::DamagePerCounter: return "Damage Per Counter";
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
