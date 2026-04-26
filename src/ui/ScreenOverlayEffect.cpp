#include "ScreenOverlayEffect.h"

#include "raylib.h"

#include <algorithm>

bool ScreenOverlayEffect::load(const char* fragmentShaderPath) {
    m_shader = LoadShader(nullptr, fragmentShaderPath);
    m_loaded = (m_shader.id != 0);
    if (m_loaded) {
        m_locElapsed    = GetShaderLocation(m_shader, "elapsed");
        m_locFade       = GetShaderLocation(m_shader, "fade");
        m_locResolution = GetShaderLocation(m_shader, "resolution");
    }
    return m_loaded;
}

void ScreenOverlayEffect::unload() {
    if (m_loaded) {
        UnloadShader(m_shader);
        m_shader = {};
        m_loaded = false;
        m_active = false;
    }
}

bool ScreenOverlayEffect::isLoaded() const { return m_loaded; }

void ScreenOverlayEffect::trigger(float durationSecs) {
    m_duration = durationSecs;
    m_elapsed  = 0.0f;
    m_active   = true;
}

void ScreenOverlayEffect::update(float dt) {
    if (!m_active) return;
    m_elapsed += dt;
    if (m_elapsed >= m_duration) {
        m_active = false;
    }
}

bool  ScreenOverlayEffect::isActive()  const { return m_active; }

float ScreenOverlayEffect::progress() const {
    if (m_duration <= 0.0f) return 1.0f;
    return std::min(m_elapsed / m_duration, 1.0f);
}

void ScreenOverlayEffect::draw() const {
    if (!m_loaded || !m_active) return;

    const float fade        = 1.0f - progress();
    const float res[2]      = { (float)GetScreenWidth(), (float)GetScreenHeight() };

    SetShaderValue(m_shader, m_locElapsed,    &m_elapsed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locFade,       &fade,      SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locResolution,  res,       SHADER_UNIFORM_VEC2);

    BeginShaderMode(m_shader);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
}
