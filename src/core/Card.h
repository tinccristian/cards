#pragma once

#include "gameplay/CardEffect.h"

#include <string>
#include <vector>

class Card {
public:
    Card(std::string id,
         std::string name,
         int cost,
         CardType type,
         std::vector<std::string> tags,
         std::vector<CardEffect> effects,
         std::string description = "",
         std::string artPath = "");

    const std::string&              getId() const;
    const std::string&              getName() const;
    int                             getCost() const;
    CardType                        getType() const;
    const std::vector<std::string>& getTags() const;
    const std::vector<CardEffect>&  getEffects() const;
    const std::string&              getDescription() const;
    const std::string&              getArtPath() const;

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
    std::vector<std::string> m_tags;
    std::vector<CardEffect>  m_effects;
    std::string              m_description;
    std::string              m_artPath;
};
