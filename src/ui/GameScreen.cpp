#include "GameScreen.h"

#include "config/Defines.h"
#include "content/EnemySpriteConfig.h"
#include "ui/FontUtils.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>

#define DrawText UiFont::drawText
#define MeasureText UiFont::measureText

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

namespace {

float easeOutBack(float t);

std::vector<Card> cardsRemovedFromDiscard(const std::vector<Card>& previous,
                                          const std::vector<Card>& current) {
    std::vector<Card> removed;
    std::vector<bool> used(current.size(), false);

    for (const Card& oldCard : previous) {
        int matchIndex = -1;
        for (int index = 0; index < static_cast<int>(current.size()); ++index) {
            if (!used[index] && current[index].getId() == oldCard.getId()) {
                matchIndex = index;
                break;
            }
        }

        if (matchIndex >= 0) {
            used[matchIndex] = true;
        } else {
            removed.push_back(oldCard);
        }
    }

    return removed;
}

void drawPixelSegment(Vector2 from, Vector2 to, float step, float pixelSize, Color color) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.0f) {
        return;
    }

    const int count = std::max(1, (int)std::floor(length / std::max(1.0f, step)));
    for (int i = 0; i <= count; ++i) {
        const float t = (float)i / (float)count;
        const int x = (int)std::round(from.x + dx * t - pixelSize * 0.5f);
        const int y = (int)std::round(from.y + dy * t - pixelSize * 0.5f);
        DrawRectangle(x, y, (int)pixelSize, (int)pixelSize, color);
    }
}

void drawPixelConnection(Vector2 from, Vector2 to, float step, float pixelSize, Color color) {
    const Vector2 midA = { from.x, std::round((from.y + to.y) * 0.5f) };
    const Vector2 midB = { to.x, midA.y };
    const Color shadow = ColorAlpha(BLACK, 0.55f);
    const float shadowSize = pixelSize + 2.0f;
    drawPixelSegment({ from.x + 1.0f, from.y + 1.0f }, { midA.x + 1.0f, midA.y + 1.0f }, step, shadowSize, shadow);
    drawPixelSegment({ midA.x + 1.0f, midA.y + 1.0f }, { midB.x + 1.0f, midB.y + 1.0f }, step, shadowSize, shadow);
    drawPixelSegment({ midB.x + 1.0f, midB.y + 1.0f }, { to.x + 1.0f, to.y + 1.0f }, step, shadowSize, shadow);
    drawPixelSegment(from, midA, step, pixelSize, color);
    drawPixelSegment(midA, midB, step, pixelSize, color);
    drawPixelSegment(midB, to, step, pixelSize, color);
}

void drawTextureCover(Texture2D texture, int width, int height, Color tint) {
    if (texture.id == 0 || width <= 0 || height <= 0) {
        return;
    }

    const float screenAspect = (float)width / (float)height;
    const float textureAspect = (float)texture.width / (float)texture.height;
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

    if (textureAspect > screenAspect) {
        source.width = texture.height * screenAspect;
        source.x = ((float)texture.width - source.width) * 0.5f;
    } else {
        source.height = texture.width / screenAspect;
        source.y = ((float)texture.height - source.height) * 0.5f;
    }

    const Rectangle dest = { 0.0f, 0.0f, (float)width, (float)height };
    DrawTexturePro(texture, source, dest, { 0.0f, 0.0f }, 0.0f, tint);
}

