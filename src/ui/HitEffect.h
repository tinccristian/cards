#pragma once

#include "raylib.h"
#include <string>

// Colour-cycling hit-flash shader effect.
//
// Drives three uniforms over time:
//   start_frame   — fixed per-instance palette offset (0 = reds, 3 = blues)
//   current_frame — advances every 50 ms to cycle through the palette
//   mix_ratio     — 1.0 at peak, fades to 0.0 as the effect ends
//
// Usage each frame:
//   update(dt);
//   if (isActive()) { beginShader(); DrawTexturePro(...); endShader(); }
class HitEffect {
public:
    bool load(const std::string& fragmentShaderPath);
    void unload();
    bool isLoaded() const;

    // Start the flash. startFrame selects the palette region:
    //   0 = red/dark-red/black (enemy taking damage)
    //   3 = blue/dark-blue/black (player taking damage)
    void trigger(int startFrame = 0);

    // Advance timers and push uniforms. dt in seconds.
    void update(float dt);

    // True while the flash animation is running.
    bool isActive() const;

    void beginShader() const;
    void endShader()   const;

private:
    Shader m_shader       = {};
    bool   m_loaded       = false;
    bool   m_active       = false;
    int    m_startFrame   = 0;
    int    m_currentFrame = 0;
    float  m_timer        = 0.0f;
    float  m_frameTimer   = 0.0f;

    int m_locStartFrame   = -1;
    int m_locCurrentFrame = -1;
    int m_locMixRatio     = -1;

    static constexpr float Duration      = 0.40f; // total flash duration
    static constexpr float FrameInterval = 0.05f; // palette advance rate
    static constexpr float FadeStart     = 0.25f; // when mix_ratio starts falling
};
