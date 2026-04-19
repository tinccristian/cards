#include "GameState.h"
#include "CardDatabase.h"

#include <cassert>
#include <iostream>
#include <random>
#include <unordered_map>

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

    // Load deck config — single source of truth for starter deck composition
    std::vector<std::string> deckConfig =
        CardDatabase::loadDeckConfigFromJSON("assets/decks/player/deck_config.json");
    assert(!deckConfig.empty() && "deck_config.json is missing or empty");

    // Tally for the summary line
    std::unordered_map<std::string, int> tally;
    for (const auto& id : deckConfig) tally[id]++;

    std::cout << "[GameState] Starter deck config: " << deckConfig.size() << " cards total (";
    bool first = true;
    for (const auto& [id, cnt] : tally) {
        if (!first) std::cout << ", ";
        std::cout << cnt << "x " << id;
        first = false;
    }
    std::cout << ")\n";

    // Build deck — exact copy per entry, no deduplication
    for (const auto& id : deckConfig)
        m_player.addCardToDeck(CardDatabase::getCard(id));

    std::cout << "[GameState] Player deck built with "
              << m_player.getDeck().getDrawPileSize() << " cards\n";

    m_player.getDeck().shuffle();

    std::cout << "[GameState] Draw pile before drawing hand: "
              << m_player.getDeck().getDrawPileSize() << " cards\n";

    // Draw opening hand of 5
    for (int i = 0; i < 5; ++i) m_player.drawCardFromDeck();

    std::cout << "[GameState] After drawing hand: "
              << m_player.getHand().size()             << " in hand, "
              << m_player.getDeck().getDrawPileSize()  << " in draw pile, "
              << m_player.getDeck().getDiscardPileSize() << " in discard\n";

    // Spawn first enemy with its own deck
    Deck enemyDeck = CardDatabase::loadEnemyDeckFromJSON("assets/decks/enemy/bacteria.json");
    m_enemy = std::make_unique<Enemy>("Bacteria", 30, 5, std::move(enemyDeck));
    m_enemy->decideIntent();
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
