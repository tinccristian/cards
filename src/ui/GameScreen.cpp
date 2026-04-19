#include "GameScreen.h"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr int CARD_W   = LayoutConfig::CardWidth;
constexpr int CARD_H   = LayoutConfig::CardHeight;
constexpr int CARD_GAP = LayoutConfig::CardGap;
constexpr int ART_H    = LayoutConfig::CardArtHeight;
constexpr int PILE_W   = LayoutConfig::PileWidgetWidth;
constexpr int PILE_H   = LayoutConfig::PileWidgetHeight;
} // namespace

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
    const float marginX = m_width * LayoutConfig::PileSideMarginPercent;
    const float y = m_height - PILE_H - (m_height * LayoutConfig::PileBottomMarginPercent);
    return { marginX, y, (float)PILE_W, (float)PILE_H };
}

Rectangle GameScreen::discardPileRect() const {
    const float marginX = m_width * LayoutConfig::PileSideMarginPercent;
    const float y = m_height - PILE_H - (m_height * LayoutConfig::PileBottomMarginPercent);
    return { (float)(m_width - marginX - PILE_W), y,
             (float)PILE_W, (float)PILE_H };
}

// ---------------------------------------------------------------------------
// Main Menu
// ---------------------------------------------------------------------------

MenuAction GameScreen::drawMenu() {
    const char* title   = "Medical Deckbuilder";
    const int   titleSz = LayoutConfig::MenuTitleFontSize;
    int titleW = MeasureText(title, titleSz);
    DrawText(title, (m_width - titleW) / 2, m_height / 5, titleSz, Colors::text_primary);

    const int btnW = LayoutConfig::MenuButtonWidth;
    const int btnH = LayoutConfig::MenuButtonHeight;
    const int gap = LayoutConfig::MenuButtonGap;
    const int startY  = m_height / 2 - btnH - gap;
    const int centerX = (m_width - btnW) / 2;

    Rectangle rects[3] = {
        { (float)centerX, (float)startY,                 (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + btnH + gap),  (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + 2*(btnH+gap)),(float)btnW, (float)btnH },
    };
    const char* labels[3] = { "New Game", "Continue", "Quit" };

    MenuAction clicked = MenuAction::None;
    for (int i = 0; i < 3; ++i) {
        bool hovered = mouseOver(rects[i]);
        drawButton(rects[i], labels[i], hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = static_cast<MenuAction>(i);
        }
    }
    return clicked;
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
    bool empty   = (count == 0);
    Color bg     = empty ? Colors::empty_pile_bg : Colors::card_bg;
    Color border = empty ? Colors::text_secondary   : accentColor;

    // Stack shadow (3 offset rectangles behind the widget)
    if (!empty) {
        for (int s = LayoutConfig::PileShadowLayers; s >= 1; --s) {
            Rectangle shadow = { rect.x + s * LayoutConfig::PileShadowOffset, rect.y + s * LayoutConfig::PileShadowOffset,
                                  rect.width, rect.height };
            DrawRectangleRec(shadow, Color{ (unsigned char)(accentColor.r / 2),
                                            (unsigned char)(accentColor.g / 2),
                                            (unsigned char)(accentColor.b / 2),
                                            (unsigned char)LayoutConfig::PileShadowAlpha });
        }
    }

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, (float)LayoutConfig::PanelBorderThickness, border);

    // Label (top)
    int lw = MeasureText(label.c_str(), LayoutConfig::PileLabelFontSize);
    DrawText(label.c_str(), (int)rect.x + ((int)rect.width - lw) / 2,
             (int)rect.y + LayoutConfig::PileLabelOffsetY,
             LayoutConfig::PileLabelFontSize,
             Colors::text_primary);

    // Count (centre, large)
    std::string countStr = empty ? "EMPTY" : std::to_string(count);
    int cSz  = empty ? LayoutConfig::PileEmptyFontSize : LayoutConfig::PileCountFontSize;
    Color cCol = empty ? Colors::text_secondary : accentColor;
    int cw = MeasureText(countStr.c_str(), cSz);
    DrawText(countStr.c_str(), (int)rect.x + ((int)rect.width - cw) / 2,
             (int)rect.y + (int)rect.height / 2 - cSz / 2, cSz, cCol);

    // Hover highlight
    bool hovered = mouseOver(rect);
    if (hovered) {
        DrawRectangleLinesEx(rect, (float)(LayoutConfig::PanelBorderThickness + 1), Colors::text_primary);
        // Tooltip hint
        DrawText("Click to view", (int)rect.x, (int)rect.y + (int)rect.height + LayoutConfig::PileHintOffsetY,
                 LayoutConfig::PileHintFontSize, Colors::text_secondary);
    }

    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ---------------------------------------------------------------------------
