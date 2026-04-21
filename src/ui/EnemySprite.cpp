#include "EnemySprite.h"

bool EnemySprite::load(const EnemySpriteConfig& config,
                       const std::string& dissolveShaderPath,
                       const std::string& hitShaderPath) {
    unload();

    if (config.sheetPath.empty()) {
        return false;
    }
    if (!FileExists(config.sheetPath.c_str())) {
        TraceLog(LOG_WARNING, "EnemySprite: sheet not found: %s", config.sheetPath.c_str());
        return false;
    }

    m_texture = LoadTexture(config.sheetPath.c_str());
    if (m_texture.id == 0) {
        TraceLog(LOG_WARNING, "EnemySprite: failed to load texture: %s", config.sheetPath.c_str());
        return false;
    }

    // Pixel-art sprites: keep them crisp even when scaled up.
    SetTextureFilter(m_texture, TEXTURE_FILTER_POINT);

    m_config    = config;
    m_loaded    = true;
    m_state     = EnemyAnimState::Idle;
    m_frame     = config.idle.startFrame;
    m_elapsedMs = 0.0f;
    m_done      = false;

    if (!dissolveShaderPath.empty()) m_dissolve.load(dissolveShaderPath);
    if (!hitShaderPath.empty())     m_hitEffect.load(hitShaderPath);

    TraceLog(LOG_INFO, "EnemySprite: loaded %s (%dx%d, %d frames wide)",
             config.sheetPath.c_str(),
             m_texture.width, m_texture.height,
             m_texture.width / config.frameWidth);

    return true;
}

void EnemySprite::unload() {
    if (m_loaded && m_texture.id != 0) {
        UnloadTexture(m_texture);
    }
    m_texture   = {};
    m_loaded    = false;
    m_frame     = 0;
    m_elapsedMs = 0.0f;
    m_done      = false;
    m_state     = EnemyAnimState::Idle;
    m_dissolve.unload();
    m_hitEffect.unload();
}

bool EnemySprite::isLoaded() const {
    return m_loaded;
}

void EnemySprite::setState(EnemyAnimState newState) {
    if (newState == m_state) return;
    m_state     = newState;
    m_frame     = currentClip().startFrame;
    m_elapsedMs = 0.0f;
    m_done      = false;
}

EnemyAnimState EnemySprite::getState() const {
    return m_state;
}

bool EnemySprite::isDone() const {
    return m_done;
}

void EnemySprite::triggerHit()          { m_hitEffect.trigger(0);   } // 0 = red palette
bool EnemySprite::isHitActive() const   { return m_hitEffect.isActive(); }
void EnemySprite::triggerDeathDissolve(){ m_dissolve.triggerDeath(); }

bool EnemySprite::isDeathDissolveComplete() const {
    if (!m_dissolve.isLoaded()) return true; // no shader → don't block transition
    return m_dissolve.isDeathComplete();
}

void EnemySprite::update(float dt) {
    m_dissolve.update(dt);
    m_hitEffect.update(dt);
    if (!m_loaded || m_done) return;

    const AnimClip& clip = currentClip();
    // A clip with 0 frames has no animation; signal done immediately.
    if (clip.frameCount <= 0) {
        m_done = true;
        return;
    }

    m_elapsedMs += dt * 1000.0f;

    // Consume as many frame ticks as have elapsed (handles very low frame rates).
    while (m_elapsedMs >= clip.frameDurationMs) {
        m_elapsedMs -= clip.frameDurationMs;

        int localFrame = (m_frame - clip.startFrame) + 1;
        if (localFrame >= clip.frameCount) {
            if (clip.looping) {
                localFrame = 0;                 // wrap back to first frame
            } else {
                localFrame = clip.frameCount - 1; // freeze on last frame
                m_done = true;
                m_elapsedMs = 0.0f;
                m_frame = clip.startFrame + localFrame;
                return;
            }
        }
        m_frame = clip.startFrame + localFrame;
    }
}

void EnemySprite::draw(Rectangle destRect, Color tint) const {
    if (!m_loaded || m_texture.id == 0) return;

    Rectangle src = {
        (float)(m_frame * m_config.frameWidth),
        0.0f,
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

const AnimClip& EnemySprite::currentClip() const {
    switch (m_state) {
    case EnemyAnimState::Attack: return m_config.attack;
    case EnemyAnimState::Death:  return m_config.death;
    default:                     return m_config.idle;
    }
}
