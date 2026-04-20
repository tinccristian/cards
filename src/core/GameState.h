#pragma once

#include "Enemy.h"
#include "Player.h"
#include <memory>
#include <string>

enum class GamePhase {
    MENU,
    MAP,
    NEW_GAME,
    COMBAT,
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

    // --- Game flow ---

    // Initialise a fresh run: player, starter deck from CardDatabase, enemy with
    // its deck, opening hand, first enemy intent decided.
    bool startNewGame(std::string& error);
    bool startNewRun(std::string& error);
    bool startCombatForEnemy(const std::string& enemyId, std::string& error);
    void endCombat();

    // Draw one card from player's deck to hand (reshuffles discard if needed).
    void playerDrawCard();

    // Attempt to play the card at handIndex.
    // Resolves the card's effects and updates the combat log.
    // Returns false if the player has insufficient mana.
    bool playCard(int cardIndex);

    // Signal end of player turn: sets phase to ENEMY_TURN.
    void endPlayerTurn();

    // Execute enemy intent (called after the ENEMY_TURN display delay).
    // Applies damage/block, resets player state, draws card, decides next intent,
    // increments turn, returns to PLAYER_TURN.
    void executeEnemyTurn();

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
    std::string            m_lastAction;
};