// Parse an EnemySpriteConfig from a JSON file with a "sprite" section.
// Returns a default (no-sprite) config on any error.
EnemySpriteConfig loadSpriteConfigFromJson(const std::string& path) {
    EnemySpriteConfig cfg;
    std::ifstream file(path);
    if (!file.is_open()) return cfg;

    nlohmann::json root;
    try { file >> root; } catch (...) { return cfg; }

    if (!root.contains("sprite") || !root["sprite"].is_object()) return cfg;
    const auto& s = root["sprite"];

    cfg.sheetPath       = s.value("sheet",       "");
    cfg.backgroundPath  = s.value("background",  "");
    cfg.frameWidth      = s.value("frameWidth",  80);
    cfg.frameHeight     = s.value("frameHeight", 80);

    auto parseClip = [&s](const char* key, AnimClip& clip) {
        if (!s.contains(key) || !s[key].is_object()) return;
        const auto& c = s[key];
        clip.startFrame      = c.value("startFrame",      clip.startFrame);
        clip.frameCount      = c.value("frameCount",      clip.frameCount);
        clip.frameDurationMs = c.value("frameDurationMs", clip.frameDurationMs);
        clip.looping         = c.value("looping",         clip.looping);
    };
    parseClip("idle",   cfg.idle);
    parseClip("attack", cfg.attack);
    parseClip("death",  cfg.death);
    return cfg;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GameScreen::GameScreen(int screenWidth, int screenHeight, CardAudio* cardAudio)
    : m_width(screenWidth)
    , m_height(screenHeight)
    , m_cardAudio(cardAudio)
{}

void GameScreen::setTimeScale(float scale) {
    m_timeScale = std::max(0.01f, scale);
}

void GameScreen::showNextTurnVignette() {
    m_turnVignetteTimer = LayoutConfig::TurnVignetteDuration;
}

void GameScreen::addDamageNumber(DamageNumberTarget target,
                                 int value,
                                 DamageNumberStyle style,
                                 int hitIndex,
                                 float delaySecs) {
    if (value <= 0) {
        return;
    }

    FloatingDamageNumber number;
    number.target = target;
    number.value = value;
    number.style = style;
    number.age = 0.0f;
    number.delaySecs = delaySecs;
    number.xOffset = (hitIndex % 2 == 0 ? -1.0f : 1.0f) * scalef(LayoutConfig::DamageNumberSpawnJitterX)
        * (1.0f + 0.25f * (float)(hitIndex / 2));
    number.yOffset = -(float)hitIndex * scalef(LayoutConfig::DamageNumberStackGap);
    m_damageNumbers.push_back(number);
}

void GameScreen::addDamageNumbers(DamageNumberTarget target,
                                  const std::vector<DamageBreakdown>& events,
                                  int baseHitIndex,
                                  float baseDelaySecs,
                                  float delayStepSecs) {
    int hitIndex = baseHitIndex;
    float delaySecs = baseDelaySecs;
    for (const DamageBreakdown& event : events) {
        if (event.blocked > 0) {
            addDamageNumber(target, event.blocked, DamageNumberStyle::Block, hitIndex++, delaySecs);
            delaySecs += delayStepSecs;
        }
        if (event.health > 0) {
            addDamageNumber(target, event.health, DamageNumberStyle::Health, hitIndex++, delaySecs);
            delaySecs += delayStepSecs;
        }
    }
}

void GameScreen::notifyCardPlayRejected(int handIndex) {
    m_rejectedHandIndex = handIndex;
    m_rejectedHandTimer = LayoutConfig::InvalidCardShakeDuration;
    if (handIndex >= 0 && handIndex < static_cast<int>(m_handVisualOrder.size())) {
        m_pendingHandExitTargets.erase(m_handVisualOrder[handIndex]);
        m_pendingHandExitDelays.erase(m_handVisualOrder[handIndex]);
    }
}

void GameScreen::animateHandToDiscardPile() {
    m_visuallyDiscardedHandCards.clear();
    const Rectangle target = discardPileRect();
    for (int index = 0; index < static_cast<int>(m_handVisualOrder.size()); ++index) {
        const std::string& key = m_handVisualOrder[index];
        auto stateIt = m_handCardMotion.find(key);
        auto cardIt = m_handCardMotionCards.find(key);
        if (stateIt == m_handCardMotion.end() || !stateIt->second.initialized
            || cardIt == m_handCardMotionCards.end()) {
            continue;
        }

        const Rectangle start = {
            stateIt->second.bounds.x + (stateIt->second.bounds.width - target.width) * 0.5f,
            stateIt->second.bounds.y + (stateIt->second.bounds.height - target.height) * 0.5f,
            target.width,
            target.height
        };
        addCardMotionGhost(cardIt->second,
                           start,
                           target,
                           stateIt->second.rotation,
                           0.0f,
                           LayoutConfig::ShuffleCardMoveDuration,
                           false,
                           static_cast<float>(index) * LayoutConfig::HandDiscardStagger);
        m_visuallyDiscardedHandCards.push_back(cardIt->second);
    }

    clearHandMotionState();
}

void GameScreen::animateDiscardToDrawPile(const std::vector<Card>& cards, float baseDelaySecs) {
    const Rectangle from = discardPileRect();
    const Rectangle to = drawPileRect();
    for (int index = 0; index < static_cast<int>(cards.size()); ++index) {
        addCardMotionGhost(cards[index],
                           from,
                           to,
                           0.0f,
                           0.0f,
                           LayoutConfig::ShuffleCardMoveDuration,
                           false,
                           baseDelaySecs + static_cast<float>(index) * LayoutConfig::HandDiscardStagger);
    }
    m_lastObservedDiscardPile.clear();
}

void GameScreen::resetCombatEffects() {
    m_turnVignetteTimer = 0.0f;
    m_enemyDeathPresentationTimer = 0.0f;
    m_playerDeathPresentationTimer = 0.0f;
    m_enemyDeathPresentationStarted = false;
    m_playerDeathPresentationStarted = false;
    m_damageNumbers.clear();
    m_hoveredCardIndex = -1;
    m_hoverProgress = 0.0f;
    m_hoverProgressIndex = -1;
    m_draggedCardIndex = -1;
    m_cardHoverProgress.clear();
    clearHandMotionState();
    m_cardMotionGhosts.clear();
    m_lastObservedDiscardPile.clear();
    m_visuallyDiscardedHandCards.clear();
    m_handMotionDuration = LayoutConfig::HandRelayoutDuration;
    m_lastObservedTurnNumber = -1;
    m_rejectedHandIndex = -1;
    m_rejectedHandTimer = 0.0f;
    m_dragVelocityTilt = 0.0f;
    m_dragTiltVelocity = 0.0f;
    m_lastDragMouse = { 0.0f, 0.0f };
}

// Screen-specific overlays and menu/pause/options implementations live in
// src/ui/screens/ so this file can stay focused on combat/map rendering and the
// shared UI helpers that those screens depend on.

// ---------------------------------------------------------------------------
// Pile widget rects (used both for drawing and click detection in main)
// ---------------------------------------------------------------------------

Rectangle GameScreen::drawPileRect() const {
    const float pileWidth = (float)scalei(LayoutConfig::PileWidgetWidth);
    const float pileHeight = (float)scalei(LayoutConfig::PileWidgetHeight);
    const float marginX = m_width * LayoutConfig::PileSideMarginPercent;
    const float y = m_height - pileHeight - (m_height * LayoutConfig::PileBottomMarginPercent);
    return { marginX, y, pileWidth, pileHeight };
}

Rectangle GameScreen::discardPileRect() const {
    const float pileWidth = (float)scalei(LayoutConfig::PileWidgetWidth);
    const float pileHeight = (float)scalei(LayoutConfig::PileWidgetHeight);
    const float marginX = m_width * LayoutConfig::PileSideMarginPercent;
    const float y = m_height - pileHeight - (m_height * LayoutConfig::PileBottomMarginPercent);
    return { (float)(m_width - marginX - pileWidth), y, pileWidth, pileHeight };
}

// ---------------------------------------------------------------------------
// Main Menu
// ---------------------------------------------------------------------------

MenuAction GameScreen::drawMenu(bool allowInteraction) {
    syncWindowSize();

    const char* title   = "Medical Deckbuilder";
    const int   titleSz = scalei(LayoutConfig::MenuTitleFontSize);
    int titleW = MeasureText(title, titleSz);
    DrawText(title, (m_width - titleW) / 2, m_height / 5, titleSz, Colors::text_primary);

    const int btnW = scalei(LayoutConfig::MenuButtonWidth);
    const int btnH = scalei(LayoutConfig::MenuButtonHeight);
    const int gap = scalei(LayoutConfig::MenuButtonGap);
    const int startY  = m_height / 2 - btnH - gap / 2;
    const int centerX = (m_width - btnW) / 2;

    Rectangle rects[3] = {
        { (float)centerX, (float)startY,                 (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + btnH + gap),  (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + 2*(btnH+gap)),(float)btnW, (float)btnH },
    };
    const char* labels[3] = { "New Game", "Options", "Quit" };

    MenuAction clicked = MenuAction::None;
    for (int i = 0; i < 3; ++i) {
        bool hovered = mouseOver(rects[i]);
        drawButton(rects[i], labels[i], hovered);
        if (allowInteraction && hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = static_cast<MenuAction>(i);
        }
    }
    return clicked;
}

int GameScreen::drawMapScreen(const MapData& mapData,
                              const MapRunState& runState,
                              bool allowInteraction) {
    syncWindowSize();
    ensureMapTextureLoaded(mapData.texturePath);
    ensureMapMarkerTexturesLoaded();

    if (!m_mapTextureLoaded || m_mapTexture.id == 0) {
        const char* errorText = "Map texture missing";
        const int errorFontSize = scalei(LayoutConfig::MapTitleFontSize);
        const int errorWidth = MeasureText(errorText, errorFontSize);
        DrawText(errorText,
                 (m_width - errorWidth) / 2,
                 m_height / 2 - errorFontSize / 2,
                 errorFontSize,
                 Colors::damage_color);
        return -1;
    }

    Rectangle mapRect = mapTextureRect();

    const float wheelMove = allowInteraction ? GetMouseWheelMove() : 0.0f;
    if (std::fabs(wheelMove) > 0.0f) {
        m_mapScrollOffset = clampedMapOffset(
            m_mapScrollOffset + wheelMove * scalef(LayoutConfig::MapScrollWheelStep));
    }

    if (allowInteraction && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)
        && CheckCollisionPointRec(GetMousePosition(), mapRect)) {
        m_mapDragging = true;
        m_mapDragStartMouseY = (float)GetMouseY();
        m_mapDragStartOffset = m_mapScrollOffset;
    }

    bool releasedWithoutDrag = false;
    if (m_mapDragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            const float dragDelta = (float)GetMouseY() - m_mapDragStartMouseY;
            m_mapScrollOffset = clampedMapOffset(m_mapDragStartOffset + dragDelta);
            mapRect = mapTextureRect();
        } else {
            releasedWithoutDrag = std::fabs((float)GetMouseY() - m_mapDragStartMouseY)
                <= scalef(LayoutConfig::MapDragThreshold);
            m_mapDragging = false;
        }
    }

    DrawTexturePro(
        m_mapTexture,
        { 0.0f, 0.0f, (float)m_mapTexture.width, (float)m_mapTexture.height },
        mapRect,
        { 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    std::vector<Vector2> nodePositions;
    nodePositions.reserve(mapData.nodes.size());
    for (const auto& node : mapData.nodes) {
        nodePositions.push_back({
            mapRect.x + (node.x / mapData.sourceWidth) * mapRect.width,
            mapRect.y + (node.y / mapData.sourceHeight) * mapRect.height
        });
    }

    const float markerSize = (float)scalei(LayoutConfig::MapMarkerSize);
    const float markerHitSize = markerSize * LayoutConfig::MapMarkerHoverScale;
    const float connectionStep = scalef(LayoutConfig::MapConnectionPixelStep);
    const float connectionPixel = scalef(LayoutConfig::MapConnectionPixelSize);
    for (const auto& connection : mapData.connections) {
        const int fromIndex = mapData.findNodeIndex(connection.fromId);
        const int toIndex = mapData.findNodeIndex(connection.toId);
        if (fromIndex < 0 || toIndex < 0) {
            continue;
        }

        const bool pathUnlocked = runState.isCompleted(fromIndex) && runState.isUnlocked(toIndex);
        const Color lineColor = pathUnlocked
            ? Colors::text_primary
            : ColorAlpha(Colors::card_border, 0.35f);
        drawPixelConnection(nodePositions[fromIndex], nodePositions[toIndex], connectionStep, connectionPixel, lineColor);
    }

    int hoveredNodeIndex = -1;
    const Vector2 mouse = GetMousePosition();
    for (int index = 0; index < static_cast<int>(mapData.nodes.size()); ++index) {
        if (!allowInteraction || !runState.canEnterNode(mapData, index)) {
            continue;
        }
        const Rectangle markerHitBounds = {
            nodePositions[index].x - markerHitSize * 0.5f,
            nodePositions[index].y - markerHitSize * 0.5f,
            markerHitSize,
            markerHitSize
        };
        if (CheckCollisionPointRec(mouse, markerHitBounds)) {
            hoveredNodeIndex = index;
            break;
        }
    }

    const float dt = GetFrameTime();
    if (hoveredNodeIndex != m_mapHoveredNodeIndex) {
        m_mapHoveredNodeIndex = hoveredNodeIndex;
        m_mapHoverProgress = 0.0f;
    } else if (hoveredNodeIndex >= 0) {
        m_mapHoverProgress = std::min(1.0f, m_mapHoverProgress + dt * 8.0f);
    } else {
        m_mapHoverProgress = 0.0f;
    }

    for (int index = 0; index < static_cast<int>(mapData.nodes.size()); ++index) {
        const auto* nodeType = mapData.findNodeType(mapData.nodes[index].typeId);
        if (nodeType == nullptr) {
            continue;
        }

        const bool completed = runState.isCompleted(index);
        const bool canEnter = allowInteraction && runState.canEnterNode(mapData, index);
        const bool eventNode = nodeType->kind == MapNodeKind::Event;
        const bool hovered = canEnter && hoveredNodeIndex == index;

        const Texture2D& marker = eventNode
            ? (completed ? m_mapEventDoneMarker : m_mapEventMarker)
            : (completed ? m_mapBattleDoneMarker : m_mapBattleMarker);
        if (marker.id == 0) {
            const Rectangle markerHitBounds = {
                nodePositions[index].x - markerHitSize * 0.5f,
                nodePositions[index].y - markerHitSize * 0.5f,
                markerHitSize,
                markerHitSize
            };
            DrawRectangleRec(markerHitBounds, ColorAlpha(nodeType->fillColor, hovered ? 0.95f : 0.60f));
        } else {
            const float hoverAmount = hovered ? easeOutBack(m_mapHoverProgress) : 0.0f;
            const float drawScale = 1.0f + (LayoutConfig::MapMarkerHoverScale - 1.0f) * hoverAmount;
            const float drawSize = std::round(markerSize * drawScale);
            const float bounceY = -std::sin(hoverAmount * PI) * scalef(LayoutConfig::MapMarkerHoverBounce);
            const Rectangle dst = {
                std::round(nodePositions[index].x - drawSize * 0.5f),
                std::round(nodePositions[index].y - drawSize * 0.5f + bounceY),
                drawSize,
                drawSize
            };
            DrawTexturePro(marker,
                           { 0.0f, 0.0f, (float)marker.width, (float)marker.height },
                           dst,
                           { 0.0f, 0.0f },
                           0.0f,
                           WHITE);
        }
    }

    if (allowInteraction && releasedWithoutDrag && hoveredNodeIndex >= 0) {
        return hoveredNodeIndex;
    }

    return -1;
}

void GameScreen::setCharacterPositions(const MapCharacterPositions& positions) {
    m_characterPositions = positions;
}

void GameScreen::resetMapView() {
    m_mapScrollOffset = 0.0f;
    m_mapDragging = false;
    m_mapViewInitialized = false;
    m_mapHoveredNodeIndex = -1;
    m_mapHoverProgress = 0.0f;
    m_mapDragStartMouseY = 0.0f;
    m_mapDragStartOffset = 0.0f;
}

Rectangle GameScreen::debugMapTextureRect() const {
    return mapTextureRect();
}

Vector2 GameScreen::debugMapSourceToScreen(const MapData& mapData, Vector2 sourcePoint) const {
    const Rectangle mapRect = mapTextureRect();
    if (mapData.sourceWidth <= 0.0f || mapData.sourceHeight <= 0.0f) {
        return { mapRect.x, mapRect.y };
    }
    return {
        mapRect.x + (sourcePoint.x / mapData.sourceWidth) * mapRect.width,
        mapRect.y + (sourcePoint.y / mapData.sourceHeight) * mapRect.height
    };
}

Vector2 GameScreen::debugMapScreenToSource(const MapData& mapData, Vector2 screenPoint) const {
    const Rectangle mapRect = mapTextureRect();
    if (mapRect.width <= 0.0f || mapRect.height <= 0.0f) {
        return { 0.0f, 0.0f };
    }
    return {
        std::clamp((screenPoint.x - mapRect.x) / mapRect.width, 0.0f, 1.0f) * mapData.sourceWidth,
        std::clamp((screenPoint.y - mapRect.y) / mapRect.height, 0.0f, 1.0f) * mapData.sourceHeight
    };
}

void GameScreen::drawDebugCardFace(Rectangle rect, const Card& card) const {
    drawCardFace(rect, card, true, 0.0f, -1, -1, true);
}

void GameScreen::drawDebugCardFace(Rectangle rect,
                                   const Card& card,
                                   const CardFaceCache::FaceLayout& layout) const {
    const int width = std::max(1, (int)std::lround(rect.width));
    const int height = std::max(1, (int)std::lround(rect.height));
    const std::string key = card.getId()
        + "|" + card.getDisplayName()
        + "|" + card.getDisplayDescription()
        + "|" + std::to_string(width)
        + "|" + std::to_string(height)
        + "|" + std::to_string(layout.manaLeft)
        + "," + std::to_string(layout.manaTop)
        + "," + std::to_string(layout.manaRight)
        + "," + std::to_string(layout.manaBottom)
        + "|" + std::to_string(layout.artLeft)
        + "," + std::to_string(layout.artTop)
        + "," + std::to_string(layout.artRight)
        + "," + std::to_string(layout.artBottom)
        + "|" + std::to_string(layout.nameLeft)
        + "," + std::to_string(layout.nameTop)
        + "," + std::to_string(layout.nameRight)
        + "," + std::to_string(layout.nameBottom)
        + "|" + std::to_string(layout.descriptionLeft)
        + "," + std::to_string(layout.descriptionTop)
        + "," + std::to_string(layout.descriptionRight)
        + "," + std::to_string(layout.descriptionBottom);
    if (key != m_debugCardPreviewKey) {
        if (m_debugCardPreviewTexture.id != 0) {
            UnloadTexture(m_debugCardPreviewTexture);
            m_debugCardPreviewTexture = {};
        }
        m_debugCardPreviewTexture = m_cardFaceCache.buildPreviewTexture(
            card,
            width,
            height,
            std::to_string(card.getCost()),
            layout);
        m_debugCardPreviewKey = key;
    }
    if (m_debugCardPreviewTexture.id == 0) {
        return;
    }
    DrawTexturePro(m_debugCardPreviewTexture,
                   { 0.0f, 0.0f, (float)m_debugCardPreviewTexture.width, (float)m_debugCardPreviewTexture.height },
                   rect,
                   { 0.0f, 0.0f },
                   0.0f,
                   WHITE);
}

// ---------------------------------------------------------------------------
// Arch offset helper
// ---------------------------------------------------------------------------

float GameScreen::archOffset(int i, int n) {
    if (n <= 1) return 0.0f;
    float t      = (float)i / (float)(n - 1);
    float dist   = t - 0.5f;
    return -LayoutConfig::HandArchHeight * (1.0f - 4.0f * dist * dist);
}

namespace {
float snapToPixel(float value) {
    return std::round(value);
}

Rectangle snapRect(Rectangle rect) {
    rect.x = snapToPixel(rect.x);
    rect.y = snapToPixel(rect.y);
    rect.width = snapToPixel(rect.width);
    rect.height = snapToPixel(rect.height);
    return rect;
}

int quantizeTextureSize(int size) {
    constexpr int step = 8;
    return std::max(step, ((std::max(1, size) + step / 2) / step) * step);
}

float handTValue(int index, int count) {
    if (count <= 1) {
        return 0.5f;
    }
    return (float)index / (float)(count - 1);
}

float easeOutCubic(float t) {
    const float inv = 1.0f - std::clamp(t, 0.0f, 1.0f);
    return 1.0f - inv * inv * inv;
}

float easeInOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    const float u = -2.0f * t + 2.0f;
    return 1.0f - (u * u * u) * 0.5f;
}

float easeOutBack(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

float lerpValue(float a, float b, float t) {
    return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
}

Rectangle lerpRect(Rectangle from, Rectangle to, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        lerpValue(from.x, to.x, t),
        lerpValue(from.y, to.y, t),
        lerpValue(from.width, to.width, t),
        lerpValue(from.height, to.height, t)
    };
}

void drawTexturePerspective(Texture2D texture, Rectangle dest, float rotationDegrees, float perspectiveProgress) {
    const float clamped = std::clamp(perspectiveProgress, 0.0f, 1.0f);
    const Vector2 mouse = GetMousePosition();
    const float centerX = dest.x + dest.width * 0.5f;
    const float centerY = dest.y + dest.height * 0.5f;
    const float nx = std::clamp((mouse.x - centerX) / std::max(1.0f, dest.width * 0.5f), -1.0f, 1.0f);
    const float ny = std::clamp((mouse.y - centerY) / std::max(1.0f, dest.height * 0.5f), -1.0f, 1.0f);
    const float yaw = nx * dest.width * 0.022f * clamped;
    const float pitch = ny * dest.height * 0.014f * clamped;
    const float inset = std::sin(clamped * PI) * 0.75f;

    Vector2 corners[4] = {
        { dest.x + yaw + inset, dest.y - pitch + inset },
        { dest.x + dest.width + yaw - inset, dest.y + pitch + inset },
        { dest.x + dest.width - yaw - inset, dest.y + dest.height - pitch - inset },
        { dest.x - yaw + inset, dest.y + dest.height + pitch - inset }
    };

    const Vector2 pivot = { centerX, dest.y + dest.height };
    const float radians = rotationDegrees * DEG2RAD;
    const float s = std::sin(radians);
    const float c = std::cos(radians);
    for (Vector2& point : corners) {
        const float dx = point.x - pivot.x;
        const float dy = point.y - pivot.y;
        point = {
            pivot.x + dx * c - dy * s,
            pivot.y + dx * s + dy * c
        };
    }

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlNormal3f(0.0f, 0.0f, 1.0f);

        rlTexCoord2f(0.0f, 0.0f);
        rlVertex2f(corners[0].x, corners[0].y);

        rlTexCoord2f(0.0f, 1.0f);
        rlVertex2f(corners[3].x, corners[3].y);

        rlTexCoord2f(1.0f, 1.0f);
        rlVertex2f(corners[2].x, corners[2].y);

        rlTexCoord2f(1.0f, 0.0f);
        rlVertex2f(corners[1].x, corners[1].y);
    rlEnd();
    rlSetTexture(0);
}

float rectCenterX(Rectangle rect) {
    return rect.x + rect.width * 0.5f;
}

// Draw text with a 1-pixel black outline (4-direction).
void DrawTextOutlined(const char* text, int x, int y, int fontSize, Color color) {
    DrawText(text, x - 1, y,     fontSize, BLACK);
    DrawText(text, x + 1, y,     fontSize, BLACK);
    DrawText(text, x,     y - 1, fontSize, BLACK);
    DrawText(text, x,     y + 1, fontSize, BLACK);
    DrawText(text, x,     y,     fontSize, color);
}

void DrawTextOutlinedAlpha(const char* text, int x, int y, int fontSize, Color color) {
    const Color outline = { 0, 0, 0, color.a };
    DrawText(text, x - 1, y,     fontSize, outline);
    DrawText(text, x + 1, y,     fontSize, outline);
    DrawText(text, x,     y - 1, fontSize, outline);
    DrawText(text, x,     y + 1, fontSize, outline);
    DrawText(text, x,     y,     fontSize, color);
}

std::string statusTooltipLine(const StatusInstance& status) {
    switch (status.type) {
    case StatusType::Poison:
        return "Poisoned for " + std::to_string(status.duration)
            + " turns. Takes " + std::to_string(status.magnitude)
            + " damage at the end of your turn, then loses 1 poison each turn.";
    case StatusType::BonusManaNextTurn:
        return "Gain +" + std::to_string(status.magnitude) + " mana next turn.";
    case StatusType::NextCardFree:
        return "Your next card costs 0.";
    case StatusType::DamageOnDraw:
        return "Deal " + std::to_string(status.magnitude) + " damage each time you draw a card this turn.";
    case StatusType::SkipTurn:
    case StatusType::Infection:
    case StatusType::Weakness:
    case StatusType::Vulnerable:
        return "Status active for " + std::to_string(status.duration) + " turns.";
    }
    return "";
}

const char* organStatusIconPath(const std::string& organId) {
    if (organId == "pacemaker") {
        return AssetPaths::PACEMAKER_STATUS_ICON;
    }
    if (organId == "synthetic_lung") {
        return AssetPaths::SYNTHETIC_LUNG_STATUS_ICON;
    }
    if (organId == "histamine") {
        return AssetPaths::HISTAMINE_STATUS_ICON;
    }
    return nullptr;
}

std::string organStatusDescription(const Card& organ) {
    std::string description = organ.getDisplayDescription();
    const std::string prefix = "Organ. ";
    if (description.rfind(prefix, 0) == 0) {
        description.erase(0, prefix.size());
    }
    if (description.empty()) {
        description = organ.getDescription();
    }
    return description;
}

std::string organStatusTooltipLine(const Card& organ) {
    return organ.getDisplayName() + ": " + organStatusDescription(organ);
}

std::vector<std::string> wrapTooltipText(const std::string& body, int fontSize, int maxContentW) {
    std::vector<std::string> wrappedLines;
    std::string paragraph;

    const auto wrapParagraph = [&](const std::string& text) {
        if (text.empty()) {
            wrappedLines.push_back("");
            return;
        }

        std::vector<std::string> words;
        std::string word;
        for (char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }

        std::string current;
        for (const std::string& w : words) {
            const std::string candidate = current.empty() ? w : current + " " + w;
            if (MeasureText(candidate.c_str(), fontSize) <= maxContentW) {
                current = candidate;
            } else {
                if (!current.empty()) {
                    wrappedLines.push_back(current);
                }
                current = w;
            }
        }
        if (!current.empty()) {
            wrappedLines.push_back(current);
        }
    };

    for (char c : body) {
        if (c == '\n') {
            wrapParagraph(paragraph);
            paragraph.clear();
        } else {
            paragraph += c;
        }
    }
    wrapParagraph(paragraph);

    return wrappedLines;
}

} // namespace