// Combat Screen
// ---------------------------------------------------------------------------

int GameScreen::drawCombat(GameState& state, bool& endTurnClicked,
                            bool& drawPileClicked, bool& discardPileClicked) {
    endTurnClicked     = false;
    drawPileClicked    = false;
    discardPileClicked = false;

    const bool   isPlayerTurn = (state.getTurnPhase() == TurnPhase::PLAYER_TURN);
    const Player& player = state.getPlayer();
    const Enemy&  enemy  = state.getEnemy();
    const Deck&   deck   = player.getDeck();

    m_wiggleTime += GetFrameTime();

    // --- Turn number ---
    std::string turnStr = "Turn " + std::to_string(state.getTurnNumber());
    int turnW = MeasureText(turnStr.c_str(), LayoutConfig::CombatTurnFontSize);
    DrawText(turnStr.c_str(),
             (m_width - turnW) / 2,
             LayoutConfig::HealthBarHeight,
             LayoutConfig::CombatTurnFontSize,
             Colors::text_secondary);

    // --- Combatant boxes ---
    const int boxW = LayoutConfig::CombatantBoxWidth;
    const int boxH = LayoutConfig::CombatantBoxHeight;
    const int boxY = LayoutConfig::CombatantBoxTop;
    Rectangle playerBox = { (float)LayoutConfig::CombatantBoxMargin, (float)boxY, (float)boxW, (float)boxH };
    Rectangle enemyBox  = { (float)(m_width - LayoutConfig::CombatantBoxMargin - boxW), (float)boxY,
                             (float)boxW, (float)boxH };

    drawPlayerBox(playerBox, player);
    drawEnemyBox(enemyBox, enemy);
    drawIntentIndicator(enemy, enemyBox);

    // --- Combat log ---
    const std::string& action = state.getLastAction();
    if (!action.empty()) {
        int aw = MeasureText(action.c_str(), LayoutConfig::CombatLogFontSize);
        DrawText(action.c_str(), (m_width - aw) / 2, boxY + boxH + LayoutConfig::CombatLogOffsetY, LayoutConfig::CombatLogFontSize,
                 Colors::text_secondary);
    }

    // --- Draw pile widget (left side) ---
    if (drawPileWidget(drawPileRect(), "DRAW", deck.getDrawPileSize(), Colors::draw_pile_accent))
        drawPileClicked = true;

    // --- Discard pile widget (right side) ---
    if (drawPileWidget(discardPileRect(), "DISCARD", deck.getDiscardPileSize(), Colors::discard_pile_accent))
        discardPileClicked = true;

    // --- ENEMY_TURN overlay ---
    if (!isPlayerTurn) {
        const char* msg = "Enemy is acting...";
        int msgW = MeasureText(msg, LayoutConfig::PileViewerTitleSize);
        DrawText(msg, (m_width - msgW) / 2, m_height / 2 - LayoutConfig::TooltipTextSize,
                 LayoutConfig::PileViewerTitleSize, Colors::damage_color);
        return -1;
    }

    // --- End Turn button ---
    Rectangle etBtn = endTurnButtonRect();
    bool etHovered = mouseOver(etBtn);
    drawButton(etBtn, "End Turn", etHovered);
    if (etHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) endTurnClicked = true;

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
    if (m_draggedCardIndex < 0) {
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

    if (m_draggedCardIndex < 0 && m_hoveredCardIndex >= 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_draggedCardIndex = m_hoveredCardIndex;
        m_dragGrabOffset = {
            GetMouseX() - layout[m_draggedCardIndex].bounds.x,
            GetMouseY() - layout[m_draggedCardIndex].bounds.y
        };
        layout = buildHandLayout(n, m_draggedCardIndex);
    }

    if (m_draggedCardIndex >= 0 && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
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
    if (m_hoveredCardIndex >= 0 && m_draggedCardIndex < 0) {
        int i = m_hoveredCardIndex;
        Rectangle r = layout[i].bounds;
        drawCardFace(r, hand[i], true, layout[i].rotation);

        float tipX = r.x + r.width + LayoutConfig::TooltipHorizontalGap;
        if (tipX + LayoutConfig::TooltipWidth > m_width) {
            tipX = r.x - LayoutConfig::TooltipWidth - LayoutConfig::TooltipScreenMargin;
        }
        drawCardTooltip(hand[i], tipX, r.y);

    }

    if (m_draggedCardIndex >= 0) {
        const int dragIndex = m_draggedCardIndex;
        const float t = handTValue(dragIndex, n);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        Rectangle draggedRect = {
            GetMouseX() - m_dragGrabOffset.x,
            GetMouseY() - m_dragGrabOffset.y,
            layout[dragIndex].bounds.width,
            layout[dragIndex].bounds.height
        };
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
    std::string winner   = state.getWinner();
    std::string msg      = (winner == "Player") ? "You Win!" : "You Lose!";
    Color       msgColor = (winner == "Player") ? Colors::heal_color : Colors::damage_color;

    const int msgSz = LayoutConfig::GameOverFontSize;
    DrawText(msg.c_str(), (m_width - MeasureText(msg.c_str(), msgSz)) / 2,
             m_height / 3, msgSz, msgColor);

    const int btnW = LayoutConfig::GameOverButtonWidth;
    const int btnH = LayoutConfig::GameOverButtonHeight;
    Rectangle btn = { (float)((m_width - btnW) / 2), (float)(m_height / 2 + LayoutConfig::GameOverButtonOffsetY),
                       (float)btnW, (float)btnH };
    bool hovered = mouseOver(btn);
    drawButton(btn, "Return to Menu", hovered);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void GameScreen::unloadAssets() {
    m_artCache.unloadAll();
}

Rectangle GameScreen::handDropZone() const {
    const float left = m_width * LayoutConfig::HandLeftBoundPercent;
    const float right = m_width * LayoutConfig::HandRightBoundPercent;
    const float top = m_height - CARD_H - LayoutConfig::HandBottomMargin - LayoutConfig::HandDropZoneTopPadding;
    const float bottom = (float)m_height;
    return { left, top, right - left, bottom - top };
}

Rectangle GameScreen::drawPileButtonRect() const {
    return drawPileRect();
}

Rectangle GameScreen::endTurnButtonRect() const {
    const Rectangle discardRect = discardPileRect();
    return {
        discardRect.x + (discardRect.width - LayoutConfig::EndTurnButtonWidth) / 2.0f,
        discardRect.y - LayoutConfig::EndTurnButtonHeight - LayoutConfig::EndTurnToPileGap,
        (float)LayoutConfig::EndTurnButtonWidth,
        (float)LayoutConfig::EndTurnButtonHeight
    };
}

std::vector<GameScreen::HandLayoutCard> GameScreen::buildHandLayout(int cardCount, int draggedCardIndex) const {
    std::vector<HandLayoutCard> layout(cardCount);
    if (cardCount <= 0) {
        return layout;
    }

    const float handLeftBound  = m_width * LayoutConfig::HandLeftBoundPercent;
    const float handRightBound = m_width * LayoutConfig::HandRightBoundPercent;
    const float handUsableWidth = std::max((float)CARD_W, handRightBound - handLeftBound);
    const int baseY = m_height - CARD_H - LayoutConfig::HandBottomMargin;
    float cardSpacing = (cardCount > 1)
        ? (handUsableWidth - CARD_W * cardCount) / (float)(cardCount - 1)
        : 0.0f;
    cardSpacing = std::min((float)CARD_GAP, cardSpacing);
    const float totalW = CARD_W * cardCount + cardSpacing * (cardCount - 1);
    const float startX = handLeftBound + (handUsableWidth - totalW) / 2.0f;

    const float wX = std::sin(m_wiggleTime * LayoutConfig::WiggleXFrequency) * LayoutConfig::WiggleXAmplitude;
    const float wY = std::cos(m_wiggleTime * LayoutConfig::WiggleYFrequency) * LayoutConfig::WiggleYAmplitude;

    for (int index = 0; index < cardCount; ++index) {
        float cx = startX + index * (CARD_W + cardSpacing) + wX;
        float cy = (float)baseY + archOffset(index, cardCount) + wY;
        const float t = handTValue(index, cardCount);
        const float normalizedOffset = (t - 0.5f) * 2.0f;
        float rotation = normalizedOffset * LayoutConfig::HandMaxTiltDegrees;
        bool scaled = false;

        if (index == m_hoveredCardIndex && draggedCardIndex < 0) {
            const float sw = CARD_W * LayoutConfig::HoveredCardScale;
            const float sh = CARD_H * LayoutConfig::HoveredCardScale;
            cx -= (sw - CARD_W) / 2.0f;
            cy -= LayoutConfig::HoveredCardLift + (sh - CARD_H);
            rotation *= LayoutConfig::HoveredTiltFactor;
            layout[index] = { { cx, cy, sw, sh }, rotation, true };
            continue;
        }

        if (m_hoveredCardIndex >= 0 && draggedCardIndex < 0) {
            const int diff = index - m_hoveredCardIndex;
            if (diff == -1) {
                cx -= LayoutConfig::NeighborCardShift;
            } else if (diff == 1) {
                cx += LayoutConfig::NeighborCardShift;
            }
        }

        layout[index] = { { cx, cy, (float)CARD_W, (float)CARD_H }, rotation, scaled };
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

void GameScreen::drawButton(Rectangle rect, const std::string& text, bool hovered) const {
    DrawRectangleRec(rect, hovered ? Colors::button_hover : Colors::button_bg);
    DrawRectangleLinesEx(rect, (float)LayoutConfig::PanelBorderThickness, Colors::card_border);
    const int fs = LayoutConfig::DefaultButtonFontSize;
    int tw = MeasureText(text.c_str(), fs);
    DrawText(text.c_str(),
             (int)rect.x + ((int)rect.width  - tw) / 2,
             (int)rect.y + ((int)rect.height - fs) / 2,
             fs, Colors::text_primary);
}

void GameScreen::drawHealthBar(Rectangle bar, float ratio) const {
    ratio = std::max(0.0f, std::min(1.0f, ratio));
    DrawRectangleRec(bar, Colors::light_bg);
    Color fill = (ratio > 0.5f)  ? Colors::heal_color
               : (ratio > 0.25f) ? Colors::damage_color
               :                   Colors::health_color;
    Rectangle filled = { bar.x, bar.y, bar.width * ratio, bar.height };
    DrawRectangleRec(filled, fill);
    DrawRectangleLinesEx(bar, (float)LayoutConfig::ThinBorderThickness, Colors::card_border);
}

void GameScreen::drawPlayerBox(Rectangle box, const Player& player) const {
    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, 2.0f, Colors::card_border);

    DrawText("You", (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::TooltipScreenMargin,
             LayoutConfig::EntityNameFontSize, Colors::text_primary);

    Rectangle bar = {
        box.x + LayoutConfig::EntityTextPadding,
        box.y + LayoutConfig::HealthBarOffsetY,
        box.width - LayoutConfig::EntityTextPadding * 2,
        (float)LayoutConfig::HealthBarHeight
    };
    float hpRatio = (player.getMaxHealth() > 0)
                  ? (float)player.getHealth() / player.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(player.getHealth()) + " / "
                      + std::to_string(player.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::HealthTextOffsetY,
             LayoutConfig::EntityStatFontSize, Colors::text_secondary);

    std::string manaStr = "MANA: " + std::to_string(player.getMana())
                        + " / " + std::to_string(player.getMaxMana());
    DrawText(manaStr.c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::ManaTextOffsetY,
             LayoutConfig::EntityStatFontSize, Colors::text_secondary);

    if (player.getBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(player.getBlock());
        DrawText(blkStr.c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::BlockTextOffsetY,
                 LayoutConfig::EntityStatFontSize,
                 Colors::block_color);
    }
}

void GameScreen::drawEnemyBox(Rectangle box, const Enemy& enemy) const {
    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, 2.0f, Colors::card_border);

    DrawText(enemy.getName().c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::TooltipScreenMargin,
             LayoutConfig::EntityNameFontSize, Colors::text_primary);

    Rectangle bar = {
        box.x + LayoutConfig::EntityTextPadding,
        box.y + LayoutConfig::HealthBarOffsetY,
        box.width - LayoutConfig::EntityTextPadding * 2,
        (float)LayoutConfig::HealthBarHeight
    };
    float hpRatio = (enemy.getMaxHealth() > 0)
                  ? (float)enemy.getHealth() / enemy.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(enemy.getHealth()) + " / "
                      + std::to_string(enemy.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::HealthTextOffsetY,
             LayoutConfig::EntityStatFontSize, Colors::text_secondary);

    if (enemy.getEnemyBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(enemy.getEnemyBlock());
        DrawText(blkStr.c_str(), (int)box.x + LayoutConfig::EntityTextPadding, (int)box.y + LayoutConfig::ManaTextOffsetY,
                 LayoutConfig::EntityStatFontSize,
                 Colors::block_color);
    }
}

void GameScreen::drawCardFace(Rectangle rect, const Card& card, bool scaled, float rotationDegrees) const {
    const Vector2 pivot = { rect.x + rect.width / 2.0f, rect.y + rect.height };

    rlPushMatrix();
    rlTranslatef(pivot.x, pivot.y, 0.0f);
    rlRotatef(rotationDegrees, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-pivot.x, -pivot.y, 0.0f);

    Color bg = scaled ? Colors::button_hover : Colors::card_bg;
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, (float)LayoutConfig::PanelBorderThickness, Colors::card_border);

    float artH = ART_H * (rect.height / (float)CARD_H);
    Rectangle artRect = { rect.x + 2, rect.y + 2, rect.width - 4, artH - 2 };

    auto texOpt = m_artCache.getTexture(card.getArtPath());
    if (texOpt.has_value()) {
        const Texture2D& tex = texOpt.value();
        Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        DrawTexturePro(tex, src, artRect, { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        DrawRectangleRec(artRect, Colors::placeholder_art_bg);
        DrawRectangleLinesEx(artRect, (float)LayoutConfig::ThinBorderThickness, Colors::light_bg);
    }

    float divY = rect.y + artH;
    DrawLine((int)rect.x, (int)divY, (int)(rect.x + rect.width), (int)divY, Colors::card_border);

    int textX = (int)rect.x + LayoutConfig::CardTextPadding;
    int textY = (int)divY + LayoutConfig::CardTextTopPadding;

    int nameSz = scaled ? LayoutConfig::HoveredCardNameSize : LayoutConfig::CardNameFontSize;
    int nameW  = MeasureText(card.getName().c_str(), nameSz);
    DrawText(card.getName().c_str(),
             (int)rect.x + ((int)rect.width - nameW) / 2,
             textY, nameSz, Colors::text_primary);
    textY += nameSz + LayoutConfig::CardTextGap;

    const std::string& desc = card.getDescription();
    if (!desc.empty()) {
        const int descSz  = LayoutConfig::CardDescriptionSize;
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
            textY += descSz + LayoutConfig::CardDescriptionGap;
            pos = end;
            while (pos < (int)desc.size() && desc[pos] == ' ') ++pos;
            ++lines;
        }
    }

    int footerY = (int)(rect.y + rect.height) - LayoutConfig::CardFooterMargin;
    int infoSz  = scaled ? LayoutConfig::HoveredCardFooterSize : LayoutConfig::CardFooterSize;

    std::string costStr = std::to_string(card.getCost()) + " mana";
    DrawText(costStr.c_str(), textX, footerY, infoSz, Colors::text_secondary);

    if (card.getDamageAmount() > 0) {
        std::string pwrStr = std::to_string(card.getDamageAmount()) + " dmg";
        int pw = MeasureText(pwrStr.c_str(), infoSz);
        DrawText(pwrStr.c_str(), (int)(rect.x + rect.width) - pw - LayoutConfig::CardRightStatPadding, footerY,
                 infoSz, Colors::damage_color);
    }
    if (card.getBlockAmount() > 0) {
        std::string blkStr = std::to_string(card.getBlockAmount()) + " blk";
        int bw = MeasureText(blkStr.c_str(), infoSz);
        DrawText(blkStr.c_str(), (int)(rect.x + rect.width) - bw - LayoutConfig::CardRightStatPadding, footerY,
                 infoSz, Colors::block_color);
    }

    rlPopMatrix();
}

void GameScreen::drawCardTooltip(const Card& card, float x, float y) const {
    const int tipW = LayoutConfig::TooltipWidth;
    const int tipH = LayoutConfig::TooltipHeight;
    const int pad = LayoutConfig::TooltipPadding;
    const int sz = LayoutConfig::TooltipTextSize;
    if (y + tipH > m_height) y = (float)(m_height - tipH - 4);
    if (x < 0) x = 4.0f;

    Rectangle tip = { x, y, (float)tipW, (float)tipH };
    DrawRectangleRec(tip, Colors::light_bg);
    DrawRectangleLinesEx(tip, (float)LayoutConfig::PanelBorderThickness, Colors::card_border);

    int tx = (int)x + pad, ty = (int)y + pad;
    DrawText(card.getName().c_str(), tx, ty, LayoutConfig::TooltipTitleSize, Colors::text_primary);
    ty += LayoutConfig::TooltipTitleSpacing;

    std::string meta = std::string(card.getTypeLabel()) + "  Cost:" + std::to_string(card.getCost());
    if (card.getDamageAmount() > 0) meta += "  Dmg:" + std::to_string(card.getDamageAmount());
    if (card.getBlockAmount() > 0) meta += "  Blk:" + std::to_string(card.getBlockAmount());
    if (card.getHealAmount() > 0)  meta += "  Heal:" + std::to_string(card.getHealAmount());
    DrawText(meta.c_str(), tx, ty, LayoutConfig::TooltipMetaSize, Colors::text_secondary);
    ty += LayoutConfig::TooltipMetaSpacing;

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

    const int iH = LayoutConfig::IntentHeight;
    Rectangle iRect = { enemyBox.x, enemyBox.y + enemyBox.height + 6,
                         enemyBox.width, (float)iH };
    DrawRectangleRec(iRect, Colors::light_bg);
    DrawRectangleLinesEx(iRect, (float)LayoutConfig::PanelBorderThickness, intentColor);

    std::string desc = enemy.getIntentDescription();
    int dw = MeasureText(desc.c_str(), LayoutConfig::IntentFontSize);
    DrawText(desc.c_str(),
             (int)iRect.x + ((int)iRect.width - dw) / 2,
             (int)iRect.y + (iH - LayoutConfig::IntentFontSize) / 2,
             LayoutConfig::IntentFontSize, intentColor);

    if (mouseOver(iRect)) {
        float tipX = iRect.x + iRect.width + 4.0f;
        if (tipX + LayoutConfig::IntentTooltipWidth > m_width) {
            tipX = iRect.x - LayoutConfig::IntentTooltipWidth - LayoutConfig::TooltipScreenMargin;
        }
        Rectangle tipRect = { tipX, iRect.y, (float)LayoutConfig::IntentTooltipWidth, (float)LayoutConfig::IntentTooltipHeight };
        DrawRectangleRec(tipRect, Colors::light_bg);
        DrawRectangleLinesEx(tipRect, (float)LayoutConfig::ThinBorderThickness, intentColor);
        DrawText(desc.c_str(),
                 (int)tipX + LayoutConfig::IntentTooltipPadding,
                 (int)iRect.y + LayoutConfig::TooltipPadding,
                 LayoutConfig::IntentTooltipFontSize,
                 intentColor);
    }
}

bool GameScreen::mouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}
