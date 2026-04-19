#pragma once

#include "Colors.h"
#include "core/Card.h"
#include "core/GameState.h"
#include "raylib.h"
#include <string>

// Handles all rendering and input detection for every game screen.
class GameScreen {
public:
    GameScreen(int screenWidth, int screenHeight);

    // Draw the main menu. Returns clicked button (-1=none, 0=New Game, 1=Continue, 2=Quit).
    int drawMenu();

    // Draw the combat screen.
    //   endTurnClicked: set to true if End Turn was pressed (only during PLAYER_TURN).
    //   Returns hand index of card clicked (-1 if none).
    int drawCombat(GameState& state, bool& endTurnClicked);

    // Draw the game-over screen. Returns true when "Return to Menu" is clicked.
    bool drawGameOver(const GameState& state);

private:
    int   m_width;
    int   m_height;

    // Card hover tracking
    int   m_hoveredCardIndex = -1;

    // Wiggle animation timer (seconds elapsed, incremented each frame)
    float m_wiggleTime = 0.0f;

    // Card dimensions
    static constexpr int CARD_W = 120;
    static constexpr int CARD_H = 180;
    static constexpr int CARD_GAP = 12;
    // Art area: top portion of card
    static constexpr int ART_H  = 80;

    // --- helpers ---

    void drawButton(Rectangle rect, const std::string& text, bool hovered) const;
    void drawHealthBar(Rectangle bar, float ratio) const;
    void drawPlayerBox(Rectangle box, const Player& player) const;
    void drawEnemyBox(Rectangle box, const Enemy& enemy) const;

    // Draw a single card face. rect is the final destination (already positioned).
    // 'scaled' renders with a slightly brighter bg to indicate hover state.
    void drawCardFace(Rectangle rect, const Card& card, bool scaled) const;

    // Draw tooltip panel with full card info
    void drawCardTooltip(const Card& card, float x, float y) const;

    // Draw enemy intent indicator below enemyBox
    void drawIntentIndicator(const Enemy& enemy, Rectangle enemyBox) const;

    // Compute the arch Y offset for card index i out of n total cards.
    // Centre card is baseline (offset 0); outer cards rise upward (negative Y).
    static float archOffset(int i, int n);

    static bool mouseOver(Rectangle rect);
};
