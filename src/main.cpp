#include "app/AppSettings.h"
#include "audio/CardAudio.h"
#include "raylib.h"
#include "config/Defines.h"
#include "core/CardDatabase.h"
#include "core/Deck.h"
#include "core/GameState.h"
#include "ui/Colors.h"
#include "ui/GameScreen.h"
#include "ui/InputHandler.h"
#include "ui/UIState.h"

#include <string>

int main() {
    std::string settingsWarning;
    AppSettings appSettings = SettingsManager::loadOrCreate(settingsWarning);
    if (!settingsWarning.empty()) {
        TraceLog(LOG_WARNING, "%s", settingsWarning.c_str());
    }

    SettingsManager::clamp(appSettings);
    const ResolutionOption startupResolution =
        SettingsManager::resolutionOptions()[appSettings.resolutionIndex];
    const int screenWidth  = startupResolution.width;
    const int screenHeight = startupResolution.height;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (appSettings.vsyncEnabled ? FLAG_VSYNC_HINT : 0));
    InitWindow(screenWidth, screenHeight, "Medical Deckbuilder");
    InitAudioDevice();
    SetTargetFPS(WindowConfig::TargetFps);
    SetExitKey(KEY_NULL); // Disable ESC-closes-window; handled manually per context

    CardAudio cardAudio;
    std::string audioWarning;
    if (!cardAudio.initialize(audioWarning)) {
        TraceLog(LOG_WARNING, "%s", audioWarning.c_str());
    }
    SettingsManager::applyDisplaySettings(appSettings);
    SettingsManager::applyAudioSettings(appSettings);
    Deck::setShuffleCallback([&cardAudio]() {
        cardAudio.playShuffle();
    });

    std::string startupError;
    if (!CardDatabase::loadCardsFromJSON(AssetPaths::PLAYER_CARD_LIBRARY, startupError)) {
        TraceLog(LOG_ERROR, "%s", startupError.c_str());
        Deck::setShuffleCallback({});
        cardAudio.shutdown();
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    GameState  state;
    GameScreen screen(GetScreenWidth(), GetScreenHeight(), &cardAudio);
    UIState    uiState;
    bool       showMainMenuOptions = false;
    bool       shouldQuit = false;
    OptionsSection mainMenuOptionsSection = OptionsSection::Display;
    OptionsSection pauseOptionsSection = OptionsSection::Display;

    float       enemyTurnElapsed    = 0.0f;
    const float enemyTurnDuration   = CombatConfig::EnemyTurnDelaySecs;

    const auto saveSettings = [&appSettings]() {
        std::string saveError;
        if (!SettingsManager::save(appSettings, saveError) && !saveError.empty()) {
            TraceLog(LOG_WARNING, "%s", saveError.c_str());
        }
    };

    const auto applySettings = [&appSettings, &saveSettings]() {
        SettingsManager::clamp(appSettings);
        SettingsManager::applyDisplaySettings(appSettings);
        SettingsManager::applyAudioSettings(appSettings);
        saveSettings();
    };

    while (!WindowShouldClose() && !shouldQuit) {
        BeginDrawing();
        ClearBackground(Colors::dark_bg);

        switch (state.getPhase()) {

        // -------------------------------------------------------------------
        case GamePhase::MENU: {
            if (showMainMenuOptions) {
                if (InputHandler::getEscapePressed()) {
                    showMainMenuOptions = false;
                    break;
                }
                const AppSettings previousSettings = appSettings;
                screen.drawMenu(false);
                if (screen.drawOptionsMenu(appSettings, mainMenuOptionsSection, false) == OptionsMenuAction::Back) {
                    showMainMenuOptions = false;
                }
                if (appSettings != previousSettings) {
                    applySettings();
                }
            } else {
                const MenuAction action = screen.drawMenu();
                if (action == MenuAction::NewGame) {
                    screen.resetMapView();
                    state.setPhase(GamePhase::MAP);
                } else if (action == MenuAction::Options) {
                    showMainMenuOptions = true;
                    mainMenuOptionsSection = OptionsSection::Display;
                } else if (action == MenuAction::Quit) {
                    shouldQuit = true;
                }
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::MAP: {
            if (InputHandler::getEscapePressed()) {
                state.setPhase(GamePhase::MENU);
                break;
            }

            if (screen.drawMapScreen() == MapAction::StartCombat) {
                state.setPhase(GamePhase::NEW_GAME);
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::NEW_GAME: {
            std::string newGameError;
            if (!state.startNewGame(newGameError)) {
                TraceLog(LOG_ERROR, "%s", newGameError.c_str());
                state.setPhase(GamePhase::MENU);
                break;
            }
            uiState.setMode(UIMode::NORMAL);
            showMainMenuOptions = false;
            enemyTurnElapsed = 0.0f;
            state.setPhase(GamePhase::COMBAT);
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::COMBAT: {
            if (InputHandler::getEscapePressed()) {
                if (uiState.getCurrentMode() == UIMode::NORMAL) {
                    uiState.setMode(UIMode::PAUSED);
                } else if (uiState.getCurrentMode() == UIMode::PAUSED) {
                    uiState.setMode(UIMode::NORMAL);
                } else if (uiState.getCurrentMode() == UIMode::OPTIONS) {
                    uiState.setMode(UIMode::PAUSED);
                } else {
                    uiState.setMode(UIMode::NORMAL);
                }
            }

            if (uiState.getCurrentMode() == UIMode::PAUSED) {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);

                switch (screen.drawPauseMenu()) {
                case PauseAction::Resume:
                    uiState.setMode(UIMode::NORMAL);
                    break;
                case PauseAction::Options:
                    uiState.setMode(UIMode::OPTIONS);
                    pauseOptionsSection = OptionsSection::Display;
                    break;
                case PauseAction::MainMenu:
                    uiState.setMode(UIMode::NORMAL);
                    state.setPhase(GamePhase::MENU);
                    enemyTurnElapsed = 0.0f;
                    break;
                case PauseAction::Quit:
                    shouldQuit = true;
                    break;
                case PauseAction::None:
                    break;
                }
                break;
            }

            if (uiState.getCurrentMode() == UIMode::OPTIONS) {
                const AppSettings previousSettings = appSettings;
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);
                if (screen.drawOptionsMenu(appSettings, pauseOptionsSection, true) == OptionsMenuAction::Back) {
                    uiState.setMode(UIMode::PAUSED);
                }
                if (appSettings != previousSettings) {
                    applySettings();
                }
                break;
            }

            // Handle pile-viewer overlay (drawn on top of combat screen)
            if (uiState.getCurrentMode() == UIMode::VIEWING_DRAW_PILE
                || uiState.getCurrentMode() == UIMode::VIEWING_DISCARD_PILE) {
                // Still draw the combat screen beneath the overlay
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);

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
                    state.playCard(cardIdx);
                } else if (endTurn && !state.isGameOver()) {
                    state.endPlayerTurn();
                    enemyTurnElapsed = 0.0f;
                }
            } else {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3);
                enemyTurnElapsed += GetFrameTime();
                if (enemyTurnElapsed >= enemyTurnDuration) {
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

        if (appSettings.showFps) {
            DrawFPS(LayoutConfig::FpsCounterX, LayoutConfig::FpsCounterY);
        }

        EndDrawing();
    }

    screen.unloadAssets();
    Deck::setShuffleCallback({});
    cardAudio.shutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
