#include "CardFaceCache.h"

#include "config/Defines.h"
#include "ui/Colors.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace {

void hashCombine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

int scaledInt(int value, float scale) {
    return std::max(1, (int)std::lround((float)value * scale));
}

int outlineThickness(float renderScale) {
    return std::max(1, (int)std::lround(renderScale));
}

Rectangle sourceBoxToRect(int left, int top, int right, int bottom, int width, int height) {
    const float sx = (float)width / (float)LayoutConfig::CardBorderSourceWidth;
    const float sy = (float)height / (float)LayoutConfig::CardBorderSourceHeight;
    return {
        left * sx,
        top * sy,
        std::max(1.0f, (right - left) * sx),
        std::max(1.0f, (bottom - top) * sy)
    };
}

void imageDrawTextOutlined(Image* image,
                           Font font,
                           const char* text,
                           float x,
                           float y,
                           float fontSize,
                           float spacing,
                           float renderScale,
                           Color color) {
    const float o = (float)outlineThickness(renderScale);
    ImageDrawTextEx(image, font, text, { x - o, y }, fontSize, spacing, BLACK);
    ImageDrawTextEx(image, font, text, { x + o, y }, fontSize, spacing, BLACK);
    ImageDrawTextEx(image, font, text, { x, y - o }, fontSize, spacing, BLACK);
    ImageDrawTextEx(image, font, text, { x, y + o }, fontSize, spacing, BLACK);
    ImageDrawTextEx(image, font, text, { x, y }, fontSize, spacing, color);
}

float measureTextWidth(Font font, const std::string& text, float fontSize, float spacing) {
    return MeasureTextEx(font, text.c_str(), fontSize, spacing).x;
}

std::string fitTextWithEllipsis(Font font, const std::string& text, float fontSize, float spacing, float maxWidth) {
    if (measureTextWidth(font, text, fontSize, spacing) <= maxWidth) {
        return text;
    }

    const std::string ellipsis = "...";
    std::string fitted;
    for (char ch : text) {
        const std::string candidate = fitted + ch;
        if (measureTextWidth(font, candidate + ellipsis, fontSize, spacing) > maxWidth) {
            break;
        }
        fitted = candidate;
    }

    if (fitted.empty()) {
        return ellipsis;
    }
    return fitted + ellipsis;
}

std::vector<std::string> wrapTextToWidth(Font font,
                                         const std::string& text,
                                         float fontSize,
                                         float spacing,
                                         float maxWidth,
                                         int maxLines) {
    std::vector<std::string> lines;
    if (text.empty() || maxLines <= 0) {
        return lines;
    }

    std::vector<std::string> words;
    std::string currentWord;
    for (char ch : text) {
        if (ch == ' ') {
            if (!currentWord.empty()) {
                words.push_back(currentWord);
                currentWord.clear();
            }
        } else {
            currentWord += ch;
        }
    }
    if (!currentWord.empty()) {
        words.push_back(currentWord);
    }

    std::string currentLine;
    for (std::size_t i = 0; i < words.size(); ++i) {
        const std::string& word = words[i];
        const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;

        if (measureTextWidth(font, candidate, fontSize, spacing) <= maxWidth) {
            currentLine = candidate;
            continue;
        }

        if (!currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine.clear();
            if ((int)lines.size() >= maxLines - 1) {
                std::string remaining = word;
                for (std::size_t j = i + 1; j < words.size(); ++j) {
                    remaining += " " + words[j];
                }
                lines.push_back(fitTextWithEllipsis(font, remaining, fontSize, spacing, maxWidth));
                return lines;
            }
        }

        if (measureTextWidth(font, word, fontSize, spacing) <= maxWidth) {
            currentLine = word;
            continue;
        }

        std::string chunk;
        for (char ch : word) {
            const std::string chunkCandidate = chunk + ch;
            if (!chunk.empty() && measureTextWidth(font, chunkCandidate, fontSize, spacing) > maxWidth) {
                lines.push_back(chunk);
                if ((int)lines.size() >= maxLines - 1) {
                    std::string remaining = std::string(1, ch);
                    if (&word != &words.back()) {
                        for (std::size_t j = i + 1; j < words.size(); ++j) {
                            remaining += " " + words[j];
                        }
                    }
                    lines.push_back(fitTextWithEllipsis(font, remaining, fontSize, spacing, maxWidth));
                    return lines;
                }
                chunk = std::string(1, ch);
            } else {
                chunk = chunkCandidate;
            }
        }
        currentLine = chunk;
    }

    if (!currentLine.empty() && (int)lines.size() < maxLines) {
        lines.push_back(currentLine);
    }

    return lines;
}

} // namespace

