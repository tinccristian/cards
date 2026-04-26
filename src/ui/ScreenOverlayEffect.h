#pragma once

#include "raylib.h"

#include <string>
#include <unordered_map>

// Fullscreen post-process overlay driven by an arbitrary fragment shader.
// Shaders are cached by path and loaded lazily (or eagerly via preload).
// draw() always composites the scene texture — with or without a shader active.
class ScreenOverlayEffect {
public:
    void preload(const std::string& path);
    void trigger(const std::string& path, float durationSecs);
    void update(float dt);
    bool isActive() const;
    void draw(const Texture2D& sceneTexture) const;
    void unloadAll();

private:
    struct ShaderEntry {
        Shader shader    = {};
        int    locElapsed = -1;
        int    locFade    = -1;
    };

    std::unordered_map<std::string, ShaderEntry> m_cache;
    const ShaderEntry* m_current  = nullptr;
    float              m_duration = 0.0f;
    float              m_elapsed  = 0.0f;
    bool               m_active   = false;
};
