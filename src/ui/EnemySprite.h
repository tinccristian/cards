#pragma once

#include "content/EnemySpriteConfig.h"
#include "DissolveEffect.h"
#include "HitEffect.h"
#include "raylib.h"
#include <string>

enum class EnemyAnimState {
    Idle,
    Attack,
    Death
};

// Renders an enemy's animated sprite from a horizontal sprite sheet.
//
// Usage pattern (each frame):
//   1. Call setState() whenever the game state demands a new animation.
//   2. Call update(dt) to advance the timer.
//   3. Call draw(destRect) to render the current frame.
//
// Non-looping animations (attack, death) freeze on their last frame once
// complete; isDone() returns true so the caller knows to transition away.
class EnemySprite {
public:
    // Load the texture and shaders. shaderPaths may be empty — effects are skipped silently.
    bool load(const EnemySpriteConfig& config,
              const std::string& dissolveShaderPath = "",
              const std::string& hitShaderPath      = "");

    // Release the GPU texture.  Safe to call even if not loaded.
    void unload();

    bool isLoaded() const;

    // Switch animation.  Resets frame index and elapsed time when the state
    // actually changes.
    void setState(EnemyAnimState newState);
    EnemyAnimState getState() const;

    // True once a non-looping animation has finished playing through once.
    bool isDone() const;

    // Trigger a brief hit-flash (always fires, even on a killing blow).
    void triggerHit();

    // True while the hit flash is still playing.
    bool isHitActive() const;

    // Start the permanent death dissolve (call once isDone() is true after death anim).
    void triggerDeathDissolve();

    // True once the death dissolve has fully completed (or if no shader loaded).
    bool isDeathDissolveComplete() const;

    // Advance the animation clock.  dt is in seconds (e.g. GetFrameTime()).
    void update(float dt);

    // Blit the current frame scaled to fill destRect.
    void draw(Rectangle destRect, Color tint = WHITE) const;

private:
    const AnimClip& currentClip() const;

    EnemySpriteConfig m_config;
    Texture2D         m_texture   = {};
    bool              m_loaded    = false;
    EnemyAnimState    m_state     = EnemyAnimState::Idle;
    int               m_frame     = 0;     // absolute frame index into the sheet
    float             m_elapsedMs = 0.0f;
    bool              m_done      = false; // set when a non-looping clip finishes
    DissolveEffect    m_dissolve;
    HitEffect         m_hitEffect;
};
