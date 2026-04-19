#include "Card.h"
#include <utility>

Card::Card(std::string id, std::string name, int cost, int power,
           std::string type, std::vector<std::string> tags)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_cost(cost)
    , m_power(power)
    , m_type(std::move(type))
    , m_tags(std::move(tags))
{}

const std::string& Card::getId()    const { return m_id;    }
const std::string& Card::getName()  const { return m_name;  }
int                Card::getCost()  const { return m_cost;  }
int                Card::getPower() const { return m_power; }
const std::string& Card::getType()  const { return m_type;  }
const std::vector<std::string>& Card::getTags() const { return m_tags; }
