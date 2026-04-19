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

// Central game state: owns the player and current enemy, drives combat logic.
class GameState {
public:
    GameState();

    GamePhase getPhase() const;
    void      setPhase(GamePhase phase);

    Player& getPlayer();
    Enemy&  getEnemy();

    // Initialize a fresh run: full health player, starting enemy
    void startNewGame();

    // Reset encounter state (mana, etc.) for a new fight
    void startCombat();

    // Add 10 test cards to the player's deck and shuffle it
    void initializeTestDeck();

    // Draw one card from player's deck to player's hand
    void playerDrawCard();

    // Play card at handIndex: deal damage to enemy, return actual damage dealt
    int playerAttack(int cardIndex);

    // Enemy attacks the player, returns actual damage dealt
    int enemyAttack();

    bool        isGameOver()  const;
    std::string getWinner()   const;

private:
    GamePhase            m_phase;
    Player               m_player;
    std::unique_ptr<Enemy> m_enemy;  // null until startNewGame()

    // Simple RNG helper: returns uniform int in [lo, hi]
    int randomInt(int lo, int hi);
};
