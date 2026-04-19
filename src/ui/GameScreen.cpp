#include "GameScreen.h"

#include <algorithm>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GameScreen::GameScreen(int screenWidth, int screenHeight)
    : m_width(screenWidth)
    , m_height(screenHeight)
{}

// ---------------------------------------------------------------------------
// Main Menu
// ---------------------------------------------------------------------------

int GameScreen::drawMenu() {
    const char* title   = "Medical Deckbuilder";
    const int   titleSz = 48;
    int titleW = MeasureText(title, titleSz);
    DrawText(title, (m_width - titleW) / 2, m_height / 5, titleSz, Colors::text_primary);

    const int btnW = 200, btnH = 60, gap = 20;
    const int startY  = m_height / 2 - btnH - gap;
    const int centerX = (m_width - btnW) / 2;

    Rectangle rects[3] = {
        { (float)centerX, (float)startY,                (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + btnH + gap), (float)btnW, (float)btnH },
        { (float)centerX, (float)(startY + 2*(btnH+gap)),(float)btnW, (float)btnH },
    };
    const char* labels[3] = { "New Game", "Continue", "Quit" };

    int clicked = -1;
    for (int i = 0; i < 3; ++i) {
        bool hovered = mouseOver(rects[i]);
        drawButton(rects[i], labels[i], hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) clicked = i;
    }
    return clicked;
}

// ---------------------------------------------------------------------------
// Arch offset helper
// ---------------------------------------------------------------------------

float GameScreen::archOffset(int i, int n) {
    if (n <= 1) return 0.0f;
    // Parabolic: centre card at 0, outer cards raised by up to 24 px
    float t = (float)i / (float)(n - 1); // 0..1
    float centre = 0.5f;
    float dist = t - centre;             // -0.5..0.5
    return -24.0f * (1.0f - 4.0f * dist * dist); // 0 at ends, -24 at middle
    // Note: negative Y is UP in screen space, so outer cards are higher
}

// ---------------------------------------------------------------------------
// Combat Screen
// ---------------------------------------------------------------------------

