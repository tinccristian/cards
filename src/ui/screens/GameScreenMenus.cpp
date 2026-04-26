#include "ui/GameScreen.h"
#include "ui/FontUtils.h"

#include <algorithm>
#include <iterator>

#define DrawText UiFont::drawText
#define MeasureText UiFont::measureText

GameOverAction GameScreen::drawGameOver(const GameState& state) {
    syncWindowSize();
    (void)state;

    const Color mid = { 0, 0, 0, (unsigned char)(LayoutConfig::GameOverVignetteAlpha / 2) };
    const Color edge = { 0, 0, 0, (unsigned char)LayoutConfig::GameOverVignetteAlpha };
    DrawRectangle(0, 0, m_width, m_height, mid);
    DrawRectangleGradientV(0, 0, m_width, m_height / 3, edge, BLANK);
    DrawRectangleGradientV(0, m_height - m_height / 3, m_width, m_height / 3, BLANK, edge);
    DrawRectangleGradientH(0, 0, m_width / 4, m_height, edge, BLANK);
    DrawRectangleGradientH(m_width - m_width / 4, 0, m_width / 4, m_height, BLANK, edge);

    const int panelW = scalei(LayoutConfig::GameOverPanelWidth);
    const int panelH = scalei(LayoutConfig::GameOverPanelHeight);
    const Rectangle panel = {
        (float)((m_width - panelW) / 2),
        (float)((m_height - panelH) / 2),
        (float)panelW,
        (float)panelH
    };
    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const char* title = "You Died";
    const int titleSize = scalei(LayoutConfig::GameOverFontSize);
    DrawText(title,
             (int)(panel.x + (panel.width - MeasureText(title, titleSize)) / 2.0f),
             (int)(panel.y + scalef(62.0f)),
             titleSize,
             Colors::damage_color);

    const int btnW = scalei(LayoutConfig::GameOverButtonWidth);
    const int btnH = scalei(LayoutConfig::GameOverButtonHeight);
    const int btnGap = scalei(LayoutConfig::GameOverButtonGap);
    Rectangle newRunBtn = {
        panel.x + (panel.width - btnW) / 2.0f,
        panel.y + panel.height - btnH * 2.0f - btnGap - scalef(32.0f),
        (float)btnW,
        (float)btnH
    };
    Rectangle menuBtn = {
        panel.x + (panel.width - btnW) / 2.0f,
        newRunBtn.y + btnH + btnGap,
        (float)btnW,
        (float)btnH
    };

    const bool newRunHovered = mouseOver(newRunBtn);
    const bool menuHovered = mouseOver(menuBtn);
    drawButton(newRunBtn, "New Run", newRunHovered);
    drawButton(menuBtn, "Main Menu", menuHovered);

    if (newRunHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return GameOverAction::NewRun;
    }
    if (menuHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return GameOverAction::MainMenu;
    }
    return GameOverAction::None;
}

PauseAction GameScreen::drawPauseMenu() {
    syncWindowSize();
    const int panelWidth = scalei(LayoutConfig::OverlayPanelWidth);
    const int panelHeight = scalei(LayoutConfig::PausePanelHeight);
    const int titleFontSize = scalei(LayoutConfig::OverlayTitleFontSize);
    const int contentInset = scalei(LayoutConfig::OverlayContentInset);
    const int buttonWidth = scalei(LayoutConfig::OverlayButtonWidth);
    const int buttonHeight = scalei(LayoutConfig::OverlayButtonHeight);
    const int buttonGap = scalei(LayoutConfig::OverlayButtonGap);
    const int buttonsTopOffset = scalei(LayoutConfig::PauseButtonsTopOffset);

    DrawRectangle(0, 0, m_width, m_height, Colors::overlay_bg);

    const float panelX = (m_width - panelWidth) / 2.0f;
    const float panelY = (m_height - panelHeight) / 2.0f;
    Rectangle panel = {
        panelX,
        panelY,
        (float)panelWidth,
        (float)panelHeight
    };
    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const char* title = "Paused";
    DrawText(title,
             (int)(panel.x + (panel.width - MeasureText(title, titleFontSize)) / 2.0f),
             (int)(panel.y + contentInset),
             titleFontSize,
             Colors::text_primary);

    struct ButtonEntry {
        const char* label;
        PauseAction action;
    };

    const ButtonEntry buttons[] = {
        { "Resume", PauseAction::Resume },
        { "Options", PauseAction::Options },
        { "Main Menu", PauseAction::MainMenu },
        { "Quit", PauseAction::Quit }
    };

    for (int index = 0; index < static_cast<int>(std::size(buttons)); ++index) {
        Rectangle buttonRect = {
            panel.x + (panel.width - buttonWidth) / 2.0f,
            panel.y + buttonsTopOffset + index * (buttonHeight + buttonGap),
            (float)buttonWidth,
            (float)buttonHeight
        };
        const bool hovered = mouseOver(buttonRect);
        drawButton(buttonRect, buttons[index].label, hovered);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            return buttons[index].action;
        }
    }

    return PauseAction::None;
}

