#pragma once

#include "Enemy.h"
#include "Player.h"
#include "gameplay/CombatResolver.h"
#include "gameplay/NoahEventState.h"
#include "gameplay/RewardState.h"
#include <optional>
#include <memory>
#include <string>

enum class GamePhase {
    MENU,
    MAP,
    NEW_GAME,
    COMBAT,
    EVENT,
    REWARDS,
    GAME_OVER
};

enum class TurnPhase {
    PLAYER_TURN,
    ENEMY_TURN,
    GAME_OVER_PHASE
};

// Central game state: owns the player and current enemy, drives combat logic.
class GameState {
public:
    GameState();

    // --- Phase management ---
    GamePhase getPhase()     const;
    void      setPhase(GamePhase phase);
    TurnPhase getTurnPhase() const;
    int       getTurnNumber() const;

    // Last action text for the combat log
    const std::string& getLastAction() const;

    // --- Accessors ---
    Player&       getPlayer();
    const Player& getPlayer() const;
    Enemy&        getEnemy();
    const Enemy&  getEnemy()  const;
    RewardState&       getRewardState();
    const RewardState& getRewardState() const;
    NoahEventState&       getNoahEventState();
    const NoahEventState& getNoahEventState() const;

    // --- Game flow ---

    // Initialise a fresh run: player, starter deck from CardDatabase, enemy with
    // its deck, opening hand, first enemy intent decided.
    bool startNewGame(std::string& error);
    bool startNewRun(std::string& error);
    bool startCombatForEnemy(const std::string& enemyId, std::string& error);
    bool beginNoahEvent(std::string& error);
    bool chooseNoahGainCards(std::string& error);
    bool chooseNoahTransform(std::string& error);
    bool confirmNoahTransform(std::string& error);
    bool chooseNoahRemoveRandom(std::string& error);
    bool claimSelectedNoahCards(std::string& error);
    void endNoahEvent();
    bool prepareCombatRewards(std::string& error);
    int  collectRewardGold();
    bool claimRewardCard(int rewardIndex);
    bool skipRewardCard();
    void endCombat();

    // Draw one card from player's deck to hand (reshuffles discard if needed).
    void playerDrawCard();

    // Attempt to play the card at handIndex.
    // Resolves the card's effects and updates the combat log.
    // Returns nullopt if the player has insufficient mana.
    std::optional<CardResolutionSummary> playCard(int cardIndex);

    // Signal end of player turn: sets phase to ENEMY_TURN.
    void endPlayerTurn();

    // Execute enemy intent (called after the ENEMY_TURN display delay).
    // Applies damage/block, resets player state, draws card, decides next intent,
    // increments turn, returns to PLAYER_TURN.
    EnemyTurnResult executeEnemyTurn();

    bool        isGameOver() const;
    bool        isCombatWon() const;
    bool        isPlayerDefeated() const;
    std::string getWinner()  const;

private:
    GamePhase              m_phase;
    TurnPhase              m_turnPhase;
    int                    m_turnNumber;
    Player                 m_player;
    std::unique_ptr<Enemy> m_enemy;
    RewardState            m_rewardState;
    NoahEventState         m_noahEventState;
    std::string            m_lastAction;
};
