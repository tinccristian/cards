#include "GameState.h"
#include "CardDatabase.h"

#include <cassert>
#include <random>

// ---------------------------------------------------------------------------
// RNG
// ---------------------------------------------------------------------------

int GameState::randomInt(int lo, int hi) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GameState::GameState()
    : m_phase(GamePhase::MENU)
    , m_turnPhase(TurnPhase::PLAYER_TURN)
    , m_turnNumber(1)
    , m_player(100, 3)
{}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

GamePhase          GameState::getPhase()      const { return m_phase;      }
void               GameState::setPhase(GamePhase p) { m_phase = p;         }
TurnPhase          GameState::getTurnPhase()  const { return m_turnPhase;  }
int                GameState::getTurnNumber() const { return m_turnNumber; }
const std::string& GameState::getLastAction() const { return m_lastAction; }

Player&       GameState::getPlayer()       { return m_player; }
const Player& GameState::getPlayer() const { return m_player; }

Enemy& GameState::getEnemy() {
    assert(m_enemy && "No enemy – call startNewGame() first");
    return *m_enemy;
}
const Enemy& GameState::getEnemy() const {
    assert(m_enemy && "No enemy – call startNewGame() first");
    return *m_enemy;
}

// ---------------------------------------------------------------------------
// Game flow
// ---------------------------------------------------------------------------

void GameState::startNewGame() {
    m_player     = Player(100, 3);
    m_turnNumber = 1;
    m_turnPhase  = TurnPhase::PLAYER_TURN;
    m_lastAction = "";

    // Build 10-card medical starter deck
    Deck& deck = m_player.getDeck();
    for (int i = 0; i < 5; ++i) deck.addCard(CardDatabase::getCard("strike"));
    for (int i = 0; i < 4; ++i) deck.addCard(CardDatabase::getCard("defend"));
    deck.addCard(CardDatabase::getCard("cleanse"));
    deck.shuffle();

    // Spawn first enemy with its own deck
    Deck enemyDeck = CardDatabase::loadEnemyDeckFromJSON("assets/decks/enemy/bacteria.json");
    m_enemy = std::make_unique<Enemy>("Bacteria", 30, 5, std::move(enemyDeck));
    m_enemy->decideIntent();

    // Draw opening hand of 5
    for (int i = 0; i < 5; ++i) m_player.drawCardFromDeck();
}

void GameState::playerDrawCard() {
    m_player.drawCardFromDeck();
}

bool GameState::playerAttack(int cardIndex) {
    assert(m_enemy && "No enemy present");
    assert(m_turnPhase == TurnPhase::PLAYER_TURN && "Not player's turn");

    // Player::playCard handles mana check, hand removal, and discard
    auto optCard = m_player.playCard(cardIndex);
    if (!optCard) {
        m_lastAction = "Not enough mana!";
        return false;
    }

    const Card& card = *optCard;

    if (card.getType() == "attack" && card.getPower() > 0) {
        int damage = card.getPower() + randomInt(0, 2);
        m_enemy->takeDamage(damage);
        m_lastAction = "You played " + card.getName()
                     + " for " + std::to_string(damage) + " damage!";
    } else if (card.getBlockAmount() > 0) {
        m_player.addBlock(card.getBlockAmount());
        m_lastAction = "You played " + card.getName()
                     + ", gained " + std::to_string(card.getBlockAmount()) + " block!";
    } else {
        m_lastAction = "You played " + card.getName() + "!";
    }

    return true;
}

void GameState::endPlayerTurn() {
    m_turnPhase  = TurnPhase::ENEMY_TURN;
    m_lastAction = m_enemy->getName() + " – " + m_enemy->getIntentDescription() + "!";
}

void GameState::executeEnemyTurn() {
    assert(m_enemy && "No enemy present");

    // Execute intent (player block absorbs damage — block is NOT reset here)
    int dmg = m_enemy->executeIntent(m_player);

    if (dmg > 0) {
        m_lastAction = m_enemy->getName() + " attacked for "
                     + std::to_string(dmg) + " damage!";
    } else {
        m_lastAction = m_enemy->getName() + " defended!";
    }

    // Discard entire player hand, reset mana
    m_player.discardHand();
    m_player.resetMana();

    // Draw a fresh hand of 5 cards (reshuffles discard automatically if needed)
    for (int i = 0; i < 5; ++i) m_player.drawCardFromDeck();

    // Enemy plans next turn
    m_enemy->decideIntent();
    m_turnNumber++;
    m_turnPhase = TurnPhase::PLAYER_TURN;
}

// ---------------------------------------------------------------------------
// Win / loss
// ---------------------------------------------------------------------------

bool GameState::isGameOver() const {
    return m_player.isDead() || (m_enemy && m_enemy->isDead());
}

std::string GameState::getWinner() const {
    if (m_player.isDead())            return "Enemy";
    if (m_enemy && m_enemy->isDead()) return "Player";
    return "";
}
