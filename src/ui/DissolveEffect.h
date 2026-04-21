#pragma once

#include "raylib.h"
#include <string>

// Pixel-dissolve + wind-displacement shader effect (death only).
//
// Sensitivity rises from 0 → 1 over DeathDuration seconds, then freezes.
// Use isDeathComplete() to know when the entity has fully dissolved.
//
// Usage each frame:
//   update(dt);
//   if (isActive()) {
//       beginShader(texSize);
//       DrawTexturePro(...);
//       endShader();
//   }
class DissolveEffect {
public:
    // Load the fragment shader from file. Returns false on failure;
    // the effect is silently skipped when not loaded.
    bool load(const std::string& fragmentShaderPath);
    void unload();
    bool isLoaded() const;

    // Start the permanent death dissolve (no-op if already dying/done).
    void triggerDeath();

    // Advance the effect clock. dt is in seconds.
    void update(float dt);

    // True while the shader should be applied (Dying or Done).
    bool isActive() const;

    // True once the death dissolve has fully completed.
    bool isDeathComplete() const;

    // Wrap the sprite draw call with these when isActive().
    // texSize: full texture dimensions in pixels (width, height).
    void beginShader(Vector2 texSize) const;
    void endShader()                  const;

private:
    enum class State { Idle, Dying, Done };

    Shader m_shader      = {};
    bool   m_loaded      = false;
    State  m_state       = State::Idle;
    float  m_sensitivity = 0.0f;
    float  m_time        = 0.0f;
    float  m_deathTimer  = 0.0f;

    int m_locSensitivity = -1;
    int m_locTexSize     = -1;
    int m_locTime        = -1;

    static constexpr float DeathDuration = 0.9f;
};
