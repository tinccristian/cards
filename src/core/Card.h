#pragma once

#include "raylib.h"
#include <optional>
#include <string>
#include <vector>

// Represents a single playable card (treatment, defense, etc.)
class Card {
public:
    Card(std::string id, std::string name, int cost, int power,
         std::string type, std::vector<std::string> tags,
         std::string description = "", std::string artPath = "",
         int blockAmount = 0);

    const std::string&              getId()          const;
    const std::string&              getName()        const;
    int                             getCost()        const;
    int                             getPower()       const;
    // Block applied to the player when this skill card is played
    int                             getBlockAmount() const;
    const std::string&              getType()        const;
    const std::vector<std::string>& getTags()        const;
    const std::string&              getDescription() const;
    const std::string&              getArtPath()     const;

    // Load texture from artPath (must be called after InitWindow).
    // Silently skips missing or invalid files.
    void loadArt();

    // Returns the loaded texture if loadArt() succeeded, otherwise empty.
    std::optional<Texture2D> getArtTexture() const;

private:
    std::string              m_id;
    std::string              m_name;
    int                      m_cost;
    int                      m_power;
    int                      m_blockAmount;
    std::string              m_type;
    std::vector<std::string> m_tags;
    std::string              m_description;
    std::string              m_artPath;
    std::optional<Texture2D> m_artTexture;
};
