#pragma once

#include "core/Card.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

enum class NoahEventChoice {
    None,
    AddCardsGainGold,
    TransformCardsLoseGold,
    RemoveRandomAddCard
};

enum class NoahEventStage {
    Options,
    SelectNoahCards,
    SelectTransformCards,
    RevealResult
};

class NoahEventState {
public:
    void clear() {
        m_active = false;
        m_choice = NoahEventChoice::None;
        m_stage = NoahEventStage::Options;
        m_offeredCards.clear();
        m_selectedOfferIndices.clear();
        m_selectedDeckIndices.clear();
        m_resultCards.clear();
        m_removedCardName.clear();
        m_goldDelta = 0;
        m_requiredSelections = 0;
    }

    void begin() {
        clear();
        m_active = true;
    }

    bool isActive() const { return m_active; }
    NoahEventChoice getChoice() const { return m_choice; }
    NoahEventStage getStage() const { return m_stage; }
    int getGoldDelta() const { return m_goldDelta; }
    int getRequiredSelections() const { return m_requiredSelections; }
    const std::vector<Card>& getOfferedCards() const { return m_offeredCards; }
    const std::vector<int>& getSelectedOfferIndices() const { return m_selectedOfferIndices; }
    const std::vector<int>& getSelectedDeckIndices() const { return m_selectedDeckIndices; }
    const std::vector<Card>& getResultCards() const { return m_resultCards; }
    const std::string& getRemovedCardName() const { return m_removedCardName; }

    void startGainCards(std::vector<Card> offeredCards, int goldDelta, int requiredSelections) {
        m_choice = NoahEventChoice::AddCardsGainGold;
        m_stage = NoahEventStage::SelectNoahCards;
        m_offeredCards = std::move(offeredCards);
        m_selectedOfferIndices.clear();
        m_selectedDeckIndices.clear();
        m_resultCards.clear();
        m_removedCardName.clear();
        m_goldDelta = goldDelta;
        m_requiredSelections = requiredSelections;
    }

    void startTransform(std::vector<Card> offeredCards, int goldDelta) {
        m_choice = NoahEventChoice::TransformCardsLoseGold;
        m_stage = NoahEventStage::SelectTransformCards;
        m_offeredCards = std::move(offeredCards);
        m_selectedOfferIndices.clear();
        m_selectedDeckIndices.clear();
        m_resultCards.clear();
        m_removedCardName.clear();
        m_goldDelta = goldDelta;
        m_requiredSelections = 2;
    }

    void showResult(NoahEventChoice choice,
                    std::vector<Card> resultCards,
                    int goldDelta,
                    std::string removedCardName = "") {
        m_choice = choice;
        m_stage = NoahEventStage::RevealResult;
        m_resultCards = std::move(resultCards);
        m_removedCardName = std::move(removedCardName);
        m_selectedOfferIndices.clear();
        m_selectedDeckIndices.clear();
        m_goldDelta = goldDelta;
        m_requiredSelections = 0;
    }

    void toggleOfferSelection(int index) {
        toggleIndex(m_selectedOfferIndices, index, m_requiredSelections);
    }

    void toggleDeckSelection(int index) {
        toggleIndex(m_selectedDeckIndices, index, 2);
    }

    bool hasRequiredOfferSelections() const {
        return static_cast<int>(m_selectedOfferIndices.size()) == m_requiredSelections;
    }

    bool hasRequiredDeckSelections() const {
        return static_cast<int>(m_selectedDeckIndices.size()) == 2;
    }

private:
    static void toggleIndex(std::vector<int>& selections, int index, int maxSelections) {
        if (index < 0 || maxSelections <= 0) {
            return;
        }

        const auto existing = std::find(selections.begin(), selections.end(), index);
        if (existing != selections.end()) {
            selections.erase(existing);
            return;
        }

        if (static_cast<int>(selections.size()) >= maxSelections) {
            return;
        }

        selections.push_back(index);
    }

    bool              m_active = false;
    NoahEventChoice   m_choice = NoahEventChoice::None;
    NoahEventStage    m_stage = NoahEventStage::Options;
    std::vector<Card> m_offeredCards;
    std::vector<int>  m_selectedOfferIndices;
    std::vector<int>  m_selectedDeckIndices;
    std::vector<Card> m_resultCards;
    std::string       m_removedCardName;
    int               m_goldDelta = 0;
    int               m_requiredSelections = 0;
};
