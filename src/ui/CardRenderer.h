#pragma once

#include "config/Defines.h"
#include "core/Card.h"
#include "raylib.h"

#include <string>
#include <unordered_map>

struct CardRenderLayout {
    int manaLeft = LayoutConfig::CardManaBoxLeft;
    int manaTop = LayoutConfig::CardManaBoxTop;
    int manaRight = LayoutConfig::CardManaBoxRight;
    int manaBottom = LayoutConfig::CardManaBoxBottom;
    int artLeft = LayoutConfig::CardArtBoxLeft;
    int artTop = LayoutConfig::CardArtBoxTop;
    int artRight = LayoutConfig::CardArtBoxRight;
    int artBottom = LayoutConfig::CardArtBoxBottom;
    int nameLeft = LayoutConfig::CardNameBoxLeft;
    int nameTop = LayoutConfig::CardNameBoxTop;
    int nameRight = LayoutConfig::CardNameBoxRight;
    int nameBottom = LayoutConfig::CardNameBoxBottom;
    int descriptionLeft = LayoutConfig::CardDescriptionBoxLeft;
    int descriptionTop = LayoutConfig::CardDescriptionBoxTop;
    int descriptionRight = LayoutConfig::CardDescriptionBoxRight;
    int descriptionBottom = LayoutConfig::CardDescriptionBoxBottom;
};

enum class CardRenderMode {
    Animated,
    Static
};

struct CardRenderOptions {
    bool emphasized = false;
    std::string visibleCostText;
    bool affordable = true;
    CardRenderMode mode = CardRenderMode::Animated;
};

class CardRenderer {
public:
    Texture2D buildTexture(const Card& card,
                           int targetWidth,
                           int targetHeight,
                           const CardRenderOptions& options,
                           const CardRenderLayout& layout);
    void unloadAssets();

private:
    bool ensureBorderLoaded();
    const Image* getArtImage(const std::string& path);

    std::unordered_map<std::string, Image> m_artImages;
    Image m_borderImage = {};
    bool m_borderLoaded = false;
    bool m_borderAvailable = false;
};
