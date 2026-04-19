#pragma once

#include "Colors.h"
#include "core/GameState.h"
#include "raylib.h"
#include <string>

// Handles all rendering and input detection for every game screen.
class GameScreen {
public:
    GameScreen(int screenWidth, int screenHeight);

    // Draw the main menu; returns clicked button (-1=none, 0=NewGame, 1=Continue, 2=Quit)
    int drawMenu();

    // Draw the combat screen; returns index of card clicked (-1 if none)
    int drawCombat(GameState& state);

    // Draw the game over screen; returns true when "Return to Menu" is clicked
    bool drawGameOver(const GameState& state);

private:
    int m_width;
    int m_height;

    // --- helpers ---

    // Draw a button rectangle with label; highlight when hovered
    void drawButton(Rectangle rect, const std::string& text, bool hovered) const;

    // Draw a health bar filling `bar` rectangle, ratio is current/max [0,1]
    void drawHealthBar(Rectangle bar, float ratio) const;

    // Draw a combatant box (player or enemy info) at position
    void drawCombatantBox(Rectangle box, const std::string& name,
                          int health, int maxHealth) const;

    // Draw a single card at `rect`; highlighted if hovered
    void drawCard(Rectangle rect, const std::string& name,
                  int cost, int power, bool hovered) const;

    // Returns true if the mouse is inside rect
    static bool mouseOver(Rectangle rect);
};
