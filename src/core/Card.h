#pragma once

#include <string>
#include <vector>

// Represents a single playable card (treatment, defense, etc.)
class Card {
public:
    Card(std::string id, std::string name, int cost, int power,
         std::string type, std::vector<std::string> tags);

    const std::string&              getId()   const;
    const std::string&              getName() const;
    int                             getCost() const;
    int                             getPower() const;
    const std::string&              getType() const;
    const std::vector<std::string>& getTags() const;

private:
    std::string              m_id;
    std::string              m_name;
    int                      m_cost;
    int                      m_power;
    std::string              m_type;
    std::vector<std::string> m_tags;
};
