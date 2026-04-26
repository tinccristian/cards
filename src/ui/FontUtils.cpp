#include "FontUtils.h"

#include "config/Defines.h"

#include <cmath>

namespace {

Font g_font = {};
bool g_loaded = false;
bool g_available = false;

} // namespace

namespace UiFont {

Font getFont() {
    if (g_loaded) {
        return g_available ? g_font : GetFontDefault();
    }

    g_loaded = true;
    if (!FileExists(AssetPaths::CARD_FONT)) {
        TraceLog(LOG_WARNING, "Missing UI font: %s", AssetPaths::CARD_FONT);
        return GetFontDefault();
    }

    g_font = LoadFontEx(AssetPaths::CARD_FONT, 64, nullptr, 0);
    g_available = (g_font.texture.id != 0);
    if (!g_available) {
        TraceLog(LOG_WARNING, "Failed to load UI font: %s", AssetPaths::CARD_FONT);
        return GetFontDefault();
    }

    SetTextureFilter(g_font.texture, TEXTURE_FILTER_POINT);
    return g_font;
}

int measureText(const char* text, int fontSize) {
    if (text == nullptr || text[0] == '\0') {
        return 0;
    }
    const Vector2 size = MeasureTextEx(getFont(), text, (float)fontSize, 0.0f);
    return (int)std::ceil(size.x);
}

void drawText(const char* text, int x, int y, int fontSize, Color color) {
    if (text == nullptr) {
        return;
    }
    DrawTextEx(getFont(), text, { (float)x, (float)y }, (float)fontSize, 0.0f, color);
}

void unload() {
    if (g_available) {
        UnloadFont(g_font);
        g_font = {};
    }
    g_loaded = false;
    g_available = false;
}

} // namespace UiFont
