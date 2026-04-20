#include "GameScreen.h"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GameScreen::GameScreen(int screenWidth, int screenHeight, CardAudio* cardAudio)
    : m_width(screenWidth)
    , m_height(screenHeight)
    , m_cardAudio(cardAudio)
{}

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

MapAction GameScreen::drawMapScreen() {
    syncWindowSize();
    ensureMapTextureLoaded();

    if (!m_mapTextureLoaded || m_mapTexture.id == 0) {
        const char* errorText = "Map texture missing";
        const int errorFontSize = scalei(LayoutConfig::MapTitleFontSize);
        const int errorWidth = MeasureText(errorText, errorFontSize);
        DrawText(errorText,
                 (m_width - errorWidth) / 2,
                 m_height / 2 - errorFontSize / 2,
                 errorFontSize,
                 Colors::damage_color);
        return MapAction::None;
    }

    Rectangle mapRect = mapTextureRect();

    const float wheelMove = GetMouseWheelMove();
    if (std::fabs(wheelMove) > 0.0f) {
        m_mapScrollOffset = clampedMapOffset(
            m_mapScrollOffset + wheelMove * scalef(LayoutConfig::MapScrollWheelStep));
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), mapRect)) {
        m_mapDragging = true;
        m_mapDragStartMouseY = (float)GetMouseY();
        m_mapDragStartOffset = m_mapScrollOffset;
    }

    if (m_mapDragging) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            const float dragDelta = (float)GetMouseY() - m_mapDragStartMouseY;
            m_mapScrollOffset = clampedMapOffset(m_mapDragStartOffset + dragDelta);
            mapRect = mapTextureRect();
        } else {
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

    const char* title = "Choose Entry Point";
    const char* hint = "Mouse wheel or drag to scroll";
    const int titleFontSize = scalei(LayoutConfig::MapTitleFontSize);
    const int hintFontSize = scalei(LayoutConfig::MapHintFontSize);
    DrawText(title,
             (m_width - MeasureText(title, titleFontSize)) / 2,
             scalei(LayoutConfig::MapTitleTopMargin),
             titleFontSize,
             Colors::text_primary);
    DrawText(hint,
             m_width - MeasureText(hint, hintFontSize) - scalei(LayoutConfig::MapHintSideMargin),
             m_height - scalei(LayoutConfig::MapHintBottomMargin),
             hintFontSize,
             Colors::text_secondary);

    const Vector2 nodePositions[2] = {
        {
            mapRect.x + (MapConfig::StartNodeOneX / MapConfig::SourceWidth) * mapRect.width,
            mapRect.y + (MapConfig::StartNodeOneY / MapConfig::SourceHeight) * mapRect.height
        },
        {
            mapRect.x + (MapConfig::StartNodeTwoX / MapConfig::SourceWidth) * mapRect.width,
            mapRect.y + (MapConfig::StartNodeTwoY / MapConfig::SourceHeight) * mapRect.height
        }
    };

    const float nodeRadius = (float)scalei(LayoutConfig::MapNodeRadius);
    const float outlineRadius = (float)scalei(LayoutConfig::MapNodeOutlineRadius);
    const float outlineThickness = scalef(LayoutConfig::MapNodeOutlineThickness);

    for (const Vector2& nodePosition : nodePositions) {
        const bool hovered = CheckCollisionPointCircle(GetMousePosition(), nodePosition, outlineRadius);
        DrawCircleV(nodePosition, outlineRadius, ColorAlpha(Colors::button_hover, hovered ? 0.95f : 0.70f));
        DrawCircleLines((int)std::lround(nodePosition.x),
                        (int)std::lround(nodePosition.y),
                        outlineRadius,
                        hovered ? Colors::text_primary : Colors::card_border);
        DrawCircleV(nodePosition, nodeRadius, hovered ? Colors::heal_color : Colors::button_bg);
        DrawRing(nodePosition,
                 outlineRadius - outlineThickness,
                 outlineRadius,
                 0.0f,
                 360.0f,
                 32,
                 hovered ? Colors::text_primary : Colors::card_border);

        if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)
            && std::fabs((float)GetMouseY() - m_mapDragStartMouseY) <= scalef(LayoutConfig::MapDragThreshold)) {
            m_mapDragging = false;
            return MapAction::StartCombat;
        }
    }

    return MapAction::None;
}