// ---------------------------------------------------------------------------
// Pile widget
// ---------------------------------------------------------------------------

bool GameScreen::drawPileWidget(Rectangle rect, const std::string& label,
                                int count, Color accentColor) const {
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const float shadowOffset = scalef(LayoutConfig::PileShadowOffset);
    const int labelFontSize = scalei(LayoutConfig::PileLabelFontSize);
    const int labelOffsetY = scalei(LayoutConfig::PileLabelOffsetY);
    const int emptyFontSize = scalei(LayoutConfig::PileEmptyFontSize);
    const int countFontSize = scalei(LayoutConfig::PileCountFontSize);
    const int hintFontSize = scalei(LayoutConfig::PileHintFontSize);
    const int hintOffsetY = scalei(LayoutConfig::PileHintOffsetY);
    bool empty   = (count == 0);
    Color bg     = empty ? Colors::empty_pile_bg : Colors::card_bg;
    Color border = empty ? Colors::text_secondary   : accentColor;

    // Stack shadow (3 offset rectangles behind the widget)
    if (!empty) {
        for (int s = LayoutConfig::PileShadowLayers; s >= 1; --s) {
            Rectangle shadow = { rect.x + s * shadowOffset, rect.y + s * shadowOffset,
                                  rect.width, rect.height };
            DrawRectangleRec(shadow, Color{ (unsigned char)(accentColor.r / 2),
                                            (unsigned char)(accentColor.g / 2),
                                            (unsigned char)(accentColor.b / 2),
                                            (unsigned char)LayoutConfig::PileShadowAlpha });
        }
    }

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, borderThickness, border);

    // Label (top)
    int lw = MeasureText(label.c_str(), labelFontSize);
    DrawText(label.c_str(), (int)rect.x + ((int)rect.width - lw) / 2,
             (int)rect.y + labelOffsetY,
             labelFontSize,
             Colors::text_primary);

    // Count (centre, large)
    std::string countStr = empty ? "EMPTY" : std::to_string(count);
    int cSz  = empty ? emptyFontSize : countFontSize;
    Color cCol = empty ? Colors::text_secondary : accentColor;
    int cw = MeasureText(countStr.c_str(), cSz);
    DrawText(countStr.c_str(), (int)rect.x + ((int)rect.width - cw) / 2,
             (int)rect.y + (int)rect.height / 2 - cSz / 2, cSz, cCol);

    // Hover highlight
    bool hovered = mouseOver(rect);
    if (hovered) {
        DrawRectangleLinesEx(rect, borderThickness + scalef(1.0f), Colors::text_primary);
        // Tooltip hint
        DrawText("Click to view", (int)rect.x, (int)rect.y + (int)rect.height + hintOffsetY,
                 hintFontSize, Colors::text_secondary);
    }

    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ---------------------------------------------------------------------------
// Combat Screen
// ---------------------------------------------------------------------------

int GameScreen::drawCombat(GameState& state, bool& endTurnClicked,
                            bool& drawPileClicked, bool& discardPileClicked,
                            bool allowInteraction) {
    syncWindowSize();

    endTurnClicked     = false;
    drawPileClicked    = false;
    discardPileClicked = false;
    m_pendingTooltip.active = false; // reset each frame; re-populated by hover checks below

    const bool   isPlayerTurn = (state.getTurnPhase() == TurnPhase::PLAYER_TURN);
    const Player& player = state.getPlayer();
    const Enemy&  enemy  = state.getEnemy();
    const Deck&   deck   = player.getDeck();

    const float dt = GetFrameTime() * m_timeScale;
    m_wiggleTime += dt;
    updateTransientEffects(dt);
    const bool turnNumberChanged = m_lastObservedTurnNumber >= 0
        && m_lastObservedTurnNumber != state.getTurnNumber();
    std::vector<Card> shuffledCards = cardsRemovedFromDiscard(m_lastObservedDiscardPile,
                                                              deck.getDiscardPileCards());
    if (turnNumberChanged
        && deck.getDiscardPileCards().empty()
        && deck.getDrawPileSize() > 0) {
        shuffledCards.insert(shuffledCards.end(),
                             m_visuallyDiscardedHandCards.begin(),
                             m_visuallyDiscardedHandCards.end());
    }
    if (!shuffledCards.empty() && deck.getDrawPileSize() > 0) {
        animateDiscardToDrawPile(shuffledCards);
    }
    m_lastObservedDiscardPile = deck.getDiscardPileCards();
    if (turnNumberChanged) {
        m_visuallyDiscardedHandCards.clear();
        for (const std::string& key : m_handVisualOrder) {
            auto stateIt = m_handCardMotion.find(key);
            auto cardIt = m_handCardMotionCards.find(key);
            if (stateIt != m_handCardMotion.end() && stateIt->second.initialized
                && cardIt != m_handCardMotionCards.end()) {
                addCardMotionGhost(cardIt->second,
                                   stateIt->second.bounds,
                                   discardPileRect(),
                                   stateIt->second.rotation,
                                   0.0f,
                                   LayoutConfig::HandCardExitDuration,
                                   false);
            }
        }
        clearHandMotionState();
    }
    m_lastObservedTurnNumber = state.getTurnNumber();
    m_rejectedHandTimer = std::max(0.0f, m_rejectedHandTimer - dt);
    if (m_rejectedHandTimer <= 0.0f) {
        m_rejectedHandIndex = -1;
    }
    {
        const bool isHovering = (m_hoveredCardIndex >= 0 && m_draggedCardIndex < 0);
        if (isHovering) {
            if (m_hoveredCardIndex != m_hoverProgressIndex) {
                m_hoverProgress = 0.0f;
                m_hoverProgressIndex = m_hoveredCardIndex;
            }
            m_hoverProgress = std::min(1.0f, m_hoverProgress + dt * LayoutConfig::HoverAnimSpeed);
        } else {
            m_hoverProgress = std::max(0.0f, m_hoverProgress - dt * LayoutConfig::HoverAnimSpeed);
            if (m_hoverProgress <= 0.0f)
                m_hoverProgressIndex = -1;
        }
    }
    const int turnFontSize = scalei(LayoutConfig::CombatTurnFontSize);
    const int combatLogFontSize = scalei(LayoutConfig::CombatLogFontSize);
    const float spriteSize = scalef(LayoutConfig::EntitySpriteSize);
    const float playerSpriteTop = (float)scalei(m_characterPositions.playerSpriteTop);
    const float enemySpriteTop = (float)scalei(m_characterPositions.enemySpriteTop);
    const float playerCenterX = m_width * m_characterPositions.playerCenterXPercent;
    const float enemyCenterX = m_width * m_characterPositions.enemyCenterXPercent;
    const float playerHudCenterX = m_width * m_characterPositions.playerHudCenterXPercent;
    const float enemyHudCenterX = m_width * m_characterPositions.enemyHudCenterXPercent;
    const float playerHudTop = (float)scalei(m_characterPositions.playerHudTop);
    const float enemyHudTop = (float)scalei(m_characterPositions.enemyHudTop);
    const Rectangle playerSpriteRect = {
        playerCenterX - spriteSize / 2.0f,
        playerSpriteTop,
        spriteSize,
        spriteSize
    };
    const Rectangle enemySpriteRect = {
        enemyCenterX - spriteSize / 2.0f,
        enemySpriteTop,
        spriteSize,
        spriteSize
    };
    const Rectangle playerHudAnchorRect = {
        playerHudCenterX - spriteSize / 2.0f,
        playerHudTop - spriteSize - scalef(LayoutConfig::EntityHudGap),
        spriteSize,
        spriteSize
    };
    const Rectangle enemyHudAnchorRect = {
        enemyHudCenterX - spriteSize / 2.0f,
        enemyHudTop - spriteSize - scalef(LayoutConfig::EntityHudGap),
        spriteSize,
        spriteSize
    };

    const std::string& enemyBackgroundPath = enemy.getSpriteConfig().backgroundPath;
    if (m_loadedEnemyBackgroundPath != enemyBackgroundPath) {
        if (m_enemyBackgroundTexture.id != 0) {
            UnloadTexture(m_enemyBackgroundTexture);
            m_enemyBackgroundTexture = {};
        }
        m_loadedEnemyBackgroundPath = enemyBackgroundPath;
        if (!enemyBackgroundPath.empty() && FileExists(enemyBackgroundPath.c_str())) {
            m_enemyBackgroundTexture = LoadTexture(enemyBackgroundPath.c_str());
            if (m_enemyBackgroundTexture.id != 0) {
                SetTextureFilter(m_enemyBackgroundTexture, TEXTURE_FILTER_POINT);
            }
        }
    }
    if (m_enemyBackgroundTexture.id != 0) {
        drawTextureCover(m_enemyBackgroundTexture, m_width, m_height, WHITE);
    }

    // --- Turn number ---
    std::string turnStr = "Turn " + std::to_string(state.getTurnNumber());
    int turnW = MeasureText(turnStr.c_str(), turnFontSize);
    DrawText(turnStr.c_str(),
             (m_width - turnW) / 2,
             scalei(LayoutConfig::HealthBarHeight),
             turnFontSize,
             Colors::text_secondary);

    // --- Player sprite: load once, draw below player box ---
    if (!m_playerSpriteLoaded) {
        m_playerSpriteLoaded = true;
        EnemySpriteConfig playerCfg = loadSpriteConfigFromJson(AssetPaths::PLAYER_SPRITE);
        m_playerSprite.load(playerCfg, "", AssetPaths::HIT_SHADER);
    }
    {
        // Hit detection for player.
        const int curPlayerHp = player.getHealth();
        if (m_lastPlayerHp >= 0 && curPlayerHp < m_lastPlayerHp)
            m_playerSprite.triggerHit();
        m_lastPlayerHp = curPlayerHp;
        if (!state.isPlayerDefeated()) {
            m_playerDeathPresentationStarted = false;
        }
        if (state.isPlayerDefeated() && !m_playerSprite.isHitActive() && !m_playerDeathPresentationStarted) {
            m_playerDeathPresentationTimer = LayoutConfig::DeathPresentationDuration;
            m_playerDeathPresentationStarted = true;
        }

        m_playerSprite.update(dt);

        if (m_playerSprite.isLoaded()) {
            m_playerSprite.draw(playerSpriteRect,
                                m_playerDeathPresentationTimer > 0.0f ? BLACK : WHITE);
        }
    }

    // --- Enemy sprite: load on enemy change, update + draw ---
    {
        const std::string& sheetPath = enemy.getSpriteConfig().sheetPath;

        // (Re)load when a different sprite sheet is needed.
        if (m_loadedEnemySpritePath != sheetPath) {
            m_enemySprite.unload();
            m_loadedEnemySpritePath = sheetPath;
            m_lastEnemyHp           = -1; // reset for new enemy
            if (!sheetPath.empty()) {
                m_enemySprite.load(enemy.getSpriteConfig(), "", AssetPaths::HIT_SHADER);
            }
        }

        // Hit detection for enemy — trigger even on a killing blow.
        const int curEnemyHp = enemy.getHealth();
        if (m_lastEnemyHp >= 0 && curEnemyHp < m_lastEnemyHp)
            m_enemySprite.triggerHit();
        m_lastEnemyHp = curEnemyHp;

        if (m_enemySprite.isLoaded()) {
            // Drive animation state from game state.
            if (enemy.isDead()) {
                // Wait for hit flash to finish before starting the death animation.
                if (!m_enemySprite.isHitActive()) {
                    if (m_enemySprite.getState() != EnemyAnimState::Death)
                        m_enemySprite.setState(EnemyAnimState::Death);
                    if (!m_enemyDeathPresentationStarted) {
                        m_enemyDeathPresentationTimer = LayoutConfig::DeathPresentationDuration;
                        m_enemyDeathPresentationStarted = true;
                    }
                }
            } else if (m_enemySprite.getState() == EnemyAnimState::Death) {
                // Same enemy re-encountered (new combat) – reset to idle.
                m_enemySprite.setState(EnemyAnimState::Idle);
                m_enemyDeathPresentationStarted = false;
            } else if (!isPlayerTurn) {
                // Enemy turn begins: trigger attack animation once.
                if (m_enemySprite.getState() == EnemyAnimState::Idle)
                    m_enemySprite.setState(EnemyAnimState::Attack);
            } else {
                // Player turn: return to idle after the attack clip finishes.
                if (m_enemySprite.getState() == EnemyAnimState::Attack
                    && m_enemySprite.isDone())
                    m_enemySprite.setState(EnemyAnimState::Idle);
            }

            m_enemySprite.update(dt);
            m_enemySprite.draw(enemySpriteRect,
                               m_enemyDeathPresentationTimer > 0.0f ? BLACK : WHITE);
        }
    }

    drawEntityHud(playerHudAnchorRect, "You",
                  player.getHealth(), player.getMaxHealth(), player.getBlock(),
                  &player.getStatuses(),
                  &player.getActiveOrgans());
    drawEntityHud(enemyHudAnchorRect, enemy.getName(),
                  enemy.getHealth(), enemy.getMaxHealth(), enemy.getEnemyBlock());
    drawIntentIndicator(enemy, enemyHudAnchorRect);
    drawFloatingDamageNumbers(playerSpriteRect, enemySpriteRect);
    drawDeathPresentationOverlay();

    // --- Combat log ---
    const std::string& action = state.getLastAction();
    if (!action.empty()) {
        int aw = MeasureText(action.c_str(), combatLogFontSize);
        DrawText(action.c_str(), (m_width - aw) / 2, scalei(LayoutConfig::CombatLogTop), combatLogFontSize,
                 Colors::text_secondary);
    }

    // --- Draw pile widget (left side) ---
    drawManaHud(player);
    if (allowInteraction
        && drawPileWidget(drawPileRect(), "DRAW", deck.getDrawPileSize(), Colors::draw_pile_accent))
        drawPileClicked = true;
    else
        drawPileWidget(drawPileRect(), "DRAW", deck.getDrawPileSize(), Colors::draw_pile_accent);

    // --- Discard pile widget (right side) ---
    if (allowInteraction
        && drawPileWidget(discardPileRect(), "DISCARD", deck.getDiscardPileSize(), Colors::discard_pile_accent))
        discardPileClicked = true;
    else
        drawPileWidget(discardPileRect(), "DISCARD", deck.getDiscardPileSize(), Colors::discard_pile_accent);

    // --- ENEMY_TURN overlay ---
    if (!isPlayerTurn) {
        const char* msg = "Enemy is acting...";
        const int actingFontSize = scalei(LayoutConfig::PileViewerTitleSize);
        int msgW = MeasureText(msg, actingFontSize);
        DrawText(msg, (m_width - msgW) / 2, m_height / 2 - scalei(LayoutConfig::TooltipTextSize),
                 actingFontSize, Colors::damage_color);
        updateAndDrawCardMotionGhosts(dt);
        return -1;
    }

    // --- End Turn button ---
    Rectangle etBtn = endTurnButtonRect();
    bool etHovered = mouseOver(etBtn);
    drawButton(etBtn, "End Turn", etHovered);
    if (allowInteraction && etHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) endTurnClicked = true;

    // -------------------------------------------------------------------
    // Hand layout: arch + wiggle
    // -------------------------------------------------------------------
    const auto& hand = player.getHand();
    const int   n    = (int)hand.size();

    if (n == 0) {
        const std::vector<HandLayoutCard> emptyLayout;
        syncHandVisualKeys(hand, emptyLayout);
        updateAndDrawCardMotionGhosts(dt);
        return -1;
    }

    std::vector<HandLayoutCard> targetLayout = buildHandLayout(n, m_draggedCardIndex);
    if (allowInteraction && m_draggedCardIndex >= 0) {
        const int targetIndex = handInsertIndexFromMouseX(targetLayout, (float)GetMouseX());
        std::vector<int> drawOrder;
        drawOrder.reserve(n);
        for (int index = 0; index < n; ++index) {
            if (index != m_draggedCardIndex) {
                drawOrder.push_back(index);
            }
        }
        const int insertIndex = std::clamp(targetIndex, 0, static_cast<int>(drawOrder.size()));
        drawOrder.insert(drawOrder.begin() + insertIndex, m_draggedCardIndex);
        const std::vector<HandLayoutCard> slotLayout = buildHandLayout(n, -1);
        for (int slot = 0; slot < n; ++slot) {
            targetLayout[drawOrder[slot]] = slotLayout[slot];
        }
    }
    std::vector<HandLayoutCard> layout = animateHandLayout(hand, targetLayout, dt);

    // Determine hovered card
    const int previousHoveredCardIndex = m_hoveredCardIndex;
    m_hoveredCardIndex = -1;
    if (!allowInteraction) {
        m_draggedCardIndex = -1;
    } else if (m_draggedCardIndex < 0) {
        for (int i = 0; i < n; ++i) {
            if (layout[i].visible && mouseOver(layout[i].bounds)) {
                m_hoveredCardIndex = i;
            }
        }
    }

    if (m_cardAudio
        && m_draggedCardIndex < 0
        && m_hoveredCardIndex >= 0
        && m_hoveredCardIndex != previousHoveredCardIndex) {
        m_cardAudio->playHover();
    }

    if (allowInteraction
        && m_draggedCardIndex < 0
        && m_hoveredCardIndex >= 0
        && layout[m_hoveredCardIndex].visible
        && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_draggedCardIndex = m_hoveredCardIndex;
        m_dragGrabOffset = {
            GetMouseX() - layout[m_draggedCardIndex].bounds.x,
            GetMouseY() - layout[m_draggedCardIndex].bounds.y
        };
        m_lastDragMouse = GetMousePosition();
        m_dragVelocityTilt = 0.0f;
        m_dragTiltVelocity = 0.0f;
        targetLayout = buildHandLayout(n, m_draggedCardIndex);
        layout = animateHandLayout(hand, targetLayout, dt);
    }

    int pendingPlayIndex = -1;
    Rectangle pendingPlayRect = {};
    float pendingPlayRotation = 0.0f;

    if (allowInteraction && m_draggedCardIndex >= 0 && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        const int draggedIndex = m_draggedCardIndex;
        const float t = handTValue(draggedIndex, n);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        const Rectangle draggedRect = currentDraggedCardRect(layout[draggedIndex]);
        if (draggedIndex >= 0 && draggedIndex < static_cast<int>(m_handVisualOrder.size())) {
            AnimatedCardState& state = m_handCardMotion[m_handVisualOrder[draggedIndex]];
            state.bounds = draggedRect;
            state.rotation = normalizedOffset * LayoutConfig::HandMaxTiltDegrees * LayoutConfig::HoveredTiltFactor;
            state.initialized = true;
        }
        m_draggedCardIndex = -1;
        m_hoveredCardIndex = -1;
        m_dragVelocityTilt = 0.0f;
        m_dragTiltVelocity = 0.0f;
        targetLayout = buildHandLayout(n, -1);
        m_handMotionDuration = LayoutConfig::HandDragCancelDuration;
        layout = animateHandLayout(hand, targetLayout, dt);
    }

    if (allowInteraction && m_draggedCardIndex >= 0 && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        const int draggedIndex = m_draggedCardIndex;
        const bool releasedInHand = CheckCollisionPointRec(GetMousePosition(), handDropZone());
        const float t = handTValue(draggedIndex, n);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        pendingPlayRect = currentDraggedCardRect(layout[draggedIndex]);
        pendingPlayRotation = normalizedOffset * LayoutConfig::HandMaxTiltDegrees * LayoutConfig::HoveredTiltFactor;
        m_draggedCardIndex = -1;
        m_dragVelocityTilt = 0.0f;
        m_dragTiltVelocity = 0.0f;

        if (releasedInHand) {
            const std::vector<HandLayoutCard> slotLayout = buildHandLayout(n, -1);
            const int targetIndex = handInsertIndexFromMouseX(slotLayout, (float)GetMouseX());
            state.getPlayer().moveCardInHand(draggedIndex, targetIndex);
        } else {
            pendingPlayIndex = draggedIndex;
            if (draggedIndex >= 0 && draggedIndex < static_cast<int>(m_handVisualOrder.size())) {
                m_pendingHandExitTargets[m_handVisualOrder[draggedIndex]] = CardExitTarget::DiscardPile;
                m_pendingHandExitDelays[m_handVisualOrder[draggedIndex]] = LayoutConfig::PlayedCardDiscardDelay;
                AnimatedCardState& state = m_handCardMotion[m_handVisualOrder[draggedIndex]];
                state.bounds = pendingPlayRect;
                state.rotation = pendingPlayRotation;
                state.initialized = true;
            }
        }
    }

    const int curMana = player.getMana();

    // Draw non-hovered cards first
    for (int i = 0; i < n; ++i) {
        if (!layout[i].visible) continue;
        if (i == pendingPlayIndex) continue;
        if (i == m_hoveredCardIndex || i == m_draggedCardIndex) continue;
        drawCardFace(layout[i].bounds,
                     hand[i],
                     layout[i].scaled,
                     layout[i].rotation,
                     curMana,
                     player.getEffectiveCost(hand[i]));
    }

    // Hovered card on top
    if (allowInteraction && m_hoveredCardIndex >= 0 && m_draggedCardIndex < 0 && m_hoveredCardIndex != pendingPlayIndex) {
        int i = m_hoveredCardIndex;
        if (layout[i].visible) {
            Rectangle r = layout[i].bounds;
            drawCardFace(r,
                         hand[i],
                         false,
                         layout[i].rotation,
                         curMana,
                         player.getEffectiveCost(hand[i]),
                         false,
                         m_hoverProgress);

            float tipX = r.x + r.width + scalef(LayoutConfig::TooltipHorizontalGap);
            if (tipX + scalei(LayoutConfig::TooltipWidth) > m_width) {
                tipX = r.x - scalei(LayoutConfig::TooltipWidth) - scalei(LayoutConfig::TooltipScreenMargin);
            }
            drawCardTooltip(hand[i], tipX, r.y, player.getEffectiveCost(hand[i]));
        }

    }

    if (allowInteraction && m_draggedCardIndex >= 0 && layout[m_draggedCardIndex].visible) {
        const int dragIndex = m_draggedCardIndex;
        const float t = handTValue(dragIndex, n);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        Rectangle draggedRect = currentDraggedCardRect(layout[dragIndex]);
        const Vector2 mouse = GetMousePosition();
        const float frameDt = std::max(dt, 0.001f);
        const float velocityX = (mouse.x - m_lastDragMouse.x) / frameDt;
        const float targetTilt = std::clamp(velocityX * 0.018f,
                                            -LayoutConfig::DragVelocityTiltDegrees,
                                            LayoutConfig::DragVelocityTiltDegrees);
        const float acceleration = (targetTilt - m_dragVelocityTilt) * LayoutConfig::DragTiltSpring
            - m_dragTiltVelocity * LayoutConfig::DragTiltDamping;
        m_dragTiltVelocity += acceleration * frameDt;
        m_dragVelocityTilt += m_dragTiltVelocity * frameDt;
        m_lastDragMouse = mouse;
        drawCardFace(draggedRect,
                     hand[dragIndex],
                     false,
                     normalizedOffset * LayoutConfig::HandMaxTiltDegrees * LayoutConfig::HoveredTiltFactor
                        + m_dragVelocityTilt,
                     curMana,
                     player.getEffectiveCost(hand[dragIndex]));
    }

    if (pendingPlayIndex >= 0 && pendingPlayIndex < n) {
        drawCardFace(pendingPlayRect,
                     hand[pendingPlayIndex],
                     false,
                     pendingPlayRotation,
                     curMana,
                     player.getEffectiveCost(hand[pendingPlayIndex]));
    }

    drawTurnVignetteOverlay();
    updateAndDrawCardMotionGhosts(dt);

    // Flush deferred tooltips last so they render above every other element.
    flushTooltip();

    return pendingPlayIndex;
}


