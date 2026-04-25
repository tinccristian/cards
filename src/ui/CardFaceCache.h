#pragma once

#include "config/Defines.h"
#include "core/Card.h"
#include "raylib.h"

#include <optional>
#include <string>
#include <unordered_map>

class CardFaceCache {
public:
    struct FaceLayout {
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

    // `crispPresentation` is used for stable large-card displays (rewards,
    // viewers) where we want sharper sampling than the animated hand cards.
    std::optional<Texture2D> getTexture(const Card& card,
                                        int targetWidth,
                                        int targetHeight,
                                        bool emphasized,
                                        const std::string& visibleCostText,
                                        bool affordable,
                                        bool crispPresentation = false);
    Texture2D buildPreviewTexture(const Card& card,
                                  int targetWidth,
                                  int targetHeight,
                                  const std::string& visibleCostText,
                                  const FaceLayout& layout);
    void unloadAll();

private:
    struct FaceKey {
        std::string id;
        std::string name;
        std::string description;
        std::string artPath;
        std::string visibleCostText;
        int         damage = 0;
        int         block = 0;
        int         heal = 0;
        int         draw = 0;
        int         nextTurnMana = 0;
        bool        hideFooterStats = false;
        int         targetWidth = 0;
        int         targetHeight = 0;
        bool        emphasized = false;
        bool        affordable = false;
        bool        crispPresentation = false;

        bool operator==(const FaceKey& other) const;
    };

    struct FaceKeyHash {
        std::size_t operator()(const FaceKey& key) const;
    };

    bool ensureBorderLoaded();
    const Image* getArtImage(const std::string& path);
    Texture2D buildTexture(const Card& card,
                           int targetWidth,
                           int targetHeight,
                           bool emphasized,
                           const std::string& visibleCostText,
                           bool affordable,
                           bool crispPresentation,
                           const FaceLayout& layout);

    std::unordered_map<FaceKey, Texture2D, FaceKeyHash> m_faces;
    std::unordered_map<std::string, Image>              m_artImages;
    Image                                               m_borderImage = {};
    bool                                                m_borderLoaded = false;
    bool                                                m_borderAvailable = false;
};
