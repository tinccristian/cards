#pragma once

#include "config/Defines.h"
#include "Card.h"
#include "DamageResult.h"
#include "Deck.h"
#include "gameplay/StatusCollection.h"
#include <optional>
#include <vector>

// Tracks all player state: health, mana, block, hand, and deck.
struct PlayerTurnStartResult {
    DamageBreakdown poisonDamage;
    int             organHealingDone = 0;
};

class Player {
public:
    explicit Player(int maxHealth = CombatConfig::PlayerMaxHealth,
                    int maxMana = CombatConfig::PlayerBaseMana);

    // --- Health ---
    int  getHealth()    const;
    int  getMaxHealth() const;
    bool isDead()       const;

    // Block absorbs damage first; overflow reduces health.
    int  takeDamage(int amount);
    DamageBreakdown takeDamageDetailed(int amount);
    int  loseHealth(int amount); // bypasses block; used by damage-over-time effects
    int  heal(int amount); // Clamps to maxHealth, returns HP restored

    // --- Block (expires after the opposing side finishes its turn) ---
    int  getBlock()         const;
    void addBlock(int amount);
    void clearBlock();
    // Absorbs 'damage' with block. Returns any overflow not covered by block.
    int  reduceBlock(int damage);

    // --- Mana ---
    int  getMana()    const;
    int  getMaxMana() const;
    PlayerTurnStartResult startTurn();
    DamageBreakdown tickPoison();
    void gainMana(int amount);
    int  getGold() const;
    void addGold(int amount);
    bool spendGold(int amount);
    // Deduct 'amount' mana. Returns false (no change) if insufficient.
    bool useMana(int amount);
    int  getEffectiveCost(const Card& card) const;

    // --- Hand / Deck ---
    std::vector<Card>&       getHand();
    const std::vector<Card>& getHand() const;

    // Draw one card from deck to hand. Reshuffles discard into deck if needed.
    // Returns false if both deck and discard are empty.
    bool drawCardFromDeck();

    // Add a card directly to hand (used when constructing the opening hand).
    void drawCard(const Card& card);
    void drawCardFreeThisTurn(const Card& card);

    // Attempt to play card at handIndex.
    // Deducts mana equal to card cost; removes card from hand and discards it.
    // Returns the played card, or nullopt if mana is insufficient.
    std::optional<Card> playCard(int handIndex);

    // Move all cards in hand to the discard pile, then clear hand.
    void discardHand();

    // Returns true if a card can be drawn (deck or discard non-empty).
    bool canDrawCard() const;

    // Add a card directly to the deck (used when building the starter deck).
    void addCardToDeck(const Card& card);
    bool removeOwnedCardAt(int index);
    bool replaceOwnedCards(const std::vector<int>& indices, const std::vector<Card>& replacements);
    void rebuildCombatDeck();
    void moveCardInHand(int fromIndex, int toIndex);
    const std::vector<Card>& getOwnedCards() const;

    void addStatus(StatusType type, int magnitude, int duration, StatusDisposition disposition);
    int  clearNegativeStatuses();
    int  getStatusMagnitude(StatusType type) const;
    const StatusCollection& getStatuses() const;

    Deck& getDeck();
    const Deck& getDeck() const;

    // --- Organ system ---
    // Organs are played like cards but persist in the combat zone; they are not discarded.
    void activateOrgan(const Card& card);
    bool hasOrgan(const std::string& id) const;
    void clearOrgans();
    const std::vector<Card>& getActiveOrgans() const;

private:
    bool drawCardRaw(); // draws one card without triggering organ bonuses

    int               m_health;
    int               m_maxHealth;
    int               m_block = 0;
    int               m_maxMana;
    int               m_currentMana;
    int               m_gold = 0;
    StatusCollection  m_statuses;
    std::vector<Card> m_ownedCards;
    std::vector<Card> m_hand;
    Deck              m_deck;
    std::vector<Card> m_activeOrgans;
};