bool CardFaceCache::FaceKey::operator==(const FaceKey& other) const {
    return id == other.id
        && name == other.name
        && description == other.description
        && artPath == other.artPath
        && cost == other.cost
        && damage == other.damage
        && block == other.block
        && heal == other.heal
        && draw == other.draw
        && nextTurnMana == other.nextTurnMana
        && targetWidth == other.targetWidth
        && targetHeight == other.targetHeight
        && emphasized == other.emphasized
        && affordable == other.affordable
        && crispPresentation == other.crispPresentation;
}

std::size_t CardFaceCache::FaceKeyHash::operator()(const FaceKey& key) const {
    std::size_t seed = 0;
    hashCombine(seed, std::hash<std::string>{}(key.id));
    hashCombine(seed, std::hash<std::string>{}(key.name));
    hashCombine(seed, std::hash<std::string>{}(key.description));
    hashCombine(seed, std::hash<std::string>{}(key.artPath));
    hashCombine(seed, std::hash<int>{}(key.cost));
    hashCombine(seed, std::hash<int>{}(key.damage));
    hashCombine(seed, std::hash<int>{}(key.block));
    hashCombine(seed, std::hash<int>{}(key.heal));
    hashCombine(seed, std::hash<int>{}(key.draw));
    hashCombine(seed, std::hash<int>{}(key.nextTurnMana));
    hashCombine(seed, std::hash<int>{}(key.targetWidth));
    hashCombine(seed, std::hash<int>{}(key.targetHeight));
    hashCombine(seed, std::hash<bool>{}(key.emphasized));
    hashCombine(seed, std::hash<bool>{}(key.affordable));
    hashCombine(seed, std::hash<bool>{}(key.crispPresentation));
    return seed;
}

std::optional<Texture2D> CardFaceCache::getTexture(const Card& card,
                                                   int targetWidth,
                                                   int targetHeight,
                                                   bool emphasized,
                                                   bool affordable,
                                                   bool crispPresentation) {
    if (targetWidth <= 0 || targetHeight <= 0) {
        return std::nullopt;
    }

    const FaceKey key{
        card.getId(),
        card.getName(),
        card.getDescription(),
        card.getArtPath(),
        card.getCost(),
        card.getDamageAmount(),
        card.getBlockAmount(),
        card.getHealAmount(),
        card.getCardsDrawn(),
        card.getNextTurnManaAmount(),
        targetWidth,
        targetHeight,
        emphasized,
        affordable,
        crispPresentation
    };

    auto existing = m_faces.find(key);
    if (existing != m_faces.end()) {
        return existing->second;
    }

    Texture2D texture = buildTexture(card, targetWidth, targetHeight, emphasized, affordable, crispPresentation);
    if (texture.id == 0) {
        return std::nullopt;
    }

    m_faces.emplace(key, texture);
    return texture;
}

void CardFaceCache::unloadAll() {
    for (auto& [_, texture] : m_faces) {
        if (texture.id != 0) {
            UnloadTexture(texture);
            texture = {};
        }
    }
    m_faces.clear();

    for (auto& [_, image] : m_artImages) {
        if (image.data != nullptr) {
            UnloadImage(image);
            image = {};
        }
    }
    m_artImages.clear();

    if (m_borderImage.data != nullptr) {
        UnloadImage(m_borderImage);
        m_borderImage = {};
    }
    m_borderLoaded = false;
    m_borderAvailable = false;
}

bool CardFaceCache::ensureBorderLoaded() {
    if (m_borderLoaded) {
        return m_borderAvailable;
    }

    m_borderLoaded = true;
    if (!FileExists(AssetPaths::CARD_BORDER)) {
        return false;
    }

    m_borderImage = LoadImage(AssetPaths::CARD_BORDER);
    m_borderAvailable = (m_borderImage.data != nullptr);
    return m_borderAvailable;
}

const Image* CardFaceCache::getArtImage(const std::string& path) {
    if (path.empty()) {
        return nullptr;
    }

    auto existing = m_artImages.find(path);
    if (existing != m_artImages.end()) {
        return &existing->second;
    }

    if (!FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "Missing card art: %s", path.c_str());
        return nullptr;
    }

    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) {
        TraceLog(LOG_WARNING, "Failed to load card art image: %s", path.c_str());
        return nullptr;
    }

    auto [it, _] = m_artImages.emplace(path, image);
    return &it->second;
}

