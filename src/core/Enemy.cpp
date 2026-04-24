#include "Enemy.h"
#include "config/Defines.h"
#include "gameplay/CardEffect.h"
#include "Player.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace {

std::optional<StatusType> statusTypeFromKey(const std::string& key) {
    if (key == "poison") {
        return StatusType::Poison;
    }
    return std::nullopt;
}

StatusDisposition statusDispositionFromEffect(const CardEffect& effect) {
    return effect.target == EffectTarget::Opponent
        ? StatusDisposition::Negative
        : StatusDisposition::Positive;
}

void appendFragment(std::vector<std::string>& fragments, const std::string& fragment) {
    if (!fragment.empty()) {
        fragments.push_back(fragment);
    }
}

std::string joinFragments(const std::vector<std::string>& fragments) {
    if (fragments.empty()) {
        return "";
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < fragments.size(); ++index) {
        if (index > 0) {
            stream << (index + 1 == fragments.size() ? " and " : ", ");
        }
        stream << fragments[index];
    }
    return stream.str();
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Enemy::Enemy(std::string name, int health, Deck deck, EnemySpriteConfig spriteConfig, int goldReward)
    : m_name(std::move(name))
    , m_health(health)
    , m_maxHealth(health)
    , m_goldReward(goldReward)
    , m_spriteConfig(std::move(spriteConfig))
    , m_enemyDeck(std::move(deck))
{}

// ---------------------------------------------------------------------------
// Basic stats
// ---------------------------------------------------------------------------

const std::string& Enemy::getName()      const { return m_name;      }
int                Enemy::getHealth()    const { return m_health;    }
int                Enemy::getMaxHealth() const { return m_maxHealth; }
int                Enemy::getGoldReward() const { return m_goldReward; }
bool               Enemy::isDead()       const { return m_health <= 0; }

int Enemy::takeDamage(int amount) {
    // Block absorbs first
    if (m_enemyBlock >= amount) {
        m_enemyBlock -= amount;
        return 0;
    }
    amount -= m_enemyBlock;
    m_enemyBlock = 0;
    m_health = std::max(0, m_health - amount);
    return amount;
}

// ---------------------------------------------------------------------------
// Block
// ---------------------------------------------------------------------------

int  Enemy::getEnemyBlock()  const { return m_enemyBlock; }
void Enemy::addBlock(int amt)      { m_enemyBlock += amt; }
void Enemy::clearBlock()           { m_enemyBlock = 0; }

// ---------------------------------------------------------------------------
// Intent system
// ---------------------------------------------------------------------------

EnemyIntent Enemy::getIntent()        const { return m_intent;       }
int         Enemy::getIntentDamage()  const {
    return m_statuses.has(StatusType::SkipTurn) ? 0 : m_intentDamage;
}
int         Enemy::getIntentBlock()   const {
    return m_statuses.has(StatusType::SkipTurn) ? 0 : m_intentBlock;
}
const std::vector<IntentIndicator>& Enemy::getIntentIndicators() const { return m_intentIndicators; }

const std::vector<Card>& Enemy::getPlayedCards() const { return m_playedCards; }

void Enemy::addCardToDeck(const Card& card) { m_enemyDeck.addCard(card); }

const EnemySpriteConfig& Enemy::getSpriteConfig() const { return m_spriteConfig; }

void Enemy::addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition) {
    m_statuses.add(type, magnitude, duration, disposition);
}

void Enemy::queueSkipTurn(int magnitude, int duration) {
    addStatus(StatusType::SkipTurn, magnitude, duration, StatusDisposition::Negative);
    discardPlannedCards();
    resetIntent();
}

int Enemy::getStatusMagnitude(StatusType type) const {
    return m_statuses.getMagnitude(type);
}

int Enemy::getCounterValue(const std::string& key) const {
    const auto it = m_counters.find(key);
    return it == m_counters.end() ? 0 : it->second;
}

void Enemy::discardPlannedCards() {
    for (const auto& card : m_playedCards) {
        m_enemyDeck.discard(card);
    }
    m_playedCards.clear();
}

