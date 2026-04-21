#pragma once

#include "raylib.h"

#include <string>

class ScreenTransition {
public:
    bool load(const std::string& fragmentShaderPath);
    void unload();
    bool isLoaded() const;

    void start();
    void update(float dt);
    bool isActive() const;

    bool needsFromCapture() const;
    bool needsToCapture() const;
    void captureFrom(const Texture2D& sourceTexture);
    void beginToCapture();
    void captureTo(const Texture2D& sourceTexture);

    void drawOverlay(int screenWidth, int screenHeight);

private:
    void replaceSnapshot(Texture2D& destination, bool& loadedFlag, const Texture2D& sourceTexture);
    void rebuildTransitionTexture();
    void clearTransitionTexture();

    Shader m_shader = {};
    bool   m_loaded = false;
    bool   m_active = false;
    bool   m_needFromCapture = false;
    bool   m_needToCapture = false;
    float  m_progress = 1.0f;

    Texture2D m_fromTexture = {};
    Texture2D m_toTexture = {};
    Texture2D m_transitionTexture = {};
    bool      m_fromLoaded = false;
    bool      m_toLoaded = false;
    bool      m_transitionLoaded = false;

    int m_locProgress = -1;
    int m_locHexSize = -1;
    int m_locRandomness = -1;
    int m_locEdgeSoftness = -1;
    int m_locTransitionSpeed = -1;
};
