#pragma once

#include "raylib.h"

namespace UiFont {

Font getFont();
int measureText(const char* text, int fontSize);
void drawText(const char* text, int x, int y, int fontSize, Color color);
void unload();

} // namespace UiFont
