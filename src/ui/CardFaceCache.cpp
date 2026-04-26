#include "CardFaceCache.h"

#include <functional>

namespace {

void hashCombine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

CardRenderLayout defaultLayout() {
    return {};
}

} // namespace

bool CardFaceCache::FaceKey::operator==(const FaceKey& other) const {
    return id == other.id
        && name == other.name
        && description == other.description
        && artPath == other.artPath
        && visibleCostText == other.visibleCostText
        && damage == other.damage
        && block == other.block
        && heal == other.heal
        && draw == other.draw
        && nextTurnMana == other.nextTurnMana
        && hideFooterStats == other.hideFooterStats
        && targetWidth == other.targetWidth
        && targetHeight == other.targetHeight
        && emphasized == other.emphasized
        && affordable == other.affordable
        && mode == other.mode
        && layout.manaLeft == other.layout.manaLeft
        && layout.manaTop == other.layout.manaTop
        && layout.manaRight == other.layout.manaRight
        && layout.manaBottom == other.layout.manaBottom
        && layout.artLeft == other.layout.artLeft
        && layout.artTop == other.layout.artTop
        && layout.artRight == other.layout.artRight
        && layout.artBottom == other.layout.artBottom
        && layout.nameLeft == other.layout.nameLeft
        && layout.nameTop == other.layout.nameTop
        && layout.nameRight == other.layout.nameRight
        && layout.nameBottom == other.layout.nameBottom
        && layout.descriptionLeft == other.layout.descriptionLeft
        && layout.descriptionTop == other.layout.descriptionTop
        && layout.descriptionRight == other.layout.descriptionRight
        && layout.descriptionBottom == other.layout.descriptionBottom;
}

std::size_t CardFaceCache::FaceKeyHash::operator()(const FaceKey& key) const {
    std::size_t seed = 0;
    hashCombine(seed, std::hash<std::string>{}(key.id));
    hashCombine(seed, std::hash<std::string>{}(key.name));
    hashCombine(seed, std::hash<std::string>{}(key.description));
    hashCombine(seed, std::hash<std::string>{}(key.artPath));
    hashCombine(seed, std::hash<std::string>{}(key.visibleCostText));
    hashCombine(seed, std::hash<int>{}(key.damage));
    hashCombine(seed, std::hash<int>{}(key.block));
    hashCombine(seed, std::hash<int>{}(key.heal));
    hashCombine(seed, std::hash<int>{}(key.draw));
    hashCombine(seed, std::hash<int>{}(key.nextTurnMana));
    hashCombine(seed, std::hash<bool>{}(key.hideFooterStats));
    hashCombine(seed, std::hash<int>{}(key.targetWidth));
    hashCombine(seed, std::hash<int>{}(key.targetHeight));
    hashCombine(seed, std::hash<bool>{}(key.emphasized));
    hashCombine(seed, std::hash<bool>{}(key.affordable));
    hashCombine(seed, std::hash<int>{}((int)key.mode));
    hashCombine(seed, std::hash<int>{}(key.layout.manaLeft));
    hashCombine(seed, std::hash<int>{}(key.layout.manaTop));
    hashCombine(seed, std::hash<int>{}(key.layout.manaRight));
    hashCombine(seed, std::hash<int>{}(key.layout.manaBottom));
    hashCombine(seed, std::hash<int>{}(key.layout.artLeft));
    hashCombine(seed, std::hash<int>{}(key.layout.artTop));
    hashCombine(seed, std::hash<int>{}(key.layout.artRight));
    hashCombine(seed, std::hash<int>{}(key.layout.artBottom));
    hashCombine(seed, std::hash<int>{}(key.layout.nameLeft));
    hashCombine(seed, std::hash<int>{}(key.layout.nameTop));
    hashCombine(seed, std::hash<int>{}(key.layout.nameRight));
    hashCombine(seed, std::hash<int>{}(key.layout.nameBottom));
    hashCombine(seed, std::hash<int>{}(key.layout.descriptionLeft));
    hashCombine(seed, std::hash<int>{}(key.layout.descriptionTop));
    hashCombine(seed, std::hash<int>{}(key.layout.descriptionRight));
    hashCombine(seed, std::hash<int>{}(key.layout.descriptionBottom));
    return seed;
}

std::optional<Texture2D> CardFaceCache::getTexture(const Card& card,
                                                   int targetWidth,
                                                   int targetHeight,
                                                   bool emphasized,
                                                   const std::string& visibleCostText,
                                                   bool affordable,
                                                   bool crispPresentation) {
    if (targetWidth <= 0 || targetHeight <= 0) {
        return std::nullopt;
    }

    const CardRenderMode mode = crispPresentation ? CardRenderMode::Static : CardRenderMode::Animated;
    const FaceKey key{
        card.getId(),
        card.getDisplayName(),
        card.getDisplayDescription(),
        card.getArtPath(),
        visibleCostText,
        card.getDamageAmount(),
        card.getBlockAmount(),
        card.getHealAmount(),
        card.getCardsDrawn(),
        card.getNextTurnManaAmount(),
        card.shouldHideFooterStats(),
        targetWidth,
        targetHeight,
        emphasized,
        affordable,
        mode,
        defaultLayout()
    };

    auto existing = m_faces.find(key);
    if (existing != m_faces.end()) {
        return existing->second;
    }

    Texture2D texture = m_renderer.buildTexture(
        card,
        targetWidth,
        targetHeight,
        CardRenderOptions{ emphasized, visibleCostText, affordable, mode },
        key.layout);
    if (texture.id == 0) {
        return std::nullopt;
    }

    m_faces.emplace(key, texture);
    return texture;
}

Texture2D CardFaceCache::buildPreviewTexture(const Card& card,
                                             int targetWidth,
                                             int targetHeight,
                                             const std::string& visibleCostText,
                                             const FaceLayout& layout) {
    return m_renderer.buildTexture(
        card,
        targetWidth,
        targetHeight,
        CardRenderOptions{ true, visibleCostText, true, CardRenderMode::Static },
        layout);
}

void CardFaceCache::unloadAll() {
    for (auto& [_, texture] : m_faces) {
        if (texture.id != 0) {
            UnloadTexture(texture);
            texture = {};
        }
    }
    m_faces.clear();
    m_renderer.unloadAssets();
}
