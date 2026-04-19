#include "GameScreen.h"

#include <algorithm>
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
    const char* title    = "Medical Deckbuilder";
    const int   titleSz  = 48;
    int titleW = MeasureText(title, titleSz);
    DrawText(title,
             (m_width - titleW) / 2,
             m_height / 5,
             titleSz,
             Colors::text_primary);

    // Three centred buttons stacked with 20px gap
    const int btnW = 200, btnH = 60, gap = 20;
    const int startY = m_height / 2 - btnH - gap;
    const int centerX = (m_width - btnW) / 2;

    Rectangle rects[3] = {
        { static_cast<float>(centerX), static_cast<float>(startY),
          static_cast<float>(btnW),    static_cast<float>(btnH) },
        { static_cast<float>(centerX), static_cast<float>(startY + btnH + gap),
          static_cast<float>(btnW),    static_cast<float>(btnH) },
        { static_cast<float>(centerX), static_cast<float>(startY + 2 * (btnH + gap)),
          static_cast<float>(btnW),    static_cast<float>(btnH) },
    };
    const char* labels[3] = { "New Game", "Continue", "Quit" };

    int clicked = -1;
    for (int i = 0; i < 3; ++i) {
        bool hovered = mouseOver(rects[i]);
        drawButton(rects[i], labels[i], hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = i;
        }
    }
    return clicked;
}

// ---------------------------------------------------------------------------
// Combat Screen
// ---------------------------------------------------------------------------

int GameScreen::drawCombat(GameState& state) {
    const Player& player = state.getPlayer();
    const Enemy&  enemy  = state.getEnemy();

    // --- Combatant boxes ---
    const int boxW = 250, boxH = 150, boxY = 80;
    Rectangle playerBox = { 60.0f, static_cast<float>(boxY),
                             static_cast<float>(boxW), static_cast<float>(boxH) };
    Rectangle enemyBox  = { static_cast<float>(m_width - 60 - boxW),
                             static_cast<float>(boxY),
                             static_cast<float>(boxW), static_cast<float>(boxH) };

    drawCombatantBox(playerBox, "You",
                     player.getHealth(), player.getMaxHealth());
    drawCombatantBox(enemyBox, enemy.getName(),
                     enemy.getHealth(), enemy.getMaxHealth());

    // --- Mana display ---
    std::string manaStr = "Mana: " + std::to_string(player.getMana());
    DrawText(manaStr.c_str(), 60, boxY + boxH + 10, 22, Colors::text_secondary);

    // --- Hand ---
    const auto& hand  = player.getHand();
    const int   cardW = 80, cardH = 120, cardGap = 16;
    int         totalW = static_cast<int>(hand.size()) * (cardW + cardGap) - cardGap;
    int         handX  = (m_width - totalW) / 2;
    const int   handY  = m_height - cardH - 30;

    int clicked = -1;
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        Rectangle r = { static_cast<float>(handX + i * (cardW + cardGap)),
                        static_cast<float>(handY),
                        static_cast<float>(cardW),
                        static_cast<float>(cardH) };
        bool hovered = mouseOver(r);
        drawCard(r, hand[i].getName(), hand[i].getCost(), hand[i].getPower(), hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = i;
        }
    }

    // --- Instructions ---
    DrawText("Click a card to attack", (m_width - MeasureText("Click a card to attack", 18)) / 2,
             handY - 30, 18, Colors::text_secondary);

    return clicked;
}

// ---------------------------------------------------------------------------
// Game Over Screen
// ---------------------------------------------------------------------------