OptionsMenuAction GameScreen::drawOptionsMenu(AppSettings& settings,
                                              OptionsSection& activeSection,
                                              bool openedFromPause) {
    syncWindowSize();

    const int panelWidth = scalei(LayoutConfig::OverlayPanelWidth);
    const int panelHeight = scalei(LayoutConfig::OptionsPanelHeight);
    const int titleFontSize = scalei(LayoutConfig::OverlayTitleFontSize);
    const int contentInset = scalei(LayoutConfig::OverlayContentInset);
    const int tabWidth = scalei(LayoutConfig::OptionsTabWidth);
    const int tabHeight = scalei(LayoutConfig::OptionsTabHeight);
    const int tabGap = scalei(LayoutConfig::OptionsTabGap);
    const int tabTopOffset = scalei(LayoutConfig::OptionsTabTopOffset);
    const int rowsTopOffset = scalei(LayoutConfig::OptionsRowsTopOffset);
    const int rowHeight = scalei(LayoutConfig::OptionsRowHeight);
    const int footerOffset = scalei(LayoutConfig::OptionsFooterOffsetY);
    const int buttonWidth = scalei(LayoutConfig::OverlayButtonWidth);
    const int buttonHeight = scalei(LayoutConfig::OverlayButtonHeight);
    const int monitorCount = std::max(1, GetMonitorCount());

    DrawRectangle(0, 0, m_width, m_height, Colors::overlay_bg);

    const float panelX = (m_width - panelWidth) / 2.0f;
    const float panelY = (m_height - panelHeight) / 2.0f;
    Rectangle panel = {
        panelX,
        panelY,
        (float)panelWidth,
        (float)panelHeight
    };
    DrawRectangleRec(panel, Colors::light_bg);
    DrawRectangleLinesEx(panel, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);

    const char* title = openedFromPause ? "Pause Options" : "Options";
    DrawText(title,
             (int)(panel.x + (panel.width - MeasureText(title, titleFontSize)) / 2.0f),
             (int)(panel.y + contentInset),
             titleFontSize,
             Colors::text_primary);

    struct TabEntry {
        const char* label;
        OptionsSection section;
    };

    const TabEntry tabs[] = {
        { "Display", OptionsSection::Display },
        { "Sounds", OptionsSection::Sounds }
    };

    const int tabCount = static_cast<int>(std::size(tabs));
    const float totalTabsWidth = tabCount * tabWidth + (tabCount - 1) * tabGap;
    const float tabsStartX = panel.x + (panel.width - totalTabsWidth) / 2.0f;
    const float tabY = panel.y + tabTopOffset;

    for (int index = 0; index < tabCount; ++index) {
        Rectangle tabRect = {
            tabsStartX + index * (tabWidth + tabGap),
            tabY,
            (float)tabWidth,
            (float)tabHeight
        };
        const bool selected = activeSection == tabs[index].section;
        const bool hovered = mouseOver(tabRect);
        DrawRectangleRec(tabRect, selected ? Colors::button_hover : Colors::button_bg);
        DrawRectangleLinesEx(tabRect, scalef(LayoutConfig::PanelBorderThickness), Colors::card_border);
        const int fontSize = scalei(LayoutConfig::DefaultButtonFontSize);
        DrawText(tabs[index].label,
                 (int)(tabRect.x + (tabRect.width - MeasureText(tabs[index].label, fontSize)) / 2.0f),
                 (int)(tabRect.y + (tabRect.height - fontSize) / 2.0f),
                 fontSize,
                 Colors::text_primary);
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            activeSection = tabs[index].section;
        }
    }

    const float rowStartY = panel.y + rowsTopOffset;
    int rowIndex = 0;

    if (activeSection == OptionsSection::Display) {
        const auto& resolutionOptions = SettingsManager::resolutionOptions();
        const int resolutionCount = static_cast<int>(resolutionOptions.size());

        std::string resolution = SettingsManager::resolutionLabel(settings.resolutionIndex);
        const int resolutionDelta = drawStepperRow(rowStartY + rowIndex++ * rowHeight, "Resolution", resolution);
        if (resolutionDelta != 0) {
            settings.resolutionIndex = std::clamp(settings.resolutionIndex + resolutionDelta, 0, resolutionCount - 1);
        }

        const char* modeLabel = SettingsManager::windowModeLabel(settings.windowMode);
        const int modeDelta = drawStepperRow(rowStartY + rowIndex++ * rowHeight, "Window Mode", modeLabel);
        if (modeDelta != 0) {
            int modeIndex = static_cast<int>(settings.windowMode);
            modeIndex = std::clamp(modeIndex + modeDelta, 0, 2);
            settings.windowMode = static_cast<WindowMode>(modeIndex);
        }

        const std::string displayLabel = std::to_string(settings.monitorIndex + 1);
        const int displayDelta = drawStepperRow(rowStartY + rowIndex++ * rowHeight, "Display", displayLabel);
        if (displayDelta != 0) {
            settings.monitorIndex = std::clamp(settings.monitorIndex + displayDelta, 0, monitorCount - 1);
        }

        if (drawCheckboxRow(rowStartY + rowIndex++ * rowHeight, "VSync", settings.vsyncEnabled)) {
            settings.vsyncEnabled = !settings.vsyncEnabled;
        }

        if (drawCheckboxRow(rowStartY + rowIndex++ * rowHeight, "Show FPS", settings.showFps)) {
            settings.showFps = !settings.showFps;
        }
    } else {
        const int volumeDelta = drawStepperRow(rowStartY, "Master Volume", std::to_string(settings.masterVolume));
        if (volumeDelta != 0) {
            settings.masterVolume = std::clamp(settings.masterVolume + volumeDelta * AudioConfig::MasterVolumeStep, 0, 100);
        }
    }

    Rectangle backButton = {
        panel.x + (panel.width - buttonWidth) / 2.0f,
        panel.y + panel.height - footerOffset,
        (float)buttonWidth,
        (float)buttonHeight
    };
    const bool hovered = mouseOver(backButton);
    drawButton(backButton, "Back", hovered);
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return OptionsMenuAction::Back;
    }

    return OptionsMenuAction::None;
}