int GameScreen::drawCombat(GameState& state, bool& endTurnClicked) {
    endTurnClicked = false;
    const bool   isPlayerTurn = (state.getTurnPhase() == TurnPhase::PLAYER_TURN);
    const Player& player = state.getPlayer();
    const Enemy&  enemy  = state.getEnemy();

    // Advance wiggle timer
    m_wiggleTime += GetFrameTime();

    // --- Turn number (top centre) ---
    std::string turnStr = "Turn " + std::to_string(state.getTurnNumber());
    int turnW = MeasureText(turnStr.c_str(), 26);
    DrawText(turnStr.c_str(), (m_width - turnW) / 2, 18, 26, Colors::text_secondary);

    // --- Deck / discard pile counts (bottom-left corner) ---
    {
        const Deck& deck = player.getDeck();
        std::string deckStr    = "Deck: "    + std::to_string(deck.size());
        std::string discardStr = "Discard: " + std::to_string((int)deck.getDiscard().size());
        DrawText(deckStr.c_str(),    16, m_height - 40, 16, Colors::text_secondary);
        DrawText(discardStr.c_str(), 16, m_height - 22, 16, Colors::text_secondary);
    }

    // --- Combatant boxes ---
    const int boxW = 260, boxH = 160, boxY = 55;
    Rectangle playerBox = { 50.0f, (float)boxY, (float)boxW, (float)boxH };
    Rectangle enemyBox  = { (float)(m_width - 50 - boxW), (float)boxY,
                             (float)boxW, (float)boxH };

    drawPlayerBox(playerBox, player);
    drawEnemyBox(enemyBox, enemy);
    drawIntentIndicator(enemy, enemyBox);

    // --- Combat log ---
    const std::string& action = state.getLastAction();
    if (!action.empty()) {
        int aw = MeasureText(action.c_str(), 17);
        DrawText(action.c_str(), (m_width - aw) / 2, boxY + boxH + 8, 17,
                 Colors::text_secondary);
    }

    // --- ENEMY_TURN overlay ---
    if (!isPlayerTurn) {
        const char* msg = "Enemy is acting...";
        int msgW = MeasureText(msg, 28);
        DrawText(msg, (m_width - msgW) / 2, m_height / 2 - 14, 28, Colors::damage_color);
        return -1;
    }

    // --- End Turn button ---
    const int etW = 160, etH = 50;
    Rectangle etBtn = { (float)(m_width - etW - 24), (float)(m_height - etH - 24),
                         (float)etW, (float)etH };
    bool etHovered = mouseOver(etBtn);
    drawButton(etBtn, "End Turn", etHovered);
    if (etHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) endTurnClicked = true;

    // -------------------------------------------------------------------
    // Hand layout: arch + wiggle
    // -------------------------------------------------------------------
    const auto& hand  = player.getHand();
    const int   n     = (int)hand.size();

    if (n == 0) return -1;

    // Baseline Y: cards sit near the bottom with enough room for the scaled hover
    const int baseY = m_height - CARD_H - 50;

    // Total width of the hand (un-scaled)
    const int totalW = n * CARD_W + (n - 1) * CARD_GAP;
    const int startX = (m_width - totalW) / 2;

    // Per-card base X/Y (arch, no wiggle yet)
    // We compute these upfront for hover detection
    std::vector<float> baseCardX(n), baseCardY(n);
    for (int i = 0; i < n; ++i) {
        baseCardX[i] = (float)(startX + i * (CARD_W + CARD_GAP));
        baseCardY[i] = (float)baseY + archOffset(i, n);
    }

    // --- First pass: determine hovered card (use base rects for hit test) ---
    m_hoveredCardIndex = -1;
    for (int i = 0; i < n; ++i) {
        Rectangle r = { baseCardX[i], baseCardY[i], (float)CARD_W, (float)CARD_H };
        if (mouseOver(r)) m_hoveredCardIndex = i;
    }

    // --- Compute final positions with wiggle and hover displacement ---
    const float wX = std::sin(m_wiggleTime * 3.0f) * 3.0f;
    const float wY = std::cos(m_wiggleTime * 2.0f) * 2.0f;

    struct CardPos { float x, y, w, h; };
    std::vector<CardPos> positions(n);
    for (int i = 0; i < n; ++i) {
        float cx = baseCardX[i] + wX;
        float cy = baseCardY[i] + wY;

        if (i == m_hoveredCardIndex) {
            // Scale 1.4×, move up 40 px
            float sw = CARD_W * 1.4f, sh = CARD_H * 1.4f;
            cx -= (sw - CARD_W) / 2.0f;
            cy -= 40.0f + (sh - CARD_H); // raise so bottom stays roughly aligned
            positions[i] = { cx, cy, sw, sh };
        } else {
            // Adjacent cards slide sideways to make room
            float sideShift = 0.0f;
            if (m_hoveredCardIndex >= 0) {
                int diff = i - m_hoveredCardIndex;
                if      (diff == -1) sideShift = -20.0f;
                else if (diff ==  1) sideShift =  20.0f;
            }
            positions[i] = { cx + sideShift, cy, (float)CARD_W, (float)CARD_H };
        }
    }

    // --- Draw non-hovered cards first ---
    int clicked = -1;
    for (int i = 0; i < n; ++i) {
        if (i == m_hoveredCardIndex) continue;
        Rectangle r = { positions[i].x, positions[i].y,
                         positions[i].w, positions[i].h };
        drawCardFace(r, hand[i], false);
        if (mouseOver(r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) clicked = i;
    }

    // --- Draw hovered card last (on top) with tooltip ---
    if (m_hoveredCardIndex >= 0) {
        int i = m_hoveredCardIndex;
        Rectangle r = { positions[i].x, positions[i].y,
                         positions[i].w, positions[i].h };
        drawCardFace(r, hand[i], true);

        float tipX = r.x + r.width + 8.0f;
        if (tipX + 210 > m_width) tipX = r.x - 214.0f;
        drawCardTooltip(hand[i], tipX, r.y);

        if (mouseOver(r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) clicked = i;
    }

    return clicked;
}

// ---------------------------------------------------------------------------
// Game Over Screen
// ---------------------------------------------------------------------------

bool GameScreen::drawGameOver(const GameState& state) {
    std::string winner   = state.getWinner();
    std::string msg      = (winner == "Player") ? "You Win!" : "You Lose!";
    Color       msgColor = (winner == "Player") ? Colors::heal_color : Colors::damage_color;

    const int msgSz = 60;
    DrawText(msg.c_str(), (m_width - MeasureText(msg.c_str(), msgSz)) / 2,
             m_height / 3, msgSz, msgColor);

    const int btnW = 240, btnH = 60;
    Rectangle btn = { (float)((m_width - btnW) / 2), (float)(m_height / 2 + 20),
                       (float)btnW, (float)btnH };
    bool hovered = mouseOver(btn);
    drawButton(btn, "Return to Menu", hovered);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void GameScreen::drawButton(Rectangle rect, const std::string& text, bool hovered) const {
    DrawRectangleRec(rect, hovered ? Colors::button_hover : Colors::button_bg);
    DrawRectangleLinesEx(rect, 2.0f, Colors::card_border);
    const int fs = 22;
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
    DrawRectangleLinesEx(bar, 1.0f, Colors::card_border);
}

void GameScreen::drawPlayerBox(Rectangle box, const Player& player) const {
    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, 2.0f, Colors::card_border);

    DrawText("You", (int)box.x + 10, (int)box.y + 8, 20, Colors::text_primary);

    Rectangle bar = { box.x + 10, box.y + 35, box.width - 20, 18 };
    float hpRatio = (player.getMaxHealth() > 0)
                  ? (float)player.getHealth() / player.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(player.getHealth()) + " / "
                      + std::to_string(player.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + 10, (int)box.y + 58, 16, Colors::text_secondary);

    std::string manaStr = "MANA: " + std::to_string(player.getMana())
                        + " / " + std::to_string(player.getMaxMana());
    DrawText(manaStr.c_str(), (int)box.x + 10, (int)box.y + 78, 16, Colors::text_secondary);

    if (player.getBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(player.getBlock());
        DrawText(blkStr.c_str(), (int)box.x + 10, (int)box.y + 98, 16,
                 Color{ 80, 130, 220, 255 });
    }
}

void GameScreen::drawEnemyBox(Rectangle box, const Enemy& enemy) const {
    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, 2.0f, Colors::card_border);

    DrawText(enemy.getName().c_str(), (int)box.x + 10, (int)box.y + 8, 20, Colors::text_primary);

    Rectangle bar = { box.x + 10, box.y + 35, box.width - 20, 18 };
    float hpRatio = (enemy.getMaxHealth() > 0)
                  ? (float)enemy.getHealth() / enemy.getMaxHealth() : 0.0f;
    drawHealthBar(bar, hpRatio);

    std::string hpStr = std::to_string(enemy.getHealth()) + " / "
                      + std::to_string(enemy.getMaxHealth()) + " HP";
    DrawText(hpStr.c_str(), (int)box.x + 10, (int)box.y + 58, 16, Colors::text_secondary);

    if (enemy.getEnemyBlock() > 0) {
        std::string blkStr = "BLOCK: " + std::to_string(enemy.getEnemyBlock());
        DrawText(blkStr.c_str(), (int)box.x + 10, (int)box.y + 78, 16,
                 Color{ 80, 130, 220, 255 });
    }
}

void GameScreen::drawCardFace(Rectangle rect, const Card& card, bool scaled) const {
    // Background
    Color bg = scaled ? Colors::button_hover : Colors::card_bg;
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, Colors::card_border);

    // --- Art area (top ART_H pixels of the card, scaled proportionally) ---
    // Scale the art height proportionally to the card's current size
    float artH = ART_H * (rect.height / (float)CARD_H);
    Rectangle artRect = { rect.x + 2, rect.y + 2, rect.width - 4, artH - 2 };

    auto texOpt = card.getArtTexture();
    if (texOpt.has_value()) {
        const Texture2D& tex = texOpt.value();
        // Draw texture scaled to fit art area
        Rectangle src  = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        DrawTexturePro(tex, src, artRect, { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        // Placeholder: dark rectangle with subtle grid
        DrawRectangleRec(artRect, Color{ 28, 30, 42, 255 });
        DrawRectangleLinesEx(artRect, 1.0f, Colors::light_bg);
    }

    // Divider between art and text area
    float divY = rect.y + artH;
    DrawLine((int)rect.x, (int)divY, (int)(rect.x + rect.width), (int)divY,
             Colors::card_border);

    // --- Text area below art ---
    int textX = (int)rect.x + 5;
    int textY = (int)divY + 4;

    // Card name (centred)
    int nameSz = scaled ? 14 : 12;
    int nameW  = MeasureText(card.getName().c_str(), nameSz);
    DrawText(card.getName().c_str(),
             (int)rect.x + ((int)rect.width - nameW) / 2,
             textY, nameSz, Colors::text_primary);
    textY += nameSz + 4;

    // Short description (small, up to 2 lines)
    const std::string& desc = card.getDescription();
    if (!desc.empty()) {
        const int descSz  = 10;
        const int lineLen = (int)rect.width / 6; // rough chars per line
        int pos = 0;
        int lines = 0;
        while (pos < (int)desc.size() && lines < 2) {
            int end = std::min(pos + lineLen, (int)desc.size());
            if (end < (int)desc.size() && desc[end] != ' ') {
                int sp = (int)desc.rfind(' ', end);
                if (sp > pos) end = sp;
            }
            DrawText(desc.substr(pos, end - pos).c_str(), textX, textY,
                     descSz, Colors::text_secondary);
            textY += descSz + 2;
            pos = end;
            while (pos < (int)desc.size() && desc[pos] == ' ') ++pos;
            ++lines;
        }
    }

    // --- Footer: cost bottom-left, power/block bottom-right ---
    int footerY = (int)(rect.y + rect.height) - 20;
    int infoSz  = scaled ? 13 : 11;

    std::string costStr = std::to_string(card.getCost()) + " mana";
    DrawText(costStr.c_str(), textX, footerY, infoSz, Colors::text_secondary);

    if (card.getPower() > 0) {
        std::string pwrStr = std::to_string(card.getPower()) + " dmg";
        int pw = MeasureText(pwrStr.c_str(), infoSz);
        DrawText(pwrStr.c_str(), (int)(rect.x + rect.width) - pw - 4,
                 footerY, infoSz, Colors::damage_color);
    }
    if (card.getBlockAmount() > 0) {
        std::string blkStr = std::to_string(card.getBlockAmount()) + " blk";
        int bw = MeasureText(blkStr.c_str(), infoSz);
        DrawText(blkStr.c_str(), (int)(rect.x + rect.width) - bw - 4,
                 footerY, infoSz, Color{ 80, 130, 220, 255 });
    }
}

void GameScreen::drawCardTooltip(const Card& card, float x, float y) const {
    const int tipW = 210, tipH = 140, pad = 8, sz = 14;
    if (y + tipH > m_height) y = (float)(m_height - tipH - 4);
    if (x < 0) x = 4.0f;

    Rectangle tip = { x, y, (float)tipW, (float)tipH };
    DrawRectangleRec(tip, Colors::light_bg);
    DrawRectangleLinesEx(tip, 2.0f, Colors::card_border);

    int tx = (int)x + pad, ty = (int)y + pad;

    // Title
    DrawText(card.getName().c_str(), tx, ty, 16, Colors::text_primary);
    ty += 22;

    // Meta line
    std::string meta = card.getType() + "  Cost:" + std::to_string(card.getCost());
    if (card.getPower()       > 0) meta += "  Dmg:" + std::to_string(card.getPower());
    if (card.getBlockAmount() > 0) meta += "  Blk:" + std::to_string(card.getBlockAmount());
    DrawText(meta.c_str(), tx, ty, sz - 1, Colors::text_secondary);
    ty += 20;

    DrawLine(tx, ty, tx + tipW - 2*pad, ty, Colors::card_border);
    ty += 6;

    // Description word-wrap
    const std::string& desc = card.getDescription();
    const int lineLen = 26;
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
    if      (dmg > 0 && blk > 0) intentColor = Color{ 200, 180, 40, 255 };
    else if (dmg > 0)             intentColor = Colors::damage_color;
    else                          intentColor = Color{ 80, 130, 220, 255 };

    const int iH = 34;
    Rectangle iRect = { enemyBox.x, enemyBox.y + enemyBox.height + 6,
                         enemyBox.width, (float)iH };
    DrawRectangleRec(iRect, Colors::light_bg);
    DrawRectangleLinesEx(iRect, 2.0f, intentColor);

    std::string desc = enemy.getIntentDescription();
    int dw = MeasureText(desc.c_str(), 15);
    DrawText(desc.c_str(),
             (int)iRect.x + ((int)iRect.width - dw) / 2,
             (int)iRect.y + (iH - 15) / 2,
             15, intentColor);

    if (mouseOver(iRect)) {
        float tipX = iRect.x + iRect.width + 4.0f;
        if (tipX + 200 > m_width) tipX = iRect.x - 204.0f;
        Rectangle tipRect = { tipX, iRect.y, 200.0f, 42.0f };
        DrawRectangleRec(tipRect, Colors::light_bg);
        DrawRectangleLinesEx(tipRect, 1.0f, intentColor);
        DrawText(desc.c_str(), (int)tipX + 6, (int)iRect.y + 8, 13, intentColor);
    }
}

bool GameScreen::mouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}
