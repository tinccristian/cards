#pragma once

#include "core/Card.h"
#include "ui/CardRenderer.h"
#include "raylib.h"

#include <optional>
#include <string>
#include <unordered_map>

class CardFaceCache {
public:
    using FaceLayout = CardRenderLayout;

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
        CardRenderMode mode = CardRenderMode::Animated;
        FaceLayout layout;

        bool operator==(const FaceKey& other) const;
    };

    struct FaceKeyHash {
        std::size_t operator()(const FaceKey& key) const;
    };

    std::unordered_map<FaceKey, Texture2D, FaceKeyHash> m_faces;
    CardRenderer m_renderer;
};
