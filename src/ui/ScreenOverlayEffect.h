#pragma once

#include "raylib.h"

// Generic timed fullscreen overlay driven by a fragment shader.
// Pattern mirrors HitEffect / ScreenTransition:
//   load(path) → trigger(duration) → update(dt) each frame → draw()
class ScreenOverlayEffect {
public:
    bool load(const char* fragmentShaderPath);
    void unload();
    bool isLoaded() const;

    void  trigger(float durationSecs);
    void  update(float dt);
    bool  isActive() const;
    float progress() const; // 0.0 at start → 1.0 at end

    void draw() const;

private:
    Shader m_shader   = {};
    bool   m_loaded   = false;
    bool   m_active   = false;
    float  m_duration = 0.0f;
    float  m_elapsed  = 0.0f;

    int m_locElapsed    = -1;
    int m_locFade       = -1;
    int m_locResolution = -1;
};
