#include "PlayerSprite.h"

bool PlayerSprite::load(const EnemySpriteConfig& config,
                        const std::string& dissolveShaderPath,
                        const std::string& hitShaderPath) {
    unload();
    if (config.sheetPath.empty()) return false;

    if (!FileExists(config.sheetPath.c_str())) {
        TraceLog(LOG_WARNING, "PlayerSprite: sheet not found: %s",
                 config.sheetPath.c_str());
        return false;
    }

    m_texture = LoadTexture(config.sheetPath.c_str());
    if (m_texture.id == 0) {
        TraceLog(LOG_WARNING, "PlayerSprite: failed to load texture: %s",
                 config.sheetPath.c_str());
        return false;
    }
    SetTextureFilter(m_texture, TEXTURE_FILTER_POINT);

    m_config    = config;
    m_loaded    = true;
    m_frame     = config.idle.startFrame;
    m_elapsedMs = 0.0f;

    if (!dissolveShaderPath.empty()) m_dissolve.load(dissolveShaderPath);
    if (!hitShaderPath.empty())      m_hitEffect.load(hitShaderPath);

    TraceLog(LOG_INFO, "PlayerSprite: loaded %s (%dx%d, %d idle frames)",
             config.sheetPath.c_str(),
             m_texture.width, m_texture.height,
             config.idle.frameCount);
    return true;
}

void PlayerSprite::unload() {
    if (m_loaded && m_texture.id != 0)
        UnloadTexture(m_texture);
    m_texture   = {};
    m_loaded    = false;
    m_frame     = 0;
    m_elapsedMs = 0.0f;
    m_dissolve.unload();
    m_hitEffect.unload();
}

bool PlayerSprite::isLoaded() const { return m_loaded; }

void PlayerSprite::triggerHit()   { m_hitEffect.trigger(3); } // 3 = blue palette
bool PlayerSprite::isHitActive()  const { return m_hitEffect.isActive(); }
void PlayerSprite::triggerDeath() { m_dissolve.triggerDeath(); }

bool PlayerSprite::isDeathDissolveComplete() const {
    if (!m_dissolve.isLoaded()) return true;
    return m_dissolve.isDeathComplete();
}

void PlayerSprite::update(float dt) {
    m_dissolve.update(dt);
    m_hitEffect.update(dt);
    if (!m_loaded) return;

    const AnimClip& clip = m_config.idle;
    if (clip.frameCount <= 0) return;

    m_elapsedMs += dt * 1000.0f;
    while (m_elapsedMs >= clip.frameDurationMs) {
        m_elapsedMs -= clip.frameDurationMs;
        int local = (m_frame - clip.startFrame) + 1;
        if (local >= clip.frameCount) local = 0;
        m_frame = clip.startFrame + local;
    }
}

void PlayerSprite::draw(Rectangle destRect, Color tint) const {
    if (!m_loaded || m_texture.id == 0) return;

    Rectangle src = {
        (float)(m_frame * m_config.frameWidth), 0.0f,
        (float)m_config.frameWidth,
        (float)m_config.frameHeight
    };

    if (m_dissolve.isActive()) {
        // Death dissolve takes priority over hit flash.
        m_dissolve.beginShader({ (float)m_texture.width, (float)m_texture.height });
        DrawTexturePro(m_texture, src, destRect, { 0.0f, 0.0f }, 0.0f, tint);
        m_dissolve.endShader();
    } else if (m_hitEffect.isActive()) {
        m_hitEffect.beginShader();
        DrawTexturePro(m_texture, src, destRect, { 0.0f, 0.0f }, 0.0f, tint);
        m_hitEffect.endShader();
    } else {
        DrawTexturePro(m_texture, src, destRect, { 0.0f, 0.0f }, 0.0f, tint);
    }
}
