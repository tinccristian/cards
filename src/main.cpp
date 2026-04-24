#include "app/AppSettings.h"
#include "audio/CardAudio.h"
#include "raylib.h"
#include "config/Defines.h"
#include "content/MapData.h"
#include "core/CardDatabase.h"
#include "core/Deck.h"
#include "core/GameState.h"
#include "gameplay/MapRunState.h"
#include "ui/Colors.h"
#include "ui/GameScreen.h"
#include "ui/InputHandler.h"
#include "ui/ScreenTransition.h"
#include "ui/UIState.h"

#include <algorithm>
#include <functional>
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

    MapData activeMap;
    if (!MapContentLoader::loadMap(AssetPaths::MAP_NODE_TYPES, AssetPaths::LEG_MAP_DATA, activeMap, startupError)) {
        TraceLog(LOG_ERROR, "%s", startupError.c_str());
        Deck::setShuffleCallback({});
        cardAudio.shutdown();
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    GameState  state;
    GameScreen screen(GetScreenWidth(), GetScreenHeight(), &cardAudio);
    MapRunState mapRun;
    ScreenTransition sceneTransition;
    RenderTexture2D sceneFrame = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    UIState    uiState;
    bool       showMainMenuOptions = false;
    bool       shouldQuit = false;
    OptionsSection mainMenuOptionsSection = OptionsSection::Display;
    OptionsSection pauseOptionsSection = OptionsSection::Display;

    float       enemyTurnElapsed    = 0.0f;
    const float enemyTurnDuration   = CombatConfig::EnemyTurnDelaySecs;
    float       deathSlowMoTimer    = 0.0f;
    bool        enemyDeathSoundPlayed = false;
    bool        playerDeathSoundPlayed = false;
    bool        gameOverSoundPlayed = false;

    if (!sceneTransition.load(AssetPaths::TRANSITION_SHADER)) {
        TraceLog(LOG_WARNING, "Screen transition shader unavailable: %s", AssetPaths::TRANSITION_SHADER);
    }

    std::function<void()> pendingSceneSwitch = {};

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

    const auto beginSceneTransition = [&sceneTransition, &pendingSceneSwitch](std::function<void()> action) {
        if (!sceneTransition.isLoaded()) {
            action();
            return;
        }
        if (sceneTransition.isActive()) {
            return;
        }
        pendingSceneSwitch = std::move(action);
        sceneTransition.start();
    };

    const auto ensureSceneFrameSize = [&sceneFrame]() {
        const int width = GetScreenWidth() > 0 ? GetScreenWidth() : 1;
        const int height = GetScreenHeight() > 0 ? GetScreenHeight() : 1;
        if (sceneFrame.id != 0 && sceneFrame.texture.width == width && sceneFrame.texture.height == height) {
            return;
        }

        if (sceneFrame.id != 0) {
            UnloadRenderTexture(sceneFrame);
        }

        sceneFrame = LoadRenderTexture(width, height);
    };

    const auto currentTurnLabel = [&state]() {
        return std::string("Turn ") + std::to_string(state.getTurnNumber());
    };

    const auto handleRunHudAction = [&](RunHudAction action) {
        switch (action) {
        case RunHudAction::ToggleMap:
            if (state.getPhase() == GamePhase::MAP) {
                return;
            }
            if (uiState.getCurrentMode() == UIMode::VIEWING_MAP) {
                uiState.setMode(UIMode::NORMAL);
            } else {
                uiState.setMode(UIMode::VIEWING_MAP);
            }
            break;
        case RunHudAction::Back:
            uiState.setMode(UIMode::NORMAL);
            break;
        case RunHudAction::ViewDeck:
            if (uiState.getCurrentMode() == UIMode::VIEWING_RUN_DECK) {
                uiState.setMode(UIMode::NORMAL);
            } else {
                uiState.setMode(UIMode::VIEWING_RUN_DECK);
            }
            break;
        case RunHudAction::None:
            break;
        }
    };

    while (!WindowShouldClose() && !shouldQuit) {
        ensureSceneFrameSize();
        cardAudio.update(GetFrameTime());
        deathSlowMoTimer = std::max(0.0f, deathSlowMoTimer - GetFrameTime());
        const bool deathSlowMoActive = (state.getPhase() == GamePhase::COMBAT) && deathSlowMoTimer > 0.0f;
        screen.setTimeScale(deathSlowMoActive ? LayoutConfig::DeathSlowMoScale : 1.0f);
        const bool allowSceneInteraction = !sceneTransition.blocksInteraction();

        BeginTextureMode(sceneFrame);
        ClearBackground(Colors::dark_bg);

        switch (state.getPhase()) {

        // -------------------------------------------------------------------
        case GamePhase::MENU: {
            if (showMainMenuOptions) {
                if (allowSceneInteraction && InputHandler::getEscapePressed()) {
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
                const MenuAction action = screen.drawMenu(allowSceneInteraction);
                if (action == MenuAction::NewGame) {
                    std::string newRunError;
                    if (!state.startNewRun(newRunError)) {
                        TraceLog(LOG_ERROR, "%s", newRunError.c_str());
                        break;
                    }
                    mapRun.initialize(activeMap);
                    beginSceneTransition([&screen, &state]() {
                        screen.resetMapView();
                        screen.resetCombatEffects();
                        state.setPhase(GamePhase::MAP);
                    });
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
            if (allowSceneInteraction && InputHandler::getEscapePressed()
                && uiState.getCurrentMode() == UIMode::NORMAL) {
                state.setPhase(GamePhase::MENU);
                mapRun = MapRunState{};
                break;
            }

            if (allowSceneInteraction && uiState.getCurrentMode() == UIMode::VIEWING_RUN_DECK
                && InputHandler::getEscapePressed()) {
                uiState.setMode(UIMode::NORMAL);
            }

            const bool mapInteractive = allowSceneInteraction && uiState.getCurrentMode() == UIMode::NORMAL;
            const int selectedNodeIndex = screen.drawMapScreen(activeMap, mapRun, mapInteractive);
            handleRunHudAction(screen.drawRunHud(state.getPlayer(), "Map", allowSceneInteraction));

            if (uiState.getCurrentMode() == UIMode::VIEWING_RUN_DECK) {
                bool closeClicked = false;
                const int maxScroll = screen.drawPileViewer("Run Deck",
                                                            state.getPlayer().getOwnedCards(),
                                                            uiState.getScrollOffset(),
                                                            closeClicked);
                const int scroll = InputHandler::getScrollInput();
                if (scroll < 0) uiState.scrollUp();
                if (scroll > 0) uiState.scrollDown(maxScroll);
                if (closeClicked || (allowSceneInteraction && InputHandler::getEscapePressed())) {
                    uiState.setMode(UIMode::NORMAL);
                }
                break;
            }

            if (allowSceneInteraction && selectedNodeIndex >= 0 && uiState.getCurrentMode() == UIMode::NORMAL) {
                beginSceneTransition([&, selectedNodeIndex]() {
                    if (!mapRun.selectNode(activeMap, selectedNodeIndex)) {
                        return;
                    }

                    const std::string encounterId = activeMap.nodes[selectedNodeIndex].encounterId;
                    std::string combatError;
                    if (!state.startCombatForEnemy(encounterId, combatError)) {
                        TraceLog(LOG_ERROR, "%s", combatError.c_str());
                        mapRun.clearActiveNode();
                        return;
                    }

                    uiState.setMode(UIMode::NORMAL);
                    enemyTurnElapsed = 0.0f;
                    deathSlowMoTimer = 0.0f;
                    screen.resetCombatEffects();
                    enemyDeathSoundPlayed = false;
                    playerDeathSoundPlayed = false;
                    gameOverSoundPlayed = false;
                    state.setPhase(GamePhase::COMBAT);
                });
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::NEW_GAME: {
            // Legacy phase kept for compatibility; current flow transitions
            // directly from menu to map/combat setup.
            state.setPhase(GamePhase::MENU);
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::COMBAT: {
            if (allowSceneInteraction && uiState.getCurrentMode() == UIMode::VIEWING_MAP
                && InputHandler::getEscapePressed()) {
                uiState.setMode(UIMode::NORMAL);
            }

            if (uiState.getCurrentMode() == UIMode::VIEWING_MAP) {
                screen.drawMapScreen(activeMap, mapRun, allowSceneInteraction);
                handleRunHudAction(screen.drawRunHud(state.getPlayer(), "Map", allowSceneInteraction, true, true, true));
                break;
            }

            if (allowSceneInteraction && InputHandler::getEscapePressed()) {
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

            if (allowSceneInteraction && uiState.getCurrentMode() == UIMode::PAUSED) {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);
                screen.drawRunHud(state.getPlayer(), currentTurnLabel(), false);

                switch (screen.drawPauseMenu()) {
                case PauseAction::Resume:
                    uiState.setMode(UIMode::NORMAL);
                    break;
                case PauseAction::Options:
                    uiState.setMode(UIMode::OPTIONS);
                    pauseOptionsSection = OptionsSection::Display;
                    break;
                case PauseAction::MainMenu:
                    beginSceneTransition([&]() {
                        uiState.setMode(UIMode::NORMAL);
                        state.endCombat();
                        screen.resetCombatEffects();
                        state.setPhase(GamePhase::MENU);
                        enemyTurnElapsed = 0.0f;
                    });
                    break;
                case PauseAction::Quit:
                    shouldQuit = true;
                    break;
                case PauseAction::None:
                    break;
                }
                break;
            }

            if (allowSceneInteraction && uiState.getCurrentMode() == UIMode::OPTIONS) {
                const AppSettings previousSettings = appSettings;
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);
                screen.drawRunHud(state.getPlayer(), currentTurnLabel(), false);
                if (screen.drawOptionsMenu(appSettings, pauseOptionsSection, true) == OptionsMenuAction::Back) {
                    uiState.setMode(UIMode::PAUSED);
                }
                if (appSettings != previousSettings) {
                    applySettings();
                }
                break;
            }

            // Handle pile-viewer overlay (drawn on top of combat screen)
            if (allowSceneInteraction && (uiState.getCurrentMode() == UIMode::VIEWING_DRAW_PILE
                || uiState.getCurrentMode() == UIMode::VIEWING_DISCARD_PILE
                || uiState.getCurrentMode() == UIMode::VIEWING_RUN_DECK)) {
                // Still draw the combat screen beneath the overlay
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);
                handleRunHudAction(screen.drawRunHud(state.getPlayer(), currentTurnLabel(), allowSceneInteraction));

                // Determine which pile to show
                const Deck& deck = state.getPlayer().getDeck();
                const std::vector<Card>* pileCards = nullptr;
                std::string pileTitle;
                if (uiState.getCurrentMode() == UIMode::VIEWING_DRAW_PILE) {
                    pileCards = &deck.getDrawPileCards();
                    pileTitle = "Draw Pile";
                } else if (uiState.getCurrentMode() == UIMode::VIEWING_DISCARD_PILE) {
                    pileCards = &deck.getDiscardPileCards();
                    pileTitle = "Discard Pile";
                } else {
                    pileCards = &state.getPlayer().getOwnedCards();
                    pileTitle = "Run Deck";
                }

                bool closeClicked = false;
                int maxScroll = screen.drawPileViewer(pileTitle, *pileCards,
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

            if (!allowSceneInteraction) {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3, false);
                screen.drawRunHud(state.getPlayer(), currentTurnLabel(), false);
                break;
            }

            // Normal combat flow
            if (state.getTurnPhase() == TurnPhase::PLAYER_TURN) {
                bool endTurn        = false;
                bool drawClicked    = false;
                bool discardClicked = false;
                // Disable interaction once the enemy is dead so nothing fires during death anim.
                bool allowInteraction = !state.isGameOver();
                int  cardIdx = screen.drawCombat(state, endTurn, drawClicked, discardClicked, allowInteraction);
                const RunHudAction hudAction = screen.drawRunHud(state.getPlayer(), currentTurnLabel(), allowSceneInteraction);
                if (hudAction != RunHudAction::None) {
                    handleRunHudAction(hudAction);
                    break;
                }

                if (drawClicked) {
                    uiState.setMode(UIMode::VIEWING_DRAW_PILE);
                } else if (discardClicked) {
                    uiState.setMode(UIMode::VIEWING_DISCARD_PILE);
                } else if (cardIdx >= 0 && !state.isGameOver()) {
                    const int playerHealthBefore = state.getPlayer().getHealth();
                    const auto playResult = state.playCard(cardIdx);
                    if (playResult.has_value()) {
                        float hitSoundDelay = 0.0f;
                        if (playResult->blockGained > 0) {
                            cardAudio.playArmor();
                        }
                        int hitIndex = 0;
                        for (const DamageBreakdown& event : playResult->enemyDamageEvents) {
                            if (event.blocked > 0) {
                                screen.addDamageNumber(DamageNumberTarget::Enemy,
                                                       event.blocked,
                                                       DamageNumberStyle::Block,
                                                       hitIndex++,
                                                       hitSoundDelay);
                                cardAudio.scheduleArmorHit(hitSoundDelay);
                                hitSoundDelay += LayoutConfig::HitEventDelayStep;
                            }
                            if (event.health > 0) {
                                screen.addDamageNumber(DamageNumberTarget::Enemy,
                                                       event.health,
                                                       DamageNumberStyle::Health,
                                                       hitIndex++,
                                                       hitSoundDelay);
                                cardAudio.scheduleDamage(hitSoundDelay);
                                cardAudio.scheduleEnemyHurt(hitSoundDelay);
                                hitSoundDelay += LayoutConfig::HitEventDelayStep;
                            }
                        }
                        hitIndex = 0;
                        for (const DamageBreakdown& event : playResult->playerDamageEvents) {
                            if (event.blocked > 0) {
                                screen.addDamageNumber(DamageNumberTarget::Player,
                                                       event.blocked,
                                                       DamageNumberStyle::Block,
                                                       hitIndex++,
                                                       hitSoundDelay);
                                cardAudio.scheduleArmorHit(hitSoundDelay);
                                hitSoundDelay += LayoutConfig::HitEventDelayStep;
                            }
                            if (event.health > 0) {
                                screen.addDamageNumber(DamageNumberTarget::Player,
                                                       event.health,
                                                       DamageNumberStyle::Health,
                                                       hitIndex++,
                                                       hitSoundDelay);
                                cardAudio.scheduleDamage(hitSoundDelay);
                                hitSoundDelay += LayoutConfig::HitEventDelayStep;
                            }
                        }
                        if (!enemyDeathSoundPlayed && state.getEnemy().isDead()) {
                            cardAudio.playEnemyDeath();
                            deathSlowMoTimer = LayoutConfig::DeathSlowMoDuration;
                            enemyDeathSoundPlayed = true;
                        }
                        if (!playerDeathSoundPlayed && playerHealthBefore > 0 && state.getPlayer().isDead()) {
                            cardAudio.playPlayerDeath();
                            deathSlowMoTimer = LayoutConfig::DeathSlowMoDuration;
                            playerDeathSoundPlayed = true;
                        }
                    }
                } else if (endTurn && !state.isGameOver()) {
                    state.endPlayerTurn();
                    enemyTurnElapsed = 0.0f;
                }
            } else {
                bool unused1 = false, unused2 = false, unused3 = false;
                screen.drawCombat(state, unused1, unused2, unused3);
                const RunHudAction hudAction = screen.drawRunHud(state.getPlayer(), currentTurnLabel(), allowSceneInteraction);
                if (hudAction != RunHudAction::None) {
                    handleRunHudAction(hudAction);
                    break;
                }
                enemyTurnElapsed += GetFrameTime();
                if (enemyTurnElapsed >= enemyTurnDuration) {
                    const int playerHealthBefore = state.getPlayer().getHealth();
                    const EnemyTurnResult enemyResult = state.executeEnemyTurn();
                    float hitSoundDelay = 0.0f;
                    if (enemyResult.blockGained > 0) {
                        cardAudio.playArmor();
                    }
                    if (enemyResult.maxHealthGained > 0) {
                        cardAudio.playEnemyBuff();
                    }
                    int hitIndex = 0;
                    for (const DamageBreakdown& event : enemyResult.playerDamageEvents) {
                        if (event.blocked > 0) {
                            screen.addDamageNumber(DamageNumberTarget::Player,
                                                   event.blocked,
                                                   DamageNumberStyle::Block,
                                                   hitIndex++,
                                                   hitSoundDelay);
                            cardAudio.scheduleArmorHit(hitSoundDelay);
                            hitSoundDelay += LayoutConfig::HitEventDelayStep;
                        }
                        if (event.health > 0) {
                            screen.addDamageNumber(DamageNumberTarget::Player,
                                                   event.health,
                                                   DamageNumberStyle::Health,
                                                   hitIndex++,
                                                   hitSoundDelay);
                            cardAudio.scheduleDamage(hitSoundDelay);
                            hitSoundDelay += LayoutConfig::HitEventDelayStep;
                        }
                    }
                    hitIndex = 0;
                    for (const DamageBreakdown& event : enemyResult.enemyDamageEvents) {
                        if (event.blocked > 0) {
                            screen.addDamageNumber(DamageNumberTarget::Enemy,
                                                   event.blocked,
                                                   DamageNumberStyle::Block,
                                                   hitIndex++,
                                                   hitSoundDelay);
                            cardAudio.scheduleArmorHit(hitSoundDelay);
                            hitSoundDelay += LayoutConfig::HitEventDelayStep;
                        }
                        if (event.health > 0) {
                            screen.addDamageNumber(DamageNumberTarget::Enemy,
                                                   event.health,
                                                   DamageNumberStyle::Health,
                                                   hitIndex++,
                                                   hitSoundDelay);
                            cardAudio.scheduleDamage(hitSoundDelay);
                            cardAudio.scheduleEnemyHurt(hitSoundDelay);
                            hitSoundDelay += LayoutConfig::HitEventDelayStep;
                        }
                    }
                    if (enemyResult.turnStartDamage.blocked > 0) {
                        screen.addDamageNumber(DamageNumberTarget::Player,
                                               enemyResult.turnStartDamage.blocked,
                                               DamageNumberStyle::Block,
                                               static_cast<int>(enemyResult.playerDamageEvents.size()),
                                               hitSoundDelay);
                        cardAudio.scheduleArmorHit(hitSoundDelay);
                        hitSoundDelay += LayoutConfig::HitEventDelayStep;
                    }
                    if (enemyResult.turnStartDamage.health > 0) {
                        screen.addDamageNumber(DamageNumberTarget::Player,
                                               enemyResult.turnStartDamage.health,
                                               DamageNumberStyle::Poison,
                                               static_cast<int>(enemyResult.playerDamageEvents.size()) + (enemyResult.turnStartDamage.blocked > 0 ? 1 : 0),
                                               hitSoundDelay);
                        cardAudio.scheduleDamage(hitSoundDelay);
                        hitSoundDelay += LayoutConfig::HitEventDelayStep;
                    }
                    if (!playerDeathSoundPlayed && playerHealthBefore > 0 && state.getPlayer().isDead()) {
                        cardAudio.playPlayerDeath();
                        deathSlowMoTimer = LayoutConfig::DeathSlowMoDuration;
                        playerDeathSoundPlayed = true;
                    }
                    if (!state.isGameOver() && state.getTurnPhase() == TurnPhase::PLAYER_TURN) {
                        cardAudio.playNextTurn();
                        screen.showNextTurnVignette();
                    }
                    enemyTurnElapsed = 0.0f;
                }
            }

            if (state.isGameOver()) {
                if (state.isPlayerDefeated() && screen.isPlayerDeathDissolveComplete()) {
                    beginSceneTransition([&state]() {
                        state.setPhase(GamePhase::GAME_OVER);
                    });
                } else if (state.isCombatWon() && screen.isEnemyDeathAnimDone()) {
                    std::string rewardError;
                    if (!state.prepareCombatRewards(rewardError)) {
                        TraceLog(LOG_ERROR, "%s", rewardError.c_str());
                    } else {
                        cardAudio.playRewardEnter();
                        uiState.setMode(UIMode::NORMAL);
                        enemyTurnElapsed = 0.0f;
                        deathSlowMoTimer = 0.0f;
                        state.setPhase(GamePhase::REWARDS);
                    }
                }
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::REWARDS: {
            if (allowSceneInteraction && uiState.getCurrentMode() == UIMode::VIEWING_MAP
                && InputHandler::getEscapePressed()) {
                uiState.setMode(UIMode::NORMAL);
            }

            if (uiState.getCurrentMode() == UIMode::VIEWING_MAP) {
                screen.drawMapScreen(activeMap, mapRun, allowSceneInteraction);
                handleRunHudAction(screen.drawRunHud(state.getPlayer(), "Map", allowSceneInteraction, true, true, true));
                break;
            }

            bool unused1 = false, unused2 = false, unused3 = false;
            screen.drawCombat(state, unused1, unused2, unused3, false);
            const RunHudAction rewardHudAction = screen.drawRunHud(state.getPlayer(), "Rewards", allowSceneInteraction);
            if (rewardHudAction != RunHudAction::None) {
                handleRunHudAction(rewardHudAction);
                break;
            }

            if (uiState.getCurrentMode() == UIMode::VIEWING_RUN_DECK) {
                bool closeClicked = false;
                const int maxScroll = screen.drawPileViewer("Run Deck",
                                                            state.getPlayer().getOwnedCards(),
                                                            uiState.getScrollOffset(),
                                                            closeClicked);
                const int scroll = InputHandler::getScrollInput();
                if (scroll < 0) uiState.scrollUp();
                if (scroll > 0) uiState.scrollDown(maxScroll);
                if (closeClicked || (allowSceneInteraction && InputHandler::getEscapePressed())) {
                    uiState.setMode(UIMode::NORMAL);
                }
                break;
            }

            RewardState& rewards = state.getRewardState();
            if (rewards.isChoosingCard()) {
                if (allowSceneInteraction && InputHandler::getEscapePressed()) {
                    rewards.closeCardChoice();
                    break;
                }

                const int selectedRewardCard = screen.drawRewardCardChoice(rewards, allowSceneInteraction);
                if (allowSceneInteraction && selectedRewardCard == GameScreen::RewardChoiceSkip) {
                    state.skipRewardCard();
                } else if (allowSceneInteraction && selectedRewardCard >= 0) {
                    if (state.claimRewardCard(selectedRewardCard)) {
                        cardAudio.playCardPicked();
                    }
                }
            } else {
                int rewardMaxScroll = 0;
                switch (screen.drawRewardPopup(rewards,
                                               state.getPlayer().getGold(),
                                               uiState.getScrollOffset(),
                                               rewardMaxScroll,
                                               allowSceneInteraction)) {
                case RewardPopupAction::CollectGold:
                    if (state.collectRewardGold() > 0) {
                        cardAudio.playCoinPicked();
                    }
                    break;
                case RewardPopupAction::OpenCardChoice:
                    rewards.openCardChoice();
                    break;
                case RewardPopupAction::Continue:
                    beginSceneTransition([&]() {
                        mapRun.completeActiveNode(activeMap);
                        state.endCombat();
                        uiState.setMode(UIMode::NORMAL);
                        screen.resetCombatEffects();
                        enemyTurnElapsed = 0.0f;
                        state.setPhase(GamePhase::MAP);
                    });
                    break;
                case RewardPopupAction::None:
                    break;
                }
                const int rewardScroll = InputHandler::getScrollInput();
                if (rewardScroll < 0) uiState.scrollUp();
                if (rewardScroll > 0) uiState.scrollDown(rewardMaxScroll);
                uiState.clampScroll(rewardMaxScroll);
            }
            break;
        }

        // -------------------------------------------------------------------
        case GamePhase::GAME_OVER: {
            if (!gameOverSoundPlayed) {
                cardAudio.playGameOver();
                gameOverSoundPlayed = true;
            }

            const GameOverAction action = screen.drawGameOver(state);
            if (allowSceneInteraction && action == GameOverAction::NewRun) {
                beginSceneTransition([&]() {
                    std::string newRunError;
                    if (!state.startNewRun(newRunError)) {
                        TraceLog(LOG_ERROR, "%s", newRunError.c_str());
                        return;
                    }
                    mapRun.initialize(activeMap);
                    screen.resetMapView();
                    screen.resetCombatEffects();
                    uiState.setMode(UIMode::NORMAL);
                    enemyTurnElapsed = 0.0f;
                    deathSlowMoTimer = 0.0f;
                    enemyDeathSoundPlayed = false;
                    playerDeathSoundPlayed = false;
                    gameOverSoundPlayed = false;
                    state.setPhase(GamePhase::MAP);
                });
            } else if (allowSceneInteraction && action == GameOverAction::MainMenu) {
                beginSceneTransition([&]() {
                    uiState.setMode(UIMode::NORMAL);
                    showMainMenuOptions = false;
                    screen.resetCombatEffects();
                    deathSlowMoTimer = 0.0f;
                    enemyDeathSoundPlayed = false;
                    playerDeathSoundPlayed = false;
                    gameOverSoundPlayed = false;
                    state.setPhase(GamePhase::MENU);
                });
            }
            break;
        }

        } // switch

        if (appSettings.showFps) {
            DrawFPS(LayoutConfig::FpsCounterX, LayoutConfig::FpsCounterY);
        }

        EndTextureMode();

        if (sceneTransition.needsFromCapture()) {
            sceneTransition.captureFrom(sceneFrame.texture);
            if (pendingSceneSwitch) {
                pendingSceneSwitch();
                pendingSceneSwitch = {};
            }
        }
        sceneTransition.update(GetFrameTime());

        BeginDrawing();
        ClearBackground(Colors::dark_bg);

        DrawTexturePro(
            sceneFrame.texture,
            { 0.0f, 0.0f, (float)sceneFrame.texture.width, -(float)sceneFrame.texture.height },
            { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() },
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
        if (sceneTransition.isActive()) {
            sceneTransition.drawOverlay(GetScreenWidth(), GetScreenHeight());
        }

        EndDrawing();
    }

    screen.unloadAssets();
    sceneTransition.unload();
    if (sceneFrame.id != 0) {
        UnloadRenderTexture(sceneFrame);
    }
    Deck::setShuffleCallback({});
    cardAudio.shutdown();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
