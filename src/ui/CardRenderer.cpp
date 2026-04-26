#include "CardRenderer.h"

#include "ui/FontUtils.h"
#include "ui/Colors.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

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
        std::round(left * sx),
        std::round(top * sy),
        std::max(1.0f, std::round((right - left) * sx)),
        std::max(1.0f, std::round((bottom - top) * sy))
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
    if (text.empty()) {
        return 0.0f;
    }

    float width = 0.0f;
    bool first = true;
    for (char ch : text) {
        if (!first) {
            width += spacing;
        }
        const std::string glyph(1, ch);
        width += MeasureTextEx(font, glyph.c_str(), fontSize, 0.0f).x;
        first = false;
    }
    return width;
}

void imageDrawTextTracked(Image* image,
                          Font font,
                          const std::string& text,
                          float x,
                          float y,
                          float fontSize,
                          float spacing,
                          Color color) {
    float cursorX = x;
    bool first = true;
    for (char ch : text) {
        if (!first) {
            cursorX += spacing;
        }
        const std::string glyph(1, ch);
        ImageDrawTextEx(image, font, glyph.c_str(), { cursorX, y }, fontSize, 0.0f, color);
        cursorX += MeasureTextEx(font, glyph.c_str(), fontSize, 0.0f).x;
        first = false;
    }
}

void imageDrawTextOutlinedTracked(Image* image,
                                  Font font,
                                  const std::string& text,
                                  float x,
                                  float y,
                                  float fontSize,
                                  float spacing,
                                  float renderScale,
                                  Color color) {
    const float o = (float)outlineThickness(renderScale);
    imageDrawTextTracked(image, font, text, x - o, y, fontSize, spacing, BLACK);
    imageDrawTextTracked(image, font, text, x + o, y, fontSize, spacing, BLACK);
    imageDrawTextTracked(image, font, text, x, y - o, fontSize, spacing, BLACK);
    imageDrawTextTracked(image, font, text, x, y + o, fontSize, spacing, BLACK);
    imageDrawTextTracked(image, font, text, x, y, fontSize, spacing, color);
}

