#include "CardArtCache.h"

std::optional<Texture2D> CardArtCache::getTexture(const std::string& path) {
    if (path.empty()) {
        return std::nullopt;
    }

    auto existing = m_textures.find(path);
    if (existing != m_textures.end()) {
        return existing->second;
    }

    if (!FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "Missing card art: %s", path.c_str());
        return std::nullopt;
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        TraceLog(LOG_WARNING, "Failed to load card art: %s", path.c_str());
        return std::nullopt;
    }

    m_textures.emplace(path, texture);
    return texture;
}

void CardArtCache::unloadAll() {
    for (auto& [_, texture] : m_textures) {
        if (texture.id != 0) {
            UnloadTexture(texture);
            texture = Texture2D{};
        }
    }

    m_textures.clear();
}