void Enemy::resetIntent() {
    m_intent = EnemyIntent::IDLE;
    m_intentDamage = 0;
    m_intentBlock = 0;
    m_intentIndicators.clear();
}

int Enemy::applyMaxHealthGrowthPercent(int percent, bool trackCounter, const std::string& counterKey) {
    if (percent <= 0) {
        return 0;
    }

    const int hpGain = std::max(1, (int)std::round((float)m_maxHealth * ((float)percent / 100.0f)));
    m_maxHealth += hpGain;
    m_health = std::min(m_maxHealth, m_health + hpGain);
    if (trackCounter && !counterKey.empty()) {
        ++m_counters[counterKey];
    }
    return hpGain;
}

std::string Enemy::getIntentDescription() const {
    if (m_statuses.has(StatusType::SkipTurn)) {
        return "Unable to act this turn";
    }
    if (!m_intentIndicators.empty()) {
        std::vector<std::string> descriptions;
        descriptions.reserve(m_intentIndicators.size());
        for (const auto& indicator : m_intentIndicators) {
            std::string tooltip = indicator.tooltip;
            if (!tooltip.empty() && tooltip.back() == '.') {
                tooltip.pop_back();
            }
            descriptions.push_back(std::move(tooltip));
        }
        return joinFragments(descriptions);
    }
    return "Repositioning...";
}

void Enemy::decideIntent() {
    discardPlannedCards();
    resetIntent();

    // Draw the configured number of cards each turn.
    for (int i = 0; i < CombatConfig::EnemyCardsPerTurn; ++i) {
        if (m_enemyDeck.isEmpty()) m_enemyDeck.reshuffleDiscard();
        auto nextCard = m_enemyDeck.draw();
        if (nextCard.has_value()) {
            m_playedCards.push_back(*nextCard);
        }
    }

    // Preview intent card-by-card so earlier actions can influence later ones.
    // Fungus uses this for Spore Growth -> Attack scaling, but the same preview
    // path can support future counters and status-based enemy cards.
    bool hasAttack = false;
    bool hasDefend = false;
    std::unordered_map<std::string, int> previewCounters = m_counters;
    int previewMaxHealth = m_maxHealth;

    for (const auto& card : m_playedCards) {
        for (const auto& effect : card.getEffects()) {
            switch (effect.type) {
            case EffectType::Damage: {
                hasAttack = true;
                m_intentDamage += effect.amount;
                m_intentIndicators.push_back({
                    IntentIconType::Attack,
                    std::to_string(effect.amount),
                    "Intends to attack for " + std::to_string(effect.amount)
                });
                break;
            }
            case EffectType::DamagePerCounter: {
                hasAttack = true;
                const int counterValue = effect.key.empty() ? 0 : previewCounters[effect.key];
                const int multiplier = std::max(1, counterValue);
                const int totalDamage = effect.amount * multiplier;
                m_intentDamage += totalDamage;
                m_intentIndicators.push_back({
                    IntentIconType::Attack,
                    counterValue > 0 ? std::to_string(effect.amount) + "x" + std::to_string(counterValue)
                                     : std::to_string(effect.amount),
                    "Intends to attack for " + std::to_string(totalDamage)
                });
                break;
            }
            case EffectType::Block:
                hasDefend = true;
                m_intentBlock += effect.amount;
                m_intentIndicators.push_back({
                    IntentIconType::Block,
                    "",
                    "Intends to block next turn."
                });
                break;
            case EffectType::ApplyStatus:
                if (effect.target == EffectTarget::Opponent) {
                    std::string tooltip = "Intends to apply a debuff.";
                    if (effect.key == "poison") {
                        tooltip = "Will debuff you next turn.";
                    }
                    m_intentIndicators.push_back({
                        IntentIconType::Debuff,
                        "",
                        tooltip
                    });
                }
                break;
            case EffectType::ModifyMaxHealthPercent: {
                const int hpGain = std::max(1, (int)std::round((float)previewMaxHealth * ((float)effect.amount / 100.0f)));
                previewMaxHealth += hpGain;
                if (!effect.key.empty()) {
                    ++previewCounters[effect.key];
                }
                m_intentIndicators.push_back({
                    IntentIconType::Buff,
                    "",
                    "Will buff next turn."
                });
                break;
            }
            case EffectType::Heal:
            case EffectType::CleanseDebuffs:
            case EffectType::GainMana:
            case EffectType::GainManaNextTurn:
            case EffectType::SkipEnemyTurn:
            case EffectType::DrawCards:
            case EffectType::Unknown:
                break;
            }
        }
    }

    if      (hasAttack && !hasDefend) m_intent = EnemyIntent::ATTACK;
    else if (!hasAttack && hasDefend) m_intent = EnemyIntent::DEFEND;
    else if (hasAttack && hasDefend)  m_intent = EnemyIntent::MIXED;
    else                              m_intent = EnemyIntent::IDLE;
}

