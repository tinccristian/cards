#include "UIState.h"
#include <algorithm>

UIState::UIState() = default;

UIMode UIState::getCurrentMode() const { return m_mode; }

void UIState::setMode(UIMode mode) {
    m_mode = mode;
    if (mode == UIMode::NORMAL) m_scrollOffset = 0; // reset scroll when closing
}

int UIState::getScrollOffset() const { return m_scrollOffset; }

void UIState::scrollUp() {
    if (m_scrollOffset > 0) --m_scrollOffset;
}

void UIState::scrollDown(int maxOffset) {
    if (m_scrollOffset < maxOffset) ++m_scrollOffset;
}

void UIState::clampScroll(int maxOffset) {
    if (m_scrollOffset > maxOffset) {
        m_scrollOffset = std::max(0, maxOffset);
    }
}

void UIState::resetScroll() { m_scrollOffset = 0; }
