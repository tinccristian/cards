#include "GameState.h"

#include "config/Defines.h"
#include "content/EnemyFactory.h"
#include "CardDatabase.h"
#include "gameplay/CombatResolver.h"

#include <cassert>
#include <iostream>
#include <random>
#include <unordered_set>
#include <unordered_map>

namespace {

CardTier rollRewardTier(std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float roll = dist(rng);
    if (roll < LuckConfig::CommonCardDropRate) {
        return CardTier::Common;
    }
    if (roll < LuckConfig::CommonCardDropRate + LuckConfig::UncommonCardDropRate) {
        return CardTier::Uncommon;
    }
    return CardTier::Rare;
}

const Card* pickRandomCardFromPool(const std::vector<const Card*>& pool,
                                   std::unordered_set<std::string>& usedIds,
                                   std::mt19937& rng) {
    std::vector<const Card*> available;
    available.reserve(pool.size());
    for (const Card* card : pool) {
        if (usedIds.find(card->getId()) == usedIds.end()) {
            available.push_back(card);
        }
    }

    if (available.empty()) {
        return nullptr;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(available.size()) - 1);
    const Card* selected = available[dist(rng)];
    usedIds.insert(selected->getId());
    return selected;
}

std::vector<Card> generateRewardCards(int count) {
    std::vector<Card> results;
    const auto& allCards = CardDatabase::getAllCards();
    if (count <= 0 || allCards.empty()) {
        return results;
    }

    std::vector<const Card*> commonCards;
    std::vector<const Card*> uncommonCards;
    std::vector<const Card*> rareCards;
    std::vector<const Card*> fallbackCards;
    commonCards.reserve(allCards.size());
    uncommonCards.reserve(allCards.size());
    rareCards.reserve(allCards.size());
    fallbackCards.reserve(allCards.size());

    for (const Card& card : allCards) {
        fallbackCards.push_back(&card);
        switch (card.getTier()) {
        case CardTier::Common:   commonCards.push_back(&card); break;
        case CardTier::Uncommon: uncommonCards.push_back(&card); break;
        case CardTier::Rare:     rareCards.push_back(&card); break;
        }
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::unordered_set<std::string> usedIds;

    while (static_cast<int>(results.size()) < count && usedIds.size() < allCards.size()) {
        const std::vector<const Card*>* preferredPool = nullptr;
        switch (rollRewardTier(rng)) {
        case CardTier::Common: preferredPool = &commonCards; break;
        case CardTier::Uncommon: preferredPool = &uncommonCards; break;
        case CardTier::Rare: preferredPool = &rareCards; break;
        }

        const Card* selected = pickRandomCardFromPool(*preferredPool, usedIds, rng);
        if (selected == nullptr) {
            selected = pickRandomCardFromPool(fallbackCards, usedIds, rng);
        }
        if (selected == nullptr) {
            break;
        }

        results.push_back(*selected);
    }

    return results;
}

} // namespace

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
RewardState&       GameState::getRewardState()       { return m_rewardState; }
const RewardState& GameState::getRewardState() const { return m_rewardState; }

Enemy& GameState::getEnemy() {
    assert(m_enemy && "No enemy – call startCombatForEnemy() first");
    return *m_enemy;
}
const Enemy& GameState::getEnemy() const {
    assert(m_enemy && "No enemy – call startCombatForEnemy() first");
    return *m_enemy;
}

// ---------------------------------------------------------------------------
// Game flow
// ---------------------------------------------------------------------------

bool GameState::startNewGame(std::string& error) {
    if (!startNewRun(error)) {
        return false;
    }

    return startCombatForEnemy("bacteria", error);
}

bool GameState::startNewRun(std::string& error) {
    error.clear();
    m_player     = Player(CombatConfig::PlayerMaxHealth, CombatConfig::PlayerBaseMana);
    m_turnNumber = CombatConfig::StartingTurnNumber;
    m_turnPhase  = TurnPhase::PLAYER_TURN;
    m_lastAction = "";
    m_enemy.reset();
    m_rewardState.clear();

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

    return true;
}

bool GameState::startCombatForEnemy(const std::string& enemyId, std::string& error) {
    error.clear();
    m_turnNumber = CombatConfig::StartingTurnNumber;
    m_turnPhase = TurnPhase::PLAYER_TURN;
    m_lastAction.clear();
    m_enemy.reset();
    m_rewardState.clear();

    m_player.rebuildCombatDeck();

    std::cout << "[GameState] Draw pile before drawing hand: "
              << m_player.getDeck().getDrawPileSize() << " cards\n";

    for (int i = 0; i < CombatConfig::OpeningHandSize; ++i) {
        m_player.drawCardFromDeck();
    }

    std::cout << "[GameState] After drawing hand: "
              << m_player.getHand().size()               << " in hand, "
              << m_player.getDeck().getDrawPileSize()    << " in draw pile, "
              << m_player.getDeck().getDiscardPileSize() << " in discard\n";

    m_enemy = EnemyFactory::loadById(enemyId, error);
    if (!m_enemy) {
        return false;
    }

    m_enemy->decideIntent();
    return true;
}

bool GameState::prepareCombatRewards(std::string& error) {
    error.clear();
    if (!m_enemy) {
        error = "Cannot prepare rewards without an active enemy";
        return false;
    }

    const std::vector<Card> rewardCards = generateRewardCards(LuckConfig::RewardCardChoiceCount);
    if (rewardCards.empty()) {
        error = "No reward cards available in the card database";
        return false;
    }

    m_rewardState.begin(m_enemy->getGoldReward(), rewardCards);
    return true;
}

int GameState::collectRewardGold() {
    const int awardedGold = m_rewardState.collectGold();
    if (awardedGold > 0) {
        m_player.addGold(awardedGold);
    }
    return awardedGold;
}

bool GameState::claimRewardCard(int rewardIndex) {
    const auto selectedCard = m_rewardState.chooseCard(rewardIndex);
    if (!selectedCard) {
        return false;
    }

    m_player.addCardToDeck(*selectedCard);
    return true;
}

bool GameState::skipRewardCard() {
    return m_rewardState.skipCardChoice();
}

void GameState::endCombat() {
    m_enemy.reset();
    m_rewardState.clear();
    m_turnPhase = TurnPhase::PLAYER_TURN;
    m_turnNumber = CombatConfig::StartingTurnNumber;
    m_lastAction.clear();
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

bool GameState::isCombatWon() const {
    return m_enemy && m_enemy->isDead() && !m_player.isDead();
}

bool GameState::isPlayerDefeated() const {
    return m_player.isDead();
}

std::string GameState::getWinner() const {
    if (m_player.isDead())            return "Enemy";
    if (m_enemy && m_enemy->isDead()) return "Player";
    return "";
}