EnemyTurnResult Enemy::executeTurn(Player& player) {
    if (m_statuses.consume(StatusType::SkipTurn) > 0) {
        discardPlannedCards();
        resetIntent();
        return EnemyTurnResult{ true, 0, 0, 0, 0, 0, 0, "" };
    }

    EnemyTurnResult result;
    std::vector<std::string> actionFragments;

    for (const auto& card : m_playedCards) {
        for (const auto& effect : card.getEffects()) {
            switch (effect.type) {
            case EffectType::Damage: {
                result.damageAttempted += effect.amount;
                const int dealt = (effect.target == EffectTarget::Opponent)
                    ? player.takeDamage(effect.amount)
                    : takeDamage(effect.amount);
                result.damageDealt += dealt;
                if (effect.target == EffectTarget::Opponent) {
                    result.hitCount += 1;
                    appendFragment(actionFragments, "attacked for " + std::to_string(dealt) + " damage");
                }
                break;
            }
            case EffectType::DamagePerCounter: {
                const int counterValue = getCounterValue(effect.key);
                const int multiplier = std::max(1, counterValue);
                const int totalDamage = effect.amount * multiplier;
                result.damageAttempted += totalDamage;
                const int dealt = (effect.target == EffectTarget::Opponent)
                    ? player.takeDamage(totalDamage)
                    : takeDamage(totalDamage);
                result.damageDealt += dealt;
                if (effect.target == EffectTarget::Opponent) {
                    result.hitCount += multiplier;
                    appendFragment(actionFragments, "attacked for " + std::to_string(dealt) + " damage");
                }
                break;
            }
            case EffectType::Block:
                if (effect.target == EffectTarget::Self) {
                    addBlock(effect.amount);
                    result.blockGained += effect.amount;
                    appendFragment(actionFragments, "gained " + std::to_string(effect.amount) + " block");
                }
                break;
            case EffectType::ApplyStatus:
                if (effect.target == EffectTarget::Opponent) {
                    if (const auto statusType = statusTypeFromKey(effect.key); statusType.has_value()) {
                        player.addStatus(*statusType, effect.amount, effect.duration, statusDispositionFromEffect(effect));
                        if (effect.key == "poison") {
                            appendFragment(actionFragments, "poisoned you for " + std::to_string(effect.duration) + " turns");
                        }
                    }
                }
                break;
            case EffectType::ModifyMaxHealthPercent: {
                const int hpGain = applyMaxHealthGrowthPercent(effect.amount, !effect.key.empty(), effect.key);
                if (hpGain > 0) {
                    result.maxHealthGained += hpGain;
                    appendFragment(actionFragments, "grew by +" + std::to_string(hpGain) + " max HP");
                }
                break;
            }
            case EffectType::Heal:
            case EffectType::CleanseDebuffs:
            case EffectType::GainMana:
            case EffectType::GainManaNextTurn:
            case EffectType::SkipEnemyTurn:
            case EffectType::DrawCards:
            case EffectType::Unknown:
                break;
            }
        }
    }

    discardPlannedCards();
    resetIntent();
    result.actionText = joinFragments(actionFragments);

    return result;
}