void GameScreen::resetMapView() {
    m_mapScrollOffset = 0.0f;
    m_mapDragging = false;
    m_mapViewInitialized = false;
    m_mapDragStartMouseY = 0.0f;
    m_mapDragStartOffset = 0.0f;
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

float handTValue(int index, int count) {
    if (count <= 1) {
        return 0.5f;
    }
    return (float)index / (float)(count - 1);
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

    const bool   isPlayerTurn = (state.getTurnPhase() == TurnPhase::PLAYER_TURN);
    const Player& player = state.getPlayer();
    const Enemy&  enemy  = state.getEnemy();
    const Deck&   deck   = player.getDeck();

    const float dt = GetFrameTime();
    m_wiggleTime += dt;
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
    const int boxW = scalei(LayoutConfig::CombatantBoxWidth);
    const int boxH = scalei(LayoutConfig::CombatantBoxHeight);
    const int boxY = scalei(LayoutConfig::CombatantBoxTop);
    const int boxMargin = scalei(LayoutConfig::CombatantBoxMargin);
    const int combatLogFontSize = scalei(LayoutConfig::CombatLogFontSize);

    // --- Turn number ---
    std::string turnStr = "Turn " + std::to_string(state.getTurnNumber());
    int turnW = MeasureText(turnStr.c_str(), turnFontSize);
    DrawText(turnStr.c_str(),
             (m_width - turnW) / 2,
             scalei(LayoutConfig::HealthBarHeight),
             turnFontSize,
             Colors::text_secondary);

    // --- Combatant boxes ---
    Rectangle playerBox = { (float)boxMargin, (float)boxY, (float)boxW, (float)boxH };
    Rectangle enemyBox  = { (float)(m_width - boxMargin - boxW), (float)boxY,
                             (float)boxW, (float)boxH };

    drawPlayerBox(playerBox, player);
    drawEnemyBox(enemyBox, enemy);
    drawIntentIndicator(enemy, enemyBox);

    // --- Combat log ---
    const std::string& action = state.getLastAction();
    if (!action.empty()) {
        int aw = MeasureText(action.c_str(), combatLogFontSize);
        DrawText(action.c_str(), (m_width - aw) / 2, boxY + boxH + scalei(LayoutConfig::CombatLogOffsetY), combatLogFontSize,
                 Colors::text_secondary);
    }

    // --- Draw pile widget (left side) ---
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

    if (n == 0) return -1;

    std::vector<HandLayoutCard> layout = buildHandLayout(n, m_draggedCardIndex);

    // Determine hovered card
    const int previousHoveredCardIndex = m_hoveredCardIndex;
    m_hoveredCardIndex = -1;
    if (!allowInteraction) {
        m_draggedCardIndex = -1;
    } else if (m_draggedCardIndex < 0) {
        for (int i = 0; i < n; ++i) {
            if (mouseOver(layout[i].bounds)) {
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
        && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_draggedCardIndex = m_hoveredCardIndex;
        m_dragGrabOffset = {
            GetMouseX() - layout[m_draggedCardIndex].bounds.x,
            GetMouseY() - layout[m_draggedCardIndex].bounds.y
        };
        layout = buildHandLayout(n, m_draggedCardIndex);
    }

    if (allowInteraction && m_draggedCardIndex >= 0 && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        const int draggedIndex = m_draggedCardIndex;
        const bool releasedInHand = CheckCollisionPointRec(GetMousePosition(), handDropZone());
        m_draggedCardIndex = -1;

        if (releasedInHand) {
            const int targetIndex = handInsertIndexFromMouseX(layout, (float)GetMouseX());
            state.getPlayer().moveCardInHand(draggedIndex, targetIndex);
            return -1;
        }

        return draggedIndex;
    }

    // Draw non-hovered cards first
    for (int i = 0; i < n; ++i) {
        if (i == m_hoveredCardIndex || i == m_draggedCardIndex) continue;
        drawCardFace(layout[i].bounds, hand[i], layout[i].scaled, layout[i].rotation);
    }

    // Hovered card on top
    if (allowInteraction && m_hoveredCardIndex >= 0 && m_draggedCardIndex < 0) {
        int i = m_hoveredCardIndex;
        Rectangle r = layout[i].bounds;
        drawCardFace(r, hand[i], true, layout[i].rotation);

        float tipX = r.x + r.width + scalef(LayoutConfig::TooltipHorizontalGap);
        if (tipX + scalei(LayoutConfig::TooltipWidth) > m_width) {
            tipX = r.x - scalei(LayoutConfig::TooltipWidth) - scalei(LayoutConfig::TooltipScreenMargin);
        }
        drawCardTooltip(hand[i], tipX, r.y);

    }

    if (allowInteraction && m_draggedCardIndex >= 0) {
        const int dragIndex = m_draggedCardIndex;
        const float t = handTValue(dragIndex, n);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        Rectangle draggedRect = {
            GetMouseX() - m_dragGrabOffset.x,
            GetMouseY() - m_dragGrabOffset.y,
            layout[dragIndex].bounds.width,
            layout[dragIndex].bounds.height
        };
        draggedRect = snapRect(draggedRect);
        drawCardFace(draggedRect, hand[dragIndex], true,
                     normalizedOffset * LayoutConfig::HandMaxTiltDegrees * LayoutConfig::HoveredTiltFactor);
    }

    return -1;
}

// ---------------------------------------------------------------------------
// Pile Viewer Overlay
// ---------------------------------------------------------------------------

int GameScreen::drawPileViewer(const std::string& title,
                                const std::vector<Card>& cards,
                                int scrollOffset,
                                bool& closeClicked) {
    syncWindowSize();
    closeClicked = false;
    // Semi-transparent backdrop
    DrawRectangle(0, 0, m_width, m_height, Colors::overlay_bg);

    // Panel
    const int panelW = LayoutConfig::PileViewerPanelWidth;
    const int panelH = LayoutConfig::PileViewerPanelHeight;
    const int panelX = (m_width  - panelW) / 2;
    const int panelY = (m_height - panelH) / 2;
    Rectangle panel  = { (float)panelX, (float)panelY, (float)panelW, (float)panelH };
    DrawRectangleRec(panel, Colors::dark_bg);
    DrawRectangleLinesEx(panel, (float)LayoutConfig::PanelBorderThickness, Colors::card_border);

    // Title
    int tw = MeasureText(title.c_str(), LayoutConfig::PileViewerTitleSize);
    DrawText(title.c_str(), panelX + (panelW - tw) / 2, panelY + LayoutConfig::PileViewerTitleOffsetY,
             LayoutConfig::PileViewerTitleSize, Colors::text_primary);

    // Count subtitle
    std::string sub = std::to_string((int)cards.size()) + " cards";
    int sw = MeasureText(sub.c_str(), LayoutConfig::PileViewerSubtitleSize);
    DrawText(sub.c_str(), panelX + (panelW - sw) / 2, panelY + LayoutConfig::PileViewerSubtitleOffsetY,
             LayoutConfig::PileViewerSubtitleSize, Colors::text_secondary);

    // X close button (top-right corner of panel)
    {
        const int btnSz = LayoutConfig::PileViewerCloseButtonSize;
        Rectangle xBtn = { (float)(panelX + panelW - btnSz - LayoutConfig::PileViewerCloseButtonMargin),
                            (float)(panelY + LayoutConfig::PileViewerCloseButtonMargin),
                            (float)btnSz, (float)btnSz };
        bool xHovered = mouseOver(xBtn);
        Color xBg  = xHovered ? Colors::close_button_hover : Colors::close_button_bg;
        DrawRectangleRec(xBtn, xBg);
        DrawRectangleLinesEx(xBtn, (float)LayoutConfig::PanelBorderThickness, Colors::damage_color);
        int xw = MeasureText("X", LayoutConfig::EntityNameFontSize);
        DrawText("X",
                 (int)xBtn.x + (btnSz - xw) / 2,
                 (int)xBtn.y + (btnSz - LayoutConfig::EntityNameFontSize) / 2,
                 LayoutConfig::EntityNameFontSize,
                 WHITE);
        if (xHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            closeClicked = true;
    }

    // Divider
    DrawLine(panelX + LayoutConfig::PileViewerDividerInset,
             panelY + LayoutConfig::PileViewerHeaderHeight,
             panelX + panelW - LayoutConfig::PileViewerDividerInset,
             panelY + LayoutConfig::PileViewerHeaderHeight,
             Colors::card_border);

    // --- Grid layout: 4 columns, one cell per card (no grouping) ---
    const int cols    = LayoutConfig::PileViewerGridColumns;
    const int cellW   = LayoutConfig::PileViewerCellWidth;
    const int cellH   = LayoutConfig::PileViewerCellHeight;
    const int gridX   = panelX + (panelW - cols * cellW) / 2;
    const int gridTop = panelY + LayoutConfig::PileViewerGridTopOffset;
    const int gridH   = panelH - LayoutConfig::PileViewerGridBottomPadding;

    BeginScissorMode(panelX + LayoutConfig::PileViewerScissorInset,
                     gridTop,
                     panelW - LayoutConfig::PileViewerScissorInset * 2,
                     gridH);

    int total     = (int)cards.size();
    int rows      = (total + cols - 1) / cols;
    int maxScroll = std::max(0, rows - (gridH / cellH));

    for (int idx = 0; idx < total; ++idx) {
        int col = idx % cols;
        int row = idx / cols;
        int cx  = gridX + col * cellW;
        int cy  = gridTop + (row - scrollOffset) * cellH;

        if (cy + cellH < gridTop || cy > gridTop + gridH) continue; // out of view

        Rectangle r = { (float)cx + LayoutConfig::PileViewerCellInset, (float)cy + LayoutConfig::PileViewerCellInset,
                         (float)cellW - LayoutConfig::PileViewerCellInset * 2, (float)cellH - LayoutConfig::PileViewerCellInset * 2 };
        drawCardFace(r, cards[idx], false, 0.0f);
    }

    EndScissorMode();

    // Scroll indicator
    if (rows > gridH / cellH) {
        std::string scrollHint = "Scroll: \x18/\x19 or wheel";
        DrawText(scrollHint.c_str(),
                 panelX + (panelW - MeasureText(scrollHint.c_str(), LayoutConfig::PileViewerFooterHintSize)) / 2,
                 panelY + panelH - LayoutConfig::PileViewerFooterOffsetY,
                 LayoutConfig::PileViewerFooterHintSize,
                 Colors::text_secondary);
    }

    return maxScroll;
}

// ---------------------------------------------------------------------------
// Game Over Screen
// ---------------------------------------------------------------------------

bool GameScreen::drawGameOver(const GameState& state) {
    syncWindowSize();

    std::string winner   = state.getWinner();
    std::string msg      = (winner == "Player") ? "You Win!" : "You Lose!";
    Color       msgColor = (winner == "Player") ? Colors::heal_color : Colors::damage_color;

    const int msgSz = scalei(LayoutConfig::GameOverFontSize);
    DrawText(msg.c_str(), (m_width - MeasureText(msg.c_str(), msgSz)) / 2,
             m_height / 3, msgSz, msgColor);

    const int btnW = scalei(LayoutConfig::GameOverButtonWidth);
    const int btnH = scalei(LayoutConfig::GameOverButtonHeight);
    Rectangle btn = { (float)((m_width - btnW) / 2), (float)(m_height / 2 + scalei(LayoutConfig::GameOverButtonOffsetY)),
                       (float)btnW, (float)btnH };
    bool hovered = mouseOver(btn);
    drawButton(btn, "Return to Menu", hovered);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

PauseAction GameScreen::drawPauseMenu() {
    syncWindowSize();
    const int panelWidth = scalei(LayoutConfig::OverlayPanelWidth);
    const int panelHeight = scalei(LayoutConfig::PausePanelHeight);
    const int titleFontSize = scalei(LayoutConfig::OverlayTitleFontSize);
    const int contentInset = scalei(LayoutConfig::OverlayContentInset);
    const int buttonWidth = scalei(LayoutConfig::OverlayButtonWidth);
    const int buttonHeight = scalei(LayoutConfig::OverlayButtonHeight);
    const int buttonGap = scalei(LayoutConfig::OverlayButtonGap);
    const int buttonsTopOffset = scalei(LayoutConfig::PauseButtonsTopOffset);

    DrawRectangle(0, 0, m_width, m_height, Colors::overlay_bg);

    const float panelX = (m_width - panelWidth) / 2.0f;
    const float panelY = (m_height - panelHeight) / 2.0f;
    Rectangle panel = {
        panelX,
        panelY,
        (float)panelWidth,
        (float)panelHeight
    };

    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const char* title = "Paused";
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title,
             (int)(panelX + (panel.width - titleWidth) / 2.0f),
             (int)panelY + contentInset,
             titleFontSize,
             Colors::text_primary);

    const float buttonX = panelX + (panel.width - buttonWidth) / 2.0f;
    const float buttonStartY = panelY + buttonsTopOffset;
    const char* labels[4] = { "Resume", "Options", "Main Menu", "Quit" };

    for (int index = 0; index < 4; ++index) {
        Rectangle button = {
            buttonX,
            buttonStartY + index * (buttonHeight + buttonGap),
            (float)buttonWidth,
            (float)buttonHeight
        };
        const bool hovered = mouseOver(button);
        drawButton(button, labels[index], hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return static_cast<PauseAction>(index + 1);
        }
    }

    return PauseAction::None;
}

OptionsMenuAction GameScreen::drawOptionsMenu(AppSettings& settings,
                                              OptionsSection& activeSection,
                                              bool openedFromPause) {
    syncWindowSize();
    const int panelWidth = scalei(LayoutConfig::OverlayPanelWidth);
    const int panelHeight = scalei(LayoutConfig::OptionsPanelHeight);
    const int titleFontSize = scalei(LayoutConfig::OverlayTitleFontSize);
    const int contentInset = scalei(LayoutConfig::OverlayContentInset);
    const int tabWidth = scalei(LayoutConfig::OptionsTabWidth);
    const int tabHeight = scalei(LayoutConfig::OptionsTabHeight);
    const int tabGap = scalei(LayoutConfig::OptionsTabGap);
    const int tabTopOffset = scalei(LayoutConfig::OptionsTabTopOffset);
    const int rowsTopOffset = scalei(LayoutConfig::OptionsRowsTopOffset);
    const int rowHeight = scalei(LayoutConfig::OptionsRowHeight);
    const int buttonWidth = scalei(LayoutConfig::OverlayButtonWidth);
    const int buttonHeight = scalei(LayoutConfig::OverlayButtonHeight);
    const int footerOffsetY = scalei(LayoutConfig::OptionsFooterOffsetY);
    const int buttonFontSize = scalei(LayoutConfig::DefaultButtonFontSize);

    DrawRectangle(0, 0, m_width, m_height, Colors::overlay_bg);

    const float panelX = (m_width - panelWidth) / 2.0f;
    const float panelY = (m_height - panelHeight) / 2.0f;
    Rectangle panel = {
        panelX,
        panelY,
        (float)panelWidth,
        (float)panelHeight
    };

    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const char* title = "Options";
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title,
             (int)(panelX + (panel.width - titleWidth) / 2.0f),
             (int)panelY + contentInset,
             titleFontSize,
             Colors::text_primary);

    const float totalTabsWidth = tabWidth * 2.0f + tabGap;
    const float tabsX = panelX + (panel.width - totalTabsWidth) / 2.0f;
    const float tabsY = panelY + tabTopOffset;
    const char* tabLabels[2] = { "Display", "Sounds" };

    for (int index = 0; index < 2; ++index) {
        Rectangle tab = {
            tabsX + index * (tabWidth + tabGap),
            tabsY,
            (float)tabWidth,
            (float)tabHeight
        };
        const bool selected = activeSection == static_cast<OptionsSection>(index);
        const bool hovered = mouseOver(tab);
        DrawRectangleRec(tab, selected ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(tab, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);
        const int labelWidth = MeasureText(tabLabels[index], buttonFontSize);
        DrawText(tabLabels[index],
                 (int)(tab.x + (tab.width - labelWidth) / 2.0f),
                 (int)(tab.y + (tab.height - buttonFontSize) / 2.0f),
                 buttonFontSize,
                 Colors::text_primary);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            activeSection = static_cast<OptionsSection>(index);
        }
    }

    float rowY = panelY + rowsTopOffset;
    if (activeSection == OptionsSection::Display) {
        settings.resolutionIndex = std::clamp(
            settings.resolutionIndex + drawStepperRow(rowY, "Resolution",
                SettingsManager::resolutionLabel(settings.resolutionIndex)),
            0,
            static_cast<int>(SettingsManager::resolutionOptions().size()) - 1
        );
        rowY += rowHeight;

        int windowModeIndex = static_cast<int>(settings.windowMode);
        windowModeIndex = (windowModeIndex + drawStepperRow(rowY, "Window Mode",
            SettingsManager::windowModeLabel(settings.windowMode)) + 3) % 3;
        settings.windowMode = static_cast<WindowMode>(windowModeIndex);
        rowY += rowHeight;

        const int monitorCount = std::max(1, GetMonitorCount());
        const int safeMonitorIndex = std::clamp(settings.monitorIndex, 0, monitorCount - 1);
        const std::string displayValue =
            std::to_string(safeMonitorIndex + 1) + " / " + std::to_string(monitorCount)
            + "  (" + std::to_string(GetMonitorWidth(safeMonitorIndex))
            + " x " + std::to_string(GetMonitorHeight(safeMonitorIndex)) + ")";
        settings.monitorIndex = (safeMonitorIndex + drawStepperRow(rowY, "Display", displayValue) + monitorCount) % monitorCount;
        rowY += rowHeight;

        if (drawCheckboxRow(rowY, "VSync", settings.vsyncEnabled)) {
            settings.vsyncEnabled = !settings.vsyncEnabled;
        }
        rowY += rowHeight;

        if (drawCheckboxRow(rowY, "Show FPS", settings.showFps)) {
            settings.showFps = !settings.showFps;
        }
    } else {
        settings.masterVolume = std::clamp(
            settings.masterVolume
                + drawStepperRow(rowY, "Master Volume", std::to_string(settings.masterVolume) + "%") * AudioConfig::MasterVolumeStep,
            0,
            100
        );
    }

    Rectangle backButton = {
        panelX + (panel.width - buttonWidth) / 2.0f,
        panelY + panel.height - footerOffsetY,
        (float)buttonWidth,
        (float)buttonHeight
    };
    const std::string backLabel = openedFromPause ? "Back to Pause" : "Back";
    const bool backHovered = mouseOver(backButton);
    drawButton(backButton, backLabel, backHovered);
    if (backHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return OptionsMenuAction::Back;
    }

    return OptionsMenuAction::None;
}

void GameScreen::unloadAssets() {
    m_artCache.unloadAll();
    if (m_mapTextureLoaded && m_mapTexture.id != 0) {
        UnloadTexture(m_mapTexture);
        m_mapTexture = {};
        m_mapTextureLoaded = false;
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

    const float wX = std::sin(m_wiggleTime * LayoutConfig::WiggleXFrequency) * scalef(LayoutConfig::WiggleXAmplitude);
    const float wY = std::cos(m_wiggleTime * LayoutConfig::WiggleYFrequency) * scalef(LayoutConfig::WiggleYAmplitude);

    const int activeHoverIndex = (draggedCardIndex < 0) ? m_hoverProgressIndex : -1;

    for (int index = 0; index < cardCount; ++index) {
        const float cx_base = startX + index * (cardWidth + cardSpacing);
        const float cy_base = baseY + archOffset(index, cardCount) * uiScale();
        const float t = handTValue(index, cardCount);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        const float restRotation = normalizedOffset * LayoutConfig::HandMaxTiltDegrees;

        float cx = cx_base + wX;
        float cy = cy_base + wY;
        float rotation = restRotation;

        if (index == activeHoverIndex && m_hoverProgress > 0.0f) {
            const float hp = m_hoverProgress;
            const float restCy = cy_base + wY;
            const float snappedCardHeight = std::round(cardHeight);
            const float hoverCy = std::round((float)m_height - snappedCardHeight);
            cy = restCy + (hoverCy - restCy) * hp;
            cx = cx_base + wX * (1.0f - hp);
            rotation = restRotation * (1.0f - hp);
            Rectangle hoverRect = { std::round(cx), cy, snappedCardHeight > 0 ? cardWidth : cardWidth, snappedCardHeight };
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
            if (diff == -1) cx -= scalef(LayoutConfig::NeighborCardShift);
            else if (diff == 1) cx += scalef(LayoutConfig::NeighborCardShift);
        }

        layout[index] = { snapRect({ cx, cy, cardWidth, cardHeight }), rotation, false };
    }

    return layout;
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

void GameScreen::ensureMapTextureLoaded() {
    if (m_mapTextureLoaded) {
        return;
    }

    m_mapTexture = LoadTexture(AssetPaths::LEG_MAP);
    if (m_mapTexture.id != 0) {
        SetTextureFilter(m_mapTexture, TEXTURE_FILTER_BILINEAR);
        m_mapTextureLoaded = true;
    }
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

void GameScreen::drawHealthBar(Rectangle bar, float ratio) const {
    ratio = std::max(0.0f, std::min(1.0f, ratio));
    DrawRectangleRec(bar, Colors::light_bg);
    Color fill = (ratio > 0.5f)  ? Colors::heal_color
               : (ratio > 0.25f) ? Colors::damage_color
               :                   Colors::health_color;
    Rectangle filled = { bar.x, bar.y, bar.width * ratio, bar.height };
    DrawRectangleRec(filled, fill);
    DrawRectangleLinesEx(bar, scalef(LayoutConfig::ThinBorderThickness), Colors::card_border);
}

void GameScreen::drawPlayerBox(Rectangle box, const Player& player) const {
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const int textPadding = scalei(LayoutConfig::EntityTextPadding);
    const int screenMargin = scalei(LayoutConfig::TooltipScreenMargin);
    const int nameFontSize = scalei(LayoutConfig::EntityNameFontSize);
    const int healthBarOffsetY = scalei(LayoutConfig::HealthBarOffsetY);
    const int healthBarHeight = scalei(LayoutConfig::HealthBarHeight);
    const int healthTextOffsetY = scalei(LayoutConfig::HealthTextOffsetY);
    const int manaTextOffsetY = scalei(LayoutConfig::ManaTextOffsetY);
    const int blockTextOffsetY = scalei(LayoutConfig::BlockTextOffsetY);
    const int statFontSize = scalei(LayoutConfig::EntityStatFontSize);

    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, borderThickness, Colors::card_border);

    DrawText("You", (int)box.x + textPadding, (int)box.y + screenMargin,
             nameFontSize, Colors::text_primary);

    Rectangle bar = {
        box.x + textPadding,
        box.y + healthBarOffsetY,
        box.width - textPadding * 2,
        (float)healthBarHeight
    };
    float hpRatio = (player.getMaxHealth() > 0)
                  ? (float)player.getHealth() / player.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(player.getHealth()) + " / "
                      + std::to_string(player.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + textPadding, (int)box.y + healthTextOffsetY,
             statFontSize, Colors::text_secondary);

    std::string manaStr = "MANA: " + std::to_string(player.getMana())
                        + " / " + std::to_string(player.getMaxMana());
    DrawText(manaStr.c_str(), (int)box.x + textPadding, (int)box.y + manaTextOffsetY,
             statFontSize, Colors::text_secondary);

    if (player.getBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(player.getBlock());
        DrawText(blkStr.c_str(), (int)box.x + textPadding, (int)box.y + blockTextOffsetY,
                 statFontSize,
                 Colors::block_color);
    }
}

void GameScreen::drawEnemyBox(Rectangle box, const Enemy& enemy) const {
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const int textPadding = scalei(LayoutConfig::EntityTextPadding);
    const int screenMargin = scalei(LayoutConfig::TooltipScreenMargin);
    const int nameFontSize = scalei(LayoutConfig::EntityNameFontSize);
    const int healthBarOffsetY = scalei(LayoutConfig::HealthBarOffsetY);
    const int healthBarHeight = scalei(LayoutConfig::HealthBarHeight);
    const int healthTextOffsetY = scalei(LayoutConfig::HealthTextOffsetY);
    const int manaTextOffsetY = scalei(LayoutConfig::ManaTextOffsetY);
    const int statFontSize = scalei(LayoutConfig::EntityStatFontSize);

    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, borderThickness, Colors::card_border);

    DrawText(enemy.getName().c_str(), (int)box.x + textPadding, (int)box.y + screenMargin,
             nameFontSize, Colors::text_primary);

    Rectangle bar = {
        box.x + textPadding,
        box.y + healthBarOffsetY,
        box.width - textPadding * 2,
        (float)healthBarHeight
    };
    float hpRatio = (enemy.getMaxHealth() > 0)
                  ? (float)enemy.getHealth() / enemy.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(enemy.getHealth()) + " / "
                      + std::to_string(enemy.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + textPadding, (int)box.y + healthTextOffsetY,
             statFontSize, Colors::text_secondary);

    if (enemy.getEnemyBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(enemy.getEnemyBlock());
        DrawText(blkStr.c_str(), (int)box.x + textPadding, (int)box.y + manaTextOffsetY,
                 statFontSize,
                 Colors::block_color);
    }
}

void GameScreen::drawCardFace(Rectangle rect, const Card& card, bool scaled, float rotationDegrees) const {
    rect = snapRect(rect);
    const float borderThickness = scalef(LayoutConfig::PanelBorderThickness);
    const float thinBorderThickness = scalef(LayoutConfig::ThinBorderThickness);
    const float baseCardHeight = (float)scalei(LayoutConfig::CardHeight);
    const float baseArtHeight = (float)scalei(LayoutConfig::CardArtHeight);
    const int textPadding = scalei(LayoutConfig::CardTextPadding);
    const int textTopPadding = scalei(LayoutConfig::CardTextTopPadding);
    const int textGap = scalei(LayoutConfig::CardTextGap);
    const int descriptionGap = scalei(LayoutConfig::CardDescriptionGap);
    const int footerMargin = scalei(LayoutConfig::CardFooterMargin);
    const int rightStatPadding = scalei(LayoutConfig::CardRightStatPadding);
    const Vector2 pivot = { rect.x + rect.width / 2.0f, rect.y + rect.height };

    rlPushMatrix();
    rlTranslatef(pivot.x, pivot.y, 0.0f);
    rlRotatef(rotationDegrees, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pivot.x, -pivot.y, 0.0f);

    Color bg = scaled ? Colors::button_hover : Colors::card_bg;
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, borderThickness, Colors::card_border);

    float artH = baseArtHeight * (rect.height / baseCardHeight);
    Rectangle artRect = snapRect({ rect.x + 2, rect.y + 2, rect.width - 4, artH - 2 });

    auto texOpt = m_artCache.getTexture(card.getArtPath());
    if (texOpt.has_value()) {
        const Texture2D& tex = texOpt.value();
        Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        DrawTexturePro(tex, src, artRect, { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        DrawRectangleRec(artRect, Colors::placeholder_art_bg);
        DrawRectangleLinesEx(artRect, thinBorderThickness, Colors::light_bg);
    }

    float divY = rect.y + artH;
    DrawLine((int)rect.x, (int)divY, (int)(rect.x + rect.width), (int)divY, Colors::card_border);

    int textX = (int)rect.x + textPadding;
    int textY = (int)divY + textTopPadding;

    int nameSz = scalei(scaled ? LayoutConfig::HoveredCardNameSize : LayoutConfig::CardNameFontSize);
    int nameW  = MeasureText(card.getName().c_str(), nameSz);
    DrawText(card.getName().c_str(),
             (int)rect.x + ((int)rect.width - nameW) / 2,
             textY, nameSz, Colors::text_primary);
    textY += nameSz + textGap;

    const std::string& desc = card.getDescription();
    if (!desc.empty()) {
        const int descSz  = scalei(LayoutConfig::CardDescriptionSize);
        const int lineLen = (int)rect.width / 6;
        int pos = 0, lines = 0;
        while (pos < (int)desc.size() && lines < LayoutConfig::CardDescriptionLines) {
            int end = std::min(pos + lineLen, (int)desc.size());
            if (end < (int)desc.size() && desc[end] != ' ') {
                int sp = (int)desc.rfind(' ', end);
                if (sp > pos) end = sp;
            }
            DrawText(desc.substr(pos, end - pos).c_str(), textX, textY, descSz,
                     Colors::text_secondary);
            textY += descSz + descriptionGap;
            pos = end;
            while (pos < (int)desc.size() && desc[pos] == ' ') ++pos;
            ++lines;
        }
    }

    int footerY = (int)(rect.y + rect.height) - footerMargin;
    int infoSz  = scalei(scaled ? LayoutConfig::HoveredCardFooterSize : LayoutConfig::CardFooterSize);

    std::string costStr = std::to_string(card.getCost()) + " mana";
    DrawText(costStr.c_str(), textX, footerY, infoSz, Colors::text_secondary);

    if (card.getDamageAmount() > 0) {
        std::string pwrStr = std::to_string(card.getDamageAmount()) + " dmg";
        int pw = MeasureText(pwrStr.c_str(), infoSz);
        DrawText(pwrStr.c_str(), (int)(rect.x + rect.width) - pw - rightStatPadding, footerY,
                 infoSz, Colors::damage_color);
    }
    if (card.getBlockAmount() > 0) {
        std::string blkStr = std::to_string(card.getBlockAmount()) + " blk";
        int bw = MeasureText(blkStr.c_str(), infoSz);
        DrawText(blkStr.c_str(), (int)(rect.x + rect.width) - bw - rightStatPadding, footerY,
                 infoSz, Colors::block_color);
    }

    rlPopMatrix();
}

void GameScreen::drawCardTooltip(const Card& card, float x, float y) const {
    const int tipW = scalei(LayoutConfig::TooltipWidth);
    const int tipH = scalei(LayoutConfig::TooltipHeight);
    const int pad = scalei(LayoutConfig::TooltipPadding);
    const int sz = scalei(LayoutConfig::TooltipTextSize);
    if (y + tipH > m_height) y = (float)(m_height - tipH - 4);
    if (x < 0) x = 4.0f;

    Rectangle tip = { x, y, (float)tipW, (float)tipH };
    DrawRectangleRec(tip, Colors::light_bg);
    DrawRectangleLinesEx(tip, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    int tx = (int)x + pad, ty = (int)y + pad;
    DrawText(card.getName().c_str(), tx, ty, scalei(LayoutConfig::TooltipTitleSize), Colors::text_primary);
    ty += scalei(LayoutConfig::TooltipTitleSpacing);

    std::string meta = std::string(card.getTypeLabel()) + "  Cost:" + std::to_string(card.getCost());
    if (card.getDamageAmount() > 0) meta += "  Dmg:" + std::to_string(card.getDamageAmount());
    if (card.getBlockAmount() > 0) meta += "  Blk:" + std::to_string(card.getBlockAmount());
    if (card.getHealAmount() > 0)  meta += "  Heal:" + std::to_string(card.getHealAmount());
    DrawText(meta.c_str(), tx, ty, scalei(LayoutConfig::TooltipMetaSize), Colors::text_secondary);
    ty += scalei(LayoutConfig::TooltipMetaSpacing);

    DrawLine(tx, ty, tx + tipW - 2*pad, ty, Colors::card_border);
    ty += 6;

    const std::string& desc = card.getDescription();
    const int lineLen = LayoutConfig::TooltipWrapWidth;
    int pos = 0;
    while (pos < (int)desc.size() && ty < (int)y + tipH - sz) {
        int end = std::min(pos + lineLen, (int)desc.size());
        if (end < (int)desc.size() && desc[end] != ' ') {
            int sp = (int)desc.rfind(' ', end);
            if (sp > pos) end = sp;
        }
        DrawText(desc.substr(pos, end - pos).c_str(), tx, ty, sz, Colors::text_primary);
        ty += sz + 4;
        pos = end;
        while (pos < (int)desc.size() && desc[pos] == ' ') ++pos;
    }
}

void GameScreen::drawIntentIndicator(const Enemy& enemy, Rectangle enemyBox) const {
    int dmg = enemy.getIntentDamage();
    int blk = enemy.getIntentBlock();

    Color intentColor;
    if      (dmg > 0 && blk > 0) intentColor = Colors::mixed_intent_color;
    else if (dmg > 0)             intentColor = Colors::damage_color;
    else                          intentColor = Colors::block_color;

    const int iH = scalei(LayoutConfig::IntentHeight);
    const int fontSize = scalei(LayoutConfig::IntentFontSize);
    Rectangle iRect = { enemyBox.x, enemyBox.y + enemyBox.height + scalef(6.0f),
                         enemyBox.width, (float)iH };
    DrawRectangleRec(iRect, Colors::light_bg);
    DrawRectangleLinesEx(iRect, scalef(LayoutConfig::PanelBorderThickness), intentColor);

    std::string desc = enemy.getIntentDescription();
    int dw = MeasureText(desc.c_str(), fontSize);
    DrawText(desc.c_str(),
             (int)iRect.x + ((int)iRect.width - dw) / 2,
             (int)iRect.y + (iH - fontSize) / 2,
             fontSize, intentColor);

    if (mouseOver(iRect)) {
        const int tooltipWidth = scalei(LayoutConfig::IntentTooltipWidth);
        const int tooltipHeight = scalei(LayoutConfig::IntentTooltipHeight);
        const int tooltipFontSize = scalei(LayoutConfig::IntentTooltipFontSize);
        float tipX = iRect.x + iRect.width + scalef(4.0f);
        if (tipX + tooltipWidth > m_width) {
            tipX = iRect.x - tooltipWidth - scalei(LayoutConfig::TooltipScreenMargin);
        }
        Rectangle tipRect = { tipX, iRect.y, (float)tooltipWidth, (float)tooltipHeight };
        DrawRectangleRec(tipRect, Colors::light_bg);
        DrawRectangleLinesEx(tipRect, scalef(LayoutConfig::ThinBorderThickness), intentColor);
        DrawText(desc.c_str(),
                 (int)tipX + scalei(LayoutConfig::IntentTooltipPadding),
                 (int)iRect.y + scalei(LayoutConfig::TooltipPadding),
                 tooltipFontSize,
                 intentColor);
    }
}

bool GameScreen::mouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}
