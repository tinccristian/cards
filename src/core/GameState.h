#pragma once

#include "Player.h"
#include "Enemy.h"
#include <memory>
#include <string>

enum class GamePhase {
    MENU,
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
    // its deck, opening hand of 5 cards, first enemy intent decided.
    void startNewGame();

    // Draw one card from player's deck to hand (reshuffles discard if needed).
    void playerDrawCard();

    // Attempt to play the card at handIndex.
    // Applies attack damage or block depending on card type.
    // Returns false if the player has insufficient mana.
    bool playerAttack(int cardIndex);

    // Signal end of player turn: sets phase to ENEMY_TURN.
    void endPlayerTurn();

    // Execute enemy intent (called after the ENEMY_TURN display delay).
    // Applies damage/block, resets player state, draws card, decides next intent,
    // increments turn, returns to PLAYER_TURN.
    void executeEnemyTurn();

    bool        isGameOver() const;
    std::string getWinner()  const;

private:
    GamePhase              m_phase;
    TurnPhase              m_turnPhase;
    int                    m_turnNumber;
    Player                 m_player;
    std::unique_ptr<Enemy> m_enemy;
    std::string            m_lastAction;

    static int randomInt(int lo, int hi);
};
