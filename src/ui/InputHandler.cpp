#include "InputHandler.h"

namespace InputHandler {

int getScrollInput() {
    float wheel = GetMouseWheelMove();
    if (wheel > 0.0f || IsKeyPressed(KEY_UP))   return -1;
    if (wheel < 0.0f || IsKeyPressed(KEY_DOWN)) return  1;
    return 0;
}

bool getEscapePressed() {
    return IsKeyPressed(KEY_ESCAPE);
}

bool getAreaClicked(Rectangle area) {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
           CheckCollisionPointRec(GetMousePosition(), area);
}

} // namespace InputHandler