void imageDrawTextClipped(Image* canvas,
                          Font font,
                          const std::string& text,
                          Rectangle clip,
                          float x,
                          float y,
                          float fontSize,
                          float spacing,
                          Color color) {
    const int clipWidth = std::max(1, (int)std::lround(clip.width));
    const int clipHeight = std::max(1, (int)std::lround(clip.height));
    Image layer = GenImageColor(clipWidth, clipHeight, BLANK);
    if (layer.data == nullptr) {
        return;
    }

    imageDrawTextTracked(&layer, font, text, x - clip.x, y - clip.y, fontSize, spacing, color);
    ImageDraw(canvas,
              layer,
              { 0.0f, 0.0f, (float)clipWidth, (float)clipHeight },
              { std::round(clip.x), std::round(clip.y), (float)clipWidth, (float)clipHeight },
              WHITE);
    UnloadImage(layer);
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

    return fitted.empty() ? ellipsis : fitted + ellipsis;
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
                    std::string remaining(1, ch);
                    for (std::size_t j = i + 1; j < words.size(); ++j) {
                        remaining += " " + words[j];
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

void drawImageNearest(Image* canvas, const Image& sourceImage, Rectangle dst) {
    Image resized = ImageCopy(sourceImage);
    if (resized.data == nullptr) {
        return;
    }

    const int targetWidth = std::max(1, (int)std::lround(dst.width));
    const int targetHeight = std::max(1, (int)std::lround(dst.height));
    ImageResizeNN(&resized, targetWidth, targetHeight);
    ImageDraw(canvas,
              resized,
              { 0.0f, 0.0f, (float)resized.width, (float)resized.height },
              { std::round(dst.x), std::round(dst.y), (float)targetWidth, (float)targetHeight },
              WHITE);
    UnloadImage(resized);
}

} // namespace

Texture2D CardRenderer::buildTexture(const Card& card,
                                     int targetWidth,
                                     int targetHeight,
                                     const CardRenderOptions& options,
                                     const CardRenderLayout& layout) {
    if (targetWidth <= 0 || targetHeight <= 0) {
        return {};
    }

    const bool staticMode = options.mode == CardRenderMode::Static;
    const float renderScale = staticMode ? 1.0f : LayoutConfig::CardFaceRenderScale;
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
    const Color bg = options.emphasized ? Colors::button_hover : Colors::card_bg;
    if (!hasBorderImage) {
        ImageDrawRectangleRec(&canvas, { 0.0f, 0.0f, (float)internalWidth, (float)internalHeight }, bg);
    }

    const Rectangle artRect = sourceBoxToRect(layout.artLeft, layout.artTop, layout.artRight, layout.artBottom,
                                              internalWidth, internalHeight);
    if (const Image* artImage = getArtImage(card.getArtPath())) {
        drawImageNearest(&canvas, *artImage, artRect);
    } else {
        ImageDrawRectangleRec(&canvas, artRect, Colors::placeholder_art_bg);
        ImageDrawRectangleLines(&canvas, artRect, scaledInt(LayoutConfig::ThinBorderThickness, renderScale), Colors::light_bg);
    }

    if (hasBorderImage) {
        drawImageNearest(&canvas, m_borderImage, { 0.0f, 0.0f, (float)internalWidth, (float)internalHeight });
    }

    const Font font = UiFont::getFont();
    const Rectangle manaBox = sourceBoxToRect(layout.manaLeft, layout.manaTop, layout.manaRight, layout.manaBottom,
                                              internalWidth, internalHeight);
    const Rectangle nameBox = sourceBoxToRect(layout.nameLeft, layout.nameTop, layout.nameRight, layout.nameBottom,
                                              internalWidth, internalHeight);
    const Rectangle descBox = sourceBoxToRect(layout.descriptionLeft, layout.descriptionTop,
                                              layout.descriptionRight, layout.descriptionBottom,
                                              internalWidth, internalHeight);
    const float textScale = cardScale * (staticMode ? LayoutConfig::CardStaticTextScale
                                                    : LayoutConfig::CardAnimatedTextScale);
    const float textBoxInset = (float)outlineThickness(renderScale);
    const float descInnerInset = (float)scaledInt(LayoutConfig::CardDescriptionInnerInset, textScale);
    const float descTopInset = (float)scaledInt(LayoutConfig::CardDescriptionTopInset, textScale);
    const Rectangle descClip = {
        descBox.x + textBoxInset,
        descBox.y + descTopInset,
        std::max(1.0f, descBox.width - textBoxInset * 2.0f),
        std::max(1.0f, descBox.height - descTopInset - textBoxInset)
    };
    const float titleSafeWidth = std::max(1.0f, nameBox.width - textBoxInset * 2.0f);
    const float descSafeWidth = std::max(1.0f, descClip.width - descInnerInset * 2.0f);
    const float spacingScale = staticMode ? LayoutConfig::CardStaticLetterSpacingScale : 1.0f;
    float nameSpacing = LayoutConfig::CardNameLetterSpacing * textScale * spacingScale;
    float descSpacing = LayoutConfig::CardDescriptionLetterSpacing * textScale * spacingScale;
    if (staticMode) {
        nameSpacing = std::max(nameSpacing, LayoutConfig::CardStaticMinNameTracking * textScale);
        descSpacing = std::max(descSpacing, LayoutConfig::CardStaticMinDescriptionTracking * textScale);
    } else {
        nameSpacing = std::max(nameSpacing, LayoutConfig::CardAnimatedMinNameTracking * textScale);
        descSpacing = std::max(descSpacing, LayoutConfig::CardAnimatedMinDescriptionTracking * textScale);
    }

    {
        const int fontSize = scaledInt(options.emphasized ? LayoutConfig::HoveredCardNameSize
                                                          : LayoutConfig::CardNameFontSize,
                                       cardScale);
        const Vector2 measure = MeasureTextEx(font, options.visibleCostText.c_str(), (float)fontSize, 0.0f);
        const float x = std::round(manaBox.x + (manaBox.width - measure.x) * 0.5f);
        const float y = std::round(manaBox.y + (manaBox.height - (float)fontSize) * 0.5f);
        const Color color = options.affordable ? Colors::draw_pile_accent : Colors::damage_color;
        imageDrawTextOutlined(&canvas, font, options.visibleCostText.c_str(), x, y, (float)fontSize, 0.0f, renderScale, color);
    }

    const int footerMargin = scaledInt(LayoutConfig::CardFooterMargin, cardScale);
    const int rightStatPadding = scaledInt(LayoutConfig::CardRightStatPadding, cardScale);

    const int fontBonus = options.emphasized
        ? LayoutConfig::CardHoveredFontBonus
        : (staticMode ? LayoutConfig::CardStaticFontBonus : 0);
    const int baseNameSize = (options.emphasized ? LayoutConfig::HoveredCardNameSize
                                                 : LayoutConfig::CardNameFontSize) + fontBonus;
    const int baseDescriptionSize = LayoutConfig::CardDescriptionSize + fontBonus;
    const int nameSize = scaledInt(baseNameSize, textScale);
    const std::string fittedName = fitTextWithEllipsis(font, card.getDisplayName(), (float)nameSize, nameSpacing, titleSafeWidth);
    const float fittedNameWidth = measureTextWidth(font, fittedName, (float)nameSize, nameSpacing);
    const float nameX = std::round(nameBox.x + textBoxInset + std::max(0.0f, (titleSafeWidth - fittedNameWidth) * 0.5f));
    const float nameY = std::round(nameBox.y + std::max(0.0f, (nameBox.height - (float)nameSize) * 0.5f));
    imageDrawTextOutlinedTracked(&canvas, font, fittedName, nameX, nameY, (float)nameSize, nameSpacing, renderScale, Colors::text_primary);

    const std::string& desc = card.getDisplayDescription();
    if (!desc.empty()) {
        const int descSize = scaledInt(baseDescriptionSize, textScale);
        const int descriptionGap = scaledInt(LayoutConfig::CardDescriptionGap, textScale);
        const int maxLinesByHeight = std::max(
            1,
            (int)std::floor((descClip.height + (float)descriptionGap)
                            / ((float)descSize + (float)descriptionGap)));
        const auto lines = wrapTextToWidth(font, desc, (float)descSize, descSpacing, descSafeWidth, maxLinesByHeight);
        int textY = (int)std::round(descClip.y);
        for (const std::string& line : lines) {
            const std::string fittedLine = fitTextWithEllipsis(font, line, (float)descSize, descSpacing, descSafeWidth);
            imageDrawTextClipped(&canvas,
                                 font,
                                 fittedLine,
                                 descClip,
                                 std::round(descClip.x + descInnerInset),
                                 (float)textY,
                                 (float)descSize,
                                 descSpacing,
                                 BLACK);
            textY += descSize + descriptionGap;
        }
    }

    const int infoSize = scaledInt(options.emphasized ? LayoutConfig::HoveredCardFooterSize
                                                      : LayoutConfig::CardFooterSize,
                                   cardScale);
    const int footerY = internalHeight - footerMargin;
    if (!card.shouldHideFooterStats() && card.getDamageAmount() > 0) {
        const std::string text = std::to_string(card.getDamageAmount()) + " dmg";
        const Vector2 measure = MeasureTextEx(font, text.c_str(), (float)infoSize, 0.0f);
        imageDrawTextOutlined(&canvas, font, text.c_str(), (float)internalWidth - measure.x - (float)rightStatPadding,
                              (float)footerY, (float)infoSize, 0.0f, renderScale, Colors::damage_color);
    }
    if (!card.shouldHideFooterStats() && card.getBlockAmount() > 0) {
        const std::string text = std::to_string(card.getBlockAmount()) + " blk";
        const Vector2 measure = MeasureTextEx(font, text.c_str(), (float)infoSize, 0.0f);
        imageDrawTextOutlined(&canvas, font, text.c_str(), (float)internalWidth - measure.x - (float)rightStatPadding,
                              (float)footerY, (float)infoSize, 0.0f, renderScale, Colors::block_color);
    }

    Texture2D texture = LoadTextureFromImage(canvas);
    UnloadImage(canvas);
    if (texture.id != 0) {
        if (staticMode) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        } else {
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
    }
    return texture;
}

void CardRenderer::unloadAssets() {
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

bool CardRenderer::ensureBorderLoaded() {
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

const Image* CardRenderer::getArtImage(const std::string& path) {
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
