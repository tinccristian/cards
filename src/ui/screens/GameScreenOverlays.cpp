#include "ui/GameScreen.h"

#include <algorithm>
#include <cmath>

// Overlay-style screens that sit on top of combat/map rendering.
// Kept separate from GameScreen.cpp so the core combat/map flow stays readable.

int GameScreen::drawPileViewer(const std::string& title,
                               const std::vector<Card>& cards,
                               int scrollOffset,
                               bool& closeClicked) {
    syncWindowSize();
    closeClicked = false;
    const int contentTop = scalei(LayoutConfig::TopHudBarHeight);
    const int contentHeight = std::max(1, m_height - contentTop);
    DrawRectangle(0, contentTop, m_width, contentHeight, Colors::overlay_bg);

    const int panelW = scalei(LayoutConfig::PileViewerPanelWidth);
    const int panelH = scalei(LayoutConfig::PileViewerPanelHeight);
    const int panelX = (m_width  - panelW) / 2;
    const int panelY = contentTop + (contentHeight - panelH) / 2;
    Rectangle panel  = { (float)panelX, (float)panelY, (float)panelW, (float)panelH };
    DrawRectangleRec(panel, Colors::dark_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const int titleFontSize = scalei(LayoutConfig::PileViewerTitleSize);
    const int titleOffsetY = scalei(LayoutConfig::PileViewerTitleOffsetY);
    int tw = MeasureText(title.c_str(), titleFontSize);
    DrawText(title.c_str(),
             panelX + (panelW - tw) / 2,
             panelY + titleOffsetY,
             titleFontSize,
             Colors::text_primary);

    const int subtitleFontSize = scalei(LayoutConfig::PileViewerSubtitleSize);
    const int subtitleOffsetY = scalei(LayoutConfig::PileViewerSubtitleOffsetY);
    std::string sub = std::to_string((int)cards.size()) + " cards";
    int sw = MeasureText(sub.c_str(), subtitleFontSize);
    DrawText(sub.c_str(),
             panelX + (panelW - sw) / 2,
             panelY + subtitleOffsetY,
             subtitleFontSize,
             Colors::text_secondary);

    {
        const int btnSz = scalei(LayoutConfig::PileViewerCloseButtonSize);
        const int closeMargin = scalei(LayoutConfig::PileViewerCloseButtonMargin);
        const int closeFontSize = scalei(LayoutConfig::EntityNameFontSize);
        Rectangle xBtn = { (float)(panelX + panelW - btnSz - closeMargin),
                           (float)(panelY + closeMargin),
                           (float)btnSz, (float)btnSz };
        const bool xHovered = mouseOver(xBtn);
        DrawRectangleRec(xBtn, xHovered ? Colors::close_button_hover : Colors::close_button_bg);
        DrawRectangleLinesEx(xBtn, scalef(LayoutConfig::PanelBorderThickness), Colors::damage_color);
        const int xw = MeasureText("X", closeFontSize);
        DrawText("X",
                 (int)xBtn.x + (btnSz - xw) / 2,
                 (int)xBtn.y + (btnSz - closeFontSize) / 2,
                 closeFontSize,
                 WHITE);
        if (xHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            closeClicked = true;
        }
    }

    const int dividerInset = scalei(LayoutConfig::PileViewerDividerInset);
    const int headerHeight = scalei(LayoutConfig::PileViewerHeaderHeight);
    DrawLine(panelX + dividerInset,
             panelY + headerHeight,
             panelX + panelW - dividerInset,
             panelY + headerHeight,
             Colors::card_border);

    const int cols = LayoutConfig::PileViewerGridColumns;
    const float visibleRows = LayoutConfig::PileViewerVisibleRows;
    const float gridGap = scalef(LayoutConfig::PileViewerGridGap);
    const int gridTopOffset = scalei(LayoutConfig::PileViewerGridTopOffset);
    const int gridTopPadding = scalei(LayoutConfig::PileViewerGridTopPadding);
    const int gridBottomPadding = scalei(LayoutConfig::PileViewerGridBottomPadding);
    const int gridBottomPaddingInner = scalei(LayoutConfig::PileViewerGridBottomPaddingInner);
    const int scissorInset = scalei(LayoutConfig::PileViewerScissorInset);
    const int gridTop = panelY + gridTopOffset + gridTopPadding;
    const int gridH = panelH - gridBottomPadding - gridTopPadding - gridBottomPaddingInner;
    const int availableGridWidth = panelW - dividerInset * 2;
    const float cardAspect = (float)LayoutConfig::CardWidth / (float)LayoutConfig::CardHeight;
    const float cardHeightFromRows = ((float)gridH - gridGap * (visibleRows - 1)) / visibleRows;
    const float cardWidthFromRows = cardHeightFromRows * cardAspect;
    const float cardWidthFromCols = ((float)availableGridWidth - gridGap * (cols - 1)) / (float)cols;
    float drawWidth = std::floor(std::min(cardWidthFromRows, cardWidthFromCols));
    float drawHeight = std::floor(drawWidth / cardAspect);
    if (drawHeight <= 0.0f) {
        drawHeight = scalef(LayoutConfig::CardHeight);
        drawWidth = drawHeight * cardAspect;
    }

    const float gridWidth = drawWidth * cols + gridGap * (cols - 1);
    const float gridX = panelX + (panelW - gridWidth) / 2.0f;
    const float rowStride = drawHeight + gridGap;

    BeginScissorMode(panelX + scissorInset,
                     gridTop,
                     panelW - scissorInset * 2,
                     gridH);

    const int total = (int)cards.size();
    const int rows = (total + cols - 1) / cols;
    const int fullyVisibleRows = std::max(1, (int)std::floor(visibleRows));
    const int maxScroll = std::max(0, rows - fullyVisibleRows);

    int hoveredCardIndex = -1;
    Rectangle hoveredRect = {};
    for (int idx = 0; idx < total; ++idx) {
        const int col = idx % cols;
        const int row = idx / cols;
        const float cx = gridX + col * (drawWidth + gridGap);
        const float cy = (float)gridTop + (row - scrollOffset) * rowStride;

        if (cy + drawHeight < gridTop || cy > gridTop + gridH) {
            continue;
        }

        Rectangle r = { cx, cy, drawWidth, drawHeight };
        if (mouseOver(r)) {
            hoveredCardIndex = idx;
            hoveredRect = r;
        } else {
            drawCardFace(r, cards[idx], false, 0.0f);
        }
    }

    EndScissorMode();

    // Draw the hovered card after leaving the scissor rect so the zoomed card
    // can spill outside the grid without being clipped by the viewer viewport.
    if (hoveredCardIndex >= 0) {
        const std::string hoverToken = title + ":" + std::to_string(hoveredCardIndex);
        if (m_cardAudio && hoverToken != m_lastPileViewerHoverToken) {
            m_cardAudio->playHover();
        }
        m_lastPileViewerHoverToken = hoverToken;

        const float hoverScale = LayoutConfig::PileViewerHoverScale;
        Rectangle scaledRect = hoveredRect;
        scaledRect.x -= (scaledRect.width * hoverScale - scaledRect.width) / 2.0f;
        scaledRect.y -= (scaledRect.height * hoverScale - scaledRect.height) / 2.0f;
        scaledRect.width *= hoverScale;
        scaledRect.height *= hoverScale;
        drawCardFace(scaledRect, cards[hoveredCardIndex], true, 0.0f);
    } else {
        m_lastPileViewerHoverToken.clear();
    }

    return maxScroll;
}

RunHudAction GameScreen::drawRunHud(const Player& player,
                                    const std::string& centerLabel,
                                    bool allowInteraction,
                                    bool showMapButton,
                                    bool showBackButton,
                                    bool showDeckButton) {
    syncWindowSize();
    const Rectangle bar = {
        0.0f,
        0.0f,
        (float)m_width,
        (float)scalei(LayoutConfig::TopHudBarHeight)
    };
    DrawRectangleRec(bar, Colors::light_bg);
    DrawRectangleLinesEx(bar, scalef(LayoutConfig::ThinBorderThickness), Colors::card_border);

    const float sidePadding = scalef(LayoutConfig::TopHudSidePadding);
    const int goldFontSize = scalei(LayoutConfig::TopHudGoldFontSize);
    const int centerFontSize = scalei(LayoutConfig::TopHudCenterFontSize);
    const float textY = scalef(LayoutConfig::TopHudTextPaddingY);

    const std::string goldText = "Gold: " + std::to_string(player.getGold());
    DrawText(goldText.c_str(),
             (int)std::round(sidePadding),
             (int)std::round(textY),
             goldFontSize,
             Colors::gold_color);

    DrawText(centerLabel.c_str(),
             (m_width - MeasureText(centerLabel.c_str(), centerFontSize)) / 2,
             (int)std::round(textY - scalef(1.0f)),
             centerFontSize,
             Colors::text_primary);

    const float buttonWidth = scalef(LayoutConfig::TopHudButtonWidth);
    const float buttonHeight = scalef(LayoutConfig::TopHudButtonHeight);
    const float buttonGap = scalef(LayoutConfig::TopHudButtonGap);
    const float buttonY = (bar.height - buttonHeight) / 2.0f;
    float nextRightX = (float)m_width - sidePadding - buttonWidth;

    auto drawHudButton = [&](const char* label,
                             bool visible,
                             bool hoveredLast,
                             bool& hoveredLastRef,
                             Color accent,
                             bool returnsAction,
                             RunHudAction action) -> RunHudAction {
        hoveredLastRef = false;
        if (!visible) {
            return RunHudAction::None;
        }
        Rectangle rect = { nextRightX, buttonY, buttonWidth, buttonHeight };
        nextRightX -= buttonWidth + buttonGap;
        const bool hovered = allowInteraction && mouseOver(rect);
        if (hovered && !hoveredLast && m_cardAudio) {
            m_cardAudio->playHover();
        }
        hoveredLastRef = hovered;
        if (hovered) {
            const float scaledWidth = rect.width * LayoutConfig::TopHudHoverScale;
            const float scaledHeight = rect.height * LayoutConfig::TopHudHoverScale;
            rect.x -= (scaledWidth - rect.width) / 2.0f;
            rect.y -= (scaledHeight - rect.height) / 2.0f;
            rect.width = scaledWidth;
            rect.height = scaledHeight;
        }
        DrawRectangleRec(rect, hovered ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(rect, scalef(LayoutConfig::PanelBorderThickness), accent);
        const int fontSize = scalei(LayoutConfig::DefaultButtonFontSize);
        DrawText(label,
                 (int)std::round(rect.x + (rect.width - MeasureText(label, fontSize)) / 2.0f),
                 (int)std::round(rect.y + (rect.height - fontSize) / 2.0f),
                 fontSize,
                 Colors::text_primary);
        if (returnsAction && hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return action;
        }
        return RunHudAction::None;
    };

    if (RunHudAction action = drawHudButton("Deck", showDeckButton, m_deckHudHoveredLast,
                                            m_deckHudHoveredLast, Colors::draw_pile_accent,
                                            true, RunHudAction::ViewDeck);
        action != RunHudAction::None) {
        return action;
    }
    if (RunHudAction action = drawHudButton("Map", showMapButton, m_mapHudHoveredLast,
                                            m_mapHudHoveredLast, Colors::mixed_intent_color,
                                            true, RunHudAction::ToggleMap);
        action != RunHudAction::None) {
        return action;
    }
    if (RunHudAction action = drawHudButton("Back", showBackButton, m_backHudHoveredLast,
                                            m_backHudHoveredLast, Colors::card_border,
                                            true, RunHudAction::Back);
        action != RunHudAction::None) {
        return action;
    }

    return RunHudAction::None;
}

RewardPopupAction GameScreen::drawRewardPopup(const RewardState& rewards,
                                              int currentGold,
                                              int scrollOffset,
                                              int& maxScroll,
                                              bool allowInteraction) {
    syncWindowSize();
    const int contentTop = scalei(LayoutConfig::TopHudBarHeight);
    const int contentHeight = std::max(1, m_height - contentTop);
    DrawRectangle(0, contentTop, m_width, contentHeight, Colors::overlay_bg);

    const float panelWidth = scalef(LayoutConfig::RewardPanelWidth);
    const float panelHeight = scalef(LayoutConfig::RewardPanelHeight);
    const Rectangle panel = {
        (m_width - panelWidth) / 2.0f,
        contentTop + ((float)contentHeight - panelHeight) / 2.0f,
        panelWidth,
        panelHeight
    };

    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const int titleSize = scalei(LayoutConfig::OverlayTitleFontSize);
    const int entryTitleSize = scalei(LayoutConfig::RewardEntryTitleSize);
    const int entryBodySize = scalei(LayoutConfig::RewardEntryBodySize);
    const char* title = "Collect Rewards";
    DrawText(title,
             (int)std::round(panel.x + (panel.width - MeasureText(title, titleSize)) / 2.0f),
             (int)std::round(panel.y + scalef(22.0f)),
             titleSize,
             Colors::text_primary);

    const std::string goldSummary = "Current gold: " + std::to_string(currentGold);
    DrawText(goldSummary.c_str(),
             (int)std::round(panel.x + (panel.width - MeasureText(goldSummary.c_str(), entryBodySize)) / 2.0f),
             (int)std::round(panel.y + scalef(58.0f)),
             entryBodySize,
             Colors::gold_color);

    struct RewardEntryView {
        RewardPopupAction action;
        std::string title;
        std::string body;
        Color titleColor;
    };

    std::vector<RewardEntryView> entries;
    if (!rewards.isGoldCollected()) {
        entries.push_back({
            RewardPopupAction::CollectGold,
            "Gain +" + std::to_string(rewards.getGoldReward()) + " gold",
            "Take the coin reward.",
            Colors::gold_color
        });
    }
    if (!rewards.isCardCollected()) {
        entries.push_back({
            RewardPopupAction::OpenCardChoice,
            "Select a card.",
            rewards.isCardSkipPending() ? "Skipped for now. You can still pick one." : "Add 1 card to this run.",
            Colors::draw_pile_accent
        });
    }

    const float listTop = panel.y + scalef(LayoutConfig::RewardListTopOffset);
    const float listHeight = scalef(LayoutConfig::RewardListHeight);
    const float entryHeight = scalef(LayoutConfig::RewardEntryHeight);
    const float entryGap = scalef(LayoutConfig::RewardEntryGap);
    const float entryInset = scalef(LayoutConfig::RewardEntryInset);
    const int visibleRows = std::max(1, (int)std::floor((listHeight + entryGap) / (entryHeight + entryGap)));
    maxScroll = std::max(0, (int)entries.size() - visibleRows);

    BeginScissorMode((int)std::round(panel.x), (int)std::round(listTop),
                     (int)std::round(panel.width), (int)std::round(listHeight));
    for (int index = 0; index < static_cast<int>(entries.size()); ++index) {
        const float y = listTop + (index - scrollOffset) * (entryHeight + entryGap);
        if (y + entryHeight < listTop || y > listTop + listHeight) {
            continue;
        }
        Rectangle rect = {
            panel.x + entryInset,
            y,
            panel.width - entryInset * 2.0f,
            entryHeight
        };
        const bool hovered = allowInteraction && mouseOver(rect);
        DrawRectangleRec(rect, hovered ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(rect, scalef(LayoutConfig::ThinBorderThickness),
                             hovered ? entries[index].titleColor : Colors::card_border);
        DrawText(entries[index].title.c_str(),
                 (int)std::round(rect.x + scalef(14.0f)),
                 (int)std::round(rect.y + scalef(8.0f)),
                 entryTitleSize,
                 entries[index].titleColor);
        DrawText(entries[index].body.c_str(),
                 (int)std::round(rect.x + scalef(14.0f)),
                 (int)std::round(rect.y + scalef(30.0f)),
                 entryBodySize,
                 Colors::text_secondary);
        if (allowInteraction && hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            EndScissorMode();
            return entries[index].action;
        }
    }
    EndScissorMode();

    if (rewards.isComplete()) {
        const Rectangle continueRect = {
            panel.x + (panel.width - scalef(LayoutConfig::RewardContinueWidth)) / 2.0f,
            panel.y + panel.height - scalef(LayoutConfig::RewardContinueHeight) - scalef(LayoutConfig::RewardContinueBottomGap),
            scalef(LayoutConfig::RewardContinueWidth),
            scalef(LayoutConfig::RewardContinueHeight)
        };
        const bool continueHovered = allowInteraction && mouseOver(continueRect);
        drawButton(continueRect, "Continue", continueHovered);
        if (allowInteraction && continueHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return RewardPopupAction::Continue;
        }
    }

    return RewardPopupAction::None;
}

int GameScreen::drawRewardCardChoice(const RewardState& rewards,
                                     bool allowInteraction) {
    syncWindowSize();
    drawVignetteOverlay();

    const char* title = "Choose a Card";
    const int titleSize = scalei(LayoutConfig::RewardCardTitleSize);
    DrawText(title,
             (m_width - MeasureText(title, titleSize)) / 2,
             scalei(LayoutConfig::CombatLogTop),
             titleSize,
             Colors::text_primary);

    const char* hint = "Pick one to permanently add it to this run";
    const int hintSize = scalei(LayoutConfig::RewardHintSize);
    DrawText(hint,
             (m_width - MeasureText(hint, hintSize)) / 2,
             scalei(LayoutConfig::CombatLogTop) + titleSize + scalei(10),
             hintSize,
             Colors::text_secondary);

    const auto& choices = rewards.getCardChoices();
    if (choices.empty()) {
        return RewardChoiceNone;
    }

    const float gap = scalef(LayoutConfig::RewardCardChoiceGap);
    const float preferredWidth = scalef(LayoutConfig::RewardCardChoiceWidth);
    const float preferredHeight = scalef(LayoutConfig::RewardCardChoiceHeight);
    const float availableWidth = (float)m_width - scalef(80.0f);
    const float totalPreferredWidth = preferredWidth * (float)choices.size() + gap * (float)(choices.size() - 1);
    const float widthScale = std::min(1.0f, availableWidth / std::max(1.0f, totalPreferredWidth));
    const float cardWidth = preferredWidth * widthScale;
    const float cardHeight = preferredHeight * widthScale;
    const float totalWidth = cardWidth * (float)choices.size() + gap * (float)(choices.size() - 1);
    const float startX = ((float)m_width - totalWidth) / 2.0f;
    const float y = scalef(LayoutConfig::RewardCardChoiceTop);
    const float choiceBottom = y + cardHeight;

    for (int index = 0; index < static_cast<int>(choices.size()); ++index) {
        Rectangle rect = {
            startX + index * (cardWidth + gap),
            y,
            cardWidth,
            cardHeight
        };
        const bool hovered = allowInteraction && mouseOver(rect);
        if (hovered) {
            const float hoverScale = LayoutConfig::RewardCardHoverScale;
            const float scaledWidth = rect.width * hoverScale;
            const float scaledHeight = rect.height * hoverScale;
            rect.x -= (scaledWidth - rect.width) / 2.0f;
            rect.y -= (scaledHeight - rect.height) / 2.0f;
            rect.width = scaledWidth;
            rect.height = scaledHeight;
        }
        drawCardFace(rect, choices[index], false, 0.0f);
        if (hovered && allowInteraction && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return index;
        }
    }

    const Rectangle skipRect = {
        ((float)m_width - scalef(LayoutConfig::RewardSkipButtonWidth)) / 2.0f,
        choiceBottom + scalef(LayoutConfig::RewardSkipButtonTopGap),
        scalef(LayoutConfig::RewardSkipButtonWidth),
        scalef(LayoutConfig::RewardSkipButtonHeight)
    };
    const bool skipHovered = allowInteraction && mouseOver(skipRect);
    drawButton(skipRect, "Skip", skipHovered);
    if (allowInteraction && skipHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return RewardChoiceSkip;
    }

    return RewardChoiceNone;
}

void GameScreen::drawVignetteOverlay() const {
    const Color mid = { 0, 0, 0, (unsigned char)(LayoutConfig::RewardVignetteAlpha / 2) };
    const Color edge = { 0, 0, 0, (unsigned char)LayoutConfig::RewardVignetteAlpha };
    const int topInset = scalei(LayoutConfig::TopHudBarHeight);
    const int contentHeight = std::max(1, m_height - topInset);
    DrawRectangle(0, topInset, m_width, contentHeight, mid);
    DrawRectangleGradientV(0, topInset, m_width, contentHeight / 3, edge, BLANK);
    DrawRectangleGradientV(0, m_height - contentHeight / 3, m_width, contentHeight / 3, BLANK, edge);
    DrawRectangleGradientH(0, topInset, m_width / 4, contentHeight, edge, BLANK);
    DrawRectangleGradientH(m_width - m_width / 4, topInset, m_width / 4, contentHeight, BLANK, edge);
}
