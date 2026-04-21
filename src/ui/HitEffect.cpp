#include "HitEffect.h"
#include <algorithm>

bool HitEffect::load(const std::string& fragmentShaderPath) {
    unload();
    if (!FileExists(fragmentShaderPath.c_str())) {
        TraceLog(LOG_WARNING, "HitEffect: shader not found: %s", fragmentShaderPath.c_str());
        return false;
    }
    m_shader = LoadShader(nullptr, fragmentShaderPath.c_str());
    if (m_shader.id == 0) {
        TraceLog(LOG_WARNING, "HitEffect: failed to compile shader: %s", fragmentShaderPath.c_str());
        return false;
    }
    m_locStartFrame   = GetShaderLocation(m_shader, "start_frame");
    m_locCurrentFrame = GetShaderLocation(m_shader, "current_frame");
    m_locMixRatio     = GetShaderLocation(m_shader, "mix_ratio");
    m_loaded = true;
    return true;
}

void HitEffect::unload() {
    if (m_loaded && m_shader.id != 0)
        UnloadShader(m_shader);
    m_shader       = {};
    m_loaded       = false;
    m_active       = false;
    m_startFrame   = 0;
    m_currentFrame = 0;
    m_timer        = 0.0f;
    m_frameTimer   = 0.0f;
    m_locStartFrame = m_locCurrentFrame = m_locMixRatio = -1;
}

bool HitEffect::isLoaded() const { return m_loaded; }
bool HitEffect::isActive() const { return m_loaded && m_active; }

void HitEffect::trigger(int startFrame) {
    if (!m_loaded) return;
    m_active       = true;
    m_startFrame   = startFrame;
    m_currentFrame = 0;
    m_timer        = 0.0f;
    m_frameTimer   = 0.0f;
}

void HitEffect::update(float dt) {
    if (!m_loaded || !m_active) return;

    m_timer      += dt;
    m_frameTimer += dt;

    while (m_frameTimer >= FrameInterval) {
        m_frameTimer -= FrameInterval;
        m_currentFrame++;
    }

    if (m_timer >= Duration) {
        m_active = false;
        return;
    }

    float mixRatio = 1.0f;
    if (m_timer >= FadeStart)
        mixRatio = 1.0f - (m_timer - FadeStart) / (Duration - FadeStart);

    SetShaderValue(m_shader, m_locStartFrame,   &m_startFrame,   SHADER_UNIFORM_INT);
    SetShaderValue(m_shader, m_locCurrentFrame, &m_currentFrame, SHADER_UNIFORM_INT);
    SetShaderValue(m_shader, m_locMixRatio,     &mixRatio,       SHADER_UNIFORM_FLOAT);
}

void HitEffect::beginShader() const {
    if (!m_loaded) return;
    BeginShaderMode(m_shader);
}

void HitEffect::endShader() const {
    if (!m_loaded) return;
    EndShaderMode();
}