bool GameScreen::drawGameOver(const GameState& state) {
    std::string winner  = state.getWinner();
    std::string msg     = (winner == "Player") ? "You Win!" : "You Lose!";
    Color       msgColor = (winner == "Player") ? Colors::heal_color : Colors::damage_color;

    int msgSz = 60;
    int msgW  = MeasureText(msg.c_str(), msgSz);
    DrawText(msg.c_str(), (m_width - msgW) / 2, m_height / 3, msgSz, msgColor);

    // "Return to Menu" button
    const int btnW = 240, btnH = 60;
    Rectangle btn = { static_cast<float>((m_width - btnW) / 2),
                      static_cast<float>(m_height / 2 + 20),
                      static_cast<float>(btnW),
                      static_cast<float>(btnH) };
    bool hovered = mouseOver(btn);
    drawButton(btn, "Return to Menu", hovered);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void GameScreen::drawButton(Rectangle rect, const std::string& text, bool hovered) const {
    Color bg = hovered ? Colors::button_hover : Colors::button_bg;
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, Colors::card_border);

    const int fontSize = 22;
    int textW = MeasureText(text.c_str(), fontSize);
    int textX = static_cast<int>(rect.x) + (static_cast<int>(rect.width)  - textW) / 2;
    int textY = static_cast<int>(rect.y) + (static_cast<int>(rect.height) - fontSize) / 2;
    DrawText(text.c_str(), textX, textY, fontSize, Colors::text_primary);
}

void GameScreen::drawHealthBar(Rectangle bar, float ratio) const {
    // Clamp ratio
    ratio = std::max(0.0f, std::min(1.0f, ratio));

    // Background track
    DrawRectangleRec(bar, Colors::light_bg);

    // Filled portion
    Color fill = (ratio > 0.5f) ? Colors::heal_color
               : (ratio > 0.25f) ? Colors::damage_color
               : Colors::health_color;

    Rectangle filled = { bar.x, bar.y, bar.width * ratio, bar.height };
    DrawRectangleRec(filled, fill);
    DrawRectangleLinesEx(bar, 1.0f, Colors::card_border);
}

void GameScreen::drawCombatantBox(Rectangle box, const std::string& name,
                                  int health, int maxHealth) const {
    DrawRectangleRec(box, Colors::card_bg);
    DrawRectangleLinesEx(box, 2.0f, Colors::card_border);

    // Name
    const int nameSz = 22;
    DrawText(name.c_str(),
             static_cast<int>(box.x) + 10,
             static_cast<int>(box.y) + 10,
             nameSz, Colors::text_primary);

    // Health bar
    Rectangle bar = { box.x + 10, box.y + 45, box.width - 20, 20 };
    float ratio = (maxHealth > 0) ? static_cast<float>(health) / maxHealth : 0.0f;
    drawHealthBar(bar, ratio);

    // Health text
    std::string hpStr = std::to_string(health) + " / " + std::to_string(maxHealth);
    DrawText(hpStr.c_str(),
             static_cast<int>(box.x) + 10,
             static_cast<int>(box.y) + 75,
             18, Colors::text_secondary);
}

void GameScreen::drawCard(Rectangle rect, const std::string& name,
                           int cost, int power, bool hovered) const {
    Color bg = hovered ? Colors::button_hover : Colors::card_bg;
    // Lift card slightly when hovered
    if (hovered) rect.y -= 10;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, Colors::card_border);

    const int nameSz = 13;
    // Wrap name if needed (simple single-line truncation)
    int nameW = MeasureText(name.c_str(), nameSz);
    int nameX = static_cast<int>(rect.x) + (static_cast<int>(rect.width) - nameW) / 2;
    DrawText(name.c_str(), nameX, static_cast<int>(rect.y) + 8, nameSz, Colors::text_primary);

    // Cost badge (top-left circle approximated by small rect)
    std::string costStr = "Cost:" + std::to_string(cost);
    DrawText(costStr.c_str(), static_cast<int>(rect.x) + 4,
             static_cast<int>(rect.y) + 28, 11, Colors::text_secondary);

    // Power
    std::string pwrStr = "Pow:" + std::to_string(power);
    DrawText(pwrStr.c_str(), static_cast<int>(rect.x) + 4,
             static_cast<int>(rect.y) + 44, 11, Colors::damage_color);
}

bool GameScreen::mouseOver(Rectangle rect) {
    Vector2 mouse = GetMousePosition();
    return CheckCollisionPointRec(mouse, rect);
}
