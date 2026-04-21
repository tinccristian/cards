#include "DissolveEffect.h"
#include <algorithm>

bool DissolveEffect::load(const std::string& fragmentShaderPath) {
    unload();
    if (!FileExists(fragmentShaderPath.c_str())) {
        TraceLog(LOG_WARNING, "DissolveEffect: shader not found: %s",
                 fragmentShaderPath.c_str());
        return false;
    }

    m_shader = LoadShader(nullptr, fragmentShaderPath.c_str());
    if (m_shader.id == 0) {
        TraceLog(LOG_WARNING, "DissolveEffect: failed to compile shader: %s",
                 fragmentShaderPath.c_str());
        return false;
    }

    m_locSensitivity = GetShaderLocation(m_shader, "sensitivity");
    m_locTexSize     = GetShaderLocation(m_shader, "texSize");
    m_locTime        = GetShaderLocation(m_shader, "time");

    float zero = 0.0f;
    SetShaderValue(m_shader, m_locSensitivity, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locTime,        &zero, SHADER_UNIFORM_FLOAT);

    m_loaded = true;
    m_state  = State::Idle;
    TraceLog(LOG_INFO, "DissolveEffect: loaded %s", fragmentShaderPath.c_str());
    return true;
}

void DissolveEffect::unload() {
    if (m_loaded && m_shader.id != 0)
        UnloadShader(m_shader);
    m_shader      = {};
    m_loaded      = false;
    m_state       = State::Idle;
    m_sensitivity = 0.0f;
    m_time        = 0.0f;
    m_deathTimer  = 0.0f;
    m_locSensitivity = m_locTexSize = m_locTime = -1;
}

bool DissolveEffect::isLoaded()        const { return m_loaded; }
bool DissolveEffect::isActive()        const { return m_state != State::Idle; }
bool DissolveEffect::isDeathComplete() const { return m_state == State::Done; }

void DissolveEffect::triggerDeath() {
    if (!m_loaded) return;
    if (m_state == State::Dying || m_state == State::Done) return;
    m_state       = State::Dying;
    m_deathTimer  = 0.0f;
    m_sensitivity = 0.0f;
}

void DissolveEffect::update(float dt) {
    if (!m_loaded) return;

    m_time += dt;
    SetShaderValue(m_shader, m_locTime, &m_time, SHADER_UNIFORM_FLOAT);

    switch (m_state) {
    case State::Idle:
        break;

    case State::Dying: {
        m_deathTimer += dt;
        m_sensitivity = std::min(m_deathTimer / DeathDuration, 1.0f);
        if (m_deathTimer >= DeathDuration) {
            m_sensitivity = 1.0f;
            m_state       = State::Done;
        }
        SetShaderValue(m_shader, m_locSensitivity, &m_sensitivity, SHADER_UNIFORM_FLOAT);
        break;
    }

    case State::Done:
        break;
    }
}

void DissolveEffect::beginShader(Vector2 texSize) const {
    if (!m_loaded) return;
    SetShaderValue(m_shader, m_locTexSize, &texSize, SHADER_UNIFORM_VEC2);
    BeginShaderMode(m_shader);
}

void DissolveEffect::endShader() const {
    if (!m_loaded) return;
    EndShaderMode();
}
