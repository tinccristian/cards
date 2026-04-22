#include "Card.h"

#include <optional>
#include <utility>

std::optional<CardTier> cardTierFromString(const std::string& value) {
    if (value == "common") {
        return CardTier::Common;
    }
    if (value == "uncommon") {
        return CardTier::Uncommon;
    }
    if (value == "rare") {
        return CardTier::Rare;
    }
    return std::nullopt;
}

const char* toString(CardTier tier) {
    switch (tier) {
    case CardTier::Common:   return "Common";
    case CardTier::Uncommon: return "Uncommon";
    case CardTier::Rare:     return "Rare";
    }
    return "Common";
}

Card::Card(std::string id,
           std::string name,
           int cost,
           CardType type,
           CardTier tier,
           std::vector<std::string> tags,
           std::vector<CardEffect> effects,
           std::string description,
           std::string artPath)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_cost(cost)
    , m_type(type)
    , m_tier(tier)
    , m_tags(std::move(tags))
    , m_effects(std::move(effects))
    , m_description(std::move(description))
    , m_artPath(std::move(artPath))
{}

const std::string& Card::getId() const { return m_id; }
const std::string& Card::getName() const { return m_name; }
int Card::getCost() const { return m_cost; }
CardType Card::getType() const { return m_type; }
CardTier Card::getTier() const { return m_tier; }
const std::vector<std::string>& Card::getTags() const { return m_tags; }
const std::vector<CardEffect>& Card::getEffects() const { return m_effects; }
const std::string& Card::getDescription() const { return m_description; }
const std::string& Card::getArtPath() const { return m_artPath; }

const char* Card::getTypeLabel() const {
    return toString(m_type);
}

int Card::getEffectTotal(EffectType type, EffectTarget target) const {
    int total = 0;
    for (const auto& effect : m_effects) {
        if (effect.type == type && effect.target == target) {
            total += effect.amount;
        }
    }
    return total;
}

int Card::getDamageAmount() const {
    return getEffectTotal(EffectType::Damage, EffectTarget::Opponent);
}

int Card::getBlockAmount() const {
    return getEffectTotal(EffectType::Block, EffectTarget::Self);
}

int Card::getHealAmount() const {
    return getEffectTotal(EffectType::Heal, EffectTarget::Self);
}

int Card::getCardsDrawn() const {
    return getEffectTotal(EffectType::DrawCards, EffectTarget::Self);
}

int Card::getNextTurnManaAmount() const {
    return getEffectTotal(EffectType::GainManaNextTurn, EffectTarget::Self);
}

bool Card::hasEffect(EffectType type) const {
    for (const auto& effect : m_effects) {
        if (effect.type == type) {
            return true;
        }
    }
    return false;
}
