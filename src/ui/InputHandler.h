#pragma once

#include "raylib.h"

// Thin wrapper around raylib input queries, grouped by logical action.
namespace InputHandler {
    // Returns -1 (up), 0 (none), or +1 (down) based on arrow keys or mouse wheel.
    int  getScrollInput();

    // True if ESC was pressed this frame.
    bool getEscapePressed();

    // True if the left mouse button was pressed inside `area` this frame.
    bool getAreaClicked(Rectangle area);
}
