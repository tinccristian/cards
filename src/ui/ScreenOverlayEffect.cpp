#include "ScreenOverlayEffect.h"

#include "raylib.h"

#include <algorithm>

void ScreenOverlayEffect::preload(const std::string& path) {
    if (path.empty() || m_cache.count(path)) return;
    if (!FileExists(path.c_str())) return;

    Shader s = LoadShader(nullptr, path.c_str());
    if (s.id == 0) {
        TraceLog(LOG_WARNING, "ScreenOverlayEffect: failed to load shader %s", path.c_str());
        return;
    }
    ShaderEntry entry;
    entry.shader     = s;
    entry.locElapsed = GetShaderLocation(s, "elapsed");
    entry.locFade    = GetShaderLocation(s, "fade");
    m_cache[path]    = entry;
}

void ScreenOverlayEffect::trigger(const std::string& path, float durationSecs) {
    if (path.empty()) return;
    auto it = m_cache.find(path);
    if (it == m_cache.end()) {
        preload(path);
        it = m_cache.find(path);
        if (it == m_cache.end()) return;
    }
    m_current  = &it->second;
    m_duration = durationSecs;
    m_elapsed  = 0.0f;
    m_active   = true;
}

void ScreenOverlayEffect::update(float dt) {
    if (!m_active) return;
    m_elapsed += dt;
    if (m_elapsed >= m_duration) {
        m_active  = false;
        m_current = nullptr;
    }
}

bool ScreenOverlayEffect::isActive() const { return m_active; }

void ScreenOverlayEffect::draw(const Texture2D& sceneTexture) const {
    const Rectangle srcRect = { 0.0f, 0.0f, (float)sceneTexture.width, -(float)sceneTexture.height };
    const Rectangle dstRect = { 0.0f, 0.0f, (float)GetScreenWidth(),   (float)GetScreenHeight()    };
    const Vector2   origin  = { 0.0f, 0.0f };

    if (m_active && m_current) {
        const float fade = (m_duration > 0.0f)
            ? std::max(0.0f, 1.0f - m_elapsed / m_duration)
            : 0.0f;
        SetShaderValue(m_current->shader, m_current->locElapsed, &m_elapsed, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_current->shader, m_current->locFade,    &fade,      SHADER_UNIFORM_FLOAT);
        BeginShaderMode(m_current->shader);
        DrawTexturePro(sceneTexture, srcRect, dstRect, origin, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawTexturePro(sceneTexture, srcRect, dstRect, origin, 0.0f, WHITE);
    }
}

void ScreenOverlayEffect::unloadAll() {
    for (auto& [path, entry] : m_cache) {
        UnloadShader(entry.shader);
    }
    m_cache.clear();
    m_current = nullptr;
    m_active  = false;
}
