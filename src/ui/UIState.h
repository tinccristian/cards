#pragma once

enum class UIMode {
    NORMAL,
    VIEWING_DRAW_PILE,
    VIEWING_DISCARD_PILE,
    VIEWING_RUN_DECK,
    VIEWING_MAP,
    PEEK_SELECT,   // Second Opinion: pick 1 of top-N cards to add to hand for free this turn
    PAUSED,
    OPTIONS
};

// Lightweight UI state: tracks which overlay (if any) is open and scroll position.
class UIState {
public:
    UIState();

    UIMode getCurrentMode() const;
    void   setMode(UIMode mode);

    // Scroll offset in card-rows for pile viewers
    int  getScrollOffset() const;
    void scrollUp();
    void scrollDown(int maxOffset);
    void clampScroll(int maxOffset);
    void resetScroll();

private:
    UIMode m_mode         = UIMode::NORMAL;
    int    m_scrollOffset = 0;
};