bool GameScreen::isEnemyDeathAnimDone() const {
    if (m_enemyDeathPresentationTimer > 0.0f) return false;
    if (!m_enemySprite.isLoaded()) return true;
    if (m_enemySprite.isHitActive()) return false;
    if (m_enemySprite.getState() == EnemyAnimState::Death && !m_enemySprite.isDone()) return false;
    return true;
}

bool GameScreen::isPlayerDeathDissolveComplete() const {
    if (m_playerDeathPresentationTimer > 0.0f) return false;
    if (!m_playerSprite.isLoaded()) return true;
    if (m_playerSprite.isHitActive()) return false;
    return true;
}

void GameScreen::unloadAssets() {
    m_cardFaceCache.unloadAll();
    if (m_debugCardPreviewTexture.id != 0) {
        UnloadTexture(m_debugCardPreviewTexture);
        m_debugCardPreviewTexture = {};
        m_debugCardPreviewKey.clear();
    }
    m_enemySprite.unload();
    m_loadedEnemySpritePath.clear();
    if (m_enemyBackgroundTexture.id != 0) {
        UnloadTexture(m_enemyBackgroundTexture);
        m_enemyBackgroundTexture = {};
        m_loadedEnemyBackgroundPath.clear();
    }
    m_playerSprite.unload();
    m_playerSpriteLoaded = false;
    if (m_intentFloatShaderLoaded && m_intentFloatShader.id != 0) {
        UnloadShader(m_intentFloatShader);
        m_intentFloatShader = {};
        m_intentFloatShaderLoaded = false;
        m_intentFloatTimeLoc = -1;
        m_intentFloatAmpLoc = -1;
        m_intentFloatSpeedLoc = -1;
        m_intentFloatPhaseLoc = -1;
    }
    if (m_blockIconLoaded && m_blockIcon.id != 0) {
        UnloadTexture(m_blockIcon);
        m_blockIcon = {};
        m_blockIconLoaded = false;
    }
    if (m_attackIconLoaded && m_attackIcon.id != 0) {
        UnloadTexture(m_attackIcon);
        m_attackIcon = {};
        m_attackIconLoaded = false;
    }
    if (m_buffIconLoaded && m_buffIcon.id != 0) {
        UnloadTexture(m_buffIcon);
        m_buffIcon = {};
        m_buffIconLoaded = false;
    }
    if (m_debuffIconLoaded && m_debuffIcon.id != 0) {
        UnloadTexture(m_debuffIcon);
        m_debuffIcon = {};
        m_debuffIconLoaded = false;
    }
    if (m_poisonIconLoaded && m_poisonIcon.id != 0) {
        UnloadTexture(m_poisonIcon);
        m_poisonIcon = {};
        m_poisonIconLoaded = false;
    }
    for (auto& [id, texture] : m_organStatusIcons) {
        (void)id;
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
    m_organStatusIcons.clear();
    if (m_mapTextureLoaded && m_mapTexture.id != 0) {
        UnloadTexture(m_mapTexture);
        m_mapTexture = {};
        m_mapTextureLoaded = false;
        m_loadedMapTexturePath.clear();
    }
    if (m_mapMarkersLoaded) {
        if (m_mapBattleMarker.id != 0) {
            UnloadTexture(m_mapBattleMarker);
            m_mapBattleMarker = {};
        }
        if (m_mapBattleDoneMarker.id != 0) {
            UnloadTexture(m_mapBattleDoneMarker);
            m_mapBattleDoneMarker = {};
        }
        if (m_mapEventMarker.id != 0) {
            UnloadTexture(m_mapEventMarker);
            m_mapEventMarker = {};
        }
        if (m_mapEventDoneMarker.id != 0) {
            UnloadTexture(m_mapEventDoneMarker);
            m_mapEventDoneMarker = {};
        }
        m_mapMarkersLoaded = false;
    }
    if (m_noahEventTextureLoaded && m_noahEventTexture.id != 0) {
        UnloadTexture(m_noahEventTexture);
        m_noahEventTexture = {};
        m_noahEventTextureLoaded = false;
    }
}

Rectangle GameScreen::handDropZone() const {
    const float cardHeight = (float)scalei(LayoutConfig::CardHeight);
    const float left = m_width * LayoutConfig::HandLeftBoundPercent;
    const float right = m_width * LayoutConfig::HandRightBoundPercent;
    const float top = m_height - cardHeight
        + cardHeight * LayoutConfig::HandBottomOverflowPercent
        - scalef(LayoutConfig::HandBottomMargin)
        - scalef(LayoutConfig::HandDropZoneTopPadding);
    const float bottom = (float)m_height;
    return { left, top, right - left, bottom - top };
}

Rectangle GameScreen::drawPileButtonRect() const {
    return drawPileRect();
}

Rectangle GameScreen::endTurnButtonRect() const {
    const float buttonWidth = (float)scalei(LayoutConfig::EndTurnButtonWidth);
    const float buttonHeight = (float)scalei(LayoutConfig::EndTurnButtonHeight);
    const Rectangle discardRect = discardPileRect();
    return {
        discardRect.x + (discardRect.width - buttonWidth) / 2.0f,
        discardRect.y - buttonHeight - scalef(LayoutConfig::EndTurnToPileGap),
        buttonWidth,
        buttonHeight
    };
}

std::vector<GameScreen::HandLayoutCard> GameScreen::buildHandLayout(int cardCount, int draggedCardIndex) const {
    std::vector<HandLayoutCard> layout(cardCount);
    if (cardCount <= 0) {
        return layout;
    }

    const float cardWidth = (float)scalei(LayoutConfig::CardWidth);
    const float cardHeight = (float)scalei(LayoutConfig::CardHeight);
    const float handLeftBound  = m_width * LayoutConfig::HandLeftBoundPercent;
    const float handRightBound = m_width * LayoutConfig::HandRightBoundPercent;
    const float handUsableWidth = std::max(cardWidth, handRightBound - handLeftBound);
    const float baseY = m_height - cardHeight + cardHeight * LayoutConfig::HandBottomOverflowPercent - scalef(LayoutConfig::HandBottomMargin);
    float cardSpacing = (cardCount > 1)
        ? (handUsableWidth - cardWidth * cardCount) / (float)(cardCount - 1)
        : 0.0f;
    cardSpacing = std::min(scalef(LayoutConfig::CardGap), cardSpacing);
    const float totalW = cardWidth * cardCount + cardSpacing * (cardCount - 1);
    const float startX = handLeftBound + (handUsableWidth - totalW) / 2.0f;

    const int activeHoverIndex = (draggedCardIndex < 0) ? m_hoverProgressIndex : -1;

    for (int index = 0; index < cardCount; ++index) {
        const float cx_base = startX + index * (cardWidth + cardSpacing);
        const float cy_base = baseY + archOffset(index, cardCount) * uiScale();
        const float t = handTValue(index, cardCount);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        const float idleWiggleRotation = std::sin(
            m_wiggleTime * LayoutConfig::CardIdleWiggleFrequency
            + (float)index * LayoutConfig::CardIdleWigglePhaseStep) * LayoutConfig::CardIdleWiggleDegrees;
        const float restRotation = normalizedOffset * LayoutConfig::HandMaxTiltDegrees + idleWiggleRotation;

        float cx = cx_base;
        float cy = cy_base;
        float rotation = restRotation;

        if (index == activeHoverIndex && m_hoverProgress > 0.0f) {
            const float baseEased = easeInOutCubic(m_hoverProgress);
            const float easedBack = easeOutBack(baseEased);
            const float hp = baseEased + (easedBack - baseEased) * LayoutConfig::CardHoverOvershoot;
            const float restCy = cy_base;
            const float snappedCardHeight = std::round(cardHeight);
            const float hoverCy = std::round((float)m_height - snappedCardHeight);
            cy = restCy + (hoverCy - restCy) * hp;
            rotation = restRotation * (1.0f - hp);
            Rectangle hoverRect = { std::round(cx), cy, cardWidth, snappedCardHeight };
            if (hp >= 0.999f) {
                hoverRect.y = hoverCy;
            } else {
                hoverRect.y = std::round(cy);
            }
            hoverRect.x = std::round(cx);
            hoverRect.width = cardWidth;
            hoverRect.height = snappedCardHeight;
            layout[index] = { hoverRect, rotation, false };
            continue;
        }

        if (m_hoveredCardIndex >= 0 && draggedCardIndex < 0) {
            const int diff = index - m_hoveredCardIndex;
            const float pushProgress = easeInOutCubic(m_hoverProgress);
            if (diff == -1) cx -= scalef(LayoutConfig::NeighborCardShift) * pushProgress;
            else if (diff == 1) cx += scalef(LayoutConfig::NeighborCardShift) * pushProgress;
            else if (diff == -2) cx -= scalef(LayoutConfig::NeighborCardShiftSecond) * pushProgress;
            else if (diff == 2) cx += scalef(LayoutConfig::NeighborCardShiftSecond) * pushProgress;
        }

        if (index == m_rejectedHandIndex && m_rejectedHandTimer > 0.0f) {
            const float tReject = 1.0f - (m_rejectedHandTimer / LayoutConfig::InvalidCardShakeDuration);
            const float fade = 1.0f - tReject;
            cx += std::sin(tReject * PI * 8.0f) * scalef(LayoutConfig::InvalidCardShakePixels) * fade;
        }

        layout[index] = { snapRect({ cx, cy, cardWidth, cardHeight }), rotation, false };
    }

    return layout;
}

std::vector<std::string> GameScreen::syncHandVisualKeys(const std::vector<Card>& hand,
                                                        const std::vector<HandLayoutCard>& targetLayout) {
    const int cardCount = static_cast<int>(hand.size());
    std::vector<std::string> keys(cardCount);
    std::vector<bool> usedPrevious(m_handVisualOrder.size(), false);

    for (int index = 0; index < cardCount; ++index) {
        const std::string& cardId = hand[index].getId();
        int bestPrevious = -1;
        float bestDistance = std::numeric_limits<float>::max();

        for (int previousIndex = 0; previousIndex < static_cast<int>(m_handVisualOrder.size()); ++previousIndex) {
            if (usedPrevious[previousIndex]) {
                continue;
            }
            const std::string& previousKey = m_handVisualOrder[previousIndex];
            if (m_pendingHandExitTargets.find(previousKey) != m_pendingHandExitTargets.end()) {
                continue;
            }
            auto idIt = m_handCardMotionIds.find(previousKey);
            if (idIt == m_handCardMotionIds.end() || idIt->second != cardId) {
                continue;
            }

            float distance = 0.0f;
            auto stateIt = m_handCardMotion.find(previousKey);
            if (stateIt != m_handCardMotion.end() && stateIt->second.initialized) {
                distance = std::fabs(rectCenterX(stateIt->second.bounds) - rectCenterX(targetLayout[index].bounds));
            } else {
                distance = std::fabs(static_cast<float>(previousIndex - index));
            }

            if (distance < bestDistance) {
                bestDistance = distance;
                bestPrevious = previousIndex;
            }
        }

        if (bestPrevious >= 0) {
            keys[index] = m_handVisualOrder[bestPrevious];
            usedPrevious[bestPrevious] = true;
        } else {
            keys[index] = "hand:" + cardId + ":" + std::to_string(m_nextHandVisualId++);
        }
        m_handCardMotionIds[keys[index]] = cardId;
        m_handCardMotionCards.insert_or_assign(keys[index], hand[index]);
    }

    for (auto stateIt = m_handCardMotion.begin(); stateIt != m_handCardMotion.end();) {
        if (std::find(keys.begin(), keys.end(), stateIt->first) == keys.end()) {
            auto cardIt = m_handCardMotionCards.find(stateIt->first);
            if (cardIt != m_handCardMotionCards.end() && stateIt->second.initialized) {
                const auto exitIt = m_pendingHandExitTargets.find(stateIt->first);
                const CardExitTarget exitTarget = exitIt != m_pendingHandExitTargets.end()
                    ? exitIt->second
                    : CardExitTarget::DiscardPile;
                const auto delayIt = m_pendingHandExitDelays.find(stateIt->first);
                const float delay = delayIt != m_pendingHandExitDelays.end()
                    ? delayIt->second
                    : 0.0f;
                const Rectangle pileRect = exitTarget == CardExitTarget::DrawPile
                    ? drawPileRect()
                    : discardPileRect();
                const Rectangle startRect = delay > 0.0f
                    ? stateIt->second.bounds
                    : Rectangle{
                        stateIt->second.bounds.x + (stateIt->second.bounds.width - pileRect.width) * 0.5f,
                        stateIt->second.bounds.y + (stateIt->second.bounds.height - pileRect.height) * 0.5f,
                        pileRect.width,
                        pileRect.height
                    };
                addCardMotionGhost(cardIt->second,
                                   startRect,
                                   pileRect,
                                   stateIt->second.rotation,
                                   0.0f,
                                   delay > 0.0f
                                       ? LayoutConfig::HandCardExitDuration
                                       : LayoutConfig::ShuffleCardMoveDuration,
                                   false,
                                   delay);
            }
            m_handCardMotionIds.erase(stateIt->first);
            m_handCardMotionCards.erase(stateIt->first);
            m_pendingHandExitTargets.erase(stateIt->first);
            m_pendingHandExitDelays.erase(stateIt->first);
            m_handEnterHideTimers.erase(stateIt->first);
            stateIt = m_handCardMotion.erase(stateIt);
        } else {
            ++stateIt;
        }
    }

    m_handVisualOrder = keys;
    return keys;
}

std::vector<GameScreen::HandLayoutCard> GameScreen::animateHandLayout(const std::vector<Card>& hand,
                                                                      const std::vector<HandLayoutCard>& targetLayout,
                                                                      float dt) {
    std::vector<HandLayoutCard> visualLayout = targetLayout;
    if (hand.empty() || targetLayout.empty()) {
        clearHandMotionState();
        m_handMotionDuration = LayoutConfig::HandRelayoutDuration;
        return visualLayout;
    }

    const std::vector<std::string> keys = syncHandVisualKeys(hand, targetLayout);
    const float enterDuration = std::max(LayoutConfig::HandCardEnterDuration,
                                         LayoutConfig::ShuffleCardMoveDuration);
    const float duration = std::max(0.01f, m_handMotionDuration);
    m_handMotionDuration = LayoutConfig::HandRelayoutDuration;
    const float alpha = std::clamp(dt / duration, 0.0f, 1.0f);

    for (int index = 0; index < static_cast<int>(keys.size()); ++index) {
        AnimatedCardState& state = m_handCardMotion[keys[index]];
        const HandLayoutCard& target = targetLayout[index];
        if (!state.initialized) {
            const Rectangle pileRect = drawPileRect();
            const Rectangle startRect = {
                pileRect.x + (pileRect.width - target.bounds.width) * 0.5f,
                pileRect.y + (pileRect.height - target.bounds.height) * 0.5f,
                target.bounds.width,
                target.bounds.height
            };
            addCardMotionGhost(hand[index],
                               startRect,
                               target.bounds,
                               0.0f,
                               target.rotation,
                               enterDuration,
                               false,
                               static_cast<float>(index) * LayoutConfig::HandDiscardStagger);
            m_handEnterHideTimers[keys[index]] = enterDuration
                + static_cast<float>(index) * LayoutConfig::HandDiscardStagger;
            state.bounds = target.bounds;
            state.rotation = target.rotation;
            state.initialized = true;
        } else {
            state.bounds = lerpRect(state.bounds, target.bounds, alpha);
            state.rotation = lerpValue(state.rotation, target.rotation, alpha);
        }

        auto hideIt = m_handEnterHideTimers.find(keys[index]);
        if (hideIt != m_handEnterHideTimers.end()) {
            hideIt->second -= dt;
            if (hideIt->second > 0.0f) {
                visualLayout[index].visible = false;
            } else {
                m_handEnterHideTimers.erase(hideIt);
            }
        }

        visualLayout[index].bounds = snapRect(state.bounds);
        visualLayout[index].rotation = state.rotation;
    }

    return visualLayout;
}

float GameScreen::cardHoverProgress(const std::string& key, bool hovered, float dt) {
    float& progress = m_cardHoverProgress[key];
    const float direction = hovered ? 1.0f : -1.0f;
    progress = std::clamp(progress + direction * dt * LayoutConfig::CardUiHoverAnimSpeed, 0.0f, 1.0f);
    if (!hovered && progress <= 0.0f) {
        m_cardHoverProgress.erase(key);
        return 0.0f;
    }
    return progress;
}

Rectangle GameScreen::applyCardHoverMotion(Rectangle rect, float progress, float finalScale) const {
    if (progress <= 0.0f) {
        return snapRect(rect);
    }

    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const float baseEased = easeInOutCubic(clamped);
    const float easedBack = easeOutBack(baseEased);
    const float eased = baseEased + (easedBack - baseEased) * LayoutConfig::CardHoverOvershoot;
    const float scale = 1.0f + (finalScale - 1.0f) * eased;
    const float squash = std::sin(clamped * PI) * LayoutConfig::CardHoverSquash;
    const float scaledWidth = rect.width * scale * (1.0f + squash);
    const float scaledHeight = rect.height * scale * (1.0f - squash * 0.45f);
    const float bounceY = -std::sin(clamped * PI) * scalef(LayoutConfig::CardHoverBouncePixels);
    Rectangle result = {
        rect.x - (scaledWidth - rect.width) * 0.5f,
        rect.y - (scaledHeight - rect.height) * 0.5f + bounceY,
        scaledWidth,
        scaledHeight
    };
    return snapRect(result);
}

Rectangle GameScreen::currentDraggedCardRect(const HandLayoutCard& layout) const {
    return snapRect({
        GetMouseX() - m_dragGrabOffset.x,
        GetMouseY() - m_dragGrabOffset.y,
        layout.bounds.width,
        layout.bounds.height
    });
}

void GameScreen::addCardMotionGhost(const Card& card,
                                    Rectangle startBounds,
                                    Rectangle targetBounds,
                                    float startRotation,
                                    float targetRotation,
                                    float duration,
                                    bool crispPresentation,
                                    float delay) {
    m_cardMotionGhosts.push_back(CardMotionGhost{
        card,
        snapRect(startBounds),
        snapRect(targetBounds),
        startRotation,
        targetRotation,
        0.0f,
        std::max(0.01f, duration),
        std::max(0.0f, delay),
        { startBounds.x + startBounds.width * 0.5f, startBounds.y + startBounds.height * 0.5f },
        0.0f,
        0.0f,
        crispPresentation
    });
}

void GameScreen::updateAndDrawCardMotionGhosts(float dt) {
    for (auto it = m_cardMotionGhosts.begin(); it != m_cardMotionGhosts.end();) {
        it->age += dt;
        const float activeAge = it->age - it->delay;
        const float t = std::clamp(activeAge / it->duration, 0.0f, 1.0f);
        const float eased = easeInOutCubic(t);
        const Vector2 startCenter = {
            it->startBounds.x + it->startBounds.width * 0.5f,
            it->startBounds.y + it->startBounds.height * 0.5f
        };
        const Vector2 targetCenter = {
            it->targetBounds.x + it->targetBounds.width * 0.5f,
            it->targetBounds.y + it->targetBounds.height * 0.5f
        };
        const float width = activeAge <= 0.0f ? it->startBounds.width : it->targetBounds.width;
        const float height = activeAge <= 0.0f ? it->startBounds.height : it->targetBounds.height;
        const Vector2 center = {
            lerpValue(startCenter.x, targetCenter.x, eased),
            lerpValue(startCenter.y, targetCenter.y, eased)
        };
        Rectangle rect = {
            center.x - width * 0.5f,
            center.y - height * 0.5f,
            width,
            height
        };
        if (activeAge > 0.0f) {
            rect.y -= std::sin(t * PI) * scalef(34.0f);
        }
        const Vector2 movedCenter = {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f
        };
        if (activeAge > 0.0f) {
            const float frameDt = std::max(dt, 0.001f);
            const float velocityX = (movedCenter.x - it->previousCenter.x) / frameDt;
            const float targetVelocityRotation = std::clamp(velocityX * LayoutConfig::MotionGhostTiltFactor,
                                                            -LayoutConfig::MotionGhostTiltDegrees,
                                                            LayoutConfig::MotionGhostTiltDegrees);
            const float acceleration = (targetVelocityRotation - it->velocityRotation) * LayoutConfig::MotionGhostTiltSpring
                - it->rotationVelocity * LayoutConfig::MotionGhostTiltDamping;
            it->rotationVelocity += acceleration * frameDt;
            it->velocityRotation += it->rotationVelocity * frameDt;
        }
        it->previousCenter = movedCenter;
        const float rotation = lerpValue(it->startRotation, it->targetRotation, eased) + it->velocityRotation;
        drawCardFace(snapRect(rect), it->card, false, rotation, -1, -1, it->crispPresentation);

        if (activeAge >= it->duration) {
            it = m_cardMotionGhosts.erase(it);
        } else {
            ++it;
        }
    }
}

void GameScreen::clearHandMotionState() {
    m_handVisualOrder.clear();
    m_handCardMotion.clear();
    m_handCardMotionIds.clear();
    m_handCardMotionCards.clear();
    m_pendingHandExitTargets.clear();
    m_pendingHandExitDelays.clear();
    m_handEnterHideTimers.clear();
}

int GameScreen::handInsertIndexFromMouseX(const std::vector<HandLayoutCard>& layout, float mouseX) const {
    if (layout.empty()) {
        return 0;
    }

    int bestIndex = 0;
    float bestDistance = std::fabs((layout[0].bounds.x + layout[0].bounds.width / 2.0f) - mouseX);

    for (int index = 1; index < static_cast<int>(layout.size()); ++index) {
        const float centerX = layout[index].bounds.x + layout[index].bounds.width / 2.0f;
        const float distance = std::fabs(centerX - mouseX);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = index;
        }
    }

    return bestIndex;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

float GameScreen::uiScale() const {
    const float widthScale = (float)m_width / (float)WindowConfig::Width;
    const float heightScale = (float)m_height / (float)WindowConfig::Height;
    return std::clamp(std::min(widthScale, heightScale), LayoutConfig::UiMinScale, LayoutConfig::UiMaxScale);
}

int GameScreen::scalei(int value) const {
    return std::max(1, (int)std::lround((float)value * uiScale()));
}

float GameScreen::scalef(float value) const {
    return value * uiScale();
}

void GameScreen::ensureMapTextureLoaded(const std::string& texturePath) {
    if (m_mapTextureLoaded && m_loadedMapTexturePath == texturePath) {
        return;
    }

    if (m_mapTextureLoaded && m_mapTexture.id != 0) {
        UnloadTexture(m_mapTexture);
        m_mapTexture = {};
        m_mapTextureLoaded = false;
        m_loadedMapTexturePath.clear();
    }

    m_mapTexture = LoadTexture(texturePath.c_str());
    if (m_mapTexture.id != 0) {
        SetTextureFilter(m_mapTexture, TEXTURE_FILTER_BILINEAR);
        m_mapTextureLoaded = true;
        m_loadedMapTexturePath = texturePath;
    }
}

void GameScreen::ensureMapMarkerTexturesLoaded() {
    if (m_mapMarkersLoaded) {
        return;
    }

    auto loadMarker = [](const char* path) {
        Texture2D texture = {};
        if (!FileExists(path)) {
            TraceLog(LOG_WARNING, "Map marker texture missing: %s", path);
            return texture;
        }
        texture = LoadTexture(path);
        if (texture.id != 0) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        } else {
            TraceLog(LOG_WARNING, "Map marker texture failed to load: %s", path);
        }
        return texture;
    };

    m_mapBattleMarker = loadMarker(AssetPaths::MAP_BATTLE_MARKER);
    m_mapBattleDoneMarker = loadMarker(AssetPaths::MAP_BATTLE_DONE_MARKER);
    m_mapEventMarker = loadMarker(AssetPaths::MAP_EVENT_MARKER);
    m_mapEventDoneMarker = loadMarker(AssetPaths::MAP_EVENT_DONE_MARKER);
    m_mapMarkersLoaded = true;
}

