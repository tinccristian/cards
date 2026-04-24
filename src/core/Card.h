#pragma once

#include "gameplay/CardEffect.h"

#include <optional>
#include <string>
#include <vector>

enum class CardTier {
    Common,
    Uncommon,
    Rare
};

std::optional<CardTier> cardTierFromString(const std::string& value);
const char*             toString(CardTier tier);

class Card {
public:
    Card(std::string id,
         std::string name,
         int cost,
         CardType type,
         CardTier tier,
         std::vector<std::string> tags,
         std::vector<CardEffect> effects,
         std::string description = "",
         std::string artPath = "",
         std::string displayName = "",
         std::string displayDescription = "",
         bool obscured = false,
         bool hideFooterStats = false);

    const std::string&              getId() const;
    const std::string&              getName() const;
    int                             getCost() const;
    CardType                        getType() const;
    CardTier                        getTier() const;
    const std::vector<std::string>& getTags() const;
    const std::vector<CardEffect>&  getEffects() const;
    const std::string&              getDescription() const;
    const std::string&              getArtPath() const;
    const std::string&              getDisplayName() const;
    const std::string&              getDisplayDescription() const;
    bool                            isObscured() const;
    bool                            shouldHideFooterStats() const;

    const char* getTypeLabel() const;

    int  getEffectTotal(EffectType type, EffectTarget target) const;
    int  getDamageAmount() const;
    int  getBlockAmount() const;
    int  getHealAmount() const;
    int  getCardsDrawn() const;
    int  getNextTurnManaAmount() const;
    bool hasEffect(EffectType type) const;

private:
    std::string              m_id;
    std::string              m_name;
    int                      m_cost;
    CardType                 m_type;
    CardTier                 m_tier;
    std::vector<std::string> m_tags;
    std::vector<CardEffect>  m_effects;
    std::string              m_description;
    std::string              m_artPath;
    std::string              m_displayName;
    std::string              m_displayDescription;
    bool                     m_obscured = false;
    bool                     m_hideFooterStats = false;
};
