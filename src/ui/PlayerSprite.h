#pragma once

#include "content/EnemySpriteConfig.h"
#include "DissolveEffect.h"
#include "HitEffect.h"
#include "raylib.h"
#include <string>

// Renders the player's animated sprite (single looping idle animation)
// with optional dissolve hit-flash and death dissolve effects.
class PlayerSprite {
public:
    // Load texture and shaders. Paths may be empty — effects are skipped silently.
    bool load(const EnemySpriteConfig& config,
              const std::string& dissolveShaderPath,
              const std::string& hitShaderPath = "");
    void unload();
    bool isLoaded() const;

    // Trigger a brief hit-flash (always fires, even on a killing blow).
    void triggerHit();

    // True while the hit flash is still playing.
    bool isHitActive() const;

    // Start the permanent death dissolve.
    void triggerDeath();

    // Advance animation and effect clocks. dt is in seconds.
    void update(float dt);

    // Blit the current frame scaled to fill destRect.
    void draw(Rectangle destRect, Color tint = WHITE) const;

    // True once the death dissolve has fully completed (or if no shader loaded).
    bool isDeathDissolveComplete() const;

private:
    EnemySpriteConfig m_config;
    Texture2D         m_texture   = {};
    bool              m_loaded    = false;
    int               m_frame     = 0;
    float             m_elapsedMs = 0.0f;
    DissolveEffect    m_dissolve;
    HitEffect         m_hitEffect;
};
