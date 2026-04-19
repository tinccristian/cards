#pragma once

#include "raylib.h"

#include <optional>
#include <string>
#include <unordered_map>

class CardArtCache {
public:
    std::optional<Texture2D> getTexture(const std::string& path);
    void                     unloadAll();

private:
    std::unordered_map<std::string, Texture2D> m_textures;
};
