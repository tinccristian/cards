#include "raylib.h"
#include "core/CardDatabase.h"
#include "core/GameState.h"
#include "ui/GameScreen.h"
#include "ui/InputHandler.h"
#include "ui/UIState.h"

int main() {
    const int screenWidth  = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Medical Deckbuilder");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Disable ESC-closes-window; handled manually per context

    CardDatabase::loadCardsFromJSON("assets/decks/player/cards.json");

    GameState  state;
    GameScreen screen(screenWidth, screenHeight);
    UIState    uiState;

    float       enemyTurnElapsed    = 0.0f;
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
            uiState.setMode(UIMode::NORMAL);
            enemyTurnElapsed = 0.0f;
            state.setPhase(GamePhase::COMBAT);
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::COMBAT: {
            // Handle pile-viewer overlay (drawn on top of combat screen)
            if (uiState.getCurrentMode() != UIMode::NORMAL) {
                // Still draw the combat screen beneath the overlay
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3);

                // Determine which pile to show
                const Deck& deck = state.getPlayer().getDeck();
                bool isDrawViewer = (uiState.getCurrentMode() == UIMode::VIEWING_DRAW_PILE);
                const std::vector<Card>& pileCards = isDrawViewer
                    ? deck.getDrawPileCards()
                    : deck.getDiscardPileCards();
                std::string pileTitle = isDrawViewer ? "Draw Pile" : "Discard Pile";

                bool closeClicked = false;
                int maxScroll = screen.drawPileViewer(pileTitle, pileCards,
                                                      uiState.getScrollOffset(),
                                                      closeClicked);

                // Scroll input
                int scroll = InputHandler::getScrollInput();
                if (scroll < 0) uiState.scrollUp();
                if (scroll > 0) uiState.scrollDown(maxScroll);

                // Close overlay via X button or ESC (never exits the game)
                if (closeClicked || InputHandler::getEscapePressed()) {
                    uiState.setMode(UIMode::NORMAL);
                }
                break; // skip normal combat input while overlay is open
            }

            // Normal combat flow
            if (state.getTurnPhase() == TurnPhase::PLAYER_TURN) {
                bool endTurn        = false;
                bool drawClicked    = false;
                bool discardClicked = false;
                int  cardIdx = screen.drawCombat(state, endTurn, drawClicked, discardClicked);

                if (drawClicked) {
                    uiState.setMode(UIMode::VIEWING_DRAW_PILE);
                } else if (discardClicked) {
                    uiState.setMode(UIMode::VIEWING_DISCARD_PILE);
                } else if (cardIdx >= 0 && !state.isGameOver()) {
                    state.playerAttack(cardIdx);
                } else if (endTurn && !state.isGameOver()) {
                    state.endPlayerTurn();
                    enemyTurnElapsed = 0.0f;
                }
            } else {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3);
                enemyTurnElapsed += GetFrameTime();
                if (enemyTurnElapsed >= ENEMY_TURN_DURATION) {
                    state.executeEnemyTurn();
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
