#pragma once

#include "core/Card.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class RewardState {
public:
    void clear() {
        m_active = false;
        m_goldReward = 0;
        m_goldCollected = false;
        m_cardCollected = false;
        m_cardSkipPending = false;
        m_choosingCard = false;
        m_selectedCardName.clear();
        m_cardChoices.clear();
    }

    void begin(int goldReward, std::vector<Card> cardChoices) {
        m_active = true;
        m_goldReward = goldReward;
        m_goldCollected = false;
        m_cardCollected = false;
        m_cardSkipPending = false;
        m_choosingCard = false;
        m_selectedCardName.clear();
        m_cardChoices = std::move(cardChoices);
    }

    bool isActive() const { return m_active; }
    bool isChoosingCard() const { return m_choosingCard; }
    bool isGoldCollected() const { return m_goldCollected; }
    bool isCardCollected() const { return m_cardCollected; }
    bool isCardSkipPending() const { return m_cardSkipPending; }
    bool isComplete() const { return m_active && m_goldCollected && (m_cardCollected || m_cardSkipPending); }
    int  getGoldReward() const { return m_goldReward; }
    const std::vector<Card>& getCardChoices() const { return m_cardChoices; }
    const std::string& getSelectedCardName() const { return m_selectedCardName; }

    bool canCollectGold() const { return m_active && !m_goldCollected; }
    bool canOpenCardChoice() const { return m_active && !m_cardCollected && !m_cardChoices.empty(); }

    int collectGold() {
        if (!canCollectGold()) {
            return 0;
        }
        m_goldCollected = true;
        return m_goldReward;
    }

    void openCardChoice() {
        if (canOpenCardChoice()) {
            m_choosingCard = true;
        }
    }

    void closeCardChoice() {
        m_choosingCard = false;
    }

    std::optional<Card> chooseCard(int index) {
        if (!m_choosingCard || index < 0 || index >= static_cast<int>(m_cardChoices.size())) {
            return std::nullopt;
        }
        m_cardCollected = true;
        m_cardSkipPending = false;
        m_choosingCard = false;
        m_selectedCardName = m_cardChoices[index].getName();
        return m_cardChoices[index];
    }

    bool skipCardChoice() {
        if (!m_choosingCard || m_cardCollected) {
            return false;
        }
        m_choosingCard = false;
        m_cardSkipPending = true;
        m_selectedCardName.clear();
        return true;
    }

private:
    bool              m_active = false;
    int               m_goldReward = 0;
    bool              m_goldCollected = false;
    bool              m_cardCollected = false;
    bool              m_cardSkipPending = false;
    bool              m_choosingCard = false;
    std::string       m_selectedCardName;
    std::vector<Card> m_cardChoices;
};
