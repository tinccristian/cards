#include "raylib.h"
#include "core/CardDatabase.h"
#include "core/GameState.h"
#include "ui/GameScreen.h"

int main() {
    const int screenWidth  = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Medical Deckbuilder");
    SetTargetFPS(60);

    // Load player card definitions into the database at startup.
    // Enemy decks are loaded on-demand inside GameState::startNewGame().
    CardDatabase::loadCardsFromJSON("assets/decks/player/cards.json");

    GameState  state;
    GameScreen screen(screenWidth, screenHeight);

    // Timer for the enemy-turn display delay (1 second)
    float enemyTurnElapsed = 0.0f;
    const float ENEMY_TURN_DURATION = 1.0f;

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
                EndDrawing();
                CloseWindow();
                return 0;
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::NEW_GAME: {
            state.startNewGame();
            enemyTurnElapsed = 0.0f;
            state.setPhase(GamePhase::COMBAT);
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::COMBAT: {
            if (state.getTurnPhase() == TurnPhase::PLAYER_TURN) {
                bool endTurn = false;
                int  cardIdx = screen.drawCombat(state, endTurn);

                if (cardIdx >= 0 && !state.isGameOver()) {
                    state.playerAttack(cardIdx);
                }

                if (endTurn && !state.isGameOver()) {
                    state.endPlayerTurn();      // switches to ENEMY_TURN
                    enemyTurnElapsed = 0.0f;
                }
            } else {
                // ENEMY_TURN: render the "acting" overlay for 1 second,
                // then resolve the enemy's intent.
                bool unused = false;
                screen.drawCombat(state, unused); // draws overlay, no interaction
                enemyTurnElapsed += GetFrameTime();
                if (enemyTurnElapsed >= ENEMY_TURN_DURATION) {
                    state.executeEnemyTurn();   // switches back to PLAYER_TURN
                    enemyTurnElapsed = 0.0f;
                }
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