void drawCardArtNearest(Image* canvas, const Image& artImage, Rectangle artRect) {
    Image resizedArt = ImageCopy(artImage);
    if (resizedArt.data == nullptr) {
        return;
    }

    const int targetWidth = std::max(1, (int)std::lround(artRect.width));
    const int targetHeight = std::max(1, (int)std::lround(artRect.height));
    ImageResizeNN(&resizedArt, targetWidth, targetHeight);
    Rectangle src = { 0.0f, 0.0f, (float)resizedArt.width, (float)resizedArt.height };
    Rectangle dst = {
        std::round(artRect.x),
        std::round(artRect.y),
        (float)targetWidth,
        (float)targetHeight
    };
    ImageDraw(canvas, resizedArt, src, dst, WHITE);
    UnloadImage(resizedArt);
}

Texture2D CardFaceCache::buildTexture(const Card& card,
                                      int targetWidth,
                                      int targetHeight,
                                      bool emphasized,
                                      bool affordable,
                                      bool crispPresentation) {
    const float renderScale = LayoutConfig::CardFaceRenderScale;
    const int internalWidth = std::max(1, (int)std::lround((float)targetWidth * renderScale));
    const int internalHeight = std::max(1, (int)std::lround((float)targetHeight * renderScale));
    const float cardScale = std::min(
        (float)internalWidth / (float)LayoutConfig::CardWidth,
        (float)internalHeight / (float)LayoutConfig::CardHeight);

    Image canvas = GenImageColor(internalWidth, internalHeight, BLANK);
    if (canvas.data == nullptr) {
        return {};
    }

    const bool hasBorderImage = ensureBorderLoaded();
    const Color bg = emphasized ? Colors::button_hover : Colors::card_bg;
    if (!hasBorderImage) {
        ImageDrawRectangleRec(&canvas, { 0.0f, 0.0f, (float)internalWidth, (float)internalHeight }, bg);
    }

    const float artHeight = (float)LayoutConfig::CardArtHeight * cardScale;
    const float inset = std::max(1.0f, std::round(renderScale));
    const Rectangle artRect = {
        inset,
        inset,
        (float)internalWidth - inset * 2.0f,
        artHeight - inset
    };

    if (const Image* artImage = getArtImage(card.getArtPath())) {
        drawCardArtNearest(&canvas, *artImage, artRect);
    } else {
        ImageDrawRectangleRec(&canvas, artRect, Colors::placeholder_art_bg);
        ImageDrawRectangleLines(&canvas, artRect, scaledInt(LayoutConfig::ThinBorderThickness, renderScale), Colors::light_bg);
    }

    if (hasBorderImage) {
        Rectangle src = { 0.0f, 0.0f, (float)m_borderImage.width, (float)m_borderImage.height };
        Rectangle dst = { 0.0f, 0.0f, (float)internalWidth, (float)internalHeight };
        ImageDraw(&canvas, m_borderImage, src, dst, WHITE);
    }

    const Font font = GetFontDefault();
    const Rectangle manaBox = sourceBoxToRect(LayoutConfig::CardManaBoxLeft,
                                              LayoutConfig::CardManaBoxTop,
                                              LayoutConfig::CardManaBoxRight,
                                              LayoutConfig::CardManaBoxBottom,
                                              internalWidth,
                                              internalHeight);
    const Rectangle nameBox = sourceBoxToRect(LayoutConfig::CardNameBoxLeft,
                                              LayoutConfig::CardNameBoxTop,
                                              LayoutConfig::CardNameBoxRight,
                                              LayoutConfig::CardNameBoxBottom,
                                              internalWidth,
                                              internalHeight);
    const Rectangle descBox = sourceBoxToRect(LayoutConfig::CardDescriptionBoxLeft,
                                              LayoutConfig::CardDescriptionBoxTop,
                                              LayoutConfig::CardDescriptionBoxRight,
                                              LayoutConfig::CardDescriptionBoxBottom,
                                              internalWidth,
                                              internalHeight);
    const float textBoxInset = (float)outlineThickness(renderScale);
    const float descInnerInset = (float)scaledInt(LayoutConfig::CardDescriptionInnerInset, cardScale);
    const float titleSafeWidth = std::max(1.0f, nameBox.width - textBoxInset * 2.0f);
    const float descSafeWidth = std::max(
        1.0f,
        descBox.width - textBoxInset * 2.0f - descInnerInset * 2.0f
            - std::ceil(cardScale * (crispPresentation ? 6.0f : 4.0f)));

    {
        const std::string costStr = std::to_string(card.getCost());
        const int fontSize = scaledInt(emphasized ? LayoutConfig::HoveredCardNameSize
                                                  : LayoutConfig::CardNameFontSize,
                                       cardScale);
        const Vector2 measure = MeasureTextEx(font, costStr.c_str(), (float)fontSize, 0.0f);
        const float x = std::round(manaBox.x + (manaBox.width - measure.x) * 0.5f);
        const float y = std::round(manaBox.y + (manaBox.height - (float)fontSize) * 0.5f);
        const Color color = affordable ? Colors::draw_pile_accent : Colors::damage_color;
        imageDrawTextOutlined(&canvas, font, costStr.c_str(), x, y, (float)fontSize, 0.0f, renderScale, color);
    }

    const int descriptionGap = scaledInt(LayoutConfig::CardDescriptionGap, cardScale);
    const int footerMargin = scaledInt(LayoutConfig::CardFooterMargin, cardScale);
    const int rightStatPadding = scaledInt(LayoutConfig::CardRightStatPadding, cardScale);

    const int nameSize = scaledInt(emphasized ? LayoutConfig::HoveredCardNameSize
                                              : LayoutConfig::CardNameFontSize,
                                   cardScale);
    const float nameSpacing = LayoutConfig::CardNameLetterSpacing * cardScale;
    const std::string fittedName = fitTextWithEllipsis(font, card.getName(), (float)nameSize, nameSpacing, titleSafeWidth);
    const Vector2 nameMeasure = MeasureTextEx(font, fittedName.c_str(), (float)nameSize, nameSpacing);
    const float nameX = std::round(nameBox.x + textBoxInset + std::max(0.0f, (titleSafeWidth - nameMeasure.x) * 0.5f));
    const float nameY = std::round(nameBox.y + std::max(0.0f, (nameBox.height - (float)nameSize) * 0.5f));
    imageDrawTextOutlined(&canvas, font, fittedName.c_str(), nameX, nameY, (float)nameSize, nameSpacing, renderScale, Colors::text_primary);

    const std::string& desc = card.getDescription();
    if (!desc.empty()) {
        const int descSize = scaledInt(LayoutConfig::CardDescriptionSize, cardScale);
        const float descSpacing = LayoutConfig::CardDescriptionLetterSpacing * cardScale;
        const int maxLinesByHeight = std::max(
            1,
            (int)std::floor((descBox.height + (float)descriptionGap) / ((float)descSize + (float)descriptionGap)));
        const auto lines = wrapTextToWidth(font,
                                           desc,
                                           (float)descSize,
                                           descSpacing,
                                           descSafeWidth,
                                           std::min(LayoutConfig::CardDescriptionLines, maxLinesByHeight));
        int textY = (int)std::round(descBox.y);
        for (const std::string& line : lines) {
            const std::string fittedLine = fitTextWithEllipsis(font, line, (float)descSize, descSpacing, descSafeWidth);
            ImageDrawTextEx(&canvas, font, fittedLine.c_str(), { descBox.x + textBoxInset + descInnerInset, (float)textY }, (float)descSize, descSpacing, BLACK);
            textY += descSize + descriptionGap;
        }
    }

    const int infoSize = scaledInt(emphasized ? LayoutConfig::HoveredCardFooterSize
                                              : LayoutConfig::CardFooterSize,
                                   cardScale);
    const int footerY = internalHeight - footerMargin;

    if (card.getDamageAmount() > 0) {
        const std::string text = std::to_string(card.getDamageAmount()) + " dmg";
        const Vector2 measure = MeasureTextEx(font, text.c_str(), (float)infoSize, 0.0f);
        const float x = (float)internalWidth - measure.x - (float)rightStatPadding;
        imageDrawTextOutlined(&canvas, font, text.c_str(), x, (float)footerY, (float)infoSize, 0.0f, renderScale, Colors::damage_color);
    }

    if (card.getBlockAmount() > 0) {
        const std::string text = std::to_string(card.getBlockAmount()) + " blk";
        const Vector2 measure = MeasureTextEx(font, text.c_str(), (float)infoSize, 0.0f);
        const float x = (float)internalWidth - measure.x - (float)rightStatPadding;
        imageDrawTextOutlined(&canvas, font, text.c_str(), x, (float)footerY, (float)infoSize, 0.0f, renderScale, Colors::block_color);
    }

    Texture2D texture = LoadTextureFromImage(canvas);
    UnloadImage(canvas);
    if (texture.id != 0) {
        if (crispPresentation) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        } else {
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
    }
    return texture;
}
