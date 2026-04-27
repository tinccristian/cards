#include "ui/GameScreen.h"
#include "ui/FontUtils.h"

#include <algorithm>
#include <cmath>

#define DrawText UiFont::drawText
#define MeasureText UiFont::measureText

// Overlay-style screens that sit on top of combat/map rendering.
// Kept separate from GameScreen.cpp so the core combat/map flow stays readable.

int GameScreen::drawPileViewer(const std::string& title,
                               const std::vector<Card>& cards,
                               int scrollOffset,
                               bool& closeClicked) {
    syncWindowSize();
    closeClicked = false;
    const float dt = GetFrameTime() * m_timeScale;
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
    float hoveredProgress = 0.0f;
    for (int idx = 0; idx < total; ++idx) {
        const int col = idx % cols;
        const int row = idx / cols;
        const float cx = gridX + col * (drawWidth + gridGap);
        const float cy = (float)gridTop + (row - scrollOffset) * rowStride;

        if (cy + drawHeight < gridTop || cy > gridTop + gridH) {
            continue;
        }

        Rectangle r = { cx, cy, drawWidth, drawHeight };
        const bool hovered = mouseOver(r);
        const float progress = cardHoverProgress("pile:" + title + ":" + std::to_string(idx), hovered, dt);
        if (hovered) {
            hoveredCardIndex = idx;
            hoveredRect = r;
            hoveredProgress = progress;
        } else {
            drawCardFace(applyCardHoverMotion(r, progress, LayoutConfig::PileViewerHoverScale),
                         cards[idx],
                         progress > 0.0f,
                         0.0f,
                         -1,
                         -1,
                         true);
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

        drawCardFace(applyCardHoverMotion(hoveredRect, hoveredProgress, LayoutConfig::PileViewerHoverScale),
                     cards[hoveredCardIndex],
                     true,
                     0.0f,
                     -1,
                     -1,
                     true);
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

int GameScreen::drawCardChoiceOverlay(const std::string& title,
                                      const std::string& hint,
                                      const std::vector<Card>& choices,
                                      float cardScale,
                                      bool showSkip,
                                      bool allowInteraction) {
    syncWindowSize();
    drawVignetteOverlay();
    const float dt = GetFrameTime() * m_timeScale;

    const int titleSize = scalei(LayoutConfig::RewardCardTitleSize);
    DrawText(title.c_str(),
             (m_width - MeasureText(title.c_str(), titleSize)) / 2,
             scalei(LayoutConfig::CombatLogTop),
             titleSize,
             Colors::text_primary);

    const int hintSize = scalei(LayoutConfig::RewardHintSize);
    DrawText(hint.c_str(),
             (m_width - MeasureText(hint.c_str(), hintSize)) / 2,
             scalei(LayoutConfig::CombatLogTop) + titleSize + scalei(10),
             hintSize,
             Colors::text_secondary);

    if (choices.empty()) {
        return RewardChoiceNone;
    }

    const float gap = scalef(LayoutConfig::RewardCardChoiceGap);
    const float clampedCardScale = std::max(0.1f, cardScale);
    const float preferredWidth = scalef(LayoutConfig::RewardCardChoiceWidth) * clampedCardScale;
    const float preferredHeight = scalef(LayoutConfig::RewardCardChoiceHeight) * clampedCardScale;
    const float availableWidth = (float)m_width - scalef(80.0f);
    const float totalPreferredWidth = preferredWidth * (float)choices.size() + gap * (float)(choices.size() - 1);
    const float widthScale = std::min(1.0f, availableWidth / std::max(1.0f, totalPreferredWidth));
    const float cardWidth = preferredWidth * widthScale;
    const float cardHeight = preferredHeight * widthScale;
    const float totalWidth = cardWidth * (float)choices.size() + gap * (float)(choices.size() - 1);
    const float startX = ((float)m_width - totalWidth) / 2.0f;
    const float y = scalef(LayoutConfig::RewardCardChoiceTop);
    const float choiceBottom = y + cardHeight;

    int hoveredCardIndex = -1;
    Rectangle hoveredRect = {};
    float hoveredProgress = 0.0f;
    for (int index = 0; index < static_cast<int>(choices.size()); ++index) {
        Rectangle rect = {
            startX + index * (cardWidth + gap),
            y,
            cardWidth,
            cardHeight
        };
        const bool hovered = allowInteraction && mouseOver(rect);
        const std::string hoverKey = (showSkip ? "reward:" : "choice:") + title + ":" + std::to_string(index);
        const float progress = cardHoverProgress(hoverKey, hovered, dt);
        if (hovered) {
            hoveredCardIndex = index;
            hoveredRect = rect;
            hoveredProgress = progress;
        } else {
            drawCardFace(applyCardHoverMotion(rect, progress, LayoutConfig::RewardCardHoverScale),
                         choices[index],
                         progress > 0.0f,
                         0.0f,
                         -1,
                         -1,
                         true);
        }
        if (hovered && allowInteraction && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return index;
        }
    }

    if (hoveredCardIndex >= 0) {
        const std::string hoverToken = title + ":" + std::to_string(hoveredCardIndex);
        if (m_cardAudio && hoverToken != m_lastPileViewerHoverToken) {
            m_cardAudio->playHover();
        }
        m_lastPileViewerHoverToken = hoverToken;
        drawCardFace(applyCardHoverMotion(hoveredRect, hoveredProgress, LayoutConfig::RewardCardHoverScale),
                     choices[hoveredCardIndex],
                     true,
                     0.0f,
                     -1,
                     -1,
                     true);
    } else {
        m_lastPileViewerHoverToken.clear();
    }

    if (!showSkip) {
        return RewardChoiceNone;
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

int GameScreen::drawRewardCardChoice(const RewardState& rewards,
                                     bool allowInteraction) {
    return drawCardChoiceOverlay("Choose a Card",
                                 "Pick one to permanently add it to this run",
                                 rewards.getCardChoices(),
                                 1.0f,
                                 true,
                                 allowInteraction);
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

NoahEventUiAction GameScreen::drawNoahEvent(const NoahEventState& eventState,
                                            const Player& player,
                                            int scrollOffset,
                                            int& maxScroll,
                                            bool allowInteraction) {
    syncWindowSize();
    maxScroll = 0;
    ensureNoahEventTextureLoaded();

    if (m_noahEventTextureLoaded && m_noahEventTexture.id != 0) {
        const float scale = std::max((float)m_width / (float)m_noahEventTexture.width,
                                     (float)m_height / (float)m_noahEventTexture.height);
        const Rectangle dst = {
            (m_width - m_noahEventTexture.width * scale) / 2.0f,
            (m_height - m_noahEventTexture.height * scale) / 2.0f,
            m_noahEventTexture.width * scale,
            m_noahEventTexture.height * scale
        };
        DrawTexturePro(m_noahEventTexture,
                       { 0.0f, 0.0f, (float)m_noahEventTexture.width, (float)m_noahEventTexture.height },
                       dst,
                       { 0.0f, 0.0f },
                       0.0f,
                       WHITE);
    } else {
        ClearBackground(Colors::dark_bg);
    }

    DrawRectangle(0, 0, m_width, m_height, ColorAlpha(BLACK, 0.35f));

    const int titleSize = scalei(LayoutConfig::NoahEventTitleSize);
    const int subtitleSize = scalei(LayoutConfig::NoahEventSubtitleSize);
    const int leftX = scalei(LayoutConfig::NoahEventLeftMargin);
    const int topY = scalei(LayoutConfig::NoahEventTopMargin);
    DrawText("Noah's Event", leftX, topY, titleSize, Colors::text_primary);

    std::string subtitle = "He offers unreadable help.";
    switch (eventState.getStage()) {
    case NoahEventStage::Options:
        subtitle = "Pick one bargain.";
        break;
    case NoahEventStage::SelectNoahCards:
        subtitle = "Select both Noah cards to keep.";
        break;
    case NoahEventStage::SelectTransformCards:
        subtitle = "Select 2 of your cards to transform.";
        break;
    case NoahEventStage::RevealResult:
        subtitle = "Noah changed your run.";
        break;
    }
    DrawText(subtitle.c_str(), leftX, topY + titleSize + scalei(10), subtitleSize, Colors::text_secondary);

    if (eventState.getStage() == NoahEventStage::Options) {
        const float panelWidth = scalef(LayoutConfig::NoahPanelWidth);
        const float panelHeight = scalef(LayoutConfig::NoahPanelHeight);
        const Rectangle panel = {
            m_width - panelWidth - scalef(LayoutConfig::NoahPanelRightMargin),
            m_height - panelHeight - scalef(LayoutConfig::NoahPanelBottomMargin),
            panelWidth,
            panelHeight
        };
        DrawRectangleRec(panel, ColorAlpha(Colors::light_bg, 0.95f));
        DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);
        DrawText("Options",
                 (int)std::round(panel.x + scalef(18.0f)),
                 (int)std::round(panel.y + scalef(LayoutConfig::NoahPanelTitleTop)),
                 scalei(LayoutConfig::NoahPanelTitleSize),
                 Colors::text_primary);

        struct EventOption {
            const char* title;
            const char* body;
            bool enabled;
        };

        const EventOption options[3] = {
            { "Add 2 Noah cards", "Gain 50g", true },
            { "Transform 2 cards", "Lose 10g", player.getGold() >= 10 && player.getOwnedCards().size() >= 2 },
            { "Remove random card", "Add 1 Noah card", !player.getOwnedCards().empty() }
        };

        for (int index = 0; index < 3; ++index) {
            Rectangle optionRect = {
                panel.x + (panel.width - scalef(LayoutConfig::NoahOptionWidth)) / 2.0f,
                panel.y + scalef(LayoutConfig::NoahOptionTopOffset)
                    + index * scalef(LayoutConfig::NoahOptionHeight + LayoutConfig::NoahOptionGap),
                scalef(LayoutConfig::NoahOptionWidth),
                scalef(LayoutConfig::NoahOptionHeight)
            };
            const bool hovered = allowInteraction && options[index].enabled && mouseOver(optionRect);
            DrawRectangleRec(optionRect, hovered ? Colors::button_hover : Colors::button_bg);
            DrawRectangleLinesEx(optionRect,
                                 scalef(LayoutConfig::ThinBorderThickness),
                                 options[index].enabled ? Colors::card_border : Colors::text_secondary);
            DrawText(options[index].title,
                     (int)std::round(optionRect.x + scalef(12.0f)),
                     (int)std::round(optionRect.y + scalef(10.0f)),
                     scalei(LayoutConfig::NoahOptionTitleSize),
                     options[index].enabled ? Colors::text_primary : Colors::text_secondary);
            DrawText(options[index].body,
                     (int)std::round(optionRect.x + scalef(12.0f)),
                     (int)std::round(optionRect.y + scalef(38.0f)),
                     scalei(LayoutConfig::NoahOptionBodySize),
                     index == 0 ? Colors::gold_color : Colors::text_secondary);
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                return { NoahEventUiActionType::SelectOption, index };
            }
        }

        return {};
    }

    if (eventState.getStage() == NoahEventStage::SelectNoahCards) {
        const float dt = GetFrameTime() * m_timeScale;
        const auto& cards = eventState.getOfferedCards();
        const auto& selected = eventState.getSelectedOfferIndices();
        const float preferredWidth = scalef(LayoutConfig::NoahChoiceCardWidth);
        const float preferredHeight = scalef(LayoutConfig::NoahChoiceCardHeight);
        const float gap = scalef(LayoutConfig::NoahChoiceCardGap);
        const float totalWidth = preferredWidth * cards.size() + gap * std::max(0, (int)cards.size() - 1);
        const float startX = ((float)m_width - totalWidth) / 2.0f;
        const float y = scalef(LayoutConfig::NoahChoiceTop);

        int hoveredCardIndex = -1;
        Rectangle hoveredRect = {};
        float hoveredProgress = 0.0f;
        bool hoveredPicked = false;
        for (int index = 0; index < static_cast<int>(cards.size()); ++index) {
            Rectangle rect = {
                startX + index * (preferredWidth + gap),
                y,
                preferredWidth,
                preferredHeight
            };
            const bool picked = std::find(selected.begin(), selected.end(), index) != selected.end();
            const bool hovered = allowInteraction && mouseOver(rect);
            const float progress = cardHoverProgress("noah-offer:" + std::to_string(index), hovered, dt);
            if (hovered) {
                hoveredCardIndex = index;
                hoveredRect = rect;
                hoveredProgress = progress;
                hoveredPicked = picked;
            } else {
                const Rectangle drawRect = applyCardHoverMotion(rect, progress, LayoutConfig::NoahChoiceHoverScale);
                drawCardFace(drawRect, cards[index], progress > 0.0f, 0.0f, -1, -1, true);
                if (picked) {
                    DrawRectangleLinesEx(drawRect, scalef(LayoutConfig::NoahDeckSelectionThickness), Colors::gold_color);
                }
            }
            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                return { NoahEventUiActionType::ToggleOfferCard, index };
            }
        }

        if (hoveredCardIndex >= 0) {
            const Rectangle drawRect = applyCardHoverMotion(hoveredRect, hoveredProgress, LayoutConfig::NoahChoiceHoverScale);
            drawCardFace(drawRect, cards[hoveredCardIndex], true, 0.0f, -1, -1, true);
            if (hoveredPicked) {
                DrawRectangleLinesEx(drawRect, scalef(LayoutConfig::NoahDeckSelectionThickness), Colors::gold_color);
            }
        }

        const Rectangle confirmRect = {
            ((float)m_width - scalef(LayoutConfig::NoahConfirmButtonWidth)) / 2.0f,
            y + preferredHeight + scalef(LayoutConfig::NoahConfirmButtonBottomGap),
            scalef(LayoutConfig::NoahConfirmButtonWidth),
            scalef(LayoutConfig::NoahConfirmButtonHeight)
        };
        const bool ready = eventState.hasRequiredOfferSelections();
        const bool hovered = allowInteraction && ready && mouseOver(confirmRect);
        DrawRectangleRec(confirmRect, hovered ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(confirmRect,
                             scalef(LayoutConfig::ThinBorderThickness),
                             ready ? Colors::gold_color : Colors::text_secondary);
        DrawText("Claim Both",
                 (int)std::round(confirmRect.x + (confirmRect.width - MeasureText("Claim Both", scalei(LayoutConfig::DefaultButtonFontSize))) / 2.0f),
                 (int)std::round(confirmRect.y + (confirmRect.height - scalei(LayoutConfig::DefaultButtonFontSize)) / 2.0f),
                 scalei(LayoutConfig::DefaultButtonFontSize),
                 ready ? Colors::text_primary : Colors::text_secondary);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return { NoahEventUiActionType::ConfirmOfferCards, 0 };
        }

        return {};
    }

    if (eventState.getStage() == NoahEventStage::SelectTransformCards) {
        const float dt = GetFrameTime() * m_timeScale;
        const float panelWidth = scalef(LayoutConfig::NoahDeckPanelWidth);
        const float panelHeight = scalef(LayoutConfig::NoahDeckPanelHeight);
        const Rectangle panel = {
            ((float)m_width - panelWidth) / 2.0f,
            scalef(LayoutConfig::NoahDeckPanelTop),
            panelWidth,
            panelHeight
        };
        DrawRectangleRec(panel, ColorAlpha(Colors::dark_bg, 0.94f));
        DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

        const int cols = LayoutConfig::NoahDeckColumns;
        const float visibleRows = LayoutConfig::NoahDeckVisibleRows;
        const float gridGap = scalef(LayoutConfig::NoahDeckGridGap);
        const float gridTop = panel.y + scalef(LayoutConfig::NoahDeckTopPadding);
        const float gridHeight = panel.height - scalef(LayoutConfig::NoahDeckTopPadding + LayoutConfig::NoahDeckBottomPadding);
        const float cardAspect = (float)LayoutConfig::CardWidth / (float)LayoutConfig::CardHeight;
        const float cardHeightFromRows = ((gridHeight - gridGap * (visibleRows - 1.0f)) / visibleRows);
        const float cardWidthFromCols = ((panel.width - scalef(48.0f) - gridGap * (cols - 1)) / (float)cols);
        const float drawWidth = std::floor(std::min(cardWidthFromCols, cardHeightFromRows * cardAspect));
        const float drawHeight = std::floor(drawWidth / cardAspect);
        const float gridWidth = drawWidth * cols + gridGap * (cols - 1);
        const float gridX = panel.x + (panel.width - gridWidth) / 2.0f;
        const float rowStride = drawHeight + gridGap;
        const int rows = ((int)player.getOwnedCards().size() + cols - 1) / cols;
        const int fullyVisibleRows = std::max(1, (int)std::floor(visibleRows));
        maxScroll = std::max(0, rows - fullyVisibleRows);

        BeginScissorMode((int)std::round(panel.x),
                         (int)std::round(gridTop),
                         (int)std::round(panel.width),
                         (int)std::round(gridHeight));

        int hoveredCardIndex = -1;
        Rectangle hoveredRect = {};
        float hoveredProgress = 0.0f;
        for (int index = 0; index < static_cast<int>(player.getOwnedCards().size()); ++index) {
            const int col = index % cols;
            const int row = index / cols;
            const Rectangle rect = {
                gridX + col * (drawWidth + gridGap),
                gridTop + (row - scrollOffset) * rowStride,
                drawWidth,
                drawHeight
            };
            if (rect.y + rect.height < gridTop || rect.y > gridTop + gridHeight) {
                continue;
            }

            const bool hovered = allowInteraction && mouseOver(rect);
            const float progress = cardHoverProgress("noah-deck:" + std::to_string(index), hovered, dt);
            if (hovered) {
                hoveredCardIndex = index;
                hoveredRect = rect;
                hoveredProgress = progress;
            } else {
                const Rectangle drawRect = applyCardHoverMotion(rect, progress, LayoutConfig::NoahDeckHoverScale);
                drawCardFace(drawRect, player.getOwnedCards()[index], progress > 0.0f, 0.0f, -1, -1, true);
                if (std::find(eventState.getSelectedDeckIndices().begin(),
                              eventState.getSelectedDeckIndices().end(),
                              index) != eventState.getSelectedDeckIndices().end()) {
                    DrawRectangleLinesEx(drawRect, scalef(LayoutConfig::NoahDeckSelectionThickness), Colors::gold_color);
                }
            }
        }
        EndScissorMode();

        if (hoveredCardIndex >= 0) {
            Rectangle drawRect = applyCardHoverMotion(hoveredRect, hoveredProgress, LayoutConfig::NoahDeckHoverScale);
            drawCardFace(drawRect, player.getOwnedCards()[hoveredCardIndex], true, 0.0f, -1, -1, true);
            if (std::find(eventState.getSelectedDeckIndices().begin(),
                          eventState.getSelectedDeckIndices().end(),
                          hoveredCardIndex) != eventState.getSelectedDeckIndices().end()) {
                DrawRectangleLinesEx(drawRect, scalef(LayoutConfig::NoahDeckSelectionThickness), Colors::gold_color);
            }
            if (allowInteraction && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                return { NoahEventUiActionType::ToggleDeckCard, hoveredCardIndex };
            }
        }

        const Rectangle confirmRect = {
            panel.x + panel.width - scalef(LayoutConfig::NoahConfirmButtonWidth) - scalef(24.0f),
            panel.y + panel.height - scalef(LayoutConfig::NoahConfirmButtonHeight) - scalef(20.0f),
            scalef(LayoutConfig::NoahConfirmButtonWidth),
            scalef(LayoutConfig::NoahConfirmButtonHeight)
        };
        const bool ready = eventState.hasRequiredDeckSelections();
        const bool hovered = allowInteraction && ready && mouseOver(confirmRect);
        DrawRectangleRec(confirmRect, hovered ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(confirmRect,
                             scalef(LayoutConfig::ThinBorderThickness),
                             ready ? Colors::gold_color : Colors::text_secondary);
        DrawText("Transform",
                 (int)std::round(confirmRect.x + (confirmRect.width - MeasureText("Transform", scalei(LayoutConfig::DefaultButtonFontSize))) / 2.0f),
                 (int)std::round(confirmRect.y + (confirmRect.height - scalei(LayoutConfig::DefaultButtonFontSize)) / 2.0f),
                 scalei(LayoutConfig::DefaultButtonFontSize),
                 ready ? Colors::text_primary : Colors::text_secondary);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return { NoahEventUiActionType::ConfirmTransform, 0 };
        }

        return {};
    }

    if (eventState.getStage() == NoahEventStage::RevealResult) {
        const auto& cards = eventState.getResultCards();
        if (!eventState.getRemovedCardName().empty()) {
            const std::string removed = "Removed: " + eventState.getRemovedCardName();
            DrawText(removed.c_str(),
                     leftX,
                     scalei(LayoutConfig::NoahRevealTop),
                     scalei(LayoutConfig::NoahRevealBodySize),
                     Colors::damage_color);
        }
        if (eventState.getGoldDelta() != 0) {
            const std::string goldText = eventState.getGoldDelta() > 0
                ? "Gold +" + std::to_string(eventState.getGoldDelta())
                : "Gold " + std::to_string(eventState.getGoldDelta());
            DrawText(goldText.c_str(),
                     leftX,
                     scalei(LayoutConfig::NoahRevealTop) + scalei(26),
                     scalei(LayoutConfig::NoahRevealBodySize),
                     eventState.getGoldDelta() > 0 ? Colors::gold_color : Colors::damage_color);
        }

        if (!cards.empty()) {
            const float cardWidth = scalef(LayoutConfig::NoahChoiceCardWidth);
            const float cardHeight = scalef(LayoutConfig::NoahChoiceCardHeight);
            const float gap = scalef(LayoutConfig::NoahChoiceCardGap);
            const float totalWidth = cardWidth * cards.size() + gap * std::max(0, (int)cards.size() - 1);
            const float startX = ((float)m_width - totalWidth) / 2.0f;
            for (int index = 0; index < static_cast<int>(cards.size()); ++index) {
                Rectangle rect = {
                    startX + index * (cardWidth + gap),
                    scalef(LayoutConfig::NoahChoiceTop),
                    cardWidth,
                    cardHeight
                };
                drawCardFace(rect, cards[index], false, 0.0f, -1, -1, true);
            }
        }

        const Rectangle continueRect = {
            ((float)m_width - scalef(LayoutConfig::NoahConfirmButtonWidth)) / 2.0f,
            m_height - scalef(LayoutConfig::NoahConfirmButtonHeight + LayoutConfig::NoahConfirmButtonBottomGap),
            scalef(LayoutConfig::NoahConfirmButtonWidth),
            scalef(LayoutConfig::NoahConfirmButtonHeight)
        };
        const bool hovered = allowInteraction && mouseOver(continueRect);
        drawButton(continueRect, "Continue", hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return { NoahEventUiActionType::Continue, 0 };
        }
    }

    return {};
}
