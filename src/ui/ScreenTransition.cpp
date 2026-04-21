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
    clearTransitionTexture();
    if (m_fromLoaded && m_fromTexture.id != 0) {
        UnloadTexture(m_fromTexture);
    }
    if (m_toLoaded && m_toTexture.id != 0) {
        UnloadTexture(m_toTexture);
    }
    if (m_loaded && m_shader.id != 0) {
        UnloadShader(m_shader);
    }

    m_shader = {};
    m_loaded = false;
    m_active = false;
    m_needFromCapture = false;
    m_needToCapture = false;
    m_progress = 1.0f;
    m_fromTexture = {};
    m_toTexture = {};
    m_transitionTexture = {};
    m_fromLoaded = false;
    m_toLoaded = false;
    m_transitionLoaded = false;
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

    clearTransitionTexture();
    if (m_fromLoaded && m_fromTexture.id != 0) {
        UnloadTexture(m_fromTexture);
        m_fromTexture = {};
        m_fromLoaded = false;
    }
    if (m_toLoaded && m_toTexture.id != 0) {
        UnloadTexture(m_toTexture);
        m_toTexture = {};
        m_toLoaded = false;
    }

    m_active = true;
    m_needFromCapture = true;
    m_needToCapture = false;
    m_progress = 0.0f;
}

void ScreenTransition::update(float dt) {
    if (!m_active) {
        return;
    }

    m_progress += dt / LayoutConfig::ScreenTransitionDuration;
    if (m_progress >= 1.0f) {
        m_progress = 1.0f;
        m_active = false;
        m_needFromCapture = false;
        m_needToCapture = false;
    }
}

bool ScreenTransition::isActive() const {
    return m_active;
}

void ScreenTransition::beginToCapture() {
    m_needToCapture = true;
}

bool ScreenTransition::needsFromCapture() const {
    return m_needFromCapture;
}

bool ScreenTransition::needsToCapture() const {
    return m_needToCapture;
}

void ScreenTransition::captureFrom(const Texture2D& sourceTexture) {
    replaceSnapshot(m_fromTexture, m_fromLoaded, sourceTexture);
    m_needFromCapture = false;
}

void ScreenTransition::captureTo(const Texture2D& sourceTexture) {
    replaceSnapshot(m_toTexture, m_toLoaded, sourceTexture);
    rebuildTransitionTexture();
    m_needToCapture = false;
}

void ScreenTransition::drawOverlay(int screenWidth, int screenHeight) {
    if (!m_loaded || !m_active || !m_fromLoaded) {
        return;
    }

    if (!m_transitionLoaded) {
        DrawTexturePro(
            m_fromTexture,
            { 0.0f, 0.0f, (float)m_fromTexture.width, (float)m_fromTexture.height },
            { 0.0f, 0.0f, (float)screenWidth, (float)screenHeight },
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
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
        m_transitionTexture,
        { 0.0f, 0.0f, (float)m_transitionTexture.width, (float)m_transitionTexture.height },
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

void ScreenTransition::rebuildTransitionTexture() {
    clearTransitionTexture();
    if (!m_fromLoaded || !m_toLoaded || m_fromTexture.id == 0 || m_toTexture.id == 0) {
        return;
    }

    Image fromImage = LoadImageFromTexture(m_fromTexture);
    Image toImage = LoadImageFromTexture(m_toTexture);
    if (fromImage.data == nullptr || toImage.data == nullptr) {
        if (fromImage.data != nullptr) {
            UnloadImage(fromImage);
        }
        if (toImage.data != nullptr) {
            UnloadImage(toImage);
        }
        return;
    }

    const int combinedWidth = fromImage.width + toImage.width;
    const int combinedHeight = fromImage.height > toImage.height ? fromImage.height : toImage.height;
    Image transitionImage = GenImageColor(combinedWidth, combinedHeight, BLACK);

    ImageDraw(&transitionImage, fromImage,
              { 0.0f, 0.0f, (float)fromImage.width, (float)fromImage.height },
              { 0.0f, 0.0f, (float)fromImage.width, (float)fromImage.height },
              WHITE);
    ImageDraw(&transitionImage, toImage,
              { 0.0f, 0.0f, (float)toImage.width, (float)toImage.height },
              { (float)fromImage.width, 0.0f, (float)toImage.width, (float)toImage.height },
              WHITE);

    m_transitionTexture = LoadTextureFromImage(transitionImage);
    m_transitionLoaded = m_transitionTexture.id != 0;

    UnloadImage(transitionImage);
    UnloadImage(fromImage);
    UnloadImage(toImage);
}

void ScreenTransition::clearTransitionTexture() {
    if (m_transitionLoaded && m_transitionTexture.id != 0) {
        UnloadTexture(m_transitionTexture);
    }
    m_transitionTexture = {};
    m_transitionLoaded = false;
}
