#include "GameState.h"

#include "config/Defines.h"
#include "content/EnemyCatalog.h"
#include "content/EnemyFactory.h"
#include "CardDatabase.h"
#include "gameplay/CombatResolver.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <unordered_set>
#include <unordered_map>

namespace {

bool hasTag(const Card& card, const std::string& tag) {
    const auto& tags = card.getTags();
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

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
        if (hasTag(card, "noah")) {
            continue;
        }
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

std::vector<Card> generateTaggedCards(const std::string& tag, int count) {
    std::vector<Card> results;
    const auto& allCards = CardDatabase::getAllCards();
    if (count <= 0 || allCards.empty()) {
        return results;
    }

    std::vector<const Card*> pool;
    pool.reserve(allCards.size());
    for (const Card& card : allCards) {
        if (hasTag(card, tag)) {
            pool.push_back(&card);
        }
    }

    if (pool.empty()) {
        return results;
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::unordered_set<std::string> usedIds;
    while (static_cast<int>(results.size()) < count && usedIds.size() < pool.size()) {
        const Card* selected = pickRandomCardFromPool(pool, usedIds, rng);
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
NoahEventState&       GameState::getNoahEventState()       { return m_noahEventState; }
const NoahEventState& GameState::getNoahEventState() const { return m_noahEventState; }

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
    m_noahEventState.clear();

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
    m_noahEventState.clear();

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

    std::string resolvedEnemyId = enemyId;
    if (resolvedEnemyId == "random") {
        resolvedEnemyId = EnemyCatalog::pickRandomEnemyId(AssetPaths::ENEMY_DIRECTORY);
        if (resolvedEnemyId.empty()) {
            error = "No enemies are available for random encounter selection.";
            return false;
        }
    }

    m_enemy = EnemyFactory::loadById(resolvedEnemyId, error);
    if (!m_enemy) {
        return false;
    }

    m_enemy->decideIntent();
    return true;
}

bool GameState::beginNoahEvent(std::string& error) {
    error.clear();
    m_rewardState.clear();
    m_enemy.reset();
    m_noahEventState.begin();
    m_lastAction = "Noah wants to trade scribbles.";
    return true;
}

bool GameState::chooseNoahGainCards(std::string& error) {
    error.clear();
    const std::vector<Card> offers = generateTaggedCards("noah", 2);
    if (offers.empty()) {
        error = "No Noah cards are available.";
        return false;
    }

    m_player.addGold(50);
    m_noahEventState.startGainCards(offers, 50, static_cast<int>(offers.size()));
    m_lastAction = "You took Noah's bargain and gained 50 gold.";
    return true;
}

bool GameState::chooseNoahTransform(std::string& error) {
    error.clear();
    if (m_player.getGold() < 10) {
        error = "Not enough gold for Noah's transform.";
        return false;
    }
    if (static_cast<int>(m_player.getOwnedCards().size()) < 2) {
        error = "You need at least 2 cards to transform.";
        return false;
    }

    const std::vector<Card> offers = generateTaggedCards("noah", 2);
    if (static_cast<int>(offers.size()) < 2) {
        error = "Not enough Noah cards are available for a transform.";
        return false;
    }

    m_noahEventState.startTransform(offers, -10);
    m_lastAction = "Pick 2 cards for Noah to rewrite.";
    return true;
}

bool GameState::confirmNoahTransform(std::string& error) {
    error.clear();
    if (m_noahEventState.getStage() != NoahEventStage::SelectTransformCards
        || !m_noahEventState.hasRequiredDeckSelections()) {
        error = "Select 2 cards to transform.";
        return false;
    }
    if (!m_player.spendGold(10)) {
        error = "Not enough gold for Noah's transform.";
        return false;
    }

    if (!m_player.replaceOwnedCards(m_noahEventState.getSelectedDeckIndices(),
                                    m_noahEventState.getOfferedCards())) {
        error = "Could not transform the selected cards.";
        return false;
    }

    m_noahEventState.showResult(NoahEventChoice::TransformCardsLoseGold,
                                m_noahEventState.getOfferedCards(),
                                -10);
    m_lastAction = "Noah transformed 2 cards.";
    return true;
}

bool GameState::chooseNoahRemoveRandom(std::string& error) {
    error.clear();
    if (m_player.getOwnedCards().empty()) {
        error = "There are no cards to remove.";
        return false;
    }

    const std::vector<Card> offers = generateTaggedCards("noah", 1);
    if (offers.empty()) {
        error = "No Noah cards are available.";
        return false;
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(m_player.getOwnedCards().size()) - 1);
    const int removedIndex = dist(rng);
    const std::string removedName = m_player.getOwnedCards()[removedIndex].getName();
    if (!m_player.removeOwnedCardAt(removedIndex)) {
        error = "Could not remove a random card.";
        return false;
    }

    m_player.addCardToDeck(offers.front());
    m_noahEventState.showResult(NoahEventChoice::RemoveRandomAddCard,
                                offers,
                                0,
                                removedName);
    m_lastAction = "Noah traded one random card.";
    return true;
}

bool GameState::claimSelectedNoahCards(std::string& error) {
    error.clear();
    if (m_noahEventState.getStage() != NoahEventStage::SelectNoahCards
        || !m_noahEventState.hasRequiredOfferSelections()) {
        error = "Select Noah's cards first.";
        return false;
    }

    std::vector<Card> selectedCards;
    for (int index : m_noahEventState.getSelectedOfferIndices()) {
        if (index < 0 || index >= static_cast<int>(m_noahEventState.getOfferedCards().size())) {
            error = "Selected Noah card index is invalid.";
            return false;
        }
        selectedCards.push_back(m_noahEventState.getOfferedCards()[index]);
    }

    for (const Card& card : selectedCards) {
        m_player.addCardToDeck(card);
    }

    m_noahEventState.showResult(NoahEventChoice::AddCardsGainGold,
                                selectedCards,
                                50);
    m_lastAction = "You took Noah's cards.";
    return true;
}

void GameState::endNoahEvent() {
    m_noahEventState.clear();
    m_lastAction.clear();
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
    m_noahEventState.clear();
    m_turnPhase = TurnPhase::PLAYER_TURN;
    m_turnNumber = CombatConfig::StartingTurnNumber;
    m_lastAction.clear();
}

void GameState::playerDrawCard() {
    m_player.drawCardFromDeck();
}

std::optional<CardResolutionSummary> GameState::playCard(int cardIndex) {
    if (!m_enemy || m_turnPhase != TurnPhase::PLAYER_TURN) {
        return std::nullopt;
    }

    // Player::playCard handles mana check, hand removal, and discard
    auto optCard = m_player.playCard(cardIndex);
    if (!optCard) {
        m_lastAction = "Not enough mana!";
        return std::nullopt;
    }

    const CardResolutionSummary summary =
        CombatResolver::applyPlayerCard(*optCard, m_player, *m_enemy);

    m_lastAction = CombatResolver::buildPlayerActionText(*optCard, summary);

    return summary;
}

void GameState::endPlayerTurn() {
    if (m_enemy) {
        m_enemy->clearBlock();
    }
    m_turnPhase  = TurnPhase::ENEMY_TURN;
    m_lastAction = m_enemy->getName() + " – " + m_enemy->getIntentDescription() + "!";
}

EnemyTurnResult GameState::executeEnemyTurn() {
    if (!m_enemy) {
        return EnemyTurnResult{};
    }

    EnemyTurnResult result = m_enemy->executeTurn(m_player);

    if (result.skipped) {
        m_lastAction = m_enemy->getName() + " could not act.";
    } else if (!result.actionText.empty()) {
        m_lastAction = m_enemy->getName() + " " + result.actionText + "!";
    } else {
        m_lastAction = m_enemy->getName() + " repositioned.";
    }

    m_player.clearBlock();

    // Discard entire player hand, then begin the next player turn.
    m_player.discardHand();
    const PlayerTurnStartResult startTurnResult = m_player.startTurn();

    // Draw a fresh hand for the next player turn
    for (int i = 0; i < CombatConfig::OpeningHandSize; ++i) {
        m_player.drawCardFromDeck();
    }

    // Enemy plans next turn
    m_enemy->decideIntent();
    if (startTurnResult.poisonDamage.blocked > 0 || startTurnResult.poisonDamage.health > 0) {
        result.turnStartDamage = startTurnResult.poisonDamage;
        result.turnStartDamageTaken = startTurnResult.poisonDamage.health;
        m_lastAction += " Poison dealt " + std::to_string(startTurnResult.poisonDamage.blocked + startTurnResult.poisonDamage.health) + " damage.";
    }
    m_turnNumber++;
    m_turnPhase = TurnPhase::PLAYER_TURN;
    return result;
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
