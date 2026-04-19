#include "GameState.h"

#include "config/Defines.h"
#include "content/EnemyFactory.h"
#include "CardDatabase.h"
#include "gameplay/CombatResolver.h"

#include <cassert>
#include <iostream>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GameState::GameState()
    : m_phase(GamePhase::MENU)
    , m_turnPhase(TurnPhase::PLAYER_TURN)
    , m_turnNumber(CombatConfig::StartingTurnNumber)
    , m_player(CombatConfig::PlayerMaxHealth, CombatConfig::PlayerBaseMana)
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

bool GameState::startNewGame(std::string& error) {
    error.clear();
    m_player     = Player(CombatConfig::PlayerMaxHealth, CombatConfig::PlayerBaseMana);
    m_turnNumber = CombatConfig::StartingTurnNumber;
    m_turnPhase  = TurnPhase::PLAYER_TURN;
    m_lastAction = "";
    m_enemy.reset();

    // Load deck config — single source of truth for starter deck composition
    std::vector<std::string> deckConfig =
        CardDatabase::loadDeckConfigFromJSON(AssetPaths::PLAYER_STARTER_DECK, error);
    if (deckConfig.empty()) {
        if (error.empty()) {
            error = "Starter deck is empty: " + std::string(AssetPaths::PLAYER_STARTER_DECK);
        }
        return false;
    }

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
    for (const auto& id : deckConfig) {
        auto card = CardDatabase::findCard(id);
        if (!card) {
            error = "Starter deck references unknown card id: " + id;
            return false;
        }
        m_player.addCardToDeck(*card);
    }

    std::cout << "[GameState] Player deck built with "
              << m_player.getDeck().getDrawPileSize() << " cards\n";

    m_player.getDeck().shuffle();

    std::cout << "[GameState] Draw pile before drawing hand: "
              << m_player.getDeck().getDrawPileSize() << " cards\n";

    // Draw the opening hand
    for (int i = 0; i < CombatConfig::OpeningHandSize; ++i) {
        m_player.drawCardFromDeck();
    }

    std::cout << "[GameState] After drawing hand: "
              << m_player.getHand().size()             << " in hand, "
              << m_player.getDeck().getDrawPileSize()  << " in draw pile, "
              << m_player.getDeck().getDiscardPileSize() << " in discard\n";

    // Spawn first enemy with its own deck
    m_enemy = EnemyFactory::loadFromJSON(AssetPaths::DEFAULT_ENEMY, error);
    if (!m_enemy) {
        return false;
    }

    m_enemy->decideIntent();
    return true;
}

void GameState::playerDrawCard() {
    m_player.drawCardFromDeck();
}

bool GameState::playCard(int cardIndex) {
    if (!m_enemy || m_turnPhase != TurnPhase::PLAYER_TURN) {
        return false;
    }

    // Player::playCard handles mana check, hand removal, and discard
    auto optCard = m_player.playCard(cardIndex);
    if (!optCard) {
        m_lastAction = "Not enough mana!";
        return false;
    }

    const CardResolutionSummary summary =
        CombatResolver::applyPlayerCard(*optCard, m_player, *m_enemy);

    m_lastAction = CombatResolver::buildPlayerActionText(*optCard, summary);

    return true;
}

void GameState::endPlayerTurn() {
    if (m_enemy) {
        m_enemy->clearBlock();
    }
    m_turnPhase  = TurnPhase::ENEMY_TURN;
    m_lastAction = m_enemy->getName() + " – " + m_enemy->getIntentDescription() + "!";
}

void GameState::executeEnemyTurn() {
    if (!m_enemy) {
        return;
    }

    const EnemyTurnResult result = m_enemy->executeTurn(m_player);

    if (result.skipped) {
        m_lastAction = m_enemy->getName() + " could not act.";
    } else if (result.damageDealt > 0 && result.blockGained > 0) {
        m_lastAction = m_enemy->getName() + " attacked for "
                     + std::to_string(result.damageDealt) + " damage and gained "
                     + std::to_string(result.blockGained) + " block!";
    } else if (result.damageDealt > 0) {
        m_lastAction = m_enemy->getName() + " attacked for "
                     + std::to_string(result.damageDealt) + " damage!";
    } else if (result.blockGained > 0) {
        m_lastAction = m_enemy->getName() + " gained "
                     + std::to_string(result.blockGained) + " block!";
    } else {
        m_lastAction = m_enemy->getName() + " repositioned.";
    }

    m_player.clearBlock();

    // Discard entire player hand, then begin the next player turn.
    m_player.discardHand();
    m_player.startTurn();

    // Draw a fresh hand for the next player turn
    for (int i = 0; i < CombatConfig::OpeningHandSize; ++i) {
        m_player.drawCardFromDeck();
    }

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
