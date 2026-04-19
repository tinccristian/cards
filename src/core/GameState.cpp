#include "GameState.h"

#include <cassert>
#include <random>

// --- Static RNG ----------------------------------------------------------------

static std::mt19937& getRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

// --- GameState -----------------------------------------------------------------

GameState::GameState()
    : m_phase(GamePhase::MENU)
    , m_player(100)
{}

GamePhase GameState::getPhase() const { return m_phase; }
void      GameState::setPhase(GamePhase phase) { m_phase = phase; }

Player& GameState::getPlayer() { return m_player; }

Enemy& GameState::getEnemy() {
    assert(m_enemy && "No enemy – call startNewGame() first");
    return *m_enemy;
}

void GameState::startNewGame() {
    m_player = Player(100);
    m_enemy  = std::make_unique<Enemy>("Bacteria", 30, 5);
}

void GameState::startCombat() {
    // Reset mana at the start of each encounter
    m_player.setMana(3);
}

void GameState::initializeTestDeck() {
    Deck& deck = m_player.getDeck();

    for (int i = 0; i < 5; ++i)
        deck.addCard(Card("strike", "Strike", 1, 5, "attack", {"basic"}));

    for (int i = 0; i < 3; ++i)
        deck.addCard(Card("defend", "Defend", 1, 0, "defense", {"basic"}));

    for (int i = 0; i < 2; ++i)
        deck.addCard(Card("powerup", "Power Up", 2, 8, "attack", {"power"}));

    deck.shuffle();
}

void GameState::playerDrawCard() {
    Deck& deck = m_player.getDeck();
    if (deck.isEmpty()) {
        deck.reshuffleDiscard();
    }
    if (!deck.isEmpty()) {
        m_player.drawCard(deck.draw());
    }
}

int GameState::playerAttack(int cardIndex) {
    assert(m_enemy && "No enemy present");
    Card played  = m_player.playCard(cardIndex);
    int  damage  = played.getPower() + randomInt(0, 3);
    m_enemy->takeDamage(damage);
    m_player.getDeck().discard(played);
    return damage;
}

int GameState::enemyAttack() {
    assert(m_enemy && "No enemy present");
    int damage = m_enemy->getAttack() + randomInt(-1, 2);
    if (damage < 0) damage = 0;
    m_player.takeDamage(damage);
    return damage;
}

bool GameState::isGameOver() const {
    return m_player.isDead() || (m_enemy && m_enemy->isDead());
}

std::string GameState::getWinner() const {
    if (m_player.isDead()) return "Enemy";
    if (m_enemy && m_enemy->isDead()) return "Player";
    return "";
}

int GameState::randomInt(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(getRng());
}
