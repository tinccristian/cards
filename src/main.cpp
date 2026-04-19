#include "raylib.h"
#include "core/GameState.h"
#include "ui/GameScreen.h"

int main() {
    const int screenWidth  = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Medical Deckbuilder");
    SetTargetFPS(60);

    GameState  state;
    GameScreen screen(screenWidth, screenHeight);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({ 18, 18, 24, 255 });

        switch (state.getPhase()) {

        // -------------------------------------------------------------------
        case GamePhase::MENU: {
            int btn = screen.drawMenu();
            if (btn == 0) {
                state.setPhase(GamePhase::NEW_GAME);
            } else if (btn == 2) {
                // Quit
                EndDrawing();
                CloseWindow();
                return 0;
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::NEW_GAME: {
            state.startNewGame();
            state.initializeTestDeck();
            state.startCombat();
            // Draw opening hand of 5 cards
            for (int i = 0; i < 5; ++i) {
                state.playerDrawCard();
            }
            state.setPhase(GamePhase::COMBAT);
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::COMBAT: {
            int cardIdx = screen.drawCombat(state);

            if (cardIdx >= 0 && cardIdx < static_cast<int>(state.getPlayer().getHand().size())) {
                // Player plays a card
                state.playerAttack(cardIdx);

                // Enemy retaliates if still alive
                if (!state.getEnemy().isDead()) {
                    state.enemyAttack();
                }

                // Draw a replacement card if deck is not exhausted
                state.playerDrawCard();
            }

            if (state.isGameOver()) {
                state.setPhase(GamePhase::GAME_OVER);
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::GAME_OVER: {
            bool returnToMenu = screen.drawGameOver(state);
            if (returnToMenu) {
                state.setPhase(GamePhase::MENU);
            }
            break;
        }

        } // switch

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
