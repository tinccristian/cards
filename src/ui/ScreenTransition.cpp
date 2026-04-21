#include "ScreenTransition.h"

#include "config/Defines.h"

bool ScreenTransition::load(const std::string& fragmentShaderPath) {
    unload();

    if (!FileExists(fragmentShaderPath.c_str())) {
        TraceLog(LOG_WARNING, "ScreenTransition: shader not found: %s", fragmentShaderPath.c_str());
        return false;
    }

    m_shader = LoadShader(nullptr, fragmentShaderPath.c_str());
    if (m_shader.id == 0) {
        TraceLog(LOG_WARNING, "ScreenTransition: failed to compile shader: %s", fragmentShaderPath.c_str());
        return false;
    }

    m_locProgress = GetShaderLocation(m_shader, "progress");
    m_locHexSize = GetShaderLocation(m_shader, "hex_size");
    m_locRandomness = GetShaderLocation(m_shader, "randomness");
    m_locEdgeSoftness = GetShaderLocation(m_shader, "edge_softness");
    m_locTransitionSpeed = GetShaderLocation(m_shader, "transition_speed");

    m_loaded = true;
    return true;
}

void ScreenTransition::unload() {
    clearSnapshot();
    if (m_loaded && m_shader.id != 0) {
        UnloadShader(m_shader);
    }

    m_shader = {};
    m_loaded = false;
    m_active = false;
    m_needFromCapture = false;
    m_progress = 1.0f;
    m_locProgress = -1;
    m_locHexSize = -1;
    m_locRandomness = -1;
    m_locEdgeSoftness = -1;
    m_locTransitionSpeed = -1;
}

bool ScreenTransition::isLoaded() const {
    return m_loaded;
}

void ScreenTransition::start() {
    if (!m_loaded) {
        return;
    }

    clearSnapshot();
    m_active = true;
    m_needFromCapture = true;
    m_progress = 0.0f;
}

void ScreenTransition::update(float dt) {
    if (!m_active || m_needFromCapture) {
        return;
    }

    m_progress += dt / LayoutConfig::ScreenTransitionDuration;
    if (m_progress >= 1.0f) {
        m_progress = 1.0f;
        m_active = false;
        m_needFromCapture = false;
    }
}

bool ScreenTransition::isActive() const {
    return m_active;
}

bool ScreenTransition::blocksInteraction() const {
    return m_active && m_needFromCapture;
}

bool ScreenTransition::needsFromCapture() const {
    return m_needFromCapture;
}

void ScreenTransition::captureFrom(const Texture2D& sourceTexture) {
    replaceSnapshot(m_fromTexture, m_fromLoaded, sourceTexture);
    m_needFromCapture = false;
}

void ScreenTransition::drawOverlay(int screenWidth, int screenHeight) {
    if (!m_loaded || !m_active || !m_fromLoaded) {
        return;
    }

    const float hexSize = LayoutConfig::ScreenTransitionHexSize;
    const float randomness = LayoutConfig::ScreenTransitionRandomness;
    const float edgeSoftness = LayoutConfig::ScreenTransitionEdgeSoftness;
    const float transitionSpeed = LayoutConfig::ScreenTransitionSpeed;

    SetShaderValue(m_shader, m_locProgress, &m_progress, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locHexSize, &hexSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locRandomness, &randomness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locEdgeSoftness, &edgeSoftness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader, m_locTransitionSpeed, &transitionSpeed, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(m_shader);
    DrawTexturePro(
        m_fromTexture,
        { 0.0f, 0.0f, (float)m_fromTexture.width, (float)m_fromTexture.height },
        { 0.0f, 0.0f, (float)screenWidth, (float)screenHeight },
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );
    EndShaderMode();
}

void ScreenTransition::replaceSnapshot(Texture2D& destination, bool& loadedFlag, const Texture2D& sourceTexture) {
    if (loadedFlag && destination.id != 0) {
        UnloadTexture(destination);
        destination = {};
        loadedFlag = false;
    }

    Image image = LoadImageFromTexture(sourceTexture);
    ImageFlipVertical(&image);
    destination = LoadTextureFromImage(image);
    UnloadImage(image);

    loadedFlag = destination.id != 0;
}

void ScreenTransition::clearSnapshot() {
    if (m_fromLoaded && m_fromTexture.id != 0) {
        UnloadTexture(m_fromTexture);
    }
    m_fromTexture = {};
    m_fromLoaded = false;
}
