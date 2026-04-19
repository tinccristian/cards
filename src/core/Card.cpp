#include "Card.h"
#include <utility>

Card::Card(std::string id, std::string name, int cost, int power,
           std::string type, std::vector<std::string> tags,
           std::string description, std::string artPath, int blockAmount)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_cost(cost)
    , m_power(power)
    , m_blockAmount(blockAmount)
    , m_type(std::move(type))
    , m_tags(std::move(tags))
    , m_description(std::move(description))
    , m_artPath(std::move(artPath))
{}

const std::string& Card::getId()          const { return m_id;          }
const std::string& Card::getName()        const { return m_name;        }
int                Card::getCost()        const { return m_cost;        }
int                Card::getPower()       const { return m_power;       }
int                Card::getBlockAmount() const { return m_blockAmount; }
const std::string& Card::getType()        const { return m_type;        }
const std::vector<std::string>& Card::getTags() const { return m_tags;  }
const std::string& Card::getDescription() const { return m_description; }
const std::string& Card::getArtPath()     const { return m_artPath;     }

void Card::loadArt() {
    if (m_artPath.empty()) {
        TraceLog(LOG_WARNING, "Card %s has empty art path", m_id.c_str());
        return;
    }

    if (!FileExists(m_artPath.c_str())) {
        TraceLog(LOG_ERROR, "File does not exist: %s", m_artPath.c_str());
        return;
    }

    Image img = LoadImage(m_artPath.c_str());
    if (img.data == nullptr) {
        TraceLog(LOG_ERROR, "Failed to decode image: %s", m_artPath.c_str());
        return;
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    if (tex.id == 0) {
        TraceLog(LOG_ERROR, "Failed to create texture: %s", m_artPath.c_str());
        return;
    }

    m_artTexture = tex;
}

std::optional<Texture2D> Card::getArtTexture() const {
    return m_artTexture;
}