float GameScreen::clampedMapOffset(float offset) const {
    if (!m_mapTextureLoaded || m_mapTexture.id == 0) {
        return 0.0f;
    }

    const float scaleToCover = std::max(
        (float)m_width / (float)m_mapTexture.width,
        (float)m_height / (float)m_mapTexture.height);
    const float drawHeight = (float)m_mapTexture.height * scaleToCover * LayoutConfig::MapZoomFactor;
    const float baseY = ((float)m_height - drawHeight) / 2.0f;
    const float minY = std::min(0.0f, (float)m_height - drawHeight);
    const float maxY = std::max(0.0f, (float)m_height - drawHeight);
    return std::clamp(offset, minY - baseY, maxY - baseY);
}

Rectangle GameScreen::mapTextureRect() const {
    if (!m_mapTextureLoaded || m_mapTexture.id == 0) {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    const float scaleToCover = std::max(
        (float)m_width / (float)m_mapTexture.width,
        (float)m_height / (float)m_mapTexture.height);
    const float drawScale = scaleToCover * LayoutConfig::MapZoomFactor;
    const float drawWidth = (float)m_mapTexture.width * drawScale;
    const float drawHeight = (float)m_mapTexture.height * drawScale;
    const float x = ((float)m_width - drawWidth) / 2.0f;
    const float baseY = ((float)m_height - drawHeight) / 2.0f;

    if (!m_mapViewInitialized) {
        const float minY = std::min(0.0f, (float)m_height - drawHeight);
        const float maxY = std::max(0.0f, (float)m_height - drawHeight);
        const float targetY = maxY + (minY - maxY) * LayoutConfig::MapInitialAnchor;
        const_cast<GameScreen*>(this)->m_mapScrollOffset = targetY - baseY;
        const_cast<GameScreen*>(this)->m_mapViewInitialized = true;
    }

    const float y = baseY + clampedMapOffset(m_mapScrollOffset);
    return snapRect({ x, y, drawWidth, drawHeight });
}

void GameScreen::syncWindowSize() {
    m_width = GetScreenWidth();
    m_height = GetScreenHeight();
}

void GameScreen::drawButton(Rectangle rect, const std::string& text, bool hovered) const {
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const int fontSize = scalei(LayoutConfig::DefaultButtonFontSize);
    DrawRectangleRec(rect, hovered ? Colors::button_hover : Colors::button_bg);
    DrawRectangleLinesEx(rect, borderThickness, Colors::card_border);
    int tw = MeasureText(text.c_str(), fontSize);
    DrawText(text.c_str(),
             (int)rect.x + ((int)rect.width  - tw) / 2,
             (int)rect.y + ((int)rect.height - fontSize) / 2,
             fontSize, Colors::text_primary);
}

int GameScreen::drawStepperRow(float y, const std::string& label, const std::string& value) const {
    const float panelX = (m_width - scalei(LayoutConfig::OverlayPanelWidth)) / 2.0f;
    const float contentLeft = panelX + scalei(LayoutConfig::OverlayContentInset);
    const float contentRight = panelX + scalei(LayoutConfig::OverlayPanelWidth) - scalei(LayoutConfig::OverlayContentInset);
    const float arrowSize = (float)scalei(LayoutConfig::OptionsArrowButtonSize);
    const float valueAreaWidth = (float)scalei(LayoutConfig::OptionsValueAreaWidth);
    const float valueAreaX = contentRight - valueAreaWidth;
    const float textY = y + scalei(LayoutConfig::OptionsRowTextOffsetY);
    const int labelFontSize = scalei(LayoutConfig::OptionsLabelFontSize);
    const int valueFontSize = scalei(LayoutConfig::OptionsValueFontSize);
    const float valuePadding = (float)scalei(LayoutConfig::OptionsValuePadding);

    DrawText(label.c_str(),
             (int)contentLeft,
             (int)textY,
             labelFontSize,
             Colors::text_primary);

    Rectangle leftButton = { valueAreaX, y, arrowSize, arrowSize };
    Rectangle rightButton = { contentRight - arrowSize, y, arrowSize, arrowSize };
    const bool leftHovered = mouseOver(leftButton);
    const bool rightHovered = mouseOver(rightButton);
    drawButton(leftButton, "<", leftHovered);
    drawButton(rightButton, ">", rightHovered);

    const float valueTextX = valueAreaX + arrowSize + valuePadding;
    const float valueTextWidth = valueAreaWidth - arrowSize * 2.0f - valuePadding * 2.0f;
    const int valueWidth = MeasureText(value.c_str(), valueFontSize);
    DrawText(value.c_str(),
             (int)(valueTextX + (valueTextWidth - valueWidth) / 2.0f),
             (int)(textY + scalei(LayoutConfig::OptionsValueTextOffsetY)),
             valueFontSize,
             Colors::text_secondary);

    if (leftHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return -1;
    }
    if (rightHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return 1;
    }
    return 0;
}

bool GameScreen::drawCheckboxRow(float y, const std::string& label, bool checked) const {
    const float panelX = (m_width - scalei(LayoutConfig::OverlayPanelWidth)) / 2.0f;
    const float contentLeft = panelX + scalei(LayoutConfig::OverlayContentInset);
    const float contentRight = panelX + scalei(LayoutConfig::OverlayPanelWidth) - scalei(LayoutConfig::OverlayContentInset);
    const float boxSize = (float)scalei(LayoutConfig::OptionsCheckboxSize);
    const float tickInset = (float)scalei(LayoutConfig::OptionsCheckboxTickInset);
    const float textY = y + tickInset;
    const int labelFontSize = scalei(LayoutConfig::OptionsLabelFontSize);

    DrawText(label.c_str(),
             (int)contentLeft,
             (int)textY,
             labelFontSize,
             Colors::text_primary);

    Rectangle checkbox = {
        contentRight - boxSize,
        y,
        boxSize,
        boxSize
    };
    DrawRectangleRec(checkbox, Colors::card_bg);
    DrawRectangleLinesEx(checkbox, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);
    if (checked) {
        DrawLineEx({ checkbox.x + tickInset, checkbox.y + boxSize / 2.0f },
                   { checkbox.x + boxSize / 2.2f, checkbox.y + boxSize - tickInset },
                   scalef(LayoutConfig::OptionsCheckboxTickThickness),
                   Colors::heal_color);
        DrawLineEx({ checkbox.x + boxSize / 2.2f, checkbox.y + boxSize - tickInset },
                   { checkbox.x + boxSize - tickInset, checkbox.y + tickInset },
                   scalef(LayoutConfig::OptionsCheckboxTickThickness),
                   Colors::heal_color);
    }

    return mouseOver(checkbox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void GameScreen::drawHealthBar(Rectangle bar, float ratio, bool hasBlock) const {
    ratio = std::max(0.0f, std::min(1.0f, ratio));
    DrawRectangleRec(bar, Colors::light_bg);
    Color fill;
    if (hasBlock) {
        fill = Colors::block_color;
    } else {
        fill = (ratio > 0.5f)  ? Colors::heal_color
             : (ratio > 0.25f) ? Colors::damage_color
             :                   Colors::health_color;
    }
    Rectangle filled = { bar.x, bar.y, bar.width * ratio, bar.height };
    DrawRectangleRec(filled, fill);
    DrawRectangleLinesEx(bar, scalef(LayoutConfig::ThinBorderThickness), Colors::card_border);
}

void GameScreen::drawEntityHud(Rectangle spriteRect, const std::string& name,
                               int health, int maxHealth, int block,
                               const StatusCollection* statuses,
                               const std::vector<Card>* activeOrgans) const {
    // Lazy-load block icon (const_cast: texture loading is logically const).
    if (!m_blockIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_blockIconLoaded = true;
        if (FileExists(AssetPaths::BLOCK_ICON)) {
            self->m_blockIcon = LoadTexture(AssetPaths::BLOCK_ICON);
            SetTextureFilter(self->m_blockIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_buffIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_buffIconLoaded = true;
        if (FileExists(AssetPaths::BUFF_ICON)) {
            self->m_buffIcon = LoadTexture(AssetPaths::BUFF_ICON);
            SetTextureFilter(self->m_buffIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_debuffIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_debuffIconLoaded = true;
        if (FileExists(AssetPaths::DEBUFF_ICON)) {
            self->m_debuffIcon = LoadTexture(AssetPaths::DEBUFF_ICON);
            SetTextureFilter(self->m_debuffIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_poisonIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_poisonIconLoaded = true;
        if (FileExists(AssetPaths::POISON_ICON)) {
            self->m_poisonIcon = LoadTexture(AssetPaths::POISON_ICON);
            SetTextureFilter(self->m_poisonIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_intentFloatShaderLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_intentFloatShaderLoaded = true;
        if (FileExists(AssetPaths::INTENT_FLOAT_SHADER)) {
            self->m_intentFloatShader = LoadShader(AssetPaths::INTENT_FLOAT_SHADER, nullptr);
            if (self->m_intentFloatShader.id != 0) {
                self->m_intentFloatTimeLoc = GetShaderLocation(self->m_intentFloatShader, "time");
                self->m_intentFloatAmpLoc = GetShaderLocation(self->m_intentFloatShader, "amplitude");
                self->m_intentFloatSpeedLoc = GetShaderLocation(self->m_intentFloatShader, "speed");
                self->m_intentFloatPhaseLoc = GetShaderLocation(self->m_intentFloatShader, "phase");
            }
        }
    }

    const float hudWidth     = scalef(LayoutConfig::EntityHudWidth);
    const float hudX         = spriteRect.x + (spriteRect.width - hudWidth) / 2.0f;
    const float hudTop       = spriteRect.y + spriteRect.height + scalef(LayoutConfig::EntityHudGap);
    const int   statFontSize = scalei(LayoutConfig::EntityStatFontSize);
    const int   barHeight    = scalei(LayoutConfig::HealthBarHeight);
    const float barY = hudTop;

    // --- Block slot geometry ---
    // Slot is 1.5× the bar height (square), vertically centred on the bar,
    // and overlaps the bar's left edge by half the slot's width.
    const bool  hasBlock  = (block > 0);
    const float slotSize  = barHeight * 1.5f;                // larger square
    const float slotX     = hudX;
    const float slotY     = barY + ((float)barHeight - slotSize) / 2.0f; // vertically centred
    const float barX      = hudX + slotSize / 2.0f;          // bar starts at slot mid-point
    const float barWidth  = std::max(scalef(LayoutConfig::EntityMinHealthBarWidth),
                                     hudWidth - slotSize / 2.0f);

    // --- Health bar (draw first so the slot renders on top) ---
    Rectangle bar = { barX, barY, barWidth, (float)barHeight };
    const float hpRatio = maxHealth > 0 ? (float)health / (float)maxHealth : 0.0f;
    drawHealthBar(bar, hpRatio, hasBlock);

    // HP text centred inside the bar.
    const std::string hpText  = std::to_string(health) + "/" + std::to_string(maxHealth);
    const int         hpTextW = MeasureText(hpText.c_str(), statFontSize);
    DrawText(hpText.c_str(),
             (int)std::round(bar.x + (bar.width  - hpTextW)           / 2.0f),
             (int)std::round(bar.y + (bar.height - statFontSize)       / 2.0f),
             statFontSize,
             WHITE);

    // --- Block slot (drawn on top of the bar's left edge) ---
    if (hasBlock) {
        const Rectangle slot = { slotX, slotY, slotSize, slotSize };
        if (m_blockIcon.id != 0) {
            Rectangle src = { 0, 0, (float)m_blockIcon.width, (float)m_blockIcon.height };
            DrawTexturePro(m_blockIcon, src, slot, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slot, Colors::block_color);
        }
        // Block number centred on the slot.
        const std::string blockStr = std::to_string(block);
        const int bw = MeasureText(blockStr.c_str(), statFontSize);
        DrawTextOutlined(blockStr.c_str(),
                         (int)std::round(slot.x + (slot.width  - bw)          / 2.0f),
                         (int)std::round(slot.y + (slot.height - statFontSize) / 2.0f),
                         statFontSize, WHITE);

        if (mouseOver(slot))
            queueTooltip("Block",
                         "Will block " + std::to_string(block) + " damage",
                         slot);
    }

    const bool hasStatuses = statuses && !statuses->getAll().empty();
    const bool hasOrgans = activeOrgans && !activeOrgans->empty();
    if (!hasStatuses && !hasOrgans) {
        return;
    }

    const float statusY = barY + barHeight + scalef(LayoutConfig::EntityStatusGap);
    float statusX = bar.x;
    const float statusSize = scalei(LayoutConfig::HealthBarHeight) * 2.25f;
    const float statusGap = scalef(LayoutConfig::IntentIconGap * 0.5f);
    const int statusValueSize = scalei(LayoutConfig::EntityStatusValueSize);
    const float bobAmplitude = scalef(LayoutConfig::IntentFloatAmplitude * 0.55f);
    const float bobSpeed = LayoutConfig::IntentFloatSpeed * 0.72f;
    const float timeValue = (float)GetTime();
    const float textBobOffset = std::sin(timeValue * bobSpeed) * bobAmplitude;
    bool hoveredStatusIcon = false;

    const auto beginStatusFloatShader = [&]() {
        if (m_intentFloatShader.id == 0) {
            return false;
        }
        if (m_intentFloatTimeLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatTimeLoc, &timeValue, SHADER_UNIFORM_FLOAT);
        }
        if (m_intentFloatAmpLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatAmpLoc, &bobAmplitude, SHADER_UNIFORM_FLOAT);
        }
        if (m_intentFloatSpeedLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatSpeedLoc, &bobSpeed, SHADER_UNIFORM_FLOAT);
        }
        const float phase = 0.0f;
        if (m_intentFloatPhaseLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatPhaseLoc, &phase, SHADER_UNIFORM_FLOAT);
        }
        BeginShaderMode(m_intentFloatShader);
        return true;
    };

    if (hasStatuses) {
        for (const auto& status : statuses->getAll()) {
            Texture2D icon = {};
            bool iconLoaded = false;
            std::string title;
            std::string tooltip;

            switch (status.type) {
            case StatusType::Poison:
                icon = m_poisonIcon;
                iconLoaded = m_poisonIconLoaded && m_poisonIcon.id != 0;
                title = "Poison";
                tooltip = "Poisoned for " + std::to_string(status.duration)
                    + " turns. Takes " + std::to_string(status.magnitude)
                    + " damage at the end of your turn, then loses 1 poison each turn.";
                break;
            case StatusType::BonusManaNextTurn:
                icon = m_buffIcon;
                iconLoaded = m_buffIconLoaded && m_buffIcon.id != 0;
                title = "Bonus Mana";
                tooltip = "Gain +" + std::to_string(status.magnitude) + " mana next turn.";
                break;
            case StatusType::NextCardFree:
                icon = m_buffIcon;
                iconLoaded = m_buffIconLoaded && m_buffIcon.id != 0;
                title = "Quick Thought";
                tooltip = "Your next card costs 0.";
                break;
            case StatusType::SkipTurn:
            case StatusType::Infection:
            case StatusType::Weakness:
            case StatusType::Vulnerable:
                icon = status.disposition == StatusDisposition::Positive ? m_buffIcon : m_debuffIcon;
                iconLoaded = icon.id != 0;
                title = status.disposition == StatusDisposition::Positive ? "Buff" : "Debuff";
                tooltip = "Status active for " + std::to_string(status.duration) + " turns.";
                break;
            case StatusType::DamageOnDraw:
                icon = m_buffIcon;
                iconLoaded = m_buffIconLoaded && m_buffIcon.id != 0;
                title = "Cytotoxic Rush";
                tooltip = "Each card you draw this turn deals " + std::to_string(status.magnitude) + " damage.";
                break;
            }

            const Rectangle statusRect = { statusX, statusY, statusSize, statusSize };
            const bool shaderActive = beginStatusFloatShader();
            if (iconLoaded) {
                const Rectangle src = { 0, 0, (float)icon.width, (float)icon.height };
                DrawTexturePro(icon, src, statusRect, { 0, 0 }, 0.0f, WHITE);
            } else {
                DrawRectangleRec(statusRect,
                    status.disposition == StatusDisposition::Positive ? Colors::heal_color : Colors::damage_color);
            }
            if (shaderActive) {
                EndShaderMode();
            }

            const std::string valueText = std::to_string(status.magnitude);
            const int valueWidth = MeasureText(valueText.c_str(), statusValueSize);
            DrawTextOutlined(valueText.c_str(),
                             (int)std::round(statusRect.x + (statusRect.width - valueWidth) / 2.0f),
                             (int)std::round(statusRect.y + textBobOffset + statusRect.height - statusValueSize),
                             statusValueSize,
                             WHITE);

            if (mouseOver(statusRect)) {
                queueTooltip(title, tooltip, statusRect);
                hoveredStatusIcon = true;
            }

            statusX += statusSize + statusGap;
        }
    }

    if (hasOrgans) {
        for (const Card& organ : *activeOrgans) {
            Texture2D icon = {};
            const char* iconPath = organStatusIconPath(organ.getId());
            if (iconPath != nullptr) {
                auto* self = const_cast<GameScreen*>(this);
                auto iconIt = self->m_organStatusIcons.find(organ.getId());
                if (iconIt == self->m_organStatusIcons.end()) {
                    Texture2D loaded = {};
                    if (FileExists(iconPath)) {
                        loaded = LoadTexture(iconPath);
                        if (loaded.id != 0) {
                            SetTextureFilter(loaded, TEXTURE_FILTER_POINT);
                        }
                    }
                    iconIt = self->m_organStatusIcons.emplace(organ.getId(), loaded).first;
                }
                icon = iconIt->second;
            }

            const Rectangle statusRect = { statusX, statusY, statusSize, statusSize };
            const bool shaderActive = beginStatusFloatShader();
            if (icon.id != 0) {
                const Rectangle src = { 0, 0, (float)icon.width, (float)icon.height };
                DrawTexturePro(icon, src, statusRect, { 0, 0 }, 0.0f, WHITE);
            } else {
                DrawRectangleRec(statusRect, Colors::heal_color);
            }
            if (shaderActive) {
                EndShaderMode();
            }

            if (mouseOver(statusRect)) {
                queueTooltip(organ.getDisplayName(), organStatusDescription(organ), statusRect);
                hoveredStatusIcon = true;
            }

            statusX += statusSize + statusGap;
        }
    }

    const float rowBottom = statusY + statusSize + std::abs(textBobOffset);
    const Rectangle hoverRect = {
        std::min(spriteRect.x, hudX),
        spriteRect.y,
        std::max(spriteRect.x + spriteRect.width, hudX + hudWidth) - std::min(spriteRect.x, hudX),
        rowBottom - spriteRect.y
    };
    if (mouseOver(hoverRect) && !hoveredStatusIcon) {
        std::string body;
        if (hasStatuses) {
            for (const auto& status : statuses->getAll()) {
                const std::string line = statusTooltipLine(status);
                if (line.empty()) {
                    continue;
                }
                if (!body.empty()) {
                    body += "\n";
                }
                body += line;
            }
        }
        if (hasOrgans) {
            for (const Card& organ : *activeOrgans) {
                const std::string line = organStatusTooltipLine(organ);
                if (line.empty()) {
                    continue;
                }
                if (!body.empty()) {
                    body += "\n";
                }
                body += line;
            }
        }
        if (!body.empty()) {
            queueTooltip(name, body, hoverRect);
        }
    }
}

void GameScreen::ensureNoahEventTextureLoaded() {
    if (m_noahEventTextureLoaded && m_noahEventTexture.id != 0) {
        return;
    }
    if (!FileExists(AssetPaths::NOAH_EVENT_BG)) {
        return;
    }

    m_noahEventTexture = LoadTexture(AssetPaths::NOAH_EVENT_BG);
    if (m_noahEventTexture.id != 0) {
        SetTextureFilter(m_noahEventTexture, TEXTURE_FILTER_BILINEAR);
        m_noahEventTextureLoaded = true;
    }
}

void GameScreen::drawManaHud(const Player& player) const {
    const Rectangle pileRect = drawPileRect();
    const float hudWidth = (float)scalei(LayoutConfig::ManaHudWidth);
    const float hudHeight = (float)scalei(LayoutConfig::ManaHudHeight);
    const float hudX = pileRect.x + (pileRect.width - hudWidth) / 2.0f;
    const float hudY = pileRect.y - hudHeight - scalef(LayoutConfig::ManaHudGapToPile);
    const Rectangle hudRect = snapRect({ hudX, hudY, hudWidth, hudHeight });
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const int labelSize = scalei(LayoutConfig::ManaHudLabelSize);
    const int valueSize = scalei(LayoutConfig::ManaHudValueSize);
    const int valueOffsetY = scalei(LayoutConfig::ManaHudValueOffsetY);
    const int labelTop = scalei(LayoutConfig::ManaHudLabelTop);

    DrawRectangleRec(hudRect, Colors::card_bg);
    DrawRectangleLinesEx(hudRect, borderThickness, Colors::draw_pile_accent);

    const char* label = "MANA";
    const int labelWidth = MeasureText(label, labelSize);
    DrawText(label,
             (int)std::round(hudRect.x + (hudRect.width - labelWidth) / 2.0f),
             (int)std::round(hudRect.y + labelTop),
             labelSize,
             Colors::text_secondary);

    const std::string manaText = std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana());
    const int manaTextWidth = MeasureText(manaText.c_str(), valueSize);
    DrawText(manaText.c_str(),
             (int)std::round(hudRect.x + (hudRect.width - manaTextWidth) / 2.0f),
             (int)std::round(hudRect.y + valueOffsetY),
             valueSize,
             Colors::draw_pile_accent);
}

void GameScreen::drawCardFace(Rectangle rect, const Card& card, bool scaled, float rotationDegrees,
                              int playerMana, int effectiveCostOverride, bool crispPresentation,
                              float perspectiveProgress) const {
    rect = snapRect(rect);
    const int effectiveCost = effectiveCostOverride >= 0 ? effectiveCostOverride : card.getCost();
    const bool affordable = (playerMana < 0 || effectiveCost <= playerMana);
    const bool revealEffectiveCost = effectiveCostOverride >= 0 && effectiveCost == 0;
    const std::string visibleCostText = (card.isObscured() && !revealEffectiveCost)
        ? "?"
        : std::to_string(effectiveCost);
    int textureWidth = std::max(1, (int)std::lround(rect.width));
    int textureHeight = std::max(1, (int)std::lround(rect.height));
    if (crispPresentation) {
        textureWidth = quantizeTextureSize(textureWidth);
        textureHeight = quantizeTextureSize(textureHeight);
    }
    const auto faceOpt = m_cardFaceCache.getTexture(
        card,
        textureWidth,
        textureHeight,
        scaled,
        visibleCostText,
        affordable,
        crispPresentation);
    if (!faceOpt.has_value()) {
        return;
    }
    const Texture2D& face = faceOpt.value();
    if (perspectiveProgress > 0.0f && !crispPresentation) {
        drawTexturePerspective(face, rect, rotationDegrees, perspectiveProgress);
        return;
    }
    const Vector2 pivot = { rect.x + rect.width / 2.0f, rect.y + rect.height };

    rlPushMatrix();
    rlTranslatef(pivot.x, pivot.y, 0.0f);
    rlRotatef(rotationDegrees, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pivot.x, -pivot.y, 0.0f);
    const Rectangle src = { 0.0f, 0.0f, (float)face.width, (float)face.height };
    DrawTexturePro(face, src, rect, { 0.0f, 0.0f }, 0.0f, WHITE);

    rlPopMatrix();
}

void GameScreen::drawCardTooltip(const Card& card, float x, float y, int effectiveCostOverride) const {
    const int pad      = scalei(LayoutConfig::TooltipPadding);
    const int tsz      = scalei(LayoutConfig::TooltipTextSize);
    const int ttsz     = scalei(LayoutConfig::TooltipTitleSize);
    const int mtsz     = scalei(LayoutConfig::TooltipMetaSize);
    const int maxContentW = scalei(280);
    const int effectiveCost = effectiveCostOverride >= 0 ? effectiveCostOverride : card.getCost();
    const bool revealEffectiveCost = effectiveCostOverride >= 0 && effectiveCost == 0;
    const std::string visibleCost = (card.isObscured() && !revealEffectiveCost)
        ? "?"
        : std::to_string(effectiveCost);

    // Build meta line.
    std::string meta = std::string(card.getTypeLabel()) + "  Cost:" + visibleCost;
    if (!card.shouldHideFooterStats()) {
        if (card.getDamageAmount() > 0) meta += "  Dmg:" + std::to_string(card.getDamageAmount());
        if (card.getBlockAmount()  > 0) meta += "  Blk:" + std::to_string(card.getBlockAmount());
        if (card.getHealAmount()   > 0) meta += "  Heal:" + std::to_string(card.getHealAmount());
    }

    const std::vector<std::string> descLines = wrapTooltipText(card.getDisplayDescription(), tsz, maxContentW);

    // Measure all rows to get the required panel width.
    int maxW = MeasureText(card.getDisplayName().c_str(), ttsz);
    maxW = std::max(maxW, MeasureText(meta.c_str(), mtsz));
    for (const std::string& l : descLines)
        maxW = std::max(maxW, MeasureText(l.c_str(), tsz));

    // Compute panel height from actual content rows.
    const int dividerRelY = pad + ttsz + 4;          // 4px gap: title baseline → divider
    const int metaRelY    = dividerRelY + 6;          // 6px gap: divider → meta
    const int bodyRelY    = metaRelY + mtsz + 6;      // 6px gap: meta → description
    const int lineStep    = tsz + 4;
    const int bodyH       = (int)descLines.size() * lineStep - (descLines.empty() ? 0 : 4);
    const int tipW        = maxW + 2 * pad;
    const int tipH        = bodyRelY + (descLines.empty() ? 0 : bodyH) + pad;

    if (y + tipH > m_height) y = (float)(m_height - tipH - 4);
    if (x < 0) x = 4.0f;

    Rectangle tip = { x, y, (float)tipW, (float)tipH };
    DrawRectangleRec(tip, Colors::light_bg);
    DrawRectangleLinesEx(tip, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const int tx = (int)x + pad;
    DrawText(card.getDisplayName().c_str(), tx, (int)y + pad, ttsz, Colors::text_primary);

    const int divAbsY = (int)y + dividerRelY;
    DrawLine(tx, divAbsY, tx + tipW - 2 * pad, divAbsY, Colors::card_border);

    DrawText(meta.c_str(), tx, (int)y + metaRelY, mtsz, Colors::text_secondary);

    int ty = (int)y + bodyRelY;
    for (const std::string& l : descLines) {
        DrawText(l.c_str(), tx, ty, tsz, Colors::text_primary);
        ty += lineStep;
    }
}

void GameScreen::queueTooltip(const std::string& title, const std::string& body,
                              Rectangle anchor) const {
    m_pendingTooltip = { true, title, body, anchor };
}

void GameScreen::flushTooltip() const {
    if (!m_pendingTooltip.active) return;
    m_pendingTooltip.active = false;

    const int pad  = scalei(LayoutConfig::TooltipPadding);
    const int tsz  = scalei(LayoutConfig::TooltipTextSize);
    const int ttsz = scalei(LayoutConfig::TooltipTitleSize);
    // Max content width before we start wrapping body text.
    const int maxContentW = scalei(280);

    // --- Word-wrap body into lines using pixel measurement ---
    std::vector<std::string> lines;
    lines = wrapTooltipText(m_pendingTooltip.body, tsz, maxContentW);

    // --- Measure content to determine panel size ---
    const int titleW = MeasureText(m_pendingTooltip.title.c_str(), ttsz);
    int maxLineW = titleW;
    for (const std::string& l : lines)
        maxLineW = std::max(maxLineW, MeasureText(l.c_str(), tsz));

    const int dividerY  = pad + ttsz + 4;   // 4px gap: title baseline → divider
    const int bodyTop   = dividerY + 6;      // 6px gap: divider → body text
    const int lineStep  = tsz + 4;
    const int bodyH     = (int)lines.size() * lineStep - 4;   // last line has no trailing gap
    const int tipW      = maxLineW + 2 * pad;
    const int tipH      = bodyTop + (lines.empty() ? 0 : bodyH) + pad;

    // --- Position (flip left if overflowing right edge) ---
    const Rectangle& anchor = m_pendingTooltip.anchor;
    float tipX = anchor.x + anchor.width + scalef(4.0f);
    float tipY = anchor.y;
    if (tipX + tipW > m_width)  tipX = anchor.x - tipW - scalef(4.0f);
    if (tipY + tipH > m_height) tipY = (float)(m_height - tipH - 4);
    if (tipX < 4.0f) tipX = 4.0f;

    // --- Draw ---
    Rectangle tip = { tipX, tipY, (float)tipW, (float)tipH };
    DrawRectangleRec(tip, Colors::light_bg);
    DrawRectangleLinesEx(tip, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const int tx = (int)tipX + pad;
    DrawText(m_pendingTooltip.title.c_str(), tx, (int)tipY + pad, ttsz, Colors::text_primary);
    const int divAbsY = (int)tipY + dividerY;
    DrawLine(tx, divAbsY, tx + tipW - 2 * pad, divAbsY, Colors::card_border);

    int ty = (int)tipY + bodyTop;
    for (const std::string& l : lines) {
        DrawText(l.c_str(), tx, ty, tsz, Colors::text_primary);
        ty += lineStep;
    }
}

void GameScreen::drawIntentIndicator(const Enemy& enemy, Rectangle enemySpriteRect) const {
    // Lazy-load attack icon.
    if (!m_attackIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_attackIconLoaded = true;
        if (FileExists(AssetPaths::ATTACK_ICON)) {
            self->m_attackIcon = LoadTexture(AssetPaths::ATTACK_ICON);
            SetTextureFilter(self->m_attackIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_blockIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_blockIconLoaded = true;
        if (FileExists(AssetPaths::BLOCK_ICON)) {
            self->m_blockIcon = LoadTexture(AssetPaths::BLOCK_ICON);
            SetTextureFilter(self->m_blockIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_buffIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_buffIconLoaded = true;
        if (FileExists(AssetPaths::BUFF_ICON)) {
            self->m_buffIcon = LoadTexture(AssetPaths::BUFF_ICON);
            SetTextureFilter(self->m_buffIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_debuffIconLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_debuffIconLoaded = true;
        if (FileExists(AssetPaths::DEBUFF_ICON)) {
            self->m_debuffIcon = LoadTexture(AssetPaths::DEBUFF_ICON);
            SetTextureFilter(self->m_debuffIcon, TEXTURE_FILTER_POINT);
        }
    }
    if (!m_intentFloatShaderLoaded) {
        auto* self = const_cast<GameScreen*>(this);
        self->m_intentFloatShaderLoaded = true;
        if (FileExists(AssetPaths::INTENT_FLOAT_SHADER)) {
            self->m_intentFloatShader = LoadShader(AssetPaths::INTENT_FLOAT_SHADER, nullptr);
            if (self->m_intentFloatShader.id != 0) {
                self->m_intentFloatTimeLoc = GetShaderLocation(self->m_intentFloatShader, "time");
                self->m_intentFloatAmpLoc = GetShaderLocation(self->m_intentFloatShader, "amplitude");
                self->m_intentFloatSpeedLoc = GetShaderLocation(self->m_intentFloatShader, "speed");
                self->m_intentFloatPhaseLoc = GetShaderLocation(self->m_intentFloatShader, "phase");
            }
        }
    }

    const auto& indicators = enemy.getIntentIndicators();
    if (indicators.empty()) return;

    // Intent sprites sit directly below the HUD (name + gap + health bar + gap).
    const float hudWidth    = scalef(LayoutConfig::EntityHudWidth);
    const float hudX        = enemySpriteRect.x + (enemySpriteRect.width - hudWidth) / 2.0f;
    const float hudTop      = enemySpriteRect.y + enemySpriteRect.height + scalef(LayoutConfig::EntityHudGap);
    const float barY        = hudTop;
    const float belowBarY   = barY + scalei(LayoutConfig::HealthBarHeight)
                              + scalef(LayoutConfig::EntityHudGap);

    // Sprite size matches the block slot in drawEntityHud: barHeight * 1.5
    const float iconSize    = scalei(LayoutConfig::HealthBarHeight) * 1.5f;
    const int   statFontSize = scalei(LayoutConfig::EntityStatFontSize);
    const float gap         = scalef(LayoutConfig::IntentIconGap);
    const float bobAmplitude = scalef(LayoutConfig::IntentFloatAmplitude);
    const float bobSpeed = LayoutConfig::IntentFloatSpeed;
    const float timeValue = (float)GetTime();
    const float synchronizedPhase = 0.0f;
    const float textBobOffset = std::sin(timeValue * bobSpeed + synchronizedPhase) * bobAmplitude;

    // Centre the group of icons under the HUD.
    const int   count       = (int)indicators.size();
    const float groupW      = count * iconSize + (count - 1) * gap;
    float iconX             = hudX + (hudWidth - groupW) / 2.0f;
    const float iconY       = belowBarY;

    const auto beginFloatShader = [&](float phase) {
        if (m_intentFloatShader.id == 0) return false;
        if (m_intentFloatTimeLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatTimeLoc, &timeValue, SHADER_UNIFORM_FLOAT);
        }
        if (m_intentFloatAmpLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatAmpLoc, &bobAmplitude, SHADER_UNIFORM_FLOAT);
        }
        if (m_intentFloatSpeedLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatSpeedLoc, &bobSpeed, SHADER_UNIFORM_FLOAT);
        }
        if (m_intentFloatPhaseLoc >= 0) {
            SetShaderValue(m_intentFloatShader, m_intentFloatPhaseLoc, &phase, SHADER_UNIFORM_FLOAT);
        }
        BeginShaderMode(m_intentFloatShader);
        return true;
    };

    // Helper: draw one intent icon and return its clickable rect.
    const auto drawIcon = [&](Texture2D tex, bool texLoaded, Color fallback, float phase) -> Rectangle {
        const Rectangle slot = { iconX, iconY, iconSize, iconSize };
        const bool shaderActive = beginFloatShader(phase);
        if (texLoaded && tex.id != 0) {
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            DrawTexturePro(tex, src, slot, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(slot, fallback);
        }
        if (shaderActive) EndShaderMode();
        iconX += iconSize + gap;
        return slot;
    };

    for (const auto& indicator : indicators) {
        Texture2D icon = {};
        bool iconLoaded = false;
        Color fallback = Colors::mixed_intent_color;

        switch (indicator.type) {
        case IntentIconType::Attack:
            icon = m_attackIcon;
            iconLoaded = m_attackIconLoaded && m_attackIcon.id != 0;
            fallback = Colors::damage_color;
            break;
        case IntentIconType::Block:
            icon = m_blockIcon;
            iconLoaded = m_blockIconLoaded && m_blockIcon.id != 0;
            fallback = Colors::block_color;
            break;
        case IntentIconType::Buff:
            icon = m_buffIcon;
            iconLoaded = m_buffIconLoaded && m_buffIcon.id != 0;
            fallback = Colors::heal_color;
            break;
        case IntentIconType::Debuff:
            icon = m_debuffIcon;
            iconLoaded = m_debuffIconLoaded && m_debuffIcon.id != 0;
            fallback = Colors::damage_color;
            break;
        }

        const Rectangle iconRect = drawIcon(icon, iconLoaded, fallback, synchronizedPhase);
        if (!indicator.label.empty()) {
            const int labelWidth = MeasureText(indicator.label.c_str(), statFontSize);
            const int numberX = (int)std::round(iconRect.x - labelWidth + scalef(LayoutConfig::IntentAttackValueOffsetX));
            const int numberY = (int)std::round(
                iconRect.y + textBobOffset + iconRect.height - statFontSize + scalef(LayoutConfig::IntentAttackValueOffsetY));
            DrawTextOutlined(indicator.label.c_str(), numberX, numberY, statFontSize, WHITE);
        }
        if (mouseOver(iconRect)) {
            queueTooltip(enemy.getName(), indicator.tooltip, iconRect);
        }
    }

    const Rectangle hoverRect = {
        std::min(enemySpriteRect.x, hudX),
        enemySpriteRect.y,
        std::max(enemySpriteRect.x + enemySpriteRect.width, hudX + hudWidth) - std::min(enemySpriteRect.x, hudX),
        (iconY + iconSize) - enemySpriteRect.y
    };
    if (mouseOver(hoverRect)) {
        std::string body;
        for (const auto& indicator : indicators) {
            if (indicator.tooltip.empty()) {
                continue;
            }
            if (!body.empty()) {
                body += "\n";
            }
            body += indicator.tooltip;
        }
        if (!body.empty()) {
            queueTooltip(enemy.getName(), body, hoverRect);
        }
    }
}

void GameScreen::drawTurnVignetteOverlay() const {
    if (m_turnVignetteTimer <= 0.0f) {
        return;
    }

    const float t = std::clamp(m_turnVignetteTimer / LayoutConfig::TurnVignetteDuration, 0.0f, 1.0f);
    const float alphaScale = easeOutCubic(t);
    const unsigned char edgeAlpha = (unsigned char)std::lround(LayoutConfig::TurnVignetteAlpha * alphaScale);
    const unsigned char midAlpha = (unsigned char)std::lround(LayoutConfig::TurnVignetteAlpha * 0.28f * alphaScale);
    const Color mid = { 255, 255, 255, midAlpha };
    const Color edge = { 180, 30, 30, edgeAlpha };
    const int topInset = scalei(LayoutConfig::TopHudBarHeight);
    const int contentHeight = std::max(1, m_height - topInset);
    DrawRectangle(0, topInset, m_width, contentHeight, mid);
    DrawRectangleGradientV(0, topInset, m_width, contentHeight / 3, edge, BLANK);
    DrawRectangleGradientV(0, m_height - contentHeight / 3, m_width, contentHeight / 3, BLANK, edge);
    DrawRectangleGradientH(0, topInset, m_width / 5, contentHeight, edge, BLANK);
    DrawRectangleGradientH(m_width - m_width / 5, topInset, m_width / 5, contentHeight, BLANK, edge);
}

void GameScreen::drawDeathPresentationOverlay() const {
    const float timer = std::max(m_enemyDeathPresentationTimer, m_playerDeathPresentationTimer);
    if (timer <= 0.0f) {
        return;
    }

    const float t = std::clamp(timer / LayoutConfig::DeathPresentationDuration, 0.0f, 1.0f);
    const unsigned char alpha = (unsigned char)std::lround(LayoutConfig::DeathScreenRedAlpha * easeOutCubic(t));
    DrawRectangle(0, 0, m_width, m_height, Color{ 120, 0, 0, alpha });
}

void GameScreen::drawFloatingDamageNumbers(Rectangle playerSpriteRect, Rectangle enemySpriteRect) const {
    if (m_damageNumbers.empty()) {
        return;
    }

    const Vector2 playerCenter = {
        playerSpriteRect.x + playerSpriteRect.width / 2.0f,
        playerSpriteRect.y + playerSpriteRect.height / 2.0f
    };
    const Vector2 enemyCenter = {
        enemySpriteRect.x + enemySpriteRect.width / 2.0f,
        enemySpriteRect.y + enemySpriteRect.height / 2.0f
    };

    for (const FloatingDamageNumber& number : m_damageNumbers) {
        if (number.delaySecs > 0.0f) {
            continue;
        }
        const float t = std::clamp(number.age / LayoutConfig::DamageNumberLifetime, 0.0f, 1.0f);
        const float rise = easeOutCubic(std::min(t / 0.65f, 1.0f));
        const float settle = t > 0.55f ? (t - 0.55f) / 0.45f : 0.0f;
        const float yOffset = -scalef(LayoutConfig::DamageNumberRiseDistance) * rise
            + scalef(LayoutConfig::DamageNumberFallDistance) * easeOutCubic(settle)
            + number.yOffset;
        const float scale = t < 0.28f
            ? lerpValue(LayoutConfig::DamageNumberScaleStart,
                        LayoutConfig::DamageNumberScalePeak,
                        easeOutBack(t / 0.28f))
            : lerpValue(LayoutConfig::DamageNumberScalePeak,
                        LayoutConfig::DamageNumberScaleEnd,
                        (t - 0.28f) / 0.72f);
        const float fade = t < 0.62f ? 1.0f : 1.0f - ((t - 0.62f) / 0.38f);
        const int fontSize = std::max(1, (int)std::lround(scalei(LayoutConfig::DamageNumberFontSize) * scale));
        const std::string text = std::to_string(number.value);
        const int textWidth = MeasureText(text.c_str(), fontSize);
        Vector2 center = number.target == DamageNumberTarget::Player ? playerCenter : enemyCenter;
        center.x += number.target == DamageNumberTarget::Player
            ? scalef(LayoutConfig::DamageNumberCenterOffsetX)
            : -scalef(LayoutConfig::DamageNumberCenterOffsetX);
        Color baseColor = Colors::health_color;
        if (number.style == DamageNumberStyle::Block) {
            baseColor = Colors::block_color;
        } else if (number.style == DamageNumberStyle::Poison) {
            baseColor = Colors::heal_color;
        }
        const Color color = {
            baseColor.r,
            baseColor.g,
            baseColor.b,
            (unsigned char)std::lround(255.0f * std::clamp(fade, 0.0f, 1.0f))
        };
        DrawTextOutlinedAlpha(text.c_str(),
                              (int)std::lround(center.x + number.xOffset - textWidth / 2.0f),
                              (int)std::lround(center.y + yOffset - fontSize / 2.0f),
                              fontSize,
                              color);
    }
}

void GameScreen::updateTransientEffects(float dt) {
    m_turnVignetteTimer = std::max(0.0f, m_turnVignetteTimer - dt);
    m_enemyDeathPresentationTimer = std::max(0.0f, m_enemyDeathPresentationTimer - dt);
    m_playerDeathPresentationTimer = std::max(0.0f, m_playerDeathPresentationTimer - dt);
    for (FloatingDamageNumber& number : m_damageNumbers) {
        if (number.delaySecs > 0.0f) {
            number.delaySecs = std::max(0.0f, number.delaySecs - dt);
        } else {
            number.age += dt;
        }
    }
    m_damageNumbers.erase(
        std::remove_if(m_damageNumbers.begin(), m_damageNumbers.end(),
                       [](const FloatingDamageNumber& number) {
                           return number.age >= LayoutConfig::DamageNumberLifetime;
                       }),
        m_damageNumbers.end());
}

bool GameScreen::mouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}
