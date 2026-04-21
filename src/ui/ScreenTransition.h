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
    bool blocksInteraction() const;

    bool needsFromCapture() const;
    void captureFrom(const Texture2D& sourceTexture);

    void drawOverlay(int screenWidth, int screenHeight);

private:
    void replaceSnapshot(Texture2D& destination, bool& loadedFlag, const Texture2D& sourceTexture);
    void clearSnapshot();

    Shader m_shader = {};
    bool   m_loaded = false;
    bool   m_active = false;
    bool   m_needFromCapture = false;
    float  m_progress = 1.0f;

    Texture2D m_fromTexture = {};
    bool      m_fromLoaded = false;

    int m_locProgress = -1;
    int m_locHexSize = -1;
    int m_locRandomness = -1;
    int m_locEdgeSoftness = -1;
    int m_locTransitionSpeed = -1;
};
