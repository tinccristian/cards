#pragma once

#include "Colors.h"
#include "core/Card.h"
#include "core/Deck.h"
#include "core/GameState.h"
#include "raylib.h"
#include <string>
#include <vector>

// Handles all rendering and input detection for every game screen.
class GameScreen {
public:
    GameScreen(int screenWidth, int screenHeight);

    // Draw the main menu. Returns clicked button (-1=none, 0=New Game, 1=Continue, 2=Quit).
    int drawMenu();

    // Draw the combat screen.
    //   endTurnClicked: set to true if End Turn was pressed (only during PLAYER_TURN).
    //   drawPileClicked / discardPileClicked: set if corresponding widget was clicked.
    //   Returns hand index of card clicked (-1 if none).
    int drawCombat(GameState& state, bool& endTurnClicked,
                   bool& drawPileClicked, bool& discardPileClicked);

    // Draw a full-screen overlay listing cards in a pile.
    //   title: heading text ("Draw Pile" / "Discard Pile")
    //   cards: pile contents
    //   scrollOffset: first visible row index
    //   closeClicked: set to true if the X button was pressed
    // Returns the max scroll offset (caller may clamp against it).
    int drawPileViewer(const std::string& title,
                       const std::vector<Card>& cards,
                       int scrollOffset,
                       bool& closeClicked);

    // Draw the game-over screen. Returns true when "Return to Menu" is clicked.
    bool drawGameOver(const GameState& state);

    // Pile widget rectangles (needed by main.cpp for click detection)
    Rectangle drawPileRect()    const;
    Rectangle discardPileRect() const;

private:
    int   m_width;
    int   m_height;
    int   m_hoveredCardIndex = -1;
    float m_wiggleTime       = 0.0f;

    // Card dimensions
    static constexpr int CARD_W   = 120;
    static constexpr int CARD_H   = 180;
    static constexpr int CARD_GAP = 12;
    static constexpr int ART_H    = 80;

    // Pile widget dimensions
    static constexpr int PILE_W = 100;
    static constexpr int PILE_H = 150;
    static constexpr int PILE_Y = 260; // vertical centre of widget

    // --- helpers ---
    void drawButton(Rectangle rect, const std::string& text, bool hovered) const;
    void drawHealthBar(Rectangle bar, float ratio) const;
    void drawPlayerBox(Rectangle box, const Player& player) const;
    void drawEnemyBox(Rectangle box, const Enemy& enemy) const;
    void drawCardFace(Rectangle rect, const Card& card, bool scaled) const;
    void drawCardTooltip(const Card& card, float x, float y) const;
    void drawIntentIndicator(const Enemy& enemy, Rectangle enemyBox) const;

    // Draw a pile widget (stack visual + label + count). Returns true if clicked.
    bool drawPileWidget(Rectangle rect, const std::string& label,
                        int count, Color accentColor) const;

    static float archOffset(int i, int n);
    static bool  mouseOver(Rectangle rect);
};
